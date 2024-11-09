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
#include "sddl.h"
#include <QMutexLocker>

#include <ranges>

#define print_function static unsigned long int number = 0; \
number++; \
qDebug() << number << __FUNCTION__

using namespace WebSocket;

Filesystem::ptr Filesystem::instance = {};

constexpr const char* kFileAttributesName = "file_attributes.json";

Filesystem::Filesystem(QObject *parent) : QThread{parent}
{
    print_function;
    connect(this, &QThread::finished, this, &QObject::deleteLater);

    // std::memset(&filesystem_stat, 0, sizeof(decltype(filesystem_stat)));
}

Filesystem::~Filesystem()
{
    print_function;
    // for (OpenedFile& pair : opened_files.values())
    // {
    //     delete pair.mutex;
    // }

    if (volumeInfo != nullptr) {
        delete volumeInfo;
    }

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
    print_function;
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
    print_function;

    if (volumeInfo != nullptr) {
        *VolumeInfo = *volumeInfo;
        return;
    }

    volumeInfo = new FSP_FSCTL_VOLUME_INFO();
    volumeInfo->FreeSize = 99999999999;
    volumeInfo->TotalSize = 9999999999999;

    volumeInfo->VolumeLabelLength = sizeof L"AIRFS" - sizeof(WCHAR);
    memcpy(volumeInfo->VolumeLabel, L"AIRFS", volumeInfo->VolumeLabelLength);

    // stat_fs
    StatFSCmd command("/");

    Connection_pool::get_instance()->send_text(command)
        .then(QtFuture::Launch::Sync, [VolumeInfo, this](QString message) {
                                                           QJsonDocument json = QJsonDocument::fromJson(message.toUtf8());

                                                           volumeInfo->FreeSize = json["free_size"].toInteger();
                                                           volumeInfo->TotalSize = json["block_size"].toInteger() * json["blocks_count"].toInteger();

                                                           *VolumeInfo = *volumeInfo;
                                                       }).waitForFinished();
}

void Filesystem::ReadFile(PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, PULONG PBytesTransferred)
{
    print_function << " " << FileContext;
    // read_file
}

void Filesystem::WriteFile(PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, BOOLEAN WriteToEndOfFile, BOOLEAN ConstrainedIo, PULONG PBytesTransferred, FSP_FSCTL_FILE_INFO *FileInfo)
{
    print_function << " " << FileContext;
    // write_file
}

void Filesystem::GetFileInfo(PVOID FileContext, FSP_FSCTL_FILE_INFO *FileInfo)
{
    print_function << " " << FileContext;
    // getattr
}

BOOLEAN Filesystem::AddDirInfo(QString fileName, FSP_FSCTL_FILE_INFO fileInfo, PVOID Buffer, ULONG Length, PULONG PBytesTransferred)
{
    print_function << " " << fileName;

    UINT8* DirInfoBuf = new UINT8[sizeof(FSP_FSCTL_DIR_INFO) + fileName.size() * sizeof(WCHAR)];
    FSP_FSCTL_DIR_INFO *DirInfo = (FSP_FSCTL_DIR_INFO *)DirInfoBuf;

    memset(DirInfo->Padding, 0, sizeof(DirInfo->Padding));

    DirInfo->Size = (UINT16)(sizeof(FSP_FSCTL_DIR_INFO) + fileName.size() * sizeof(WCHAR));
    DirInfo->FileInfo = fileInfo;

    WCHAR* array = new WCHAR[fileName.size() * sizeof(WCHAR)];
    fileName.toWCharArray(array);
    memcpy(DirInfo->FileNameBuf, array, fileName.size() * sizeof(WCHAR));

    auto Result = FspFileSystemAddDirInfo(DirInfo, Buffer, Length, PBytesTransferred);

    delete[] DirInfoBuf;
    delete[] array;

    return Result;
}

void Filesystem::ReadDirectory(PVOID FileContext, PWSTR Pattern, PWSTR Marker, PVOID Buffer, ULONG Length, PULONG PBytesTransferred)
{
    auto item = reinterpret_cast<OpenedItem*>(FileContext);

    QMutexLocker lock(&(item->lock));

    print_function << " " << FileContext << item->name << QThread::currentThreadId();

    ReadDirCmd cmd(item->real_name, this);

    Connection_pool::get_instance()->send_text(cmd)
        .then(QtFuture::Launch::Sync, [=, this](QString message) {
                                                        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

                                                        for (auto item : doc.array()) {
                                                            QJsonObject json = item.toObject();
                                                            QString name = json["file_name"].toString();

                                                            FSP_FSCTL_FILE_INFO stats;
                                                            convert_to_stat(json["stats"].toString(), stats);

                                                            AddDirInfo(name, stats, Buffer, Length, PBytesTransferred);
                                                        }

                                                        FspFileSystemAddDirInfo(nullptr, Buffer, Length, PBytesTransferred);
                                                   }).waitForFinished();
}

void Filesystem::GetDirInfoByName(PVOID FileContext, PWSTR FileName, FSP_FSCTL_DIR_INFO *DirInfo)
{
    print_function << " " << FileContext << " " << QString::fromWCharArray(FileName);
    // getattr
}

void Filesystem::RemoveFile(PVOID FileContext, PWSTR FileName)
{
    print_function << " " << FileContext << " " << QString::fromWCharArray(FileName);
    // remove_file
}

void Filesystem::RemoveDir(PVOID FileContext, PWSTR FileName)
{
    print_function << " " << FileContext << " " << QString::fromWCharArray(FileName);
    // rmdir
}

void Filesystem::OpenedItem::copyInfo(FSP_FSCTL_FILE_INFO* other) {
    info.FileAttributes = other->FileAttributes;
    info.ReparseTag = other->ReparseTag;
    info.AllocationSize = other->AllocationSize;
    info.FileSize = other->FileSize;
    info.CreationTime = other->CreationTime;
    info.LastAccessTime = other->LastAccessTime;
    info.LastWriteTime = other->LastWriteTime;
    info.ChangeTime = other->ChangeTime;
    info.IndexNumber = other->IndexNumber;
    info.HardLinks = other->HardLinks;
    info.EaSize = other->EaSize;
}

static inline UINT64 MemfsGetSystemTime(VOID)
{
    FILETIME FileTime;
    GetSystemTimeAsFileTime(&FileTime);
    return ((PLARGE_INTEGER)&FileTime)->QuadPart;
}

NTSTATUS Filesystem::Open(PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo)
{
    if (QString::fromWCharArray(FileName).contains("desktop.ini")) {
        return STATUS_NOT_FOUND;
    }
    auto iterator = std::ranges::find_if(opened_items, [FileName](const OpenedItem* item) -> bool {
        return item->name == QString::fromWCharArray(FileName);
    });

    if (iterator != opened_items.end()) {
        (*iterator)->links++;

        *PFileContext = *iterator;
        // set iterator->info to FileInfo
    } else {
        auto item = new OpenedItem();
        item->name = QString::fromWCharArray(FileName);
        item->real_name = item->name;
        item->real_name.replace("\\", "/");
        // set FileInfo

        *PFileContext = item;

        opened_items.insert(item);
    }

    FileInfo->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    FileInfo->AllocationSize = FileInfo->FileSize = 999;
    FileInfo->ChangeTime = FileInfo->CreationTime = FileInfo->LastAccessTime = FileInfo->LastWriteTime = MemfsGetSystemTime();
    FileInfo->IndexNumber = 0;
    FileInfo->HardLinks = 0;
    FileInfo->EaSize = 0;

    print_function << " " << QString::fromWCharArray(FileName) << " " << *PFileContext;
    return STATUS_SUCCESS;
}

NTSTATUS Filesystem::GetSecurityByName(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, PUINT32 PFileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize)
{
    if (QString::fromWCharArray(FileName).contains("desktop.ini")) {
        return STATUS_NOT_FOUND;
    }
    *PFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    if (this->SecurityDescriptor == nullptr)
    {
        LPCTSTR              SACL = TEXT("O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)");
            /*TEXT("D:")                   // Discretionary ACL
            TEXT("(A;OICI;GA;;;BG)")     // Allow access to
            // built-in guests
            TEXT("(A;OICI;GA;;;AN)")     // Allow access to
            // anonymous logon
            TEXT("(A;OICI;GA;;;AU)")     // Allow
            // read/write/execute
            // to authenticated
            // users
            TEXT("(A;OICI;GA;;;BA)");*/    // Allow full control
        // to administrators
        ConvertStringSecurityDescriptorToSecurityDescriptor(SACL, SDDL_REVISION, &(this->SecurityDescriptor), &(this->SecurityDescriptorSize));
        qDebug() << __FUNCTION__ << " error " << FspNtStatusFromWin32(GetLastError());
    }

    if (PSecurityDescriptorSize)
    {
        if (SecurityDescriptorSize > *PSecurityDescriptorSize)
        {
            *PSecurityDescriptorSize = SecurityDescriptorSize;
            qDebug() << __FUNCTION__ << " overflow";
            return STATUS_BUFFER_OVERFLOW;
        }

        *PSecurityDescriptorSize = SecurityDescriptorSize;

        if (SecurityDescriptor)
            memcpy(SecurityDescriptor, this->SecurityDescriptor, SecurityDescriptorSize);
    }

    print_function << QString::fromWCharArray(FileName) << " " << SecurityDescriptor;

    return STATUS_SUCCESS;
}

void Filesystem::Close(PVOID FileContext)
{
    print_function << " " << FileContext;
    auto item = reinterpret_cast<OpenedItem*>(FileContext);

    item->links--;

    if (item->links <= 0) {
        opened_items.remove(reinterpret_cast<OpenedItem*>(FileContext));

        delete reinterpret_cast<OpenedItem*>(FileContext);

        qDebug() << __FUNCTION__ << " unregister";
    }
}

void Filesystem::destroy(FSP_SERVICE *service)
{
    print_function;
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

void Filesystem::convert_to_stat(QString message, FSP_FSCTL_FILE_INFO &stbuf)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    stbuf.AllocationSize = doc["st_size"].toInteger();
    stbuf.FileAttributes = doc["st_mode"].toInteger();
    stbuf.FileSize = stbuf.AllocationSize;
    stbuf.CreationTime = doc["st_ctim_tv_sec"].toInteger();
    stbuf.LastAccessTime = doc["st_atime_tv_sec"].toInteger();
    stbuf.LastWriteTime = doc["st_mtim_tv_sec"].toInteger();
    stbuf.ChangeTime = doc["st_mtim_tv_sec"].toInteger();
    stbuf.ReparseTag = 0;
    stbuf.IndexNumber = 0;
    stbuf.HardLinks = 0;
    stbuf.EaSize = 0;

    // stbuf->st_dev = doc["st_dev"].toInteger();
    // stbuf->st_ino = doc["st_ino"].toInteger();
    // stbuf->st_mode = doc["st_mode"].toInteger();
    // stbuf->st_nlink = doc["st_nlink"].toInteger();
    // stbuf->st_rdev = doc["st_rdev"].toInteger();
    // stbuf->st_size = doc["st_size"].toInteger();
    // stbuf->st_blksize = doc["st_blksize"].toInteger();
    // stbuf->st_blocks = doc["st_blocks"].toInteger();

    // fuse_timespec atime;
    // atime.tv_sec = doc["st_atime_tv_sec"].toInteger();
    // atime.tv_nsec = doc["st_atime_tv_nsec"].toInteger();
    // stbuf->st_atim = atime;

    // fuse_timespec mtime;
    // mtime.tv_sec = doc["st_mtim_tv_sec"].toInteger();
    // mtime.tv_nsec = doc["st_mtim_tv_nsec"].toInteger();
    // stbuf->st_mtim = mtime;

    // fuse_timespec ctime;
    // ctime.tv_sec = doc["st_ctim_tv_sec"].toInteger();
    // ctime.tv_nsec = doc["st_ctim_tv_nsec"].toInteger();
    // stbuf->st_ctim = ctime;

    // fuse3_context* context = fuse3_get_context();
    // stbuf->st_uid = context->uid;
    // stbuf->st_gid = context->gid;
}
