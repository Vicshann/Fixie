
#pragma once

// This is basically Kernel32.dll wrapper now. It imports Console,Process,Thread and Window API. 
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
 NT::ULONG Length;      // Size of a derived struct 
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

struct SMsgConMode: SMsgBase       // Code: 0x01000002
{
 NT::ULONG Mode;
};
//------------------------------------------------------------------------------------------------------------
// https://www.unknowncheats.me/forum/c-and-c-/467327-cconsolecallservergeneric.html
// Starting from Win8
static NT::NTSTATUS CallConDrv(SMsgBase* Msg, NT::HANDLE hndl=0)   // For a single command
{
 NT::IO_STATUS_BLOCK IoStatusBlock;
 SConDrvIO io;
 io.Handle = hndl;
 io.InCnt  = 1;   // To the driver
 io.OutCnt = 1;   // From the driver
 io.Messages[0].Length  = Msg->Length + sizeof(SMsgBase);  // derived size + sizeof(SMsgBase)
 io.Messages[0].Pointer = Msg;   // A message, derived from SMsgBase
 io.Messages[io.InCnt].Length  = Msg->Length;  // derived size
 io.Messages[io.InCnt].Pointer = &Msg[1];      // Points to a derived struct body (Data + sizeof(SMsgBase))

 int TotalBufs = io.InCnt + io.OutCnt + 3;  // +1 extra?
 return SAPI::NtDeviceIoControlFile(NT::NtCurrentPeb()->ProcessParameters->ConsoleHandle, 0, nullptr, nullptr, &IoStatusBlock, IOCTRL_CONDRV_CMD, &io, sizeof(SMsgBuf) * TotalBufs, nullptr, 0);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS SetConsoleCP(uint32 CodePageID, bool Output=true)    // Persistent?
{
 SMsgConInOutCP msg;
 msg.CtrlCode = 0x02000004;
 msg.Length   = sizeof(msg);     
 msg.CodePage = CodePageID;
 msg.OutputCP = Output; 
 return CallConDrv(&msg);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS GetConsoleMode(NT::HANDLE ConHndl, uint32* Mode) 
{
 SMsgConMode msg;
 msg.CtrlCode = 0x1000001;
 msg.Length   = sizeof(msg);    
 msg.Mode     = 0;
 NT::NTSTATUS res = CallConDrv(&msg, ConHndl);
 *Mode = msg.Mode;
 return res;
}
//------------------------------------------------------------------------------------------------------------

static NT::NTSTATUS SetConsoleMode(NT::HANDLE ConHndl, uint32 Mode) 
{
 SMsgConMode msg;
 msg.CtrlCode = 0x01000002;
 msg.Length   = sizeof(msg);    
 msg.Mode     = Mode;
 return CallConDrv(&msg, ConHndl);
}
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
// By default, the Linux TTY driver operates in Canonical Mode. This is very similar to Windows' ENABLE_LINE_INPUT.
//    Line Buffered: The kernel buffers input until the user hits Enter.
//    Automatic Echo: The kernel automatically draws characters to the screen as they are typed.
//    Editing: Backspace is handled by the terminal driver itself, not your application.
//    Behavior: In this mode, read() in Linux behaves just like ReadFile() in Windows with default flags.

// Applications like shells (Bash, Zsh) or text editors (Vim) immediately switch the terminal to Non-Canonical Mode using the termios struct.
//    Char-by-Char: Every byte is sent to the application the moment it is pressed.
//    No Automatic Echo: The application is now responsible for displaying the character.
//    This is the only way to support features like Tab-completion or Syntax highlighting while typing. If the terminal driver automatically echoed the characters, the application couldn't "intercept" a Tab key to show a list of files—it would just print a literal Tab.

// Canonical Mode (also called "cooked mode"):
// 
//   This is the default mode
//   The terminal driver buffers input line-by-line
//   Special characters are processed (Ctrl+C, Ctrl+D, backspace, etc.)
//   Input is only sent to the program when you press Enter
//   Line editing works (you can backspace, use arrow keys for history)
//   Most typical command-line programs use this
// 
// Raw Mode (also called "non-canonical mode"):
// 
//   Input is passed character-by-character immediately
//   No special processing of control characters
//   No line buffering - programs get each keystroke instantly
//   Programs like vim, less, top, and full-screen terminal applications use this
//   The program must handle everything itself (including backspace)
// 
enum EConModeFlags           // https://learn.microsoft.com/en-us/windows/console/setconsolemode
{
// Input mode flags:
 ENABLE_PROCESSED_INPUT             = 0x0001,
 ENABLE_LINE_INPUT                  = 0x0002,  // The ReadFile or ReadConsole function returns only when a carriage return character is read. If this mode is disabled, the functions return when one or more characters are available.
 ENABLE_ECHO_INPUT                  = 0x0004,  // Characters read by the ReadFile or ReadConsole function are written to the active screen buffer as they are typed into the console. Only with ENABLE_LINE_INPUT
 ENABLE_WINDOW_INPUT                = 0x0008,  // Retrievable only by ReadConsoleInput
 ENABLE_MOUSE_INPUT                 = 0x0010,  // Retrievable only by ReadConsoleInput
 ENABLE_INSERT_MODE                 = 0x0020,
 ENABLE_QUICK_EDIT_MODE             = 0x0040,  // To enable this mode, use ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS  // Quick Edit Mode blocks the program while the user is selecting text!
 ENABLE_EXTENDED_FLAGS              = 0x0080,  // Extends affected flags to ENABLE_QUICK_EDIT_MODE
 ENABLE_AUTO_POSITION               = 0x0100,
 ENABLE_VIRTUAL_TERMINAL_INPUT      = 0x0200,  // Converts user input (like arrow keys) into standard VT escape sequences that ReadFile can then retrieve as simple bytes.
                                    
// Output Mode flags:               
 ENABLE_PROCESSED_OUTPUT            = 0x0001,  // Characters written by the WriteFile or WriteConsole function or echoed by the ReadFile or ReadConsole function are parsed for ASCII control sequences, and the correct action is performed. Backspace, tab, bell, carriage return, and line feed characters are processed. It should be enabled when using control sequences or when ENABLE_VIRTUAL_TERMINAL_PROCESSING is set.
 ENABLE_WRAP_AT_EOL_OUTPUT          = 0x0002,  // Linux terminals do wrap by default
 ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004,  // Enables the console to interpret VT escape sequences for formatted output.
 DISABLE_NEWLINE_AUTO_RETURN        = 0x0008,  // A key piece for emulating Linux terminals (like WSL) where a Line Feed (\n) should not automatically perform a Carriage Return (\r). (\n) will move the cursor straight down to the next row but keep it in the same column.
 ENABLE_LVB_GRID_WORLDWIDE          = 0x0010,
};

enum EConEvents
{
 CTRL_C_EVENT        = 0,
 CTRL_BREAK_EVENT    = 1,
 CTRL_CLOSE_EVENT    = 2,
 CTRL_LOGOFF_EVENT   = 5,
 CTRL_SHUTDOWN_EVENT = 6
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
static inline NT::BOOL (_scall *pAttachConsole)(uint32 dwProcessId);                                                  
static inline NT::BOOL (_scall *pSetConsoleCP)(uint32 wCodePageID);
static inline NT::BOOL (_scall *pSetConsoleOutputCP)(uint32 wCodePageID);
static inline NT::BOOL (_scall *pGenerateConsoleCtrlEvent)(uint32 dwCtrlEvent, uint32 dwProcessGroupId);     // TODO: Implement process_signal with it
static inline NT::BOOL (_scall *pSetConsoleCtrlHandler)(vptr HandlerRoutine, uint32 Add);
static inline NT::BOOL (_scall *pSetConsoleMode)(NT::HANDLE hConsoleHandle, uint32 dwMode);
static inline NT::BOOL (_scall *pGetConsoleMode)(NT::HANDLE hConsoleHandle, uint32* dwMode);
static inline NT::BOOL (_scall *pWriteConsoleA)(NT::HANDLE hConsoleOutput, vptr lpBuffer, uint32 nNumberOfCharsToWrite, uint32* lpNumberOfCharsWritten, vptr lpReserved);     // On >= Win8 will still use ConsoleCallServerGeneric ( NtDeviceIoControlFile )
static inline NT::BOOL (_scall *pReadConsoleA)(NT::HANDLE hConsoleInput, vptr lpBuffer, uint32 nNumberOfCharsToRead, uint32* lpNumberOfCharsRead, CONSOLE_READCONSOLE_CONTROL* pInputControl);
static inline NT::BOOL (_scall *pCreateProcessW)(NT::PWSTR lpApplicationName, NT::PWSTR lpCommandLine, SECURITY_ATTRIBUTES* lpProcessAttributes, SECURITY_ATTRIBUTES* lpThreadAttributes, NT::BOOL bInheritHandles, NT::DWORD dwCreationFlags, NT::PVOID lpEnvironment, NT::PWSTR lpCurrentDirectory, STARTUPINFOW* lpStartupInfo, PROCESS_INFORMATION* lpProcessInformation);
static inline NT::HANDLE (_scall *pCreateThread)(SECURITY_ATTRIBUTES* lpThreadAttributes, NT::SIZE_T dwStackSize, PTHREAD_START_ROUTINE lpStartAddress, NT::PVOID lpParameter, NT::DWORD dwCreationFlags, NT::PDWORD lpThreadId);

static inline NT::ULONG (_scall *pRtlRemoveVectoredExceptionHandler)(NT::PVECTORED_EXCEPTION_HANDLER Handler);
static inline NT::PVOID (_scall *pRtlAddVectoredExceptionHandler)(NT::ULONG First, NT::PVECTORED_EXCEPTION_HANDLER Handler);

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
 if(pDllKrnlBase)ptr = NPE::GetProcAddr(vptr(size_t(pDllKrnlBase)|1), (achar*)size_t(NameHash));
 if(!ptr)
  {
   if(pDllKernel32)ptr = NPE::GetProcAddr(vptr(size_t(pDllKernel32)|1), (achar*)size_t(NameHash));
     else if(pDllNtDll)ptr = NPE::GetProcAddr(vptr(size_t(pDllNtDll)|1), (achar*)size_t(NameHash));
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

 if(pDllNtDll)
  {
   vptr MPtr = vptr(size_t(pDllNtDll)|1);
   if(NPE::GetProcAddr(MPtr, (achar*)size_t(ConstEval(NCRYPT::CRC32("NtWaitForAlertByThreadId")))))Flags |= flNewWinVer;    // Beware of the CRC32 collisions (Especially in SAPI)
   *(vptr*)&pRtlAddVectoredExceptionHandler = NPE::GetProcAddr(MPtr, (achar*)size_t(ConstEval(NCRYPT::CRC32("RtlAddVectoredExceptionHandler"))));
   *(vptr*)&pRtlRemoveVectoredExceptionHandler = NPE::GetProcAddr(MPtr, (achar*)size_t(ConstEval(NCRYPT::CRC32("RtlRemoveVectoredExceptionHandler"))));
  }                                                                             
 ResolveKProc((vptr*)&pAllocConsole, ConstEval(NCRYPT::CRC32("AllocConsole")));        // TODO: Compile-time hash obfuscation
 ResolveKProc((vptr*)&pAttachConsole, ConstEval(NCRYPT::CRC32("AttachConsole")));
 ResolveKProc((vptr*)&pSetConsoleCP, ConstEval(NCRYPT::CRC32("SetConsoleCP")));  
 ResolveKProc((vptr*)&pSetConsoleOutputCP, ConstEval(NCRYPT::CRC32("SetConsoleOutputCP")));
 ResolveKProc((vptr*)&pSetConsoleCtrlHandler, ConstEval(NCRYPT::CRC32("SetConsoleCtrlHandler")));
 ResolveKProc((vptr*)&pGetConsoleMode, ConstEval(NCRYPT::CRC32("GetConsoleMode")));
 ResolveKProc((vptr*)&pSetConsoleMode, ConstEval(NCRYPT::CRC32("SetConsoleMode")));
 ResolveKProc((vptr*)&pWriteConsoleA, ConstEval(NCRYPT::CRC32("WriteConsoleA")));
 ResolveKProc((vptr*)&pReadConsoleA, ConstEval(NCRYPT::CRC32("ReadConsoleA")));
 ResolveKProc((vptr*)&pCreateProcessW, ConstEval(NCRYPT::CRC32("CreateProcessW")));     
 ResolveKProc((vptr*)&pCreateThread, ConstEval(NCRYPT::CRC32("CreateThread")));
 // TODO: GetConsoleScreenBufferInfo for console size and cursor position, SetConsoleCursorPosition, SetConsoleTextAttribute for colors, etc.  // Maybe not needed since the framework should be able to work with any console and does not need to control it directly
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
static bool _finline HasConsole(void ) 
{
 auto ppar = NT::NtCurrentPeb()->ProcessParameters;
 return ppar && ppar->ConsoleHandle && (ppar->ConsoleHandle != NT::NtInvalidHandle);
}
//--------------------------------------------------           // https://github.com/EvilBytecode/GhostVEH    // Needs RtlEncodePointer and RtlDecodePointer anyway
};

//============================================================================================================
// https://unix.stackexchange.com/questions/251195/difference-between-less-violent-kill-signal-hup-1-int-2-and-term-15
// 
// Propagates console events to a signal handler. It runs on a new, injected thread.
// Unlike POSIX SIGTERM, where a process can choose to stay alive indefinitely, Windows will unconditionally kill the process after 
//  the HandlerRoutine returns or times out (usually 5 seconds for close/logoff
// If your console app loads user32.dll or gdi32.dll (even indirectly through another library), you will stop receiving CTRL_LOGOFF_EVENT and CTRL_SHUTDOWN_EVENT via the console handler. 
//  In those cases, you must create a Hidden Window and listen for WM_QUERYENDSESSION.
// Returning TRUE for CTRL_CLOSE_EVENT prevents the "Application Not Responding" dialog but doesn't stop the exit; it just gives you the timeout window to finish.
// The 5-Second Rule: If your cleanup takes 10 seconds, and the user clicked the "X" button, Windows will pop up a "This app is not responding" box at the 5-second mark. 
//  There is no way to "ignore" a Close event forever like you can with SIG_IGN in Linux.
// The "Main Thread" issue: In Linux, your main thread is interrupted. In Windows, your main thread keeps running! If your main thread is blocked on a 
// synchronous call (like std::cin.get() or a blocking recv()), it will never see the g_stop_requested flag. You may need to use CancelSynchronousIo or non-blocking 
// calls to ensure the main thread can actually reach the cleanup code.
// 
// To send a Ctrl+C to another process group, you must use GenerateConsoleCtrlEvent.
// Constraint: The target process must share the same console as the caller.
// Group ID: The dwProcessGroupId used here is the PID of the process that was the "leader" of that group.
// kill(pid, SIGKILL): Use NtTerminateProcess (Ntdll). It is the only way to guarantee the process stops immediately.
// kill(pgid, SIGINT): Use GenerateConsoleCtrlEvent (Kernel32). This triggers the CTRL_C_EVENT in all processes that share the console and belong to the specified group. 
// SIGQUIT	PGID	Call GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, PGID).
// GenerateConsoleCtrlEvent only works if the target process shares the same console window as you. If you are writing a background service or a GUI app trying to "kill" a console app,
// this call will fail. In those cases, most cross-platform frameworks (like libuv) resort to NtTerminateProcess for all signals, as there is no reliable way to "signal" a detached process without a custom IPC pipe.
// Windows processes created with CREATE_NEW_PROCESS_GROUP have Ctrl+C ignored by default. To make them responsive to signals, they must explicitly call SetConsoleCtrlHandler(NULL, FALSE) to re-enable them.
// In some Windows versions, the call may return success (non-zero) even if the target process is not actually attached to a console (e.g., it's a DETACHED_PROCESS). In this case, the signal is sent but essentially "disappears" because there is no console handler to receive it.
// SIGKILL	PID	Call NtTerminateProcess (immediate, non-catchable).
// SIGINT	PID	Call GenerateConsoleCtrlEvent(CTRL_C_EVENT, PID).
// SIGQUIT	PGID	Call GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, PGID).
//
static NT::BOOL _scall ConsoleHandlerProc(NT::DWORD CtrlType)   // NOTE: No point in calling this directly
{
 PX::siginfo_t inf = {};  
 switch(CtrlType) 
  {
   case CTRL_C_EVENT:        // Keyboard Ctrl+C or kill -2      // Can be non-terminating if we return TRUE
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_INT))return 0;  // Not handling this 
     inf.signo = PX::SIG_INT;
     inf.code  = PX::SI_KERNEL;
     if(NSIG::SignalActionHandler(inf.signo, &inf, nullptr))return 1;     // SIGINT is the "weakest" of the lot. Its conventional meaning is "stop what you're doing right now and wait for further user input". It's the signal generated by Ctrl+C in a terminal. Non-interactive programs generally treat it like SIGTERM.
     break;

   case CTRL_BREAK_EVENT:    // Keyboard Ctrl+Break or kill -3  // Can be non-terminating if we return TRUE 
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_QUIT))return 0;  // Not handling this
     inf.signo = PX::SIG_QUIT;
     inf.code  = PX::SI_KERNEL;
     if(NSIG::SignalActionHandler(inf.signo, &inf, nullptr))return 1;     // Break is a "harder" interrupt   // Actually kind of an error in POSIX
     break;

   case CTRL_CLOSE_EVENT:    // User closed the console window  // Some programs use SIGHUP to reload configuration files (daemons). 
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_TERM))return 0;  // Not handling this
     inf.signo = PX::SIG_TERM;
     inf.code  = PX::SI_USER;    // (simulating a "polite" request to die).      // For SIGTERM / SIGHUP   // SI_USER (if sent via kill) 
     NSIG::SignalActionHandler(inf.signo, &inf, nullptr);    // While SIGHUP (terminal hang up) is semantically close, most cross-platform libraries use SIGTERM because it signals a request for graceful termination.
     break;

// Windows 7, Windows 8, Windows 8.1 and Windows 10: Should use a hidden windows  and listen to WM_QUERYENDSESSION and WM_ENDSESSION if using gdi32.dll or user32.dll library
// Once the Handler returns TRUE, the OS immediately kills the process. You don't call exit() yourself; returning from the handler is what triggers the final termination.
   case CTRL_LOGOFF_EVENT:       // ~5s seconds timeout  // Note that this signal is received only by services. Interactive applications are terminated at logoff, so they are not present when the system sends this signal.
   case CTRL_SHUTDOWN_EVENT:     // ~5s seconds timeout (20 for a service)  // Interactive applications are not present by the time the system sends this signal, therefore it can be received only be services in this situation. Services also have their own notification mechanism for shutdown events.
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_TERM))return 0;  // Not handling this
     inf.signo = PX::SIG_TERM;
     inf.code  = PX::SI_KERNEL;
     NSIG::SignalActionHandler(inf.signo, &inf, nullptr);    // These are system-wide termination requests. 
     break;

   default: return 0;  // We do not handle this
  }

 AtomicAnd(&fwsinf.Flags, sfTerminating);
 NT::LARGE_INTEGER delay = -(4 * 10000000LL);   // Wait for 4 seconds (More mat cause 'App in not responding' message)  (DispatchSignal is supposed to set some termination flag too)
 SAPI::NtDelayExecution(false, &delay);         // just "buy time" for any workers to finish
 return 0;  // Give other handlers a chance to clean up (only if we ignored the termination notification and didn't terminated the process ourselves)
}
//------------------------------------------------------------------------------------------------------------
// On Linux, when the kernel can't determine the exact faulting address (rare but possible), it also sets si_addr to 0 or leaves it undefined.
//
static NT::LONG _scall ExceptionHandlerProc(NT::PEXCEPTION_POINTERS ep) 
{
 if(ep->ExceptionRecord->ExceptionFlags & NT::EXCEPTION_SOFTWARE_ORIGINATE)return NT::EXCEPTION_CONTINUE_SEARCH;   // All RaiseException exceptions (C++ throw)
 PX::siginfo_t inf = {};  
 switch(ep->ExceptionRecord->ExceptionCode) 
  {
   case NT::STATUS_BREAKPOINT:              // int 3 or debugger trap
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_TRAP))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_TRAP;
     inf.code  = PX::TRAP_BRKPT;
     break;

   case NT::STATUS_SINGLE_STEP:             // Single-step trap (TF flag)
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_TRAP))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_TRAP;
     inf.code  = PX::TRAP_TRACE;
     break;

   case NT::STATUS_STACK_OVERFLOW:          // Stack guard page hit
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_SEGV))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_SEGV;               
     inf.code  = PX::SEGV_ACCERR;
     inf.addr  = nullptr;  // Unknown faulting address
     break;

   // If this value is zero, the thread attempted to read the inaccessible data. If this value is 1, the thread attempted to write to an inaccessible address.
   // If this value is 8, the thread caused a user-mode data execution prevention (DEP) violation.
   case NT::STATUS_ACCESS_VIOLATION:        // Invalid memory access 
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_SEGV))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_SEGV;
     if(ep->ExceptionRecord->NumberParameters >= 1) {
       uint32 op = ep->ExceptionRecord->ExceptionInformation[0];     // ep->ExceptionRecord->ExceptionInformation[0] == 0 (read), 1 (write), 8 (DEP)
       if(ep->ExceptionRecord->NumberParameters >= 2) inf.addr = (vptr)ep->ExceptionRecord->ExceptionInformation[1];  // Faulting address
       inf.code = (op == 8) ? PX::SEGV_ACCERR : PX::SEGV_MAPERR;        // Can distinguish read vs write but not in POSIX
     } else inf.code = PX::SEGV_MAPERR;
     break;

   // Unaligned memory access, Accessing non-existent physical addresses
   case NT::STATUS_IN_PAGE_ERROR:           // Page fault during I/O (file mapping)   // Memory not present
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_BUS))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_BUS;
     if(ep->ExceptionRecord->NumberParameters >= 1) {
       uint32 op = ep->ExceptionRecord->ExceptionInformation[0];     // ep->ExceptionRecord->ExceptionInformation[0] == 0 (read), 1 (write), 8 (DEP)
       if(ep->ExceptionRecord->NumberParameters >= 2) inf.addr  = (vptr)ep->ExceptionRecord->ExceptionInformation[1];  // Faulting address
       if(ep->ExceptionRecord->NumberParameters >= 3) inf.errno = (uint32)ep->ExceptionRecord->ExceptionInformation[2]; 
       inf.code = (op == 8) ? PX::SEGV_ACCERR : PX::BUS_ADRERR;        // Can distinguish read vs write but not in POSIX
     } else inf.code = PX::BUS_ADRERR;
     break;


   case NT::STATUS_ILLEGAL_INSTRUCTION:      // CPU couldn't decode instruction.
   case NT::STATUS_INVALID_DISPOSITION:      // Invalid exception disposition return
   case NT::STATUS_NONCONTINUABLE_EXCEPTION: // Tried to continue non-continuable exception
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_ILL))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_ILL;
     inf.code  = PX::ILL_ILLOPC;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_PRIVILEGED_INSTRUCTION:  // Attempted privileged instruction
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_ILL))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_ILL;
     inf.code  = PX::ILL_PRVOPC;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_DATATYPE_MISALIGNMENT:   // Alignment fault
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_BUS))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_BUS;              
     inf.code  = PX::BUS_ADRALN;
     inf.addr  = nullptr;  // Unknown faulting address
     break;

   case NT::STATUS_INTEGER_DIVIDE_BY_ZERO:      // Integer math error
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_INTDIV;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_FLOAT_DIVIDE_BY_ZERO:      // Floating point math error
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_FLTDIV;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_INTEGER_OVERFLOW:        // Signed integer overflow (INTO instruction)
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_INTOVF;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_FLOAT_OVERFLOW:          // FP overflow
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_FLTOVF;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_FLOAT_UNDERFLOW:         // FP underflow
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_FLTUND;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_FLOAT_INEXACT_RESULT:    // FP inexact result
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_FLTRES;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_FLOAT_INVALID_OPERATION: // FP invalid operation
   case NT::STATUS_FLOAT_DENORMAL_OPERAND:  // FP denormal operand
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_FLTINV;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_FLOAT_STACK_CHECK:       // x87 stack over/underflow
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_FPE))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_FPE;
     inf.code  = PX::FPE_FLTSUB;
     inf.addr  = ep->ExceptionRecord->ExceptionAddress;
     break;

   case NT::STATUS_GUARD_PAGE_VIOLATION:    // Guard page access (used for stack growth)
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_SEGV))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_SEGV;            
     inf.code  = PX::SEGV_ACCERR;
     inf.addr  = nullptr;  // Unknown faulting address
     break;

   case NT::STATUS_INVALID_HANDLE:          // Invalid handle passed to syscall
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_SYS))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_SYS;
     inf.code  = 1;   // SYS_SECCOMP or generic
     break;

   case 0xC0000420:   // STATUS_ASSERTION_FAILURE/STATUS_ASSERTION_EXCEPTION       // Debug assertion failed
   case 0xC0000602:   // STATUS_FAIL_FAST_EXCEPTION      // Used by __fastfail() and RaiseFailFastException (probably)
     if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_ABRT))return NT::EXCEPTION_CONTINUE_SEARCH; 
     inf.signo = PX::SIG_ABRT;
     inf.code  = 0;
     break;

  case NT::STATUS_CONTROL_C_EXIT:  // 0xC000013A - Ctrl+C pressed
    if(!(AtomicLoad(&fwsinf.SigMask) & PX::SIG_INT))return NT::EXCEPTION_CONTINUE_SEARCH;
    inf.signo = PX::SIG_INT;
    inf.code  = PX::SI_USER;
    break;

   default: return NT::EXCEPTION_CONTINUE_SEARCH;    // We do not handle this
  }

 if(NSIG::SignalActionHandler(inf.signo, &inf, ep)) return NT::EXCEPTION_CONTINUE_EXECUTION;   // Attempting to continue (Assuming that IP was changed or something else has been fixed)
 return NT::EXCEPTION_CONTINUE_SEARCH;  // Continue to fail if SignalActionHandler doesn't terminate the process
}
//------------------------------------------------------------------------------------------------------------
static bool _finline IsSpecConHandle(NT::HANDLE hndl){return !(SConCtx::Flags & SConCtx::flNewWinVer) && SConCtx::IsConHandle(hndl);}
// TODO: IsConsoleHandle  // To skip ANSI sequences for it (optionally)
//------------------------------------------------------------------------------------------------------------
// https://github.com/golang/term/blob/master/term_windows.go
// ENABLE_PROCESSED_INPUT is better to be preserved for password input to keep the ability to Ctrl+C
// On Linux, password input typically uses canonical mode with echo disabled: ECHO flag is OFF, ICANON flag is ON 
// State: 0 - Normal, 1 - Password, > 1 - Raw
static bool SetConsoleState(NT::HANDLE HndlIn, NT::HANDLE HndlOut, uint32 State)          // also called "non-canonical mode"   // Used mostly for passwords    //  in raw mode, Ctrl+C doesn't generate a signal
{
 bool res = false;
 uint32 ModeIn  = 0; 
 uint32 ModeOut = 0;
 if(NCON::SConCtx::pGetConsoleMode)
  {
   NCON::SConCtx::pGetConsoleMode(HndlIn , &ModeIn);
   if(HndlOut)NCON::SConCtx::pGetConsoleMode(HndlOut, &ModeOut);
  }
   else    // The driver
    {
     NCON::SConDrv::GetConsoleMode(HndlIn, &ModeIn); 
     if(HndlOut)NCON::SConDrv::GetConsoleMode(HndlOut, &ModeOut); 
    }
 ModeOut |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_VIRTUAL_TERMINAL_PROCESSING;  // Setting again just to refresh the state
 if(State <= 1)   // Normal/Canonical mode (Linux-like defaults)
  {          
   ModeIn  |= ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT;        // Keep VT input for Linux-like behavior (handles escape sequences)      
   //ModeIn  |= ENABLE_EXTENDED_FLAGS;    // Only when disabling ENABLE_QUICK_EDIT_MODE
   ModeIn  &= ~(ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);     // ENABLE_QUICK_EDIT_MODE - Having apps freeze while selecting text is expected behavior in Windows
   if(State > 0) ModeIn  &= ~ENABLE_ECHO_INPUT;     // Passwords are in normal mode except for echoing disabled
  }
 else  //  Raw mode
  {
   ModeIn  &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);    // ENABLE_INSERT_MODE should not matter here
   ModeIn  |= ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_EXTENDED_FLAGS;
  }
 if(NCON::SConCtx::pSetConsoleMode)
  {
   res = NCON::SConCtx::pSetConsoleMode(HndlIn , ModeIn);
   if(HndlOut)NCON::SConCtx::pSetConsoleMode(HndlOut, ModeOut);
  }
   else    // The driver
    {
     res = !NCON::SConDrv::SetConsoleMode(HndlIn, ModeIn);    // Returns HRESULT
     if(HndlOut)NCON::SConDrv::SetConsoleMode(HndlOut, ModeOut); 
    }
 return res;
}
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
};  // NCON
//============================================================================================================
static int InitConsole(uint32 DefState=0, bool CrtIfMissing=true)
{
 if(NCON::SConCtx::Flags & NCON::SConCtx::flInitialized)return 1;
 NCON::SConCtx::Init();
 if(!NPTM::GetStdOut()) // || !NPTM::GetStdIn())    // NOTE: Those may be redirected (accept that but still try to configure them as a console)                      
  {
   if(!CrtIfMissing)return 0;       // No console and not allowed to create our own
   if(NCON::SConCtx::pAttachConsole)   // Calling AttachConsole, AllocConsole, or FreeConsole will reset the table of control handlers in the client process to its initial state. 
    {
     if(!NCON::SConCtx::pAttachConsole(-1))      // ATTACH_PARENT_PROCESS 
       if(NCON::SConCtx::pAllocConsole) { NCON::SConCtx::pAllocConsole(); AtomicOr(&fwsinf.Flags, sfConOwned); }   // Needed for only for GUI subsystem apps 
    } 
  }
 if(NCON::SConCtx::pSetConsoleCtrlHandler)NCON::SConCtx::pSetConsoleCtrlHandler(nullptr, 0);   // Unmask SIGINT in case we were spawned as a New Group Leader  // Re-enable Ctrl+C which is disabled by CREATE_NEW_PROCESS_GROUP

// UTF-8 code page  // SetConsoleOutputCP: Not full UTF-8 and not always works on old Windows 
 if(NCON::SConCtx::pSetConsoleCP)NCON::SConCtx::pSetConsoleCP(65001);
    else NCON::SConDrv::SetConsoleCP(65001, false);
 if(NCON::SConCtx::pSetConsoleOutputCP)NCON::SConCtx::pSetConsoleOutputCP(65001);
    else NCON::SConDrv::SetConsoleCP(65001, true); 

 NCON::SetConsoleState(NPTM::GetStdIn(), NPTM::GetStdOut(), DefState); 
 AtomicOr(&NCON::SConCtx::Flags, NCON::SConCtx::flInitialized);
 return 1;
}
//------------------------------------------------------------------------------------------------------------
// Supposed to restore some original state 
static int ReleaseConsole(void)     // Useless?
{
 if(!(NCON::SConCtx::Flags & NCON::SConCtx::flInitialized))return 1;
 NCON::SetConsoleState(NPTM::GetStdIn(), NPTM::GetStdOut(), 0);    // Should remember those before InitConsole?
 if(NCON::SConCtx::pSetConsoleCtrlHandler)NCON::SConCtx::pSetConsoleCtrlHandler((vptr)&NCON::ConsoleHandlerProc, 0);  // Remove the handler (if set)
 return 1;
}
//============================================================================================================

