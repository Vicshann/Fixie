
#pragma once

#include "Wow64Ext.hpp"
//============================================================================================================
struct WOW64
{
#include "Wow64Structs.hpp"
//using wowu64 = uint64 alignas(uint64);

static uint64 _finline HndlExp(NT::HANDLE Handle){return uint64(sint64((sint32)Handle));}  // Some special handles are negative and must be correctly expanded with sign
static uint64 _finline FromPtrU64(auto ptr){return uint64(ptr?usize(*ptr):0);}
static void   _finline ToPtrU64(auto ptr, uint64 val){if(ptr)*ptr = (decltype(*ptr))val;}  // No setting SIZE_T to -1 of more than 0xFFFFFFFF (unlike actual WOW64 does)
// NOTE: Order of these functions is not have to be in sync with SAPI
// NOTE: Watch that PULONG usage weren't actually PSIZE_T
// https://groups.google.com/g/llvm-dev/c/xC_dgJBqmJo
//
// MSVC fastcall: ECX, EDX, [STACK]
// GCC:
// https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html
//    fastcall: ECX, EDX, [STACK]     // should be        // First arg (sci) should take two registers (uint64)
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtProtectVirtualMemory(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PPVOID BaseAddress, NT::PSIZE_T RegionSize, NT::ULONG NewProtect, NT::PULONG OldProtect)
{
 //uint64 exOldProtect  = FromPtrU64(OldProtect);
 uint64 exRegionSize  = FromPtrU64(RegionSize);    // x64 expects larger PVOIDs
 uint64 exBaseAddress = FromPtrU64(BaseAddress);
 NT::NTSTATUS res = (NT::NTSTATUS)WOW64E::X64SysCall(sci, 5, HndlExp(ProcessHandle), (uint64)&exBaseAddress, (uint64)&exRegionSize, (uint64)NewProtect, (uint64)OldProtect);    // The handle MUST be expanded if it is NtCurrentProcess (uint32(-1))
 //ToPtrU64(OldProtect,  exOldProtect);
 ToPtrU64(RegionSize,  exRegionSize);
 ToPtrU64(BaseAddress, exBaseAddress);
 return res;
}
//------------------------------------------------------------------------------------------------------------
// Untested!
static NT::NTSTATUS _fcall NtAllocateVirtualMemory(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PPVOID BaseAddress, NT::ULONG_PTR ZeroBits, NT::PSIZE_T RegionSize, NT::ULONG AllocationType, NT::ULONG Protect)
{
 uint64 exBaseAddr  = FromPtrU64(BaseAddress);
 uint64 exRegionSz  = FromPtrU64(RegionSize);
 uint64 exZeroBits  = ZeroBits;   // fits in uint64 without trouble
 NT::NTSTATUS res   = (NT::NTSTATUS)WOW64E::X64SysCall(sci, 6, HndlExp(ProcessHandle), (uint64)&exBaseAddr, (uint64)exZeroBits, (uint64)&exRegionSz, (uint64)AllocationType, (uint64)Protect);
 ToPtrU64(BaseAddress, exBaseAddr);
 ToPtrU64(RegionSize, exRegionSz);
 return res;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtFreeVirtualMemory(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PPVOID BaseAddress, NT::PSIZE_T RegionSize, NT::ULONG FreeType)
{
 uint64 exBaseAddress = FromPtrU64(BaseAddress);
 uint64 exRegionSize  = FromPtrU64(RegionSize);
 NT::NTSTATUS res     = (NT::NTSTATUS)WOW64E::X64SysCall(sci, 4, HndlExp(ProcessHandle), (uint64)&exBaseAddress, (uint64)&exRegionSize, (uint64)FreeType);
 ToPtrU64(BaseAddress, exBaseAddress);
 ToPtrU64(RegionSize,  exRegionSize);
 return res;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtReadVirtualMemory(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PVOID BaseAddress, NT::PVOID Buffer, NT::SIZE_T BufferSize, NT::PSIZE_T NumberOfBytesRead)
{   
 uint64 exNumberRead = 0;
 NT::NTSTATUS res    = (NT::NTSTATUS)WOW64E::X64SysCall(sci, 5, HndlExp(ProcessHandle), (uint64)BaseAddress, (uint64)Buffer, (uint64)BufferSize, (uint64)&exNumberRead);
 ToPtrU64(NumberOfBytesRead, exNumberRead);
 return res;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtWriteVirtualMemory(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PVOID BaseAddress, NT::PVOID Buffer, NT::SIZE_T BufferSize, NT::PSIZE_T NumberOfBytesWritten)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryVirtualMemory(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PVOID BaseAddress, NT::MEMORY_INFORMATION_CLASS MemoryInformationClass, NT::PVOID MemoryInformation, NT::SIZE_T MemoryInformationLength, NT::PSIZE_T ReturnLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtCreateFile(uint32 sci, uint32 hint, NT::PHANDLE FileHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PLARGE_INTEGER AllocationSize, NT::ULONG FileAttributes, NT::ULONG ShareAccess, NT::ULONG CreateDisposition, NT::ULONG CreateOptions, NT::PVOID EaBuffer, NT::ULONG EaLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtWriteFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::HANDLE Event, NT::PIO_APC_ROUTINE ApcRoutine, NT::PVOID ApcContext, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PVOID Buffer, NT::ULONG Length, NT::PLARGE_INTEGER ByteOffset, NT::PULONG Key)
{
 NT64::IO_STATUS_BLOCK ExIoStatusBlock;
 NT64::PIO_STATUS_BLOCK pExIoStatusBlock = IoStatusBlock?&ExIoStatusBlock:nullptr;
 uint64 ExApcRoutine = 0; // (uint64)ApcRoutine     // Requires some extra initialization!
 uint64 ExApcContext = 0; // (uint64)ApcContext
 NT::NTSTATUS res = (NT::NTSTATUS)WOW64E::X64SysCall(sci, 9, HndlExp(FileHandle), HndlExp(Event), ExApcRoutine, ExApcContext, (uint64)pExIoStatusBlock, (uint64)Buffer, (uint64)Length, (uint64)ByteOffset, (uint64)Key);
 if(IoStatusBlock)
  {
   IoStatusBlock->Pointer = ExIoStatusBlock.Pointer;
   IoStatusBlock->Status  = ExIoStatusBlock.Status;
   IoStatusBlock->Information = ExIoStatusBlock.Information;
  }
 return res;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtReadFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::HANDLE Event, NT::PIO_APC_ROUTINE ApcRoutine, NT::PVOID ApcContext, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PVOID Buffer, NT::ULONG Length, NT::PLARGE_INTEGER ByteOffset, NT::PULONG Key)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtDeleteFile(uint32 sci, uint32 hint, NT::POBJECT_ATTRIBUTES ObjectAttributes)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtWriteFileGather(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::HANDLE Event, NT::PIO_APC_ROUTINE ApcRoutine, NT::PVOID ApcContext, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PFILE_SEGMENT_ELEMENT SegmentArray, NT::ULONG Length, NT::PLARGE_INTEGER ByteOffset, NT::PULONG Key)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtReadFileScatter(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::HANDLE Event, NT::PIO_APC_ROUTINE ApcRoutine, NT::PVOID ApcContext, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PFILE_SEGMENT_ELEMENT SegmentArray, NT::ULONG Length, NT::PLARGE_INTEGER ByteOffset, NT::PULONG Key)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtFlushBuffersFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::PIO_STATUS_BLOCK IoStatusBlock)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryAttributesFile(uint32 sci, uint32 hint, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::PFILE_BASIC_INFORMATION FileInformation)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryInformationFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PVOID FileInformation, NT::ULONG Length, NT::FILE_INFORMATION_CLASS FileInformationClass)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryDirectoryFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::HANDLE Event, NT::PIO_APC_ROUTINE ApcRoutine, NT::PVOID ApcContext, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PVOID FileInformation, NT::ULONG Length, NT::FILE_INFORMATION_CLASS FileInformationClass, NT::BOOLEAN ReturnSingleEntry, NT::PUNICODE_STRING FileName, NT::BOOLEAN RestartScan)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtSetInformationFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PVOID FileInformation, NT::ULONG Length, NT::FILE_INFORMATION_CLASS FileInformationClass)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtMapViewOfSection(uint32 sci, uint32 hint, NT::HANDLE SectionHandle, NT::HANDLE ProcessHandle, NT::PVOID* BaseAddress, NT::ULONG_PTR ZeroBits, NT::SIZE_T CommitSize, NT::PLARGE_INTEGER SectionOffset, NT::PSIZE_T ViewSize, NT::SECTION_INHERIT InheritDisposition, NT::ULONG AllocationType, NT::ULONG Win32Protect)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtUnmapViewOfSection(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PVOID BaseAddress)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtCreateSection(uint32 sci, uint32 hint, NT::PHANDLE SectionHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::PLARGE_INTEGER MaximumSize, NT::ULONG SectionPageProtection, NT::ULONG AllocationAttributes, NT::HANDLE FileHandle)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtOpenSection(uint32 sci, uint32 hint, NT::PHANDLE SectionHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQuerySection(uint32 sci, uint32 hint, NT::HANDLE SectionHandle, NT::SECTION_INFORMATION_CLASS SectionInformationClass, NT::PVOID SectionInformation, NT::SIZE_T SectionInformationLength, NT::PSIZE_T ReturnLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtCreateSymbolicLinkObject(uint32 sci, uint32 hint, NT::PHANDLE LinkHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::PUNICODE_STRING DestinationName)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtOpenSymbolicLinkObject(uint32 sci, uint32 hint, NT::PHANDLE LinkHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQuerySymbolicLinkObject(uint32 sci, uint32 hint, NT::HANDLE LinkHandle, NT::PUNICODE_STRING LinkTarget, NT::PULONG ReturnedLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _scall NtQueryInformationThread(uint32 sci, uint32 hint, NT::HANDLE ThreadHandle, NT::THREADINFOCLASS ThreadInformationClass, NT::PVOID ThreadInformation, NT::ULONG ThreadInformationLength, NT::PULONG ReturnLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryInformationProcess(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::PROCESSINFOCLASS ProcessInformationClass, NT::PVOID ProcessInformation, NT::ULONG ProcessInformationLength, NT::PULONG ReturnLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtClose(uint32 sci, uint32 hint, NT::HANDLE Handle)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtDelayExecution(uint32 sci, uint32 hint, NT::BOOLEAN Alertable, NT::PLARGE_INTEGER DelayInterval)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtCreateThread(uint32 sci, uint32 hint, NT::PHANDLE ThreadHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::HANDLE ProcessHandle, NT::PCLIENT_ID ClientId, NT::PVOID ThreadContext, NT::PUSER_STACK UserStack, NT::BOOLEAN CreateSuspended)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtCreateProcess(uint32 sci, uint32 hint, NT::PHANDLE ProcessHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::HANDLE ParentProcess, NT::BOOLEAN InheritObjectTable, NT::HANDLE SectionHandle, NT::HANDLE DebugPort, NT::HANDLE ExceptionPort)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtCreateProcessEx(uint32 sci, uint32 hint, NT::PHANDLE ProcessHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::HANDLE ParentProcess, NT::ULONG Flags, NT::HANDLE SectionHandle, NT::HANDLE DebugPort, NT::HANDLE ExceptionPort, NT::BOOLEAN InJob)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtCreateUserProcess(uint32 sci, uint32 hint,
			 NT::PHANDLE ProcessHandle,
			 NT::PHANDLE ThreadHandle,
			 NT::ACCESS_MASK ProcessDesiredAccess,
			 NT::ACCESS_MASK ThreadDesiredAccess,
			 NT::POBJECT_ATTRIBUTES ProcessObjectAttributes,
			 NT::POBJECT_ATTRIBUTES ThreadObjectAttributes,
			 NT::ULONG ProcessFlags,
			 NT::ULONG ThreadFlags,
			 NT::PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
			 NT::PPS_CREATE_INFO CreateInfo,
			 NT::PPS_ATTRIBUTE_LIST AttributeList)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtResumeThread(uint32 sci, uint32 hint, NT::HANDLE ThreadHandle, NT::PULONG PreviousSuspendCount)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtSuspendThread(uint32 sci, uint32 hint, NT::HANDLE ThreadHandle, NT::PULONG PreviousSuspendCount)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtGetContextThread(uint32 sci, uint32 hint, NT::HANDLE ThreadHandle, NT::PCONTEXT ThreadContext)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtSetContextThread(uint32 sci, uint32 hint, NT::HANDLE ThreadHandle, NT::PCONTEXT ThreadContext)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtTerminateThread(uint32 sci, uint32 hint, NT::HANDLE ThreadHandle, NT::NTSTATUS ExitStatus)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtTerminateProcess(uint32 sci, uint32 hint, NT::HANDLE ProcessHandle, NT::NTSTATUS ExitStatus)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtWaitForSingleObject(uint32 sci, uint32 hint, NT::HANDLE Handle, NT::BOOLEAN Alertable, NT::PLARGE_INTEGER Timeout)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _scall NtWaitForMultipleObjects(uint32 sci, uint32 hint, NT::ULONG Count, NT::PHANDLE Handles, NT::WAIT_TYPE WaitType, NT::BOOLEAN Alertable, NT::PLARGE_INTEGER Timeout)
{
 return 0;
}
static NT::NTSTATUS _fcall NtLoadDriver(uint32 sci, uint32 hint, NT::PUNICODE_STRING DriverServiceName)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtUnloadDriver(uint32 sci, uint32 hint, NT::PUNICODE_STRING DriverServiceName)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtFsControlFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::HANDLE Event, NT::PIO_APC_ROUTINE ApcRoutine, NT::PVOID ApcContext, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::ULONG FsControlCode, NT::PVOID InputBuffer, NT::ULONG InputBufferLength, NT::PVOID OutputBuffer, NT::ULONG OutputBufferLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtDeviceIoControlFile(uint32 sci, uint32 hint, NT::HANDLE FileHandle, NT::HANDLE Event, NT::PIO_APC_ROUTINE ApcRoutine, NT::PVOID ApcContext, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::ULONG IoControlCode, NT::PVOID InputBuffer, NT::ULONG InputBufferLength, NT::PVOID OutputBuffer, NT::ULONG OutputBufferLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryObject(NT::HANDLE Handle, NT::OBJECT_INFORMATION_CLASS ObjectInformationClass, NT::PVOID ObjectInformation, NT::ULONG ObjectInformationLength, NT::PULONG ReturnLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtOpenThread(NT::PHANDLE ThreadHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::PCLIENT_ID ClientId)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtOpenProcess(NT::PHANDLE ProcessHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes, NT::PCLIENT_ID ClientId)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryVolumeInformationFile(NT::HANDLE FileHandle, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::PVOID FsInformation, NT::ULONG Length, NT::FS_INFORMATION_CLASS FsInformationClass)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtOpenDirectoryObject(NT::PHANDLE DirectoryHandle, NT::ACCESS_MASK DesiredAccess, NT::POBJECT_ATTRIBUTES ObjectAttributes)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall NtQueryDirectoryObject(NT::HANDLE DirectoryHandle, NT::PVOID Buffer, NT::ULONG BufferLength, NT::BOOLEAN ReturnSingleEntry, NT::BOOLEAN RestartScan, NT::PULONG Context, NT::PULONG ReturnLength)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
};
