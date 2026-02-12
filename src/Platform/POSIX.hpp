
#pragma once

//============================================================================================================
// https://docs.oracle.com/cd/E19048-01/chorus5/806-6897/auto1/index.html
// https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
// https://man7.org/linux/man-pages/man2/syscalls.2.html
// https://filippo.io/linux-syscall-table/
// https://marcin.juszkiewicz.com.pl/download/tables/syscalls.html
// NOTE!!: https://pkg.go.dev/syscall

// Only most useful POSIX functions will go here for now
// NOTE: We should be able to use these definitions to call X64 functions from X32 code if necessary (Windows only
// TODO: size of int is platform dependant, replace it
// PHS is size of pointer we require (set it default to sizeof(void*) here?))

#define PXERR(err) (-NPTM::PX::err)
#define MMERR(addr) (((size_t)addr & 0xFFF))

template<typename PHT> struct NPOSIX  // For members: alignas(sizeof(PHT))
{
template<typename T, typename H=uint> using MPTR = TSW< (sizeof(PHT)==sizeof(void*)),T*,SPTR<T,H> >::T;   // SPTR is too unstable to be used by default 

// Which Linux?
// Creates: VSLB(7,10>, VSLB(10,30>, VSLB(17,20>, etc.  // Result: N unique template instantiations
/*template<uint vLINUX, uint vBSD_MAC, typename T=int> struct DCV   // Can be used for Kernel too
{
 enum : T {
#if defined(SYS_MACOS) || defined(SYS_BSD)
 V = (T)vBSD_MAC
#else       // Linux and Windows(emulation)
 V = (T)vLINUX
#endif
 };
}; */

 using PVOID    = MPTR<void,   PHT>;    // All of this is to be able to call X64 syscalls from X32 code
 using PCHAR    = MPTR<achar,  PHT>;  //MPTR<pchar,  PHT>;
 using PCVOID   = MPTR<const void,   PHT>;
 using PCCHAR   = MPTR<const achar,  PHT>;  //MPTR<const pchar,  PHT>;
 using PPCHAR   = MPTR<const achar*, PHT>;  //MPTR<const pchar*, PHT>;    // achar**  // Const?
//using HANDLE   = PVOID;
 using SIZE_T   = decltype(TypeToUnsigned<PHT>());  //  MPTR<uint,   PHT>;
 using SSIZE_T  = decltype(TypeToSigned<PHT>());  //  MPTR<sint,   PHT>;

//using LONG     = int32;
//using ULONG    = uint32;
//using PSIZE_T  = SIZE_T*;
//using PULONG   = ULONG*;
//using NTSTATUS = LONG;
 using PSSIZE_T  = MPTR<SSIZE_T, PHT>;
 using PSIZE_T   = MPTR<SIZE_T, PHT>;
 using PUINT32   = MPTR<uint32, PHT>;
 using PUINT64   = MPTR<uint64, PHT>;
 using PINT32    = MPTR<int32, PHT>;
 using PINT64    = MPTR<int64, PHT>;
 using PUINT8    = MPTR<uint8, PHT>;
 using mode_t    = int32;     //uint32;
 using fdsc_t    = SSIZE_T;	 // NOTE: Should be of a pointer size for platform compatibility    // NOTE: Do not place in structs that expect it to be 'int'
 using dev_t     = uint32;    // See makedev macro
 //using off_t     = int64;
 using pid_t     = SSIZE_T;   // Should be of size_t size to contain extra info on x64 if needed  // Actual pid_t is a signed 32-bit integer (int) on both 32-bit and 64-bit systems
 //using fd_t      = int;
 using time_t    = SSIZE_T;   // Old time_t which is 32-bit on x32 platforms
 using time64_t  = sint64;  
 using timeout_t = sint64;    // For timeouts. Positive: Relative wait (0 - no wait). Negative: Absolute time (since epoch). WAIT_INFINITE is max negative value

 SCVR timeout_t WAIT_INFINITE = timeout_t(-1); // Max absolute time bot won't be threated as one anyway

SCVR int EOF    = -1;
SCVR int BadFD  = -1;

static constexpr auto _finline Error(auto val){return -val;} 
static constexpr bool _finline IsBadFD(fdsc_t fd){return fd < 0;}
static constexpr bool _finline IsError(int val){return (bool)val;}   // Anything but NOERROR   // TODO: Fix for ssize

enum EDFD  // These are just for convenience. These descriptors don`t have to be open on every system (Android?)
{
 STDIN,
 STDOUT,
 STDERR,
// TODO: Add support for descriptors for TEMP, NULL and RAND
 FD_NULL,
 FD_RAND,
 FD_TEMP  // A temp file or a temp directory(Relative open, may be virtual(WASM))
};

// NOTE: BSD codes are taken from 'https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/errno.h'
enum EErrs   // Linux / XNU (Darwin/MacOS)
{
 NOERROR         = 0,
 
 // POSIX standard errors (1-34) - identical on all platforms
 EPERM           = 1,                      // Operation not permitted
 ENOENT          = 2,                      // No such file or directory
 ESRCH           = 3,                      // No such process
 EINTR           = 4,                      // Interrupted system call
 EIO             = 5,                      // I/O error
 ENXIO           = 6,                      // No such device or address  ( Device not configured )
 E2BIG           = 7,                      // Argument list too long
 ENOEXEC         = 8,                      // Exec format error
 EBADF           = 9,                      // Bad file number
 ECHILD          = 10,                     // No child processes
 EAGAIN          = VSLB(11, 35),           // Try again / Would block  (Resource deadlock avoided) // BSD: EDEADLK ( 11 was EAGAIN )
 ENOMEM          = 12,                     // Out of memory
 EACCES          = 13,                     // Permission denied
 EFAULT          = 14,                     // Bad address
 ENOTBLK         = 15,                     // Block device required
 EBUSY           = 16,                     // Device or resource busy
 EEXIST          = 17,                     // File exists
 EXDEV           = 18,                     // Cross-device link
 ENODEV          = 19,                     // Operation not supported by device  (No such device)
 ENOTDIR         = 20,                     // Not a directory
 EISDIR          = 21,                     // Is a directory
 EINVAL          = 22,                     // Invalid argument
 ENFILE          = 23,                     // Too many open files in system  (File table overflow)
 EMFILE          = 24,                     // Too many open files
 ENOTTY          = 25,                     // Inappropriate ioctl for device  (Not a typewriter)
 ETXTBSY         = 26,                     // Text file busy
 EFBIG           = 27,                     // File too large
 ENOSPC          = 28,                     // No space left on device
 ESPIPE          = 29,                     // Illegal seek
 EROFS           = 30,                     // Read-only file system
 EMLINK          = 31,                     // Too many links
 EPIPE           = 32,                     // Broken pipe
 // Math software
 EDOM            = 33,                     // Math argument out of domain
 ERANGE          = 34,                     // Math result not representable
 
 // File locking / IPC
 EDEADLK         = VSLB(35, 11),           // Resource deadlock would occur     // EAGAIN, EWOULDBLOCK  ??? 
 ENOLCK          = VSLB(37, 77),           // No record locks available
 EIDRM           = VSLB(43, 90),           // Identifier removed
 ENOMSG          = VSLB(42, 91),           // No message of desired type
 
 // Filesystem
 ENAMETOOLONG    = VSLB(36, 63),           // File name too long
 ENOSYS          = VSLB(38, 78),           // Function not implemented
 ENOTEMPTY       = VSLB(39, 66),           // Directory not empty
 ELOOP           = VSLB(40, 62),           // Too many symbolic links
 ESTALE          = VSLB(116, 70),          // Stale NFS file handle
 EREMOTE         = VSLB(66, 71),           // Object is remote
 
 EWOULDBLOCK     = EAGAIN,                 // Operation would block (same as EAGAIN)
 
 // X.25 protocol (obsolete - never seen in userland)
 ECHRNG          = VSLB(44, -1),           // Channel number out of range
 EL2NSYNC        = VSLB(45, -1),           // Level 2 not synchronized
 EL3HLT          = VSLB(46, -1),           // Level 3 halted
 EL3RST          = VSLB(47, -1),           // Level 3 reset
 ELNRNG          = VSLB(48, -1),           // Link number out of range
 EUNATCH         = VSLB(49, -1),           // Protocol driver not attached
 ENOCSI          = VSLB(50, -1),           // No CSI structure available
 EL2HLT          = VSLB(51, -1),           // Level 2 halted
 
 // STREAMS tape/exchange (obsolete)
 EBADE           = VSLB(52, -1),           // Invalid exchange
 EBADR           = VSLB(53, -1),           // Invalid request descriptor
 EXFULL          = VSLB(54, -1),           // Exchange full
 ENOANO          = VSLB(55, -1),           // No anode
 EBADRQC         = VSLB(56, -1),           // Invalid request code
 EBADSLT         = VSLB(57, -1),           // Invalid slot
 
 EDEADLOCK       = VSLB(EDEADLK, -1),      // File locking deadlock (same as EDEADLK on Linux)   // 58 ???
 
 // Font/display (obsolete)
 EBFONT          = VSLB(59, -1),           // Bad font file format
 
 // STREAMS API (obsolete - removed from Linux)
 ENOSTR          = VSLB(60, 99),           // Device not a stream
 ENODATA         = VSLB(61, 96),           // No data available
 ETIME           = VSLB(62, 101),          // Timer expired   // macOS: 92 (not 101)
 ENOSR           = VSLB(63, 98),           // Out of streams resources
 ESTRPIPE        = VSLB(86, -1),           // Streams pipe error
 
 // Ancient networking (obsolete)
 ENONET          = VSLB(64, -1),           // Machine is not on the network
 ENOPKG          = VSLB(65, -1),           // Package not installed
 EADV            = VSLB(68, -1),           // Advertise error  // RFS - obsolete
 ESRMNT          = VSLB(69, -1),           // Srmount error   // RFS - obsolete
 ECOMM           = VSLB(70, -1),           // Communication error on send
 EDOTDOT         = VSLB(73, -1),           // RFS specific error  // RFS - obsolete
 EREMCHG         = VSLB(78, -1),           // Remote address changed   // RFS - obsolete
 
 // Message passing / protocols
 ENOLINK         = VSLB(67, 97),           // Link has been severed
 EPROTO          = VSLB(71, 100),          // Protocol error
 EMULTIHOP       = VSLB(72, 95),           // Multihop attempted
 EBADMSG         = VSLB(74, 94),           // Not a data message
 
 // Data overflow
 EOVERFLOW       = VSLB(75, 84),           // Value too large for defined data type
 
 // Network naming (obsolete/rare)
 ENOTUNIQ        = VSLB(76, -1),           // Name not unique on network
 
 // File descriptor state (rare but real)
 EBADFD          = VSLB(77, -1),           // File descriptor in bad state
 
 // Shared library loading (rare)
 ELIBACC         = VSLB(79, -1),           // Can not access a needed shared library
 ELIBBAD         = VSLB(80, -1),           // Accessing a corrupted shared library
 ELIBSCN         = VSLB(81, -1),           // .lib section in a.out corrupted
 ELIBMAX         = VSLB(82, -1),           // Attempting to link in too many shared libraries
 ELIBEXEC        = VSLB(83, -1),           // Cannot exec a shared library directly
 
 // Character encoding
 EILSEQ          = VSLB(84, 92),           // Illegal byte sequence   // macOS: 92 (Illegal byte sequence)
 
 // Kernel internal (never reaches userland)
 ERESTART        = VSLB(85, -1),           // Interrupted system call should be restarted
 
 // Quotas / limits
 EUSERS          = VSLB(87, 68),           // Too many users
  
 // Network errors
 ENETDOWN        = VSLB(100, 50),          // Network is down
 ENETUNREACH     = VSLB(101, 51),          // Network is unreachable
 ENETRESET       = VSLB(102, 52),          // Network dropped connection because of reset
 ECONNABORTED    = VSLB(103, 53),          // Software caused connection abort
 ECONNRESET      = VSLB(104, 54),          // Connection reset by peer
 ENOBUFS         = VSLB(105, 55),          // No buffer space available
 EISCONN         = VSLB(106, 56),          // Transport endpoint is already connected
 ENOTCONN        = VSLB(107, 57),          // Transport endpoint is not connected
 ESHUTDOWN       = VSLB(108, 58),          // Cannot send after transport endpoint shutdown
 ETOOMANYREFS    = VSLB(109, 59),          // Too many references: cannot splice
 ETIMEDOUT       = VSLB(110, 60),          // Connection timed out
 ECONNREFUSED    = VSLB(111, 61),          // Connection refused
 EHOSTDOWN       = VSLB(112, 64),          // Host is down
 EHOSTUNREACH    = VSLB(113, 65),          // No route to host
 EALREADY        = VSLB(114, 37),          // Operation already in progress
 EINPROGRESS     = VSLB(115, 36),          // Operation now in progress
 
 // Filesystem corruption (actually used)
 EUCLEAN         = VSLB(117, -1),          // Structure needs cleaning (ext2/3/4, XFS)
 
 // XENIX compatibility (obsolete)
 ENOTNAM         = VSLB(118, -1),          // Not a XENIX named type file
 ENAVAIL         = VSLB(119, -1),          // No XENIX semaphores available
 EISNAM          = VSLB(120, -1),          // Is a named type file
 
 // Remote I/O (actually used - NFS, FUSE)
 EREMOTEIO       = VSLB(121, -1),
 
 // macOS-specific additional errors (beyond what Linux has)
 ENOTSUP         = VSLB(-1, 45),           // Operation not supported (macOS differs from EOPNOTSUPP)
 EPROCLIM        = VSLB(-1, 67),           // Too many processes
 EBADRPC         = VSLB(-1, 72),           // RPC struct is bad
 ERPCMISMATCH    = VSLB(-1, 73),           // RPC version wrong
 EPROGUNAVAIL    = VSLB(-1, 74),           // RPC prog. not avail
 EPROGMISMATCH   = VSLB(-1, 75),           // Program version wrong
 EPROCUNAVAIL    = VSLB(-1, 76),           // Bad procedure for program
 EFTYPE          = VSLB(-1, 79),           // Inappropriate file type or format
 EAUTH           = VSLB(-1, 80),           // Authentication error
 ENEEDAUTH       = VSLB(-1, 81),           // Need authenticator
 // Intelligent device errors
 EPWROFF         = VSLB(-1, 82),           // Device power is off
 EDEVERR         = VSLB(-1, 83),           // Device error
 // Program loading errors 
 EBADEXEC        = VSLB(-1, 85),           // Bad executable
 EBADARCH        = VSLB(-1, 86),           // Bad CPU type in executable
 ESHLIBVERS      = VSLB(-1, 87),           // Shared library version mismatch
 EBADMACHO       = VSLB(-1, 88),           // Malformed Mach-o file

 ECANCELED       = VSLB(125, 89),          // Operation canceled
 EOWNERDEAD      = VSLB(130, 105),         // Owner died (robust mutexes)
 ENOTRECOVERABLE = VSLB(131, 104),         // State not recoverable (robust mutexes)
 EQFULL          = VSLB(-1, 106),          // Interface output queue is full (macOS)

 // Socket errors - Berkeley sockets (all platforms)
 ENOTSOCK        = VSLB(88, 38),           // Socket operation on non-socket
 EDESTADDRREQ    = VSLB(89, 39),           // Destination address required
 EMSGSIZE        = VSLB(90, 40),           // Message too long
 EPROTOTYPE      = VSLB(91, 41),           // Protocol wrong type for socket
 ENOPROTOOPT     = VSLB(92, 42),           // Protocol not available
 EPROTONOSUPPORT = VSLB(93, 43),           // Protocol not supported
 ESOCKTNOSUPPORT = VSLB(94, 44),           // Socket type not supported
 EOPNOTSUPP      = VSLB(95, ENOTSUP),      // Operation not supported on transport endpoint  // macOS: 102 (not 45!)
 EPFNOSUPPORT    = VSLB(96, 46),           // Protocol family not supported
 EAFNOSUPPORT    = VSLB(97, 47),           // Address family not supported by protocol
 EADDRINUSE      = VSLB(98, 48),           // Address already in use
 EADDRNOTAVAIL   = VSLB(99, 49),           // Cannot assign requested address
};

// https://android.googlesource.com/platform/bionic/+/master/libc/include/bits/signal_types.h
// Signal 0 has special meaning: kill(pid, 0) tests if a process exists without sending a signal. 
enum ESignal 
{
 SIG_HUP    = 1,                     // terminate  // terminal hangup / config reload / daemon restart
 SIG_INT    = 2,                     // terminate  // Ctrl+C
 SIG_QUIT   = 3,                     // coredump   // Ctrl+\ (usually generates core)
 SIG_ILL    = 4,                     // coredump   // illegal instruction
 SIG_TRAP   = 5,                     // coredump   // trace trap (not reset when caught)
 SIG_ABRT   = 6,                     // coredump   // abort()
 SIG_BUS    = VSLB(7,  10),          // coredump   // alignment / unmapped access (more common on BSD/macOS) // BSD: SIGBUS happens FAR more than Linux (alignment faults, mmap edge cases)
 SIG_FPE    = 8,                     // coredump   // divide-by-zero etc
 SIG_KILL   = 9,                     // terminate  // kill (cannot be caught or ignored) 
 SIG_USR1   = VSLB(10, 30),          // terminate  // a free custom interrupt
 SIG_SEGV   = 11,                    // coredump   // invalid memory access
 SIG_USR2   = VSLB(12, 31),          // terminate  // a free custom interrupt
 SIG_PIPE   = 13,                    // terminate  // write to closed pipe/socket (macOS/BSD this bites hard if unhandled)  // Most frameworks just: signal(SIGPIPE, SIG_IGN);
 SIG_ALRM   = 14,                    // terminate  // alarm()
 SIG_TERM   = 15,                    // terminate  // Polite shutdown request (your primary 'please exit' signal)
 SIG_STKFLT = VSLB(16, -1),          // terminate  // Linux-only (0 = invalid on BSD)
 SIG_CHLD   = VSLB(17, 20),          // ignore     // child exited / stopped / continued
 SIG_CONT   = VSLB(18, 19),          // ignore     // continue a stopped process
 SIG_STOP   = VSLB(19, 17),          // stop       // cannot be caught
 SIG_TSTP   = VSLB(20, 18),          // stop       // If the framework runs daemons or services: ignore all three. Otherwise background writes to stdout can suspend your process.
 SIG_TTIN   = 21,                    // stop       // Terminal I/O signals (BSD/macOS have more of these, Linux only has 2)
 SIG_TTOU   = 22,                    // stop       // Terminal I/O signals (BSD/macOS have more of these, Linux only has 2)
 SIG_URG    = VSLB(23, 16),          // ignore     // Urgent I/O (4.2BSD)  // Linux only ? 
 SIG_XCPU   = 24,                    // coredump   // CPU limit exceeded. Mostly from ulimit. - Ignore
 SIG_XFSZ   = 25,                    // coredump   // File size limit exceeded. Much better to ignore and handle write() failure.
 SIG_VTALRM = 26,                    // terminate  // virtual time alarm
 SIG_PROF   = 27,                    // terminate  // profiling time alarm
 SIG_WINCH  = 28,                    // ignore     // window size changes ( It is a signal sent to a process when its controlling terminal changes its size.)
 SIG_IO     = VSLB(29, 23),          // terminate  // input/output possible signal  // Different! (SIGIO/SIGPOLL)  // macOS doesn’t implement realtime signals at all.
 SIG_POLL   = SIG_IO,                // terminate  
 SIG_PWR    = VSLB(30, -1),          // terminate  // Linux-only
 SIG_SYS    = VSLB(31, 12),          // SIGUNUSED 
 SIG_EMT    = VSLB(-1, 7),           // coredump   // BSD-only (use -1 for "doesn't exist" on Linux)
};

// Signal codes (Values for si_code)
enum ESigCode 
{
 // Generic codes
 SI_USER    = VSLB(0,  0x10001),     // Sent via kill, sigsend, or raise.
 SI_QUEUE   = VSLB(-1, 0x10002),     // Sent via sigqueue.
 SI_TIMER   = VSLB(-2, 0x10003),     // Timer expiration (POSIX timers).
 SI_MESGQ	= VSLB(-3, 0x10005),     // sent by real time mesq state change
 SI_ASYNCIO = VSLB(-4, 0x10004),     // sent by AIO completion 
 SI_SIGIO	= VSLB(-5, 0),           // Linux only sent by queued SIGIO 
 SI_TKILL   = VSLB(-6, 0),           // Linux only Sent via tkill or tgkill (Linux specific)   
 SI_KERNEL  = VSLB(0x80, 0x10006),   // XNU/Darwin doesn't have this
 
 // SIGILL codes
 ILL_NOOP   = 0,                // if only I knew... 
 ILL_ILLOPC = 1,                // illegal opcode 
 ILL_ILLOPN = 2,                // illegal trap
 ILL_ILLADR = 3,                // privileged opcode
 ILL_ILLTRP = 4,                // illegal operand 
 ILL_PRVOPC = 5,                // illegal addressing mode 
 ILL_PRVREG = 6,                // privileged register  
 ILL_COPROC = 7,                // coprocessor error 
 ILL_BADSTK = 8,                // internal stack error 
                                
 // SIGFPE codes                
 FPE_NOOP   = 0,                // if only I knew...
 FPE_INTDIV = 1,                // floating point divide by zero
 FPE_INTOVF = 2,                // floating point overflow
 FPE_FLTDIV = 3,                // floating point underflow
 FPE_FLTOVF = 4,                // floating point inexact result
 FPE_FLTUND = 5,                // invalid floating point operation
 FPE_FLTRES = 6,                // subscript out of range
 FPE_FLTINV = 7,                // integer divide by zero
 FPE_FLTSUB = 8,                // integer overflow 
                                
 // SIGSEGV codes               
 SEGV_NOOP   = 0,               // if only I knew... 
 SEGV_MAPERR = 1,               // address not mapped to object
 SEGV_ACCERR = 2,               // invalid permission for mapped object
 SEGV_BNDERR = VSLB(3, 0),      // failed address bound checks    // Linux only
 SEGV_PKUERR = VSLB(4, 0),      // failed protection key checks   // Linux only
 
 // SIGBUS codes
 BUS_NOOP      = 0,             // if only I knew... 
 BUS_ADRALN    = 1,             // Invalid address alignment
 BUS_ADRERR    = 2,             // Nonexistent physical address
 BUS_OBJERR    = 3,             // Object-specific HW error 
 BUS_MCEERR_AR = VSLB(4, 0),
 BUS_MCEERR_AO = VSLB(5, 0),
 
 // SIGTRAP codes
 TRAP_BRKPT  = 1,               // Process breakpoint 
 TRAP_TRACE  = 2,               // Process trace trap
 TRAP_BRANCH = VSLB(3, 0),      // codes 3-4 might exist on some BSD variants
 TRAP_HWBKPT = VSLB(4, 0),
 
 // SIGCHLD codes
 CLD_NOOP      = 0,             // if only I knew...
 CLD_EXITED    = 1,             // Child has exited 
 CLD_KILLED    = 2,             // Child was killed, no core file 
 CLD_DUMPED    = 3,             // Child terminated abnormally, core file 
 CLD_TRAPPED   = 4,             // Traced child has trapped  
 CLD_STOPPED   = 5,             // Child has stopped
 CLD_CONTINUED = 6,             // Stopped child has continued

 // Codes for SIGPOLL (BSD/XNU?)
 POLL_IN       = 1,             // Data input available
 POLL_OUT      = 2,             // Output buffers available 
 POLL_MSG      = 3,             // Input message available 
 POLL_ERR      = 4,             // I/O error 
 POLL_PRI      = 5,             // High priority input available 
 POLL_HUP      = 6,             // Device disconnected 
};

// =========================================== FILE/DIRECTORY ===========================================
// http://cqctworld.org/src/l1/lib/linux-x86-enum.names
// https://man7.org/linux/man-pages/man2/open.2.html
// https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2uapi_2asm-generic_2fcntl_8h.html
// https://opensource.apple.com/source/xnu/xnu-344/bsd/sys/fcntl.h
// https://github.com/dlang/druntime/blob/master/src/core/sys/posix/fcntl.d

// MacOS: xnu-2422.1.72\bsd\dev\dtrace\scripts\io.d

// Extended/sys-unsupported (for the Framework) flags are put in the high byte
enum EOpnFlg    // For 'flags' field    // Platform/Arch dependant?
{               //      LINUX       BSD/MacOS
  O_ACCMODE   = 0x00000003,    // Mask to test one of access modes: if((mode & O_ACCMODE) == O_RDONLY)
  O_RDONLY    = 0x00000000,    // Open for reading only.
  O_WRONLY    = 0x00000001,    // Open for writing only.
  O_RDWR      = 0x00000002,    // Open for reading and writing.

//O_SHLOCK    = 0x00000010,    // BSD/MacOS ???
//O_EXLOCK    = 0x00000020,    // BSD/MacOS ???
  O_ASYNC     = VSLB( 0         , 0x00000040 ),    // Sends SIGIO or SIGPOLL
  O_SYMLINK   = VSLB( 0x80000000, 0x00200000 ),    // WinNT+: BSD/MacOS: allow open of symlinks: if the target file passed to open() is a symbolic link then the open() will be for the symbolic link itself, not what it links to.

  O_CREAT     = VSLB( 0x00000040, 0x00000200 ),    // If the file exists, this flag has no effect. Otherwise, the owner ID of the file is set to the user ID of the c_actor, the group ID of the file is set to the group ID of the c_actor, and the low-order 12 bits of the file mode are set to the value of mode.
  O_EXCL      = VSLB( 0x00000080, 0x00000800 ),    // Ensure that this call creates the file. If O_EXCL and O_CREAT are set, open will fail if the file exists. In general, the behavior of O_EXCL is undefined if it is used without O_CREAT

  O_NOCTTY    = VSLB( 0x00000100, 0          ),    // If pathname refers to a terminal device - see tty(4) - it will not become the process's controlling terminal even if the process does not have one. // On GNU/Hurd systems and 4.4 BSD, opening a file never makes it the controlling terminal and O_NOCTTY is zero. 
  O_TRUNC     = VSLB( 0x00000200, 0x00000400 ),    // If the file exists, its length is truncated to 0 and the mode and owner are unchanged.
  O_APPEND    = VSLB( 0x00000400, 0x00000008 ),    // If set, the file pointer will be set to the end of the file prior to each write.
  O_NONBLOCK  = VSLB( 0x00000800, 0x00000004 ),    // If O_NONBLOCK is set, the open will return without waiting for the device to be ready or available. Subsequent behavior of the device is device-specific.
 // Additional:
  O_DSYNC     = VSLB( 0x00001000, 0          ),
  O_DIRECT    = VSLB( VSIA(0x00010000,0x00004000), 0          ),    // direct disk access hint - currently ignored
  O_LARGEFILE = VSLB( VSIA(0x00020000,0x00008000), 0          ),
  O_DIRECTORY = VSLB( VSIA(0x00004000,0x00010000), 0x00100000 ),    // must be a directory  // This flag is unreliable: X86=0x00010000, ARM=0x00004000 (Probably)    // NOTE: Required to open directories on Windows
  O_NOFOLLOW  = VSLB( VSIA(0x00008000,0x00020000), 0x00000100 ),    // don't follow links: if the target file passed to open() is a symbolic link then the open() will fail
  O_NOATIME   = VSLB( 0x00040000, 0          ),
  O_CLOEXEC   = VSLB( 0x00080000, 0x01000000 ),    // set close_on_exec    // DARWIN LEVEL >= 200809
  O_PATH      = VSLB( 0x00200000, 0          ),
  O_TMPFILE   = VSLB( 0x00410000, 0          ),    // Create an unnamed temporary regular file. The pathname argument specifies a directory; an unnamed inode will be created in that directory's filesystem.
  O_SYNC      = VSLB( 0x00101000, 0x00000080 ),    // By the time write(2) (or similar) returns, the output data and associated file metadata have been transferred to the underlying hardware (i.e., as though each write(2) was followed by a call to fsync(2))
};

// https://github.com/torvalds/linux/blob/master/include/uapi/linux/stat.h
enum ENode     // for mknod
{
 S_IFMT   = 0x0000F000, // 00170000
 S_IFSOCK = 0x0000C000, // 0140000
 S_IFLNK  = 0x0000A000, // 0120000    // Link
 S_IFREG  = 0x00008000, // 0100000    // Regular file
 S_IFBLK  = 0x00006000, // 0060000
 S_IFDIR  = 0x00004000, // 0040000    // Directory
 S_IFCHR  = 0x00002000, // 0020000
 S_IFIFO  = 0x00001000, // 0010000
// S_ISUID  = 0x00000800, // 0004000
// S_ISGID  = 0x00000400, // 0002000
// S_ISVTX  = 0x00000200, // 0001000

//#define S_ISLNK(m)    (((m) & S_IFMT) == S_IFLNK)
//#define S_ISREG(m)    (((m) & S_IFMT) == S_IFREG)
//#define S_ISDIR(m)    (((m) & S_IFMT) == S_IFDIR)
//#define S_ISCHR(m)    (((m) & S_IFMT) == S_IFCHR)
//#define S_ISBLK(m)    (((m) & S_IFMT) == S_IFBLK)
//#define S_ISFIFO(m)   (((m) & S_IFMT) == S_IFIFO)
//#define S_ISSOCK(m)   (((m) & S_IFMT) == S_IFSOCK)
};

enum EMode
{
 S_IXOTH = 0x00000001, // 00001 // others have execute permission
 S_IWOTH = 0x00000002, // 00002 // others have write permission
 S_IROTH = 0x00000004, // 00004 // others have read permission
 S_IRWXO = 0x00000007, // 00007 // others have read, write, and execute permission

 S_IXGRP = 0x00000008, // 00010 // group has execute permission
 S_IWGRP = 0x00000010, // 00020 // group has write permission
 S_IRGRP = 0x00000020, // 00040 // group has read permission
 S_IRWXG = 0x00000038, // 00070 // group has read, write, and execute permission
 S_IXUSR = 0x00000040, // 00100 // user has execute permission
 S_IWUSR = 0x00000080, // 00200 // user has write permission
 S_IRUSR = 0x00000100, // 00400 // user has read permission
 S_IRWXU = 0x000001C0, // 00700 // user (file owner) has read, write, and execute permission

// According to POSIX, the effect when other bits are set in mode is unspecified.  On Linux, the following bits are also honored in mode:
 S_ISVTX = 0x00000200, // 01000 // sticky bit (see inode(7)).
 S_ISGID = 0x00000400, // 02000 // set-group-ID bit (see inode(7)).
 S_ISUID = 0x00000800, // 04000 // set-user-ID bit
};

// The path parameter points to a path name naming a file. The open function opens a file descriptor for the named file and sets the file status flags according to the value of oflag.
static fdsc_t PXCALL open(PCCHAR pathname, int flags, mode_t mode);  // 'open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode)' is same as call to 'creat'

// Equivalent to: open(path, O_WRONLY|O_CREAT|O_TRUNC, mode)
//static int PXCALL creat(PCCHAR path, mode_t mode);    // Use it as fallback if a correct O_CREAT cannot be found

// The fildes field contains a file descriptor obtained from an open(2POSIX), dup(2POSIX), accept(2POSIX), socket(2POSIX), or shm_open(2POSIX) system call. The close function closes the file descriptor indicated by fildes.
static int PXCALL close(fdsc_t fd);


struct SIOV
{
 PVOID  base;   // base address
 SIZE_T size;   // length
};

using PIOVec = MPTR<SIOV,   PHT>;

// Attempts to read nbytes of data from the object referenced by the descriptor d into the buffer pointed to by buf . readv() performs the same action, but scatters the input data into the iovcnt buffers specified by the members of the iov array: iov[0] , iov[1] , ..., iov[iovcnt-1] .
// Upon successful completion, read() and readv() return the number of bytes actually read and placed in the buffer. The system guarantees to read the number of bytes requested if the descriptor references a normal file that contains that many bytes before the end-of-file, but in no other case. Upon end-of-file, 0 is returned. read() and readv() also return 0 if a non-blocking read is attempted on a socket that is no longer open. Otherwise, -1 is returned, and the global variable errno is set to indicate the error.
// return 0 is when the size argument was 0 or end-of-file has been reached
// for pipes, end-of-file means the writing end of the pipe has been closed
static SSIZE_T PXCALL read(fdsc_t fd, PVOID buf, SIZE_T nbytes);
static SSIZE_T PXCALL readv(fdsc_t fd, PIOVec iov, int iovcnt);

// Attempts to write nbytes of data to the object referenced by the descriptor d from the buffer pointed to by buf . writev() performs the same action, but gathers the output data from the iovcnt buffers specified by the members of the iov array: iov[0] , iov[1] , ..., iov[iovcnt-1] .
// Upon successful completion, write() and writev() return the number of bytes actually written. Otherwise, they return -1 and set errno to indicate the error.
static SSIZE_T PXCALL write(fdsc_t fd, PCVOID buf, SIZE_T nbytes);
static SSIZE_T PXCALL writev(fdsc_t fd, PIOVec iov, int iovcnt);     // Windows: WriteFileGather

enum ESeek
{
 SEEK_SET  = 0,       // Seek relative to begining of file
 SEEK_CUR  = 1,       // Seek relative to current file position
 SEEK_END  = 2,       // Seek relative to end of file

 // Linux 3.1: SEEK_DATA, SEEK_HOLE
};

// Repositions the file offset of the open file description associated with the file descriptor fd to the argument offset according to the directive whence
// The offset can be negative
// Negative return values are error codes
static SSIZE_T PXCALL lseek(fdsc_t fd, SSIZE_T offset, ESeek whence);   // This definition is not good for X32, use INT64 (lseekGD) declaration and llseek wrapper on X32

static sint64 PXCALL lseekGD(fdsc_t fd, sint64 offset, ESeek whence);   // Generic definition, wraps lseek(on x64) and llseek(on x32)

// x32 only(Not present on x64)!
static int PXCALL llseek(fdsc_t fd, uint32 offset_high, uint32 offset_low, PINT64 result, ESeek whence);

// Truncate/enlarge a file (On Linux extended bytes read as 0)
// 64bit offsets since Linuc 2.4
static int PXCALL ftruncate(fdsc_t fd, int64 length);	  // ftruncate64 on x32	systems

static int PXCALL truncate(const achar* path, int64 length);   // truncate64 on x32	systems	  // NOTE: There is no 'At' variant so better to avoid it for consistency

// Attempts to create a directory named pathname.
static int PXCALL mkdir(PCCHAR pathname, mode_t mode);

// Deletes a directory, which must be empty
static int PXCALL rmdir(PCCHAR pathname);

// Renames a file, moving it between directories if required.  Any other hard links to the file (as created using link(2)) are unaffected.  Open file descriptors for oldpath are also unaffected.
static int PXCALL rename(PCCHAR oldpath, PCCHAR newpath);

// Places the contents of the SYMBOLIC link pathname in the buffer buf, which has size bufsiz.  readlink() does not append a terminating null byte to buf.  It will (silently) truncate the contents (to a length of bufsiz characters), in case the buffer is too small to hold all of the contents.
// On success return the number of bytes placed in buf. (If the returned value equals bufsiz, then truncation may have occurred.)
static SSIZE_T PXCALL readlink(PCCHAR pathname, PCHAR buf, SIZE_T bufsize);

// Deletes a name from the filesystem.  If that name was the last link to a file and no processes have the file open, the file is deleted and the space it was using is made available for reuse.
// If the name was the last link to a file but any processes still have the file open, the file will remain in existence until the last file descriptor referring to it is closed.
// If the name referred to a SYMBOLIC link, the link is removed.
// If the name referred to a socket, FIFO, or device, the name for it is removed but processes which have the object open may continue to use it.
static int PXCALL unlink(PCCHAR pathname);

// creates a new link (also known as a hard link (INODE on current FS)) to an existing file.
// File`s Hard links all refer to same data (identified by INODE) so any of them can be moved freely within the FS
// The file itself exist while at least one Hard Link to its INODE exists and there is open descriptors to it
// since kernel 2.0, Linux does not do so: if oldpath is a symbolic link, then newpath is created as a (hard) link to the same symbolic link file (i.e., newpath becomes a symbolic link to the same file that oldpath refers to).
static int PXCALL link(PCCHAR oldpath, PCCHAR newpath);

// creates a SYMBOLIC link named linkpath which contains the string target.
// Symbolic links are interpreted at run time as if the contents of the link had been substituted into the path being followed to find a file or directory.
// Symbolic links may contain ..  path components, which (if used at the start of the link) refer to the parent directories of that in which the link resides.
// A symbolic link (also known as a soft link) may point to an existing file or to a nonexistent one; the latter case is known as a dangling link.
static int PXCALL symlink(PCCHAR target, PCCHAR linkpath);

enum EAcss
{
 F_OK   =   0,      // test for existence of file
 X_OK   =   0x01,   // test for execute or search permission
 W_OK   =   0x02,   // test for write permission
 R_OK   =   0x04,   // test for read permission
};

// Checks whether the calling process can access the file pathname.  If pathname is a symbolic link, it is dereferenced.
static int PXCALL access(PCCHAR pathname, int mode);

// Creates a filesystem node (file, device special file, or named pipe) named pathname, with attributes specified by mode and dev.
static int PXCALL mknod(PCCHAR pathname, mode_t mode, dev_t dev);

// Create a named pipe
static int PXCALL mkfifo(PCCHAR pathname, mode_t mode);  // Not a syscall on linux and BSD, is just a wrapper for mknod

// Create an unnamed pipe
// pipefd[0] refers to the read end of the pipe.  pipefd[1] refers to the write end of the pipe.
// NOTE: reading the read end of a pipe will return EOF when and only when all copies of the write end of the pipe are closed.
static int PXCALL pipe(PINT32 fds);
static int PXCALL pipe2(PINT32 pipefd, int flags);   // int pipefd[2]    // Is Linux-specific? On BSD since V10 (2014)

// Uses the lowest-numbered unused descriptor for the new descriptor
static int PXCALL dup(fdsc_t oldfd);

// Makes newfd be the copy of oldfd, closing newfd first if necessary
// If oldfd is not a valid file descriptor, then the call fails, and newfd is not closed
// If oldfd is a valid file descriptor, and newfd has the same value as oldfd, then dup2() does nothing, and returns newfd.
static int PXCALL dup3(fdsc_t oldfd, fdsc_t newfd, int flags);

// The getcwd() function copies an absolute pathname of the current working directory to the array pointed to by buf, which is of length size.
// On many architectures, PATH_MAX and the system page size are both 4096 bytes, but a few architectures have a larger page size.)  
// If the length of the pathname of the current working directory exceeds this limit, then the system call fails with the error ENAMETOOLONG.
// the pathname returned by the getcwd() system call will be prefixed with the string "(unreachable)" if the current directory is not below the 
// root directory of the current process (e.g., because the process set a new filesystem root using chroot(2) without changing its current directory into the new root).
// Do not expect terminating '/' at the end of returned path
static int PXCALL getcwd(PCHAR buf, size_t size);  // Returns number of chars in full CWD path (including term 0)    // ERANGE if buffer is too small

// Changes the current working directory of the calling process to the directory specified in path
static int PXCALL chdir(PCCHAR path);
static int PXCALL fchdir(fdsc_t fd);

enum EFcntl
{
 F_DUPFD         = 0,        // dup
 F_GETFD         = 1,        // get close_on_exec
 F_SETFD         = 2,        // set/clear close_on_exec
 F_GETFL         = 3,        // get file->f_flags
 F_SETFL         = 4,        // set file->f_flags

 F_GETLK         = 5,
 F_SETLK         = 6,
 F_SETLKW        = 7,

 F_SETOWN        = 8,        // for sockets
 F_GETOWN        = 9,        // for sockets

 F_SETSIG        = 10,       // for sockets
 F_GETSIG        = 11,       // for sockets

 F_NOTIFY          = 0x402,    // (F_LINUX_SPECIFIC_BASE + 2)
 F_DUPFD_CLOEXEC   = 0x406     // (F_LINUX_SPECIFIC_BASE + 6),  // F_LINUX_SPECIFIC_BASE = 1024,
};

enum EDNotify  // Types of directory notifications that may be requested with fcntl
{
 DN_ACCESS     = 0x00000001,   // File accessed
 DN_MODIFY     = 0x00000002,   // File modified
 DN_CREATE     = 0x00000004,   // File created
 DN_DELETE     = 0x00000008,   // File removed
 DN_RENAME     = 0x00000010,   // File renamed
 DN_ATTRIB     = 0x00000020,   // File changed attibutes
 DN_MULTISHOT  = 0x80000000    // Don't remove notifier
};

static int PXCALL fcntl(fdsc_t fd, uint32 cmd, PVOID ArgPtr, PVOID ArgLen);  // for a directory change notification (lagacy, Linux only) : https://linux.die.net/man/7/inotify

static int PXCALL ioctl(fdsc_t fd, uint32 cmd, PVOID ArgPtr, PVOID ArgLen);	 //	ArgLen is added for consistency (Especially useful on Windows)	// Size of size fields is supposed to be stored in the code value itself, but assume it to be uint32 (which is size_t on x32)

enum EFDLock
{
 LOCK_SH    = 1,    // Shared lock
 LOCK_EX    = 2,    // Exclusive lock
 LOCK_NB    = 4,    // Or'd with one of the above to prevent blocking
 LOCK_UN    = 8  // Remove lock
};

static int PXCALL fsync(fdsc_t fd);

static int PXCALL fdatasync(fdsc_t fd);

static int PXCALL flock(fdsc_t fd, int operation);


// https://en.cppreference.com/w/c/chrono/timespec
template<typename T, T FracPerSec> struct STime
{
 SCVR T FPS = FracPerSec;
 T sec;   // seconds
 T frac;  // fractional part (0 to FRAC_PER_SEC-1)

 _minline STime<T,FracPerSec>& operator= (const auto& tm){this->sec = (T)tm.sec; this->frac = (T)tm.frac; return *this;}
};

template<typename T = time_t> using STVal  = STime<T, 1000000>;     // microseconds    // suseconds_t (long)          // gettimeofday
template<typename T = time_t> using STSpec = STime<T, 1000000000>;  // nanoseconds     // valid values are [0, 999999999]   // long (long long on some platforms?)    // nanosleep, fstat (SFStat)

using timespec = STSpec<time_t>;       // time_t is 32-bit on x32!
using timeval  = STVal<time_t>;
using PTiSp    = MPTR<timespec,   PHT>;
using PTiVl    = MPTR<timeval,   PHT>;

enum ETimeUnits: time64_t
{
 tuSeconds      = 1,
 tuMilliseconds = 1000,
 tuMicroseconds = 1000000,
 tuNanoseconds  = 1000000000
};

struct timezone
{
 sint32 utcoffs;     // minutes west of Greenwich  // Seconds now (to avoid multiplication when modifying UTC time in seconds)
 sint32 dsttime;     // type of DST correction     // Unused
};

static int PXCALL gettimeofday(timeval* tv, timezone* tz);   // Returns 0 in timezone on Linux
static int PXCALL settimeofday(timeval* tv, timezone* tz);


enum EPollFlags
{
// Event types that can be polled for. These bits may be set in `events' to indicate the interesting event types; they will appear in `revents' to indicate the status of the file descriptor. 
 POLLIN   = 0x001,      // There is data to read.  
 POLLPRI  = 0x002,      // There is urgent data to read.
 POLLOUT  = 0x004,      // Writing now will not block. 

// Event types always implicitly polled for. These bits need not be set in `events', but they will appear in `revents' to indicate the status of the file descriptor.
 POLLERR  = 0x008,      // Error condition.  
 POLLHUP  = 0x010,      // Hung up.  
 POLLNVAL = 0x020,      // Invalid polling request. 
};

// If none of the events requested (and no error) has occurred for any of the file descriptors, then poll() blocks until one of the events occurs
// POLLHUP and POLLOUT are mutually exclusive and should never be present in the revents bitmask at the same time
// POLLNVAL is equivalent to EBADF: it means the file descriptor does not actually refer to any open file
// POLLNVAL will trigger if I close a file descriptor, then try reading from the closed fd.
// POLLNVAL is only set when the timeout expires, closing the socket doesn't seem to make the poll() return.
// It is probably unwise to close file descriptors while they may be in use by system calls in other threads in the same process. Since a file descriptor may be reused, there are some obscure race conditions that may cause unintended side effects.
struct pollfd
{
 int   fd;         // file descriptor    // Set to negative to ignore the record 
 short events;     // requested events   // No need to request for POLLHUP
 short revents;    // returned events    // If 0, return only POLLHUP, POLLERR, and POLLNVAL
};

struct fd_set      // Each bit represents a triggered file descriptor. Total bits is max file descriptors range which is passed in nfds
{
 static const int BitsPerElem = (sizeof(size_t)*8);
 size_t fds_bits[1024 / BitsPerElem];  // long x64=64,x32=32 // FD_SET // Max possible file descriptors is 4096(Hard limit) and max 1024 per process (Soft limit)
};
// typedef size_t fd_set[1024 / sizeof(size_t)];

struct sigset_safe_t    // Old was single uint32   // XNU: always uint32 
{
 uint32 Mask[4];
};

using PSigSet = MPTR<sigset_safe_t,   PHT>;

// A return value of zero indicates that the system call timed out before any file descriptors became ready. Negative is an error code.
// On some other UNIX systems, poll() can fail with the error EAGAIN if the system fails to allocate kernel-internal resources, rather than ENOMEM as Linux does. 
// POSIX permits this behavior.  Portable programs may wish to check for EAGAIN and loop, just as with EINTR.
// https://github.com/openssh/openssh-portable/blob/master/openbsd-compat/bsd-poll.c
//
static int PXCALL poll(pollfd* fds, uint32 nfds, int timeout);   // Volatile (Flags only?)  // -1 is infinite
// Poll Process/Thread, File/Pipe, socket
static int PXCALL pollGD(pollfd* fds, uint32 nfds, sint64 timeout, PINT64 time_rem);   // Extension   // Will not break on signalls (loops)     // -1 is infinite, negative timeout - use ppoll with microsec timeout
static int PXCALL spollGD(fdsc_t fd, uint32 events, sint64 timeout, PINT64 time_rem);  // Extension   // Returns Events, Error if negative      // Will not return on signals if wait_rem is NULL
// Does it writes remaining time if an event received?
static int PXCALL ppoll(pollfd* fds, uint32 nfds, PTiSp tmo_p, const PSigSet sigmask, SIZE_T sigsetsize);   // Updates tmo_p with remaining time if interrupted  // 'sigsetsize' is Linux only

// https://unix.stackexchange.com/questions/84227/limits-on-the-number-of-file-descriptors
// https://stackoverflow.com/questions/18952564/understanding-fd-set-in-unix-sys-select-h
// WARNING: select() can monitor only file descriptors numbers that are less than FD_SETSIZE (1024) an unreasonably low limit for many modern applications and this limitation will not change.  All modern applications should instead use poll(2) or epoll(7), which do not suffer this limitation.
//static int PXCALL select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval* timeout);    // Volatile and complicated   // Ignore it completely, poll is just faster because there is no need to parse bitfields

// /arch/{ARCH}/include/uapi/asm/stat.h
// https://stackoverflow.com/questions/29249736/what-is-the-precise-definition-of-the-structure-passed-to-the-stat-system-call
// Since kernel 2.5.48, the stat structure supports nanosecond resolution for the three file timestamp fields.
// Too volatile to use crossplatform?

struct SFStat      // Generic     // This one is used in NAPI
{
 uint64 dev;            // ID of device containing file
 uint64 ino;            // inode number
 uint32 nlink;          // number of hard links
 uint32 mode;           // protection and type
 uint32 uid;            // user ID of owner
 uint32 gid;            // group ID of owner
 uint64 rdev;           // device ID (if special file)
 sint64 size;           // total size, in bytes
 uint64 blksize;        // blocksize for file system I/O
 uint64 blocks;         // Number 512-byte blocks allocated.
 STSpec<uint64> atime;  // The field st_atime is changed by file accesses, for example, by execve(2), mknod(2), pipe(2), utime(2) and read(2) (of more than zero bytes)
 STSpec<uint64> mtime;  // The field st_mtime is changed by file modifications, for example, by mknod(2), truncate(2), utime(2) and write(2) (of more than zero bytes)
 STSpec<uint64> ctime;  // The field st_ctime is changed by writing or by setting inode information (i.e., owner, group, link count, mode, etc.)
};

struct SFStatX86x64      // On x86_64
{
 uint64 dev;
 uint64 ino;
 uint64 nlink;
 uint32 mode;
 uint32 uid;
 uint32 gid;
 uint32 __pad0;
 uint64 rdev;
 sint64 size;
 uint64 blksize;
 uint64 blocks;
 STSpec<uint64> atime;
 STSpec<uint64> mtime;
 STSpec<uint64> ctime;
 int64  __unused[3];
};

/*struct SFStatX86x32
{

};*/

struct SFStatArm64      // On raspberry pi ARMx64
{
 uint64 dev;
 uint64 ino;
 uint32 mode;
 uint32 nlink;
 uint64 uid;
 uint64 gid;
 uint64 rdev;
 sint64 size;
 uint64 blksize;
 uint64 blocks;
 STSpec<uint64> atime;
 STSpec<uint64> mtime;
 STSpec<uint64> ctime;
 int64  __unused[3];
};

/*struct SFStatArm32     // Do not use - 32bit sizes! (Use SFStat64 on x32 systems with xxx64 functions instead)
{
 uint32 dev;
 uint32 ino;
 uint16 mode;
 uint16 nlink;
 uint16 uid;
 uint16 gid;
 uint32 rdev;
 uint32 size;
 uint32 blksize;
 uint32 blocks;
 STSpec<uint32> atime;
 STSpec<uint32> mtime;
 STSpec<uint32> ctime;
 uint32  unused[2]
}; */

struct SFStat64    // On ARMx32,...  For xxx64 functions (mode,nlink,uid,gid,size is compatible with SFStatArm64)
{
 uint64  dev;
 uint8   __pad0[4];
 uint32  __ino;       // inode number
 uint32  mode;
 uint32  nlink;
 uint32  uid;
 uint32  gid;
 uint64  rdev;
 uint8   __pad3[4];
 sint64  size;
 uint32  blksize;
 uint64  blocks;      // Number 512-byte blocks allocated.
 STSpec<uint32>  atime;       // time of last access
 STSpec<uint32>  mtime;       // time of last modification
 STSpec<uint32>  ctime;       // time of last status change
 uint64  ino;
};

static void ConvertToNormalFstat(SFStat* Dst, vptr Src)
{
 using TSrcType = typename TSW<IsArchX64, typename TSW<IsCpuARM, SFStatArm64, SFStatX86x64>::T, typename TSW<IsCpuARM, SFStat64, SFStat64>::T>::T;   // TODO: BSD, XNU    // Is SFStat64 same on ARM and X86?
 TSrcType* SrcPtr = (TSrcType*)Src;
 SFStat Tmp;   // Dst may be same as Src

 Tmp.dev     = SrcPtr->dev;
 Tmp.ino     = SrcPtr->ino;
 Tmp.nlink   = SrcPtr->nlink;
 Tmp.mode    = SrcPtr->mode;
 Tmp.uid     = SrcPtr->uid;
 Tmp.gid     = SrcPtr->gid;
 Tmp.rdev    = SrcPtr->rdev;
 Tmp.size    = SrcPtr->size;
 Tmp.blksize = SrcPtr->blksize;
 Tmp.blocks  = SrcPtr->blocks;
 Tmp.atime   = SrcPtr->atime;   // May be STSpec<uint32>  to  STSpec<uint64>
 Tmp.mtime   = SrcPtr->mtime;
 Tmp.ctime   = SrcPtr->ctime;

 Dst->dev     = Tmp.dev;
 Dst->ino     = Tmp.ino;
 Dst->nlink   = Tmp.nlink;
 Dst->mode    = Tmp.mode;
 Dst->uid     = Tmp.uid;
 Dst->gid     = Tmp.gid;
 Dst->rdev    = Tmp.rdev;
 Dst->size    = Tmp.size;
 Dst->blksize = Tmp.blksize;
 Dst->blocks  = Tmp.blocks;
 Dst->atime   = Tmp.atime;
 Dst->mtime   = Tmp.mtime;
 Dst->ctime   = Tmp.ctime;
}
//------------------------------------------------------------------------------------------------------------

// st_mode:
// xxxx xxxx  xxxx xxxx  xxxx xxxOOOGGGTTT
// O - Owner
// G - Group
// T - Other
//
//
// Flags: AT_SYMLINK_NOFOLLOW  AT_RESOLVE_BENEATH  AT_EMPTY_PATH

using PFStat   = MPTR<SFStat, PHT>;
using PFStat64 = MPTR<SFStat64, PHT>;

static int PXCALL stat(PCCHAR path, PFStat buf);         // Not on Arm64      // NOTE: Do not use, returns 32bit sizes!
static int PXCALL stat64(PCCHAR path, PFStat64 buf);     // stat64 for x32 only

static int PXCALL fstat(fdsc_t fildes, PFStat buf);      // On x32 and x64
static int PXCALL fstat64(fdsc_t fildes, PFStat64 buf);  // fstat64 for x32 only

static int PXCALL fstatat(fdsc_t dirfd, PCCHAR pathname, PFStat buf, int flags);      // On x64 only (called newfstatat)
static int PXCALL fstatat64(fdsc_t dirfd, PCCHAR pathname, PFStat64 buf, int flags);  // fstatat64 for x32 only

enum EATExtra
{
 AT_FDCWD            = IsSysWindows?0:VSLB( (uint)-100, (uint)-2 ),  // Special value used to indicate openat should use the current working directory.
 AT_SYMLINK_NOFOLLOW = VSLB( 0x100, 0x0020 ),  // Do not follow symbolic links.
 AT_REMOVEDIR        = VSLB( 0x200, 0x0080 ),  // Remove directory instead of unlinking file.
 AT_SYMLINK_FOLLOW   = VSLB( 0x400, 0x0040 ),  // Follow symbolic links.
};


// The  d_type field is implemented since Linux 2.6.4.  It occupies a space that was previously a zero-filled padding byte in the linux_dirent structure.  Thus, on kernels up to and including 2.6.3, attempting to access this field always provides the value 0 (DT_UNKNOWN).
// Currently, only some filesystems (among them: Btrfs, ext2, ext3, and ext4) have full support for returning the file type  in  d_type.   All applications must properly handle a return of DT_UNKNOWN.
enum EDEntType
{
 DT_UNKNOWN = 0,   // The file type is unknown
 DT_FIFO    = 1,   // This is a named pipe (FIFO)
 DT_CHR     = 2,   // This is a character device
 DT_DIR     = 4,   // This is a directory
 DT_BLK     = 6,   // This is a block device
 DT_REG     = 8,   // This is a regular file
 DT_LNK     = 10,  // This is a symbolic link
 DT_SOCK    = 12,  // This is a UNIX domain socket
 DT_WHT     = 14   // BSD/Darwin

// Framework extended:   // Bad idea - on Linux will have to loop through all records after getdents to change the flags and do 'stat' on links even if none of this will be useful afterwards
/* DET_FIFO    = 0x01,
 DET_CHR     = 0x02,
 DET_DIR     = 0x04,
 DET_BLK     = 0x08,
 DET_REG     = 0x10,
 DET_LNK     = 0x20,
 DET_SOCK    = 0x40,
 DET_WHT     = 0x80 */
};

struct darwin_dirent32   // when _DARWIN_FEATURE_64_BIT_INODE is NOT defined     // Untested!
{
 uint32 ino;             // file number of entry
 uint16 reclen;          // length of this record
 uint8  type;            // file type, see below
 uint8  namlen;          // length of string in d_name
 achar  name[255 + 1];   // name must be no longer than this
};

struct darwin_dirent64   // when _DARWIN_FEATURE_64_BIT_INODE is defined         // Untested!
{
 uint64 fileno;          // file number of entry
 uint64 seekoff;         // seek offset (optional, used by servers)
 uint16 reclen;          // length of this record
 uint16 namlen;          // length of string in d_name
 uint8  type;            // file type, see below
 achar  name[1024];      // name must be no longer than this
};

struct bsd_dirent32      // For syscall 196 (freebsd11)     _WANT_FREEBSD11_DIRENT
{
 uint32 fileno;          // file number of entry
 uint16 reclen;          // length of this record
 uint8  type;            // file type, see below
 uint8  namlen;          // length of string in d_name
 achar  name[255 + 1];   // name must be no longer than this
};

struct bsd_dirent64      // For syscall 554  // BSDSysVer >= 1200031
{
 uint64 fileno;		     // file number of entry
 sint64 off;		     // directory offset of next entry
 uint16 reclen;		     // length of this record
 uint8  type;		     // file type, see below
 uint8  pad0;
 uint16 namlen;		     // length of string in d_name
 uint16 pad1;
 achar  name[255 + 1];   // name must be no longer than this
};


struct SDirEnt     // linux_dirent64  // For getdents64
{
 uint64 ino;       // Inode number // BSD: ino_t   d_fileno
 sint64 off;       // Offset to next linux_dirent   // BSD: ff_t d_off
 uint16 reclen;    // Length of this linux_dirent
 uint8  type;      // File type (only since Linux 2.6.4;
 achar  name[1];   // Filename (null-terminated)    // length is actually (d_reclen - 2 - offsetof(struct linux_dirent, d_name)
};

//using PSDirEnt = MPTR<SDirEnt, PHT>;

// Queries a directory atomically, advances the file position if the buffer is too small to fit all entries
static int PXCALL getdentsGD(fdsc_t fd, PVOID buf, SIZE_T bufsize);   // General definition
static int PXCALL getdents32(fdsc_t fd, PVOID buf, SIZE_T bufsize);      // Unused
static int PXCALL getdents64(fdsc_t fd, PVOID buf, SIZE_T bufsize);      // unsigned int count

// If the basep pointer value is non-NULL, the getdirentries() system call writes the position of the block read into the location pointed to by basep.
static int PXCALL getdirentries32(fdsc_t fd, PVOID buf, SIZE_T bufsize, PSSIZE_T basep);    // BSD: 196;  XNU: 196  [freebsd11] getdirentries   // Returns SSIZE_T?
static int PXCALL getdirentries64(fdsc_t fd, PVOID buf, SIZE_T bufsize, PSSIZE_T basep);    // BSD: 554;  XNU: 344                              // Returns SSIZE_T?

// Kernel 2.6.16 (Released 20 March, 2006) (Arm64 have only openat)
// https://stackoverflow.com/questions/1670135/what-non-linux-unixes-support-openat
static int PXCALL openat(fdsc_t dirfd, PCCHAR pathname, int flags, mode_t mode);

static int PXCALL mknodat(fdsc_t dirfd, PCCHAR pathname, mode_t mode, dev_t dev);
static int PXCALL mkdirat(fdsc_t dirfd, PCCHAR pathname, mode_t mode);
static int PXCALL linkat(fdsc_t olddirfd, PCCHAR oldpath, fdsc_t newdirfd, PCCHAR newpath, int flags);
static int PXCALL unlinkat(fdsc_t dirfd, PCCHAR pathname, int flags);
static int PXCALL renameat(fdsc_t olddirfd, PCCHAR oldpath, fdsc_t newdirfd, PCCHAR newpath);
static int PXCALL symlinkat(PCCHAR target, fdsc_t newdirfd, PCCHAR linkpath);
static int PXCALL readlinkat(fdsc_t dirfd, PCCHAR pathname, PCHAR buf, SIZE_T bufsiz);
static int PXCALL faccessat(fdsc_t dirfd, PCCHAR pathname, int mode, int flags);

// =========================================== MEMORY ===========================================
enum EMapProt
{
 PROT_NONE  = 0x00,    // Page can not be accessed.
 PROT_READ  = 0x01,    // Page can be read.
 PROT_WRITE = 0x02,    // Page can be written.
 PROT_EXEC  = 0x04,    // Page can be executed.
};

// Changes the access protections for the calling process's memory pages containing any part of the address range in the interval [addr, addr+len-1].  addr must be aligned to a page boundary.
// On success, mprotect() and pkey_mprotect() return zero.  On error, these system calls return -1, and errno is set to indicate the error.
static int PXCALL mprotect(PVOID addr, SIZE_T len, uint32 prot);

static int PXCALL mprotectex(PVOID addr, SIZE_T len, uint32 prot, uint32* prev);  // Entension    // prev may be NULL

// https://github.com/nneonneo/osx-10.9-opensource/blob/master/xnu-2422.1.72/bsd/sys/mman.h#L150     // <<<<<<<<<<<<<<< Not match!
//
enum EMapFlg
{
 MAP_TYPE       = 0x0f,                // Mask for type of mapping.

 MAP_SHARED     = 0x01,                // Share changes.
 MAP_PRIVATE    = 0x02,                // Changes are private.

 MAP_FIXED      = 0x10,                // Interpret addr exactly.  MAP_FIXED_NOREPLACE is preferrable (since Linux 4.17)
 MAP_ANONYMOUS  = VSLB( 0x20, 0x1000  ),                 // Don't use a file.  // BSD?
 MAP_ANON       = MAP_ANONYMOUS,       // allocated from memory, swap space
 MAP_32BIT      = VSLB( 0x40, 0x8000  ),                 // Only give out 32-bit addresses(< 4GB). // BSD?
 MAP_FILE       = 0x00,                // map from file (default)

 MAP_GROWSDOWN  = VSLB( 0x00100, 0  ),              // Stack-like segment.
 MAP_LOCKED     = VSLB( 0x02000, 0  ),              // Lock the mapping.
 MAP_NORESERVE  = VSLB( 0x04000, 0x0040 ),          // Don't check for reservations.
 MAP_POPULATE   = VSLB( 0x08000, 0  ),              // Populate (prefault) pagetables.
 MAP_NONBLOCK   = VSLB( 0x10000, 0  ),              // Do not block on IO.
 MAP_STACK      = VSLB( 0x20000, 0  ),              // Allocation is for a stack.
 MAP_HUGETLB    = VSLB( 0x40000, 0  ),              // arch specific
 MAP_SYNC       = VSLB( 0x80000, 0  ),              // perform synchronous page faults for the mapping
 MAP_JIT        = VSLB( 0,  0x0800    ), // MacOS only // Allocate a region that will be used for JIT purposes  // BSD?
 MAP_NOCACHE    = VSLB( 0,  0x0400    ), // don't cache pages for this mapping

 MAP_UNINITIALIZED = VSLB( 0x4000000, 0 ),

// MacOS: MAP_RESILIENT_MEDIA=0x4000, MAP_RESILIENT_CODESIGN=0x2000
};

// Provides the same interface as mmap, except that the final argument specifies the offset into the file in 4096-byte units
static PVOID PXCALL mmapGD(PVOID addr, SIZE_T length, uint prot, uint flags, fdsc_t fd, uint64 pgoffset);    // Generic definition for x32/x64   // On X32 value of x64 pgoffset is shifted right for mmap2

static PVOID PXCALL mmap2(PVOID addr, SIZE_T length, uint prot, uint flags, fdsc_t fd, SIZE_T pgoffset);     // This system call does not exist on x86-64 and ARM64

// Creates a new mapping in the virtual address space of the calling process.
// Offset must be a multiple of the page size
// Check a returned addr as ((size_t)addr & 0xFFF) and if it is non zero then we have an error code which we red as -((ssize_t)addr)
static PVOID PXCALL mmap(PVOID addr, SIZE_T length, uint prot, uint flags, fdsc_t fd, SIZE_T offset);     // Last 4 args are actually int32 (on x64 too!)   // Since kernel 2.4 glibc mmap() invokes mmap2 with an adjusted value for offset

// Deletes the mappings for the specified address range, and causes further references to addresses within the range to generate invalid memory references.
// The address addr must be a multiple of the page size (but length need not be).
static int   PXCALL munmap(PVOID addr, SIZE_T length);

SCVR int MREMAP_MAYMOVE = 1;
SCVR int MREMAP_FIXED   = 2;

// https://stackoverflow.com/questions/69864177/how-to-increase-the-size-of-memory-region-allocated-with-mmap
/*
With just portable POSIX calls, mmap() with a non-NULL hint address = right after you existing mapping, but without MAP_FIXED; 
it will pick that address if the pages are free (and as @datenwolf says, merge with the earlier mapping into one long extent). 
Otherwise it will pick somewhere else. (Then you have to munmap that mapping that ended up not where you wanted it.)

There is a Linux-specific mmap option: MAP_FIXED_NOREPLACE will return an error instead of mapping at an address different from the hint. 
Kernels older than 4.17 don't know about that flag and will typically treat it as if you used no other flags besides MAP_ANONYMOUS, 
so you should check the return value against the hint.

Do not use MAP_FIXED_NOREPLACE | MAP_FIXED; that would act as MAP_FIXED on old kernels, and maybe also on new kernels that do know about MAP_FIXED_NOREPLACE.

Assuming you know the start of the mapping you want to extend, and the desired new total size, mremap is a better choice than mmap(MAP_FIXED_NOREPLACE). 
It's been supported since at least Linux 2.4, i.e. decades, and keeps the existing mapping flags and permissions automatically (e.g. MAP_PRIVATE, PROT_READ|PROT_WRITE)

If you only knew the end address of the existing mapping, mmap(MAP_FIXED_NOREPLACE) might be a good choice.
*/
static PVOID PXCALL mremap(PVOID old_address, SIZE_T old_size, SIZE_T new_size, int flags, PVOID new_address);   // LINUX specific

enum EMadv
{
 MADV_NORMAL      = 0,         // No further special treatment.
 MADV_RANDOM      = 1,         // Expect random page references.
 MADV_SEQUENTIAL  = 2,         // Expect sequential page references.
 MADV_WILLNEED    = 3,         // Will need these pages.
 MADV_DONTNEED    = 4,         // Don't need these pages.
 MADV_FREE        = VSLB( 8,   5 ),      // Free pages only if memory pressure(or immediately?).
// Linux-cpecific
 MADV_REMOVE      = VSLB( 9,   0 ),      // Remove these pages and resources.
 MADV_DONTFORK    = VSLB( 10,  0 ),      // Do not inherit across fork.
 MADV_DOFORK      = VSLB( 11,  0 ),      // Do inherit across fork.
 MADV_MERGEABLE   = VSLB( 12,  0 ),      // KSM may merge identical pages.
 MADV_UNMERGEABLE = VSLB( 13,  0 ),      // KSM may not merge identical pages.
 MADV_HUGEPAGE    = VSLB( 14,  0 ),      // Worth backing with hugepages.
 MADV_NOHUGEPAGE  = VSLB( 15,  0 ),      // Not worth backing with hugepages.
 MADV_DONTDUMP    = VSLB( 16,  0 ),      // Explicity exclude from the core dump, overrides the coredump filter bits.
 MADV_DODUMP      = VSLB( 17,  0 ),      // Clear the MADV_DONTDUMP flag.
 MADV_WIPEONFORK  = VSLB( 18,  0 ),      // Zero memory on fork, child only.
 MADV_KEEPONFORK  = VSLB( 19,  0 ),      // Undo MADV_WIPEONFORK.
 MADV_HWPOISON    = VSLB( 100, 0 ),      // Poison a page for testing.
};

enum EMSyFlg   // Flags for msync
{
 MS_ASYNC      = 0x0001,    // sync memory asynchronously
 MS_INVALIDATE = 0x0002,    // invalidate mappings & caches
 MS_SYNC       = 0x0004,    // synchronous memory sync
};

// Used to give advice or directions to the kernel about the address range beginning at address addr and with size length bytes In most cases, the goal of such advice is to improve system or application performance.
static int PXCALL madvise(PVOID addr, SIZE_T length, EMadv advice);

static int PXCALL msync(PVOID addr, SIZE_T len, int flags);
static int PXCALL mlock(PCVOID addr, SIZE_T len);
static int PXCALL munlock(PCVOID addr, SIZE_T len);

// added in Linux 3.2
// The flags argument is currently unused and must be set to 0
// In order to read from or write to another process, either the caller must have the capability CAP_SYS_PTRACE, or the real user ID, effective user ID, and saved set-user-ID of the remote process 
//   must match the real user ID of the caller and the real group ID, effective group ID, and saved set-group-ID of the remote process must match the real group ID of the caller.
static SSIZE_T PXCALL process_vm_readv(pid_t pid, const PIOVec local_iov,SIZE_T liovcnt, const PIOVec remote_iov, SIZE_T riovcnt, SIZE_T flags);
static SSIZE_T PXCALL process_vm_writev(pid_t pid, const PIOVec local_iov, SIZE_T liovcnt, const PIOVec remote_iov, SIZE_T riovcnt, SIZE_T flags);

// BEWARE: There will be Corner Cases!
// https://stackoverflow.com/questions/21311080/linux-shared-memory-shmget-vs-mmap
// If your OS has /dev/shm/ then shm_open is equivalent to opening a file in /dev/shm/.
// On OSX you want mmap as the max shared memory with shmget is only 4mb across all processes sadly
// msync
// NOTE: mmap needs a file handle for a named/unnamed shared memory or it it only can do an inheritable shared memory which is not well compatible with Windows
//  but there is a problem with munmap which does not require a file handle on Linux but would do so on Windows
// FileMapping: FileHandle->SectionHandle
// NOTE: Use shared memorry wrapper, not this compatibility hack
// Windows: After a file mapping object is created, the size of the file must not exceed the size of the file mapping object; if it does, not all of the file contents are available for sharing (No resizing)
// If an application specifies a size for the file mapping object that is larger than the size of the actual named file on disk and if the page protection allows write access 
// (that is, the flProtect parameter specifies PAGE_READWRITE or PAGE_EXECUTE_READWRITE), 
// then the file on disk is increased to match the specified size of the file mapping object. 
// If the file is extended, the contents of the file between the old end of the file and the new end of the file are not guaranteed to be zero; the behavior is defined by the file system. 
// If the file on disk cannot be increased, CreateFileMapping fails and GetLastError returns ERROR_DISK_FULL
//  Means: NO MAPPED FILE GROWTH!
//  On windows, shared memory objects is garbage collected and removed when there is no more references
// 'shm_open' is just a wrapper for 'openat'  // Not even present in libc 2.28 on ARM64
// shm_open(3) on Linux relies on tmpfs, usually mounted under /dev/shm. What shm_open() does is to convert the object name into a file path by prepending it with the mount point of the tmpfs filesystem. 
// Some way to enumerate mount points to figure out where tmpfs is actually mounted?
// https://nullprogram.com/blog/2016/04/10/		 // <<< Mapping Multiple Memory Views in User Space.mhtml
// On POSIX systems (Linux, *BSD, OS X, etc.), the three key functions are shm_open(3), ftruncate(2), and mmap(2).
// Windows: FILE_ATTRIBUTE_TEMPORARY	// causes file systems to avoid writing data back to mass storage if sufficient cache memory is available  // At least something if an actual file handle is required (For fread/fwrite ?)

// NOTE: Some older kernels segfault executing memfd_create() rather than returning ENOSYS  (Appeared in kernel 3.17, not supported by WSL1)	 // https://benjamintoll.com/2022/08/21/on-memfd_create/
static int memfd_create(PCCHAR name, uint32 flags);	   // NOTE: The name is not important and can be duplicate  // Most similair on Windows, Linux and BSD for named memory mappings. Requires extra checks and emulation

// https://stackoverflow.com/questions/55704758/is-there-anything-like-shm-open-without-filename
// Unnamed(Windows - no name), Linux(memfd_create, if available), BSD (mkstemp/shm_mkstemp/shm_open(SHM_ANON))
static int memfd_openGD(PCCHAR name, uint32 flags, uint64 size);	   // Generic definition for a named shared memory object  // shm_open() + ftruncate()	 // fstat/NtQuerySection
// =========================================== SOCKET ==================================
enum ESockCall  // socketcall calls  (x86_32 only)
{
 SYS_SOCKET      = 1,
 SYS_BIND        = 2,
 SYS_CONNECT     = 3,
 SYS_LISTEN      = 4,
 SYS_ACCEPT      = 5,
 SYS_GETSOCKNAME = 6,
 SYS_GETPEERNAME = 7,
 SYS_SOCKETPAIR  = 8,
 SYS_SEND        = 9,
 SYS_RECV        = 10,
 SYS_SENDTO      = 11,
 SYS_RECVFROM    = 12,
 SYS_SHUTDOWN    = 13,
 SYS_SETSOCKOPT  = 14,
 SYS_GETSOCKOPT  = 15,
 SYS_SENDMSG     = 16,
 SYS_RECVMSG     = 17,
 SYS_ACCEPT4     = 18,
 SYS_RECVMMSG    = 19,
 SYS_SENDMMSG    = 20
};

enum EShtdn
{
 SHUT_RD,
 SHUT_WR,
 SHUT_RDWR
};

enum ESockDomain
{
 AF_UNSPEC     = 0,
 AF_UNIX            = 1,    // Unix domain sockets (local pipes)
 AF_INET            = 2,    // Internet IP Protocol
// AF_AX25       =  3,  // Amateur Radio AX.25
// AF_IPX         = 4,  // Novell IPX
// AF_APPLETALK = 5,    // Appletalk DDP
//  AF_NETROM      = 6, // Amateur radio NetROM
// AF_BRIDGE       = 7, // Multiprotocol bridge
// AF_AAL5       =  8,  // Reserved for Werner's ATM
// AF_X25       =   9,  // Reserved for X.25 project
 AF_INET6       = VSLB( 10,  30  ), // IP version 6  // Linux 10 ?
// AF_MAX         = 12, // For now..
};

enum ESockType
{
 SOCK_STREAM       = 1,  // stream socket
 SOCK_DGRAM     = 2,  // datagram socket
 SOCK_RAW         = 3,  // raw-protocol interface
 SOCK_RDM         = 4,  // reliably-delivered message
 SOCK_SEQPACKET = 5,  // sequenced packet stream
 SOCK_PACKET       = VSLB( 10, 0 ),
};

enum ESockProto
{
 IPPROTO_IP   = 0,      // dummy for IP
 IPPROTO_ICMP = 1,      // control message protocol
 IPPROTO_IGMP = 2,      // group management protocol
 IPPROTO_GGP  = 3,      // gateway^2 (deprecated)
 IPPROTO_IPV4 = 4,      // IPv4 encapsulation
 IPPROTO_IPIP = IPPROTO_IPV4,    //for compatibility
 IPPROTO_TCP  = 6,      // tcp    // This is what you need in most cases
 IPPROTO_PUP  = 12,     // pup
 IPPROTO_UDP  = 17,     // user datagram protocol
 IPPROTO_IDP  = 22,     // xns idp
 IPPROTO_ND   = 77,     // UNOFFICIAL net disk proto

 IPPROTO_RAW  = 255,    // raw IP packet
 IPPROTO_MAX  = 256
};

enum ESockOpt
{
 SO_DEBUG      = 0x0001,          // turn on debugging info recording
 SO_ACCEPTCONN = 0x0002,          // socket has had listen()
 SO_REUSEADDR  = 0x0004,          // allow local address reuse
 SO_KEEPALIVE  = 0x0008,          // keep connections alive
 SO_DONTROUTE  = 0x0010,          // just use interface addresses
 SO_BROADCAST  = 0x0020,          // permit sending of broadcast msgs
};

using socklen_t = int;   // Should be same size as int, not size_t

// The format and size of the address is usually protocol specific.
struct sockaddr
{
 uint16 sa_family;           // address family, AF_xxx
 uint8  sa_data[14];         // 14 bytes of protocol address
};

struct msghdr
{
 PVOID      msg_name;       // Optional address
 socklen_t  msg_namelen;    // Size of address
 PIOVec     msg_iov;        // Scatter/gather array
 SIZE_T     msg_iovlen;     // # elements in msg_iov
 PVOID      msg_control;    // Ancillary data, see below
 SIZE_T     msg_controllen; // Ancillary data buffer len
 int        msg_flags;      // Flags on received message
};

using PMsgHdr    = MPTR<msghdr,   PHT>;
using PSockAddr  = MPTR<sockaddr, PHT>;
using PSockLent  = MPTR<socklen_t,PHT>;

static int PXCALL socketcall(int call, unsigned long *args);    // Deprecated?

static int PXCALL socket(ESockDomain domain, ESockType type, ESockProto protocol);
static int PXCALL connect(int sockfd, PSockAddr addr, socklen_t addrlen);
static int PXCALL bind(int sockfd, PSockAddr addr, socklen_t addrlen);
static int PXCALL accept(int sockfd, PSockAddr addr, PSockLent addrlen);
static int PXCALL accept4(int sockfd, PSockAddr addr, PSockLent addrlen, int flags=0);  // Linux x32   // INTERNAL
static int PXCALL listen(int sockfd, int backlog);
static int PXCALL shutdown(int sockfd, int how);

static int PXCALL getsockopt(int sockfd, int level, int optname, PVOID optval, PSockLent optlen);
static int PXCALL setsockopt(int sockfd, int level, int optname, PVOID optval, socklen_t optlen);

// With zero flags read and write can be used instead of recv and send.
static SSIZE_T PXCALL send(int sockfd, PVOID buf, size_t len, int flags);
static SSIZE_T PXCALL sendto(int sockfd, PVOID buf, size_t len, int flags, PSockAddr dest_addr, socklen_t addrlen);
static SSIZE_T PXCALL sendmsg(int sockfd, PMsgHdr msg, int flags);

static SSIZE_T PXCALL recv(int sockfd, PVOID buf, size_t len, int flags);
static SSIZE_T PXCALL recvfrom(int sockfd, PVOID buf, size_t len, int flags, PSockAddr src_addr, PSockLent addrlen);
static SSIZE_T PXCALL recvmsg(int sockfd, PMsgHdr msg, int flags);

// =========================================== PROCESS/THREAD/DEBUG ===========================================
// Terminates the calling process "immediately".  Any open file descriptors belonging to the process are closed.  Any children of the process are inherited by init(1) (or by the nearest "subreaper" process as defined through the use of the prctl(2) PR_SET_CHILD_SUBREAPER operation).  The process's parent is sent a SIGCHLD signal.
// The value status & 0xFF is returned to the parent process as the process's exit status, and can be collected by the parent using one of the wait(2) family of calls.
// The raw _exit() system call terminates only the calling thread, and actions such as reparenting child processes or sending SIGCHLD to the parent process are performed only if this is the last thread in the thread group.
static void PXCALL exit(int status);

// This system call is equivalent to _exit(2) except that it terminates not only the calling thread, but all threads in the calling process's thread group.
static void PXCALL exit_group(int status);

// returns the caller's thread ID (TID). In a single-threaded process, the thread ID is equal to the process ID (PID, as returned by getpid(2)).
// In a multithreaded process, all threads have the same PID, but each one has a unique TID.
static pid_t PXCALL gettid(void);      // Linux specific // MacOS have gettid name but it is related to user groups  // use pthread_self which usually a pointer or other big number
// returns the process ID (PID) of the calling process.
static pid_t PXCALL getpid(void);
// returns the process ID of the parent of the calling process.  This will be either the ID of the process that created this process using fork(),
// or, if that process has already terminated, the ID of the process to which this process has been reparented (either init(1) or a "subreaper" process defined via the prctl(2) PR_SET_CHILD_SUBREAPER operation).
// If the caller's parent is in a different PID namespace (see pid_namespaces(7)), getppid() returns 0.
static pid_t PXCALL getppid(void);

// get the process group ID of the calling process
static pid_t PXCALL getpgrp(void);    // Deprecated on ARM64

// returns the PGID of the process specified by pid. If pid is zero, the process ID of the calling process is used (same as getpgrp). 
static pid_t PXCALL getpgid(pid_t pid);

// For job control
// If pgid is zero, then the PGID of the process specified by pid is made the same as its process ID
static int   PXCALL setpgid(pid_t pid, pid_t pgid);

// On success, the PID of the child process is returned in the parent, and 0 is returned in the child.  On failure, -1 is returned in the parent, no child process is created
static pid_t PXCALL vfork(void);
static pid_t PXCALL fork(void);

// The kill() system call can be used to send any signal to any process group or process.
static int   PXCALL kill(pid_t pid, int sig);
static int   PXCALL tgkill(pid_t tgid, pid_t tid, int sig);

// A child created via fork(2) inherits its parent's process group ID.  The PGID is preserved across an execve(2).
static int   PXCALL execve(PCCHAR pathname, PPCHAR argv, PPCHAR envp);
// prctl is Linux only

enum ECloneFlags    // TODO: BSD/MacOS mapping     // NOTE: 'clone' is not exported to NAPI
{
 CLONE_SIGMSK         = 0x000000ff, // signal mask to be sent at exit
 CLONE_VM             = 0x00000100, // set if VM shared between processes
 CLONE_FS             = 0x00000200, // set if fs info shared between processes
 CLONE_FILES          = 0x00000400, // set if open files shared between processes
 CLONE_SIGHAND        = 0x00000800, // set if signal handlers and blocked signals shared
 CLONE_PIDFD          = 0x00001000, // set if a pidfd should be placed in parent       // Linux specific
 CLONE_PTRACE         = 0x00002000, // set if we want to let tracing continue on the child too
 CLONE_VFORK          = 0x00004000, // set if the parent wants the child to wake it up on mm_release (execution of the calling process is suspended)
 CLONE_PARENT         = 0x00008000, // set if we want to have the same parent as the cloner
 CLONE_THREAD         = 0x00010000, // Same thread group?
 CLONE_NEWNS          = 0x00020000, // New mount namespace group
 CLONE_SYSVSEM        = 0x00040000, // share system V SEM_UNDO semantics
 CLONE_SETTLS         = 0x00080000, // create a new TLS for the child
 CLONE_PARENT_SETTID  = 0x00100000, // set the TID in the parent
 CLONE_CHILD_CLEARTID = 0x00200000, // clear the TID in the child
 CLONE_DETACHED       = 0x00400000, // Unused, ignored
 CLONE_UNTRACED       = 0x00800000, // set if the tracing process can't force CLONE_PTRACE on this clone
 CLONE_CHILD_SETTID   = 0x01000000, // set the TID in the child
 CLONE_NEWCGROUP      = 0x02000000, // New cgroup namespace
 CLONE_NEWUTS         = 0x04000000, // New utsname namespace
 CLONE_NEWIPC         = 0x08000000, // New ipc namespace
 CLONE_NEWUSER        = 0x10000000, // New user namespace
 CLONE_NEWPID         = 0x20000000, // New pid namespace
 CLONE_NEWNET         = 0x40000000, // New network namespace
 CLONE_IO             = 0x80000000, // Clone io context
};
// https://github.com/raspberrypi/linux/blob/rpi-5.15.y/kernel/fork.c
// glibc/glibc/sysdeps/unix/sysv/linux/x86_64/clone.S.html
// It returns 0 in the child process and returns the PID of the child in the parent.
static pid_t  PXCALL cloneB0(uint32 flags, PVOID newsp, PINT32 parent_tid, PINT32 child_tid, PVOID tls);  // Linux specific  // x86-x64, ...            // struct user_desc* tls     // Use this as a generic definition
static pid_t  PXCALL cloneB1(uint32 flags, PVOID newsp, PINT32 parent_tid, PVOID tls, PINT32 child_tid);  // Linux specific  // x86-32, ARM32, ARM64, ...

SCVR int  WNOHANG    = 1;   // Don't block waiting.
SCVR int  WUNTRACED  = 2;   // Report status of stopped children.
SCVR int  WCONTINUED = 3;   // Return if a stopped child has been resumed by delivery of SIGCONT

static constexpr _finline int32 WEXITSTATUS(int32 s) {return ((s & 0xff00) >> 8);}
static constexpr _finline int32 WTERMSIG(int32 s) {return (s & 0x7f);}
static constexpr _finline int32 WSTOPSIG(int32 s) {return WEXITSTATUS(s);}
static constexpr _finline bool  WCOREDUMP(int32 s) {return (s & 0x80);}
static constexpr _finline bool  WIFEXITED(int32 s) {return (!WTERMSIG(s));}		// returns true if the child terminated normally, that is, by calling exit(3) or _exit(2), or by returning from main().
static constexpr _finline bool  WIFSTOPPED(int32 s) {return ((short)(((s & 0xffff)*0x10001U)>>8) > 0x7f00);}
static constexpr _finline bool  WIFSIGNALED(int32 s) {return ((s & 0xffff)-1U < 0xffu);}
static constexpr _finline bool  WIFCONTINUED(int32 s) {return (s == 0xffff);}

/*
When a process terminates its parent process must acknowledge this using the wait or waitpid function. These functions also return the exit status.

A child that terminates, but has not been waited for becomes a "zombie". The kernel maintains a minimal set of information about the
zombie process (PID, termination status, resource usage information) in order to allow the parent to later perform a wait to obtain
information about the child. As long as a zombie is not removed from the system via a wait, it will consume a slot in the kernel
process table, and if this table fills, it will not be possible to create further processes. If a parent process terminates,
then its "zombie" children (if any) are adopted by init(8), which automatically performs a wait to remove the zombies.

The wait() system call suspends execution of the calling process until one of its children terminates. The call wait(&status) is equivalent to: waitpid(-1, &status, 0);
The waitpid() system call suspends execution of the calling process until a child specified by pid argument has changed state.

https://stackoverflow.com/questions/18441760/linux-where-are-the-return-codes-stored-of-system-daemons-and-other-processes?noredirect=1&lq=1
/proc/[pid]/stat
kill(getpid(), SIGKILL);
*/
static pid_t  PXCALL wait4(pid_t pid, PINT32 wstatus, int options, PVOID rusage);    // Old, should use  waitpid or waitid


/*
// Linux
 CLOCK_REALTIME                  0
 CLOCK_MONOTONIC                 1
 CLOCK_PROCESS_CPUTIME_ID        2
 CLOCK_THREAD_CPUTIME_ID         3
 CLOCK_MONOTONIC_RAW             4
 CLOCK_REALTIME_COARSE           5
 CLOCK_MONOTONIC_COARSE          6
 CLOCK_BOOTTIME                  7
 CLOCK_REALTIME_ALARM            8
 CLOCK_BOOTTIME_ALARM            9
 CLOCK_SGI_CYCLE                10      // In linux/time.h only.
 CLOCK_TAI                      11      // In linux/time.h only.

// FreeBSD
 CLOCK_REALTIME                  0
 CLOCK_VIRTUAL                   1
 CLOCK_PROF                      2
 CLOCK_MONOTONIC                 4
 CLOCK_UPTIME                    5       // Synonymous to CLOCK_BOOTTIME?
 CLOCK_UPTIME_PRECISE            7
 CLOCK_UPTIME_FAST               8
 CLOCK_REALTIME_PRECISE          9       // Same as CLOCK_REALTIME?
 CLOCK_REALTIME_FAST            10       // Synonymous to CLOCK_REALTIME_COARSE?
 CLOCK_MONOTONIC_PRECISE        11       // Same as CLOCK_MONOTONIC?
 CLOCK_MONOTONIC_FAST           12       // Synonymous to CLOCK_MONOTONIC_COARSE?
 CLOCK_SECOND                   13
 CLOCK_THREAD_CPUTIME_ID        14
 CLOCK_PROCESS_CPUTIME_ID       15
 */
// For measuring elapsed time, CLOCK_MONOTONIC is recommended. This clock will not necessarily reflect the time of day but, unlike CLOCK_REALTIME, 
//   it is guaranteed to always be linearly increasing (although not necessarily between reboots). CLOCK_MONOTONIC is affected by adjustments caused by 
//   the Network Time Protocol (NTP) daemon. However, NTP adjustments will not cause this clock to jump; it's rate might be adjusted to compensate for clock drift. 
//   CLOCK_REALTIME, on the other hand, may leap forward or even backward after a time adjustment.
enum EClockType
{
 CLOCK_REALTIME  = 0,        // Higher resolution than the one gettimeofday uses (nsecs)
 CLOCK_MONOTONIC = VSLB( 1, 4 ),

// Flags
 TIMER_ABSTIME   = 1,  // BSD?

 // TODO: Allow other (non cross platform clock types)?
 CLKFG_ABSOLUTE = TIMER_ABSTIME << 16,   // The Framework extension
 CLKFG_INTRABLE = 0x0100 << 16,          // The wait can be interrupted by a signal    // nsleep, usleep, msleep, sleep
 CLK_LOWMSK = 0xFFFFFF
};

/*
https://stackoverflow.com/questions/12392278/measure-time-in-linux-time-vs-clock-vs-getrusage-vs-clock-gettime-vs-gettimeof

POSIX.1 specifies that nanosleep() should measure time against the CLOCK_REALTIME clock.  However, Linux measures the time using the CLOCK_MONOTONIC clock.  
This probably does not matter, since the POSIX.1 specification for clock_settime(2) says that discontinuous changes in CLOCK_REALTIME should not affect nanosleep()
If the duration is not an exact multiple of the granularity underlying clock (see time(7)), then the interval will be rounded up to the next multiple.  
  Furthermore, after the sleep completes, there may still be a delay before the CPU becomes free to once again execute the calling thread.

EINTR  The pause has been interrupted by a signal that was delivered to the thread (see signal(7)).  The remaining sleep time has been written 
  into *rem so that the thread can easily call nanosleep() again and continue with the pause.

EINVAL The value in the tv_nsec field was not in the range [0,999999999] or tv_sec was negative.
*/
static int PXCALL nanosleep(const PTiSp duration, PTiSp remain);   // Always alertable, always relative time
static int PXCALL clocksleep(const PTiSp duration, PTiSp remain, uint32 ClkType);   // Always alertable  // Extension for NAPI (merges clockid and flags) // ClkType is defaulted to -1 which is CLOCK_MONOTONIC

// https://stackoverflow.com/questions/31073923/clockid-t-clock-gettime-first-argument-portability
// Only CLOCK_REALTIME and CLOCK_MONOTONIC is crossplatform. 'remain' is only used in relative sleep 
// Calling clock_nanosleep() without TIMER_ABSTIME flag and with a clock_id of CLOCK_REALTIME is equivalent to calling nanosleep()
static int PXCALL clock_nanosleep(EClockType clockid, int flags, const PTiSp duration, PTiSp remain);

static int PXCALL clock_gettime(EClockType clk_id, PTiSp tp);
static int PXCALL gettime(PTiSp tp, EClockType clk_id);     // Extension for NAPI (shortens the name, reorders args)  // clk_id defaults to 0

// Extension, non POSIX
static int PXCALL nsleep(PTiSp nsecs, uint32 flags);   // If alertable and interrupted, updates nsecs   // Returns positive if interrupted, negative if error, 0 if succeeds
// Extension, non POSIX
// Suspend execution for microsecond intervals
// 'usleep' takes microseconds, so you will have to multiply the input by 1000 in order to sleep in milliseconds.
// Return: Zero if the requested time has elapsed, or the number of seconds left to sleep
//
static uint64 PXCALL usleep(uint64 usec, uint32 flags);   // Returns remaining time if interrupted
// Milliseconds. Extension, non POSIX
static uint32 PXCALL msleep(uint32 msec, uint32 flags);   // Returns remaining time if interrupted
// Seconds. Extension, non POSIX
static uint32 PXCALL sleep(uint32 sec, uint32 flags);     // Separate functions to avoid double multiplication (because they are wrappers for nanosleep)

enum EFutex
{
 FUTEX_WAIT            = 0,    // BSD
 FUTEX_WAKE            = 1,    // BSD
 // FUTEX_FD              = 2,    // Probably can be done on Windows (any use?)  // Has been removed from Linux 2.6.26 onward
 // FUTEX_REQUEUE         = 3,    // BSD (Can it be done on Windows?)
 // FUTEX_CMP_REQUEUE     = 4,
 // FUTEX_WAKE_OP         = 5,
 // FUTEX_LOCK_PI         = 6,
 // FUTEX_UNLOCK_PI       = 7,
 // FUTEX_TRYLOCK_PI      = 8,
    FUTEX_WAIT_BITSET     = 9,    //  If timeout is not NULL, the structure it points to specifies an absolute timeout for the wait operation. Bitset itself is not supported
 // FUTEX_WAKE_BITSET     = 10,
 // FUTEX_WAIT_REQUEUE_PI = 11,
 // FUTEX_CMP_REQUEUE_PI  = 12,
 // FUTEX_LOCK_PI2        = 13,
 FUTEX_PRIVATE_FLAG    = 128,	  // Linux, new  // Gives much better performance
 FUTEX_CLOCK_REALTIME  = 256,
 FUTEX_CMD_MASK        = ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME),

 // FUTEX_WAIT_PRIVATE = (FUTEX_WAIT | FUTEX_PRIVATE_FLAG),
 // FUTEX_WAKE_PRIVATE = (FUTEX_WAKE | FUTEX_PRIVATE_FLAG),
 // FUTEX_REQUEUE_PRIVATE = (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG),

// Not portable (Only have advantage when done in the kernel)
 // FUTEX_OP_SET       = 0,   // *(int *)UADDR2  = OPARG;
 // FUTEX_OP_ADD       = 1,   // *(int *)UADDR2 += OPARG; 
 // FUTEX_OP_OR        = 2,   // *(int *)UADDR2 |= OPARG; 
 // FUTEX_OP_ANDN      = 3,   // *(int *)UADDR2 &= ~OPARG; 
 // FUTEX_OP_XOR       = 4,   // *(int *)UADDR2 ^= OPARG; 
 // 
 // FUTEX_OP_OPARG_SHIFT = 8,   // Use (1 << OPARG) instead of OPARG. 
 // 
 // FUTEX_OP_CMP_EQ    = 0,   // if (oldval == CMPARG) wake 
 // FUTEX_OP_CMP_NE    = 1,   // if (oldval != CMPARG) wake 
 // FUTEX_OP_CMP_LT    = 2,   // if (oldval <  CMPARG) wake 
 // FUTEX_OP_CMP_LE    = 3,   // if (oldval <= CMPARG) wake 
 // FUTEX_OP_CMP_GT    = 4,   // if (oldval >  CMPARG) wake 
 // FUTEX_OP_CMP_GE    = 5,   // if (oldval >= CMPARG) wake 
};

// Before the thread is suspended the value of the futex variable is checked. If it does not have the same value as the val1 parameter the system call immediately returns with the error EWOULDBLOCK.
// If the time runs out without a notification being sent, the system call returns with the error ETIMEDOUT
// system call can return if the thread received a signal. In this case the error is EINTR.
// for FUTEX_WAIT, timeout is interpreted as a relative value.  This differs from other futex operations, where timeout is interpreted as an absolute value.
// https://man7.org/linux/man-pages/man2/futex.2.html
// To obtain the equivalent of FUTEX_WAIT with an absolute timeout, employ FUTEX_WAIT_BITSET with val3 specified as FUTEX_BITSET_MATCH_ANY
// 
// Note that a wake-up can also be caused by common futex usage patterns in unrelated code that happened to have previously used the
//   futex word's memory location (e.g., typical futex-based implementations of Pthreads mutexes can cause this under some conditions).  Therefore, callers should always
//   conservatively assume that a return value of 0 can mean a spurious wake-up, and use the futex word's value (i.e., the user-space synchronization scheme) to decide whether to continue to block or not.
//
static sint32 PXCALL futexGD(PUINT32 uaddr, int op, uint32 val, const PTiSp timeout);	  // Minimal operation
static sint32 PXCALL futex(PUINT32 uaddr, int op, uint32 val, const PTiSp timeout, PUINT32 uaddr2, uint32 val3);    // Linux   // timespec may be uint32_t val2 (see op)     // Returns long
// OpenBSD:      int futex(PUINT32 uaddr, int op, uint32 val, const PTiSp timeout, PUINT32 uaddr2);
// BSD:		  https://github.com/mumble-voip/sbcelt/blob/master/lib/futex-freebsd.c

struct alignas(vptr) futex_t     // Size: 8/16 bytes   
{
 union {
#ifdef SYS_WINDOWS   // Futex emulation
#  ifdef ARCH_X64    // Have Data field as part of alignment
  struct {
   uint32 Value;     // Pointer-aligned   
   uint32 Data;      // Can be used to store some user's data (like a thread ID)
   usize  HeadDesc;  // Should be initially set to 0
  };
#  else     // X32: Size 8, alignment 4   // No Data field
  struct {          
   uint32 Value; 
   usize  HeadDesc;
  };
#  endif
  usize  SingleValue[2];   // How to make it cover the entire object as 'SingleValue = 0' ?   // Default alignment is of the largest member type size(uint128 on X64) // Use ZeroObj() on it
#else  // Linux - real futex
  uint32 Value; 
  uint32 SingleValue[1];   // Used for zeroing the object on any platform: 'ftx = 0;'
#endif
 };
};

struct futex_ext: futex_t    // Provides a guaranteed Data field (Not used by Futex API)
{
#if !defined(SYS_WINDOWS) || !defined(ARCH_X64)   // No Data, 4-byte alignment of futex_t
 uint32 Data;    // Linux: 8, Win32: 12
#endif
};

struct futex_not    // Size: 4/8 bytes     // Do not use this with Futex API, this is for original Windows conditional variables
{
#if defined(SYS_WINDOWS)
 union {
  usize  HeadDesc;   
  uint32 Value;      // To satisfy compile-time checks
 };
#else
 uint32  Value;      // Can't ignore the variable on Linux
#endif
};

// NOTE: Most of the Flags are just hints
enum EFutexFlg
{
 fxfNoExpected    = 0x10,       // The 'expected' doesn't need to be checked
 fxfNoWakeFIFO    = 0x20,       // No FIFO fairness for Wake() is required (Only going to do WakeAll)
 fxfTypeMsk       = 0x0F,
 fxfTypeAny       = 0,          // Nothing specific
 fxfTypeMutex     = 1,
 fxfTypeEvent     = 2,          // Conditional variable
 fxfTypeRWLock    = 3,
 fxfTypeCondVar   = 4,
 fxfTypeSemaphore = 5
};

SCVR uint32 WAKE_ALL = uint32(-1);

// On success, FUTEX_WAIT returns 0 if the caller was woken up. callers should always conservatively assume that a return value of 0 can mean a spurious wake-up, and use the futex word's value
static sint32 PXCALL futex_wait(futex_t* addr, uint32 expected, timeout_t timeout_ms, uint32 flags);      // macOS doesn't have a stable/public API for timeouts
// On success, FUTEX_WAKE returns the number of waiters that were woken up. The wake operations don't return the number of threads woken up. (Only Linux supports this.)
static sint32 PXCALL futex_wake(futex_t* addr, uint32 count, uint32 flags);


// SA_RESTART should be the default. Any other default is a bad default. Code that doesn't use UNIX signals will never be written with signals in mind and it won't be prepared for EINTR.
// Unfortunately you have to explicitly request for SA_RESTART on the new sigaction() (and not the other way around).
enum ESAFlags
{
 SA_NOCLDSTOP = VSLB(0x00000001, 0x00000008),  // If signum is SIGCHLD, do not receive notification when child processes stop (i.e., when they receive one of SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU) or resume (i.e., they receive SIGCONT)
 SA_NOCLDWAIT = VSLB(0x00000002, 0x00000020),  // If signum is SIGCHLD, do not transform children into zombies when they terminate. 
 SA_SIGINFO   = VSLB(0x00000004, 0x00000040),  // The signal handler takes three arguments, not one. In this case, sa_sigaction should be set instead of sa_handler. 
 SA_NODEFER   = VSLB(0x40000000, 0x00000010),  // Do not prevent the signal from being received from within its own signal handler.
 SA_ONSTACK   = VSLB(0x08000000, 0x00000001),  // Call the signal handler on an alternate signal stack provided by sigaltstack(2).
 SA_RESTART   = VSLB(0x10000000, 0x00000002),  // Provide behavior compatible with BSD signal semantics by making certain system calls restartable across signals. (which were the default long ago) 
 SA_RESETHAND = VSLB(0x80000000, 0x00000004),  // Restore the signal action to the default upon entry to the signal handler. Was (SA_ONESHOT )

 SA_RESTORER  = VSLB(0x04000000, -1),          // Linux only
 // SA_USERTRAMP = VSLB(-1, 0x00000100),          // BSD/XNU(kernel): do not bounce off kernel's sigtramp 
 // SA_64REGSET  = VSLB(-1, 0x00000200),          // BSD/XNU(kernel): signal handler with SA_SIGINFO args with 64bit regs information 
};

enum SAHow
{
 SIG_BLOCK = 0 + bool(IsSysBSD|IsSysMacOS),  // The set of blocked signals is the union of the current set and the set argument.
 SIG_UNBLOCK,  // The signals in set are removed from the current set of blocked signals. It is permissible to attempt to unblock a signal which is not blocked.
 SIG_SETMASK   // The set of blocked signals is set to the argument set.
};

// Generic siginfo_t stable minimum
// https://github.com/torvalds/linux/blob/master/include/linux/signal.h
// Per POSIX, si_addr is only used by SIGILL, SIGFPE, SIGSEGV, and SIGBUS. Linux also provides si_addr data for SIGTRAP
// SIGILL,SIGFPE:  si_addr is an address of the instruction
// SIGBUS,SIGSEGV: si_addr is an address of the accessed memory
//
struct siginfo_t 
{
 sint32 signo;  // Signal number
 sint32 errno;  // An errno value (rarely used, but POSIX)
 sint32 code;   // Signal code (why it happened)

#if defined(SYS_LINUX) || defined(SYS_ANDROID)
 sint32 __pad0;      // 4 bytes (alignment padding)

 // The union starts here. POSIX guarantees these fields 
 // for specific signals (SIGSEGV, SIGFPE, SIGILL, SIGBUS).
 // https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/siginfo.h
 union {          // 112 bytes total for union on x86_64
     // Faulting info (SIGSEGV, SIGILL, etc.)
     struct {
         void* addr;        // Faulting instruction or memory address
    //    short addr_lsb;    // SYS_LINUX/ANDROID  Least significant bit of address (for some traps)
         // More Linux-specific fields follow...
     };// _sigfault;

     // Kill info (SIGKILL, SIGINT, SIGTERM)
     struct {
         sint32 pid;           // Sending process ID 
         uint32 uid;           // Real user ID of sending process
     };// _sigkill;

     // Child info (SIGCHLD)
     struct {
         sint32 pid;           // Child process ID
         uint32 uid;           // User ID of child
         sint32 status;        // Exit value or signal
     };// _sigchld;
     
     // Add more padding if you need to match the actual OS size (usually 128 bytes)
     sint32 _pad[28];      // 112 bytes (128 - 16 header bytes)
 }; // _sdata;
#elif defined(SYS_MACOS) || defined(SYS_BSD)
 sint32 pid;      // NOT in union!
 uint32 uid;
 sint32 status;
 void* addr;
 union {     // union sigval si_value;      
     int32_t _pad[16];  // 64 bytes
 };  // _reason;
 //long    si_band;               // band event for SIGPOLL
#elif defined(SYS_WINDOWS) 
 void* addr;
#endif

 /*   // Portable interface
    // Platform-specific when needed
#if defined(SYS_LINUX) || defined(SYS_ANDROID)
    int16_t address_lsb() const noexcept {
        return _sifields._sigfault.si_addr_lsb;
    }
#endif   */
};

/*
        // Timer info (SI_TIMER)
        struct {
            int si_tid;           // Timer ID
            int si_overrun;       // Overrun count
            union {
                int sival_int;    // Sigval value (integer)
                void* sival_ptr;  // Sigval value (pointer)
            } si_value;
*/


/*
Linux (and POSIX) track three independent things per signal:
   Disposition   - what to do when delivered (SIG_DFL, SIG_IGN, or handler)
   Blocked mask  - whether delivery is deferred (sigprocmask / rt_sigprocmask)
   Pending state - signal already generated but waiting

Ignored (SIG_IGN):
    Kernel throws the signal away immediately.
    It never becomes pending.
    It never wakes anything.
    It never queues.

Blocked (via sigprocmask):
    Signal is generated
    Kernel marks it pending
    Delivery is deferred
    When you unblock → it is delivered

So blocked signals accumulate.

Special case: SIGKILL / SIGSTOP:
    cannot be ignored
    cannot be blocked
    kernel-enforced
    They bypass everything.

Crash signals (SIGSEGV etc):
    are synchronous
    ignore sigprocmask entirely
    delivered immediately
    So blocking SIGSEGV doesn’t prevent a fault.
*/
// https://man7.org/linux/man-pages/man2/sigaction.2.html
// signum specifies the signal and can be any valid signal except SIGKILL and SIGSTOP.

// MacOS: Traditionally uses a 32-bit mask (4 bytes) for its BSD-layer syscalls, though the header defines it as a uint32_t
// glibc / musl: define sigset_t as a massive 128-byte structure (1024 bits) to allow for future expansion.
// FreeBSD: Uses a 128-bit mask (16 bytes). If you are bypassing the FreeBSD System Call Interface and writing raw assembly, you need to provide two uint64_t values.
// Self-Blocking: By default, the signal that triggered the handler is automatically blocked while the handler is running (unless you use the SA_NODEFER flag). 
// sa_mask: You can specify other signals to block only during the handler's execution by populating the sa_mask field in your sigaction struct.

using signal_handler_t = void (PXCALL*)(sint32 sig); 
using signal_action_t  = void (PXCALL*)(sint32 sig, siginfo_t* info, void* ucontext);       // Linux, BSD and MacOS support it natively 

// The void* context must be cast to ucontext_t*
static void PXCALL SignalHandler(sint32 sig); 
static void PXCALL SignalAction(sint32 sig, siginfo_t* info, void* context); 

//TODO: Make sure thet these aren't actually allocated as globals
static inline const signal_handler_t SIG_DFL = (signal_handler_t)0;
static inline const signal_handler_t SIG_IGN = (signal_handler_t)1;
static inline const signal_handler_t SIG_ERR = (signal_handler_t)-1;

struct sigset_old_t    // 16 bytes
{
 uint32 bits[1];
};

struct sigset_new_t    // 16 bytes
{
 uint32 bits[4];
};

// https://github.com/torvalds/linux/blob/master/arch/arm/include/uapi/asm/signal.h
template<typename SM=sigset_old_t> struct sigaction_linux_x32_t   // ARM_X32, X86_X32
{
 //vptr     handler;
 SM       mask;
 size_t   flags;
 vptr     restorer;
};

// https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/signal.h
// https://github.com/torvalds/linux/blob/master/arch/x86/include/uapi/asm/signal.h
template<typename SM=sigset_new_t> struct sigaction_linux_generic_t     // Generic, X86_X64
{
 //vptr     handler;
 size_t   flags;
 vptr     restorer;     // X86,ARM,PPC have it
 SM       mask;         // mask last for extensibility
};

// https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/signal.h
template<typename SM=sigset_old_t> struct sigaction_xnu_t    // MacOS, iOS
{
 //vptr     handler;
 vptr     tramp;
 SM       mask;        //  uint32_t sa_mask[4]; 
 uint32   flags;  
};

// https://github.com/openbsd/src/blob/master/sys/sys/signal.h
// https://github.com/NetBSD/src/blob/trunk/sys/sys/signal.h
// https://github.com/NetBSD/src/blob/trunk/sys/compat/sys/signal.h
template<typename SM=sigset_new_t> struct sigaction_bsd_t    // NetBSD, OpenBSD
{
 //vptr     handler;
 SM       mask;        // uint32_t sa_mask[4];  // Compat: uint32 ?
 uint32   flags;  
};

// https://github.com/freebsd/freebsd-src/blob/main/sys/sys/signal.h
// https://github.com/DragonFlyBSD/DragonFlyBSD/blob/master/sys/sys/signal.h
template<typename SM=sigset_new_t> struct sigaction_fbsd_t   // FreeBSD, DragonFlyBSD
{
 //vptr     handler;
 uint32   flags; 
 SM       mask;        // uint32_t sa_mask[4]; 
};

struct sigaction_t
{
 union {
 signal_handler_t sighandler;
 signal_action_t  sigaction;
 };
 union {
   sigaction_linux_generic_t<> sa_linux_rst;
   sigaction_linux_x32_t<>     sa_linux_x32;
   sigaction_fbsd_t<>          sa_fbsd;
   sigaction_bsd_t<>           sa_bsd;
   sigaction_xnu_t<>           sa_xnu;
 };
};

/*
Crucial Warning: 
  In modern macOS (10.14+), the kernel enforces that the tramp must be the standard libSystem trampoline for security reasons (to prevent ROP attacks). 
  If you point tramp to your own raw assembly memory, the kernel may kill your process with a code-signing violation unless you have specific entitlements.

  The Problem: On macOS, the kernel jumps to the tramp address instead of your handler. The tramp is responsible for calling your handler and then calling sigreturn.
  The Trap: Modern macOS (especially Apple Silicon) validates that the trampoline resides in authorized memory (usually libSystem). If you point tramp to your own bare-metal assembly, the kernel may trigger a Code Signing Violation or SIGILL.
  The Check: The kernel checks the tramp address. If it points to your .text segment, and the .text segment is marked as Executable, the kernel will allow it. If you try to point tramp to a heap/stack address, it will SIGKILL.
  Avoiding SA_SIGINFO does not solve the trampoline requirement on macOS: Even with a simple sa_handler, the kernel still expects the tramp field to be valid because the return mechanism remains the same.
  On iOS/macOS, the kernel expects the trampoline to be responsible for calling the handler and then returning. If your trampoline only calls sigreturn, your handler will never run.
  On macOS/iOS, if tramp is provided, the kernel jumps there. Setting sighandler and tramp to the same assembly entry point is a common "No LibC" trick.

  Use LC_UNIXTHREAD: This is the "old school" way to start a process at a specific register state (rip/pc).
// ARM64 macOS/iOS Trampoline
.text
.align 4
.global _my_sigtramp
_my_sigtramp:
    // At this point, the kernel has pushed the frame.
    // x0 = signal number
    // x1 = siginfo_t* (if SA_SIGINFO)
    // x2 = ucontext_t*
    
    // 1. Call the handler (stored in the sigaction struct, 
    //    but the kernel usually passes it in a register or 
    //    you hardcode a wrapper).
    // Note: For a generic stub, you'd need to know where the handler is.
    // In a "No LibC" static binary, you might jump to a C function here:
    bl _my_c_signal_handler 

    // 2. Call sigreturn
    mov x16, #184      // SYS_sigreturn (0x20000B8)
    svc #0x80

Default Behavior (No Flag): The kernel handles the transition from kernel mode to user mode using its own internal signal trampoline. It sets up the stack frame and ensures the program returns to the correct execution point after the handler finishes.
With SA_USERTRAMP: You (typically via a system library like libsystem) are telling the kernel: "Don't use your default return logic; I am providing a specific user-space 'tramp' address to manage this transition".
Darwin uses a "Comm Page" (a kernel-mapped read-only memory area) that contains a pre-verified sigreturn stub. You usually don't provide your own; the kernel handles the return via a "Tramp" address it manages.
FreeBSD / OpenBSD / NetBSD: On these systems, the kernel typically maps a "Signal Trampoline" into the process's address space automatically. When you call sigaction, the kernel ignores your sa_restorer (if it even exists in that OS's struct) and uses its own internal trampoline.
 
*/

static sint32 PXCALL rt_sigaction(sint32 signum, const sigaction_t* act, sigaction_t* oldact, size_t sigsetsize);   // BSD/MacOS: (sint32 signum, const sigaction_t* act, sigaction_t* oldact); 
//static sint32 PXCALL rt_sigprocmask(sint32 how, const PSigSet set, PSigSet oldset, size_t sigsetsize);          // BSD/MacOS: int sigprocmask(int how, const sigset_t *set, sigset_t *oset)
static sint32 PXCALL rt_sigreturn(vptr ctx, int infostyle, int token);  // Needed only for Linux (Present in VDSO too) // rt_sigreturn - Linux: (size_t __unused)  // BSD: (const struct __ucontext *ucontextp)  // MacOS: (struct ucontext *ucontextp, int token)

};

using PX   = NPOSIX<uint>;   // Current build
using PX64 = NPOSIX<uint64>;
//============================================================================================================
