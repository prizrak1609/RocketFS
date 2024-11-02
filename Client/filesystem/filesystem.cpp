#include "filesystem.h"
#include <QDebug>
#include "fuse/fuse.h"
#include "command/getattrcmd.h"
#include "command/readdircmd.h"
#include <QJsonDocument>
#include <QJsonArray>
#include "command/mkdircmd.h"
#include "command/rmdircmd.h"
#include "command/renamecmd.h"
#include "command/createfilecmd.h"
#include "command/rmfilecmd.h"
#include "command/readfilecmd.h"
#include "command/writefilecmd.h"
#include "command/statfscmd.h"
#include "connection_pool.h"

#define print_function static unsigned long int number = 0; \
qDebug() << number << __FUNCTION__ << "" << path; \
number++;

using namespace WebSocket;

Filesystem::ptr Filesystem::instance = {};

constexpr const char* kFileAttributesName = "file_attributes.json";

Filesystem::Filesystem(QObject *parent) : QThread{parent}
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);

    // std::memset(&filesystem_stat, 0, sizeof(decltype(filesystem_stat)));
}

Filesystem::~Filesystem()
{
    // for (OpenedFile& pair : opened_files.values())
    // {
    //     delete pair.mutex;
    // }

    quit();
}

Filesystem::ptr& Filesystem::get_instance()
{
    static std::once_flag flag;
    std::call_once(flag, [&]{
        Filesystem::instance.reset(new Filesystem());
    });
    return Filesystem::instance;
}

void Filesystem::init(FSP_SERVICE *service, ULONG argc, PWSTR *argv)
{
    // cache.setPath(cache_folder);
    // cache.mkpath(cache_folder);

    // QJsonObject attributes_obj;
    // QFile attributes_file(kFileAttributesName);
    // if(attributes_file.exists())
    // {
    //     attributes_file.open(QFile::ReadOnly);

    //     QByteArray body = attributes_file.readAll();
    //     attributes_file.close();

    //     attributes_obj = QJsonDocument::fromJson(body).object();

    //     for(const QString& key : attributes_obj.keys())
    //     {
    //         file_attributes.insert(key, attributes_obj.value(key).toString());
    //     }
    // }
}

void Filesystem::GetVolumeInfo(FSP_FSCTL_VOLUME_INFO *VolumeInfo)
{
    // stat_fs
    StatFSCmd command("/", this);

    Connection_pool::get_instance()->send_text(command).then(QtFuture::Launch::Sync, [VolumeInfo](QString message) {
                                                           QJsonDocument json = QJsonDocument::fromJson(message.toUtf8());
                                                           VolumeInfo->FreeSize = json["free_size"].toInteger();
                                                           VolumeInfo->TotalSize = json["block_size"].toInteger() * json["blocks_count"].toInteger();
    }).waitForFinished();
}

void Filesystem::ReadFile(PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, PULONG PBytesTransferred)
{
    // read_file
}

void Filesystem::WriteFile(PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, BOOLEAN WriteToEndOfFile, BOOLEAN ConstrainedIo, PULONG PBytesTransferred, FSP_FSCTL_FILE_INFO *FileInfo)
{
    // write_file
}

void Filesystem::GetFileInfo(PVOID FileContext, FSP_FSCTL_FILE_INFO *FileInfo)
{
    // getattr
}

static BOOLEAN AddDirInfo(PVOID FileNode, PWSTR FileName,
                          PVOID Buffer, ULONG Length, PULONG PBytesTransferred)
{
    UINT8 DirInfoBuf[sizeof(FSP_FSCTL_DIR_INFO) + sizeof FileNode->FileName];
    FSP_FSCTL_DIR_INFO *DirInfo = (FSP_FSCTL_DIR_INFO *)DirInfoBuf;
    WCHAR Root[2] = L"\\";
    PWSTR Remain, Suffix;

    if (0 == FileName)
    {
        FspPathSuffix(FileNode->FileName, &Remain, &Suffix, Root);
        FileName = Suffix;
        FspPathCombine(FileNode->FileName, Suffix);
    }

    memset(DirInfo->Padding, 0, sizeof DirInfo->Padding);
    DirInfo->Size = (UINT16)(sizeof(FSP_FSCTL_DIR_INFO) + wcslen(FileName) * sizeof(WCHAR));
    DirInfo->FileInfo = FileNode->FileInfo;
    memcpy(DirInfo->FileNameBuf, FileName, DirInfo->Size - sizeof(FSP_FSCTL_DIR_INFO));

    return FspFileSystemAddDirInfo(DirInfo, Buffer, Length, PBytesTransferred);
}

void Filesystem::ReadDirectory(PVOID FileContext, PWSTR Pattern, PWSTR Marker, PVOID Buffer, ULONG Length, PULONG PBytesTransferred)
{
    AddDirInfo(FileContext, L".", Buffer, Length, PBytesTransferred);
    // readdir
}

void Filesystem::GetDirInfoByName(PVOID FileContext, PWSTR FileName, FSP_FSCTL_DIR_INFO *DirInfo)
{
    // getattr
}

void Filesystem::RemoveFile(PVOID FileContext, PWSTR FileName)
{
    // remove_file
}

void Filesystem::RemoveDir(PVOID FileContext, PWSTR FileName)
{
    // rmdir
}

void Filesystem::Open(PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo)
{
}

void Filesystem::Close(PVOID FileContext)
{
}

void Filesystem::destroy(FSP_SERVICE *service)
{
    // QJsonObject attributes_obj;
    // for(const QString& key : file_attributes.keys())
    // {
    //     attributes_obj[key] = file_attributes.value(key);
    // }

    // QFile attributes_file(kFileAttributesName);
    // if(attributes_file.exists())
    // {
    //     attributes_file.remove();
    // }
    // attributes_file.open(QFile::WriteOnly);
    // attributes_file.write(QJsonDocument(attributes_obj).toJson());
    // attributes_file.close();
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

    // fuse3_context* context = fuse3_get_context();
    // stbuf->st_uid = context->uid;
    // stbuf->st_gid = context->gid;
}
