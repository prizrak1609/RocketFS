#include "filesystem.h"
#include <QDebug>
#include "fuse/fuse.h"
#include "getattrcmd.h"
#include "readdircmd.h"
#include <QJsonDocument>
#include <QJsonArray>
#include "mkdircmd.h"
#include "rmdircmd.h"
#include "renamecmd.h"
#include "createfilecmd.h"
#include "rmfilecmd.h"
#include "openfilecmd.h"
#include "readfilecmd.h"
#include "writefilecmd.h"
#include "closefilecmd.h"

Filesystem* Filesystem::instance = nullptr;

extern int fuse_exited(struct fuse *f);

Filesystem::Filesystem(QObject *parent) : QThread{parent}
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
    setTerminationEnabled(true);
}

Filesystem::~Filesystem()
{
    quit();
}

Filesystem* Filesystem::get_instance()
{
    static std::once_flag flag;
    std::call_once(flag, [&]{
        Filesystem::instance = new Filesystem();
    });
    return Filesystem::instance;
}

void* Filesystem::init(fuse3_conn_info *conn, fuse3_config *conf)
{
    conn->capable |= FUSE_CAP_CASE_INSENSITIVE | FUSE_READDIR_PLUS;
    return nullptr;
}

int Filesystem::get_attr(const char *path, fuse_stat *stbuf, fuse_file_info *fi)
{
    if(file_attributes.contains(path))
    {
        convert_to_stat(file_attributes[path], stbuf);
        return 0;
    }

    GetAttrCmd command(path);
    send(command).then(QtFuture::Launch::Sync, [stbuf, this, path](QString message){
            file_attributes.insert(path, message);
            convert_to_stat(message, stbuf);
        }).waitForFinished();
    return 0;
}

int Filesystem::read_dir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off, fuse_file_info *fi, fuse_readdir_flags flags)
{
    ReadDirCmd command(path);
    send(command).then([buf, this, filler, &path](QString message){
            if(message.isEmpty())
            {
                return;
            }
            QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
            QJsonArray array = doc.array();
            for(QJsonValueRef item : array)
            {
                QJsonObject obj = item.toObject();
                QString file_name = obj["file_name"].toString();
                fuse_stat* stats = new fuse_stat();
                QString attributes = QString(QJsonDocument(obj["stats"].toObject()).toJson(QJsonDocument::Compact));
                convert_to_stat(attributes, stats);

                QString full_path = path;
                if(!full_path.endsWith("/"))
                {
                    full_path += "/";
                }
                full_path += file_name;
                file_attributes.insert(full_path, attributes);

                if(filler(buf, file_name.toStdString().c_str(), stats, 0, FUSE_FILL_DIR_PLUS) == 1) // buffer is full
                {
                    qDebug() << "buffer is full";
                    emit error("read_dir: buffer is full");
                    break;
                }
            }
        }).waitForFinished();
    return 0;
}

int Filesystem::mkdir(const char *path, fuse_mode_t mode)
{
    MkDirCmd command(path);
    send(command);
    return 0;
}

int Filesystem::rmdir(const char *path)
{
    RmDirCmd command(path);
    send(command);
    return 0;
}

int Filesystem::rename(const char *oldpath, const char *newpath, unsigned int flags)
{
    RenameCmd command(oldpath, newpath);
    send(command);
    return 0;
}

int Filesystem::create_file(const char *path, fuse_mode_t mode, fuse_file_info *fi)
{
    CreateFileCmd command(path);
    send(command);
    return 0;
}

int Filesystem::remove_file(const char *path)
{
    RmFileCmd command(path);
    send(command);
    return 0;
}

int Filesystem::open_file(const char *path, fuse_file_info *fi)
{
    OpenFileCmd command(path);
    send(command).waitForFinished();
    return 0;
}

int Filesystem::read_file(const char *path, char *buf, size_t size, fuse_off_t off, fuse_file_info *fi)
{
    QString file_path = cache_folder;
    file_path += "\\" + QString(path).replace("/", "_");

    QFile file(file_path);

    if(written_ofsets.contains(file_path) && file.exists())
    {
        QMap<fuse_off_t, size_t>* meta = written_ofsets.value(file_path);
        if(meta->contains(off))
        {
            size_t chunk_size = meta->value(off);
            if(chunk_size >= size)
            {
                file.open(QFile::ReadOnly);
                file.seek(off);
                QByteArray bytes = file.read(size);
                file.close();
                std::string data = bytes.toStdString();
                size_t actual_size = std::min(data.size(), size);
                memcpy(buf, data.c_str(), actual_size);
                return 0;
            }
        }
        meta->insert(off, size);
    } else
    {
        QMap<fuse_off_t, size_t>* meta = new QMap<fuse_off_t, size_t>();
        meta->insert(off, size);
        written_ofsets.insert(file_path, meta);
    }

    ReadFileCmd command(path, size, off);
    int read_bytes = 0;
    send(command).then(QtFuture::Launch::Sync, [buf, size, off, &file, &read_bytes](QString message){
        if(message.isEmpty())
        {
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        QJsonObject item = doc.object();
        QByteArray bytes = QByteArray::fromBase64(item["chunk"].toString().toUtf8());

        file.open(QFile::WriteOnly);
        file.seek(off);
        file.write(bytes);

        std::string data = bytes.toStdString();
        size_t actual_size = std::min(data.size(), size);
        read_bytes = actual_size;
        memcpy(buf, data.c_str(), actual_size);
        file.close();
    }).waitForFinished();
    return read_bytes;
}

int Filesystem::write_file(const char *path, const char *buf, size_t size, fuse_off_t off, fuse_file_info *fi)
{
    WriteFileCmd command(path, QString(buf).toUtf8().toBase64(), size, off);
    send(command).waitForFinished();
    return 0;
}

int Filesystem::close_file(const char *path, fuse_file_info *fi)
{
    CloseFileCmd command(path);
    send(command);
    return 0;
}

void Filesystem::convert_to_stat(QString message, fuse_stat *stbuf)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    stbuf->st_dev = doc["st_dev"].toInteger();
    stbuf->st_ino = doc["st_ino"].toInteger();
    stbuf->st_mode = doc["st_mode"].toInteger();
    stbuf->st_nlink = doc["st_nlink"].toInteger();
    stbuf->st_rdev = doc["st_rdev"].toInteger();
    stbuf->st_size = doc["st_size"].toInteger();
    stbuf->st_blksize = doc["st_blksize"].toInteger();
    stbuf->st_blocks = doc["st_blocks"].toInteger();

    fuse_timespec atime;
    atime.tv_sec = doc["st_atime_tv_sec"].toInteger();
    atime.tv_nsec = doc["st_atime_tv_nsec"].toInteger();
    stbuf->st_atim = atime;

    fuse_timespec mtime;
    mtime.tv_sec = doc["st_mtim_tv_sec"].toInteger();
    mtime.tv_nsec = doc["st_mtim_tv_nsec"].toInteger();
    stbuf->st_mtim = mtime;

    fuse_timespec ctime;
    ctime.tv_sec = doc["st_ctim_tv_sec"].toInteger();
    ctime.tv_nsec = doc["st_ctim_tv_nsec"].toInteger();
    stbuf->st_ctim = ctime;

    fuse3_context* context = fuse3_get_context();
    stbuf->st_uid = context->uid;
    stbuf->st_gid = context->gid;
}

QFuture<QString> Filesystem::send(ICommand& command)
{
    Connection* conn = pool->get_connection();
    connect(this, &Filesystem::request, conn, &Connection::send, Qt::SingleShotConnection);

    QFuture<QString> result = QtFuture::connect(conn, &Connection::response);

    emit request(command.to_json());
    return result;
}
