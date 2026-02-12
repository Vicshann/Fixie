
#pragma once

//============================================================================================================
// Only most useful NTDLL functions will go here for now
// https://github.com/winsiderss/phnt
// TODO: Rename to NTSYS and include definitions for drivers too and everything from NtDllEx(impossible,uses imports)?
//
// Default alignment: 8 bytes (x32 and x64)
//
// Default packing:
//    8-byte boundaries (default for x86, ARM, and ARM64).
//    16-byte boundaries (default for x64 and ARM64EC).
//
template<typename PHT> struct NTSYS  // For members: alignas(sizeof(PHT))
{

template<typename T, typename H=uint> using MPTR = TSW< (sizeof(PHT)==sizeof(void*)), T* ,SPTR<T,H> >::T;   // SPTR is too unstable to be used by default

using SIZE_T    = decltype(TypeToUnsigned<PHT>());  //  MPTR<uint,   PHT>;  // PHT; //TSW<sizeof(PHT) == sizeof(uint64), uint64, uint32>::T;    // Use direct type instead of MPTR<uint, PHT> to avoid unnecessary truncation
using SSIZE_T   = decltype(TypeToSigned<PHT>());  //  MPTR<sint,   PHT>;

using PVOID     = MPTR<void, PHT>;    // PHT aligned void* of current native type (32 bit for 32bit build)    // x32: You can assign a x32 void* and read it back but if some x64 code assigned a x64 value to it then you have to read it as UINT64 in your x32 code
//using ADDR      = TSW<sizeof(PHT) == sizeof(void*), MPTR<void, PHT>, MPTR<uint, PHT>>::T;   // Can hold an address of PHT size for current build (Unsigned if PHT is not of native void* size)
using HANDLE    = SIZE_T;    // Had to make it SIZE_T and unsafe because math operations is now applicable to it (Cannot make pointer constants from integer constants at compile time)

using VOID      = void;
using BOOL      = unsigned int;
using CHAR      = achar;
using BYTE      = uint8;
using WORD      = uint16;
using LONG      = int32;
using CCHAR     = uint8;   // Unsignned?
using UCHAR     = uint8;
using ULONG     = uint32;
using DWORD     = uint32;
using QWORD     = uint64;
using INT64     = sint64;
using UINT64    = uint64;
using USHORT    = uint16;
using WCHAR     = wchar;
using BOOLEAN   = BYTE;
using NTSTATUS  = ULONG;
using LONGLONG  = sint64;
using ULONGLONG = uint64;
using ULONG_PTR = SIZE_T;
using LONG_PTR  = SSIZE_T;
// All pointers must be aligned to PHT size to make a correct stack frame
using PSIZE_T   = MPTR<SIZE_T, PHT>;  //SIZE_T*;
using PDWORD    = MPTR<DWORD, PHT>;
using PULONG    = MPTR<ULONG, PHT>;   //ULONG*;
using PPVOID    = MPTR<PVOID, PHT>;
using PHANDLE   = MPTR<HANDLE, PHT>;
using PVOID64   = MPTR<void, uint64>;     // Only MSVC have __ptr64
using PWSTR     = MPTR<wchar, PHT>;
using LPSTR     = MPTR<achar, PHT>;
using PBYTE     = MPTR<uint8, PHT>;

using LCID           = DWORD;
using KPRIORITY      = LONG;
using ACCESS_MASK    = DWORD;
using LARGE_INTEGER  = sint64;          //    struct LARGE_INTEGER {LONGLONG QuadPart;};
using ULARGE_INTEGER = uint64;

using PLARGE_INTEGER  = MPTR<LARGE_INTEGER, PHT>;
using PULARGE_INTEGER = MPTR<ULARGE_INTEGER, PHT>;

SCVR uint32 MAX_PATH = 260;
SCVR uint32 MAXIMUM_WAIT_OBJECTS = 64;

SCVR HANDLE NtInvalidHandle  = ((HANDLE)(SIZE_T)-1);

SCVR HANDLE NtCurrentThread  = ((HANDLE)(SIZE_T)-2);   // HANDLE had to be made SIZE_T instead of PVOID because such constants is forbidden to create
SCVR HANDLE NtCurrentProcess = ((HANDLE)(SIZE_T)-1);   // HACK: static constexpr const void* ptr = __builtin_constant_p( reinterpret_cast<const void*>(0x1) ) ? reinterpret_cast<const void*>(0x1) : reinterpret_cast<const void*>(0x1)  ;

static _finline bool IsSuccess(NTSTATUS Status){return (LONG)Status >= 0;}        //#define NT_SUCCESS(Status)              ((NT::NTSTATUS)(Status) >= 0)
static _finline bool IsError(NTSTATUS Status){return (Status >> 30) == 3;}  //#define NT_ERROR(Status)                ((((NT::NTSTATUS)(Status)) >> 30) == 3)
//============================================================================================================
//                                          CORE FUNCTIONS
//============================================================================================================
enum EMFlags
{
// Memory protection flags
 PAGE_NOACCESS          = 0x00000001,   // Disables all access to the committed region of pages. An attempt to read from, write to, or execute the committed region results in an access violation.
 PAGE_READONLY          = 0x00000002,   // Enables read-only access to the committed region of pages. An attempt to write to the committed region results in an access violation.
 PAGE_READWRITE         = 0x00000004,   // Enables read-only or read/write access to the committed region of pages.
 PAGE_WRITECOPY         = 0x00000008,   // Enables read-only or copy-on-write access to a mapped view of a file mapping object. An attempt to write to a committed copy-on-write page results in a private copy of the page being made for the process. The private page is marked as PAGE_READWRITE, and the change is written to the new page.
 PAGE_EXECUTE           = 0x00000010,   // Enables execute access to the committed region of pages. An attempt to write to the committed region results in an access violation.
 PAGE_EXECUTE_READ      = 0x00000020,   // Enables execute or read-only access to the committed region of pages. An attempt to write to the committed region results in an access violation.
 PAGE_EXECUTE_READWRITE = 0x00000040,   // Enables execute, read-only, or read/write access to the committed region of pages.
 PAGE_EXECUTE_WRITECOPY = 0x00000080,   // Enables execute, read-only, or copy-on-write access to a mapped view of a file mapping object. An attempt to write to a committed copy-on-write page results in a private copy of the page being made for the process. The private page is marked as PAGE_EXECUTE_READWRITE, and the change is written to the new page.
 PAGE_GUARD             = 0x00000100,   // Pages in the region become guard pages. Any attempt to access a guard page causes the system to raise a STATUS_GUARD_PAGE_VIOLATION exception and turn off the guard page status.
 PAGE_NOCACHE           = 0x00000200,   // Sets all pages to be non-cachable. Applications should not use this attribute except when explicitly required for a device.
 PAGE_WRITECOMBINE      = 0x00000400,   // Sets all pages to be write-combined. Applications should not use this attribute except when explicitly required for a device.

// Memory type flags (AllocationType)
 MEM_COMMIT             = 0x00001000,   // Indicates committed pages for which physical storage has been allocated, either in memory or in the paging file on disk.
 MEM_RESERVE            = 0x00002000,   // Indicates reserved pages where a range of the process's virtual address space is reserved without any physical storage being allocated.
 MEM_DECOMMIT           = 0x00004000,
 MEM_RELEASE            = 0x00008000,
 MEM_FREE               = 0x00010000,   // Indicates free pages not accessible to the calling process and available to be allocated.
 MEM_PRIVATE            = 0x00020000,   // Indicates that the memory pages within the region are private (that is, not shared by other processes).
 MEM_MAPPED             = 0x00040000,   // Indicates that the memory pages within the region are mapped into the view of a section.
 MEM_RESET              = 0x00080000,
 MEM_TOP_DOWN           = 0x00100000,
 MEM_WRITE_WATCH        = 0x00200000,
 MEM_PHYSICAL           = 0x00400000,
 MEM_ROTATE             = 0x00800000,
 MEM_LARGE_PAGES        = 0x20000000,
 MEM_4MB_PAGES          = 0x80000000,

// Mapped section types
 SEC_FILE               = 0x00800000,
 SEC_IMAGE              = 0x01000000,
 SEC_PROTECTED_IMAGE    = 0x02000000,
 SEC_RESERVE            = 0x04000000,
 SEC_COMMIT             = 0x08000000,
 SEC_NOCACHE            = 0x10000000,
 SEC_WRITECOMBINE       = 0x40000000,
 SEC_LARGE_PAGES        = 0x80000000,

 MEM_IMAGE              = SEC_IMAGE     // Indicates that the memory pages within the region are mapped into the view of an image section.
};

enum EFileFlg     // winnt.h
{
 FILE_SHARE_READ                       =  0x00000001,
 FILE_SHARE_WRITE                      =  0x00000002,
 FILE_SHARE_DELETE                     =  0x00000004,
 FILE_SHARE_VALID_FLAGS                =  0x00000007,

// File attribute values
 FILE_ATTRIBUTE_READONLY               =  0x00000001,
 FILE_ATTRIBUTE_HIDDEN                 =  0x00000002,
 FILE_ATTRIBUTE_SYSTEM                 =  0x00000004,

 FILE_ATTRIBUTE_DIRECTORY              =  0x00000010,
 FILE_ATTRIBUTE_ARCHIVE                =  0x00000020,
 FILE_ATTRIBUTE_DEVICE                 =  0x00000040,
 FILE_ATTRIBUTE_NORMAL                 =  0x00000080,

 FILE_ATTRIBUTE_TEMPORARY              =  0x00000100,
 FILE_ATTRIBUTE_SPARSE_FILE            =  0x00000200,
 FILE_ATTRIBUTE_REPARSE_POINT          =  0x00000400,
 FILE_ATTRIBUTE_COMPRESSED             =  0x00000800,

 FILE_ATTRIBUTE_OFFLINE                =  0x00001000,
 FILE_ATTRIBUTE_NOT_CONTENT_INDEXED    =  0x00002000,
 FILE_ATTRIBUTE_ENCRYPTED              =  0x00004000,

 FILE_ATTRIBUTE_INTEGRITY_STREAM       =  0x00008000,
 FILE_ATTRIBUTE_VIRTUAL                =  0x00010000,
 FILE_ATTRIBUTE_NO_SCRUB_DATA          =  0x00020000,

 FILE_ATTRIBUTE_EA                     =  0x00040000,
 FILE_ATTRIBUTE_PINNED                 =  0x00080000,
 FILE_ATTRIBUTE_UNPINNED               =  0x00100000,
 FILE_ATTRIBUTE_RECALL_ON_OPEN         =  0x00040000,
 FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS  =  0x00400000,

/*#if NTDDI_VERSION < NTDDI_WIN8
 FILE_ATTRIBUTE_VALID_FLAGS            =  0x00007fb7,
 FILE_ATTRIBUTE_VALID_SET_FLAGS        =  0x000031a7,
#elif NTDDI_VERSION < NTDDI_WIN10_RS2
 FILE_ATTRIBUTE_VALID_FLAGS            =  0x0002ffb7,
 FILE_ATTRIBUTE_VALID_SET_FLAGS        =  0x000231a7,
#else
 FILE_ATTRIBUTE_VALID_FLAGS            =  0x005affb7,
 FILE_ATTRIBUTE_VALID_SET_FLAGS        =  0x001a31a7,
#endif  */

// File create disposition values
 FILE_SUPERSEDE                        =  0x00000000,    // If the file already exists, replace it with the given file. If it does not, create the given file.
 FILE_OPEN                             =  0x00000001,    // If the file already exists, open it instead of creating a new file. If it does not, fail the request and do not create a new file.
 FILE_CREATE                           =  0x00000002,    // If the file already exists, fail the request and do not create or open the given file. If it does not, create the given file.
 FILE_OPEN_IF                          =  0x00000003,    // If the file already exists, open it. If it does not, create the given file.
 FILE_OVERWRITE                        =  0x00000004,    // If the file already exists, open it and overwrite it. If it does not, fail the request.
 FILE_OVERWRITE_IF                     =  0x00000005,    // If the file already exists, open it and overwrite it. If it does not, create the given file.
 FILE_MAXIMUM_DISPOSITION              =  0x00000005,

// File create/open option flags
 FILE_DIRECTORY_FILE                   =  0x00000001,    // The file being created or opened is a directory file. With this flag, the CreateDisposition parameter must be set to FILE_CREATE, FILE_OPEN, or FILE_OPEN_IF.
 FILE_WRITE_THROUGH                    =  0x00000002,    // Applications that write data to the file must actually transfer the data into the file before any requested write operation is considered complete. This flag is automatically set if the CreateOptions flag FILE_NO_INTERMEDIATE _BUFFERING is set.
 FILE_SEQUENTIAL_ONLY                  =  0x00000004,    // All accesses to the file are sequential.
 FILE_NO_INTERMEDIATE_BUFFERING        =  0x00000008,    // The file cannot be cached or buffered in a driver's internal buffers. This flag is incompatible with the DesiredAccess FILE_APPEND_DATA flag.

 FILE_SYNCHRONOUS_IO_ALERT             =  0x00000010,    // All operations on the file are performed synchronously. Any wait on behalf of the caller is subject to premature termination from alerts. This flag also causes the I/O system to maintain the file position context. If this flag is set, the DesiredAccess SYNCHRONIZE flag also must be set.
 FILE_SYNCHRONOUS_IO_NONALERT          =  0x00000020,    // All operations on the file are performed synchronously. Waits in the system to synchronize I/O queuing and completion are not subject to alerts. This flag also causes the I/O system to maintain the file position context. If this flag is set, the DesiredAccess SYNCHRONIZE flag also must be set.
 FILE_NON_DIRECTORY_FILE               =  0x00000040,    // The file being opened must not be a directory file or this call fails. The file object being opened can represent a data file, a logical, virtual, or physical device, or a volume.
 FILE_CREATE_TREE_CONNECTION           =  0x00000080,

 FILE_COMPLETE_IF_OPLOCKED             =  0x00000100,
 FILE_NO_EA_KNOWLEDGE                  =  0x00000200,
 FILE_OPEN_FOR_RECOVERY                =  0x00000400,
 FILE_RANDOM_ACCESS                    =  0x00000800,    // Accesses to the file can be random, so no sequential read-ahead operations should be performed on the file by FSDs or the system.

 FILE_DELETE_ON_CLOSE                  =  0x00001000,    // Delete the file when the last handle to it is passed to NtClose. If this flag is set, the DELETE flag must be set in the DesiredAccess parameter.
 FILE_OPEN_BY_FILE_ID                  =  0x00002000,
 FILE_OPEN_FOR_BACKUP_INTENT           =  0x00004000,    // The file is being opened for backup intent. Therefore, the system should check for certain access rights and grant the caller the appropriate access to the file before checking the DesiredAccess parameter against the file's security descriptor.
 FILE_NO_COMPRESSION                   =  0x00008000,

//#if NTDDI_VERSION >= NTDDI_WIN7
 FILE_OPEN_REQUIRING_OPLOCK            =  0x00010000,
 FILE_DISALLOW_EXCLUSIVE               =  0x00020000,
//#endif
//#if NTDDI_VERSION >= NTDDI_WIN8
 FILE_SESSION_AWARE                    =  0x00040000,
//#endif

 FILE_RESERVE_OPFILTER                 =  0x00100000,
 FILE_OPEN_REPARSE_POINT               =  0x00200000,   // Open a file with a reparse point and bypass normal reparse point processing for the file.
 FILE_OPEN_NO_RECALL                   =  0x00400000,
 FILE_OPEN_FOR_FREE_SPACE_QUERY        =  0x00800000,

 FILE_VALID_OPTION_FLAGS               =  0x00ffffff,
 FILE_VALID_PIPE_OPTION_FLAGS          =  0x00000032,
 FILE_VALID_MAILSLOT_OPTION_FLAGS      =  0x00000032,
 FILE_VALID_SET_FLAGS                  =  0x00000036,

// Named pipe type flags
 FILE_PIPE_BYTE_STREAM_TYPE            =  0x00000000,
 FILE_PIPE_MESSAGE_TYPE                =  0x00000001,
 FILE_PIPE_ACCEPT_REMOTE_CLIENTS       =  0x00000000,
 FILE_PIPE_REJECT_REMOTE_CLIENTS       =  0x00000002,
 FILE_PIPE_TYPE_VALID_MASK             =  0x00000003,

// Named pipe completion mode flags
 FILE_PIPE_QUEUE_OPERATION             =  0x00000000,
 FILE_PIPE_COMPLETE_OPERATION          =  0x00000001,

// Named pipe read mode flags
 FILE_PIPE_BYTE_STREAM_MODE            =  0x00000000,
 FILE_PIPE_MESSAGE_MODE                =  0x00000001,

// NamedPipeConfiguration flags
 FILE_PIPE_INBOUND                     =  0x00000000,
 FILE_PIPE_OUTBOUND                    =  0x00000001,
 FILE_PIPE_FULL_DUPLEX                 =  0x00000002,

// NamedPipeState flags
 FILE_PIPE_DISCONNECTED_STATE          =  0x00000001,
 FILE_PIPE_LISTENING_STATE             =  0x00000002,
 FILE_PIPE_CONNECTED_STATE             =  0x00000003,
 FILE_PIPE_CLOSING_STATE               =  0x00000004,

// NamedPipeEnd flags
 FILE_PIPE_CLIENT_END                  =  0x00000000,
 FILE_PIPE_SERVER_END                  =  0x00000001,


 FILE_READ_DATA                        =  0x00000001,    // file & pipe
 FILE_LIST_DIRECTORY                   =  0x00000001,    // directory      // Files in the directory can be listed.

 FILE_WRITE_DATA                       =  0x00000002,    // file & pipe
 FILE_ADD_FILE                         =  0x00000002,    // directory

 FILE_APPEND_DATA                      =  0x00000004,    // file
 FILE_ADD_SUBDIRECTORY                 =  0x00000004,    // directory
 FILE_CREATE_PIPE_INSTANCE             =  0x00000004,    // named pipe

 FILE_READ_EA                          =  0x00000008,    // file & directory

 FILE_WRITE_EA                         =  0x00000010,    // file & directory

 FILE_EXECUTE                          =  0x00000020,    // file       // Data can be read into memory from the file using system paging I/O.   // For NtCreateSection of executables
 FILE_TRAVERSE                         =  0x00000020,    // directory  // The directory can be traversed: that is, it can be part of the pathname of a file.   // For a directory, the right to traverse the directory.

 FILE_DELETE_CHILD                     =  0x00000040,    // directory

 FILE_READ_ATTRIBUTES                  =  0x00000080,    // all

 FILE_WRITE_ATTRIBUTES                 =  0x00000100,    // all

};

enum EFMisc1
{
//
//  The following are masks for the predefined standard access types
//
 DELETE                         = 0x00010000,   // The caller can delete the object.
 READ_CONTROL                   = 0x00020000,   // The caller can read the access control list (ACL) and ownership information for the file.
 WRITE_DAC                      = 0x00040000,   // The caller can change the discretionary access control list (DACL) information for the object.
 WRITE_OWNER                    = 0x00080000,   // The caller can change the ownership information for the file.
 SYNCHRONIZE                    = 0x00100000,   // The caller can perform a wait operation on the object. (For example, the object can be passed to WaitForMultipleObjects.)

 STANDARD_RIGHTS_REQUIRED       = 0x000F0000,

 STANDARD_RIGHTS_READ           = READ_CONTROL,
 STANDARD_RIGHTS_WRITE          = READ_CONTROL,
 STANDARD_RIGHTS_EXECUTE        = READ_CONTROL,

 STANDARD_RIGHTS_ALL            = 0x001F0000,

 SPECIFIC_RIGHTS_ALL            = 0x0000FFFF,
//
// AccessSystemAcl access type
//
 ACCESS_SYSTEM_SECURITY         = 0x01000000,
//
// MaximumAllowed access type
//
 MAXIMUM_ALLOWED                = 0x02000000,
//
//  These are the generic rights
//
 GENERIC_READ                   = 0x80000000,
 GENERIC_WRITE                  = 0x40000000,
 GENERIC_EXECUTE                = 0x20000000,
 GENERIC_ALL                    = 0x10000000,

 FILE_WRITE_TO_END_OF_FILE      = 0xffffffff,
 FILE_USE_FILE_POINTER_POSITION = 0xfffffffe,
};

// Define the various device characteristics flags
enum EFDefType
{
 FILE_REMOVABLE_MEDIA              = 0x00000001,
 FILE_READ_ONLY_DEVICE             = 0x00000002,
 FILE_FLOPPY_DISKETTE              = 0x00000004,
 FILE_WRITE_ONCE_MEDIA             = 0x00000008,
 FILE_REMOTE_DEVICE                = 0x00000010,
 FILE_DEVICE_IS_MOUNTED            = 0x00000020,
 FILE_VIRTUAL_VOLUME               = 0x00000040,
 FILE_AUTOGENERATED_DEVICE_NAME    = 0x00000080,
 FILE_DEVICE_SECURE_OPEN           = 0x00000100,
 FILE_CHARACTERISTIC_PNP_DEVICE    = 0x00000800,
 FILE_CHARACTERISTIC_TS_DEVICE     = 0x00001000,
 FILE_CHARACTERISTIC_WEBDAV_DEVICE = 0x00002000,
};

// Define the I/O status information return values for NtCreateFile/NtOpenFile
enum EFIOStatus
{
 FILE_SUPERSEDED                = 0x00000000,
 FILE_OPENED                    = 0x00000001,
 FILE_CREATED                   = 0x00000002,
 FILE_OVERWRITTEN               = 0x00000003,
 FILE_EXISTS                    = 0x00000004,
 FILE_DOES_NOT_EXIST            = 0x00000005,
};

enum EOFlags
{
 OBJ_INHERIT                       = 0x00000002,   // This handle can be inherited by child processes of the current process. irrelevant to device and intermediate drivers.
 OBJ_PERMANENT                     = 0x00000010,   // This flag only applies to objects that are named within the object manager. By default, such objects are deleted when all open handles to them are closed.
 OBJ_EXCLUSIVE                     = 0x00000020,   // Only a single handle can be open for this object.
 OBJ_CASE_INSENSITIVE              = 0x00000040,   // If this flag is specified, a case-insensitive comparison is used when matching the ObjectName parameter against the names of existing objects.
 OBJ_OPENIF                        = 0x00000080,   // If this flag is specified to a routine that creates objects, and that object already exists then the routine should open that object. Otherwise, the routine creating the object returns an NTSTATUS code of STATUS_OBJECT_NAME_COLLISION.
 OBJ_OPENLINK                      = 0x00000100,   // if the object is a symbolic link object, the routine should open the symbolic link object itself, rather than the object that the symbolic link refers to (which is the default behavior)
 OBJ_KERNEL_HANDLE                 = 0x00000200,   // Specifies that the handle can only be accessed in kernel mode.
 OBJ_FORCE_ACCESS_CHECK            = 0x00000400,   // The routine opening the handle should enforce all access checks for the object, even if the handle is being opened in kernel mode.
 OBJ_IGNORE_IMPERSONATED_DEVICEMAP = 0x00000800,   // Separate device maps exists for each user in the system, and users can manage their own device maps. Typically during impersonation, the impersonated user's device map would be used.
 OBJ_DONT_REPARSE                  = 0x00001000,   // If this flag is set, no reparse points will be followed when parsing the name of the associated object. If any reparses are encountered the attempt will fail and return an STATUS_REPARSE_POINT_ENCOUNTERED result.
 OBJ_VALID_ATTRIBUTES              = 0x00001FF2,   // Reserved

 OBJ_PATH_GLOBAL_NS                = 0x01000000,   // FRAMEWORK: Prefix the object path with '\\GLOBAL??\\'
 OBJ_PATH_PARSE_DOTS               = 0x02000000,   // FRAMEWORK: Process '.' and '..' as POSIX defines
 OBJ_EXTRA_MASK                    = 0xFFF00000    // FFF only, Reserving for some extra too
};

//Source: http://processhacker.sourceforge.net
enum FILE_INFORMATION_CLASS
{
 FileDirectoryInformation = 1,            // FILE_DIRECTORY_INFORMATION
 FileFullDirectoryInformation,            // FILE_FULL_DIR_INFORMATION
 FileBothDirectoryInformation,            // FILE_BOTH_DIR_INFORMATION
 FileBasicInformation,                    // FILE_BASIC_INFORMATION
 FileStandardInformation,                 // FILE_STANDARD_INFORMATION
 FileInternalInformation,                 // FILE_INTERNAL_INFORMATION
 FileEaInformation,                       // FILE_EA_INFORMATION
 FileAccessInformation,                   // FILE_ACCESS_INFORMATION
 FileNameInformation,                     // FILE_NAME_INFORMATION
 FileRenameInformation,                   // FILE_RENAME_INFORMATION // 10
 FileLinkInformation,                     // FILE_LINK_INFORMATION
 FileNamesInformation,                    // FILE_NAMES_INFORMATION
 FileDispositionInformation,              // FILE_DISPOSITION_INFORMATION
 FilePositionInformation,                 // FILE_POSITION_INFORMATION
 FileFullEaInformation,                   // FILE_FULL_EA_INFORMATION
 FileModeInformation,                     // FILE_MODE_INFORMATION
 FileAlignmentInformation,                // FILE_ALIGNMENT_INFORMATION
 FileAllInformation,                      // FILE_ALL_INFORMATION
 FileAllocationInformation,               // FILE_ALLOCATION_INFORMATION
 FileEndOfFileInformation,                // FILE_END_OF_FILE_INFORMATION // 20
 FileAlternateNameInformation,            // FILE_NAME_INFORMATION
 FileStreamInformation,                   // FILE_STREAM_INFORMATION
 FilePipeInformation,                     // FILE_PIPE_INFORMATION
 FilePipeLocalInformation,                // FILE_PIPE_LOCAL_INFORMATION
 FilePipeRemoteInformation,               // FILE_PIPE_REMOTE_INFORMATION
 FileMailslotQueryInformation,            // FILE_MAILSLOT_QUERY_INFORMATION
 FileMailslotSetInformation,              // FILE_MAILSLOT_SET_INFORMATION
 FileCompressionInformation,              // FILE_COMPRESSION_INFORMATION
 FileObjectIdInformation,                 // FILE_OBJECTID_INFORMATION
 FileCompletionInformation,               // FILE_COMPLETION_INFORMATION // 30
 FileMoveClusterInformation,              // FILE_MOVE_CLUSTER_INFORMATION
 FileQuotaInformation,                    // FILE_QUOTA_INFORMATION
 FileReparsePointInformation,             // FILE_REPARSE_POINT_INFORMATION
 FileNetworkOpenInformation,              // FILE_NETWORK_OPEN_INFORMATION
 FileAttributeTagInformation,             // FILE_ATTRIBUTE_TAG_INFORMATION
 FileTrackingInformation,                 // FILE_TRACKING_INFORMATION
 FileIdBothDirectoryInformation,          // FILE_ID_BOTH_DIR_INFORMATION
 FileIdFullDirectoryInformation,          // FILE_ID_FULL_DIR_INFORMATION
 FileValidDataLengthInformation,          // FILE_VALID_DATA_LENGTH_INFORMATION
 FileShortNameInformation,                // FILE_NAME_INFORMATION // 40
 FileIoCompletionNotificationInformation, // FILE_IO_COMPLETION_NOTIFICATION_INFORMATION // since VISTA
 FileIoStatusBlockRangeInformation,       // FILE_IOSTATUSBLOCK_RANGE_INFORMATION
 FileIoPriorityHintInformation,           // FILE_IO_PRIORITY_HINT_INFORMATION
 FileSfioReserveInformation,              // FILE_SFIO_RESERVE_INFORMATION
 FileSfioVolumeInformation,               // FILE_SFIO_VOLUME_INFORMATION
 FileHardLinkInformation,                 // FILE_LINKS_INFORMATION            // since VISTA
 FileProcessIdsUsingFileInformation,      // FILE_PROCESS_IDS_USING_FILE_INFORMATION
 FileNormalizedNameInformation,           // FILE_NAME_INFORMATION
 FileNetworkPhysicalNameInformation,      // FILE_NETWORK_PHYSICAL_NAME_INFORMATION
 FileIdGlobalTxDirectoryInformation,      // FILE_ID_GLOBAL_TX_DIR_INFORMATION // since WIN7 // 50
 FileIsRemoteDeviceInformation,           // FILE_IS_REMOTE_DEVICE_INFORMATION
 FileUnusedInformation,
 FileNumaNodeInformation,                 // FILE_NUMA_NODE_INFORMATION
 FileStandardLinkInformation,             // FILE_STANDARD_LINK_INFORMATION
 FileRemoteProtocolInformation,           // FILE_REMOTE_PROTOCOL_INFORMATION
 FileRenameInformationBypassAccessCheck,  // (kernel-mode only); FILE_RENAME_INFORMATION // since WIN8
 FileLinkInformationBypassAccessCheck,    // (kernel-mode only); FILE_LINK_INFORMATION
 FileVolumeNameInformation,               // FILE_VOLUME_NAME_INFORMATION
 FileIdInformation,                       // FILE_ID_INFORMATION
 FileIdExtdDirectoryInformation,          // FILE_ID_EXTD_DIR_INFORMATION
 FileReplaceCompletionInformation,        // FILE_COMPLETION_INFORMATION // since WINBLUE
 FileHardLinkFullIdInformation,           // FILE_LINK_ENTRY_FULL_ID_INFORMATION
 FileIdExtdBothDirectoryInformation,      // FILE_ID_EXTD_BOTH_DIR_INFORMATION // since THRESHOLD
 FileDispositionInformationEx,            // FILE_DISPOSITION_INFO_EX // since REDSTONE
 FileRenameInformationEx,                 // FILE_RENAME_INFORMATION_EX
 FileRenameInformationExBypassAccessCheck,
 FileDesiredStorageClassInformation,      // FILE_DESIRED_STORAGE_CLASS_INFORMATION // since REDSTONE2
 FileStatInformation,                     // FILE_STAT_INFORMATION

 FileMemoryPartitionInformation,          // 69
 FileStatLxInformation,                   // 70
 FileCaseSensitiveInformation,            // 71
 FileLinkInformationEx,                   // 72
 FileLinkInformationExBypassAccessCheck,  // 73
 FileStorageReserveIdInformation,         // 74

 FileMaximumInformation
};
using PFILE_INFORMATION_CLASS = MPTR<FILE_INFORMATION_CLASS, PHT>;

enum SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation, // q: SYSTEM_BASIC_INFORMATION
    SystemProcessorInformation, // q: SYSTEM_PROCESSOR_INFORMATION
    SystemPerformanceInformation, // q: SYSTEM_PERFORMANCE_INFORMATION
    SystemTimeOfDayInformation, // q: SYSTEM_TIMEOFDAY_INFORMATION
    SystemPathInformation, // not implemented
    SystemProcessInformation, // q: SYSTEM_PROCESS_INFORMATION
    SystemCallCountInformation, // q: SYSTEM_CALL_COUNT_INFORMATION
    SystemDeviceInformation, // q: SYSTEM_DEVICE_INFORMATION
    SystemProcessorPerformanceInformation, // q: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
    SystemFlagsInformation, // q: SYSTEM_FLAGS_INFORMATION
    SystemCallTimeInformation, // not implemented // SYSTEM_CALL_TIME_INFORMATION // 10
    SystemModuleInformation, // q: RTL_PROCESS_MODULES
    SystemLocksInformation, // q: RTL_PROCESS_LOCKS
    SystemStackTraceInformation, // q: RTL_PROCESS_BACKTRACES
    SystemPagedPoolInformation, // not implemented
    SystemNonPagedPoolInformation, // not implemented
    SystemHandleInformation, // q: SYSTEM_HANDLE_INFORMATION
    SystemObjectInformation, // q: SYSTEM_OBJECTTYPE_INFORMATION mixed with SYSTEM_OBJECT_INFORMATION
    SystemPageFileInformation, // q: SYSTEM_PAGEFILE_INFORMATION
    SystemVdmInstemulInformation, // q
    SystemVdmBopInformation, // not implemented // 20
    SystemFileCacheInformation, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypeSystemCache)
    SystemPoolTagInformation, // q: SYSTEM_POOLTAG_INFORMATION
    SystemInterruptInformation, // q: SYSTEM_INTERRUPT_INFORMATION
    SystemDpcBehaviorInformation, // q: SYSTEM_DPC_BEHAVIOR_INFORMATION; s: SYSTEM_DPC_BEHAVIOR_INFORMATION (requires SeLoadDriverPrivilege)
    SystemFullMemoryInformation, // not implemented
    SystemLoadGdiDriverInformation, // s (kernel-mode only)
    SystemUnloadGdiDriverInformation, // s (kernel-mode only)
    SystemTimeAdjustmentInformation, // q: SYSTEM_QUERY_TIME_ADJUST_INFORMATION; s: SYSTEM_SET_TIME_ADJUST_INFORMATION (requires SeSystemtimePrivilege)
    SystemSummaryMemoryInformation, // not implemented
    SystemMirrorMemoryInformation, // s (requires license value "Kernel-MemoryMirroringSupported") (requires SeShutdownPrivilege) // 30
    SystemPerformanceTraceInformation, // q; s: (type depends on EVENT_TRACE_INFORMATION_CLASS)
    SystemObsolete0, // not implemented
    SystemExceptionInformation, // q: SYSTEM_EXCEPTION_INFORMATION
    SystemCrashDumpStateInformation, // s (requires SeDebugPrivilege)
    SystemKernelDebuggerInformation, // q: SYSTEM_KERNEL_DEBUGGER_INFORMATION
    SystemContextSwitchInformation, // q: SYSTEM_CONTEXT_SWITCH_INFORMATION
    SystemRegistryQuotaInformation, // q: SYSTEM_REGISTRY_QUOTA_INFORMATION; s (requires SeIncreaseQuotaPrivilege)
    SystemExtendServiceTableInformation, // s (requires SeLoadDriverPrivilege) // loads win32k only
    SystemPrioritySeperation, // s (requires SeTcbPrivilege)
    SystemVerifierAddDriverInformation, // s (requires SeDebugPrivilege) // 40
    SystemVerifierRemoveDriverInformation, // s (requires SeDebugPrivilege)
    SystemProcessorIdleInformation, // q: SYSTEM_PROCESSOR_IDLE_INFORMATION
    SystemLegacyDriverInformation, // q: SYSTEM_LEGACY_DRIVER_INFORMATION
    SystemCurrentTimeZoneInformation, // q
    SystemLookasideInformation, // q: SYSTEM_LOOKASIDE_INFORMATION
    SystemTimeSlipNotification, // s (requires SeSystemtimePrivilege)
    SystemSessionCreate, // not implemented
    SystemSessionDetach, // not implemented
    SystemSessionInformation, // not implemented
    SystemRangeStartInformation, // q: SYSTEM_RANGE_START_INFORMATION // 50
    SystemVerifierInformation, // q: SYSTEM_VERIFIER_INFORMATION; s (requires SeDebugPrivilege)
    SystemVerifierThunkExtend, // s (kernel-mode only)
    SystemSessionProcessInformation, // q: SYSTEM_SESSION_PROCESS_INFORMATION
    SystemLoadGdiDriverInSystemSpace, // s (kernel-mode only) (same as SystemLoadGdiDriverInformation)
    SystemNumaProcessorMap, // q
    SystemPrefetcherInformation, // q: PREFETCHER_INFORMATION; s: PREFETCHER_INFORMATION // PfSnQueryPrefetcherInformation
    SystemExtendedProcessInformation, // q: SYSTEM_PROCESS_INFORMATION
    SystemRecommendedSharedDataAlignment, // q
    SystemComPlusPackage, // q; s
    SystemNumaAvailableMemory, // 60
    SystemProcessorPowerInformation, // q: SYSTEM_PROCESSOR_POWER_INFORMATION
    SystemEmulationBasicInformation, // q
    SystemEmulationProcessorInformation,
    SystemExtendedHandleInformation, // q: SYSTEM_HANDLE_INFORMATION_EX
    SystemLostDelayedWriteInformation, // q: ULONG
    SystemBigPoolInformation, // q: SYSTEM_BIGPOOL_INFORMATION
    SystemSessionPoolTagInformation, // q: SYSTEM_SESSION_POOLTAG_INFORMATION
    SystemSessionMappedViewInformation, // q: SYSTEM_SESSION_MAPPED_VIEW_INFORMATION
    SystemHotpatchInformation, // q; s
    SystemObjectSecurityMode, // q // 70
    SystemWatchdogTimerHandler, // s (kernel-mode only)
    SystemWatchdogTimerInformation, // q (kernel-mode only); s (kernel-mode only)
    SystemLogicalProcessorInformation, // q: SYSTEM_LOGICAL_PROCESSOR_INFORMATION
    SystemWow64SharedInformationObsolete, // not implemented
    SystemRegisterFirmwareTableInformationHandler, // s (kernel-mode only)
    SystemFirmwareTableInformation, // SYSTEM_FIRMWARE_TABLE_INFORMATION
    SystemModuleInformationEx, // q: RTL_PROCESS_MODULE_INFORMATION_EX
    SystemVerifierTriageInformation, // not implemented
    SystemSuperfetchInformation, // q; s: SUPERFETCH_INFORMATION // PfQuerySuperfetchInformation
    SystemMemoryListInformation, // q: SYSTEM_MEMORY_LIST_INFORMATION; s: SYSTEM_MEMORY_LIST_COMMAND (requires SeProfileSingleProcessPrivilege) // 80
    SystemFileCacheInformationEx, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (same as SystemFileCacheInformation)
    SystemThreadPriorityClientIdInformation, // s: SYSTEM_THREAD_CID_PRIORITY_INFORMATION (requires SeIncreaseBasePriorityPrivilege)
    SystemProcessorIdleCycleTimeInformation, // q: SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION[]
    SystemVerifierCancellationInformation, // not implemented // name:wow64:whNT32QuerySystemVerifierCancellationInformation
    SystemProcessorPowerInformationEx, // not implemented
    SystemRefTraceInformation, // q; s: SYSTEM_REF_TRACE_INFORMATION // ObQueryRefTraceInformation
    SystemSpecialPoolInformation, // q; s (requires SeDebugPrivilege) // MmSpecialPoolTag, then MmSpecialPoolCatchOverruns != 0
    SystemProcessIdInformation, // q: SYSTEM_PROCESS_ID_INFORMATION
    SystemErrorPortInformation, // s (requires SeTcbPrivilege)
    SystemBootEnvironmentInformation, // q: SYSTEM_BOOT_ENVIRONMENT_INFORMATION // 90
    SystemHypervisorInformation, // q; s (kernel-mode only)
    SystemVerifierInformationEx, // q; s: SYSTEM_VERIFIER_INFORMATION_EX
    SystemTimeZoneInformation, // s (requires SeTimeZonePrivilege)
    SystemImageFileExecutionOptionsInformation, // s: SYSTEM_IMAGE_FILE_EXECUTION_OPTIONS_INFORMATION (requires SeTcbPrivilege)
    SystemCoverageInformation, // q; s // name:wow64:whNT32QuerySystemCoverageInformation; ExpCovQueryInformation
    SystemPrefetchPatchInformation, // not implemented
    SystemVerifierFaultsInformation, // s (requires SeDebugPrivilege)
    SystemSystemPartitionInformation, // q: SYSTEM_SYSTEM_PARTITION_INFORMATION
    SystemSystemDiskInformation, // q: SYSTEM_SYSTEM_DISK_INFORMATION
    SystemProcessorPerformanceDistribution, // q: SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION // 100
    SystemNumaProximityNodeInformation, // q
    SystemDynamicTimeZoneInformation, // q; s (requires SeTimeZonePrivilege)
    SystemCodeIntegrityInformation, // q: SYSTEM_CODEINTEGRITY_INFORMATION // SeCodeIntegrityQueryInformation
    SystemProcessorMicrocodeUpdateInformation, // s
    SystemProcessorBrandString, // q // HaliQuerySystemInformation -> HalpGetProcessorBrandString, info class 23
    SystemVirtualAddressInformation, // q: SYSTEM_VA_LIST_INFORMATION[]; s: SYSTEM_VA_LIST_INFORMATION[] (requires SeIncreaseQuotaPrivilege) // MmQuerySystemVaInformation
    SystemLogicalProcessorAndGroupInformation, // q: SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX // since WIN7 // KeQueryLogicalProcessorRelationship
    SystemProcessorCycleTimeInformation, // q: SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION[]
    SystemStoreInformation, // q; s // SmQueryStoreInformation
    SystemRegistryAppendString, // s: SYSTEM_REGISTRY_APPEND_STRING_PARAMETERS // 110
    SystemAitSamplingValue, // s: ULONG (requires SeProfileSingleProcessPrivilege)
    SystemVhdBootInformation, // q: SYSTEM_VHD_BOOT_INFORMATION
    SystemCpuQuotaInformation, // q; s // PsQueryCpuQuotaInformation
    SystemNativeBasicInformation, // not implemented
    SystemSpare1, // not implemented
    SystemLowPriorityIoInformation, // q: SYSTEM_LOW_PRIORITY_IO_INFORMATION
    SystemTpmBootEntropyInformation, // q: TPM_BOOT_ENTROPY_NT_RESULT // ExQueryTpmBootEntropyInformation
    SystemVerifierCountersInformation, // q: SYSTEM_VERIFIER_COUNTERS_INFORMATION
    SystemPagedPoolInformationEx, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypePagedPool)
    SystemSystemPtesInformationEx, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypeSystemPtes) // 120
    SystemNodeDistanceInformation, // q
    SystemAcpiAuditInformation, // q: SYSTEM_ACPI_AUDIT_INFORMATION // HaliQuerySystemInformation -> HalpAuditQueryResults, info class 26
    SystemBasicPerformanceInformation, // q: SYSTEM_BASIC_PERFORMANCE_INFORMATION // name:wow64:whNtQuerySystemInformation_SystemBasicPerformanceInformation
    SystemQueryPerformanceCounterInformation, // q: SYSTEM_QUERY_PERFORMANCE_COUNTER_INFORMATION // since WIN7 SP1
    SystemSessionBigPoolInformation, // q: SYSTEM_SESSION_POOLTAG_INFORMATION // since WIN8
    SystemBootGraphicsInformation, // q; s: SYSTEM_BOOT_GRAPHICS_INFORMATION (kernel-mode only)
    SystemScrubPhysicalMemoryInformation, // q; s: MEMORY_SCRUB_INFORMATION
    SystemBadPageInformation,
    SystemProcessorProfileControlArea, // q; s: SYSTEM_PROCESSOR_PROFILE_CONTROL_AREA
    SystemCombinePhysicalMemoryInformation, // s: MEMORY_COMBINE_INFORMATION, MEMORY_COMBINE_INFORMATION_EX, MEMORY_COMBINE_INFORMATION_EX2 // 130
    SystemEntropyInterruptTimingCallback,
    SystemConsoleInformation, // q: SYSTEM_CONSOLE_INFORMATION
    SystemPlatformBinaryInformation, // q: SYSTEM_PLATFORM_BINARY_INFORMATION
    SystemThrottleNotificationInformation,
    SystemHypervisorProcessorCountInformation, // q: SYSTEM_HYPERVISOR_PROCESSOR_COUNT_INFORMATION
    SystemDeviceDataInformation, // q: SYSTEM_DEVICE_DATA_INFORMATION
    SystemDeviceDataEnumerationInformation,
    SystemMemoryTopologyInformation, // q: SYSTEM_MEMORY_TOPOLOGY_INFORMATION
    SystemMemoryChannelInformation, // q: SYSTEM_MEMORY_CHANNEL_INFORMATION
    SystemBootLogoInformation, // q: SYSTEM_BOOT_LOGO_INFORMATION // 140
    SystemProcessorPerformanceInformationEx, // q: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX // since WINBLUE
    SystemSpare0,
    SystemSecureBootPolicyInformation, // q: SYSTEM_SECUREBOOT_POLICY_INFORMATION
    SystemPageFileInformationEx, // q: SYSTEM_PAGEFILE_INFORMATION_EX
    SystemSecureBootInformation, // q: SYSTEM_SECUREBOOT_INFORMATION
    SystemEntropyInterruptTimingRawInformation,
    SystemPortableWorkspaceEfiLauncherInformation, // q: SYSTEM_PORTABLE_WORKSPACE_EFI_LAUNCHER_INFORMATION
    SystemFullProcessInformation, // q: SYSTEM_PROCESS_INFORMATION with SYSTEM_PROCESS_INFORMATION_EXTENSION (requires admin)
    SystemKernelDebuggerInformationEx, // q: SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX
    SystemBootMetadataInformation, // 150
    SystemSoftRebootInformation,
    SystemElamCertificateInformation, // s: SYSTEM_ELAM_CERTIFICATE_INFORMATION
    SystemOfflineDumpConfigInformation,
    SystemProcessorFeaturesInformation, // q: SYSTEM_PROCESSOR_FEATURES_INFORMATION
    SystemRegistryReconciliationInformation,
    SystemEdidInformation,
    SystemManufacturingInformation, // q: SYSTEM_MANUFACTURING_INFORMATION // since THRESHOLD
    SystemEnergyEstimationConfigInformation, // q: SYSTEM_ENERGY_ESTIMATION_CONFIG_INFORMATION
    SystemHypervisorDetailInformation, // q: SYSTEM_HYPERVISOR_DETAIL_INFORMATION
    SystemProcessorCycleStatsInformation, // q: SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION // 160
    SystemVmGenerationCountInformation,
    SystemTrustedPlatformModuleInformation, // q: SYSTEM_TPM_INFORMATION
    SystemKernelDebuggerFlags,
    SystemCodeIntegrityPolicyInformation, // q: SYSTEM_CODEINTEGRITYPOLICY_INFORMATION
    SystemIsolatedUserModeInformation, // q: SYSTEM_ISOLATED_USER_MODE_INFORMATION
    SystemHardwareSecurityTestInterfaceResultsInformation,
    SystemSingleModuleInformation, // q: SYSTEM_SINGLE_MODULE_INFORMATION
    SystemAllowedCpuSetsInformation,
    SystemDmaProtectionInformation, // q: SYSTEM_DMA_PROTECTION_INFORMATION
    SystemInterruptCpuSetsInformation, // q: SYSTEM_INTERRUPT_CPU_SET_INFORMATION // 170
    SystemSecureBootPolicyFullInformation, // q: SYSTEM_SECUREBOOT_POLICY_FULL_INFORMATION
    SystemCodeIntegrityPolicyFullInformation,
    SystemAffinitizedInterruptProcessorInformation,
    SystemRootSiloInformation, // q: SYSTEM_ROOT_SILO_INFORMATION
    SystemCpuSetInformation, // q: SYSTEM_CPU_SET_INFORMATION // since THRESHOLD2
    SystemCpuSetTagInformation, // q: SYSTEM_CPU_SET_TAG_INFORMATION
    SystemWin32WerStartCallout,
    SystemSecureKernelProfileInformation, // q: SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION
    SystemCodeIntegrityPlatformManifestInformation, // q: SYSTEM_SECUREBOOT_PLATFORM_MANIFEST_INFORMATION // since REDSTONE
    SystemInterruptSteeringInformation, // 180
    SystemSupportedProcessorArchitectures,
    SystemMemoryUsageInformation, // q: SYSTEM_MEMORY_USAGE_INFORMATION
    SystemCodeIntegrityCertificateInformation, // q: SYSTEM_CODEINTEGRITY_CERTIFICATE_INFORMATION
    SystemPhysicalMemoryInformation, // q: SYSTEM_PHYSICAL_MEMORY_INFORMATION // since REDSTONE2
    SystemControlFlowTransition,
    SystemKernelDebuggingAllowed,
    SystemActivityModerationExeState, // SYSTEM_ACTIVITY_MODERATION_EXE_STATE
    SystemActivityModerationUserSettings, // SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS
    SystemCodeIntegrityPoliciesFullInformation,
    SystemCodeIntegrityUnlockInformation, // SYSTEM_CODEINTEGRITY_UNLOCK_INFORMATION // 190
    SystemIntegrityQuotaInformation,
    SystemFlushInformation, // q: SYSTEM_FLUSH_INFORMATION
    MaxSystemInfoClass
};
using PSYSTEM_INFORMATION_CLASS = MPTR<SYSTEM_INFORMATION_CLASS, PHT>;

enum OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation, // OBJECT_BASIC_INFORMATION
    ObjectNameInformation, // OBJECT_NAME_INFORMATION
    ObjectTypeInformation, // OBJECT_TYPE_INFORMATION
    ObjectTypesInformation, // OBJECT_TYPES_INFORMATION
    ObjectHandleFlagInformation, // OBJECT_HANDLE_FLAG_INFORMATION
    ObjectSessionInformation,
    ObjectSessionObjectInformation,
    MaxObjectInfoClass
};
using POBJECT_INFORMATION_CLASS = MPTR<OBJECT_INFORMATION_CLASS, PHT>;

enum MEMORY_INFORMATION_CLASS
{
 MemoryBasicInformation,           // MEMORY_BASIC_INFORMATION
 MemoryWorkingSetInformation,      // MEMORY_WORKING_SET_INFORMATION
 MemoryMappedFilenameInformation,  // UNICODE_STRING
 MemoryRegionInformation,          // MEMORY_REGION_INFORMATION
 MemoryWorkingSetExInformation,    // MEMORY_WORKING_SET_EX_INFORMATION
 MemorySharedCommitInformation,    // MEMORY_SHARED_COMMIT_INFORMATION
 MemoryImageInformation,           // MEMORY_IMAGE_INFORMATION
 MemoryRegionInformationEx,        
 MemoryPrivilegedBasicInformation,
 MemoryEnclaveImageInformation,    // MEMORY_ENCLAVE_IMAGE_INFORMATION // since REDSTONE3
 MemoryBasicInformationCapped,     // 10
 MemoryPhysicalContiguityInformation, // MEMORY_PHYSICAL_CONTIGUITY_INFORMATION // since 20H1
 MemoryBadInformation,             // since WIN11
 MemoryBadInformationAllProcesses, // since 22H1
 MaxMemoryInfoClass
};

enum SECTION_INFORMATION_CLASS
{
 SectionBasicInformation,
 SectionImageInformation,
 SectionRelocationInformation,    // name:wow64:whNtQuerySection_SectionRelocationInformation
 SectionOriginalBaseInformation,  // PVOID BaseAddress
 SectionInternalImageInformation, // SECTION_INTERNAL_IMAGE_INFORMATION // since REDSTONE2
 MaxSectionInfoClass
};

struct MEMORY_BASIC_INFORMATION
{
  PVOID  BaseAddress;
  PVOID  AllocationBase;
  DWORD  AllocationProtect;
  WORD   PartitionId;
  SIZE_T RegionSize;
  DWORD  State;
  DWORD  Protect;
  DWORD  Type;
};

struct MEMORY_REGION_INFORMATION
{
 PVOID AllocationBase;
 ULONG AllocationProtect;
 union
 {
  ULONG RegionType;
  struct
  {
   ULONG Private : 1;
   ULONG MappedDataFile : 1;
   ULONG MappedImage : 1;
   ULONG MappedPageFile : 1;
   ULONG MappedPhysical : 1;
   ULONG DirectMapped : 1;
   ULONG Reserved : 26;
  } s;
 } u;
 SIZE_T RegionSize;
 SIZE_T CommitSize;
};

struct MEMORY_IMAGE_INFORMATION
{
 PVOID ImageBase;
 SIZE_T SizeOfImage;
 union
 {
  ULONG ImageFlags;
  struct
  {
   ULONG ImagePartialMap : 1;
   ULONG ImageNotExecutable : 1;
   ULONG Reserved : 30;
  };
 };
};

struct FILE_FULL_EA_INFORMATION 
{
 ULONG  NextEntryOffset;
 UCHAR  Flags;
 UCHAR  EaNameLength;
 USHORT EaValueLength;
 CHAR   EaName[1];
};

// FILE_DIRECTORY_INFORMATION definition from Windows DDK. Used by NtQueryDirectoryFile, supported since Windows NT 4.0 (probably).
struct FILE_DIRECTORY_INFORMATION
{
 ULONG NextEntryOffset;
 ULONG FileIndex;
 LARGE_INTEGER CreationTime;
 LARGE_INTEGER LastAccessTime;
 LARGE_INTEGER LastWriteTime;
 LARGE_INTEGER ChangeTime;
 LARGE_INTEGER EndOfFile;
 LARGE_INTEGER AllocationSize;
 ULONG FileAttributes;
 ULONG FileNameLength;
 WCHAR FileName[1];
};

struct FILE_FULL_DIR_INFORMATION
{
 ULONG NextEntryOffset;
 ULONG FileIndex;
 LARGE_INTEGER CreationTime;
 LARGE_INTEGER LastAccessTime;
 LARGE_INTEGER LastWriteTime;
 LARGE_INTEGER ChangeTime;
 LARGE_INTEGER EndOfFile;
 LARGE_INTEGER AllocationSize;
 ULONG FileAttributes;
 ULONG FileNameLength;
 ULONG EaSize;
 WCHAR FileName[1];
};

struct FILE_ID_FULL_DIR_INFORMATION
{
 ULONG NextEntryOffset;
 ULONG FileIndex;
 LARGE_INTEGER CreationTime;
 LARGE_INTEGER LastAccessTime;
 LARGE_INTEGER LastWriteTime;
 LARGE_INTEGER ChangeTime;
 LARGE_INTEGER EndOfFile;
 LARGE_INTEGER AllocationSize;
 ULONG FileAttributes;
 ULONG FileNameLength;
 ULONG EaSize;
 LARGE_INTEGER FileId;
 WCHAR FileName[1];
};

struct FILE_BOTH_DIR_INFORMATION
{
 ULONG NextEntryOffset;
 ULONG FileIndex;
 LARGE_INTEGER CreationTime;
 LARGE_INTEGER LastAccessTime;
 LARGE_INTEGER LastWriteTime;
 LARGE_INTEGER ChangeTime;
 LARGE_INTEGER EndOfFile;
 LARGE_INTEGER AllocationSize;
 ULONG FileAttributes;
 ULONG FileNameLength;
 ULONG EaSize;
 CCHAR ShortNameLength;
 WCHAR ShortName[12];
 WCHAR FileName[1];
};

struct FILE_ID_BOTH_DIR_INFORMATION
{
 ULONG NextEntryOffset;
 ULONG FileIndex;
 LARGE_INTEGER CreationTime;
 LARGE_INTEGER LastAccessTime;
 LARGE_INTEGER LastWriteTime;
 LARGE_INTEGER ChangeTime;
 LARGE_INTEGER EndOfFile;
 LARGE_INTEGER AllocationSize;
 ULONG FileAttributes;
 ULONG FileNameLength;
 ULONG EaSize;
 CCHAR ShortNameLength;
 WCHAR ShortName[12];
 LARGE_INTEGER FileId;
 WCHAR FileName[1];
};

struct FILE_NAMES_INFORMATION
{
 ULONG NextEntryOffset;
 ULONG FileIndex;
 ULONG FileNameLength;
 WCHAR FileName[1];
};

enum SECTION_INHERIT
{
 ViewShare = 1,
 ViewUnmap = 2
};

struct GUID
{
 uint32 Data1;
 uint16 Data2;
 uint16 Data3;
 uint8  Data4[ 8 ];
};
using PGUID = MPTR<GUID, PHT>;

struct SECTION_BASIC_INFORMATION
{
 PVOID BaseAddress;
 ULONG AllocationAttributes;
 LARGE_INTEGER MaximumSize;
};

struct SECTION_IMAGE_INFORMATION
{
    PVOID  TransferAddress; // Entry point
    ULONG  ZeroBits;
    SIZE_T MaximumStackSize;
    SIZE_T CommittedStackSize;
    ULONG  SubSystemType;
    union
    {
        struct
        {
            USHORT SubSystemMinorVersion;
            USHORT SubSystemMajorVersion;
        } s1;
        ULONG SubSystemVersion;
    } u1;
    union
    {
        struct
        {
            USHORT MajorOperatingSystemVersion;
            USHORT MinorOperatingSystemVersion;
        } s2;
        ULONG OperatingSystemVersion;
    } u2;
    USHORT ImageCharacteristics;
    USHORT DllCharacteristics;
    USHORT Machine;
    BOOLEAN ImageContainsCode;
    union
    {
        UCHAR ImageFlags;
        struct
        {
            UCHAR ComPlusNativeReady : 1;
            UCHAR ComPlusILOnly : 1;
            UCHAR ImageDynamicallyRelocated : 1;
            UCHAR ImageMappedFlat : 1;
            UCHAR BaseBelow4gb : 1;
            UCHAR ComPlusPrefer32bit : 1;
            UCHAR Reserved : 2;
        } s3;
    } u3;
    ULONG LoaderFlags;
    ULONG ImageFileSize;
    ULONG CheckSum;
};

struct SECTION_INTERNAL_IMAGE_INFORMATION
{
    SECTION_IMAGE_INFORMATION SectionInformation;
    union
    {
        ULONG ExtendedFlags;
        struct
        {
            ULONG ImageReturnFlowGuardEnabled : 1;
            ULONG ImageReturnFlowGuardStrict : 1;
            ULONG ImageExportSuppressionEnabled : 1;
            ULONG Reserved : 29;
        } s;
    } u;
};

enum ESectAccess
{
 SECTION_QUERY                = 0x0001,
 SECTION_MAP_WRITE            = 0x0002,
 SECTION_MAP_READ             = 0x0004,
 SECTION_MAP_EXECUTE          = 0x0008,
 SECTION_EXTEND_SIZE          = 0x0010,
 SECTION_MAP_EXECUTE_EXPLICIT = 0x0020, // not included in SECTION_ALL_ACCESS
 SECTION_ALL_ACCESS           = (STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_WRITE | SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_EXTEND_SIZE),
};

// TODO: Allow only MPTR<uint>, MPTR<uint32>, MPTR<uint64>
// NOTE: All pointers must go through MPTR

static NTSTATUS _scall NtProtectVirtualMemory(HANDLE ProcessHandle, PPVOID BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);

static NTSTATUS _scall NtAllocateVirtualMemory(HANDLE ProcessHandle, PPVOID BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);

static NTSTATUS _scall NtFreeVirtualMemory(HANDLE ProcessHandle, PPVOID BaseAddress, PSIZE_T RegionSize, ULONG FreeType);

static NTSTATUS _scall NtReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesRead);

static NTSTATUS _scall NtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesWritten);

static NTSTATUS _scall NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);


struct RTL_CRITICAL_SECTION
{
    PVOID DebugInfo;     // PRTL_CRITICAL_SECTION_DEBUG
    //  The following three fields control entering and exiting the critical section for the resource
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;        // from the thread's ClientId->UniqueThread
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;        // force size on 64-bit systems when packed
};
using PRTL_CRITICAL_SECTION = MPTR<RTL_CRITICAL_SECTION, PHT>;

struct UNICODE_STRING
{
 USHORT Length;
 USHORT MaximumLength;
 PWSTR  Buffer alignas(sizeof(PHT));

 void Set(wchar* str, uint len=0)
  {
   this->Buffer = str;
   if(!len)while(*str)len++;
   this->Length = USHORT(len * sizeof(wchar));
   this->MaximumLength = USHORT(len + sizeof(wchar));
  }
 void Set(const wchar* str, wchar* buf, uint offs=0)
  {
   this->Buffer = buf;
   for(;*str;str++)buf[offs++] = *str;
   this->Length = USHORT(offs * sizeof(wchar));
   this->MaximumLength = USHORT(offs + sizeof(wchar));
  }
};
using PUNICODE_STRING = MPTR<UNICODE_STRING, PHT>;


struct ANSI_STRING
{
 USHORT Length;
 USHORT MaximumLength;
 LPSTR  Buffer alignas(sizeof(PHT));
};
using PANSI_STRING = MPTR<ANSI_STRING, PHT>;

struct USER_STACK     // Same as INITIAL_TEB
{
 PVOID FixedStackBase;
 PVOID FixedStackLimit;
 PVOID ExpandableStackBase;
 PVOID ExpandableStackLimit;
 PVOID ExpandableStackBottom;
};
using PUSER_STACK = MPTR<USER_STACK, PHT>;

struct INITIAL_TEB
{
 struct
 {
  PVOID OldStackBase;
  PVOID OldStackLimit;
 } OldInitialTeb;
 PVOID StackBase;
 PVOID StackLimit;
 PVOID StackAllocationBase;
};
using PINITIAL_TEB = MPTR<INITIAL_TEB, PHT>;

struct IO_STATUS_BLOCK
{
 union
  {
   NTSTATUS Status;
   PVOID Pointer;
  };
 ULONG_PTR Information;
};
using PIO_STATUS_BLOCK = MPTR<IO_STATUS_BLOCK, PHT>;

using PIO_APC_ROUTINE = VOID (_scall *)(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved);     //        typedef VOID (NTAPI* PIO_APC_ROUTINE)(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved);

struct OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
};
using POBJECT_ATTRIBUTES = MPTR<OBJECT_ATTRIBUTES, PHT>;

struct FILE_BASIC_INFORMATION
{
 LARGE_INTEGER CreationTime;
 LARGE_INTEGER LastAccessTime;
 LARGE_INTEGER LastWriteTime;
 LARGE_INTEGER ChangeTime;
 ULONG FileAttributes;
};
using PFILE_BASIC_INFORMATION = MPTR<FILE_BASIC_INFORMATION, PHT>;

struct FILE_STANDARD_INFORMATION
{
 LARGE_INTEGER AllocationSize;
 LARGE_INTEGER EndOfFile;
 ULONG NumberOfLinks;
 BOOLEAN DeletePending;
 BOOLEAN Directory;
};
using PFILE_STANDARD_INFORMATION = MPTR<FILE_STANDARD_INFORMATION, PHT>;

struct FILE_POSITION_INFORMATION
{
 LARGE_INTEGER CurrentByteOffset;
};
using PFILE_POSITION_INFORMATION = MPTR<FILE_POSITION_INFORMATION, PHT>;

struct FILE_INTERNAL_INFORMATION
{
 LARGE_INTEGER IndexNumber;
};

struct FILE_EA_INFORMATION
{
 ULONG EaSize;
};

struct FILE_ACCESS_INFORMATION
{
 ACCESS_MASK AccessFlags;
};

struct FILE_MODE_INFORMATION
{
 ULONG Mode;
};

struct FILE_ALIGNMENT_INFORMATION
{
 ULONG AlignmentRequirement;
};

struct FILE_NAME_INFORMATION
{
 ULONG FileNameLength;
 WCHAR FileName[1];
};

struct FILE_ATTRIBUTE_TAG_INFORMATION
{
 ULONG FileAttributes;
 ULONG ReparseTag;
};

struct FILE_DISPOSITION_INFORMATION    
{
 union {
 ULONG   Flags;        // ULONG Flags;        // For FileDispositionInformationEx     // Since WIN10_RS1 ( Windows 10 release 1709 )
 BOOLEAN DeleteFile;   // BOOLEAN DeleteFile; // For FileDispositionInformation
 };
};

struct FILE_RENAME_INFORMATION 
{
 union {
 ULONG   Flags;             // ULONG Flags;        // For FileRenameInformationEx     // Since WIN10_RS1 ( Windows 10 release 1709 )
 BOOLEAN ReplaceIfExists;   // BOOLEAN DeleteFile; // For FileRenameInformation
 };
 HANDLE  RootDirectory;
 ULONG   FileNameLength;
 WCHAR   FileName[1];
};

// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_link_information
struct FILE_LINK_INFORMATION    // The FILE_LINK_INFORMATION structure is used to create an NTFS hard link to an existing file.
{
 union {
 ULONG   Flags;             // ULONG Flags;        // For FileLinkInformationEx     // Available starting with Windows 10, version 1809.
 BOOLEAN ReplaceIfExists;   // BOOLEAN DeleteFile; // For FileLinkInformation
 };
 HANDLE  RootDirectory;
 ULONG   FileNameLength;    // in bytes 
 WCHAR   FileName[1];
};

struct FILE_STANDARD_LINK_INFORMATION 
{
 ULONG   NumberOfAccessibleLinks;
 ULONG   TotalNumberOfLinks;
 BOOLEAN DeletePending;
 BOOLEAN Directory;
};

struct FILE_LINK_ENTRY_INFORMATION    // Windows Vista
{
 ULONG    NextEntryOffset;
 LONGLONG ParentFileId;
 ULONG    FileNameLength;          // in characters
 WCHAR    FileName[1];
};

struct FILE_LINKS_INFORMATION        // Windows Vista
{
 ULONG BytesNeeded;
 ULONG EntriesReturned;
 FILE_LINK_ENTRY_INFORMATION Entry;
};

struct FILE_ID_128 
{
 BYTE Identifier[16];
};

struct FILE_LINK_ENTRY_FULL_ID_INFORMATION 
{
 ULONG NextEntryOffset;
 FILE_ID_128 ParentFileId;
 ULONG FileNameLength;
 WCHAR FileName[1];
};

struct FILE_LINKS_FULL_ID_INFORMATION 
{
 ULONG BytesNeeded;
 ULONG EntriesReturned;
 FILE_LINK_ENTRY_FULL_ID_INFORMATION Entry;
};

struct FILE_END_OF_FILE_INFORMATION
{
 LARGE_INTEGER EndOfFile;
};

struct FILE_VALID_DATA_LENGTH_INFORMATION
{
 LARGE_INTEGER ValidDataLength;
};

enum FILE_ID_TYPE 
{
 FileIdType,
 ObjectIdType,
 ExtendedFileIdType,
 MaximumFileIdType
};

struct FILE_ID_DESCRIPTOR 
{
 DWORD        dwSize;
 FILE_ID_TYPE Type;
 union {
   LARGE_INTEGER FileId;
   GUID          ObjectId;
   FILE_ID_128   ExtendedFileId;
 };
};

struct FILE_ID_INFORMATION 
{
 ULONGLONG   VolumeSerialNumber;
 FILE_ID_128 FileId;
};

enum EFileInfFlgDisp
{
 FILE_DISPOSITION_DO_NOT_DELETE             = 0x00000000,  // Specifies the system should not delete a file.
 FILE_DISPOSITION_DELETE                    = 0x00000001,  // Specifies the system should delete a file.
 FILE_DISPOSITION_POSIX_SEMANTICS           = 0x00000002,  // Specifies the system should perform a POSIX - style delete.
 FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK = 0x00000004,  // Specifies the system should force an image section check.
 FILE_DISPOSITION_ON_CLOSE                  = 0x00000008,  // Specifies if the system sets or clears the on - close state.
 FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE = 0x00000010,  // Allows read-only files to be deleted. For more information, see the Remarks section below.
};

enum EFileInfFlgRename
{
 FILE_RENAME_REPLACE_IF_EXISTS                    = 0x00000001,  // If a file with the given name already exists, it should be replaced with the given file. Equivalent to the ReplaceIfExists field used with the FileRenameInformation information class.
 FILE_RENAME_POSIX_SEMANTICS                      = 0x00000002,  // If FILE_RENAME_REPLACE_IF_EXISTS is also specified, allow replacing a file even if there are existing handles to it. Existing handles to the replaced file continue to be valid for operations such as read and write. Any subsequent opens of the target name will open the renamed file, not the replaced file.
 FILE_RENAME_SUPPRESS_PIN_STATE_INHERITANCE       = 0x00000004,  // When renaming a file to a new directory, suppress any inheritance rules related to the FILE_ATTRIBUTE_PINNED and FILE_ATTRIBUTE_UNPINNED attributes of the file.
 FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE = 0x00000008,  // When renaming a file to a new directory, suppress any inheritance rules related to the storage reserve ID property of the file.
 FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE          = 0x00000010,  // If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from increasing. Requires manage volume access.
 FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE          = 0x00000020,  // If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from decreasing. Requires manage volume access.
 FILE_RENAME_PRESERVE_AVAILABLE_SPACE             = 0x00000030,  // Equivalent to specifying both FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE and FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE.
 FILE_RENAME_IGNORE_READONLY_ATTRIBUTE            = 0x00000040,  // If FILE_RENAME_REPLACE_IF_EXISTS is also specified, allow replacing a file even if it is read-only. Requires WRITE_ATTRIBUTES access to the replaced file.
 FILE_RENAME_FORCE_RESIZE_TARGET_SR               = 0x00000080,  // If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory that is part of a different storage reserve area, always grow the target directory's storage reserve area by the full size of the file being renamed. Requires manage volume access.
 FILE_RENAME_FORCE_RESIZE_SOURCE_SR               = 0x00000100,  // If FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when renaming a file to a new directory that is part of a different storage reserve area, always shrink the source directory's storage reserve area by the full size of the file being renamed. Requires manage volume access.
 FILE_RENAME_FORCE_RESIZE_SR                      = 0x00000180,  // Equivalent to specifying both FILE_RENAME_FORCE_RESIZE_TARGET_SR and FILE_RENAME_FORCE_RESIZE_SOURCE_SR.
};

enum EFileInfFlgLink
{
 FILE_LINK_REPLACE_IF_EXISTS                    = 0x00000001,  // If a file with the given name already exists, it should be replaced with the new link. Equivalent to the ReplaceIfExists field used with the FileLinkInformation information class.
 FILE_LINK_POSIX_SEMANTICS                      = 0x00000002,  // If FILE_LINK_REPLACE_IF_EXISTS is also specified, allow replacing a file even if there are existing handles to it. Existing handles to the replaced file continue to be valid for operations such as read and write. Any subsequent opens of the target name will open the new link, not the replaced file.
 FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE = 0x00000008,  // When creating a link in a new directory, suppress any inheritance rules related to the storage reserve ID property of the file.
 FILE_LINK_NO_INCREASE_AVAILABLE_SPACE          = 0x00000010,  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from increasing. Requires manage volume access.
 FILE_LINK_NO_DECREASE_AVAILABLE_SPACE          = 0x00000020,  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory, automatically resize affected storage reserve areas as needed to prevent the user visible free space on the volume from decreasing. Requires manage volume access.
 FILE_LINK_PRESERVE_AVAILABLE_SPACE             = 0x00000030,  // Equivalent to specifying both FILE_LINK_NO_INCREASE_AVAILABLE_SPACE and FILE_LINK_NO_DECREASE_AVAILABLE_SPACE.
 FILE_LINK_IGNORE_READONLY_ATTRIBUTE            = 0x00000040,  // If FILE_LINK_REPLACE_IF_EXISTS is also specified, allow replacing a file even if it is read-only. Requires WRITE_ATTRIBUTES access to the replaced file.
 FILE_LINK_FORCE_RESIZE_TARGET_SR               = 0x00000080,  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory that is part of a different storage reserve area, always grow the target directory's storage reserve area by the full size of the file being linked. Requires manage volume access.
 FILE_LINK_FORCE_RESIZE_SOURCE_SR               = 0x00000100,  // If FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE is not also specified, when creating a link in a new directory that is part of a different storage reserve area, always shrink the source directory's storage reserve area by the full size of the file being linked. Requires manage volume access.
 FILE_LINK_FORCE_RESIZE_SR                      = 0x00000180,  // Equivalent to specifying both FILE_LINK_FORCE_RESIZE_TARGET_SR and FILE_LINK_FORCE_RESIZE_SOURCE_SR.
};


/*
#define FILE_BYTE_ALIGNMENT 0x00000000
#define FILE_WORD_ALIGNMENT 0x00000001
#define FILE_LONG_ALIGNMENT 0x00000003
#define FILE_QUAD_ALIGNMENT 0x00000007
#define FILE_OCTA_ALIGNMENT 0x0000000f
*/

// ntfs.h
struct FILE_ALL_INFORMATION
{
  FILE_BASIC_INFORMATION     BasicInformation;
  FILE_STANDARD_INFORMATION  StandardInformation;
  FILE_INTERNAL_INFORMATION  InternalInformation;
  FILE_EA_INFORMATION        EaInformation;
  FILE_ACCESS_INFORMATION    AccessInformation;
  FILE_POSITION_INFORMATION  PositionInformation;
  FILE_MODE_INFORMATION      ModeInformation;
  FILE_ALIGNMENT_INFORMATION AlignmentInformation;
  FILE_NAME_INFORMATION      NameInformation;
};
using PFILE_ALL_INFORMATION = MPTR<FILE_ALL_INFORMATION, PHT>;


union FILE_SEGMENT_ELEMENT              // Define segment buffer structure for scatter/gather read/write.
{
 PVOID64   Buffer;
 ULONGLONG Alignment;
};
using PFILE_SEGMENT_ELEMENT = MPTR<FILE_SEGMENT_ELEMENT, PHT>;

enum EObjSymLnkAccess
{
 SYMBOLIC_LINK_QUERY      = 0x0001,
 SYMBOLIC_LINK_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED | SYMBOLIC_LINK_QUERY)
};


static NTSTATUS _scall NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);

static NTSTATUS _scall NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes);

static NTSTATUS _scall NtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);

static NTSTATUS _scall NtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);

static NTSTATUS _scall NtReadFileScatter(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PFILE_SEGMENT_ELEMENT SegmentArray, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);

static NTSTATUS _scall NtWriteFileGather(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PFILE_SEGMENT_ELEMENT SegmentArray, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);

static NTSTATUS _scall NtFlushBuffersFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock);

static NTSTATUS _scall NtClose(HANDLE Handle);

static NTSTATUS _scall NtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);

static NTSTATUS _scall NtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);

static NTSTATUS _scall NtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);

static NTSTATUS _scall NtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan);

static NTSTATUS _scall NtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);

static NTSTATUS _scall NtOpenSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);

static NTSTATUS _scall NtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);

static NTSTATUS _scall NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);

static NTSTATUS _scall NtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress);

static NTSTATUS _scall NtExtendSection(HANDLE SectionHandle, PLARGE_INTEGER NewSectionSize);

static NTSTATUS _scall NtQuerySection(HANDLE SectionHandle, SECTION_INFORMATION_CLASS SectionInformationClass, PVOID SectionInformation, SIZE_T SectionInformationLength, PSIZE_T ReturnLength);

static NTSTATUS _scall NtCreateSymbolicLinkObject(PHANDLE LinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PUNICODE_STRING DestinationName);
static NTSTATUS _scall NtOpenSymbolicLinkObject(PHANDLE LinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
static NTSTATUS _scall NtQuerySymbolicLinkObject(HANDLE LinkHandle, PUNICODE_STRING LinkTarget, PULONG ReturnedLength);

// Specifies the absolute or relative time, in units of 100 nanoseconds, for which the wait is to occur. A negative value indicates relative time. 
// Absolute expiration times track any changes in system time; relative expiration times are not affected by system time changes.
static NTSTATUS _scall NtDelayExecution(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval);    // ZwSetTimerResolution ???
static NTSTATUS _scall NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);
static NTSTATUS _scall NtTerminateThread(HANDLE ThreadHandle, NTSTATUS ExitStatus);

static NTSTATUS _scall NtLoadDriver(PUNICODE_STRING DriverServiceName);
static NTSTATUS _scall NtUnloadDriver(PUNICODE_STRING DriverServiceName);

static NTSTATUS _scall NtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
static NTSTATUS _scall NtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

// Object manipulation

// A temporary object has a name only as long as its handle count is greater than zero. When the handle count reaches zero, the system deletes the object name and appropriately adjusts the object's pointer count.
static NTSTATUS _scall NtMakeTemporaryObject(HANDLE Handle);
static NTSTATUS _scall NtMakePermanentObject(HANDLE Handle);


//------------------------------------------------------------------------------------------------------------
enum FS_INFORMATION_CLASS 
{
  FileFsVolumeInformation = 1,     // FILE_FS_VOLUME_INFORMATION 
  FileFsLabelInformation,          // FILE_FS_LABEL_INFORMATION
  FileFsSizeInformation,           // FILE_FS_SIZE_INFORMATION
  FileFsDeviceInformation,         // FILE_FS_DEVICE_INFORMATION
  FileFsAttributeInformation,      // FILE_FS_ATTRIBUTE_INFORMATION
  FileFsControlInformation,        // FILE_FS_CONTROL_INFORMATION
  FileFsFullSizeInformation,       // FILE_FS_FULL_SIZE_INFORMATION 
  FileFsObjectIdInformation,       // FILE_FS_OBJECTID_INFORMATION
  FileFsDriverPathInformation,     // FILE_FS_DRIVER_PATH_INFORMATION 
  FileFsVolumeFlagsInformation,    // FILE_FS_VOLUME_FLAGS_INFORMATION
  FileFsSectorSizeInformation,     // FILE_FS_SECTOR_SIZE_INFORMATION
  FileFsDataCopyInformation,       // FILE_FS_DATA_COPY_INFORMATION
  FileFsMetadataSizeInformation,   // FILE_FS_METADATA_SIZE_INFORMATION
  FileFsFullSizeInformationEx,     // FILE_FS_FULL_SIZE_INFORMATION_EX
  FileFsGuidInformation,           // FILE_FS_GUID_INFORMATION
  FileFsMaximumInformation         // End of this enumeration
};

struct FILE_FS_LABEL_INFORMATION 
{
 ULONG VolumeLabelLength;
 WCHAR VolumeLabel[1];
};
using PFILE_FS_LABEL_INFORMATION = MPTR<FILE_FS_LABEL_INFORMATION, PHT>;

struct FILE_FS_VOLUME_INFORMATION 
{
 LARGE_INTEGER VolumeCreationTime;
 ULONG VolumeSerialNumber;
 ULONG VolumeLabelLength;
 BOOLEAN SupportsObjects;
 WCHAR VolumeLabel[1];
};
using PFILE_FS_VOLUME_INFORMATION = MPTR<FILE_FS_VOLUME_INFORMATION, PHT>;

struct FILE_FS_SIZE_INFORMATION 
{
 LARGE_INTEGER TotalAllocationUnits;
 LARGE_INTEGER AvailableAllocationUnits;
 ULONG SectorsPerAllocationUnit;
 ULONG BytesPerSector;
};
using PFILE_FS_SIZE_INFORMATION = MPTR<FILE_FS_SIZE_INFORMATION, PHT>;

struct FILE_FS_FULL_SIZE_INFORMATION 
{
 LARGE_INTEGER TotalAllocationUnits;
 LARGE_INTEGER CallerAvailableAllocationUnits;
 LARGE_INTEGER ActualAvailableAllocationUnits;
 ULONG SectorsPerAllocationUnit;
 ULONG BytesPerSector;
};
using PFILE_FS_FULL_SIZE_INFORMATION = MPTR<FILE_FS_FULL_SIZE_INFORMATION, PHT>;

struct FILE_FS_OBJECTID_INFORMATION 
{
 UCHAR ObjectId[16];
 UCHAR ExtendedInfo[48];
};
using PFILE_FS_OBJECTID_INFORMATION = MPTR<FILE_FS_OBJECTID_INFORMATION, PHT>;

static NTSTATUS _scall NtQueryVolumeInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, ULONG Length, FS_INFORMATION_CLASS FsInformationClass);
//------------------------------------------------------------------------------------------------------------
enum EODirAccess
{
 DIRECTORY_QUERY               = 0x0001,  // Query access to the directory object.
 DIRECTORY_TRAVERSE            = 0x0002,  // Name-lookup access to the directory object.
 DIRECTORY_CREATE_OBJECT       = 0x0004,  // Name-creation access to the directory object.
 DIRECTORY_CREATE_SUBDIRECTORY = 0x0008,  // Subdirectory-creation access to the directory object.
 DIRECTORY_ALL_ACCESS          = STANDARD_RIGHTS_REQUIRED | 0xF,
};

// Buffer returned by ZwQueryDirectoryObject is full of these structures.
struct OBJECT_DIRECTORY_INFORMATION 
{
 UNICODE_STRING Name;        // Name of the object
 UNICODE_STRING TypeName;    // Type of the object (string form)
};
using POBJECT_DIRECTORY_INFORMATION = MPTR<OBJECT_DIRECTORY_INFORMATION, PHT>;

static NTSTATUS _scall NtOpenDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
static NTSTATUS _scall NtQueryDirectoryObject(HANDLE DirectoryHandle, PVOID Buffer, ULONG BufferLength, BOOLEAN ReturnSingleEntry, BOOLEAN RestartScan, PULONG Context, PULONG ReturnLength);
//------------------------------------------------------------------------------------------------------------
enum ELpcMsgType
{
 UNUSED_MSG_TYPE        = 0x0,
 LPC_REQUEST            = 0x1,
 LPC_REPLY              = 0x2,
 LPC_DATAGRAM           = 0x3,
 LPC_LOST_REPLY         = 0x4,
 LPC_PORT_CLOSED        = 0x5,
 LPC_CLIENT_DIED        = 0x6,
 LPC_EXCEPTION          = 0x7,
 LPC_DEBUG_EVENT        = 0x8,
 LPC_ERROR_EVENT        = 0x9,
 LPC_CONNECTION_REQUEST = 0xa,
 LPC_CONNECTION_REFUSED = 0xb,
};

struct LPC_MESSAGE_HEADER         // _PORT_MESSAGE_HEADER
{
 union
 {
  struct
  {
   USHORT DataLength;
   USHORT TotalLength;
  };
  ULONG Length;
 };
 union
 {
  struct
  {
   USHORT MessageType;
   USHORT DataInfoOffset;
  };
  ULONG ZeroInit;
 };

 ULONG  ProcessId;    // CLIENT_ID  // (HANDLE, HANDLE) ?
 ULONG  ThreadId;

 ULONG MessageId;
 union
 {
  SIZE_T ClientViewSize;
  ULONG  CallbackId;
 };
};

struct LPC_TERMINATION_MESSAGE
{
 LPC_MESSAGE_HEADER Header;
 LARGE_INTEGER      CreationTime;
};

struct LPC_SECTION_MEMORY
{
 ULONG Length;
 ULONG ViewSize;
 PVOID ViewBase;
};

struct LPC_SECTION_OWNER_MEMORY
{
 ULONG  Length;
 HANDLE SectionHandle;
 ULONG  OffsetInSection;
 ULONG  ViewSize;
 PVOID  ViewBase;
 PVOID  OtherSideViewBase;
};

enum SECURITY_IMPERSONATION_LEVEL
{
 SecurityAnonymous = 0,
 SecurityIdentification = 1,
 SecurityImpersonation = 2,
 SecurityDelegation = 3
};

struct SECURITY_QUALITY_OF_SERVICE
{
 ULONG Length;
 SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
 UCHAR ContextTrackingMode;
 UCHAR EffectiveOnly;
};

enum PORT_INFORMATION_CLASS
{
 PortNoInformation
};


using PLPC_MESSAGE_HEADER = MPTR<LPC_MESSAGE_HEADER, PHT>;
using PLPC_SECTION_OWNER_MEMORY = MPTR<LPC_SECTION_OWNER_MEMORY, PHT>;
using PLPC_SECTION_MEMORY = MPTR<LPC_SECTION_MEMORY, PHT>;
using PSECURITY_QUALITY_OF_SERVICE = MPTR<SECURITY_QUALITY_OF_SERVICE, PHT>;

static NTSTATUS _scall NtCreatePort(PHANDLE PortHandle, POBJECT_ATTRIBUTES ObjectAttributes, ULONG MaxConnectInfoLength, ULONG MaxDataLength, ULONG MaxPoolUsage);

static NTSTATUS _scall NtCreateWaitablePort(PHANDLE PortHandle,POBJECT_ATTRIBUTES ObjectAttributes, ULONG MaxConnectInfoLength, ULONG MaxDataLength, ULONG MaxPoolUsage);

static NTSTATUS _scall NtListenPort(HANDLE PortHandle, PLPC_MESSAGE_HEADER ConnectionRequest);

static NTSTATUS _scall NtAcceptConnectPort(PHANDLE ServerPortHandle, PVOID PortContext, PLPC_MESSAGE_HEADER ConnectionMsg, BOOLEAN AcceptConnection, PLPC_SECTION_OWNER_MEMORY ServerSharedMemory, PLPC_SECTION_MEMORY ClientSharedMemory);

static NTSTATUS _scall NtCompleteConnectPort(HANDLE PortHandle);

static NTSTATUS _scall NtReplyWaitReceivePort(HANDLE PortHandle, PPVOID PortContext, PLPC_MESSAGE_HEADER Reply, PLPC_MESSAGE_HEADER IncomingRequest);

static NTSTATUS _scall NtReplyWaitReplyPort(HANDLE PortHandle, PLPC_MESSAGE_HEADER Reply);

static NTSTATUS _scall NtRequestPort(HANDLE PortHandle, PLPC_MESSAGE_HEADER Request);

static NTSTATUS _scall NtRequestWaitReplyPort(HANDLE PortHandle, PLPC_MESSAGE_HEADER Request, PLPC_MESSAGE_HEADER IncomingReply);

static NTSTATUS _scall NtWriteRequestData(HANDLE PortHandle, PLPC_MESSAGE_HEADER Request, ULONG DataIndex, PVOID Buffer, ULONG Length, PULONG ResultLength);

static NTSTATUS _scall NtReadRequestData(HANDLE PortHandle, PLPC_MESSAGE_HEADER Request, ULONG DataIndex, PVOID Buffer, ULONG Length, PULONG ResultLength);

static NTSTATUS _scall NtReplyPort(HANDLE PortHandle, PLPC_MESSAGE_HEADER Reply);

static NTSTATUS _scall NtQueryInformationPort(HANDLE PortHandle, PORT_INFORMATION_CLASS PortInformationClass, PVOID PortInformation, ULONG Length, PULONG ResultLength );

static NTSTATUS _scall NtImpersonateClientOfPort(HANDLE PortHandle, PLPC_MESSAGE_HEADER Request);

static NTSTATUS _scall NtConnectPort(PHANDLE ClientPortHandle, PUNICODE_STRING ServerPortName, PSECURITY_QUALITY_OF_SERVICE SecurityQos, PLPC_SECTION_OWNER_MEMORY ClientSharedMemory, PLPC_SECTION_MEMORY ServerSharedMemory, PULONG MaximumMessageLength, PVOID ConnectionInfo, PULONG ConnectionInfoLength);

static NTSTATUS _scall NtAlpcSendWaitReceivePort(HANDLE PortHandle, ULONG Flags, PVOID SendMessage, PVOID SendMessageAttributes, PVOID ReceiveMessage, PSIZE_T BufferLength, PVOID ReceiveMessageAttributes, PLARGE_INTEGER Timeout);   // New
//------------------------------------------------------------------------------------------------------------
#include "NtIoCtrl.hpp"

//------------------------------------------------------------------------------------------------------------
//  Doubly linked list structure.  Can be used as either a list head, or as link words.
struct LIST_ENTRY
{
   MPTR<LIST_ENTRY, PHT> Flink;
   MPTR<LIST_ENTRY, PHT> Blink;
};
using PLIST_ENTRY = MPTR<LIST_ENTRY, PHT>;

SCVR uint RTL_MAX_DRIVE_LETTERS  = 32;
SCVR uint RTL_DRIVE_LETTER_VALID = (uint16)0x0001;

struct CURDIR
{
 UNICODE_STRING DosPath;
 HANDLE Handle;
};
using PCURDIR = MPTR<CURDIR, PHT>;

SCVR uint RTL_USER_PROC_CURDIR_CLOSE   = 0x00000002;
SCVR uint RTL_USER_PROC_CURDIR_INHERIT = 0x00000003;

struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
};
using PRTL_DRIVE_LETTER_CURDIR = MPTR<RTL_DRIVE_LETTER_CURDIR, PHT>;

struct RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    HANDLE ConsoleHandle;
    ULONG  ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;

    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;

    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;

    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

    ULONG_PTR EnvironmentSize;
    ULONG_PTR EnvironmentVersion;
    PVOID PackageDependencyData;
    ULONG ProcessGroupId;
    ULONG LoaderThreads;

// New
    UNICODE_STRING RedirectionDllName;                              //0x410
    UNICODE_STRING HeapPartitionName;                               //0x420
    ULONGLONG* DefaultThreadpoolCpuSetMasks;                        //0x430
    ULONG DefaultThreadpoolCpuSetMaskCount;                         //0x438
    ULONG DefaultThreadpoolThreadMaximum;                           //0x43c
};
using PRTL_USER_PROCESS_PARAMETERS = MPTR<RTL_USER_PROCESS_PARAMETERS, PHT>;

enum EUPPFlg
{
UPP_NORMALIZED            =  0x01,
UPP_PROFILE_USER          =  0x02,
UPP_PROFILE_KERNEL        =  0x04,
UPP_PROFILE_SERVER        =  0x08,
UPP_RESERVE_1MB           =  0x20,
UPP_RESERVE_16MB          =  0x40,
UPP_CASE_SENSITIVE        =  0x80,
UPP_DISABLE_HEAP_DECOMMIT =  0x100,
UPP_DLL_REDIRECTION_LOCAL =  0x1000,
UPP_APP_MANIFEST_PRESENT  =  0x2000,
UPP_IMAGE_KEY_MISSING     =  0x4000,
UPP_NX_OPTIN              =  0x20000
};

SCVR uint GDI_HANDLE_BUFFER_SIZE = IsArchX64?60:34;
typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];

SCVR uint GDI_BATCH_BUFFER_SIZE = 310;
struct GDI_TEB_BATCH
{
    ULONG Offset;
    ULONG_PTR HDC;
    ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
};
using PGDI_TEB_BATCH = MPTR<GDI_TEB_BATCH, PHT>;

//------------------------------------------------------------------------------------------------------------
//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//
// The flags field within this record controls the contents of a CONTEXT
// record.
//
// If the context record is used as an input parameter, then for each
// portion of the context record controlled by a flag whose value is
// set, it is assumed that that portion of the context record contains
// valid context. If the context record is being used to modify a threads
// context, then only that portion of the threads context is modified.
//
// If the context record is used as an output parameter to capture the
// context of a thread, then only those portions of the thread's context
// corresponding to set flags will be returned.
//
// CONTEXT_CONTROL specifies SegSs, Rsp, SegCs, Rip, and EFlags.
//
// CONTEXT_INTEGER specifies Rax, Rcx, Rdx, Rbx, Rbp, Rsi, Rdi, and R8-R15.
//
// CONTEXT_SEGMENTS specifies SegDs, SegEs, SegFs, and SegGs.
//
// CONTEXT_FLOATING_POINT specifies Xmm0-Xmm15.
//
// CONTEXT_DEBUG_REGISTERS specifies Dr0-Dr3 and Dr6-Dr7.
//

#ifdef CPU_X86
struct M128A
{
 uint64 Low;
 sint64 High;
};

struct XMM_SAVE_AREA32
{
 WORD   ControlWord;
 WORD   StatusWord;
 BYTE   TagWord;
 BYTE   Reserved1;
 WORD   ErrorOpcode;
 DWORD  ErrorOffset;
 WORD   ErrorSelector;
 WORD   Reserved2;
 DWORD  DataOffset;
 WORD   DataSelector;
 WORD   Reserved3;
 DWORD  MxCsr;
 DWORD  MxCsr_Mask;
 M128A  FloatRegisters[8];
 M128A  XmmRegisters[16];
 BYTE   Reserved4[96];
};

struct FLOATING_SAVE_AREA
{
 DWORD ControlWord;
 DWORD StatusWord;
 DWORD TagWord;
 DWORD ErrorOffset;
 DWORD ErrorSelector;
 DWORD DataOffset;
 DWORD DataSelector;
 BYTE  RegisterArea[80];
 DWORD Cr0NpxState;
};

enum CONTEXT_FLAGS_X86X32
{
 CONTEXT_X86_i386               = 0x00010000,
 CONTEXT_X86_i486               = 0x00010000,   //  same as i386
 CONTEXT_X86_CONTROL            = CONTEXT_X86_i386 | 0x0001, // SS:SP, CS:IP, FLAGS, BP
 CONTEXT_X86_INTEGER            = CONTEXT_X86_i386 | 0x0002, // AX, BX, CX, DX, SI, DI
 CONTEXT_X86_SEGMENTS           = CONTEXT_X86_i386 | 0x0004, // DS, ES, FS, GS
 CONTEXT_X86_FLOATING_POINT     = CONTEXT_X86_i386 | 0x0008, // 387 state
 CONTEXT_X86_DEBUG_REGISTERS    = CONTEXT_X86_i386 | 0x0010, // DB 0-3,6,7
 CONTEXT_X86_EXTENDED_REGISTERS = CONTEXT_X86_i386 | 0x0020, // cpu specific extensions
 CONTEXT_X86_FULL = CONTEXT_X86_CONTROL | CONTEXT_X86_INTEGER | CONTEXT_X86_SEGMENTS,
 CONTEXT_X86_ALL  = CONTEXT_X86_CONTROL | CONTEXT_X86_INTEGER | CONTEXT_X86_SEGMENTS | CONTEXT_X86_FLOATING_POINT | CONTEXT_X86_DEBUG_REGISTERS | CONTEXT_X86_EXTENDED_REGISTERS
};

enum CONTEXT_FLAGS_X86X64
{
 CONTEXT_A64_AMD64              = 0x00100000,
 CONTEXT_A64_CONTROL            = CONTEXT_A64_AMD64 | 0x0001, // SS:SP, CS:IP, FLAGS, BP
 CONTEXT_A64_INTEGER            = CONTEXT_A64_AMD64 | 0x0002, // AX, BX, CX, DX, SI, DI
 CONTEXT_A64_SEGMENTS           = CONTEXT_A64_AMD64 | 0x0004, // DS, ES, FS, GS
 CONTEXT_A64_FLOATING_POINT     = CONTEXT_A64_AMD64 | 0x0008, // 387 state
 CONTEXT_A64_DEBUG_REGISTERS    = CONTEXT_A64_AMD64 | 0x0010, // DB 0-3,6,7
 CONTEXT_A64_EXTENDED_REGISTERS = CONTEXT_A64_AMD64 | 0x0020, // cpu specific extensions
 CONTEXT_A64_FULL = CONTEXT_A64_CONTROL | CONTEXT_A64_INTEGER | CONTEXT_A64_SEGMENTS,
 CONTEXT_A64_ALL  = CONTEXT_A64_CONTROL | CONTEXT_A64_INTEGER | CONTEXT_A64_SEGMENTS | CONTEXT_A64_FLOATING_POINT | CONTEXT_A64_DEBUG_REGISTERS | CONTEXT_A64_EXTENDED_REGISTERS
};

struct alignas(16) CONTEXT_X86X32     // Aligned to 16 in case of WOW64
{
 DWORD ContextFlags;

// This section is specified/returned if CONTEXT_DEBUG_REGISTERS is set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT included in CONTEXT_FULL.
 DWORD   Dr0;
 DWORD   Dr1;
 DWORD   Dr2;
 DWORD   Dr3;
 DWORD   Dr6;
 DWORD   Dr7;

// This section is specified/returned if the ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
 FLOATING_SAVE_AREA FloatSave;

// This section is specified/returned if the ContextFlags word contians the flag CONTEXT_SEGMENTS.
 DWORD   SegGs;
 DWORD   SegFs;
 DWORD   SegEs;
 DWORD   SegDs;

// This section is specified/returned if the ContextFlags word contians the flag CONTEXT_INTEGER.
 DWORD   Edi;
 DWORD   Esi;
 DWORD   Ebx;
 DWORD   Edx;
 DWORD   Ecx;
 DWORD   Eax;

// This section is specified/returned if the ContextFlags word contians the flag CONTEXT_CONTROL.
 DWORD   Ebp;
 union {
 DWORD   Eip;
 DWORD   Pc;
       };
 DWORD   SegCs;              // MUST BE SANITIZED
 DWORD   EFlags;             // MUST BE SANITIZED
 union {
 DWORD   Esp;
 DWORD   Sp;
       };
 DWORD   SegSs;

// This section is specified/returned if the ContextFlags word contains the flag CONTEXT_EXTENDED_REGISTERS. The format and contexts are processor specific
 BYTE    ExtendedRegisters[512];   // MAXIMUM_SUPPORTED_EXTENSION
};

struct alignas(16) CONTEXT_X86X64
{
// Register parameter home addresses.  // N.B. These fields are for convience - they could be used to extend the context record in the future.
 QWORD P1Home;
 QWORD P2Home;
 QWORD P3Home;
 QWORD P4Home;
 QWORD P5Home;
 QWORD P6Home;

// Control flags.
 DWORD ContextFlags;
 DWORD MxCsr;

// Segment Registers and processor flags.
 WORD   SegCs;
 WORD   SegDs;
 WORD   SegEs;
 WORD   SegFs;
 WORD   SegGs;
 WORD   SegSs;
 DWORD EFlags;

// Debug registers
 QWORD Dr0;
 QWORD Dr1;
 QWORD Dr2;
 QWORD Dr3;
 QWORD Dr6;
 QWORD Dr7;

// Integer registers.
 QWORD Rax;
 QWORD Rcx;
 QWORD Rdx;
 QWORD Rbx;
 union {
 QWORD Rsp;
 QWORD Sp;
       };
 QWORD Rbp;
 QWORD Rsi;
 QWORD Rdi;
 QWORD R8;
 QWORD R9;
 QWORD R10;
 QWORD R11;
 QWORD R12;
 QWORD R13;
 QWORD R14;
 QWORD R15;

// Program counter.
 union {
 QWORD Rip;
 QWORD Pc;
       };
// Floating point state.
 union {
     XMM_SAVE_AREA32 FltSave;
     struct {
         M128A Header[2];
         M128A Legacy[8];
         M128A Xmm0;
         M128A Xmm1;
         M128A Xmm2;
         M128A Xmm3;
         M128A Xmm4;
         M128A Xmm5;
         M128A Xmm6;
         M128A Xmm7;
         M128A Xmm8;
         M128A Xmm9;
         M128A Xmm10;
         M128A Xmm11;
         M128A Xmm12;
         M128A Xmm13;
         M128A Xmm14;
         M128A Xmm15;
     } DUMMYSTRUCTNAME;
 } DUMMYUNIONNAME;

// Vector registers.
 M128A VectorRegister[26];
 QWORD VectorControl;

// Special debug control registers.
 QWORD DebugControl;
 QWORD LastBranchToRip;
 QWORD LastBranchFromRip;
 QWORD LastExceptionToRip;
 QWORD LastExceptionFromRip;
};

using CONTEXT    = TSW<IsArchX64, CONTEXT_X86X64, CONTEXT_X86X32>::T;
using CONTEXT64  = CONTEXT_X86X64;
using CONTEXT32  = CONTEXT_X86X32;

using PCONTEXT   = TSW<IsArchX64, MPTR<CONTEXT_X86X64, PHT>, MPTR<CONTEXT_X86X32, PHT>>::T;      // Native
using PCONTEXT64 = MPTR<CONTEXT_X86X64, PHT>;
using PCONTEXT32 = MPTR<CONTEXT_X86X32, PHT>;

#ifdef ARCH_X64
enum CONTEXT_FLAGS
{
 CONTEXT_CONTROL            = CONTEXT_A64_CONTROL,
 CONTEXT_INTEGER            = CONTEXT_A64_INTEGER,
 CONTEXT_SEGMENTS           = CONTEXT_A64_SEGMENTS,
 CONTEXT_FLOATING_POINT     = CONTEXT_A64_FLOATING_POINT,
 CONTEXT_DEBUG_REGISTERS    = CONTEXT_A64_DEBUG_REGISTERS,
 CONTEXT_EXTENDED_REGISTERS = CONTEXT_A64_DEBUG_REGISTERS,
 CONTEXT_FULL = CONTEXT_A64_FULL,
 CONTEXT_ALL  = CONTEXT_A64_ALL
};
#else
enum CONTEXT_FLAGS
{
 CONTEXT_CONTROL            = CONTEXT_X86_CONTROL,
 CONTEXT_INTEGER            = CONTEXT_X86_INTEGER,
 CONTEXT_SEGMENTS           = CONTEXT_X86_SEGMENTS,
 CONTEXT_FLOATING_POINT     = CONTEXT_X86_FLOATING_POINT,
 CONTEXT_DEBUG_REGISTERS    = CONTEXT_X86_DEBUG_REGISTERS,
 CONTEXT_EXTENDED_REGISTERS = CONTEXT_X86_EXTENDED_REGISTERS,
 CONTEXT_FULL = CONTEXT_X86_FULL,
 CONTEXT_ALL  = CONTEXT_X86_ALL
};
#endif
#endif
//------------------------------------------------------------------------------------------------------------
// https://github.com/wine-mirror/wine/blob/master/include/winnt.h
#if defined(ARCH_X64) && defined(CPU_ARM)
static constexpr int ARM64_MAX_BREAKPOINTS = 8;
static constexpr int ARM64_MAX_WATCHPOINTS = 2;

enum ECtxFlgARM    // Arm64  // Do not want to bother implementing boolean operators for 'enum class'
{
 CONTEXT_ARM                 = 0x0200000,
 CONTEXT_ARM_CONTROL         = (CONTEXT_ARM | 0x00000001),
 CONTEXT_ARM_INTEGER         = (CONTEXT_ARM | 0x00000002),
 CONTEXT_ARM_FLOATING_POINT  = (CONTEXT_ARM | 0x00000004),
 CONTEXT_ARM_DEBUG_REGISTERS = (CONTEXT_ARM | 0x00000008),
 CONTEXT_ARM_FULL            = (CONTEXT_ARM_CONTROL | CONTEXT_ARM_INTEGER),
 CONTEXT_ARM_ALL             = (CONTEXT_ARM_FULL | CONTEXT_ARM_FLOATING_POINT | CONTEXT_ARM_DEBUG_REGISTERS),
};

union ARM64_NT_NEON128
{
 struct {
        ULONGLONG Low;
        LONGLONG High;
 } DUMMYSTRUCTNAME;
    double D[2];
    float  S[4];
    WORD   H[8];
    BYTE   B[16];
};

struct CONTEXT_ARM64    // _ARM64_NT_CONTEXT
{
  DWORD ContextFlags;
  DWORD Cpsr;
  union {
    struct {
      QWORD X0;
      QWORD X1;
      QWORD X2;
      QWORD X3;
      QWORD X4;
      QWORD X5;
      QWORD X6;
      QWORD X7;
      QWORD X8;
      QWORD X9;
      QWORD X10;
      QWORD X11;
      QWORD X12;
      QWORD X13;
      QWORD X14;
      QWORD X15;
      QWORD X16;
      QWORD X17;
      QWORD X18;
      QWORD X19;
      QWORD X20;
      QWORD X21;
      QWORD X22;
      QWORD X23;
      QWORD X24;
      QWORD X25;
      QWORD X26;
      QWORD X27;
      QWORD X28;
      QWORD Fp;
      QWORD Lr;
    } DUMMYSTRUCTNAME;
    QWORD X[31];
  } DUMMYUNIONNAME;
  QWORD          Sp;
  QWORD          Pc;
  ARM64_NT_NEON128 V[32];
  DWORD            Fpcr;
  DWORD            Fpsr;
  DWORD            Bcr[ARM64_MAX_BREAKPOINTS];
  QWORD            Bvr[ARM64_MAX_BREAKPOINTS];
  DWORD            Wcr[ARM64_MAX_WATCHPOINTS];
  QWORD            Wvr[ARM64_MAX_WATCHPOINTS];
};

using CONTEXT    = CONTEXT_ARM64;
using CONTEXT64  = CONTEXT_ARM64;
using CONTEXT32  = void;

using PCONTEXT   = MPTR<CONTEXT_ARM64, PHT>;
using PCONTEXT64 = MPTR<CONTEXT_ARM64, PHT>;
using PCONTEXT32 = PVOID;    // Nonexistent

enum CONTEXT_FLAGS
{
 CONTEXT_CONTROL            = CONTEXT_ARM_CONTROL,
 CONTEXT_INTEGER            = CONTEXT_ARM_INTEGER,
 CONTEXT_SEGMENTS           = 0,
 CONTEXT_FLOATING_POINT     = CONTEXT_ARM_FLOATING_POINT,
 CONTEXT_DEBUG_REGISTERS    = CONTEXT_ARM_DEBUG_REGISTERS,
 CONTEXT_EXTENDED_REGISTERS = 0,
 CONTEXT_FULL = CONTEXT_ARM_FULL,
 CONTEXT_ALL  = CONTEXT_ARM_ALL
};
#endif
//------------------------------------------------------------------------------------------------------------
#if defined(ARCH_X32) && defined(CPU_ARM)
using PCONTEXT   = void*;           // Linux will not compile without this
#endif

static NTSTATUS _scall NtGetContextThread(HANDLE ThreadHandle, PCONTEXT ThreadContext);
static NTSTATUS _scall NtSetContextThread(HANDLE ThreadHandle, PCONTEXT ThreadContext);

struct CLIENT_ID
{
 HANDLE UniqueProcess;    // Actually, both are IDs
 HANDLE UniqueThread;
};
using PCLIENT_ID = MPTR<CLIENT_ID, PHT>;

static NTSTATUS _scall NtOpenThread(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
static NTSTATUS _scall NtOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
//------------------------------------------------------------------------------------------------------------
SCVR uint FLS_MAXIMUM_AVAILABLE = 128;
SCVR uint TLS_MINIMUM_AVAILABLE = 64;
SCVR uint TLS_EXPANSION_SLOTS   = 1024;

SCVR uint32 LDRP_STATIC_LINK               = 0x00000002;
SCVR uint32 LDRP_IMAGE_DLL                 = 0x00000004;
SCVR uint32 LDRP_LOAD_IN_PROGRESS          = 0x00001000;
SCVR uint32 LDRP_UNLOAD_IN_PROGRESS        = 0x00002000;
SCVR uint32 LDRP_ENTRY_PROCESSED           = 0x00004000;
SCVR uint32 LDRP_ENTRY_INSERTED            = 0x00008000;
SCVR uint32 LDRP_CURRENT_LOAD              = 0x00010000;
SCVR uint32 LDRP_FAILED_BUILTIN_LOAD       = 0x00020000;
SCVR uint32 LDRP_DONT_CALL_FOR_THREADS     = 0x00040000;
SCVR uint32 LDRP_PROCESS_ATTACH_CALLED     = 0x00080000;
SCVR uint32 LDRP_DEBUG_SYMBOLS_LOADED      = 0x00100000;
SCVR uint32 LDRP_IMAGE_NOT_AT_BASE         = 0x00200000;
SCVR uint32 LDRP_COR_IMAGE                 = 0x00400000;
SCVR uint32 LDR_COR_OWNS_UNMAP             = 0x00800000;
SCVR uint32 LDRP_SYSTEM_MAPPED             = 0x01000000;
SCVR uint32 LDRP_IMAGE_VERIFYING           = 0x02000000;
SCVR uint32 LDRP_DRIVER_DEPENDENT_DLL      = 0x04000000;
SCVR uint32 LDRP_ENTRY_NATIVE              = 0x08000000;
SCVR uint32 LDRP_REDIRECTED                = 0x10000000;
SCVR uint32 LDRP_NON_PAGED_DEBUG_INFO      = 0x20000000;
SCVR uint32 LDRP_MM_LOADED                 = 0x40000000;
SCVR uint32 LDRP_COMPAT_DATABASE_PROCESSED = 0x80000000;

enum LDR_HOT_PATCH_STATE
{
	LdrHotPatchBaseImage,
	LdrHotPatchNotApplied,
	LdrHotPatchAppliedReverse,
	LdrHotPatchAppliedForward,
	LdrHotPatchFailedToPatch,
	LdrHotPatchStateMax,
};

enum LDR_DLL_LOAD_REASON
{
    LoadReasonStaticDependency,
    LoadReasonStaticForwarderDependency,
    LoadReasonDynamicForwarderDependency,
    LoadReasonDelayloadDependency,
    LoadReasonDynamicLoad,
    LoadReasonAsImageLoad,
    LoadReasonAsDataLoad,
    LoadReasonUnknown = -1
};
using PLDR_DLL_LOAD_REASON = MPTR<LDR_DLL_LOAD_REASON, PHT>;

// Exception disposition return values
enum EXCEPTION_DISPOSITION
{
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind
};

SCVR uint EXCEPTION_MAXIMUM_PARAMETERS = 15; // maximum number of exception parameters

struct EXCEPTION_RECORD
{
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    MPTR<EXCEPTION_RECORD, PHT> ExceptionRecord;
    PVOID ExceptionAddress;
    DWORD NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
};
using PEXCEPTION_RECORD = MPTR<EXCEPTION_RECORD, PHT>;

static EXCEPTION_DISPOSITION _scall EXCEPTION_ROUTINE(PEXCEPTION_RECORD ExceptionRecord, PVOID EstablisherFrame, PCONTEXT ContextRecord, PVOID DispatcherContext);
using PEXCEPTION_ROUTINE = decltype(EXCEPTION_ROUTINE)*;

struct EXCEPTION_REGISTRATION_RECORD
{
    MPTR<EXCEPTION_REGISTRATION_RECORD, PHT> Next;
    PEXCEPTION_ROUTINE Handler;
};

struct ACTIVATION_CONTEXT_STACK
{
    PVOID ActiveFrame;   // _RTL_ACTIVATION_CONTEXT_STACK_FRAME
    LIST_ENTRY FrameListCache;
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    ULONG StackId;
};
using PACTIVATION_CONTEXT_STACK = MPTR<ACTIVATION_CONTEXT_STACK, PHT>;

struct TEB_ACTIVE_FRAME_CONTEXT
{
    ULONG Flags;
    LPSTR FrameName;
};
using PTEB_ACTIVE_FRAME_CONTEXT = MPTR<TEB_ACTIVE_FRAME_CONTEXT, PHT>;

struct TEB_ACTIVE_FRAME
{
    ULONG Flags;
    MPTR<TEB_ACTIVE_FRAME, PHT> Previous;
    PTEB_ACTIVE_FRAME_CONTEXT Context;
};
using PTEB_ACTIVE_FRAME = MPTR<TEB_ACTIVE_FRAME, PHT>;


struct PROCESSOR_NUMBER
{
    WORD  Group;
    BYTE  Number;
    BYTE  Reserved;
};
using PPROCESSOR_NUMBER = MPTR<PROCESSOR_NUMBER, PHT>;

struct RTL_BALANCED_NODE
{
    union
    {
        MPTR<RTL_BALANCED_NODE, PHT> Children[2];
        struct
        {
            MPTR<RTL_BALANCED_NODE, PHT> Left;
            MPTR<RTL_BALANCED_NODE, PHT> Right;
        } s;
    };
    union
    {
        UCHAR Red : 1;
        UCHAR Balance : 2;
        ULONG_PTR ParentValue;
    } u;
};
using PRTL_BALANCED_NODE = MPTR<RTL_BALANCED_NODE, PHT>;


using PACTIVATION_CONTEXT = PVOID;  // TOO COMPLEX!!!
using PLDRP_LOAD_CONTEXT  = PVOID;  // TOO COMPLEX!!!
using PLDR_DDAG_NODE      = PVOID;  // TOO COMPLEX!!!
//------------------------------------------------------------------------------------------------------------
template<typename T> struct TLIST_ENTRY
{
   MPTR<T, PHT> Flink;
   MPTR<T, PHT> Blink;
};


template<typename T> struct TLDR_DATA_TABLE_ENTRY: public T
{
    PVOID DllBase alignas(sizeof(PHT));
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    union
    {
        UCHAR FlagGroup[4];
        ULONG Flags;
        struct
        {
            ULONG PackagedBinary : 1;
            ULONG MarkedForRemoval : 1;
            ULONG ImageDll : 1;
            ULONG LoadNotificationsSent : 1;
            ULONG TelemetryEntryProcessed : 1;
            ULONG ProcessStaticImport : 1;
            ULONG InLegacyLists : 1;
            ULONG InIndexes : 1;
            ULONG ShimDll : 1;
            ULONG InExceptionTable : 1;
            ULONG ReservedFlags1 : 2;
            ULONG LoadInProgress : 1;
            ULONG LoadConfigProcessed : 1;
            ULONG EntryProcessed : 1;
            ULONG ProtectDelayLoad : 1;
            ULONG ReservedFlags3 : 2;
            ULONG DontCallForThreads : 1;
            ULONG ProcessAttachCalled : 1;
            ULONG ProcessAttachFailed : 1;
            ULONG CorDeferredValidate : 1;
            ULONG CorImage : 1;
            ULONG DontRelocate : 1;
            ULONG CorILOnly : 1;
            ULONG ReservedFlags5 : 3;
            ULONG Redirected : 1;
            ULONG ReservedFlags6 : 2;
            ULONG CompatDatabaseProcessed : 1;
        } s;
    } u;
    USHORT ObsoleteLoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
    PACTIVATION_CONTEXT EntryPointActivationContext;
    PVOID Lock;
    PLDR_DDAG_NODE DdagNode;
    LIST_ENTRY NodeModuleLink;
    PLDRP_LOAD_CONTEXT LoadContext;
    PVOID ParentDllBase;
    PVOID SwitchBackContext;
    RTL_BALANCED_NODE BaseAddressIndexNode;
    RTL_BALANCED_NODE MappingInfoIndexNode;
    ULONG_PTR OriginalBase;
    LARGE_INTEGER LoadTime;
    ULONG BaseNameHashValue;
    LDR_DLL_LOAD_REASON LoadReason;
    ULONG ImplicitPathOptions;
    ULONG ReferenceCount;
    ULONG DependentLoadFlags;
    UCHAR SigningLevel; // Since Windows 10 RS2
};  // LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;


struct LDR_DATA_TABLE_BASE_IO
{
    union
    {
        TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO> > InInitializationOrderLinks;
        TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO> > InProgressLinks;
    };
};

struct LDR_DATA_TABLE_BASE_MO
{
 TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_MO> > InMemoryOrderLinks;
    union
    {
        TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO> > InInitializationOrderLinks;
        TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO> > InProgressLinks;
    };
};

struct LDR_DATA_TABLE_BASE_LO
{
 TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_LO> > InLoadOrderLinks;
 TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_MO> > InMemoryOrderLinks;
    union
    {
        TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO> > InInitializationOrderLinks;
        TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO> > InProgressLinks;
    };
};



typedef TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_LO>  LDR_DATA_TABLE_ENTRY;

typedef TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_LO>  LDR_DATA_TABLE_ENTRY_LO;
typedef TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_MO>  LDR_DATA_TABLE_ENTRY_MO;
typedef TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO>  LDR_DATA_TABLE_ENTRY_IO;

struct PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle alignas(sizeof(PHT));
    TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_LO> > InLoadOrderModuleList;
    TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_MO> > InMemoryOrderModuleList;
    TLIST_ENTRY<TLDR_DATA_TABLE_ENTRY<LDR_DATA_TABLE_BASE_IO> > InInitializationOrderModuleList;   // Not including EXE
    PVOID EntryInProgress;
    BOOLEAN ShutdownInProgress;
    HANDLE ShutdownThreadId alignas(sizeof(PHT));
};
using PPEB_LDR_DATA = MPTR<PEB_LDR_DATA, PHT>;

struct TIB
{
    MPTR<EXCEPTION_REGISTRATION_RECORD, PHT> ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    PVOID FiberData;
    PVOID ArbitraryUserPointer;
    MPTR<TIB, PHT> Self;
};
using PTIB = MPTR<TIB, PHT>;
//------------------------------------------------------------------------------------------------------------
// http://bytepointer.com/resources/tebpeb64.htm
// https://redplait.blogspot.com/2011/09/w8-64bit-teb-peb.html
// https://github.com/giampaolo/psutil/
//      http://terminus.rewolf.pl/terminus/structures/ntdll/_PEB_x64.html
//      http://terminus.rewolf.pl/terminus/structures/ntdll/_TEB64_x86.html
//
struct PEB    // PEB64 ???
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union
    {
        BOOLEAN BitField;
        struct
        {
            BOOLEAN ImageUsesLargePages : 1;
            BOOLEAN IsProtectedProcess : 1;
            BOOLEAN IsImageDynamicallyRelocated : 1;
            BOOLEAN SkipPatchingUser32Forwarders : 1;
            BOOLEAN IsPackagedProcess : 1;
            BOOLEAN IsAppContainer : 1;
            BOOLEAN IsProtectedProcessLight : 1;
            BOOLEAN IsLongPathAwareProcess : 1;
        } s1;
    } u1;

    HANDLE Mutant alignas(sizeof(PHT));

    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    PRTL_CRITICAL_SECTION FastPebLock;
    PVOID AtlThunkSListPtr;
    PVOID IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ProcessPreviouslyThrottled : 1;
            ULONG ProcessCurrentlyThrottled : 1;
            ULONG ReservedBits0 : 25;
        } s2;
    } u2;
    union
    {
        PVOID KernelCallbackTable;
        PVOID UserSharedInfoPtr;
    } u3;
    ULONG SystemReserved[1];
    ULONG AtlThunkSListPtr32;
    PVOID ApiSetMap;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[2];

    PVOID ReadOnlySharedMemoryBase;
    PVOID SharedData; // HotpatchInformation
    PPVOID ReadOnlyStaticServerData;

    PVOID AnsiCodePageData; // PCPTABLEINFO
    PVOID OemCodePageData; // PCPTABLEINFO
    PVOID UnicodeCaseTableData; // PNLSTABLEINFO

    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;

    LARGE_INTEGER CriticalSectionTimeout;
    SIZE_T HeapSegmentReserve;
    SIZE_T HeapSegmentCommit;
    SIZE_T HeapDeCommitTotalFreeThreshold;
    SIZE_T HeapDeCommitFreeBlockThreshold;

    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PPVOID ProcessHeaps; // PHEAP

    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    ULONG GdiDCAttributeList;

    PRTL_CRITICAL_SECTION LoaderLock;

    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG_PTR ActiveProcessAffinityMask;
    GDI_HANDLE_BUFFER GdiHandleBuffer;
    PVOID PostProcessInitRoutine;

    PVOID TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];

    ULONG SessionId;

    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    PVOID pShimData;
    PVOID AppCompatInfo; // APPCOMPAT_EXE_DATA

    UNICODE_STRING CSDVersion;

    PVOID ActivationContextData; // ACTIVATION_CONTEXT_DATA
    PVOID ProcessAssemblyStorageMap; // ASSEMBLY_STORAGE_MAP
    PVOID SystemDefaultActivationContextData; // ACTIVATION_CONTEXT_DATA
    PVOID SystemAssemblyStorageMap; // ASSEMBLY_STORAGE_MAP

    SIZE_T MinimumStackCommit;

    PPVOID FlsCallback;
    LIST_ENTRY FlsListHead;
    PVOID FlsBitmap;
    ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
    ULONG FlsHighIndex;

    PVOID WerRegistrationData;
    PVOID WerShipAssertPtr;
    PVOID pUnused; // pContextData
    PVOID pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        } s3;
    } u4;
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
    PVOID TppWorkerpListLock;
    LIST_ENTRY TppWorkerpList;
    PVOID WaitOnAddressHashTable[128];
    PVOID TelemetryCoverageHeader; // REDSTONE3
    ULONG CloudFileFlags;
};
using PPEB = MPTR<PEB, PHT>;
//------------------------------------------------------------------------------------------------------------
struct TEB     // TEB64
{
    TIB NtTib;            // 56   NT_TIB64

    PVOID EnvironmentPointer;
    CLIENT_ID ClientId;
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
    PPEB ProcessEnvironmentBlock;

    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    PVOID Win32ThreadInfo;
    ULONG User32Reserved[26];
    ULONG UserReserved[5];
    PVOID WOW32Reserved;
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    PVOID ReservedForDebuggerInstrumentation[16];
    PVOID SystemReserved1[IsArchX64?30:26];
    CHAR PlaceholderCompatibilityMode;
    CHAR PlaceholderReserved[11];
    ULONG ProxiedProcessId;
    ACTIVATION_CONTEXT_STACK ActivationStack;

    UCHAR WorkingOnBehalfTicket[8];
    NTSTATUS ExceptionCode;

    PACTIVATION_CONTEXT_STACK ActivationContextStackPointer;
    ULONG_PTR InstrumentationCallbackSp;
    ULONG_PTR InstrumentationCallbackPreviousPc;
    ULONG_PTR InstrumentationCallbackPreviousSp;
#ifdef ARCH_X64
    ULONG TxFsContext;
#endif
    BOOLEAN InstrumentationCallbackDisabled;
#ifndef ARCH_X64
    UCHAR SpareBytes[23];
    ULONG TxFsContext;
#endif
    GDI_TEB_BATCH GdiTebBatch;
    CLIENT_ID RealClientId;
    HANDLE GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    PVOID GdiThreadLocalInfo;
    ULONG_PTR Win32ClientInfo[62];
    PVOID glDispatchTable[233];
    ULONG_PTR glReserved1[29];
    PVOID glReserved2;
    PVOID glSectionInfo;
    PVOID glSection;
    PVOID glTable;
    PVOID glCurrentRC;
    PVOID glContext;

    NTSTATUS LastStatusValue;
    UNICODE_STRING StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[261];

    PVOID DeallocationStack;
    PVOID TlsSlots[64];
    LIST_ENTRY TlsLinks;

    PVOID Vdm;
    PVOID ReservedForNtRpc;
    PVOID DbgSsReserved[2];

    ULONG HardErrorMode;

    PVOID Instrumentation[IsArchX64?11:9];
    GUID ActivityId;

    PVOID SubProcessTag;
    PVOID PerflibData;
    PVOID EtwTraceData;
    PVOID WinSockData;
    ULONG GdiBatchCount;

    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;
        ULONG IdealProcessorValue;
        struct
        {
            UCHAR ReservedPad0;
            UCHAR ReservedPad1;
            UCHAR ReservedPad2;
            UCHAR IdealProcessor;
        } s1;
    } u1;

    ULONG GuaranteedStackBytes;
    PVOID ReservedForPerf;
    PVOID ReservedForOle;
    ULONG WaitingOnLoaderLock;
    PVOID SavedPriorityState;
    ULONG_PTR ReservedForCodeCoverage;
    PVOID ThreadPoolData;
    PPVOID TlsExpansionSlots;
#ifdef ARCH_X64
    PVOID DeallocationBStore;
    PVOID BStoreLimit;
#endif
    ULONG MuiGeneration;
    ULONG IsImpersonating;
    PVOID NlsCache;
    PVOID pShimData;
    USHORT HeapVirtualAffinity;
    USHORT LowFragHeapDataSlot;
    HANDLE CurrentTransactionHandle;
    PTEB_ACTIVE_FRAME ActiveFrame;
    PVOID FlsData;

    PVOID PreferredLanguages;
    PVOID UserPrefLanguages;
    PVOID MergedPrefLanguages;
    ULONG MuiImpersonation;

    union
    {
        USHORT CrossTebFlags;
        USHORT SpareCrossTebBits : 16;
    } u2;
    union
    {
        USHORT SameTebFlags;
        struct
        {
            USHORT SafeThunkCall : 1;
            USHORT InDebugPrint : 1;
            USHORT HasFiberData : 1;
            USHORT SkipThreadAttach : 1;
            USHORT WerInShipAssertCode : 1;
            USHORT RanProcessInit : 1;
            USHORT ClonedThread : 1;
            USHORT SuppressDebugMsg : 1;
            USHORT DisableUserStackWalk : 1;
            USHORT RtlExceptionAttached : 1;
            USHORT InitialThread : 1;
            USHORT SessionAware : 1;
            USHORT LoadOwner : 1;
            USHORT LoaderWorker : 1;
            USHORT SkipLoaderInit : 1;
            USHORT SpareSameTebBits : 1;
        } s2;
    } u3;

    PVOID TxnScopeEnterCallback;
    PVOID TxnScopeExitCallback;
    PVOID TxnScopeContext;
    ULONG LockCount;
    LONG WowTebOffset;
    PVOID ResourceRetValue;
    PVOID ReservedForWdf;
    ULONGLONG ReservedForCrt;
    GUID EffectiveContainerId;
};
using PTEB = MPTR<TEB, PHT>;

static_assert(alignof(TEB) == 8, "Windows SDK structs must be aligned to 8");   // One check for all structs here
//------------------------------------------------------------------------------------------------------------
//Source: http://processhacker.sourceforge.net
enum THREADINFOCLASS
{
    ThreadBasicInformation, // q: THREAD_BASIC_INFORMATION
    ThreadTimes, // q: KERNEL_USER_TIMES
    ThreadPriority, // s: KPRIORITY
    ThreadBasePriority, // s: LONG
    ThreadAffinityMask, // s: KAFFINITY
    ThreadImpersonationToken, // s: HANDLE
    ThreadDescriptorTableEntry, // q: DESCRIPTOR_TABLE_ENTRY (or WOW64_DESCRIPTOR_TABLE_ENTRY)
    ThreadEnableAlignmentFaultFixup, // s: BOOLEAN
    ThreadEventPair,
    ThreadQuerySetWin32StartAddress, // q: PVOID
    ThreadZeroTlsCell, // 10
    ThreadPerformanceCount, // q: LARGE_INTEGER
    ThreadAmILastThread, // q: ULONG
    ThreadIdealProcessor, // s: ULONG
    ThreadPriorityBoost, // qs: ULONG
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending, // q: ULONG
    ThreadHideFromDebugger, // s: void
    ThreadBreakOnTermination, // qs: ULONG
    ThreadSwitchLegacyState,
    ThreadIsTerminated, // q: ULONG // 20
    ThreadLastSystemCall, // q: THREAD_LAST_SYSCALL_INFORMATION
    ThreadIoPriority, // qs: IO_PRIORITY_HINT
    ThreadCycleTime, // q: THREAD_CYCLE_TIME_INFORMATION
    ThreadPagePriority, // q: ULONG
    ThreadActualBasePriority,
    ThreadTebInformation, // q: THREAD_TEB_INFORMATION (requires THREAD_GET_CONTEXT + THREAD_SET_CONTEXT)
    ThreadCSwitchMon,
    ThreadCSwitchPmu,
    ThreadWow64Context, // q: WOW64_CONTEXT
    ThreadGroupInformation, // q: GROUP_AFFINITY // 30
    ThreadUmsInformation, // q: THREAD_UMS_INFORMATION
    ThreadCounterProfiling,
    ThreadIdealProcessorEx, // q: PROCESSOR_NUMBER
    ThreadCpuAccountingInformation, // since WIN8
    ThreadSuspendCount, // since WINBLUE
    ThreadHeterogeneousCpuPolicy, // q: KHETERO_CPU_POLICY // since THRESHOLD
    ThreadContainerId, // q: GUID
    ThreadNameInformation, // qs: THREAD_NAME_INFORMATION
    ThreadSelectedCpuSets,
    ThreadSystemThreadInformation, // q: SYSTEM_THREAD_INFORMATION // 40
    ThreadActualGroupAffinity, // since THRESHOLD2
    ThreadDynamicCodePolicyInfo,
    ThreadExplicitCaseSensitivity,
    ThreadWorkOnBehalfTicket,
    ThreadSubsystemInformation, // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
    ThreadDbgkWerReportActive,
    ThreadAttachContainer,
    MaxThreadInfoClass
};

enum PROCESSINFOCLASS
{
    ProcessBasicInformation, // q: PROCESS_BASIC_INFORMATION, PROCESS_EXTENDED_BASIC_INFORMATION
    ProcessQuotaLimits, // qs: QUOTA_LIMITS, QUOTA_LIMITS_EX
    ProcessIoCounters, // q: IO_COUNTERS
    ProcessVmCounters, // q: VM_COUNTERS, VM_COUNTERS_EX, VM_COUNTERS_EX2
    ProcessTimes, // q: KERNEL_USER_TIMES
    ProcessBasePriority, // s: KPRIORITY
    ProcessRaisePriority, // s: ULONG
    ProcessDebugPort, // q: HANDLE
    ProcessExceptionPort, // s: HANDLE
    ProcessAccessToken, // s: PROCESS_ACCESS_TOKEN
    ProcessLdtInformation, // qs: PROCESS_LDT_INFORMATION // 10
    ProcessLdtSize, // s: PROCESS_LDT_SIZE
    ProcessDefaultHardErrorMode, // qs: ULONG
    ProcessIoPortHandlers, // (kernel-mode only)
    ProcessPooledUsageAndLimits, // q: POOLED_USAGE_AND_LIMITS
    ProcessWorkingSetWatch, // q: PROCESS_WS_WATCH_INFORMATION[]; s: void
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup, // s: BOOLEAN
    ProcessPriorityClass, // qs: PROCESS_PRIORITY_CLASS
    ProcessWx86Information,
    ProcessHandleCount, // q: ULONG, PROCESS_HANDLE_INFORMATION // 20
    ProcessAffinityMask, // s: KAFFINITY
    ProcessPriorityBoost, // qs: ULONG
    ProcessDeviceMap, // qs: PROCESS_DEVICEMAP_INFORMATION, PROCESS_DEVICEMAP_INFORMATION_EX
    ProcessSessionInformation, // q: PROCESS_SESSION_INFORMATION
    ProcessForegroundInformation, // s: PROCESS_FOREGROUND_BACKGROUND
    ProcessWow64Information, // q: ULONG_PTR
    ProcessImageFileName, // q: UNICODE_STRING
    ProcessLUIDDeviceMapsEnabled, // q: ULONG
    ProcessBreakOnTermination, // qs: ULONG
    ProcessDebugObjectHandle, // q: HANDLE // 30
    ProcessDebugFlags, // qs: ULONG
    ProcessHandleTracing, // q: PROCESS_HANDLE_TRACING_QUERY; s: size 0 disables, otherwise enables
    ProcessIoPriority, // qs: IO_PRIORITY_HINT
    ProcessExecuteFlags, // qs: ULONG
    ProcessResourceManagement,
    ProcessCookie, // q: ULONG
    ProcessImageInformation, // q: SECTION_IMAGE_INFORMATION
    ProcessCycleTime, // q: PROCESS_CYCLE_TIME_INFORMATION // since VISTA
    ProcessPagePriority, // q: ULONG
    ProcessInstrumentationCallback, // 40
    ProcessThreadStackAllocation, // s: PROCESS_STACK_ALLOCATION_INFORMATION, PROCESS_STACK_ALLOCATION_INFORMATION_EX
    ProcessWorkingSetWatchEx, // q: PROCESS_WS_WATCH_INFORMATION_EX[]
    ProcessImageFileNameWin32, // q: UNICODE_STRING
    ProcessImageFileMapping, // q: HANDLE (input)
    ProcessAffinityUpdateMode, // qs: PROCESS_AFFINITY_UPDATE_MODE
    ProcessMemoryAllocationMode, // qs: PROCESS_MEMORY_ALLOCATION_MODE
    ProcessGroupInformation, // q: USHORT[]
    ProcessTokenVirtualizationEnabled, // s: ULONG
    ProcessConsoleHostProcess, // q: ULONG_PTR
    ProcessWindowInformation, // q: PROCESS_WINDOW_INFORMATION // 50
    ProcessHandleInformation, // q: PROCESS_HANDLE_SNAPSHOT_INFORMATION // since WIN8
    ProcessMitigationPolicy, // s: PROCESS_MITIGATION_POLICY_INFORMATION
    ProcessDynamicFunctionTableInformation,
    ProcessHandleCheckingMode,
    ProcessKeepAliveCount, // q: PROCESS_KEEPALIVE_COUNT_INFORMATION
    ProcessRevokeFileHandles, // s: PROCESS_REVOKE_FILE_HANDLES_INFORMATION
    ProcessWorkingSetControl, // s: PROCESS_WORKING_SET_CONTROL
    ProcessHandleTable, // since WINBLUE
    ProcessCheckStackExtentsMode,
    ProcessCommandLineInformation, // q: UNICODE_STRING // 60
    ProcessProtectionInformation, // q: PS_PROTECTION
    ProcessMemoryExhaustion, // PROCESS_MEMORY_EXHAUSTION_INFO // since THRESHOLD
    ProcessFaultInformation, // PROCESS_FAULT_INFORMATION
    ProcessTelemetryIdInformation, // PROCESS_TELEMETRY_ID_INFORMATION
    ProcessCommitReleaseInformation, // PROCESS_COMMIT_RELEASE_INFORMATION
    ProcessDefaultCpuSetsInformation,
    ProcessAllowedCpuSetsInformation,
    ProcessSubsystemProcess,
    ProcessJobMemoryInformation, // PROCESS_JOB_MEMORY_INFO
    ProcessInPrivate, // since THRESHOLD2 // 70
    ProcessRaiseUMExceptionOnInvalidHandleClose,
    ProcessIumChallengeResponse,
    ProcessChildProcessInformation, // PROCESS_CHILD_PROCESS_INFORMATION
    ProcessHighGraphicsPriorityInformation,
    ProcessSubsystemInformation, // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
    ProcessEnergyValues, // PROCESS_ENERGY_VALUES, PROCESS_EXTENDED_ENERGY_VALUES
    ProcessActivityThrottleState, // PROCESS_ACTIVITY_THROTTLE_STATE
    ProcessActivityThrottlePolicy, // PROCESS_ACTIVITY_THROTTLE_POLICY
    ProcessWin32kSyscallFilterInformation,
    ProcessDisableSystemAllowedCpuSets,
    ProcessWakeInformation, // PROCESS_WAKE_INFORMATION
    ProcessEnergyTrackingState, // PROCESS_ENERGY_TRACKING_STATE
    MaxProcessInfoClass
};

struct THREAD_BASIC_INFORMATION
{
    NTSTATUS  ExitStatus;
    PVOID     TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    KPRIORITY Priority;
    LONG      BasePriority;
};

struct PROCESS_BASIC_INFORMATION
{
 NTSTATUS  ExitStatus;
 PPEB      PebBaseAddress;
 ULONG_PTR AffinityMask;
 KPRIORITY BasePriority;
 HANDLE    UniqueProcessId;
 HANDLE    InheritedFromUniqueProcessId;
};
using PPROCESS_BASIC_INFORMATION = MPTR<PROCESS_BASIC_INFORMATION, PHT>;

struct PROCESS_EXTENDED_BASIC_INFORMATION
{
    SIZE_T Size; // Set to sizeof structure on input
    PROCESS_BASIC_INFORMATION BasicInfo;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsProtectedProcess : 1;
            ULONG IsWow64Process : 1;
            ULONG IsProcessDeleting : 1;
            ULONG IsCrossSessionCreate : 1;
            ULONG IsFrozen : 1;
            ULONG IsBackground : 1;
            ULONG IsStronglyNamed : 1;
            ULONG IsSecureProcess : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG SpareBits : 23;
        } s;
    } u;
};
using PPROCESS_EXTENDED_BASIC_INFORMATION = MPTR<PROCESS_EXTENDED_BASIC_INFORMATION, PHT>;

struct PROCESS_DEVICEMAP_INFORMATION      // Invalid since Vista?
{    
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    };
};
using PPROCESS_DEVICEMAP_INFORMATION = MPTR<PROCESS_DEVICEMAP_INFORMATION, PHT>;
 
struct PROCESS_DEVICEMAP_INFORMATION_EX 
{
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    };
    ULONG Flags;    // specifies that the query type
};
using PPROCESS_DEVICEMAP_INFORMATION_EX = MPTR<PROCESS_DEVICEMAP_INFORMATION_EX, PHT>;

enum EUsrProcParams
{
 RTL_USER_PROCESS_PARAMETERS_NORMALIZED             = 0x01,
 RTL_USER_PROCESS_PARAMETERS_PROFILE_USER           = 0x02,
 RTL_USER_PROCESS_PARAMETERS_PROFILE_KERNEL         = 0x04,
 RTL_USER_PROCESS_PARAMETERS_PROFILE_SERVER         = 0x08,
 RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB            = 0x20,
 RTL_USER_PROCESS_PARAMETERS_RESERVE_16MB           = 0x40,
 RTL_USER_PROCESS_PARAMETERS_CASE_SENSITIVE         = 0x80,
 RTL_USER_PROCESS_PARAMETERS_DISABLE_HEAP_DECOMMIT  = 0x100,
 RTL_USER_PROCESS_PARAMETERS_DLL_REDIRECTION_LOCAL  = 0x1000,
 RTL_USER_PROCESS_PARAMETERS_APP_MANIFEST_PRESENT   = 0x2000,
 RTL_USER_PROCESS_PARAMETERS_IMAGE_KEY_MISSING      = 0x4000,
 RTL_USER_PROCESS_PARAMETERS_NX_OPTIN               = 0x20000,
};

enum EProcessAccess    // Combines with EFMisc1
{
 PROCESS_TERMINATE                 = 0x0001,
 PROCESS_CREATE_THREAD             = 0x0002,
// ???
 PROCESS_VM_OPERATION              = 0x0008,
 PROCESS_VM_READ                   = 0x0010,
 PROCESS_VM_WRITE                  = 0x0020,
 PROCESS_DUP_HANDLE                = 0x0040,
 PROCESS_CREATE_PROCESS            = 0x0080,
 PROCESS_SET_QUOTA                 = 0x0100,
 PROCESS_SET_INFORMATION           = 0x0200,
 PROCESS_QUERY_INFORMATION         = 0x0400,
 PROCESS_SUSPEND_RESUME            = 0x0800,
 PROCESS_QUERY_LIMITED_INFORMATION = 0x1000,
 PROCESS_ALL_ACCESS                = 0x1FFFFF,         // < Vista:  0xFFF
};

enum EThreadAccess     // Combines with EFMisc1
{
 THREAD_TERMINATE                 = 0x0001,
 THREAD_SUSPEND_RESUME            = 0x0002,
// ???
 THREAD_GET_CONTEXT               = 0x0008,
 THREAD_SET_CONTEXT               = 0x0010,
 THREAD_SET_INFORMATION           = 0x0020,
 THREAD_QUERY_INFORMATION         = 0x0040,
 THREAD_SET_THREAD_TOKEN          = 0x0080,
 THREAD_IMPERSONATE               = 0x0100,
 THREAD_DIRECT_IMPERSONATION      = 0x0200,
 THREAD_SET_LIMITED_INFORMATION   = 0x0400,
 THREAD_QUERY_LIMITED_INFORMATION = 0x0800,
 THREAD_ALL_ACCESS                = 0x1FFFFF,  // < Vista:  0x3FF
};

enum EFlgCrtPrEx
{
 PROCESS_CREATE_FLAGS_BREAKAWAY                    = 0x00000001, // NtCreateProcessEx & NtCreateUserProcess
 PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT             = 0x00000002, // NtCreateProcessEx & NtCreateUserProcess
 PROCESS_CREATE_FLAGS_INHERIT_HANDLES              = 0x00000004, // NtCreateProcessEx & NtCreateUserProcess
 PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE       = 0x00000008, // NtCreateProcessEx only
 PROCESS_CREATE_FLAGS_LARGE_PAGES                  = 0x00000010, // NtCreateProcessEx only, requires SeLockMemory
 PROCESS_CREATE_FLAGS_LARGE_PAGE_SYSTEM_DLL        = 0x00000020, // NtCreateProcessEx only, requires SeLockMemory
 PROCESS_CREATE_FLAGS_PROTECTED_PROCESS            = 0x00000040, // NtCreateUserProcess only
 PROCESS_CREATE_FLAGS_CREATE_SESSION               = 0x00000080, // NtCreateProcessEx & NtCreateUserProcess, requires SeLoadDriver
 PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT          = 0x00000100, // NtCreateProcessEx & NtCreateUserProcess
 PROCESS_CREATE_FLAGS_SUSPENDED                    = 0x00000200, // NtCreateProcessEx & NtCreateUserProcess
 PROCESS_CREATE_FLAGS_FORCE_BREAKAWAY              = 0x00000400, // NtCreateProcessEx & NtCreateUserProcess, requires SeTcb
 PROCESS_CREATE_FLAGS_MINIMAL_PROCESS              = 0x00000800, // NtCreateProcessEx only
 PROCESS_CREATE_FLAGS_RELEASE_SECTION              = 0x00001000, // NtCreateProcessEx & NtCreateUserProcess
 PROCESS_CREATE_FLAGS_CLONE_MINIMAL                = 0x00002000, // NtCreateProcessEx only
 PROCESS_CREATE_FLAGS_CLONE_MINIMAL_REDUCED_COMMIT = 0x00004000, //
 PROCESS_CREATE_FLAGS_AUXILIARY_PROCESS            = 0x00008000, // NtCreateProcessEx & NtCreateUserProcess, requires SeTcb
 PROCESS_CREATE_FLAGS_CREATE_STORE                 = 0x00020000, // NtCreateProcessEx & NtCreateUserProcess
 PROCESS_CREATE_FLAGS_USE_PROTECTED_ENVIRONMENT    = 0x00040000, // NtCreateProcessEx & NtCreateUserProcess
};

enum EFlgCrtThEx
{
 THREAD_CREATE_FLAGS_CREATE_SUSPENDED      = 0x00000001, // NtCreateUserProcess & NtCreateThreadEx
 THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH    = 0x00000002, // NtCreateThreadEx only
 THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER    = 0x00000004, // NtCreateThreadEx only
 THREAD_CREATE_FLAGS_LOADER_WORKER         = 0x00000010, // NtCreateThreadEx only
 THREAD_CREATE_FLAGS_SKIP_LOADER_INIT      = 0x00000020, // NtCreateThreadEx only
 THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE = 0x00000040, // NtCreateThreadEx only
 THREAD_CREATE_FLAGS_INITIAL_THREAD        = 0x00000080, // ?
};

static NTSTATUS _scall NtCreateProcessEx(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ParentProcess, ULONG Flags, HANDLE SectionHandle, HANDLE DebugPort, HANDLE ExceptionPort, BOOLEAN InJob);
static NTSTATUS _scall NtCreateProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ParentProcess, BOOLEAN InheritObjectTable, HANDLE SectionHandle, HANDLE DebugPort, HANDLE ExceptionPort);
static NTSTATUS _scall NtCreateThread(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ProcessHandle, PCLIENT_ID ClientId, PVOID ThreadContext, PUSER_STACK UserStack, BOOLEAN CreateSuspended);   // PCONTEXT ThreadContext  // Pointers to PCONTEXT is not just dependant on pointer size so must be defined as PVOID
// NtCreateThreadEx  // Vista+

static NTSTATUS _scall NtSuspendThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount);
static NTSTATUS _scall NtResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount);
static NTSTATUS _scall NtSuspendProcess(HANDLE ProcessHandle);
static NTSTATUS _scall NtResumeProcess(HANDLE ProcessHandle);

static NTSTATUS _scall NtSetInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);
static NTSTATUS _scall NtQueryInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);

static NTSTATUS _scall NtSetInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);
static NTSTATUS _scall NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);

static NTSTATUS _scall NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout);

enum WAIT_TYPE
{
    WaitAll,
    WaitAny,
    WaitNotification,
    WaitDequeue
};
// WaitForMultipleObjects() scans the handle array from 0 onwards and returns as soon as it finds a signalled handle. Only that first found handle is reset to the unsignalled state; the others are untouched.
static NTSTATUS _scall NtWaitForMultipleObjects(ULONG Count, PHANDLE Handles, WAIT_TYPE WaitType, BOOLEAN Alertable, PLARGE_INTEGER Timeout);  // WaitAll or WaitAny


// https://locklessinc.com/articles/keyed_events/
// The event object itself is shared (created once by the system)
// The key value you pass determines which wait queue you join
// Any thread in any process can wait or wake — if it uses the same key
static NTSTATUS _scall NtOpenKeyedEvent(PHANDLE handle, ACCESS_MASK access, POBJECT_ATTRIBUTES attr);
static NTSTATUS _scall NtCreateKeyedEvent(PHANDLE handle, ACCESS_MASK access, POBJECT_ATTRIBUTES attr, ULONG flags);

// The major different between this API and the Futex API is that there is no comparison value that is checked as the thread goes to sleep. 
// Instead, the NtReleaseKeyedEvent() function blocks, waiting for a thread to wake if there is none. (Compared to the Futex wake function which will immediately return.) 
// Thus to use this API we need to keep track of how many waiters there are to prevent the release function from hanging.
// Returns: STATUS_ALERTED - If wait was interrupted by an alert (if Alertable was set to TRUE); STATUS_SUCCESS - The thread successfully waited and was released
static NTSTATUS _scall NtWaitForKeyedEvent(HANDLE handle, PVOID key, BOOLEAN alertable, PLARGE_INTEGER mstimeout);	
static NTSTATUS _scall NtReleaseKeyedEvent(HANDLE handle, PVOID key, BOOLEAN alertable, PLARGE_INTEGER mstimeout);

// https://ntdoc.m417z.com/
//
static NTSTATUS _scall NtAlertThreadByThreadId(HANDLE ThreadId);

// The NtWaitForAlertByThreadId routine waits for an alert (From NtAlertThreadByThreadId only, not APC or NtAlertThread) to be delivered to the specified thread.
// Address: The address to wait for an alert on. the user-provided value that serves as a key. (Same as 'Key' in NtWaitForKeyedEvent ?)
// Timeout: The timeout value for waiting, or NULL for no timeout. A negative value indicates relative timeout for the specified number of 100-nanosecond intervals. To wait for a specific number of milliseconds, multiply them by -10,000. Positive values indicate an absolute time.
// return:  NTSTATUS Successful or errant status.  (STATUS_ALERTED, STATUS_TIMEOUT)
static NTSTATUS _scall NtWaitForAlertByThreadId(PVOID Address, PLARGE_INTEGER Timeout);
//---------------------------------------- From Windows Vista ---------------------------------------------------------
enum PS_ATTRIBUTE_NUM
{
  PsAttributeParentProcess,                // in HANDLE
  PsAttributeDebugObject,                  // in HANDLE
  PsAttributeToken,                        // in HANDLE
  PsAttributeClientId,                     // out PCLIENT_ID
  PsAttributeTebAddress,                   // out PTEB *
  PsAttributeImageName,                    // in PWSTR
  PsAttributeImageInfo,                    // out PSECTION_IMAGE_INFORMATION
  PsAttributeMemoryReserve,                // in PPS_MEMORY_RESERVE
  PsAttributePriorityClass,                // in UCHAR
  PsAttributeErrorMode,                    // in ULONG
  PsAttributeStdHandleInfo,                // 10, in PPS_STD_HANDLE_INFO
  PsAttributeHandleList,                   // in HANDLE[]
  PsAttributeGroupAffinity,                // in PGROUP_AFFINITY
  PsAttributePreferredNode,                // in PUSHORT
  PsAttributeIdealProcessor,               // in PPROCESSOR_NUMBER
  PsAttributeUmsThread,                    // ? in PUMS_CREATE_THREAD_ATTRIBUTES
  PsAttributeMitigationOptions,            // in PPS_MITIGATION_OPTIONS_MAP (PROCESS_CREATION_MITIGATION_POLICY_*) // since WIN8
  PsAttributeProtectionLevel,              // in PS_PROTECTION // since WINBLUE
  PsAttributeSecureProcess,                // in PPS_TRUSTLET_CREATE_ATTRIBUTES, since THRESHOLD
  PsAttributeJobList,                      // in HANDLE[]
  PsAttributeChildProcessPolicy,           // 20, in PULONG (PROCESS_CREATION_CHILD_PROCESS_*) // since THRESHOLD2
  PsAttributeAllApplicationPackagesPolicy, // in PULONG (PROCESS_CREATION_ALL_APPLICATION_PACKAGES_*) // since REDSTONE
  PsAttributeWin32kFilter,                 // in PWIN32K_SYSCALL_FILTER
  PsAttributeSafeOpenPromptOriginClaim,    // in
  PsAttributeBnoIsolation,                 // in PPS_BNO_ISOLATION_PARAMETERS // since REDSTONE2
  PsAttributeDesktopAppPolicy,             // in PULONG (PROCESS_CREATION_DESKTOP_APP_*)
  PsAttributeChpe,                         // in BOOLEAN // since REDSTONE3
  PsAttributeMitigationAuditOptions,       // in PPS_MITIGATION_AUDIT_OPTIONS_MAP (PROCESS_CREATION_MITIGATION_AUDIT_POLICY_*) // since 21H1
  PsAttributeMachineType,                  // in WORD // since 21H2
  PsAttributeComponentFilter,
  PsAttributeEnableOptionalXStateFeatures, // since WIN11
  PsAttributeMax
};

SCVR uint  PS_ATTRIBUTE_NUMBER_MASK  = 0x0000ffff;
SCVR uint  PS_ATTRIBUTE_THREAD       = 0x00010000; // Attribute may be used with thread creation
SCVR uint  PS_ATTRIBUTE_INPUT        = 0x00020000; // Attribute is input only
SCVR uint  PS_ATTRIBUTE_ADDITIVE     = 0x00040000; // Attribute may be "accumulated", e.g. bitmasks, counters, etc.

static consteval uint PsAttributeValue(PS_ATTRIBUTE_NUM Number, bool Thread, bool Input, bool Additive) {return ((Number & PS_ATTRIBUTE_NUMBER_MASK) | (Thread ? PS_ATTRIBUTE_THREAD : 0) | (Input ? PS_ATTRIBUTE_INPUT : 0) | (Additive ? PS_ATTRIBUTE_ADDITIVE : 0));}

enum class EPsAttribute
{
 PARENT_PROCESS                  = PsAttributeValue(PsAttributeParentProcess, false, true, true),                 // 0x60000
 DEBUG_OBJECT                    = PsAttributeValue(PsAttributeDebugObject, false, true, true),                   // 0x60001
 TOKEN                           = PsAttributeValue(PsAttributeToken, false, true, true),                         // 0x60002
 CLIENT_ID                       = PsAttributeValue(PsAttributeClientId, true, false, false),                     // 0x10003
 TEB_ADDRESS                     = PsAttributeValue(PsAttributeTebAddress, true, false, false),                   // 0x10004
 IMAGE_NAME                      = PsAttributeValue(PsAttributeImageName, false, true, false),                    // 0x20005
 IMAGE_INFO                      = PsAttributeValue(PsAttributeImageInfo, false, false, false),                   // 0x00006
 MEMORY_RESERVE                  = PsAttributeValue(PsAttributeMemoryReserve, false, true, false),                // 0x20007
 PRIORITY_CLASS                  = PsAttributeValue(PsAttributePriorityClass, false, true, false),                // 0x20008
 ERROR_MODE                      = PsAttributeValue(PsAttributeErrorMode, false, true, false),                    // 0x20009
 STD_HANDLE_INFO                 = PsAttributeValue(PsAttributeStdHandleInfo, false, true, false),                // 0x2000A
 HANDLE_LIST                     = PsAttributeValue(PsAttributeHandleList, false, true, false),                   // 0x2000B
 GROUP_AFFINITY                  = PsAttributeValue(PsAttributeGroupAffinity, true, true, false),                 // 0x2000C
 PREFERRED_NODE                  = PsAttributeValue(PsAttributePreferredNode, false, true, false),                // 0x2000D
 IDEAL_PROCESSOR                 = PsAttributeValue(PsAttributeIdealProcessor, true, true, false),                // 0x2000E
 MITIGATION_OPTIONS              = PsAttributeValue(PsAttributeMitigationOptions, false, true, false),            // 0x60010
 PROTECTION_LEVEL                = PsAttributeValue(PsAttributeProtectionLevel, false, true, false),              // 0x20011
 SECURE_PROCESS                  = PsAttributeValue(PsAttributeSecureProcess, false, true, false),                // 0x20012
 JOB_LIST                        = PsAttributeValue(PsAttributeJobList, false, true, false),                      // 0x20013
 CHILD_PROCESS_POLICY            = PsAttributeValue(PsAttributeChildProcessPolicy, false, true, false),           // 0x20014
 ALL_APPLICATION_PACKAGES_POLICY = PsAttributeValue(PsAttributeAllApplicationPackagesPolicy, false, true, false), // 0x20015
 WIN32K_FILTER                   = PsAttributeValue(PsAttributeWin32kFilter, false, true, false),                 // 0x20016
 SAFE_OPEN_PROMPT_ORIGIN_CLAIM   = PsAttributeValue(PsAttributeSafeOpenPromptOriginClaim, false, true, false),    // 0x20017
 BNO_ISOLATION                   = PsAttributeValue(PsAttributeBnoIsolation, false, true, false),                 // 0x20018
 DESKTOP_APP_POLICY              = PsAttributeValue(PsAttributeDesktopAppPolicy, false, true, false),             // 0x20019
 CHPE                            = PsAttributeValue(PsAttributeChpe, false, true, true),                          // 0x6001A
 MITIGATION_AUDIT_OPTIONS        = PsAttributeValue(PsAttributeMitigationAuditOptions, false, true, false),       // 0x2001B
 MACHINE_TYPE                    = PsAttributeValue(PsAttributeMachineType, false, true, true),                   // 0x6001C
 COMPONENT_FILTER                = PsAttributeValue(PsAttributeComponentFilter, false, true, false),              // 0x2001D
 ENABLE_OPTIONAL_XSTATE_FEATURES = PsAttributeValue(PsAttributeEnableOptionalXStateFeatures, true, true, false),  // 0x3001E
};

struct PS_ATTRIBUTE
{
 ULONG_PTR Attribute;                // PROC_THREAD_ATTRIBUTE_XXX | PROC_THREAD_ATTRIBUTE_XXX modifiers, see ProcThreadAttributeValue macro and Windows Internals 6 (372)
 SIZE_T Size;                        // Size of Value or *ValuePtr
 union
 {
  ULONG_PTR Value;                   // Reserve 8 bytes for data (such as a Handle or a data pointer)
  PVOID     ValuePtr;                // data pointer
 };
 PSIZE_T ReturnLength;               // Either 0 or specifies size of data returned to caller via "ValuePtr"
};

enum PS_IFEO_KEY_STATE
{
 PsReadIFEOAllValues,
 PsSkipIFEODebugger,
 PsSkipAllIFEO,
 PsMaxIFEOKeyStates
};

enum PS_CREATE_STATE
{
 PsCreateInitialState,
 PsCreateFailOnFileOpen,
 PsCreateFailOnSectionCreate,
 PsCreateFailExeFormat,
 PsCreateFailMachineMismatch,
 PsCreateFailExeName, // Debugger specified
 PsCreateSuccess,
 PsCreateMaximumStates
};

struct PS_CREATE_INFO
{
 SIZE_T Size;
 PS_CREATE_STATE State;

 union
 {
  // PsCreateInitialState
  struct
  {
      union
      {
          ULONG InitFlags;
          struct       // Fixed, untested
          {
              ULONG WriteOutputOnExit : 1;
              ULONG DetectManifest : 1;
              ULONG IFEOSkipDebugger : 1;
              ULONG IFEODoNotPropagateKeyState : 1;
              ULONG SpareBits1 : 4;
              ULONG SpareBits2 : 8;
              ULONG ProhibitedImageCharacteristics : 16;
          } s1;
      } u1;
      ACCESS_MASK AdditionalFileAccess;
  } InitState;

  // PsCreateFailOnSectionCreate
  struct
  {
      HANDLE FileHandle;
  } FailSection;

  // PsCreateFailExeFormat
  struct
  {
      USHORT DllCharacteristics;
  } ExeFormat;

  // PsCreateFailExeName
  struct
  {
      HANDLE IFEOKey;
  } ExeName;

  // PsCreateSuccess
  struct
  {
      union
      {
          ULONG OutputFlags;
          struct        // Fixed, untested
          {
              ULONG ProtectedProcess : 1;
              ULONG AddressSpaceOverride : 1;
              ULONG DevOverrideEnabled : 1; // From Image File Execution Options
              ULONG ManifestDetected : 1;
              ULONG ProtectedProcessLight : 1;
              ULONG SpareBits1 : 3;
              ULONG SpareBits2 : 8;
              ULONG SpareBits3 : 16;
          } s2;
      } u2;
      HANDLE FileHandle;
      HANDLE SectionHandle;
      ULONGLONG UserProcessParametersNative;
      ULONG UserProcessParametersWow64;
      ULONG CurrentParameterFlags;
      ULONGLONG PebAddressNative;
      ULONG PebAddressWow64;
      ULONGLONG ManifestAddress;
      ULONG ManifestSize;
  } SuccessState;
 };
};

using PPS_CREATE_INFO = MPTR<PS_CREATE_INFO, PHT>;

struct PS_ATTRIBUTE_LIST
{
 SIZE_T       TotalLength;           // sizeof(PS_ATTRIBUTE_LIST) + (N * sizeof(PS_ATTRIBUTE))
 PS_ATTRIBUTE Attributes[0];         // Depends on how many attribute entries should be supplied to NtCreateUserProcess
};
using PPS_ATTRIBUTE_LIST = MPTR<PS_ATTRIBUTE_LIST, PHT>;


// Handles all Envs/Args allocations and TEB/PEB hassle (CurProcess <> WOW64)
static NTSTATUS _scall NtCreateUserProcess(
			 PHANDLE ProcessHandle,
			 PHANDLE ThreadHandle,
			 ACCESS_MASK ProcessDesiredAccess,
			 ACCESS_MASK ThreadDesiredAccess,
			 POBJECT_ATTRIBUTES ProcessObjectAttributes,
			 POBJECT_ATTRIBUTES ThreadObjectAttributes,
			 ULONG ProcessFlags,         // EFlgCrtPrEx ?
			 ULONG ThreadFlags,          // EFlgCrtThEx
			 PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
			 PPS_CREATE_INFO CreateInfo,
			 PPS_ATTRIBUTE_LIST AttributeList
		);
//--------------------------------------------- Not Syscalls -------------------------------------------------
struct EXCEPTION_POINTERS     // TODO: ARM
{
 PEXCEPTION_RECORD ExceptionRecord;
 PCONTEXT          ContextRecord;
};

enum EExceptionCodes: LONG
{
 EXCEPTION_CONTINUE_SEARCH    = 0,
 EXCEPTION_EXECUTE_HANDLER    = 1,
 EXCEPTION_CONTINUE_EXECUTION = -1,

 EXCEPTION_NONCONTINUABLE     = 0x01,
 EXCEPTION_SOFTWARE_ORIGINATE = 0x80

};

using PEXCEPTION_POINTERS = MPTR<EXCEPTION_POINTERS, PHT>;

using PVECTORED_EXCEPTION_HANDLER = LONG (_scall *)(PEXCEPTION_POINTERS);

static ULONG    _scall RtlRemoveVectoredExceptionHandler(PVECTORED_EXCEPTION_HANDLER Handler);
static PVOID    _scall RtlAddVectoredExceptionHandler(ULONG First, PVECTORED_EXCEPTION_HANDLER Handler);
static NTSTATUS _scall LdrLoadDll(const PWSTR DllPath, PULONG DllCharacteristics, PUNICODE_STRING DllName, PVOID* DllHandle);
//------------------------------------------------------------------------------------------------------------
struct KSYSTEM_TIME
{
     ULONG LowPart;
     LONG High1Time;
     LONG High2Time;
};  
	
enum NT_PRODUCT_TYPE
{
         NtProductWinNt = 1,
         NtProductLanManNt = 2,
         NtProductServer = 3
};

enum ALTERNATIVE_ARCHITECTURE_TYPE
{
         StandardDesign = 0,
         NEC98x86 = 1,
         EndAlternatives = 2
};

enum SHARED_GLOBAL_FLAGS
{
  QPC_BYPASS_ENABLED       = 0x01,  // call NtQueryPerformanceCounter   // WinXP always calls NtQueryPerformanceCounter
  QPC_BYPASS_USE_HV_PAGE   = 0x02,  // (WDK for 1803);  call NtQueryPerformanceCounter
  QPC_BYPASS_DISABLE_32BIT = 0x04,  // (WDK for 1809);
  QPC_BYPASS_USE_MFENCE    = 0x10,  
  QPC_BYPASS_USE_LFENCE    = 0x20,  // For rdtsc
  QPC_BYPASS_A73_ERRATA    = 0x40,  // For rdtsc
  QPC_BYPASS_USE_RDTSCP    = 0x80,  // Use rdtscp (No fences needed?) // https://stackoverflow.com/questions/37682939/what-is-the-gcc-cpu-type-that-includes-support-for-rdtscp     // call the __cpuid intrinsic with InfoType=0x80000001 and check bit 27 of CPUInfo[3] (EDX)
};

// The TickCount and InterruptTime start from zero during the kernel's initialisation. The InterruptTime and the SystemTime are in units of 100ns. 
// Although what's wanted of the TickCount, InterruptTime and SystemTime is 64 bits each, they are stored as 12-byte KSYSTEM_TIME structures.
// The interrupt-time count begins at zero when the system starts and is incremented at each clock interrupt by the length of a clock tick. 
//   The exact length of a clock tick depends on underlying hardware and can vary between systems.
//   Unlike system time, the interrupt-time count is not subject to adjustments by users or the Windows time service.
// https://arush15june.github.io/posts/2020-07-12-clocks-timers-virtualization/
//
struct KUSER_SHARED_DATA
{
     ULONG TickCountLowDeprecated;
     ULONG TickCountMultiplier;
     KSYSTEM_TIME InterruptTime;    // 0 based (Not affected by system time change)   // QueryInterruptTime  // Vista: Related to 0x03B0 InterruptTimeBias;    // LINUX: CLOCK_MONOTONIC
     KSYSTEM_TIME SystemTime;       // Time based  // GetSystemTimeAsFileTime    // LINUX: CLOCK_REALTIME
     KSYSTEM_TIME TimeZoneBias;      
     WORD ImageNumberLow;
     WORD ImageNumberHigh;
     WCHAR NtSystemRoot[260];
     ULONG MaxStackTraceDepth;
     ULONG CryptoExponent;
     ULONG TimeZoneId;
     ULONG LargePageMinimum;
     ULONG Reserved2[7];
     NT_PRODUCT_TYPE NtProductType;
     UCHAR ProductTypeIsValid;
     ULONG NtMajorVersion;
     ULONG NtMinorVersion;
     UCHAR ProcessorFeatures[64];
     ULONG Reserved1;
     ULONG Reserved3;
     ULONG TimeSlip;
     ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
     LARGE_INTEGER SystemExpirationDate;
     ULONG SuiteMask;
     UCHAR KdDebuggerEnabled;
     UCHAR NXSupportPolicy;
     ULONG ActiveConsoleId;
     ULONG DismountCount;
     ULONG ComPlusPackage;
     ULONG LastSystemRITEventTickCount;
     ULONG NumberOfPhysicalPages;
     UCHAR SafeBootMode;
     ULONG SharedDataFlags;
     ULONG DbgErrorPortPresent: 1;
     ULONG DbgElevationEnabled: 1;
     ULONG DbgVirtEnabled: 1;
     ULONG DbgInstallerDetectEnabled: 1;
     ULONG SystemDllRelocated: 1;
     ULONG SpareBits: 27;
     UINT64 TestRetInstruction;
     ULONG SystemCall;              // LONGLONG QpcFrequency;  6.2 and higher
     ULONG SystemCallReturn;
     UINT64 SystemCallPad[3];
     union
     {
          KSYSTEM_TIME TickCount;   
          UINT64 TickCountQuad;
     };
     ULONG Cookie;
     INT64 ConsoleSessionForegroundProcessId;
     ULONG Wow64SharedInformation[16];
// Appended For Windows Vista
     WORD  UserModeGlobalLogger[8];
     ULONG HeapTracingPid[2];
     ULONG CritSecTracingPid[2];
     ULONG ImageFileExecutionOptions;
     union
     {
          UINT64 AffinityPad;
          ULONG ActiveProcessorAffinity;
     };
     UINT64 InterruptTimeBias;    // 0x03B0
// Appended For Windows 7
     ULONGLONG volatile TscQpcBias;    // 0x03B8  QpcBias
     ULONG ActiveProcessorCount;
     UCHAR volatile ActiveGroupCount;
     UCHAR Reserved9;
     union {
         USHORT QpcData;
         struct {
             UCHAR volatile QpcBypassEnabled;    // 0x03C6   BOOLEAN before 1709 then SHARED_GLOBAL_FLAGS   // Controls  rdtsc/rdtscp, lfence/mfence
             UCHAR QpcShift;                     // 0x03C7   // PerfCtr = (TscQpcBias + vRdtscVal64) >> QpcShift;
         };
     };
};
//------------------------------------------------------------------------------------------------------------
#ifdef SYS_WINDOWS

#ifdef CPU_ARM  // x32
#define CP15_PMSELR            15, 0,  9, 12, 5         // Event Counter Selection Register
#define CP15_PMXEVCNTR         15, 0,  9, 13, 2         // Event Count Register
#define CP15_TPIDRURW          15, 0, 13,  0, 2         // Software Thread ID Register, User Read/Write
#define CP15_TPIDRURO          15, 0, 13,  0, 3         // Software Thread ID Register, User Read Only
#define CP15_TPIDRPRW          15, 0, 13,  0, 4         // Software Thread ID Register, Privileged Only
#endif

static _finline TEB* NtCurrentTeb(void)
{
#ifdef ARCH_X64
#  ifdef CPU_ARM
 return (PTEB)__getReg(18);  // ARM64
#  else
 return (TEB*)__readgsqword((uint)&((NTSYS<uint64>::TEB*)nullptr)->NtTib.Self);  // Reads QWORD from TEB (TIB.self)    / !!! not 60!
#  endif
#else  // x32
#  ifdef CPU_ARM
 return (PTEB)nullptr; //(ULONG_PTR)_MoveFromCoprocessor(CP15_TPIDRURW);  // ARM32
#  else
 return (PTEB) (ULONG_PTR) __readfsdword((uint)&((NTSYS<uint32>::TEB*)nullptr)->NtTib.Self);   // 0x18
#  endif
#endif
}

static _finline PEB*   NtCurrentPeb(void) {return NtCurrentTeb()->ProcessEnvironmentBlock;}
static _finline ULONG  NtCurrentThreadId(void) {return ULONG(NtCurrentTeb()->ClientId.UniqueThread);}
static _finline ULONG  NtCurrentProcessId(void) {return ULONG(NtCurrentTeb()->ClientId.UniqueProcess);}
static _finline HANDLE RtlProcessHeap(void) {return (HANDLE)NtCurrentPeb()->ProcessHeap;}

//#define NtCurrentPeb()          (NtCurrentTeb()->ProcessEnvironmentBlock)
//#define NtCurrentProcessId()    (ULONG(NtCurrentTeb()->ClientId.UniqueProcess))
//#define NtCurrentThreadId()     (ULONG(NtCurrentTeb()->ClientId.UniqueThread))
//#define RtlProcessHeap()        (NtCurrentPeb()->ProcessHeap)

#endif // SYS_WINDOWS

#include "PlatWIN/NTStatus.hpp"

};

using NT   = NTSYS<uint>;
using NT32 = NTSYS<uint32>;
using NT64 = NTSYS<uint64>;
//============================================================================================================
/*
 https://github.com/NetSPI/MonkeyWorks/   // C# NT definitions
*/


