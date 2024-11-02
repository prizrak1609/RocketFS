#include "filesystem.h"
#include <QDebug>
#include "filesystemimpl.h"
#include "winfsp.h"
#include <winnt.h>

#define PROGNAME "remote-fs"

static inline UINT64 MemfsGetSystemTime(VOID)
{
    FILETIME FileTime;
    GetSystemTimeAsFileTime(&FileTime);
    return ((PLARGE_INTEGER)&FileTime)->QuadPart;
}

static NTSTATUS Start(FSP_SERVICE *service, ULONG argc, PWSTR *argv) {
    static FSP_FILE_SYSTEM_INTERFACE FSInterface = {
        // Get volume information.
        &FileSystemImpl::GetVolumeInfo, // NTSTATUS (*GetVolumeInfo)(FSP_FILE_SYSTEM *FileSystem, FSP_FSCTL_VOLUME_INFO *VolumeInfo);
        // Set volume information.
        nullptr, // NTSTATUS (*SetVolumeLabel)(FSP_FILE_SYSTEM *FileSystem, PWSTR VolumeLabel, FSP_FSCTL_VOLUME_INFO *VolumeInfo);
        // Get file or directory attributes and security descriptor given a file name.
        nullptr, // NTSTATUS (*GetSecurityByName)(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, PUINT32 PFileAttributes/* or ReparsePointIndex */, PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize);
        // Create new file or directory.
        nullptr, // NTSTATUS (*Create)(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, UINT32 FileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, UINT64 AllocationSize, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo);
        // Open a file or directory.
        &FileSystemImpl::Open, // NTSTATUS (*Open)(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo);
        // Overwrite a file.
        nullptr, // NTSTATUS (*Overwrite)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 FileAttributes, BOOLEAN ReplaceFileAttributes, UINT64 AllocationSize, FSP_FSCTL_FILE_INFO *FileInfo);
        // Cleanup a file.
        nullptr, // VOID (*Cleanup)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, ULONG Flags);
        // Close a file.
        &FileSystemImpl::Close, // VOID (*Close)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext);
        // Read a file
        &FileSystemImpl::ReadFile, // NTSTATUS (*Read)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, PULONG PBytesTransferred);
        // Write a file
        &FileSystemImpl::WriteFile, // NTSTATUS (*Write)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, BOOLEAN WriteToEndOfFile, BOOLEAN ConstrainedIo, PULONG PBytesTransferred, FSP_FSCTL_FILE_INFO *FileInfo);
        // Flush a file or volume.
        nullptr, // NTSTATUS (*Flush)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, FSP_FSCTL_FILE_INFO *FileInfo);
        // Get file or directory information.
        &FileSystemImpl::GetFileInfo, // NTSTATUS (*GetFileInfo)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, FSP_FSCTL_FILE_INFO *FileInfo);
        // Set file or directory basic information.
        nullptr, // NTSTATUS (*SetBasicInfo)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 FileAttributes, UINT64 CreationTime, UINT64 LastAccessTime, UINT64 LastWriteTime, UINT64 ChangeTime, FSP_FSCTL_FILE_INFO *FileInfo);
        // Set file/allocation size.
        nullptr, // NTSTATUS (*SetFileSize)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT64 NewSize, BOOLEAN SetAllocationSize, FSP_FSCTL_FILE_INFO *FileInfo);
        // Determine whether a file or directory can be deleted.
        &FileSystemImpl::CanDelete, // NTSTATUS (*CanDelete)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName);
        // Renames a file or directory.
        nullptr, // NTSTATUS (*Rename)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PWSTR NewFileName, BOOLEAN ReplaceIfExists);
        // Get file or directory security descriptor.
        nullptr, // NTSTATUS (*GetSecurity)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize);
        // Set file or directory security descriptor.
        nullptr, // NTSTATUS (*SetSecurity)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ModificationDescriptor);
        // Read a directory.
        &FileSystemImpl::ReadDirectory, // NTSTATUS (*ReadDirectory)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR Pattern, PWSTR Marker, PVOID Buffer, ULONG Length, PULONG PBytesTransferred);
        // Resolve reparse points.
        nullptr, // NTSTATUS (*ResolveReparsePoints)(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 ReparsePointIndex, BOOLEAN ResolveLastPathComponent, PIO_STATUS_BLOCK PIoStatus, PVOID Buffer, PSIZE_T PSize);
        // Get reparse point.
        nullptr, // NTSTATUS (*GetReparsePoint)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PVOID Buffer, PSIZE_T PSize);
        // Set reparse point.
        nullptr, // NTSTATUS (*SetReparsePoint)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PVOID Buffer, SIZE_T Size);
        // Delete reparse point.
        nullptr, // NTSTATUS (*DeleteReparsePoint)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PVOID Buffer, SIZE_T Size);
        // Get named streams information.
        nullptr, // NTSTATUS (*GetStreamInfo)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PVOID Buffer, ULONG Length, PULONG PBytesTransferred);
        // Get directory information for a single file or directory within a parent directory.
        &FileSystemImpl::GetDirInfoByName, // NTSTATUS (*GetDirInfoByName)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, FSP_FSCTL_DIR_INFO *DirInfo);
        // Process control code.
        nullptr, // NTSTATUS (*Control)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 ControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG PBytesTransferred);
        // Set the file delete flag.
        &FileSystemImpl::SetDelete, // NTSTATUS (*SetDelete)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, BOOLEAN DeleteFile);
        // Create new file or directory.
        nullptr, // NTSTATUS (*CreateEx)(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, UINT32 FileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, UINT64 AllocationSize, PVOID ExtraBuffer, ULONG ExtraLength, BOOLEAN ExtraBufferIsReparsePoint, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo);
        // Overwrite a file.
        nullptr, // NTSTATUS (*OverwriteEx)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 FileAttributes, BOOLEAN ReplaceFileAttributes, UINT64 AllocationSize, PFILE_FULL_EA_INFORMATION Ea, ULONG EaLength, FSP_FSCTL_FILE_INFO *FileInfo);
        // Get extended attributes.
        nullptr, // NTSTATUS (*GetEa)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PFILE_FULL_EA_INFORMATION Ea, ULONG EaLength, PULONG PBytesTransferred);
        // Set extended attributes.
        nullptr, // NTSTATUS (*SetEa)(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PFILE_FULL_EA_INFORMATION Ea, ULONG EaLength, FSP_FSCTL_FILE_INFO *FileInfo);
        nullptr, // obsolete
        // Inform the file system that its dispatcher has been stopped.
        nullptr, // VOID (*DispatcherStopped)(FSP_FILE_SYSTEM *FileSystem, BOOLEAN Normally);
        nullptr // Reserved
    };

    FileSystemImpl::init(service, argc, argv);

    FSP_FSCTL_VOLUME_PARAMS VolumeParams;
    VolumeParams.Version = sizeof(FSP_FSCTL_VOLUME_PARAMS);
    VolumeParams.SectorSize = 512;
    VolumeParams.SectorsPerAllocationUnit = 1;
    VolumeParams.VolumeCreationTime = MemfsGetSystemTime();
    VolumeParams.VolumeSerialNumber = (UINT32)(MemfsGetSystemTime() / (10000 * 1000));
    VolumeParams.FileInfoTimeout = 1000;
    VolumeParams.CaseSensitiveSearch = true;
    VolumeParams.CasePreservedNames = 1;
    VolumeParams.UnicodeOnDisk = 1;
    VolumeParams.PersistentAcls = 1;
    VolumeParams.ReparsePoints = 1;
    VolumeParams.ReparsePointsAccessCheck = 0;
    VolumeParams.NamedStreams = 0;
    VolumeParams.PostCleanupWhenModifiedOnly = 1;
    VolumeParams.PostDispositionWhenNecessaryOnly = 1;
    VolumeParams.PassQueryDirectoryFileName = 1;
    VolumeParams.FlushAndPurgeOnCleanup = true;
    VolumeParams.DeviceControl = 0;
    VolumeParams.ExtendedAttributes = 0;
    VolumeParams.WslFeatures = 0;
    VolumeParams.AllowOpenInKernelMode = 1;
    VolumeParams.RejectIrpPriorToTransact0 = 0;
    VolumeParams.SupportsPosixUnlinkRename = true;
    wcscpy_s(VolumeParams.FileSystemName, sizeof VolumeParams.FileSystemName / sizeof(WCHAR), L"RemoteFS");
\
    FSP_FILE_SYSTEM *fileSystem = nullptr;

    std::wstring device_name(L"" FSP_FSCTL_NET_DEVICE_NAME);
    NTSTATUS Result = FspFileSystemCreate(device_name.data(), &VolumeParams, &FSInterface, &fileSystem);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    std::wstring mount_point(L"Y:");
    FspFileSystemSetMountPoint(fileSystem, mount_point.data());

    fileSystem->UserContext = nullptr;

    FileSystemImpl::setFS(fileSystem);
    return Result;
}

static NTSTATUS Stop(FSP_SERVICE *service) {
    FileSystemImpl::destroy(service);
    FspFileSystemDelete(FileSystemImpl::getFS());
    return STATUS_SUCCESS;
}

void Filesystem::run()
{
    qDebug() << "Start filesystem " << QThread::currentThreadId();

    std::wstring name(L"" PROGNAME);
    FspServiceRun(name.data(), Start, Stop, 0);
}
