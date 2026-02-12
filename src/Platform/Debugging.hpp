
//---------------------------------------------------------------------------
// http://web.mit.edu/freebsd/head/contrib/llvm/tools/lldb/source/Plugins/Process/FreeBSD/ProcessMonitor.cpp
// Under FreeBSD, it is not possible to ptrace() from any other thread but the one that spawned or attached to the process from the start.
// http://fxr.watson.org/fxr/source/sys/ptrace.h?v=FREEBSD-11-0;im=10
// https://stackoverflow.com/questions/62009199/macos-ptrace-pt-getregs-doesnt-seem-to-exist
// MacOS: Use thread_get_state to get registers

// https://groups.google.com/g/fa.linux.kernel/c/rAQEQUGsbkU/m/C0oXtIaL5gcJ
// https://stackoverflow.com/questions/18577956/how-to-use-ptrace-to-get-a-consistent-view-of-multiple-threads
//
//---------------------------------------------------------------------------
struct NDBG
{
// https://stackoverflow.com/questions/44534366/what-is-the-range-for-ptrace-traceme
// NOTE: Do not believe the manual. Result is returned in 'data' (Must be a valid pointer)
// NOTE: The arm64 platform only supporting the PTRACE_GETREGSET interface. 
// https://github.com/raspberrypi/linux/blob/rpi-6.12.y/arch/arm64/kernel/ptrace.c
// https://github.com/raspberrypi/linux/blob/rpi-6.12.y/kernel/ptrace.c
//
enum EPTraceCommands
{
// GENERIC operations (LINUX, BSD, MAC) (ARM/X86, X32/X64)
 PT_TRACE_ME = 0,      //  child declares it's being traced     // Won't stop until a signal is sent, i.e. caused by exec
 PT_READ_I   = 1,      //  read word in child's I space         // PTRACE_PEEKTEXT   // The value read is returned as the return value from ptrace()
 PT_READ_D   = 2,      //  read word in child's D space         // PTRACE_PEEKDATA
 PT_READ_U   = 3,      //  read word in child's user structure  // PTRACE_PEEKUSR    // holds the registers and other information about the process (see <sys/user.h>)  // addr must be aligned on an int boundary  // Unimplemented? Always returns EFAULT (When the alignment is correct)
 PT_WRITE_I  = 4,      //  write word in child's I space        // PTRACE_POKETEXT
 PT_WRITE_D  = 5,      //  write word in child's D space        // PTRACE_POKEDATA
 PT_WRITE_U  = 6,      //  write word in child's user structure // PTRACE_POKEUSR   
 PT_CONTINUE = 7,      //  continue the child 
 PT_KILL     = 8,      //  kill the child process 
 PT_STEP     = 9,      //  single step the child 

#if defined(SYS_WINDOWS)
 PT_ATTACH_DEF  = 10,  // trace some running process 
 PT_DETACH_DEF  = 11,  // stop tracing a process
#endif

// 
#if defined(SYS_LINUX)
 PT_GETREGS         = 12,
 PT_SETREGS         = 13,
 PT_GETFPREGS       = 14,
 PT_SETFPREGS       = 15,
 PT_ATTACH_DEF      = 16,
 PT_DETACH_DEF      = 17,
 PT_GETWMMXREGS     = 18,
 PT_SETWMMXREGS     = 19,
                        
 PT_OLDSETOPTIONS   = 21,
 PT_GET_THREAD_AREA = 22,
 PT_SET_SYSCALL     = 23,  // This operation is currently supported only on arm (and arm64, though only for backwards compatibility)
 PT_SYSCALL         = 24,
                            
#if defined(CPU_ARM)           
 PT_GETCRUNCHREGS   = 25,  // obsolete 
 PT_SETCRUNCHREGS   = 26,  // obsolete
 PT_GETVFPREGS      = 27,
 PT_SETVFPREGS      = 28,
 PT_GETHBPREGS      = 29,
 PT_SETHBPREGS      = 30,
 PT_GETFDPIC        = 31,
#else                          
#if defined(ARCH_X64)         
 PT_ARCH_PRCTL      = 30,
#endif
#endif
 PT_SETOPTIONS           = 0x4200,
 PT_GETEVENTMSG	         = 0x4201,
 PT_GETSIGINFO	         = 0x4202,
 PT_SETSIGINFO	         = 0x4203,
 PT_GETREGSET            = 0x4204, // On the successful completion, iov.len will be updated by the kernel, specifying how much the kernel has written/read to/from the user's iov.buf
 PT_SETREGSET            = 0x4205,
 PT_SEIZE                = 0x4206,
 PT_INTERRUPT            = 0x4207,
 PT_LISTEN               = 0x4208,
 PT_PEEKSIGINFO          = 0x4209,
 PT_GETSIGMASK           = 0x420a,
 PT_SETSIGMASK           = 0x420b,
 PT_SECCOMP_GET_FILTER   = 0x420c,
 PT_SECCOMP_GET_METADATA = 0x420d,
 PT_GET_SYSCALL_INFO     = 0x420e,
#endif

// https://github.com/AssemblyScript/musl/blob/master/include/sys/ptrace.h
//#define PTRACE_SYSCALL_INFO_NONE 0
//#define PTRACE_SYSCALL_INFO_ENTRY 1
//#define PTRACE_SYSCALL_INFO_EXIT 2
//#define PTRACE_SYSCALL_INFO_SECCOMP 3

// 
#if defined(SYS_MACOS)
 PT_ATTACH_DEF   = 10,  //  trace some running process   // MacOS: XNU deprecated
 PT_DETACH_DEF   = 11,  //  stop tracing a process 
 PT_SIGEXC       = 12,  //  signals as exceptions for current_proc 
 PT_THUPDATE     = 13,  //  signal for thread# 
 PT_ATTACHEXC    = 14,  //  attach to running process with signal exception // Note that this call differs from the prior call ( PT_ATTACH) in that signals from the child are delivered to the parent as Mach exceptions (see EXC_SOFT_SIGNAL).
                       
 PT_FORCEQUOTA   = 30,  //  Enforce quota for root 
 PT_DENY_ATTACH  = 31,
                     
 PT_FIRSTMACH    = 32,  // for machine-specific requests 
#endif

// https://github.com/freebsd/freebsd-src/blob/main/sys/sys/ptrace.h
#if defined(SYS_BSD)
 PT_ATTACH_DEF     = 10,  // trace some running process 
 PT_DETACH_DEF     = 11,  // stop tracing a process
 PT_IO             = 12,  // do I/O to/from stopped process. 
 PT_LWPINFO        = 13,  // Info about the LWP that stopped. 
 PT_GETNUMLWPS     = 14,  // get total number of threads 
 PT_GETLWPLIST     = 15,  // get thread list 
 PT_CLEARSTEP      = 16,  // turn off single step 
 PT_SETSTEP        = 17,  // turn on single step 
 PT_SUSPEND        = 18,  // suspend a thread 
 PT_RESUME         = 19,  // resume a thread 
                      
 PT_TO_SCE         = 20,
 PT_TO_SCX         = 21,
 PT_SYSCALL        = 22,
                       
 PT_FOLLOW_FORK    = 23,
 PT_LWP_EVENTS     = 24,  // report LWP birth and exit 
                         
 PT_GET_EVENT_MASK = 25,  // get mask of optional events
 PT_SET_EVENT_MASK = 26,  // set mask of optional events
                        
 PT_GET_SC_ARGS    = 27,  // fetch syscall args
 PT_GET_SC_RET     = 28,  // fetch syscall results
                        
 PT_COREDUMP       = 29,  // create a coredump
                       
 PT_GETREGS        = 33,  // get general-purpose registers
 PT_SETREGS        = 34,  // set general-purpose registers
 PT_GETFPREGS      = 35,  // get floating-point registers
 PT_SETFPREGS      = 36,  // set floating-point registers
 PT_GETDBREGS      = 37,  // get debugging registers
 PT_SETDBREGS      = 38,  // set debugging registers
                        
 PT_VM_TIMESTAMP   = 40,  // Get VM version (timestamp)
 PT_VM_ENTRY       = 41,  // Get VM map (entry)
 PT_GETREGSET      = 42,  // Get a target register set
 PT_SETREGSET      = 43,  // Set a target register se
 PT_SC_REMOTE      = 44,  // Execute a syscal
                         
 PT_FIRSTMACH      = 64,  // for machine-specific requests
#endif

 PT_ATTACH = PT_ATTACH_DEF,    // Not on MacOS (Disabled)  // Use higher level debugger abstraction for this
 PT_DETACH = PT_DETACH_DEF,
};

#if defined(SYS_LINUX)
// Options set using PTRACE_SETOPTIONS.
enum __ptrace_setoptions 
{
 PT_O_TRACESYSGOOD   = 0x00000001,  // status: SIGTRAP | 0x80   // Allows to differentiate between a standard SIGTRAP and a system call entry or exit.
 PT_O_TRACEFORK      = 0x00000002,
 PT_O_TRACEVFORK     = 0x00000004,
 PT_O_TRACECLONE     = 0x00000008,
 PT_O_TRACEEXEC      = 0x00000010,
 PT_O_TRACEVFORKDONE = 0x00000020,
 PT_O_TRACEEXIT      = 0x00000040,
 PT_O_MASK           = 0x0000007f
};

// Wait extended result codes for the above trace options. 
enum __ptrace_eventcodes 
{
 PT_EVENT_FORK       = 1,
 PT_EVENT_VFORK      = 2,
 PT_EVENT_CLONE      = 3,
 PT_EVENT_EXEC       = 4,
 PT_EVENT_VFORK_DONE = 5,
 PT_EVENT_EXIT       = 6
};
#endif

// From elf.h
enum ERegSet
{
 NT_PRSTATUS   = 1,         // general-purpose registers
 NT_PRFPREG    = 2,
 NT_PRPSINFO   = 3,
 NT_TASKSTRUCT = 4,
 NT_AUXV       = 6,
 NT_PRXFPREG   = 0x46e62b7f  
};

static void PXCALL cache_flush(vptr begin, vptr end);

// addr and data is either caddr_t (is an alias (typedef) for char*) or void*. But size_t is more appropriate for common use cases.
static size_t PXCALL ptrace(EPTraceCommands op, PX::pid_t pid, size_t addr, size_t data);

// TODO: USER struct for PT_READ_U/PT_WRITE_U (System/Platform specific)  // sys/user.h OR sys/reg.h
// TODO: Registers ?
// https://sites.uclouvain.be/SystInfo/usr/include/sys/ptrace.h.html
// https://sites.uclouvain.be/SystInfo/usr/include/sys/reg.h.html

// https://stackoverflow.com/questions/5477976/how-to-ptrace-a-multi-threaded-application
// https://github.com/aleden/ptracetricks
// https://github.com/raspberrypi/linux/blob/rpi-6.12.y/include/uapi/linux/ptrace.h

// ARM64: X8 syscall

/*  ::: Linux
#if __WORDSIZE == 64
// Index into an array of 8 byte longs returned from ptrace for location of the users' stored general purpose registers.  
# define R15        0
# define R14        1
# define R13        2
# define R12        3
# define RBP        4
# define RBX        5
# define R11        6
# define R10        7
# define R9        8
# define R8        9
# define RAX        10
# define RCX        11
# define RDX        12
# define RSI        13
# define RDI        14
# define ORIG_RAX 15
# define RIP        16
# define CS        17
# define EFLAGS        18
# define RSP        19
# define SS        20
# define FS_BASE 21
# define GS_BASE 22
# define DS        23
# define ES        24
# define FS        25
# define GS        26
#else

// Index into an array of 4 byte integers returned from ptrace for location of the users' stored general purpose registers. 

# define EBX 0
# define ECX 1
# define EDX 2
# define ESI 3
# define EDI 4
# define EBP 5
# define EAX 6
# define DS 7
# define ES 8
# define FS 9
# define GS 10
# define ORIG_EAX 11
# define EIP 12
# define CS  13
# define EFL 14
# define UESP 15
# define SS   16
#endif
*/

/*
#define ARM_cpsr	uregs[16]
#define ARM_pc		uregs[15]
#define ARM_lr		uregs[14]
#define ARM_sp		uregs[13]
#define ARM_ip		uregs[12]
#define ARM_fp		uregs[11]
#define ARM_r10		uregs[10]
#define ARM_r9		uregs[9]
#define ARM_r8		uregs[8]
#define ARM_r7		uregs[7]
#define ARM_r6		uregs[6]
#define ARM_r5		uregs[5]
#define ARM_r4		uregs[4]
#define ARM_r3		uregs[3]
#define ARM_r2		uregs[2]
#define ARM_r1		uregs[1]
#define ARM_r0		uregs[0]
#define ARM_ORIG_r0	uregs[17]
*/
//---------------------------------------------------------------------------
struct STElapsed  // TODO: Move in Time Utils
{
 NPTM::PX::timespec start, end, diff;

 STElapsed(void) {this->Reset();}
//-----------------------------------------------------
void Reset(void)
{
 NPTM::NAPI::gettime(&this->start, NPTM::PX::CLOCK_MONOTONIC);
}
//-----------------------------------------------------
void Measure(void)
{
 NPTM::NAPI::gettime(&this->end, NPTM::PX::CLOCK_MONOTONIC);
 NPTM::NDT::TimeDiff(&this->diff, &this->start, &this->end);
 uint ms = this->diff.frac / 1000000L;  // TODO: Adjustable
 LOGMSG("Elapsed time: %u.%03u seconds\n", this->diff.sec, ms);
 this->start = this->end;
}

};
//---------------------------------------------------------------------------
};
