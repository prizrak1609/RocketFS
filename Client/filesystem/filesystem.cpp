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
    conn->capable |= FUSE_CAP_CASE_INSENSITIVE | FUSE_READDIR_PLUS | FUSE_CAP_ASYNC_DIO | FUSE_CAP_ASYNC_READ;

    QJsonObject attributes_obj;

    QFile attributes_file("file_attributes.json");
    if(attributes_file.exists())
    {
        attributes_file.open(QFile::ReadOnly);

        QByteArray body = attributes_file.readAll();
        attributes_file.close();

        attributes_obj = QJsonDocument::fromJson(body).object();

        for(const QString& key : attributes_obj.keys())
        {
            file_attributes.insert(key, attributes_obj.value(key).toString());
        }
    }

    return nullptr;
}

void Filesystem::destroy(void *data)
{
    QJsonObject attributes_obj;

    for(const QString& key : file_attributes.keys())
    {
        attributes_obj[key] = file_attributes.value(key);
    }

    QFile attributes_file("file_attributes.json");
    if(attributes_file.exists())
    {
        attributes_file.remove();
    }
    attributes_file.open(QFile::WriteOnly);
    attributes_file.write(QJsonDocument(attributes_obj).toJson());
    attributes_file.close();
}

int Filesystem::get_attr(const char *path, fuse_stat *stbuf, fuse_file_info *fi)
{
    if(file_attributes.contains(path))
    {
        convert_to_stat(file_attributes.value(path), stbuf);
        return 0;
    }

    GetAttrCmd command(path);
    int result = 0;
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).then(QtFuture::Launch::Sync, [&result, stbuf, this, path](QString message){
            if(message.isEmpty())
            {
                                    if(print_logs)
                qDebug() << "receive result none";
                result = -ENOENT;
                return;
            }
            if(print_logs)
            qDebug() << "receive result" << message;
            file_attributes.insert(path, message);
            convert_to_stat(message, stbuf);
        }).waitForFinished();
    return result;
}

int Filesystem::read_dir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off, fuse_file_info *fi, fuse_readdir_flags flags)
{
    ReadDirCmd command(path);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).then([buf, this, filler, &path](QString message){
            if(message.isEmpty())
            {
                return;
            }
            if(print_logs)
            qDebug() << "receive result" << message;
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
                    if(print_logs)
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
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result mkdir";
    return 0;
}

int Filesystem::rmdir(const char *path)
{
    if(file_attributes.contains(path))
    {
        file_attributes.remove(path);
    }
    RmDirCmd command(path);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result rmdir";
    return 0;
}

int Filesystem::rename(const char *oldpath, const char *newpath, unsigned int flags)
{
    QString attr = file_attributes.value(oldpath);
    file_attributes.remove(oldpath);
    file_attributes.insert(newpath, attr);

    if (opened_files.contains(oldpath))
    {
        OpenedFile file = opened_files.value(oldpath);
        opened_files.remove(oldpath);
        opened_files.insert(newpath, file);
    }

    RenameCmd command(oldpath, newpath);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result rename";
    return 0;
}

int Filesystem::create_file(const char *path, fuse_mode_t mode, fuse_file_info *fi)
{
    CreateFileCmd command(path);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result create_file";
    return 0;
}

int Filesystem::remove_file(const char *path)
{
    if(file_attributes.contains(path))
    {
        file_attributes.remove(path);
    }
    if (opened_files.contains(path))
    {
        OpenedFile file = opened_files.value(path);
        opened_files.remove(path);
        delete file.mutex;
    }
    RmFileCmd command(path);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result remove_file";
    return 0;
}

int Filesystem::open_file(const char *path, fuse_file_info *fi)
{
    qDebug() << "open file in thread " << QThread::currentThreadId();
    if (opened_files.contains(path))
    {
        OpenedFile file = opened_files.value(path);
        file.links++;
        opened_files[path] = file;
    } else
    {
        OpenedFile file;
        file.links = 1;
        opened_files.insert(path, file);
    }

    OpenFileCmd command(path);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result open_file";
    return 0;
}

int Filesystem::read_file(const char *path, char *buf, size_t size, fuse_off_t off, fuse_file_info *fi)
{
    qDebug() << "read file in thread " << QThread::currentThreadId();
    QMutex* mutex = nullptr;
    if (opened_files.contains(path))
    {
        mutex = opened_files.value(path).mutex;
    }

    QMutexLocker<QMutex> lock(mutex);

//    QString file_path = cache_path(path);

//    if(file_path.endsWith("folder.jpg") || file_path.endsWith("desktop.ini"))
//    {
//        return -ENOENT;
//    }

    memset(buf, 0, size);

    int read_bytes = 0;

//    QFile file(file_path);
//    if (file.exists())
//    {
//        file.seek(off);
//        QByteArray message = file.read(size);
//        size_t source_size = size;
//        if (message.size() < size)
//        {
//            source_size = message.size();
//        }

//        read_bytes = source_size;

//        memcpy_s(buf, size, message, source_size);
//        return 0;
//    }

    ReadFileCmd command(path, size, off);

    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_binary(command).then(QtFuture::Launch::Sync, [buf, size, &read_bytes, this](QByteArray message){
        if(message.isEmpty())
        {
            read_bytes = -ENOENT;
            return;
        }
        if(print_logs)
        qDebug() << "receive result" << message.size();

        size_t source_size = size;
        if (message.size() < size)
        {
            source_size = message.size();
        }

        read_bytes = source_size;

        memcpy_s(buf, size, message, source_size);
    }).waitForFinished();
    return read_bytes;
}

int Filesystem::write_file(const char *path, const char *buf, size_t size, fuse_off_t off, fuse_file_info *fi)
{
    qDebug() << "write file in thread " << QThread::currentThreadId();
    QMutex* mutex = nullptr;
    if (opened_files.contains(path))
    {
        mutex = opened_files.value(path).mutex;
    }

    QMutexLocker<QMutex> lock(mutex);

//    QString file_path = cache_path(path);

//    QFile file(file_path);
//    if(!file.exists())
//    {
//        file.open(QFile::WriteOnly);
//        file.close();
//    }

//    file.open(QFile::WriteOnly);
//    file.seek(off);
//    file.write(buf);
//    file.close();

    WriteFileCmd command(path, QString(buf).toUtf8().toBase64(), size, off);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result write_file";
    return 0;
}

int Filesystem::close_file(const char *path, fuse_file_info *fi)
{
    qDebug() << "close file in thread " << QThread::currentThreadId();
    QMutex* mutex = nullptr;
    if (opened_files.contains(path))
    {
        OpenedFile file = opened_files.value(path);
        file.links--;
        if(file.links == 0)
        {
            opened_files.remove(path);
        } else
        {
            mutex = file.mutex;
            opened_files[path] = file;
        }
    }

    QMutexLocker<QMutex> lock(mutex);

    CloseFileCmd command(path);
    if(print_logs)
    qDebug() << "send command" << command.to_json();
    pool->send_text(command).waitForFinished();
    if(print_logs)
    qDebug() << "receive result close_file";
    return 0;
}

QString Filesystem::cache_path(const char* path)
{
    QString file_path = cache_folder;
    file_path += "\\" + QString(path).replace("/", "_").replace(" ", "_");
    return file_path;
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
