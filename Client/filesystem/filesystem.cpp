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

#include <aclapi.h>
#include <ranges>
#include <algorithm>

#define print_function qDebug() << __FUNCTION__

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

    for (OpenedItem* item : qAsConst(opened_items)) {
        delete item;
    }

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

    volumeInfo->VolumeLabelLength = sizeof L"RemoteFS" - sizeof(WCHAR);
    memcpy(volumeInfo->VolumeLabel, L"RemoteFS", volumeInfo->VolumeLabelLength);

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

NTSTATUS Filesystem::ReadFile(PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, PULONG PBytesTransferred)
{
    print_function << FileContext << "offset:" << Offset << "length:" << Length;
    auto item = reinterpret_cast<OpenedItem*>(FileContext);

    ReadOp* operation = new ReadOp();
    operation->Length = Length;
    operation->Offset = Offset;
    operation->Buffer = Buffer;
    operation->requestID = FspFileSystemGetOperationContext()->Request->Hint;

    ReadFileCmd cmd(item->server_path, Length, Offset);
    Connection_pool::get_instance()->send_binary(cmd).then(QtFuture::Launch::Async, [operation, this](QByteArray message){
                                                         print_function << "offset:" << operation->Offset << "need length:" << operation->Length << "received length:" << message.length();

                                                         auto length = min(operation->Length, message.length());
                                                         FSP_FSCTL_TRANSACT_RSP response;
                                                         memset(&response, 0, sizeof response);
                                                         response.Size = sizeof response;
                                                         response.Kind = FspFsctlTransactReadKind;
                                                         response.Hint = operation->requestID;

                                                         if (message.isEmpty()) {
                                                             response.IoStatus.Status = STATUS_END_OF_FILE;
                                                         } else {
                                                             response.IoStatus.Status = STATUS_SUCCESS;

                                                             memmove(operation->Buffer, (PUINT8)message.data(), length);
                                                         }

                                                         response.IoStatus.Information = length;

                                                         delete operation;

                                                         FspFileSystemSendResponse(fileSystem, &response);
    });

    return STATUS_PENDING;
}

void Filesystem::WriteFile(PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, BOOLEAN WriteToEndOfFile, BOOLEAN ConstrainedIo, PULONG PBytesTransferred, FSP_FSCTL_FILE_INFO *FileInfo)
{
    print_function << " " << FileContext;
    // write_file
}

void Filesystem::GetFileInfo(PVOID FileContext, FSP_FSCTL_FILE_INFO *FileInfo)
{
    print_function << " " << FileContext;

    OpenedItem* item = reinterpret_cast<OpenedItem*>(FileContext);
    GetAttrCmd cmd(item->server_path);
    Connection_pool::get_instance()->send_text(cmd).then(QtFuture::Launch::Sync, [item, FileInfo, this](QString message){
                                                       convert_to_stat(QJsonDocument::fromJson(message.toUtf8()).object(), item);

                                                       *FileInfo = item->info;
    }).waitForFinished();
}

BOOLEAN Filesystem::AddDirInfo(QString fileName, FSP_FSCTL_FILE_INFO fileInfo, PVOID Buffer, ULONG Length, PULONG PBytesTransferred)
{
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

    // QMutexLocker lock(&(item->lock));

    print_function << " " << FileContext << item->path << QThread::currentThreadId();

    ReadDirCmd cmd(item->server_path, this);

    Connection_pool::get_instance()->send_text(cmd)
        .then(QtFuture::Launch::Sync, [=, this](QString message) {
                                                        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

                                                        for (auto item : doc.array()) {
                                                            QJsonObject json = item.toObject();
                                                            QString name = json["file_name"].toString();

                                                            OpenedItem oitem;
                                                            convert_to_stat(json["stats"].toObject(), &oitem);

                                                            print_function << " " << name << " stats: " << json["stats"].toObject();

                                                            AddDirInfo(name, oitem.info, Buffer, Length, PBytesTransferred);
                                                        }

                                                        FspFileSystemAddDirInfo(nullptr, Buffer, Length, PBytesTransferred);
                                                   }).waitForFinished();
}

void Filesystem::GetDirInfoByName(PVOID FileContext, PWSTR FileName, FSP_FSCTL_DIR_INFO *DirInfo)
{
    print_function << " " << FileContext << " " << QString::fromWCharArray(FileName);
    OpenedItem* item = reinterpret_cast<OpenedItem*>(FileContext);
    GetAttrCmd cmd(item->server_path);
    Connection_pool::get_instance()->send_text(cmd).then(QtFuture::Launch::Sync, [item, DirInfo, this](QString message){
                                                       convert_to_stat(QJsonDocument::fromJson(message.toUtf8()).object(), item);

                                                       FSP_FSCTL_DIR_INFO info;
                                                       info.FileInfo = item->info;
                                                       info.Size = item->info.FileSize;

                                                       *DirInfo = info;
                                                   }).waitForFinished();
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
    QString file = QString::fromWCharArray(FileName);
    auto iterator = std::ranges::find_if(opened_items, [&file](const OpenedItem* item) -> bool {
        return item->path.compare(file, Qt::CaseInsensitive) == 0;
    });

    if (iterator != opened_items.end()) {
        // (*iterator)->links++;

        *PFileContext = *iterator;
        *FileInfo = (*iterator)->info;
    } else {
        auto item = new OpenedItem();
        item->path = file;
        item->server_path = item->path;
        item->server_path.replace("\\", "/");

        NTSTATUS result = STATUS_SUCCESS;
        GetAttrCmd cmd(item->server_path);
        Connection_pool::get_instance()->send_text(cmd).then(QtFuture::Launch::Sync, [&result, item, this](QString message) {
                                                           if (message.isEmpty()) {
                                                               result = STATUS_NOT_FOUND;
                                                               return;
                                                           }
                                                           convert_to_stat(QJsonDocument::fromJson(message.toUtf8()).object(), item);
        }).waitForFinished();

        if (result == STATUS_NOT_FOUND) {
            delete item;
            return result;
        }

        *FileInfo = item->info;

        *PFileContext = item;

        opened_items.insert(item);
    }

    print_function << " " << file << " " << *PFileContext;
    return STATUS_SUCCESS;
}

NTSTATUS Filesystem::getSecurityForFile(PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize) {
    if (fileSecurityDescriptor == nullptr) {
        GetNamedSecurityInfo(TEXT("F:\\Rayman Legends\\"), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &fileSecurityDescriptor);

        fileSecurityDescriptorSize = GetSecurityDescriptorLength(fileSecurityDescriptor);
        //LPCTSTR              SACL = TEXT("O:S-1-5-21-730841570-429836790-1799428936-1001G:S-1-5-21-730841570-429836790-1799428936-1001D:(A;ID;FA;;;BA)(A;OICIIOID;GA;;;BA)(A;ID;FA;;;SY)(A;OICIIOID;GA;;;SY)(A;ID;0x1301bf;;;AU)(A;OICIIOID;SDGXGWGR;;;AU)(A;ID;0x1200a9;;;BU)(A;OICIIOID;GXGR;;;BU)");//TEXT("O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)");
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
        //ConvertStringSecurityDescriptorToSecurityDescriptor(SACL, SDDL_REVISION_1, &(this->SecurityDescriptor), &(this->SecurityDescriptorSize));
        //qDebug() << __FUNCTION__ << " error " << FspNtStatusFromWin32(GetLastError());
    }

    if (PSecurityDescriptorSize != nullptr) {
        if (fileSecurityDescriptorSize > *PSecurityDescriptorSize) {
            *PSecurityDescriptorSize = fileSecurityDescriptorSize;
            print_function << " buffer overflow";
            return STATUS_BUFFER_OVERFLOW;
        }

        *PSecurityDescriptorSize = fileSecurityDescriptorSize;

        if (SecurityDescriptor != nullptr) {
            memcpy(SecurityDescriptor, fileSecurityDescriptor, fileSecurityDescriptorSize);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS Filesystem::getSecurityForDir(PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize) {
    if (dirSecurityDescriptor == nullptr) {
        GetNamedSecurityInfo(TEXT("F:\\Rayman Legends\\"), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &dirSecurityDescriptor);

        dirSecurityDescriptorSize = GetSecurityDescriptorLength(dirSecurityDescriptor);
        //LPCTSTR              SACL = TEXT("O:S-1-5-21-730841570-429836790-1799428936-1001G:S-1-5-21-730841570-429836790-1799428936-1001D:(A;ID;FA;;;BA)(A;OICIIOID;GA;;;BA)(A;ID;FA;;;SY)(A;OICIIOID;GA;;;SY)(A;ID;0x1301bf;;;AU)(A;OICIIOID;SDGXGWGR;;;AU)(A;ID;0x1200a9;;;BU)(A;OICIIOID;GXGR;;;BU)");//TEXT("O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)");
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
        //ConvertStringSecurityDescriptorToSecurityDescriptor(SACL, SDDL_REVISION_1, &(this->SecurityDescriptor), &(this->SecurityDescriptorSize));
        //qDebug() << __FUNCTION__ << " error " << FspNtStatusFromWin32(GetLastError());
    }

    if (PSecurityDescriptorSize != nullptr) {
        if (dirSecurityDescriptorSize > *PSecurityDescriptorSize) {
            *PSecurityDescriptorSize = dirSecurityDescriptorSize;
            print_function << " buffer overflow";
            return STATUS_BUFFER_OVERFLOW;
        }

        *PSecurityDescriptorSize = dirSecurityDescriptorSize;

        if (SecurityDescriptor != nullptr) {
            memcpy(SecurityDescriptor, dirSecurityDescriptor, dirSecurityDescriptorSize);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS Filesystem::GetSecurityByName(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, PUINT32 PFileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize)
{
    PVOID context;
    FSP_FSCTL_FILE_INFO info;
    auto result = Open(FileName, 0, 0, &context, &info);

    if (result == STATUS_NOT_FOUND) {
        return result;
    }

    print_function << QString::fromWCharArray(FileName) << " " << SecurityDescriptor;

    OpenedItem* item = reinterpret_cast<OpenedItem*>(context);

    *PFileAttributes = info.FileAttributes;

    if (item->isFile) {
        return getSecurityForFile(SecurityDescriptor, PSecurityDescriptorSize);
    } else {
        return getSecurityForDir(SecurityDescriptor, PSecurityDescriptorSize);
    }
}

NTSTATUS Filesystem::GetSecurity(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize)
{
    print_function << FileContext << " " << SecurityDescriptor;

    OpenedItem* item = reinterpret_cast<OpenedItem*>(FileContext);

    if (item->isFile) {
        return getSecurityForFile(SecurityDescriptor, PSecurityDescriptorSize);
    } else {
        return getSecurityForDir(SecurityDescriptor, PSecurityDescriptorSize);
    }
}

void Filesystem::Close(PVOID FileContext)
{
    print_function << " " << FileContext;
    // auto item = reinterpret_cast<OpenedItem*>(FileContext);

    // item->links--;

    // if (item->links <= 0) {
    //     opened_items.remove(item);

    //     delete item;

    //     qDebug() << __FUNCTION__ << " unregister";
    // }
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

void Filesystem::convert_to_stat(QJsonObject doc, OpenedItem* item)
{
    FSP_FSCTL_FILE_INFO &stbuf = item->info;
    stbuf.AllocationSize = 0;

    UINT32 attributes = 0;
    if (doc["st_mode_dir"].toBool()) {
        item->isFile = false;
        attributes = FILE_ATTRIBUTE_DIRECTORY;
    } else if (doc["st_mode_file"].toBool()) {
        item->isFile = true;
        attributes = FILE_ATTRIBUTE_OFFLINE;
        //FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_NORMAL

        // if ((doc["st_mode_read"].toBool())) {
        //     attributes |= FILE_GENERIC_READ;
        // }
        // if ((doc["st_mode_write"].toBool())) {
        //     attributes |= FILE_GENERIC_WRITE;
        // }
        // if ((doc["st_mode_exec"].toBool())) {
        //     attributes |= FILE_GENERIC_EXECUTE;
        // }
    }

    stbuf.FileAttributes = attributes;

    stbuf.FileSize = doc["st_size"].toInteger();
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
