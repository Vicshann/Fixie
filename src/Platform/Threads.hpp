
//============================================================================================================
// https://libc-alpha.sourceware.narkive.com/zVXSM4qu/using-clone-with-glibc
//
struct NTHD
{
enum EThDefs: sint    // Internal
{
 THD_MAX_MMGRS   = 4,   // Max number of memory managers per thread (UNUSED)
 //THD_MAX_USR_TLS = 32,
 THD_MAX_PAGELEN  = 4096,  // MEMPAGESIZE is not available here
 THD_MAX_THID     = 65536, // Max count, not index!
 THD_MAX_IDX_NUM  = THD_MAX_PAGELEN / sizeof(uint16),  // 2048
 THD_MAX_IDX_PAGE = THD_MAX_THID / THD_MAX_IDX_NUM,    // Total index pages (32)
 THD_MAX_IDX_RECS = ((THD_MAX_THID * sizeof(vptr)) / THD_MAX_PAGELEN),   // Number of pages for SThCtx pointers (allocated independently)
 THD_MAX_STATUS   = sint((size_t)1 << ((sizeof(size_t)*8)-1)),
};

// Linux: On 32-bit platforms, 32768 is the maximum value for pid_max. On 64-bit systems, pid_max can be set to any value up to 2^22 (4,194,304)
union SHDesc
{
SCVR int SizeInBits = sizeof(size_t) * 8;    // All Windows handles are aligned to 4 so shifted left by 2 to get more bits 
SCVR uint32 IdMsk = (1ull << ((SizeInBits/2) - 1)) - 1;  // 0x7FFFFFFF or 0x7FFF
 struct
  {       
   size_t PrHd : (SizeInBits / 2);
   size_t TrHd : (SizeInBits / 2);  // NOTE: Should be high part (Perfomance, and may be trimmed)   // WINDOWS: Is ID should be actual Id or index in the thread array? (Probably the array - need to access threads from other threads)
  };
 PX::pid_t Id;    // Negative is an Error code
};

// Must be two NULLs and a hack for Clang is required to force the compound literals to be built on stack instead of RDATA
#define TERMARGLST nullptr,STR_NULL        // (const achar*[]){"-Hello","-World","123",TERMARGLST}
#define TERMFLGLST 0,ssize(STR_NULL)       // (ssize[]){NPTM::NTHD::CfgPsCurDir, (ssize)"/tmp", TERMFLGLST}

// This thread context is allocated on stack, no separate TLS memory block is used
// Need some means to retrieve its pointer without conflicts with usual TLS mechanisms to allow libc coexist with the framework
// Cannot be allocated by a thread creation function because main(system) entry point will not have same behaviour
// The max value of pid is changeable, in default it is 32768, which is the size of the max value of short int.And, it can compatible with UNIX of early version. (expect 65536)
// NOTE: It is much more efficient to just pass this structure pointer around than request it each time from ThreadID which may require a syscall
//
struct SThCtx
{
 vptr   Self;        // For checks, mostly
 vptr*  SelfPtr;     // Points to this SThCtx record ptr
 vptr   TlsBase;
 size_t TlsSize;
 vptr   StkBase;     // For unmapping
 size_t StkSize;     // Can a thread unmap its own stack before calling 'exit'?
 size_t StkOffs;     // Initial stack offset
 uint   GroupID;     // Can be changed with setpgid
 uint   ThreadID;    // May be equal ProcesssID?
 uint   ProcesssID;
 uint   LastThrdID;  // Of prev thead that owned this memory (ThreadID is set to 0 already)
 uint   LastThrdHnd;
 uint   ThreadHndl;  // Used on Windows instead of ThreadID in all thread_* functions    // MacOS?
 vptr   ThreadProc;
 vptr   ThreadData;
 size_t ThDataSize;
 sint   ExitCode;    // Only if exiting by thread_exit // Have to avoid that mess with 'wait' and children fixation (More than int8 of return value size as a bonus)
 //uint   EntryCtr;   // Need some way to detect dead threads and reuse them?
 uint   Flags;
 vptr   MMPtrs[THD_MAX_MMGRS];      // For thread local memory managers (mempool)
};

//struct SThPage   // For each 32768 threads id
//{
// SThCtx* ThRecs[THD_MAX_PAGELEN / sizeof(SThCtx*)];    // 32768/8 = 4096 (One page, usually)    // For now this memory is not released (Reallocated if a new thread with same ID requires larger stack)
//};

/*struct SThRec    //  SThRec Recs[THD_MAX_PAGELEN / sizeof(SThRec)];
{
 size_t MagicPtrA;  // Prev^Next   // Sorted by ID
 size_t MagicPtrB;  // Prev^Next   // Sorted by Stack base
 size_t RecPtr;
}; */

// https://nullprogram.com/blog/2015/05/15/

// X32: 4096 / 4 = 1024 (IDs per page);  65536 / 1024 = 64 (Page PTRs) * 4 = 256  (bytes for global thread struct)
// X64: 4096 / 8 = 512  (IDs per page);  65536 / 512 = 128 (Page PTRs) * 8 = 1024 (bytes for global thread struct)
// It could be possible to find current thread id by checking its StackFrame against stack ranges of all threads
// Have to make this structure thread safe without any sync-lock
// Showld be fast enough for a small amount of threads. And anything with large amounts of threads should cache this pointers somewhere
// Is it possible to extract ibfo from a stack frame pointer to use it in logging for indentation of function names?
// https://kernelnewbies.kernelnewbies.narkive.com/9Zd9eWeb/waitpid-2-and-clone-thread
// WARNING: BAD DESIGN!!!
//
struct SThInf  // Allocate dynamically?  // Wastes 1024 bytes on X64 in a single threaded app   // Not used for main thread!  // First pointer points to this block and a constant added to idx when accessing to start from Recs field
{
 static constexpr uint RecsOnPage = (THD_MAX_PAGELEN / sizeof(size_t))-1;
// static inline uint TotalRecs  = 0;   // No deallocation
// static inline uint TotalPages = 0;

// uint16** Indexes[THD_MAX_IDX_PAGE];     // Each record corresponds to a tid and contains index of a thread rec    // Usually just one page is wasted
// SThCtx** Pointers[THD_MAX_IDX_RECS];    // Each points to one page of THD_MAX_PAGELEN of SThCtx pointers    // Allocated by number of threads, not by tid   // Usually just one page is wasted    // SThPage* Pages[THD_MAX_THID / THD_MAX_PAGELEN];  // 65536 / 4096 (PageSize)     // idx = gettid() / 4096      // These pointers are allocated on demand
// SThCtx*  Recs[0];  // Rest of the page  // Max Threads will be less than 65536
 SThInf* NextPage;
 //size_t  Total;
 size_t  Recs[RecsOnPage];   // Low bit 1 means the record is unused
//------------------------------------------------------------------------------------------------------------
/*static _finline auto TidToIdx(uint16 tid)
{
 struct SR{uint a; uint b;};
 uint PIdx = tid >> 11;     // / 2048  (0-31)
 uint RIdx = tid & 0x7FF;   // 2047
 return SR{PIdx, RIdx};
}
//------------------------------------------------------------------------------------------------------------
static _finline auto IdxToPtr(uint16 idx)
{
 struct SR{uint a; uint b;};
 uint PIdx = tid >> 11;
 uint RIdx = tid & 0x7FF;
 return SR{PIdx, RIdx};
}  */
//------------------------------------------------------------------------------------------------------------
/*static int InitPages(SThInf** FromFirst, vptr PageBuf, uint BufLen)
{
 uint8* CurSrcPtr  = (uint8*)PageBuf;
 uint8* EndSrcPtr  = (uint8*)(((size_t)CurSrcPtr + BufLen) & size_t(~(MEMPAGESIZE-1)));
 SThInf* CurInfPtr = (SThInf*)CurSrcPtr;
 *FromFirst = CurInfPtr;
 CurSrcPtr += MEMPAGESIZE;
 int PCtr   = 1;
 for(;CurSrcPtr < EndSrcPtr;PCtr++,CurSrcPtr += MEMPAGESIZE)
  {
   CurInfPtr->NextPage = (SThInf*)CurSrcPtr;  
   CurInfPtr = CurInfPtr->NextPage;
  }
 return PCtr;
}  */
//------------------------------------------------------------------------------------------------------------
// A record is in use if its bit 0 is set
//
//
SThCtx** FindOldThreadByTID(size_t tid)       //  !!! TODO: Make everything to use packed IDs to use indexes directly (but keep ability to accept normal IDs)
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs    // Free recs are either 1 or a pointer
     SThCtx* ptr = (SThCtx*)(val & (size_t)~1);
     if(ptr->LastThrdID == tid)return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** FindOldThreadByHandle(size_t hnd)
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs    // Free recs are either 1 or a pointer
     SThCtx* ptr = (SThCtx*)(val & (size_t)~1);
     if(ptr->LastThrdHnd == hnd)return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** FindThByTID(size_t tid, bool ActiveOnly=true)
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs    // Free recs are either 1 or a pointer
     if(!(val & 1) && ActiveOnly)continue;  // Inactive
     SThCtx* ptr = (SThCtx*)(val & (size_t)~1);
     if(ptr->ThreadID == tid)return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** FindThByHandle(size_t hnd, bool ActiveOnly=true)
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs    // Free recs are either 1 or a pointer
     if(!(val & 1) && ActiveOnly)continue;  // Inactive
     SThCtx* ptr = (SThCtx*)(val & (size_t)~1);
     if(ptr->ThreadHndl == hnd)return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** FindThByStack(vptr ptr, bool ActiveOnly=true)     // Any address on the thread`s stack
{
 SThInf* ThisPage  = this;
// uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs
     if(!(val & 1) && ActiveOnly)continue;  // Inactive
     SThCtx* scptr = (SThCtx*)(val & (size_t)~1);
     if(((uint8*)ptr >= (uint8*)(scptr->StkBase)) && ((uint8*)ptr < ((uint8*)(scptr->StkBase) + scptr->StkSize)))return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** GetUnusedRec(SThInf*** NeedPage)     // Any address on the thread`s stack
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)   // Lets hope it have no problems with sync
    {
     size_t val = ThisPage->Recs[idx];     
     if(!(val & 1))      // TODO: InterlockedAnd  (__sync_val_compare_and_swap (type *ptr, type oldval type newval, ...))   //  if the current value of *ptr is oldval, then write newval into *ptr.
      {
       ThisPage->Recs[idx] = val | 1;    // TODO: Interlocked exch     // How to use 'EntryCtr' without locking?  // Need another bit?
       return (SThCtx**)&ThisPage->Recs[idx];
      }
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
//   TotalRecs = ThisPage->Total;
  }
 *NeedPage = &ThisPage->NextPage;
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** SetNewPageAndGetRec(vptr Page, SThInf** Place)   // NOTE: Rec will be a Null pointer
{
// SThInf* ThisPage  = this;
 SThInf* NewPB = (SThInf*)Page;
 *NewPB->Recs |= 1;   // Set furst rec as used BEFORE the page is added to the list
 for(;;)   // Compare exchange if zero     // Compare exch on a null ptr
  {
   if(!*Place)
    {
     *Place = (SThInf*)Page;
     break;
    }
   *Place = (*Place)->NextPage;
  }
 return (SThCtx**)&NewPB->Recs;
}
//------------------------------------------------------------------------------------------------------------

};

//------------------------------------------------------------------------------------------------------------
static _finline SThCtx* ReadRecPtr(SThCtx** ptr)
{
 return (SThCtx*)(size_t(*ptr) & (size_t)~0x0F);
}
//------------------------------------------------------------------------------------------------------------
static _finline void WriteRecPtr(SThCtx** ptr, SThCtx* rec)   // As busy
{
 *ptr = (SThCtx*)((size_t)rec | 1);
}
//------------------------------------------------------------------------------------------------------------
static _finline void OccupyRec(SThCtx** rec)
{
 *(size_t*)rec |= 1;   // TODO: InterlockedOr  ???
}
//------------------------------------------------------------------------------------------------------------
static _finline void ReleaseRec(SThCtx** rec)
{
 *(size_t*)rec &= (size_t)~1;   // TODO: InterlockedOr  ???
}
//------------------------------------------------------------------------------------------------------------
struct STDesc
{
 NTHD::SThCtx  MainTh;      // Main(Init/Entry) thread // A thread from which the framework is initialized at main entry point for a module/app (For modules this is NOT the app`s process main thread)
 NTHD::SThInf* ThreadInfo;  // For additional threads (Null if only an entry thread is used)
};

using PThreadProc = ssize_t (*)(SThCtx*);	

// format: {ACTIONID1,OPTVAL1,...OPTVALN,ACTIONID2,OPTVAL,ACTIONID3,-1}
enum EPrThCfg           // May be applicable to process creation too
{
 CfgTermVal      = 0,
 CfgManaged      = (0 << 16) | 1,     // The thread/process is managed by the OS (CSRSS on windows, pthreads on Linux [no stack allocation control])  // TODO
 CfgFlagsDef     = (1 << 16) | 2,     // Linux specific flags for 'clone'
 CfgFlagsMsk     = (1 << 16) | 3,     // Linux specific mask to exclude default 'clone' flags   // Flags to mask out
 CfgSuspended    = (0 << 16) | 4,     // Can be done on Linux?
 CfgThStkSize    = (1 << 16) | 5,     // A Value follows
 CfgThTlsSize    = (1 << 16) | 6,     // A Value follows
 CfgPsCurDir     = (1 << 16) | 7,     // Set Curren Directory for the new process
 CfgPsFDShare    = (2 << 16) | 8,     // A pair follows {oldfd,newfd}  // On Windows maps only STDIN, STDOUT, STDERR to PEB anything else is just duplicated
 CfgSetTgtCurDir = (0 << 16) | 9,     // Set the current directory as the target application path (Takes priority over CfgPsCurDir)
 CfgNoPathInArg0 = (0 << 16) | 10,    // Do not add the executable path as Arg0 (i.e. you already passed some path there)
}; 

// TODO: On windows use system allocated thread contexts (TEB)
// ??? Who and where disposes of those threads? Especialy ones that died without reaching the normal exit point. Zombies?
static PX::pid_t  PXCALL thread(PThreadProc Proc, PX::PVOID Data, PX::SIZE_T DatSize, PX::PSSIZE_T Config);     // Improvised  // Config is pairs of {Type,Value} or single {Flag}  // Actually allocates: PageAlign(Size+StkSize+TlsSize+ThreadRec)
static sint       PXCALL thread_sleep(uint64 us);                     // nleepns??? // Prefer this over nanosleep in managed threads  // -1 - wait infinitely  // Sleeps on its futex	 // -1 sleep until termination
static sint       PXCALL thread_exit(sint status);                    // Allows to exit without returning from ThreadProc directly	  // Deallocate the thread`s stack memory?
static sint       PXCALL thread_status(PX::pid_t tid);	              // Use after thread_wait if the thread has exited
static sint       PXCALL thread_kill(PX::pid_t tid, sint status);     // Terminates the thread
static sint       PXCALL thread_wait(PX::pid_t tid, uint32 wait_ms, sint* status);      // Wait for the a thread termination  // -1 - wait infinitely   // Optionally returns exit code if the thread has exited
static sint       PXCALL thread_signal(PX::pid_t tid, uint32 code);   // Wakes the thread from any wait	(thread_sleep)	// Like pthread_cancel	 // Cancel IO?  
static sint       PXCALL thread_priority_set(PX::pid_t tid, sint32 level);              // The level is signed
static sint       PXCALL thread_priority_get(PX::pid_t tid, sint32* level);
static sint       PXCALL thread_affinity_set(PX::pid_t tid, uint64 mask, uint32 from); 
static sint       PXCALL thread_affinity_get(PX::pid_t tid, size_t buf_len, PX::PSIZE_T buf); 

// https://man7.org/linux/man-pages/man3/posix_spawn.3.html
// https://totozhang.github.io/2016-01-16-linux-zombieprocess/
// Spawn a new process
// Current working directory is inherited (Same as fork/vfork)
// NOTE: A child process being a zombie means it finished/died and you need to reap its exit status with wait/waitpid.
// See posix_spawn for actions that may be required
// NOTE: Pass two terminating NULLs at the end of arrays for cross platform compatibility
// Why GCC fails with DoTest("test", (const char*[]){"TestE1","TestE2","TestE3"})  // error: expected primary-expression before ')' token     // Compound literals are a C99 feature
// Why compound literals are always have some kind of global placement and can't be constexpr?
// 'int* zx = (int []) {1, 2, 3, 4};' the array will always be in global memory. Why? C++ does not allow hidden stack object allocations?
//
static PX::pid_t  PXCALL spawn(PX::PCCHAR path, PX::PPCHAR argv, PX::PPCHAR envp, PX::PSSIZE_T Config);    // Improvised  // envp may be NULL to reuse the current one // Flags are additional flags for Clone, unused for now
static sint       PXCALL spawn_signal(PX::pid_t pid, uint32 code);
static sint       PXCALL spawn_wait(PX::pid_t pid, uint32 wait_ms, sint* status);
// Add 'priority' and 'affinity' functions?

// CPU affinity
// Suspend/Resume
// Terminate
// Exit?   (Not exit_group which terminates the entire app)

/*
 For now, no stack memory is reused between threads with different IDs
 Normally exiting threads will free their memory
 Thred pools should not exit their threads anyway.
 And any ended and immediately created threads will likely to have same IDs


       There are various ways of determining the number of CPUs
       available on the system, including: inspecting the contents of
       /proc/cpuinfo; using sysconf(3) to obtain the values of the
       _SC_NPROCESSORS_CONF and _SC_NPROCESSORS_ONLN parameters; and
       inspecting the list of CPU directories under
       /sys/devices/system/cpu/
*/
//------------------------------------------------------------------------------------------------------------
// No Constructor/destructor support as with ordinary globals. (Threads shouls exit to their initializer proc if they use TLS)
// Perfomance note: The current implementation of getting a thread's context is SLOW!. Try to access TLS variables only at the root of your code and pass further as aguments, if needed. NEVER access them in loops!
// libgcc/emutls.c
// https://stackoverflow.com/questions/64599080/making-a-cloned-thread-pthread-compatible
// Will calls to libc from our threads fail because we do not use pthreads to create and init them?    // TODO: Test it
// It is assumed that the linker will gather all 'static __thread' declared variables across all sources/static libs 
//   so we will have a complete array of __emutls_object pointers and can calculate total required TLS memory at a thread's initialization function.
//   TLS should not be used across dynamic libraries.
// 
// 
// https://maskray.me/blog/2021-02-14-all-about-thread-local-storage
//__declspec(thread)   __thread    // __thread int tls0;
//gnu::tls_model
// -mtls-dialect=desc
// -femulated-tls
// -ftls-model=  localexec  For variables defined in the executable and only used within it.
// "anyregcc" - Dynamic calling convention for code patching
// Each thread-local variable definition is associated with a __emutls_control instance which records size/alignment/index/initializer.
// Each thread has an object array (emutls_address_array), with each element being a pointer to the variable value. Each thread-local variable is assigned an index into the array.
// FS: arch_prctl(2).
// If you are using a x86-64 bit, you should use exclusively arch_prctl to set the FS register to an area of memory that you want to use as TLS (it allows you to address memory areas bigger than 4GB). 
// While for x86-32 you must use set_thread_area as it is the only system call supported by the kernel.

struct __emutls_object
{
 uint16 size;          // word
 uint16 align;         // word
 union {
   size_t offset;      // pointer
   void   *ptr;
 } loc;
 void* templ;
};

struct __emutls_array
{
 size_t skip_destructor_rounds;  // pointer
 size_t size;           // pointer
 void** data[];
};

// LIBC pthreads thread context?

//void *__emutls_get_address (struct __emutls_object *);
//void __emutls_register_common (struct __emutls_object *, word, word, void *);
//static void* __emutls_get_address(struct __emutls_object* obj) asm("__emutls_get_address");       // Not working
static vptr __emutls_get_address(vptr ptr)     // Must be made public because can't export it from here and have to use a global wrapper function
{
 __emutls_object* tlsobj = (__emutls_object*)ptr;
 // TODO TODO TODO
 return nullptr;
}

};
//============================================================================================================
static NTHD::SThCtx* GetThreadSelf(void)     // Probably can find it faster by scanning stack pages forward and testing for SThCtx at beginning
{
 return GetThreadByAddr(GETSTKFRAME());
}
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* GetThreadByID(uint id)
{
 NTHD::STDesc* ThDsc = NPTM::GetThDesc();
 if((id == (uint)-1)||(id == ThDsc->MainTh.ThreadID))return &ThDsc->MainTh;
 if(!ThDsc->ThreadInfo)return nullptr; // No more threads
 NTHD::SThCtx** ptr = ThDsc->ThreadInfo->FindThByTID(id);
 if(!ptr)return nullptr;
 return NTHD::ReadRecPtr(ptr);
}
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* GetThreadByHandle(uint hnd)
{
 NTHD::STDesc* ThDsc = NPTM::GetThDesc();
 if((hnd == (uint)-1)||(hnd == ThDsc->MainTh.ThreadHndl))return &ThDsc->MainTh;
 if(!ThDsc->ThreadInfo)return nullptr; // No more threads
 NTHD::SThCtx** ptr = ThDsc->ThreadInfo->FindThByHandle(hnd);
 if(!ptr)return nullptr;
 return NTHD::ReadRecPtr(ptr);
}
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* GetThreadByAddr(vptr addr)   // By an address on stack
{
 NTHD::STDesc* ThDsc = NPTM::GetThDesc();
 if(((uint8*)addr >= (uint8*)ThDsc->MainTh.StkBase)&&((uint8*)addr < ((uint8*)ThDsc->MainTh.StkBase + ThDsc->MainTh.StkSize)))return &ThDsc->MainTh;
 if(!ThDsc->ThreadInfo)return nullptr; // No more threads
 NTHD::SThCtx** ptr = ThDsc->ThreadInfo->FindThByStack(addr);
 if(!ptr)return nullptr;
 return NTHD::ReadRecPtr(ptr);
}
//------------------------------------------------------------------------------------------------------------
/*static NTHD::SThCtx* GetNextThread(NTHD::SThCtx* th) // Get thread by index?   // Start from NULL    // Need to think
{
 return nullptr;
}*/
//------------------------------------------------------------------------------------------------------------

// What about kernel threads?
private:
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* InitThreadRec(vptr ThProc, vptr ThData, size_t StkSize, size_t TlsSize, size_t DatSize, size_t** StkFrame)
{
 DatSize = AlignFrwdP2(DatSize, 16);
 if(StkSize)StkSize = AlignFrwdP2(StkSize, MEMPAGESIZE);   // NOTE: As StkSize is aligned to a page size, there will be at least one page wasted for ThreadContext struct (Assume it always available for some thread local data?)
   else StkSize = 0x10000;  // 64K should be optimal
 TlsSize = AlignFrwdP2(TlsSize, 16);   // Slots is at least of pointer size
 size_t FStkLen = AlignFrwdP2(DatSize+StkSize+TlsSize+sizeof(NTHD::SThCtx), MEMGRANSIZE);     // NOTE: MEMGRANSIZE may be more than MEMPAGESIZE (On windows)

// Find/alloc a new thread rec                  / TODO: Init several pages if available
 uint8* StkPtr = nullptr;
 NTHD::STDesc* ThDsc = NPTM::GetThDesc();
 if(!ThDsc->ThreadInfo)    // Alloc first thread list page
  {
   DBGMSG("Allocating first thread list page");
#ifdef SYS_WINDOWS
   vptr NewPage = NPTM::NAPI::mmap(nullptr, MEMPAGESIZE, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);  // Actually reserves the entire 64K block  // Must be a separate allocation on Windows - cannot unmap partially
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   ThDsc->ThreadInfo = (NTHD::SThInf*)NewPage;
   NewPage = NPTM::NAPI::mmap(nullptr, FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0); 
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   StkPtr  = (uint8*)NewPage;
#else
   vptr NewPage = NPTM::NAPI::mmap(nullptr, MEMPAGESIZE+FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);  // Allocate together with a new rec stack  
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   ThDsc->ThreadInfo = (NTHD::SThInf*)NewPage;
   StkPtr = ((uint8*)NewPage + MEMPAGESIZE);
#endif
  }
 NTHD::SThInf** PNewPagePtr = nullptr;
 NTHD::SThCtx** PRecPtr     = ThDsc->ThreadInfo->GetUnusedRec(&PNewPagePtr);
 if(!PRecPtr)         // NOTE: On Windows entire 64K is reserved and additional 4k pages are allocated from that
  {
   DBGMSG("Allocating another thread list page");
#ifdef SYS_WINDOWS
   vptr Addr = vptr(AlignBkwdP2((size_t)PNewPagePtr, MEMPAGESIZE) + MEMPAGESIZE);  // Next page in the reserved 64K block  // PNewPagePtr is in the last page
#else
   vptr Addr = nullptr;
#endif
   vptr NewPage = NPTM::NAPI::mmap(Addr, MEMPAGESIZE, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);    // Allocate together with a new stack area stack
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   StkPtr  = ((uint8*)NewPage + MEMPAGESIZE);
   PRecPtr = ThDsc->ThreadInfo->SetNewPageAndGetRec(NewPage, PNewPagePtr);
  }
 NTHD::SThCtx* ThRec = NTHD::ReadRecPtr(PRecPtr);   // Gets the pointer whichis either NULL or points to an already allocated stack block with a context in it
 uint OldID   = -1;
 uint OldHnd  = -1;
 sint OldStat = NTHD::THD_MAX_STATUS;  // Reset (If this code stays after a thread exits - the exit was not normal)
 if(ThRec)   // Already allocated
  {
   DBGMSG("Reusing the thread rec: %p",ThRec);
   OldID   = ThRec->LastThrdID;
   OldHnd  = ThRec->LastThrdHnd;
   OldStat = ThRec->ExitCode;    // Preserve last thread info
   if(ThRec->StkSize < FStkLen)
    {
     StkPtr = (uint8*)NPTM::NAPI::mmap(nullptr, FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   // TODO: mrealloc
     if(uint err=MMERR(StkPtr);err)return (NTHD::SThCtx*)err;
     NPTM::NAPI::munmap(ThRec->StkBase, ThRec->StkSize);   // Unmap old stack
    }
    else
     {
      StkPtr  = (uint8*)ThRec->StkBase;
      FStkLen = ThRec->StkSize;
     }
  }
  else if(!StkPtr)StkPtr = (uint8*)NPTM::NAPI::mmap(nullptr, FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);  // May be already allocated with a thread rec page
 if(uint err=MMERR(StkPtr);err)return (NTHD::SThCtx*)err;

 NTHD::SThCtx* ThrFrame = (NTHD::SThCtx*)&StkPtr[StkSize];  // Since StkSize is page-aligned, ThrFrame is also page aligned. It is possible to find it by scanning the stack forward by pages from any addr on that stack. Is it faster than scanning thread list?  (Need to place main thread`s ctx on stack too)
 vptr DataPtr = &StkPtr[StkSize+sizeof(NTHD::SThCtx)];
 vptr TlsPtr  = &StkPtr[StkSize+DatSize+sizeof(NTHD::SThCtx)];
 *StkFrame    = (size_t*)&StkPtr[StkSize];      // Decreasing stack pointer only!      // NOTE: Keep the stack aligned to 16
 if(ThData && DatSize)memcpy(DataPtr, ThData, DatSize);    // User data is right at the bottom

 ThrFrame->Self        = ThrFrame;   // For checks                           // TODO: Add thread record index so that it can be used for faster lookup
 ThrFrame->SelfPtr     = (vptr*)PRecPtr;    // Need thread id to init  (Assigned in STC::ThProcCall)
 ThrFrame->TlsBase     = TlsPtr;
 ThrFrame->TlsSize     = TlsSize;
 ThrFrame->StkBase     = StkPtr;     // For unmapping
 ThrFrame->StkSize     = FStkLen;    // StkSize; ??? // Need full size for unmap  // Can a thread unmap its own stack before calling 'exit'?
 ThrFrame->StkOffs     = StkSize;
 ThrFrame->GroupID     = NAPI::getpgrp();   // pid
 ThrFrame->ThreadID    = 0;  // Will be written to by 'clone'  // And reset at its termination by the system
 ThrFrame->ProcesssID  = NAPI::getpid();
 ThrFrame->LastThrdID  = OldID;
 ThrFrame->LastThrdHnd = OldHnd;
 ThrFrame->ThreadHndl  = 0;    // Set by system (Windows)
 ThrFrame->ThreadProc  = (vptr)ThProc;
 ThrFrame->ThreadData  = DataPtr;
 ThrFrame->ThDataSize  = DatSize;
 ThrFrame->ExitCode    = OldStat;
 //ThrFrame->EntryCtr    = 0;   // Unentered  // Later, any entered thread with zero TID will be considered dead and for reuse
 ThrFrame->Flags       = 0;   
 NTHD::WriteRecPtr(PRecPtr, ThrFrame);   // Update the pointer
 return ThrFrame;
}
//------------------------------------------------------------------------------------------------------------
_noret static void _ninline 
#ifdef SYS_WINDOWS
_scall ThProcCallStub(NT::PVOID Data, NT::SIZE_T Size)       // Static, no inlining, args in registers   // TODO: Register it with Control Flow Guard somehow or NtCreateThread will fail in CFG enabled processes
{
 NTHD::SThCtx* ThrFrame = (NTHD::SThCtx*)Data;   // Should be same ptr or something is pushed
#else
_fcall ThProcCallStub(void)       // Static, no inlining, args in registers
{
 NTHD::SThCtx* ThrFrame = (NTHD::SThCtx*)AlignFrwdP2((size_t)GETSTKFRAME(), 16);   // Should be same ptr or something is pushed
#endif
 DBGMSG("ThrFrame=%p, GroupID=%i, ProcesssID=%i, ThreadID=%i: %p",ThrFrame,ThrFrame->GroupID,ThrFrame->ProcesssID,ThrFrame->ThreadID,GetThreadByID(ThrFrame->ThreadID));
 sint res = PXERR(EFAULT);
 if(ThrFrame == ThrFrame->Self)
  {
//     ThrFrame->EntryCtr++;    // Entered, TID is not zero
   res = ((NTHD::PThreadProc)ThrFrame->ThreadProc)(ThrFrame);
   ThrFrame->LastThrdID  = ThrFrame->ThreadID;               // Is it OK that the exit point will belong to this module? May be it will be reqiured to create a thread in another module and unload this later (hot reloading?)
   ThrFrame->LastThrdHnd = ThrFrame->ThreadHndl;    
   ThrFrame->ExitCode    = res;
#ifdef SYS_WINDOWS
   NT::HANDLE hndl = ThrFrame->ThreadHndl;  // TODO: Memory barrier
   ThrFrame->ThreadHndl  = 0;  // The system will not clear this for us  // Windows will not clear ThreadID either!
   ThrFrame->ThreadID    = 0;
   SAPI::NtClose(hndl);        // If we close this handle before TerminateThread then anyone who waits on the handle will get ABORT notification. GetExitCodeThread will not be returning the valid exit code yet but we do not use that anyway 
#endif
   NTHD::ReleaseRec((NTHD::SThCtx**)(ThrFrame->SelfPtr));     // TODO: Remove from mem rec. For now: Keep the stack memory to be reused by another new thread (Cannot deallocate stack without ASM, the compiler won`t store 'res' in a register and will touch the stack for some other useless reasons anyway)
  }
 NAPI::exit(res);  // Any ABI preserved registers are not important at this point  ThProc(nullptr,0)
} 
//------------------------------------------------------------------------------------------------------------
public:
/*   https://marcusgrass.github.io/threads
            // We need to be able to unmap the thread's own stack, we can't use the stack anymore after that
            // so it needs to be done in asm.
            // With the stack_ptr and stack_len in rdi/x0 and rsi/x1, respectively we can call mmap then
            // exit the thread
            #[cfg(target_arch = "x86_64")]
            core::arch::asm!(
            // Call munmap, all args are provided in this macro call.
            "syscall",
            // Zero eax from munmap ret value
            "xor eax, eax",
            // Move exit into ax
            "mov al, 60",
            // Exit code 0 from thread.
            "mov rdi, 0",
            // Call exit, no return
            "syscall",
            in("rax") MUNMAP,
            in("rdi") map_ptr,
            in("rsi") map_len,
            options(nostack, noreturn)
            );
            #[cfg(target_arch = "aarch64")]
            core::arch::asm!(
            // Make munmap syscall, unmap stack
            "svc #0",
            // Move exit code 0 into arg 1 register
            "mov x0, 0",
            // Move exit syscall nr into syscall nr register
            "mov x8, #93",
            // Make exit syscall, thread is done
            "svc #0",
            in("x8") MUNMAP,
            in("x0") map_ptr,
            in("x1") map_len,
            options(nostack, noreturn)
*/