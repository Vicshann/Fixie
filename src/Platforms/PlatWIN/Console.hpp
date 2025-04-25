
#pragma once

struct NCON
{
// https://github.com/rprichard/win32-console-docs
//
// AllocConsole, AttachConsole, FreeConsole
// In releases prior to Windows 8, console handles are not true NT handles. Instead, the values are always multiples of four minus one (i.e. 0x3, 0x7, 0xb, 0xf, ...),
//   and the functions in kernel32.dll detect the special handles and perform LPCs to csrss.exe and/or conhost.exe
//============================================================================================================
// I was able to get it to work in C++ by calling SetConsoleOutputCP(65001) (set console code page to UTF-8)
//------------------------------------------------------------------------------------------------------------
// https://doxygen.reactos.org/d4/de1/csrmsg_8h.htm

#define 	IsConsoleHandle(h)    (((ULONG_PTR)(h) & 0x10000003) == 0x3)

// Windows Server 2003 table from http://j00ru.vexillium.org/csrss_list/api_list.html#Windows_2k3
enum CONSRV_API_NUMBER
{
    ConsolepOpenConsole = NCSR::CONSRV_FIRST_API_NUMBER,
    ConsolepGetConsoleInput,
    ConsolepWriteConsoleInput,
    ConsolepReadConsoleOutput,
    ConsolepWriteConsoleOutput,
    ConsolepReadConsoleOutputString,
    ConsolepWriteConsoleOutputString,
    ConsolepFillConsoleOutput,
    ConsolepGetMode,
    ConsolepGetNumberOfFonts,
    ConsolepGetNumberOfInputEvents,
    ConsolepGetScreenBufferInfo,
    ConsolepGetCursorInfo,
    ConsolepGetMouseInfo,
    ConsolepGetFontInfo,
    ConsolepGetFontSize,
    ConsolepGetCurrentFont,
    ConsolepSetMode,
    ConsolepSetActiveScreenBuffer,
    ConsolepFlushInputBuffer,
    ConsolepGetLargestWindowSize,
    ConsolepSetScreenBufferSize,
    ConsolepSetCursorPosition,
    ConsolepSetCursorInfo,
    ConsolepSetWindowInfo,
    ConsolepScrollScreenBuffer,
    ConsolepSetTextAttribute,
    ConsolepSetFont,
    ConsolepSetIcon,
    ConsolepReadConsole,
    ConsolepWriteConsole,
    ConsolepDuplicateHandle,
    ConsolepGetHandleInformation,
    ConsolepSetHandleInformation,
    ConsolepCloseHandle,
    ConsolepVerifyIoHandle,
    ConsolepAlloc,                          // Not present in Win7
    ConsolepFree,                           // Not present in Win7
    ConsolepGetTitle,
    ConsolepSetTitle,
    ConsolepCreateScreenBuffer,
    ConsolepInvalidateBitMapRect,
    ConsolepVDMOperation,
    ConsolepSetCursor,
    ConsolepShowCursor,
    ConsolepMenuControl,
    ConsolepSetPalette,
    ConsolepSetDisplayMode,
    ConsolepRegisterVDM,
    ConsolepGetHardwareState,
    ConsolepSetHardwareState,
    ConsolepGetDisplayMode,
    ConsolepAddAlias,
    ConsolepGetAlias,
    ConsolepGetAliasesLength,
    ConsolepGetAliasExesLength,
    ConsolepGetAliases,
    ConsolepGetAliasExes,
    ConsolepExpungeCommandHistory,
    ConsolepSetNumberOfCommands,
    ConsolepGetCommandHistoryLength,
    ConsolepGetCommandHistory,
    ConsolepSetCommandHistoryMode,          // Not present in Vista+
    ConsolepGetCP,
    ConsolepSetCP,
    ConsolepSetKeyShortcuts,
    ConsolepSetMenuClose,
    ConsolepNotifyLastClose,
    ConsolepGenerateCtrlEvent,
    ConsolepGetKeyboardLayoutName,
    ConsolepGetConsoleWindow,
    ConsolepCharType,
    ConsolepSetLocalEUDC,
    ConsolepSetCursorMode,
    ConsolepGetCursorMode,
    ConsolepRegisterOS2,
    ConsolepSetOS2OemFormat,
    ConsolepGetNlsMode,
    ConsolepSetNlsMode,
    ConsolepRegisterConsoleIME,             // Not present in Win7
    ConsolepUnregisterConsoleIME,           // Not present in Win7
    // ConsolepQueryConsoleIME,                // Added only in Vista and Win2k8, not present in Win7
    ConsolepGetLangId,
    ConsolepAttach,                         // Not present in Win7
    ConsolepGetSelectionInfo,
    ConsolepGetProcessList,

    ConsolepGetHistory,                     // Added in Vista+
    ConsolepSetHistory,                     // Added in Vista+
    // ConsolepSetCurrentFont,                 // Added in Vista+
    // ConsolepSetScreenBufferInfo,            // Added in Vista+
    // ConsolepClientConnect,                  // Added in Win7

    ConsolepMaxApiNumber
};
//============================================================================================================
struct SConSrv                // TODO: Init,SetCP,Write,Read
{
//------------------------------------------------------------------------------------------------------------
// https://doxygen.reactos.org/db/d80/conmsg_8h_source.html
//
struct CONSOLE_SETINPUTOUTPUTCP
{
 NT::HANDLE ConsoleHandle;     // NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->ConsoleHandle;
 NT::ULONG  CodePage;
 NT::BOOL   OutputCP;      // TRUE : Output Code Page ; FALSE : Input Code Page
 NT::HANDLE EventHandle;   // Present?
};
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS CallConSrv(uint32 Code, vptr Msg, uint32 Len) 
{
 //CsrClientCallServer(ApiMessage, 0i64, (CSR_API_NUMBER)0x20240, 24u);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
struct SConDrv   // From Windows 8
{
SCVR uint32 IOCTRL_CONDRV_CMD = 0x00500016;

enum CONDRV_API_NUMBER
{
 ConDrvSetInputOutputCP = 0x02000004,
};
//------------------------------------------------------------------------------------------------------------
struct SMsgBase
{
 NT::ULONG CtrlCode;
 NT::ULONG Length;     // Of a derived struct size
};
struct SMsgBuf
{
 NT::ULONG Length;      // Length comes first and we have 4 byte padding on X64
 NT::PVOID Pointer;
};
struct SConDrvIO
{
 NT::HANDLE Handle;         // Of what?
 NT::ULONG  InCnt;          // Number of buffers to the driver
 NT::ULONG  OutCnt;         // Number of buffers from the driver
 SMsgBuf    Messages[10];   // NumMsg1 + NumMsg2 + 2;
};
//------------------------------------------------------------------------------------------------------------
struct SMsgConInOutCP: SMsgBase    // Code: 0x02000004
{
 NT::ULONG  CodePage;
 NT::BOOL   OutputCP;      // TRUE : Output Code Page ; FALSE : Input Code Page
};
//------------------------------------------------------------------------------------------------------------
// https://www.unknowncheats.me/forum/c-and-c-/467327-cconsolecallservergeneric.html
// Starting from Win8
static NT::NTSTATUS CallConDrv(SMsgBase* Msg)   // For a single command
{
 NT::IO_STATUS_BLOCK IoStatusBlock;
 SConDrvIO io;
 io.Handle = 0;
 io.InCnt  = 1;   // To the driver
 io.OutCnt = 1;   // From the driver
 io.Messages[0].Length  = Msg->Length + sizeof(SMsgBase);  // derived size + sizeof(SMsgBase)
 io.Messages[0].Pointer = Msg;   // A message, derived from SMsgBase
 io.Messages[io.InCnt].Length  = Msg->Length;  // derived size
 io.Messages[io.InCnt].Pointer = &Msg[1];  // Points to a derived struct body (Data + sizeof(SMsgBase))

 int TotalBufs = io.InCnt + io.OutCnt + 3;  // +1 extra?
 return SAPI::NtDeviceIoControlFile(NT::NtCurrentPeb()->ProcessParameters->ConsoleHandle, 0, nullptr, nullptr, &IoStatusBlock, IOCTRL_CONDRV_CMD, &io, sizeof(SMsgBuf) * TotalBufs, nullptr, 0);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS SetConsoleCP(uint32 CodePageID, bool Output=true)    // Persistent?
{
 SMsgConInOutCP msg;
 msg.CtrlCode = 0x02000004;
 msg.Length   = sizeof(msg);     // Of a derived struct size
 msg.CodePage = CodePageID;
 msg.OutputCP = Output; 
 return CallConDrv(&msg);
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================

//------------------------------------------------------------------------------------------------------------
struct CONSOLE_READCONSOLE_CONTROL 
{
 NT::ULONG nLength;
 NT::ULONG nInitialChars;
 NT::ULONG dwCtrlWakeupMask;
 NT::ULONG dwControlKeyState;
};
//------------------------------------------------------------------------------------------------------------
enum EConModeFlags
{
 ENABLE_PROCESSED_INPUT             = 0x0001,
 ENABLE_LINE_INPUT                  = 0x0002,
 ENABLE_ECHO_INPUT                  = 0x0004,
 ENABLE_WINDOW_INPUT                = 0x0008,
 ENABLE_MOUSE_INPUT                 = 0x0010,
 ENABLE_INSERT_MODE                 = 0x0020,
 ENABLE_QUICK_EDIT_MODE             = 0x0040,
 ENABLE_EXTENDED_FLAGS              = 0x0080,
 ENABLE_AUTO_POSITION               = 0x0100,
 ENABLE_VIRTUAL_TERMINAL_INPUT      = 0x0200,
                                    
// Output Mode flags:               
 ENABLE_PROCESSED_OUTPUT            = 0x0001,
 ENABLE_WRAP_AT_EOL_OUTPUT          = 0x0002,
 ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004,
 DISABLE_NEWLINE_AUTO_RETURN        = 0x0008,
 ENABLE_LVB_GRID_WORLDWIDE          = 0x0010,
};
//------------------------------------------------------------------------------------------------------------
struct SConCtx
{  
struct SECURITY_ATTRIBUTES 
{
 NT::DWORD nLength;
 NT::PVOID lpSecurityDescriptor;
 NT::BOOL  bInheritHandle;
};

struct PROCESS_INFORMATION 
{
 NT::HANDLE hProcess;
 NT::HANDLE hThread;
 NT::DWORD  dwProcessId;
 NT::DWORD  dwThreadId;
};

struct STARTUPINFOW 
{
 NT::DWORD   cb;
 NT::PWSTR   lpReserved;
 NT::PWSTR   lpDesktop;
 NT::PWSTR   lpTitle;
 NT::DWORD   dwX;
 NT::DWORD   dwY;
 NT::DWORD   dwXSize;
 NT::DWORD   dwYSize;
 NT::DWORD   dwXCountChars;
 NT::DWORD   dwYCountChars;
 NT::DWORD   dwFillAttribute;
 NT::DWORD   dwFlags;
 NT::WORD    wShowWindow;
 NT::WORD    cbReserved2;
 NT::PBYTE   lpReserved2;
 NT::HANDLE  hStdInput;
 NT::HANDLE  hStdOutput;
 NT::HANDLE  hStdError;
};

struct STARTUPINFOEXW: STARTUPINFOW   // From Win7
{
 NT::PVOID ThreadAttrList;
};

// Creation flags
enum ECreationFlags: uint32
{
 DEBUG_PROCESS                    = 0x00000001,
 DEBUG_ONLY_THIS_PROCESS          = 0x00000002,
 CREATE_SUSPENDED                 = 0x00000004,
 DETACHED_PROCESS                 = 0x00000008,
                                             
 CREATE_NEW_CONSOLE               = 0x00000010,
 NORMAL_PRIORITY_CLASS            = 0x00000020,
 IDLE_PRIORITY_CLASS              = 0x00000040,
 HIGH_PRIORITY_CLASS              = 0x00000080,
                                              
 REALTIME_PRIORITY_CLASS          = 0x00000100,
 CREATE_NEW_PROCESS_GROUP         = 0x00000200,
 CREATE_UNICODE_ENVIRONMENT       = 0x00000400,
 CREATE_SEPARATE_WOW_VDM          = 0x00000800,
                                              
 CREATE_SHARED_WOW_VDM            = 0x00001000,
 CREATE_FORCEDOS                  = 0x00002000,
 BELOW_NORMAL_PRIORITY_CLASS      = 0x00004000,
 ABOVE_NORMAL_PRIORITY_CLASS      = 0x00008000,
                                             
 INHERIT_PARENT_AFFINITY          = 0x00010000,
 INHERIT_CALLER_PRIORITY          = 0x00020000,    // Deprecated
 CREATE_PROTECTED_PROCESS         = 0x00040000,
 EXTENDED_STARTUPINFO_PRESENT     = 0x00080000,    // STARTUPINFOEXW instead of STARTUPINFOW
                                              
 PROCESS_MODE_BACKGROUND_BEGIN    = 0x00100000,
 PROCESS_MODE_BACKGROUND_END      = 0x00200000,
 CREATE_SECURE_PROCESS            = 0x00400000,
 INHERIT_HANDLES                  = 0x00800000,   // Actually unused?     // Not WinSDK
                                             
 CREATE_BREAKAWAY_FROM_JOB        = 0x01000000,
 CREATE_PRESERVE_CODE_AUTHZ_LEVEL = 0x02000000,
 CREATE_DEFAULT_ERROR_MODE        = 0x04000000,
 CREATE_NO_WINDOW                 = 0x08000000,
                                              
 PROFILE_USER                     = 0x10000000,
 PROFILE_KERNEL                   = 0x20000000,
 PROFILE_SERVER                   = 0x40000000,
 CREATE_IGNORE_SYSTEM_DEFAULT     = 0x80000000,
};


using PTHREAD_START_ROUTINE = NT::DWORD (_scall *)(NT::PVOID lpThreadParameter);
             
static inline NT::BOOL (_scall *pAllocConsole)(void);                                                     
static inline NT::BOOL (_scall *pSetConsoleCP)(uint32 wCodePageID);
static inline NT::BOOL (_scall *pSetConsoleOutputCP)(uint32 wCodePageID);
static inline NT::BOOL (_scall *pSetConsoleMode)(NT::HANDLE hConsoleHandle, uint32 dwMode);
static inline NT::BOOL (_scall *pWriteConsoleA)(NT::HANDLE hConsoleOutput, vptr lpBuffer, uint32 nNumberOfCharsToWrite, uint32* lpNumberOfCharsWritten, vptr lpReserved);     // On >= Win8 will still use ConsoleCallServerGeneric ( NtDeviceIoControlFile )
static inline NT::BOOL (_scall *pReadConsoleA)(NT::HANDLE hConsoleInput, vptr lpBuffer, uint32 nNumberOfCharsToRead, uint32* lpNumberOfCharsRead, CONSOLE_READCONSOLE_CONTROL* pInputControl);
static inline NT::BOOL (_scall *pCreateProcessW)(NT::PWSTR lpApplicationName, NT::PWSTR lpCommandLine, SECURITY_ATTRIBUTES* lpProcessAttributes, SECURITY_ATTRIBUTES* lpThreadAttributes, NT::BOOL bInheritHandles, NT::DWORD dwCreationFlags, NT::PVOID lpEnvironment, NT::PWSTR lpCurrentDirectory, STARTUPINFOW* lpStartupInfo, PROCESS_INFORMATION* lpProcessInformation);
static inline NT::HANDLE (_scall *pCreateThread)(SECURITY_ATTRIBUTES* lpThreadAttributes, NT::SIZE_T dwStackSize, PTHREAD_START_ROUTINE lpStartAddress, NT::PVOID lpParameter, NT::DWORD dwCreationFlags, NT::PDWORD lpThreadId);


enum EFlags
{
 flInitialized = 0x0001,
 flNewWinVer   = 0x0002,       // NtWaitForAlertByThreadId Is appeared in Windows 8

};


static inline vptr pDllNtDll    = nullptr;
static inline vptr pDllKernel32 = nullptr;
static inline vptr pDllKrnlBase = nullptr;
static inline uint32 Flags = 0; 
//--------------------------------------------------
static vptr ResolveKProc(vptr* Addr, uint32 NameHash)
{
 vptr ptr = nullptr;
 if(pDllKrnlBase)ptr = NPE::GetProcAddr(vptr(size_t(pDllKrnlBase)|1), (achar*)NameHash);
 if(!ptr)
  {
   if(pDllKernel32)ptr = NPE::GetProcAddr(vptr(size_t(pDllKernel32)|1), (achar*)NameHash);
     else if(pDllNtDll)ptr = NPE::GetProcAddr(vptr(size_t(pDllNtDll)|1), (achar*)NameHash);
  }
 *Addr = ptr;
 return ptr;
}
//--------------------------------------------------     
static void Init(void)
{
 pDllNtDll    = NTX::LdrGetModuleBase(ConstEval(NCRYPT::CRC32("ntdll.dll")));        // Low Case only   // Is it too expensive to convert to UTF-8 at compile time?
 pDllKernel32 = NTX::LdrGetModuleBase(ConstEval(NCRYPT::CRC32("kernel32.dll")));
 pDllKrnlBase = NTX::LdrGetModuleBase(ConstEval(NCRYPT::CRC32("kernelbase.dll"))); 

 if(pDllNtDll && NPE::GetProcAddr(vptr(size_t(pDllNtDll)|1), (achar*)size_t(ConstEval(NCRYPT::CRC32("NtWaitForAlertByThreadId")))))Flags |= flNewWinVer;    // Beware of the CRC32 collisions (Especially in SAPI)
 ResolveKProc((vptr*)&pAllocConsole, ConstEval(NCRYPT::CRC32("AllocConsole")));
 ResolveKProc((vptr*)&pSetConsoleCP, ConstEval(NCRYPT::CRC32("SetConsoleCP"))); 
 ResolveKProc((vptr*)&pSetConsoleOutputCP, ConstEval(NCRYPT::CRC32("SetConsoleOutputCP")));
 ResolveKProc((vptr*)&pSetConsoleMode, ConstEval(NCRYPT::CRC32("SetConsoleMode")));
 ResolveKProc((vptr*)&pWriteConsoleA, ConstEval(NCRYPT::CRC32("WriteConsoleA")));
 ResolveKProc((vptr*)&pReadConsoleA, ConstEval(NCRYPT::CRC32("ReadConsoleA")));
 ResolveKProc((vptr*)&pCreateProcessW, ConstEval(NCRYPT::CRC32("CreateProcessW")));     
 ResolveKProc((vptr*)&pCreateThread, ConstEval(NCRYPT::CRC32("CreateThread")));
}

//--------------------------------------------------
static NT::HANDLE NormConHandle(NT::HANDLE hndl)  // Not supported by the framework
{
 switch((uint32)hndl )
  {
   case 0xFFFFFFF4:
     return (NT::HANDLE)NT::NtCurrentPeb()->ProcessParameters->StandardError;
   case 0xFFFFFFF5:
     return (NT::HANDLE)NT::NtCurrentPeb()->ProcessParameters->StandardOutput;
   case 0xFFFFFFF6:
    return (NT::HANDLE)NT::NtCurrentPeb()->ProcessParameters->StandardInput;
   default: return hndl;
  }
}
//--------------------------------------------------
// A real or pseudo handle 
//
static bool IsConHandle(NT::HANDLE hndl)
{
 if(hndl == (NT::HANDLE)NT::NtCurrentPeb()->ProcessParameters->StandardError)return true;
 if(hndl == (NT::HANDLE)NT::NtCurrentPeb()->ProcessParameters->StandardInput)return true;
 if(hndl == (NT::HANDLE)NT::NtCurrentPeb()->ProcessParameters->StandardOutput)return true;
 return false;
}
//-------------------------------------------------- 
};
//============================================================================================================

//------------------------------------------------------------------------------------------------------------
static bool _finline IsSpecConHandle(NT::HANDLE hndl){return !(SConCtx::Flags & SConCtx::flNewWinVer) && SConCtx::IsConHandle(hndl);}
//------------------------------------------------------------------------------------------------------------
static sint32 WriteConsole(NT::HANDLE hndl, vptr Buffer, uint32 Size)
{
 if(!SConCtx::pWriteConsoleA)return -1;
 uint32 res = 0;
 if(!SConCtx::pWriteConsoleA(SConCtx::NormConHandle(hndl), Buffer, Size, &res, nullptr))return -2;
 return res;
}
//------------------------------------------------------------------------------------------------------------
static sint32 ReadConsole(NT::HANDLE hndl, vptr Buffer, uint32 Size)
{
 if(!SConCtx::pWriteConsoleA)return -1;
 uint32 res = 0;
 if(!SConCtx::pReadConsoleA(SConCtx::NormConHandle(hndl), Buffer, Size, &res, nullptr))return -2;
 return res;
}
//------------------------------------------------------------------------------------------------------------
static NT::BOOL CreateProcess(wchar* AppName, wchar* CmdLine, NT::DWORD dwCreationFlags, SConCtx::STARTUPINFOW* lpStartupInfo, SConCtx::PROCESS_INFORMATION* lpProcessInformation, NT::PWSTR lpCurrentDirectory=nullptr, NT::PVOID lpEnvironment=nullptr, SConCtx::SECURITY_ATTRIBUTES* lpProcessAttributes=nullptr, SConCtx::SECURITY_ATTRIBUTES* lpThreadAttributes=nullptr)
{
 if(!SConCtx::pCreateProcessW)return false;
 bool InheritHndl = dwCreationFlags & SConCtx::INHERIT_HANDLES;
 dwCreationFlags &= ~SConCtx::INHERIT_HANDLES;
 return SConCtx::pCreateProcessW(AppName, CmdLine, lpProcessAttributes, lpThreadAttributes, dwCreationFlags, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}
//------------------------------------------------------------------------------------------------------------
static NT::HANDLE CreateThread(NT::DWORD dwCreationFlags, SConCtx::PTHREAD_START_ROUTINE lpStartAddress, NT::PVOID lpParameter=nullptr, NT::PDWORD lpThreadId=nullptr, NT::SIZE_T dwStackSize=0, SConCtx::SECURITY_ATTRIBUTES* lpThreadAttributes=nullptr)
{
 if(!SConCtx::pCreateThread)return 0;
 return SConCtx::pCreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
}
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
static int InitConsole(bool CrtIfMissing)
{
 if(NCON::SConCtx::Flags & NCON::SConCtx::flInitialized)return 0;
 NCON::SConCtx::Init();
 if(NCON::SConCtx::pAllocConsole && CrtIfMissing && (!NPTM::GetStdErr() || !NPTM::GetStdErr()))NCON::SConCtx::pAllocConsole();

// UTF-8 code page  // SetConsoleOutputCP: Not full UTF-8 and no always works on old Windows 
 if(NCON::SConCtx::pSetConsoleCP)NCON::SConCtx::pSetConsoleCP(65001);
    else NCON::SConDrv::SetConsoleCP(65001, false);
 if(NCON::SConCtx::pSetConsoleOutputCP)NCON::SConCtx::pSetConsoleOutputCP(65001);
    else NCON::SConDrv::SetConsoleCP(65001, true); 

 // TODO: SetConsoleMode  

 NCON::SConCtx::Flags &= 0x0001;
 return 0;
}
//============================================================================================================

