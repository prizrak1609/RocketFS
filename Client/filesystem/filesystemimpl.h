#pragma once

#include "filesystem.h"
#include "winfsp.h"

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
        Filesystem::get_instance()->ReadFile(FileContext, Buffer, Offset, Length, PBytesTransferred);
        return STATUS_SUCCESS;
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
        Filesystem::get_instance()->Open(FileName, CreateOptions, GrantedAccess, PFileContext, FileInfo);
        return STATUS_SUCCESS;
    }

    static VOID Close(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext)
    {
        Filesystem::get_instance()->Close(FileContext);
    }
};

