#pragma once

#include "filesystem.h"
#include "winfsp.h"

#define print_function static unsigned long int number = 0; \
qDebug() << number << __FUNCTION__; \
    number++;

struct FileSystemImpl
{
    static void setFS(FSP_FILE_SYSTEM* fs)
    {
        Filesystem::get_instance()->fileSystem = fs;
    }

    static FSP_FILE_SYSTEM* getFS()
    {
        return Filesystem::get_instance()->fileSystem;
    }

    static void init(FSP_SERVICE *service, ULONG argc, PWSTR *argv)
    {
        Filesystem::get_instance()->init(service, argc, argv);
    }

    static void destroy(FSP_SERVICE *service)
    {
        Filesystem::get_instance()->destroy(service);
    }

    static NTSTATUS GetVolumeInfo(FSP_FILE_SYSTEM *FileSystem, FSP_FSCTL_VOLUME_INFO *VolumeInfo)
    {
        Filesystem::get_instance()->GetVolumeInfo(VolumeInfo);
        return STATUS_SUCCESS;
    }

    static NTSTATUS ReadFile(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, PULONG PBytesTransferred)
    {
        return Filesystem::get_instance()->ReadFile(FileContext, Buffer, Offset, Length, PBytesTransferred);
    }

    static NTSTATUS WriteFile(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length, BOOLEAN WriteToEndOfFile, BOOLEAN ConstrainedIo, PULONG PBytesTransferred, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        Filesystem::get_instance()->WriteFile(FileContext, Buffer, Offset, Length, WriteToEndOfFile, ConstrainedIo, PBytesTransferred, FileInfo);
        return STATUS_SUCCESS;
    }

    static NTSTATUS GetFileInfo(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        Filesystem::get_instance()->GetFileInfo(FileContext, FileInfo);
        return STATUS_SUCCESS;
    }

    static NTSTATUS CanDelete(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName)
    {
        return STATUS_SUCCESS;
    }

    static NTSTATUS ReadDirectory(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR Pattern, PWSTR Marker, PVOID Buffer, ULONG Length, PULONG PBytesTransferred)
    {
        Filesystem::get_instance()->ReadDirectory(FileContext, Pattern, Marker, Buffer, Length, PBytesTransferred);
        return STATUS_SUCCESS;
    }

    static NTSTATUS GetDirInfoByName(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, FSP_FSCTL_DIR_INFO *DirInfo)
    {
        Filesystem::get_instance()->GetDirInfoByName(FileContext, FileName, DirInfo);
        return STATUS_SUCCESS;
    }

    static NTSTATUS SetDelete(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, BOOLEAN DeleteFile)
    {
        if (DeleteFile) {
            Filesystem::get_instance()->RemoveFile(FileContext, FileName);
        } else {
            Filesystem::get_instance()->RemoveDir(FileContext, FileName);
        }
        return STATUS_SUCCESS;
    }

    static NTSTATUS Open(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        return Filesystem::get_instance()->Open(FileName, CreateOptions, GrantedAccess, PFileContext, FileInfo);
    }

    static VOID Close(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext)
    {
        Filesystem::get_instance()->Close(FileContext);
    }


    static NTSTATUS SetVolumeLabel(FSP_FILE_SYSTEM *FileSystem, PWSTR VolumeLabel, FSP_FSCTL_VOLUME_INFO *VolumeInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS GetSecurityByName(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, PUINT32 PFileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize)
    {
        return Filesystem::get_instance()->GetSecurityByName(FileSystem, FileName, PFileAttributes, SecurityDescriptor, PSecurityDescriptorSize);;
    }

    static NTSTATUS Create(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, UINT32 FileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, UINT64 AllocationSize, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS Overwrite(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 FileAttributes, BOOLEAN ReplaceFileAttributes, UINT64 AllocationSize, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static VOID Cleanup(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, ULONG Flags)
    {
        print_function;
    }

    static NTSTATUS Flush(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS SetBasicInfo(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 FileAttributes, UINT64 CreationTime, UINT64 LastAccessTime, UINT64 LastWriteTime, UINT64 ChangeTime, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS SetFileSize(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT64 NewSize, BOOLEAN SetAllocationSize, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS Rename(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PWSTR NewFileName, BOOLEAN ReplaceIfExists)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS GetSecurity(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize)
    {
        return Filesystem::get_instance()->GetSecurity(FileSystem, FileContext, SecurityDescriptor, PSecurityDescriptorSize);;
    }

    static NTSTATUS SetSecurity(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ModificationDescriptor)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS ResolveReparsePoints(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 ReparsePointIndex, BOOLEAN ResolveLastPathComponent, PIO_STATUS_BLOCK PIoStatus, PVOID Buffer, PSIZE_T PSize)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS GetReparsePoint(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PVOID Buffer, PSIZE_T PSize)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS SetReparsePoint(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PVOID Buffer, SIZE_T Size)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS DeleteReparsePoint(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PWSTR FileName, PVOID Buffer, SIZE_T Size)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS GetStreamInfo(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PVOID Buffer, ULONG Length, PULONG PBytesTransferred)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS Control(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 ControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG PBytesTransferred)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS CreateEx(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess, UINT32 FileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, UINT64 AllocationSize, PVOID ExtraBuffer, ULONG ExtraLength, BOOLEAN ExtraBufferIsReparsePoint, PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS OverwriteEx(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, UINT32 FileAttributes, BOOLEAN ReplaceFileAttributes, UINT64 AllocationSize, PFILE_FULL_EA_INFORMATION Ea, ULONG EaLength, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS GetEa(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PFILE_FULL_EA_INFORMATION Ea, ULONG EaLength, PULONG PBytesTransferred)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static NTSTATUS SetEa(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext, PFILE_FULL_EA_INFORMATION Ea, ULONG EaLength, FSP_FSCTL_FILE_INFO *FileInfo)
    {
        print_function;
        return STATUS_SUCCESS;
    }

    static VOID DispatcherStopped(FSP_FILE_SYSTEM *FileSystem, BOOLEAN Normally)
    {
        print_function;
    }
};

