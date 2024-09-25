#include "filesystem.h"
#include <QDebug>
#include <windows.h>
#include <strsafe.h>

#define PROGNAME "Remote Filesystem"
#define ALLOCATION_UNIT                 4096
#define FULLPATH_SIZE                   (MAX_PATH + FSP_FSCTL_TRANSACT_PATH_SIZEMAX / sizeof(WCHAR))
#define HandleFromContext(FC)           (((PTFS_FILE_CONTEXT *)(FC))->Handle)
#define ConcatPath(PTFS, FN, FP)              (0 == StringCbPrintfW(FP, sizeof FP, L"%s%s", PTFS->Path, FN))

typedef struct
{
    HANDLE Handle;
    PVOID DirBuffer;
} PTFS_FILE_CONTEXT;

typedef struct
{
    FSP_FILE_SYSTEM *FileSystem;
    PWSTR Path;
} PTFS;

static NTSTATUS ReadDirectory(FSP_FILE_SYSTEM *FileSystem,
                              PVOID FileContext0, PWSTR Pattern, PWSTR Marker,
                              PVOID Buffer, ULONG BufferLength, PULONG PBytesTransferred)
{
    PTFS *Ptfs = (PTFS *)FileSystem->UserContext;
    PTFS_FILE_CONTEXT *FileContext = (PTFS_FILE_CONTEXT *)FileContext0;
    HANDLE Handle = HandleFromContext(FileContext);
    WCHAR FullPath[FULLPATH_SIZE];
    ULONG Length, PatternLength;
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindData;
    union
    {
        UINT8 B[FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) + MAX_PATH * sizeof(WCHAR)];
        FSP_FSCTL_DIR_INFO D;
    } DirInfoBuf;
    FSP_FSCTL_DIR_INFO *DirInfo = &DirInfoBuf.D;
    NTSTATUS DirBufferResult;

    DirBufferResult = STATUS_SUCCESS;
    if (FspFileSystemAcquireDirectoryBuffer(&FileContext->DirBuffer, 0 == Marker,
                                            &DirBufferResult))                                              // (1)
    {
        if (0 == Pattern)
            Pattern = const_cast<PWSTR>(L"*");
        PatternLength = (ULONG)wcslen(Pattern);

        Length = GetFinalPathNameByHandleW(Handle, FullPath, FULLPATH_SIZE - 1, 0);
        if (0 == Length)
            DirBufferResult = FspNtStatusFromWin32(GetLastError());
        else if (Length + 1 + PatternLength >= FULLPATH_SIZE)
            DirBufferResult = STATUS_OBJECT_NAME_INVALID;
        if (!NT_SUCCESS(DirBufferResult))
        {
            FspFileSystemReleaseDirectoryBuffer(&FileContext->DirBuffer);
            return DirBufferResult;
        }

        if (L'\\' != FullPath[Length - 1])
            FullPath[Length++] = L'\\';
        memcpy(FullPath + Length, Pattern, PatternLength * sizeof(WCHAR));
        FullPath[Length + PatternLength] = L'\0';

        FindHandle = FindFirstFileW(FullPath, &FindData);               // (2)
        if (INVALID_HANDLE_VALUE != FindHandle)
        {
            do
            {
                memset(DirInfo, 0, sizeof *DirInfo);
                Length = (ULONG)wcslen(FindData.cFileName);
                DirInfo->Size = (UINT16)(FIELD_OFFSET(FSP_FSCTL_DIR_INFO, FileNameBuf) + Length * sizeof(WCHAR));
                DirInfo->FileInfo.FileAttributes = FindData.dwFileAttributes;
                DirInfo->FileInfo.ReparseTag = 0;
                DirInfo->FileInfo.FileSize =
                    ((UINT64)FindData.nFileSizeHigh << 32) | (UINT64)FindData.nFileSizeLow;
                DirInfo->FileInfo.AllocationSize = (DirInfo->FileInfo.FileSize + ALLOCATION_UNIT - 1)
                                                   / ALLOCATION_UNIT * ALLOCATION_UNIT;
                DirInfo->FileInfo.CreationTime = ((PLARGE_INTEGER)&FindData.ftCreationTime)->QuadPart;
                DirInfo->FileInfo.LastAccessTime = ((PLARGE_INTEGER)&FindData.ftLastAccessTime)->QuadPart;
                DirInfo->FileInfo.LastWriteTime = ((PLARGE_INTEGER)&FindData.ftLastWriteTime)->QuadPart;
                DirInfo->FileInfo.ChangeTime = DirInfo->FileInfo.LastWriteTime;
                DirInfo->FileInfo.IndexNumber = 0;
                DirInfo->FileInfo.HardLinks = 0;
                memcpy(DirInfo->FileNameBuf, FindData.cFileName, Length * sizeof(WCHAR));

                if (!FspFileSystemFillDirectoryBuffer(&FileContext->DirBuffer, DirInfo,
                                                      &DirBufferResult))                                  // (2)
                    break;
            } while (FindNextFileW(FindHandle, &FindData));             // (2)

            FindClose(FindHandle);
        }

        FspFileSystemReleaseDirectoryBuffer(&FileContext->DirBuffer);   // (3)
    }

    if (!NT_SUCCESS(DirBufferResult))
        return DirBufferResult;

    FspFileSystemReadDirectoryBuffer(&FileContext->DirBuffer,
                                     Marker, Buffer, BufferLength, PBytesTransferred);               // (4)

    return STATUS_SUCCESS;
}

static NTSTATUS GetVolumeInfo(FSP_FILE_SYSTEM *FileSystem,
                              FSP_FSCTL_VOLUME_INFO *VolumeInfo)
{
    PTFS *Ptfs = (PTFS *)FileSystem->UserContext;
    WCHAR Root[MAX_PATH];
    ULARGE_INTEGER TotalSize, FreeSize;

    if (!GetVolumePathName(Ptfs->Path, Root, MAX_PATH))
        return FspNtStatusFromWin32(GetLastError());

    if (!GetDiskFreeSpaceEx(Root, 0, &TotalSize, &FreeSize))
        return FspNtStatusFromWin32(GetLastError());

    VolumeInfo->TotalSize = TotalSize.QuadPart;                         // (1)
    VolumeInfo->FreeSize = FreeSize.QuadPart;                           // (2)
        // (3)
    return STATUS_SUCCESS;
}

static NTSTATUS SetVolumeLabel_(FSP_FILE_SYSTEM *FileSystem,
                                PWSTR VolumeLabel,
                                FSP_FSCTL_VOLUME_INFO *VolumeInfo)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS Create(FSP_FILE_SYSTEM *FileSystem,
                                  PWSTR VolumeLabel,
                                  FSP_FSCTL_VOLUME_INFO *VolumeInfo)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS GetFileInfoInternal(HANDLE Handle, FSP_FSCTL_FILE_INFO *FileInfo)
{
    BY_HANDLE_FILE_INFORMATION ByHandleFileInfo;

    if (!GetFileInformationByHandle(Handle, &ByHandleFileInfo))
        return FspNtStatusFromWin32(GetLastError());

    FileInfo->FileAttributes = ByHandleFileInfo.dwFileAttributes;
    FileInfo->ReparseTag = 0;
    FileInfo->FileSize =
        ((UINT64)ByHandleFileInfo.nFileSizeHigh << 32) | (UINT64)ByHandleFileInfo.nFileSizeLow;
    FileInfo->AllocationSize = (FileInfo->FileSize + ALLOCATION_UNIT - 1)
                               / ALLOCATION_UNIT * ALLOCATION_UNIT;
    FileInfo->CreationTime = ((PLARGE_INTEGER)&ByHandleFileInfo.ftCreationTime)->QuadPart;
    FileInfo->LastAccessTime = ((PLARGE_INTEGER)&ByHandleFileInfo.ftLastAccessTime)->QuadPart;
    FileInfo->LastWriteTime = ((PLARGE_INTEGER)&ByHandleFileInfo.ftLastWriteTime)->QuadPart;
    FileInfo->ChangeTime = FileInfo->LastWriteTime;
    FileInfo->IndexNumber =
        ((UINT64)ByHandleFileInfo.nFileIndexHigh << 32) | (UINT64)ByHandleFileInfo.nFileIndexLow;
    FileInfo->HardLinks = 0;

    return STATUS_SUCCESS;
}

static NTSTATUS Open(FSP_FILE_SYSTEM *FileSystem,
                     PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess,
                     PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo)
{
    PTFS *Ptfs = (PTFS *)FileSystem->UserContext;
    WCHAR FullPath[FULLPATH_SIZE];
    ULONG CreateFlags;
    PTFS_FILE_CONTEXT *FileContext;

    if (!ConcatPath(Ptfs, FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    FileContext = (PTFS_FILE_CONTEXT *)malloc(sizeof *FileContext);                          // (1)
    if (0 == FileContext)
        return STATUS_INSUFFICIENT_RESOURCES;
    memset(FileContext, 0, sizeof *FileContext);

    CreateFlags = FILE_FLAG_BACKUP_SEMANTICS;                           // (2)
    if (CreateOptions & FILE_DELETE_ON_CLOSE)
        CreateFlags |= FILE_FLAG_DELETE_ON_CLOSE;                       // (3)

    FileContext->Handle = CreateFileW(FullPath,
                                      GrantedAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
                                      OPEN_EXISTING, CreateFlags, 0);                                 // (4)
    if (INVALID_HANDLE_VALUE == FileContext->Handle)
    {
        free(FileContext);
        return FspNtStatusFromWin32(GetLastError());
    }

    *PFileContext = FileContext;

    return GetFileInfoInternal(FileContext->Handle, FileInfo);          // (5)
}

static NTSTATUS Overwrite(FSP_FILE_SYSTEM *FileSystem,
                     PWSTR VolumeLabel,
                     FSP_FSCTL_VOLUME_INFO *VolumeInfo)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS Cleanup(FSP_FILE_SYSTEM *FileSystem,
                          PWSTR VolumeLabel,
                          FSP_FSCTL_VOLUME_INFO *VolumeInfo)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

static VOID Close(FSP_FILE_SYSTEM *FileSystem,
                  PVOID FileContext0)
{
    PTFS_FILE_CONTEXT *FileContext = (PTFS_FILE_CONTEXT *)FileContext0;
    HANDLE Handle = HandleFromContext(FileContext);

    CloseHandle(Handle);                                                // (1)

    FspFileSystemDeleteDirectoryBuffer(&FileContext->DirBuffer);        // (2)
    free(FileContext);                                                  // (3)
}

static NTSTATUS GetSecurityByName(FSP_FILE_SYSTEM *FileSystem,
                                  PWSTR FileName, PUINT32 PFileAttributes,
                                  PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize)
{
    PTFS *Ptfs = (PTFS *)FileSystem->UserContext;
    WCHAR FullPath[FULLPATH_SIZE];
    HANDLE Handle;
    FILE_ATTRIBUTE_TAG_INFO AttributeTagInfo;
    DWORD SecurityDescriptorSizeNeeded;
    NTSTATUS Result;

    if (!ConcatPath(Ptfs, FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    Handle = CreateFileW(FullPath,
                         FILE_READ_ATTRIBUTES | READ_CONTROL, 0, 0,
                         OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (INVALID_HANDLE_VALUE == Handle)
    {
        Result = FspNtStatusFromWin32(GetLastError());
        goto exit;
    }

    if (0 != PFileAttributes)
    {
        if (!GetFileInformationByHandleEx(Handle,
                                          FileAttributeTagInfo, &AttributeTagInfo, sizeof AttributeTagInfo))
        {
            Result = FspNtStatusFromWin32(GetLastError());
            goto exit;
        }

        *PFileAttributes = AttributeTagInfo.FileAttributes;             // (1)
    }

    if (0 != PSecurityDescriptorSize)
    {
        if (!GetKernelObjectSecurity(Handle,
                                     OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                     SecurityDescriptor, (DWORD)*PSecurityDescriptorSize, &SecurityDescriptorSizeNeeded))
        {
            *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;
            Result = FspNtStatusFromWin32(GetLastError());
            goto exit;
        }

        *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;        // (2)
    }

    Result = STATUS_SUCCESS;

exit:
    if (INVALID_HANDLE_VALUE != Handle)
        CloseHandle(Handle);

    return Result;
}

static NTSTATUS GetFileInfo(FSP_FILE_SYSTEM *FileSystem,
                            PVOID FileContext,
                            FSP_FSCTL_FILE_INFO *FileInfo)
{
    HANDLE Handle = HandleFromContext(FileContext);

    return GetFileInfoInternal(Handle, FileInfo);
}

static NTSTATUS GetSecurity(FSP_FILE_SYSTEM *FileSystem,
                            PVOID FileContext,
                            PSECURITY_DESCRIPTOR SecurityDescriptor, SIZE_T *PSecurityDescriptorSize)
{
    HANDLE Handle = HandleFromContext(FileContext);
    DWORD SecurityDescriptorSizeNeeded;

    if (!GetKernelObjectSecurity(Handle,
                                 OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                 SecurityDescriptor, (DWORD)*PSecurityDescriptorSize, &SecurityDescriptorSizeNeeded))
    {
        *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;
        return FspNtStatusFromWin32(GetLastError());
    }

    *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;

    return STATUS_SUCCESS;
}

static NTSTATUS Read(FSP_FILE_SYSTEM *FileSystem,
                     PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length,
                     PULONG PBytesTransferred)
{
    HANDLE Handle = HandleFromContext(FileContext);
    OVERLAPPED Overlapped = { 0 };

    Overlapped.Offset = (DWORD)Offset;                                  // (1)
    Overlapped.OffsetHigh = (DWORD)(Offset >> 32);

    if (!ReadFile(Handle, Buffer, Length, PBytesTransferred, &Overlapped))
        return FspNtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

static NTSTATUS Write(FSP_FILE_SYSTEM *FileSystem,
                      PVOID FileContext, PVOID Buffer, UINT64 Offset, ULONG Length,
                      BOOLEAN WriteToEndOfFile, BOOLEAN ConstrainedIo,
                      PULONG PBytesTransferred, FSP_FSCTL_FILE_INFO *FileInfo)
{
    HANDLE Handle = HandleFromContext(FileContext);
    LARGE_INTEGER FileSize;
    OVERLAPPED Overlapped = { 0 };

    if (ConstrainedIo)                                                  // (1)
    {
        if (!GetFileSizeEx(Handle, &FileSize))
            return FspNtStatusFromWin32(GetLastError());

        if (Offset >= (UINT64)FileSize.QuadPart)
            return STATUS_SUCCESS;
        if (Offset + Length > (UINT64)FileSize.QuadPart)
            Length = (ULONG)((UINT64)FileSize.QuadPart - Offset);
    }

    Overlapped.Offset = (DWORD)Offset;                                  // (2)
    Overlapped.OffsetHigh = (DWORD)(Offset >> 32);

    if (!WriteFile(Handle, Buffer, Length, PBytesTransferred, &Overlapped))
        return FspNtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

static NTSTATUS SetBasicInfo(FSP_FILE_SYSTEM *FileSystem,
                             PVOID FileContext, UINT32 FileAttributes,
                             UINT64 CreationTime, UINT64 LastAccessTime, UINT64 LastWriteTime, UINT64 ChangeTime,
                             FSP_FSCTL_FILE_INFO *FileInfo)
{
    HANDLE Handle = HandleFromContext(FileContext);
    FILE_BASIC_INFO BasicInfo = { 0 };

    if (INVALID_FILE_ATTRIBUTES == FileAttributes)
        FileAttributes = 0;
    else if (0 == FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    BasicInfo.FileAttributes = FileAttributes;
    BasicInfo.CreationTime.QuadPart = CreationTime;
    BasicInfo.LastAccessTime.QuadPart = LastAccessTime;
    BasicInfo.LastWriteTime.QuadPart = LastWriteTime;
    //BasicInfo.ChangeTime = ChangeTime;

    if (!SetFileInformationByHandle(Handle,
                                    FileBasicInfo, &BasicInfo, sizeof BasicInfo))
        return FspNtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

static NTSTATUS SetFileSize(FSP_FILE_SYSTEM *FileSystem,
                            PVOID FileContext, UINT64 NewSize, BOOLEAN SetAllocationSize,
                            FSP_FSCTL_FILE_INFO *FileInfo)
{
    HANDLE Handle = HandleFromContext(FileContext);
    FILE_ALLOCATION_INFO AllocationInfo;
    FILE_END_OF_FILE_INFO EndOfFileInfo;

    if (SetAllocationSize)
    {
        /*
         * This file system does not maintain AllocationSize, although NTFS clearly can.
         * However it must always be FileSize <= AllocationSize and NTFS will make sure
         * to truncate the FileSize if it sees an AllocationSize < FileSize.
         *
         * If OTOH a very large AllocationSize is passed, the call below will increase
         * the AllocationSize of the underlying file, although our file system does not
         * expose this fact. This AllocationSize is only temporary as NTFS will reset
         * the AllocationSize of the underlying file when it is closed.
         */

        AllocationInfo.AllocationSize.QuadPart = NewSize;

        if (!SetFileInformationByHandle(Handle,
                                        FileAllocationInfo, &AllocationInfo, sizeof AllocationInfo))
            return FspNtStatusFromWin32(GetLastError());
    }
    else
    {
        EndOfFileInfo.EndOfFile.QuadPart = NewSize;

        if (!SetFileInformationByHandle(Handle,
                                        FileEndOfFileInfo, &EndOfFileInfo, sizeof EndOfFileInfo))
            return FspNtStatusFromWin32(GetLastError());
    }

    return GetFileInfoInternal(Handle, FileInfo);
}

static NTSTATUS SetSecurity(FSP_FILE_SYSTEM *FileSystem,
                            PVOID FileContext,
                            SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ModificationDescriptor)
{
    HANDLE Handle = HandleFromContext(FileContext);

    if (!SetKernelObjectSecurity(Handle, SecurityInformation, ModificationDescriptor))
        return FspNtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS Flush(FSP_FILE_SYSTEM *FileSystem,
               PVOID FileContext,
               FSP_FSCTL_FILE_INFO *FileInfo)
{
    HANDLE Handle = HandleFromContext(FileContext);

    /* we do not flush the whole volume, so just return SUCCESS */
    if (0 == Handle)
        return STATUS_SUCCESS;

    if (!FlushFileBuffers(Handle))
        return FspNtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

static NTSTATUS Create(FSP_FILE_SYSTEM *FileSystem,
                       PWSTR FileName, UINT32 CreateOptions, UINT32 GrantedAccess,
                       UINT32 FileAttributes, PSECURITY_DESCRIPTOR SecurityDescriptor, UINT64 AllocationSize,
                       PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo)
{
    PTFS *Ptfs = (PTFS *)FileSystem->UserContext;
    WCHAR FullPath[FULLPATH_SIZE];
    SECURITY_ATTRIBUTES SecurityAttributes;
    ULONG CreateFlags;
    PTFS_FILE_CONTEXT *FileContext;

    if (!ConcatPath(Ptfs, FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    FileContext = (PTFS_FILE_CONTEXT*)malloc(sizeof *FileContext);                          // (1)
    if (0 == FileContext)
        return STATUS_INSUFFICIENT_RESOURCES;
    memset(FileContext, 0, sizeof *FileContext);

    SecurityAttributes.nLength = sizeof SecurityAttributes;
    SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    CreateFlags = FILE_FLAG_BACKUP_SEMANTICS;                           // (2)
    if (CreateOptions & FILE_DELETE_ON_CLOSE)
        CreateFlags |= FILE_FLAG_DELETE_ON_CLOSE;                       // (3)

    if (CreateOptions & FILE_DIRECTORY_FILE)
    {
        /*
         * It is not widely known but CreateFileW can be used to create directories!
         * It requires the specification of both FILE_FLAG_BACKUP_SEMANTICS and
         * FILE_FLAG_POSIX_SEMANTICS. It also requires that FileAttributes has
         * FILE_ATTRIBUTE_DIRECTORY set.
         */
        CreateFlags |= FILE_FLAG_POSIX_SEMANTICS;                       // (2)
        FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    else
        FileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;

    if (0 == FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    FileContext->Handle = CreateFileW(FullPath,
                                      GrantedAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &SecurityAttributes,
                                      CREATE_NEW, CreateFlags | FileAttributes, 0);                   // (4)
    if (INVALID_HANDLE_VALUE == FileContext->Handle)
    {
        free(FileContext);
        return FspNtStatusFromWin32(GetLastError());
    }

    *PFileContext = FileContext;

    return GetFileInfoInternal(FileContext->Handle, FileInfo);          // (5)
}

static NTSTATUS Overwrite(FSP_FILE_SYSTEM *FileSystem,
                          PVOID FileContext, UINT32 FileAttributes, BOOLEAN ReplaceFileAttributes, UINT64 AllocationSize,
                          FSP_FSCTL_FILE_INFO *FileInfo)
{
    HANDLE Handle = HandleFromContext(FileContext);
    FILE_BASIC_INFO BasicInfo = { 0 };
    FILE_ALLOCATION_INFO AllocationInfo = { 0 };
    FILE_ATTRIBUTE_TAG_INFO AttributeTagInfo;

    if (ReplaceFileAttributes)
    {
        if (0 == FileAttributes)
            FileAttributes = FILE_ATTRIBUTE_NORMAL;

        BasicInfo.FileAttributes = FileAttributes;                      // (1)
        if (!SetFileInformationByHandle(Handle,
                                        FileBasicInfo, &BasicInfo, sizeof BasicInfo))
            return FspNtStatusFromWin32(GetLastError());
    }
    else if (0 != FileAttributes)
    {
        if (!GetFileInformationByHandleEx(Handle,
                                          FileAttributeTagInfo, &AttributeTagInfo, sizeof AttributeTagInfo))
            return FspNtStatusFromWin32(GetLastError());

        BasicInfo.FileAttributes =
            FileAttributes | AttributeTagInfo.FileAttributes;           // (2)
        if (BasicInfo.FileAttributes ^ FileAttributes)
        {
            if (!SetFileInformationByHandle(Handle,
                                            FileBasicInfo, &BasicInfo, sizeof BasicInfo))
                return FspNtStatusFromWin32(GetLastError());
        }
    }

    if (!SetFileInformationByHandle(Handle,
                                    FileAllocationInfo, &AllocationInfo, sizeof AllocationInfo))    // (3)
        return FspNtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

static VOID Cleanup(FSP_FILE_SYSTEM *FileSystem,
                    PVOID FileContext, PWSTR FileName, ULONG Flags)
{
    HANDLE Handle = HandleFromContext(FileContext);

    if (Flags & FspCleanupDelete)                                       // (1)
    {
        CloseHandle(Handle);

        /* this will make all future uses of Handle to fail with STATUS_INVALID_HANDLE */
        HandleFromContext(FileContext) = INVALID_HANDLE_VALUE;          // (2)
    }
}

static NTSTATUS CanDelete(FSP_FILE_SYSTEM *FileSystem,
                          PVOID FileContext, PWSTR FileName)
{
    HANDLE Handle = HandleFromContext(FileContext);
    FILE_DISPOSITION_INFO DispositionInfo;

    DispositionInfo.DeleteFile = TRUE;                                  // (1)

    if (!SetFileInformationByHandle(Handle,
                                    FileDispositionInfo, &DispositionInfo, sizeof DispositionInfo))
        return FspNtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

static NTSTATUS Rename(FSP_FILE_SYSTEM *FileSystem,
                       PVOID FileContext,
                       PWSTR FileName, PWSTR NewFileName, BOOLEAN ReplaceIfExists)
{
    PTFS *Ptfs = (PTFS *)FileSystem->UserContext;
    WCHAR FullPath[FULLPATH_SIZE], NewFullPath[FULLPATH_SIZE];

    if (!ConcatPath(Ptfs, FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    if (!ConcatPath(Ptfs, NewFileName, NewFullPath))
        return STATUS_OBJECT_NAME_INVALID;

    if (!MoveFileExW(FullPath, NewFullPath, ReplaceIfExists ? MOVEFILE_REPLACE_EXISTING : 0))
        return FspNtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

static NTSTATUS EnableBackupRestorePrivileges(VOID)
{
    union
    {
        TOKEN_PRIVILEGES P;
        UINT8 B[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
    } Privileges;
    HANDLE Token;

    Privileges.P.PrivilegeCount = 2;
    Privileges.P.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    Privileges.P.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueW(0, SE_BACKUP_NAME, &Privileges.P.Privileges[0].Luid) ||
        !LookupPrivilegeValueW(0, SE_RESTORE_NAME, &Privileges.P.Privileges[1].Luid))
        return FspNtStatusFromWin32(GetLastError());

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &Token))
        return FspNtStatusFromWin32(GetLastError());

    if (!AdjustTokenPrivileges(Token, FALSE, &Privileges.P, 0, 0, 0))
    {
        CloseHandle(Token);

        return FspNtStatusFromWin32(GetLastError());
    }

    CloseHandle(Token);

    return STATUS_SUCCESS;
}

static FSP_FILE_SYSTEM_INTERFACE PtfsInterface =
    {
        GetVolumeInfo,
        SetVolumeLabel_,
        GetSecurityByName,
        Create,
        Open,
        Overwrite,
        Cleanup,
        Close,
        Read,
        Write,
        Flush,
        GetFileInfo,
        SetBasicInfo,
        SetFileSize,
        CanDelete,
        Rename,
        GetSecurity,
        SetSecurity,
        ReadDirectory,
};

static VOID PtfsDelete(PTFS *Ptfs)
{
    if (0 != Ptfs->FileSystem)
        FspFileSystemDelete(Ptfs->FileSystem);                          // (1)

    if (0 != Ptfs->Path)
        free(Ptfs->Path);                                               // (2)

    free(Ptfs);                                                         // (2)
}

static NTSTATUS PtfsCreate(PWSTR Path, PWSTR VolumePrefix, PWSTR MountPoint, UINT32 DebugFlags,
                           PTFS **PPtfs)
{
    WCHAR FullPath[MAX_PATH];
    ULONG Length;
    HANDLE Handle;
    FILETIME CreationTime;
    DWORD LastError;
    FSP_FSCTL_VOLUME_PARAMS VolumeParams;
    PTFS *Ptfs = 0;
    NTSTATUS Result;

    *PPtfs = 0;

    Handle = CreateFileW(
        Path, FILE_READ_ATTRIBUTES, 0, 0,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (INVALID_HANDLE_VALUE == Handle)
        return FspNtStatusFromWin32(GetLastError());

    Length = GetFinalPathNameByHandleW(Handle,
                                       FullPath, FULLPATH_SIZE - 1, 0);                                // (1)
    if (0 == Length)
    {
        LastError = GetLastError();
        CloseHandle(Handle);
        return FspNtStatusFromWin32(LastError);
    }
    if (L'\\' == FullPath[Length - 1])
        FullPath[--Length] = L'\0';

    if (!GetFileTime(Handle, &CreationTime, 0, 0))                      // (2)
    {
        LastError = GetLastError();
        CloseHandle(Handle);
        return FspNtStatusFromWin32(LastError);
    }

    CloseHandle(Handle);

    /* from now on we must goto exit on failure */

    Ptfs = (PTFS *)malloc(sizeof *Ptfs);                                        // (3)
    if (0 == Ptfs)
    {
        Result = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    memset(Ptfs, 0, sizeof *Ptfs);

    Length = (Length + 1) * sizeof(WCHAR);
    Ptfs->Path = (PWSTR)malloc(Length);                                        // (3)
    if (0 == Ptfs->Path)
    {
        Result = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    memcpy(Ptfs->Path, FullPath, Length);

    memset(&VolumeParams, 0, sizeof VolumeParams);                      // (4)
    VolumeParams.SectorSize = 1024;
    VolumeParams.SectorsPerAllocationUnit = 1;
    VolumeParams.VolumeCreationTime = ((PLARGE_INTEGER)&CreationTime)->QuadPart;
    VolumeParams.VolumeSerialNumber = 0;
    VolumeParams.FileInfoTimeout = 1000;
    VolumeParams.CaseSensitiveSearch = 0;
    VolumeParams.CasePreservedNames = 1;
    VolumeParams.UnicodeOnDisk = 1;
    VolumeParams.PersistentAcls = 1;
    VolumeParams.PostCleanupWhenModifiedOnly = 1;                       // (4)
    VolumeParams.UmFileContextIsUserContext2 = 1;                       // (4)
    if (0 != VolumePrefix)
        wcscpy_s(VolumeParams.Prefix, sizeof VolumeParams.Prefix / sizeof(WCHAR), VolumePrefix);
    wcscpy_s(VolumeParams.FileSystemName, sizeof VolumeParams.FileSystemName / sizeof(WCHAR),
             L"" PROGNAME);

    Result = FspFileSystemCreate(
        const_cast<PWSTR>(VolumeParams.Prefix[0] ? L"" FSP_FSCTL_NET_DEVICE_NAME : L"" FSP_FSCTL_DISK_DEVICE_NAME),
        &VolumeParams,
        &PtfsInterface,
        &Ptfs->FileSystem);                                             // (5)
    if (!NT_SUCCESS(Result))
        goto exit;
    Ptfs->FileSystem->UserContext = Ptfs;                               // (5)

    Result = FspFileSystemSetMountPoint(Ptfs->FileSystem, MountPoint);  // (6)
    if (!NT_SUCCESS(Result))
        goto exit;

    FspFileSystemSetDebugLog(Ptfs->FileSystem, DebugFlags);             // (7)

    Result = STATUS_SUCCESS;

exit:
    if (NT_SUCCESS(Result))
        *PPtfs = Ptfs;
    else if (0 != Ptfs)
        PtfsDelete(Ptfs);

    return Result;
}

Filesystem::Filesystem() {}

void Filesystem::run() {
    if (!NT_SUCCESS(FspLoad(0))) {
        qDebug() << "Cannot load FSP";
        return;
    }
    qDebug() << "Starting";
    std::wstring name = L"Remote Filesystem";
    FspServiceRunEx(name.data(), &Filesystem::start, &Filesystem::stop, &Filesystem::control, this);
}

NTSTATUS Filesystem::start(FSP_SERVICE *Service, ULONG argc, PWSTR *argv) {
    qDebug() << "Filesystem start";
    ULONG DebugFlags = 0;
    PWSTR VolumePrefix = 0;
    PWSTR PassThrough = 0;
    PWSTR MountPoint = 0;
    PTFS* Ptfs = 0;

    NTSTATUS Result = PtfsCreate(PassThrough, VolumePrefix, MountPoint, DebugFlags,
                        &Ptfs);                                                         // (1)
    if (!NT_SUCCESS(Result))
    {
        qDebug() << "cannot create file system";
        return Result;
    }

    Result = FspFileSystemStartDispatcher(Ptfs->FileSystem, 0);         // (2)
    if (!NT_SUCCESS(Result))
    {
        qDebug() << "cannot start file system";
        return Result;
    }
    Service->UserContext = Ptfs;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS Filesystem::stop(FSP_SERVICE *Service) {
    qDebug() << "Filesystem stop";

    PTFS *Ptfs = (PTFS *)Service->UserContext;                                  // (1)

    FspFileSystemStopDispatcher(Ptfs->FileSystem);                      // (2)
    PtfsDelete(Ptfs);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS Filesystem::control(FSP_SERVICE *Service, ULONG argc, ULONG param, PVOID arg) {
    qDebug() << "Filesystem control";
    return STATUS_NOT_IMPLEMENTED;
}
