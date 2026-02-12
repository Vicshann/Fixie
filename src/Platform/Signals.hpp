
struct NSIG 
{

//=======================================================================================================
// These are passed as 'context' to the signal handler

#if defined(SYS_WINDOWS)

using UCONTEXT = NT::EXCEPTION_POINTERS;    // TODO: ARM

//=======================================================================================================
#elif defined(SYS_LINUX)

#if defined(CPU_ARM)

// https://github.com/torvalds/linux/blob/master/arch/arm64/include/uapi/asm/sigcontext.h
struct sigcontext_arm_64 
{  
 uint64 fault_address;
 uint64 regs[31];
 uint64 sp;
 uint64 pc;
 uint64 pstate;
 uint8  alignas(16) __reserved[4096];   // 4K reserved for FP/SIMD state and future expansion
};

// https://github.com/torvalds/linux/blob/master/arch/arm/include/uapi/asm/sigcontext.h
struct sigcontext_arm_32 
{   
 uint32 trap_no;
 uint32 error_code;
 uint32 oldmask;
 uint32 arm_r0;
 uint32 arm_r1;
 uint32 arm_r2;
 uint32 arm_r3;
 uint32 arm_r4;
 uint32 arm_r5;
 uint32 arm_r6;
 uint32 arm_r7;
 uint32 arm_r8;
 uint32 arm_r9;
 uint32 arm_r10;
 uint32 arm_fp;
 uint32 arm_ip;
 uint32 arm_sp;
 uint32 arm_lr;
 uint32 arm_pc;
 uint32 arm_cpsr;
 uint32 fault_address;
};

using MCONTEXT32 = sigcontext_arm_32;
using MCONTEXT64 = sigcontext_arm_64;
using MCONTEXT   = TSW<IsArchX64, MCONTEXT64, MCONTEXT32>::T;

#endif

// ---------------------- X86

#if defined(CPU_X86)

// https://github.com/torvalds/linux/blob/master/arch/x86/include/uapi/asm/sigcontext.h
//
struct sigcontext_x86_32 
{   
 uint16  gs, __gsh;
 uint16  fs, __fsh;
 uint16  es, __esh;
 uint16  ds, __dsh;
 uint32  di;
 uint32  si;
 uint32  bp;
 uint32  sp;
 uint32  bx;
 uint32  dx;
 uint32  cx;
 uint32  ax;
 uint32  trapno;
 uint32  err;
 uint32  ip;
 uint16  cs, __csh;
 uint32  flags;
 uint32  sp_at_signal;
 uint16  ss, __ssh;
// fpstate is really (struct _fpstate *) or (struct _xstate *) depending on the FP_XSTATE_MAGIC1 encoded in the SW reserved bytes of (struct _fpstate) and FP_XSTATE_MAGIC2 present at the end of extended memory layout. See comments at the definition of (struct _fpx_sw_bytes)  
 uint32  fpstate; // Zero when no FPU/extended context 
 uint32  oldmask;
 uint32  cr2;
};

struct sigcontext_x86_64 
{
 uint64  r8;
 uint64  r9;
 uint64  r10;
 uint64  r11;
 uint64  r12;
 uint64  r13;
 uint64  r14;
 uint64  r15;
 uint64  di;
 uint64  si;
 uint64  bp;
 uint64  bx;
 uint64  dx;
 uint64  ax;
 uint64  cx;
 uint64  sp;
 uint64  ip;
 uint64  flags;
 uint16  cs;
 uint16  gs;
 uint16  fs;
 uint16  ss;
 uint64  err;
 uint64  trapno;
 uint64  oldmask;
 uint64  cr2;
 // fpstate is really (struct _fpstate *) or (struct _xstate *) depending on the FP_XSTATE_MAGIC1 encoded in the SW reserved bytes of (struct _fpstate) and FP_XSTATE_MAGIC2 present at the end of extended memory layout. See comments at the definition of (struct _fpx_sw_bytes) 
 uint64  fpstate;          // Zero when no FPU/extended context 
 uint64  reserved1[8];
};

using MCONTEXT32 = sigcontext_x86_32;
using MCONTEXT64 = sigcontext_x86_64;
using MCONTEXT   = TSW<IsArchX64, MCONTEXT64, MCONTEXT32>::T;

#endif


// https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/signal.h
// https://github.com/torvalds/linux/blob/master/arch/arm/include/uapi/asm/signal.h
struct stack_t 
{
 vptr   ss_sp;
 uint32 ss_flags;
 size_t ss_size;
};

// https://github.com/torvalds/linux/blob/master/arch/arm/include/asm/ucontext.h
// https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/ucontext.h

template<typename TC> struct ucontext_t   // Generic (Inspection only - the size is approximate)
{
 size_t            uc_flags;
 ucontext_t*       uc_link;
 stack_t           uc_stack;
 TC                uc_mcontext;
 uint32            uc_sigmask[32];   // mask last for extensibility  // ARM
 alignas(8) size_t uc_regspace[128];                                 // ARM
};

using UCONTEXT32 = ucontext_t<MCONTEXT32>;
using UCONTEXT64 = ucontext_t<MCONTEXT64>;
using UCONTEXT   = TSW<IsArchX64, UCONTEXT64, UCONTEXT32>::T;

//=======================================================================================================

#elif defined(SYS_MACOS)

#if defined(CPU_ARM)
// https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/mach/arm/_structs.h

SCVR int ARM_NEON_REGS_32 = 16;
SCVR int ARM_NEON_REGS_64 = 32;

template<int RN> _STRUCT_ARM_NEON_STATE
{
 uint128 q[RN];
 uint32  fpsr;
 uint32  fpcr;
};

_STRUCT_ARM_EXCEPTION_STATE
{
 uint32 exception;  // number of arm exception taken
 uint32 fsr;        // Fault status 
 uint32 far;        // Virtual Fault Address 
};

// https://github.com/apple-oss-distributions/xnu/blob/main/bsd/arm/_mcontext.h
_STRUCT_ARM_EXCEPTION_STATE64
{
 uint64 far;        // Virtual Fault Address 
 uint32 esr;        // Exception syndrome 
 uint32 exception;  // number of arm exception taken 
};

_STRUCT_ARM_THREAD_STATE
{
 uint32 r[13];      // General purpose register r0-r12 
 uint32 sp;         // Stack pointer r13 
 uint32 lr;         // Link register r14 
 uint32 pc;         // Program counter r15 
 uint32 cpsr;       // Current program status register 
};

_STRUCT_ARM_THREAD_STATE64
{
 uint64 x[29];      // General purpose registers x0-x28 
 uint64 fp;         // Frame pointer x29 
 uint64 lr;         // Link register x30 
 uint64 sp;         // Stack pointer x31
 uint64 pc;         // Program counter 
 uint32 cpsr;       // Current program status register 
 uint32 __pad;      // Same size for 32-bit or 64-bit clients 
};   

_STRUCT_MCONTEXT_ARM32
{
 _STRUCT_ARM_EXCEPTION_STATE                 es;
 _STRUCT_ARM_THREAD_STATE                    ss;
 _STRUCT_ARM_NEON_STATE64<ARM_NEON_REGS_32>  ns;
};
             
_STRUCT_MCONTEXT_ARM64
{
 _STRUCT_ARM_EXCEPTION_STATE64               es;
 _STRUCT_ARM_THREAD_STATE64                  ss;
 _STRUCT_ARM_NEON_STATE64<ARM_NEON_REGS_64>  ns;
};

using MCONTEXT32 = _STRUCT_MCONTEXT_ARM32;
using MCONTEXT64 = _STRUCT_MCONTEXT_ARM64;
using MCONTEXT   = TSW<IsArchX64, MCONTEXT64, MCONTEXT32>::T;

#endif

// ---------------------- X86

#if defined(CPU_X86)
template<typename TA> _STRUCT_X86_EXCEPTION_STATE      // uint64/uint32
{
 uint16  trapno;
 uint16  cpu;
 uint32  err;
 TA      faultvaddr;
};

_STRUCT_X86_THREAD_STATE32
{
 uint32 eax;
 uint32 ebx;
 uint32 ecx;
 uint32 edx;
 uint32 edi;
 uint32 esi;
 uint32 ebp;
 uint32 esp;
 uint32 ss;
 uint32 eflags;
 uint32 eip;
 uint32 cs;
 uint32 ds;
 uint32 es;
 uint32 fs;
 uint32 gs;
};

_STRUCT_X86_THREAD_STATE64
{
 uint64  rax;
 uint64  rbx;
 uint64  rcx;
 uint64  rdx;
 uint64  rdi;
 uint64  rsi;
 uint64  rbp;
 uint64  rsp;
 uint64  r8;
 uint64  r9;
 uint64  r10;
 uint64  r11;
 uint64  r12;
 uint64  r13;
 uint64  r14;
 uint64  r15;
 uint64  rip;
 uint64  rflags;
 uint64  cs;
 uint64  fs;
 uint64  gs;
};

_STRUCT_MCONTEXT_X86_64
{
 _STRUCT_X86_EXCEPTION_STATE<uint64> es;
 _STRUCT_X86_THREAD_STATE64          ss;
// _STRUCT_X86_FLOAT_STATE64       fs;
};

_STRUCT_MCONTEXT_X86_32
{
 _STRUCT_X86_EXCEPTION_STATE<uint32> es;
 _STRUCT_X86_THREAD_STATE32          ss;
// _STRUCT_X86_FLOAT_STATE32       fs;
};

using MCONTEXT32 = _STRUCT_MCONTEXT_X86_32;
using MCONTEXT64 = _STRUCT_MCONTEXT_X86_64;
using MCONTEXT   = TSW<IsArchX64, MCONTEXT64, MCONTEXT32>::T;

#endif

// https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/_types/_ucontext.h
// https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/_types/_ucontext64.h
// https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/_types/_sigaltstack.h
_STRUCT_SIGALTSTACK
{
 vptr    ss_sp;          // signal stack base 
 size_t  ss_size;        // signal stack length         // __darwin_size_t
 uint32  ss_flags;       // SA_DISABLE and/or SA_ONSTACK 
};

template<typename TC> _STRUCT_UCONTEXT
{
 sint32                uc_onstack;
 uint32                uc_sigmask;     // signal mask used by this context     // __darwin_sigset_t
 _STRUCT_SIGALTSTACK   uc_stack;       // stack used by this context 
 _STRUCT_UCONTEXT*     uc_link;        // pointer to resuming context 
 size_t                uc_mcsize;      // size of the machine context passed in 
 TC*                   uc_mcontext;    // pointer to machine specific context     // _STRUCT_MCONTEXT/_STRUCT_MCONTEXT64
 // _STRUCT_MCONTEXT        __mcontext_data;   // #ifdef _XOPEN_SOURCE
};

using UCONTEXT32 = _STRUCT_UCONTEXT<MCONTEXT32>;
using UCONTEXT64 = _STRUCT_UCONTEXT<MCONTEXT64>;
using UCONTEXT   = TSW<IsArchX64, UCONTEXT64, UCONTEXT32>::T;

//=======================================================================================================

#elif defined(SYS_BSD)

struct stack_t
{
 vptr   ss_sp;      // signal stack base
 size_t ss_size;    // signal stack length 
 uint32 ss_flags;   // SS_DISABLE and/or SS_ONSTACK 
};

#if defined(CPU_ARM)

enum EBSDArmRegs
{
 _REG_RV     = 0,
#if defined(ARCH_X64)
 _REG_FP     = 29,
 _REG_LR     = 30,
 _REG_SP     = 31,
 _REG_ELR    = 32,
 _REG_PC     = _REG_ELR,
 _REG_SPSR   = 33,   
 _REG_TPIDR  = 34,    
#elif defined(ARCH_X32)
 _REG_FP     = 11,
 _REG_SP     = 13,
 _REG_LR     = 14,
 _REG_PC     = 15,
#endif
};

// https://github.com/freebsd/freebsd-src/blob/main/sys/arm/include/ucontext.h
struct  mcontext_fbsd_arm32
{
 uint32 gregs[17];
 // Originally, rest of this structure was named __fpu, 35 * 4 bytes long, never accessed from kernel. 
 size_t mc_vfp_size;
 vptr   mc_vfp_ptr;
 uint32 mc_spare[33];
};

struct  mcontext_fbsd_arm64 
{
struct gpregs {
  uint64  gp_x[30];  // 0 - 29
  uint64  gp_lr;     // 30
  uint64  gp_sp;     // 31
  uint64  gp_elr;    // 32  // IP
  uint64  gp_spsr;   // 33
} mc_gpregs;

struct fpregs {
  uint12  fp_q[32];
  uint32  p_sr;
  uint32  p_cr;
  uint32  p_flags;
  uint32  p_pad;
} mc_fpregs;

 uint32 mc_flags;
 uint32 mc_pad;      // Padding 
 uint64 mc_ptr;      // Address of extra_regs struct 
 uint64 mc_spare[7]; // Space for expansion, set to zero 
};

// https://github.com/NetBSD/src/blob/trunk/sys/arch/arm/include/mcontext.h
struct mcontext_nbsd_arm64
{       
 uint64 gregs[35];       // General Purpose Register set    // GR0-30, SP, PC, SPSR, TPIDR 
 //__fregset_t __fregs;  // FPU/SIMD Register File 
 //uint64    __spare[8]; // future proof 
};

struct mcontext_nbsd_arm32 
{ 
 uint32 gregs[17];
 //union {
 //    __fpregset_t __fpregs;
 //    __vfpregset_t __vfpregs;
 //} __fpu;
 //__greg_t    _mc_tlsbase;
 //__greg_t    _mc_user_tpid;
};

// https://github.com/openbsd/src/blob/master/sys/sys/signal.h

struct  mcontext_obsd_arm32   // sigcontext
{
 ssize  sc_cookie;
 uint32 sc_mask;    // signal mask to restore

 uint32 sc_spsr;
 uint32 sc_r0;
 uint32 sc_r1;
 uint32 sc_r2;
 uint32 sc_r3;
 uint32 sc_r4;
 uint32 sc_r5;
 uint32 sc_r6;
 uint32 sc_r7;
 uint32 sc_r8;
 uint32 sc_r9;
 uint32 sc_r10;
 uint32 sc_r11;
 uint32 sc_r12;
 uint32 sc_usr_sp;
 uint32 sc_usr_lr;
 uint32 sc_svc_lr;
 uint32 sc_pc;
 
 uint32 sc_fpused;
 uint32 sc_fpscr;
 uint64 sc_fpreg[32];   // unsigned long long
};

struct  mcontext_obsd_arm64    // sigcontext
{  
 uint32 __sc_unused;
 uint32 sc_mask;    // signal mask to restore 

 uint64 sc_sp;
 uint64 sc_lr;
 uint64 sc_elr;
 uint64 sc_spsr;
 uint64 sc_x[30];

 ssize  sc_cookie;
};

union MCONTEXT32 
{
 mcontext_fbsd_arm32  fbsd;
 mcontext_nbsd_arm32  nbsd;
 mcontext_obsd_arm32  obsd;
};
union MCONTEXT64 
{
 mcontext_fbsd_arm64  fbsd;
 mcontext_nbsd_arm64  nbsd;
 mcontext_obsd_arm64  obsd;
};

using MCONTEXT        = TSW<IsArchX64, MCONTEXT64, MCONTEXT32>::T;
using MCONTEXT_FBSD32 = mcontext_fbsd_arm32;
using MCONTEXT_NBSD32 = mcontext_nbsd_arm32;
using MCONTEXT_OBSD32 = mcontext_obsd_arm32;
using MCONTEXT_FBSD64 = mcontext_fbsd_arm64;
using MCONTEXT_NBSD64 = mcontext_nbsd_arm64;
using MCONTEXT_OBSD64 = mcontext_obsd_arm64;

using MCONTEXT_FBSD  = TSW<IsArchX64, mcontext_fbsd_arm64, mcontext_fbsd_arm32>::T;
using MCONTEXT_NBSD  = TSW<IsArchX64, mcontext_nbsd_arm64, mcontext_nbsd_arm32>::T;
using MCONTEXT_OBSD  = TSW<IsArchX64, mcontext_obsd_arm64, mcontext_obsd_arm32>::T;

#endif

#if defined(CPU_X86)

enum ENBSDRegsX86
{
 _REG_GS     0
 _REG_FS     1
 _REG_ES     2
 _REG_DS     3
 _REG_EDI    4
 _REG_ESI    5
 _REG_EBP    6
 _REG_ESP    7
 _REG_EBX    8
 _REG_EDX    9
 _REG_ECX    10
 _REG_EAX    11
 _REG_TRAPNO 12
 _REG_ERR    13
 _REG_EIP    14
 _REG_CS     15
 _REG_EFL    16
 _REG_UESP   17
 _REG_SS     18
}

// https://github.com/NetBSD/src/blob/trunk/sys/arch/i386/include/mcontext.h
struct mcontext_nbsd_x86_32
{
 uint32 gregs[19];
 //__fpregset_t    __fpregs;
 //__greg_t    _mc_tlsbase;
};

// https://github.com/NetBSD/src/blob/trunk/sys/arch/amd64/include/mcontext.h
typedef struct mcontext_nbsd_x86_64
{
 uint64 gregs[26];
// __greg_t _mc_tlsbase;
// __fpregset_t __fpregs;
};

// https://github.com/freebsd/freebsd-src/blob/main/sys/x86/include/ucontext.h
// The definition of mcontext_t must match the layout of struct sigcontext after the sc_mask member.  This is so that we can support sigcontext and ucontext_t at the same time.
struct mcontext_fbsd_x86_32 
{
 uint32 mc_onstack; // XXX - sigcontext compat. 
 uint32 mc_gs;      // machine state (struct trapframe) 
 uint32 mc_fs;
 uint32 mc_es;
 uint32 mc_ds;
 uint32 mc_edi;
 uint32 mc_esi;
 uint32 mc_ebp;
 uint32 mc_isp;
 uint32 mc_ebx;
 uint32 mc_edx;
 uint32 mc_ecx;
 uint32 mc_eax;
 uint32 mc_trapno;
 uint32 mc_err;
 uint32 mc_eip;
 uint32 mc_cs;
 uint32 mc_eflags;
 uint32 mc_esp;
 uint32 mc_ss;

 sint32 mc_len;         // sizeof(mcontext_t) 
 sint32 mc_fpformat;
 sint32 mc_ownedfp;
 uint32 mc_flags;

 // See <machine/npx.h> for the internals of mc_fpstate[].
 
 sint32 mc_fpstate[128] __aligned(16);

 uint32 mc_fsbase;
 uint32 mc_gsbase;

 uint32 mc_xfpustate;
 uint32 mc_xfpustate_len;

 sint32 mc_spare2[4];
};

struct mcontext_fbsd_x86_64 
{
 uint64  mc_onstack; // XXX - sigcontext compat. 
 uint64  mc_rdi;     // machine state (struct trapframe) 
 uint64  mc_rsi;
 uint64  mc_rdx;
 uint64  mc_rcx;
 uint64  mc_r8;
 uint64  mc_r9;
 uint64  mc_rax;
 uint64  mc_rbx;
 uint64  mc_rbp;
 uint64  mc_r10;
 uint64  mc_r11;
 uint64  mc_r12;
 uint64  mc_r13;
 uint64  mc_r14;
 uint64  mc_r15;
 uint32  mc_trapno;
 uint16  mc_fs;
 uint16  mc_gs;
 uint64  mc_addr;
 uint32  mc_flags;
 uint16  mc_es;
 uint16  mc_ds;
 uint64  mc_err;
 uint64  mc_rip;
 uint64  mc_cs;
 uint64  mc_rflags;
 uint64  mc_rsp;
 uint64  mc_ss;
         
 sint64  mc_len;         /* sizeof(mcontext_t) */
 sint64  mc_fpformat;
 sint64  mc_ownedfp;
 
  // See <machine/fpu.h> for the internals of mc_fpstate[].
 
 sint64  mc_fpstate[64] alignas(16);

 uint64  mc_fsbase;
 uint64  mc_gsbase;

 uint64  mc_xfpustate;
 uint64  mc_xfpustate_len;

 uint64  mc_tlsbase;

 sint64  mc_spare[3];
};

// https://github.com/openbsd/src/blob/master/sys/arch/i386/include/signal.h
struct  sigcontext_obsd_x86_x32 
{
 uint32 sc_gs;
 uint32 sc_fs;
 uint32 sc_es;
 uint32 sc_ds;
 uint32 sc_edi;
 uint32 sc_esi;
 uint32 sc_ebp;
 uint32 sc_ebx;
 uint32 sc_edx;
 uint32 sc_ecx;
 uint32 sc_eax;
 
 uint32 sc_eip;
 uint32 sc_cs;
 uint32 sc_eflags;
 uint32 sc_esp;
 uint32 sc_ss;

 ssize  sc_cookie;
 uint32 sc_mask;        // signal mask to restore 

 uint32 sc_trapno;      // XXX should be above 
 uint32 sc_err;

 union savefpu *sc_fpstate;
};


struct  sigcontext_obsd_x86_64     // plain match trapframe
{
 uint64 sc_rdi;
 uint64 sc_rsi;
 uint64 sc_rdx;
 uint64 sc_rcx;
 uint64 sc_r8;
 uint64 sc_r9;
 uint64 sc_r10;
 uint64 sc_r11;
 uint64 sc_r12;
 uint64 sc_r13;
 uint64 sc_r14;
 uint64 sc_r15;
 uint64 sc_rbp;
 uint64 sc_rbx;
 uint64 sc_rax;
 uint64 sc_gs;
 uint64 sc_fs;
 uint64 sc_es;
 uint64 sc_ds;
 uint64 sc_trapno;
 uint64 sc_err;
 uint64 sc_rip;
 uint64 sc_cs;
 uint64 sc_rflags;
 uint64 sc_rsp;
 uint64 sc_ss;

 struct fxsave64 *sc_fpstate;
 uint32 __sc_unused;
 uint32 sc_mask;
 ssize sc_cookie;
};


union MCONTEXT32 
{
 mcontext_fbsd_x86_32  fbsd;
 mcontext_nbsd_x86_32  nbsd;
 mcontext_obsd_x86_32  obsd;
};
union MCONTEXT64 
{
 mcontext_fbsd_x86_64  fbsd;
 mcontext_nbsd_x86_64  nbsd;
 mcontext_obsd_x86_64  obsd;
};

using MCONTEXT        = TSW<IsArchX64, MCONTEXT64, MCONTEXT32>::T;
using MCONTEXT_FBSD32 = mcontext_fbsd_x86_32;
using MCONTEXT_NBSD32 = mcontext_nbsd_x86_32;
using MCONTEXT_OBSD32 = mcontext_obsd_x86_32;
using MCONTEXT_FBSD64 = mcontext_fbsd_x86_64;
using MCONTEXT_NBSD64 = mcontext_nbsd_x86_64;
using MCONTEXT_OBSD64 = mcontext_obsd_x86_64;

using MCONTEXT_FBSD   = TSW<IsArchX64, mcontext_fbsd_x86_64, mcontext_fbsd_x86_32>::T;
using MCONTEXT_NBSD   = TSW<IsArchX64, mcontext_nbsd_x86_64, mcontext_nbsd_x86_32>::T;
using MCONTEXT_OBSD   = TSW<IsArchX64, mcontext_obsd_x86_64, mcontext_obsd_x86_32>::T;

#endif

using sigset_t = PX::sigset_new_t;       // 16 bytes

// https://github.com/freebsd/freebsd-src/blob/main/sys/sys/_ucontext.h
template<typename TC> struct ucontext_fbsd_t     // The first two fields match 'sigcontext'   // ARM
{
 sigset_t         uc_sigmask;
 TC               uc_mcontext;  // machine state   // NOTE: Fields after this may be misplaced
 ucontext_fbsd_t* uc_link;
 stack_t          uc_stack;
 uint32           uc_flags;
 uint32           __spare__[4];
};

// https://github.com/NetBSD/src/blob/trunk/sys/sys/ucontext.h
template<typename TC> struct ucontext_nbsd_t 
{
 uint32           uc_flags;     // properties 
 ucontext_nbsd_t* uc_link;      // context to resume 
 sigset_t         uc_sigmask;   // signals blocked in this context 
 stack_t          uc_stack;     // the stack used by this context
 TC               uc_mcontext;  // machine state       // NOTE: Fields after this may be misplaced
 ssize_t          __uc_pad[_UC_MACHINE_PAD];   // #if _UC_MACHINE_PAD
};

template<typename TC> struct ucontext_obsd_t
{
 TC uc_mcontext;      // Just mcontext
};

using UCONTEXT32 = _STRUCT_UCONTEXT<MCONTEXT32>;
using UCONTEXT64 = _STRUCT_UCONTEXT<MCONTEXT64>;

union UCONTEXT32
{
 ucontext_fbsd_t<MCONTEXT_FBSD32> fbsd;
 ucontext_nbsd_t<MCONTEXT_NBSD32> nbsd;
 ucontext_obsd_t<MCONTEXT_OBSD32> obsd;
}; 

union UCONTEXT64
{
 ucontext_fbsd_t<MCONTEXT_FBSD64> fbsd;
 ucontext_nbsd_t<MCONTEXT_NBSD64> nbsd;
 ucontext_obsd_t<MCONTEXT_OBSD64> obsd;
}; 

using UCONTEXT = TSW<IsArchX64, UCONTEXT64, UCONTEXT32>::T;

//=======================================================================================================
#else
#pragma message(">>> Unknown architecture")
#endif
//=======================================================================================================

struct SSigInfo
{
 sint32         signo;  // Signal number  // Linux: SIGSEGV, Windows: map from ExceptionCode
 sint32         code;   // Signal code (why it happened)  // SEGV_MAPERR or SEGV_ACCERR;  // Unmapped vs protection
 usize          maddr;  // Optional: memory address that was accessed                 // Expect it to be 0
 usize*         iaddr;  // Optional: instruction that faulted     // May be writable    // Expect it to be NULL
 PX::siginfo_t* info;   // Native info: Linux - siginfo_t, Windows: siginfo_t (emulated, minimalistic)    
 UCONTEXT*      ctx;    // Native CPU context     Windows: PEXCEPTION_RECORD        // Expect it to be NULL
};

// Returns true if the signal is completely handled ( No need for further handling (use after the context modifications to continue), we can continue )
using SigHandlerT = bool (PXCALL*)(SSigInfo* Info, vptr inst);   // User-supplied

static sint32 PXCALL HandleSignals(SigHandlerT Handler, vptr inst, uint64 Mask);   // Will restore default handling of unmarked signals   // TODO: Add to validator

// Must match PX::SignalAction (signal_action_t) except for the return value      // TODO: Add to the Validator ?
static bool PXCALL SignalActionHandler(sint32 sig, PX::siginfo_t* info, vptr context)    // Windows: Console/exception handler calls it
{
 if(!fwsinf.SigHandler) return false;
 NSIG::SSigInfo isig = {};
 isig.info  = info;
 isig.ctx   = (UCONTEXT*)context;
 isig.maddr = (usize)info->addr;   // May be same as IP
 isig.signo = info->signo;
 isig.code  = info->code;
                 
#if defined (SYS_WINDOWS)
 if(isig.ctx && isig.ctx->ContextRecord) isig.iaddr = &(isig.ctx->ContextRecord->Pc);
#endif

 return false;
}   


/*
void SignalHandler(int sig, siginfo_t* info, void* context) {
    ucontext_t* uc = (ucontext_t*)context;
    void* ip = nullptr;

#if defined(__APPLE__)
    // macOS uses __ss (thread state)
    ip = (void*)uc->uc_mcontext->__ss.__rip;
#elif defined(__linux__)
    // Linux uses gregs array
    ip = (void*)uc->uc_mcontext.gregs[REG_RIP];
#elif defined(__FreeBSD__)
    // FreeBSD uses mc_rip
    ip = (void*)uc->uc_mcontext.mc_rip;
#endif

    // Now you can pass 'ip' to your framework's crash reporter
}


*/

};
