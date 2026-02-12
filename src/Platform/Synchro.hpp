
/*
Windows:
  https://dennisbabkin.com/blog/?t=how-to-put-thread-into-kernel-wait-and-to-wake-it-by-thread-id


 https://docs.omniverse.nvidia.com/kit/docs/carbonite/167.3/api/program_listing_file_carb_thread_FutexImpl.h.html
 https://lwn.net/Articles/823579/
 https://softwareengineering.stackexchange.com/questions/340284/mutex-vs-semaphore-how-to-implement-them-not-in-terms-of-the-other
*/

//struct SYNC    // Futex-like/Sync-primitive implementation (All platforms)   // Let it be in NPTM directly
//{

// Windows can wait with timeout on: Event, Mutex, Semaphore, ConditionVariable
//  So should we support that (complicated, should not continue waiting with the same timeout, should subtract elapsed time )
/*
enum EUtxFlags: uint32
{
 ufWakeHaveTimeout = 0x01,   // Infinite otherwise  // With timeout, (-1) will also mean infinite  // Can it be done on Linux (FUTEX_WAKE) and on Windows (NtAlertThreadByThreadId) ? // Swap Wait() and Wake() behaviour if there is no waiters?
 ufWaitHaveTimeout = 0x02,   // Infinite otherwise  // NOTE: If wait was interrupted by something unimportant, you must subtract elapsed time before continuing to wait
 ufWaitCheckVar    = 0x04,   // Check the 'expected' variable on wait (Works differently on Linux and Windows)  // Stores 'DT' type, makes the object larger  // If ufWaitCheckVar is set, Wait always loops on the value, regardless of platform.
 ufWaitMultiple    = 0x08,   // Wait on multiple objects (complicated on Linux) (Windows?)
 ufWakeMultiple    = 0x10,   // Should be able to wake multiple waiters
 ufHaveCounter     = 0x20,   // Have a int32 counter (unsigned?) May check it with ufWaitCheckVar
 ufHaveFlag        = 0x40,   // Have a single flag (may find space for it in an existing value/pointer)
 ufAlertable       = 0x80,   // A signal or alert may interrupt the Wait

 // Common combinations  (Timeouts where? Wait/Wake?)
 ufSemaphore       = ufWaitCheckVar | ufWakeMultiple,  // Semaphore: check + wake N
 ufCondVar         = ufWaitCheckVar | ufWakeMultiple,  // CondVar: check + wake all
 ufEvent           = ufWakeMultiple,                   // Event: just wake all (no value check needed)
 ufMutex           = ufWaitCheckVar,                   // Simple mutex: just wait with value check
};  */
//------------------------------------------------------------------------------------------------------------
// Base for all synch primitives. Supposed to be heavily compile-time composed
// DT is a variably type for
// On Linux, FUTEX_WAIT can be interrupted by a signal
// NOTE: It is bucketed on Win7+ ((Addr >> 5) & 0x3F) into multiple linked lists. WinXP uses one linked list.
//       With a per-process keyed event object NtReleaseKeyedEvent will still have to walk the linked list. (Never use the global '\KernelObjects\CritSecOutOfMemoryEvent')
//       We use NtWaitForAlertByThreadId and NtAlertThreadByThreadId. WaitOnAddress uses those with a lock-free hash table.   // https://devblogs.microsoft.com/oldnewthing/20160826-00/?p=94185
//         "spurious wakes are unavoidably just the way things are, and your code needs to be able to handle them. Even if there were a way to clear the buffered wake from the kernel, applications still have to deal with spurious wakes for other reasons."
//
// https://en.wikipedia.org/wiki/ABA_problem
// Due to the ABA problem, transient changes from old to another value and back to old might be missed, and not unblock?
// 
// NtWaitForAlertByThreadId cannot be interrupted by APC or NtAlertThread
// NtWaitForAlertByThreadId Waits Forever on Windows 8.1 and Windows Server 2012 R2 : https://gist.github.com/LionNatsu/d89318c0e3334321cebf864eb611e85c
// NTSTATUS NTAPI NtAlertThreadByThreadId(HANDLE ThreadId);
// NTSTATUS NTAPI NtWaitForAlertByThreadId(HANDLE reserved, PLARGE_INTEGER Timeout);        
//
/*template<uint32 Flags, typename DT=uint32> class CFutex   // Fast User-Space Mutex    // Not part of CAtomic - requires complex state context but may not require a user-defined value
{
// Need a constructor?
//---------------------------------------------------------------------------
int Init(_MaySkip uint32 count=1)    // Can init multiple objects at 'this' if flags have ufWaitMultiple
{
 return 0;
}
//---------------------------------------------------------------------------
// If supported, can wait on N objects of the same type, starting from 'this' (Must be an array)
// Can check 'expected' if enabled.
// Can do timeout if supported
int Wait(_MaySkip sint64 millisecs=-1, _MaySkip uint32 expected=0, _MaySkip uint32 count=1)
{
 return 0;
}
//---------------------------------------------------------------------------
// May wake N waiters if supported. -1 To wake ALL if supported
sint32 Wake(_MaySkip sint64 millisecs=-1, _MaySkip uint32 count=1)
{
 return NAPI::futex_wake(&this->futex, PX::WAKE_ALL, 0);
}
//---------------------------------------------------------------------------
};*/

// ============================================================================
// MUTEX - Simple mutual exclusion lock
// ============================================================================

template<bool Recursive = false> class CMutex
{
 typename TSW<Recursive, PX::futex_ext, PX::futex_t>::T futex;     // [31:2] = owner_tid, [1:0] = state (00=unlocked, 01=locked, 10=locked+waiters)    // Win64 will have Data field left unused for a non-recursive mutex
 //uint32 recursion_count; // Only used if Recursive=true     // futex.Data is uint32 (optional in futex_t, always present in futex_ext)
 
 static constexpr uint32 STATE_UNLOCKED = 0;
 static constexpr uint32 STATE_LOCKED   = 1;
 static constexpr uint32 STATE_WAITERS  = 2;
 static constexpr uint32 STATE_MASK     = 3;
 static constexpr uint32 TID_SHIFT      = 2;
 
 static _finline uint32 Pack(uint32 tid, uint32 state) { return (tid << TID_SHIFT) | (state & STATE_MASK); }
 static _finline uint32 GetTID(uint32 val) { return val >> TID_SHIFT; }
 static _finline uint32 GetState(uint32 val) { return val & STATE_MASK; }
 //------------------------------------------------------------------------------
 static _finline bool AdaptiveSpin(volatile uint32* value, bool do_sleep = false)
 {
  //if(GetCPUCount() <= 1) return;  // Not cores!
  //for(uint32 i=0;i < 128;i++)
  //{
  // if(GetState(AtomicLoad<moRelaxed>(value)) ==  STATE_UNLOCKED) return true;
  // YieldCPU();
  //}
  //if(do_sleep) Sleep(0);
  return false;   // Timed out
 }
 //------------------------------------------------------------------------------

 public:
 CMutex() { this->Init(); }
 ~CMutex() { this->Destroy(); }     // BEWARE of lost Unlocks!
 //------------------------------------------------------------------------------
 void Init(void)
 {
  this->futex = {};     // MOPR::ZeroObj(&this->futex);
  //if constexpr (Recursive) this->futex.Data = 0;
 }
 //------------------------------------------------------------------------------ 
 void Destroy(void) { }  // Nothing to do - futex is just memory   // Don't forget to properly cleanup: if anyone is still waiting on it they will have
 //------------------------------------------------------------------------------ 
 // Returns '1' if ownership is taken 
 template<bool NoWait = false> sint32 Lock(void)
 {
  uint32 tid = GetThreadID();  
  if constexpr (Recursive)   // Recursive lock handling
   {
    uint32 current = AtomicLoad<moAcquire>(&this->futex.Value);
    if((GetTID(current) == tid) && (GetState(current) != STATE_UNLOCKED))
     {
      AtomicAdd<moRelaxed>(&this->futex.Data, uint32(1));
      return 1;
     }
   }
  
  uint32 expected = STATE_UNLOCKED;
  uint32 desired  = Pack(tid, STATE_LOCKED);
  if(AtomicCmpExch<moAcquire,moRelaxed,false>(&this->futex.Value, &expected, desired))  // Fast path
   {
    if constexpr (Recursive) AtomicStore<moRelaxed>(&this->futex.Data, 1);
    return 1;
   }
  
  if constexpr (NoWait) return 0;     // Would block
  //AdaptiveSpin(&this->futex.Value, true)
  for(;;)   // Slow path: contended
   {
    uint32 cur   = AtomicLoad<moAcquire>(&this->futex.Value);
    uint32 state = GetState(cur);
   
    if(state == STATE_UNLOCKED)
    {
     expected = STATE_UNLOCKED;
     if(AtomicCmpExch<moAcquire,moRelaxed,false>(&this->futex.Value, &expected, Pack(tid, STATE_LOCKED))) break;
     continue;
    }
   
    if(state == STATE_LOCKED) AtomicCmpExch<moRelaxed,moRelaxed,false>(&this->futex.Value, &cur, Pack(GetTID(cur), STATE_WAITERS));  // Mark WAITERS but PRESERVE OWNER
    NAPI::futex_wait(&this->futex, cur, PX::WAIT_INFINITE, PX::fxfTypeMutex);   // Sleep on current value
   }

  if constexpr (Recursive) AtomicStore<moRelaxed>(&this->futex.Data, 1);
  return 1;
 }
 //------------------------------------------------------------------------------
 // Returns '1' if successfully unlocked some waiter (ownership could change)
 sint32 Unlock(void)
 {
  uint32 tid = GetThreadID();
  uint32 current = AtomicLoad<moRelaxed>(&this->futex.Value);
  
  if(GetTID(current) != tid) return -1;  // Error: unlocking mutex we don't own!
  if constexpr (Recursive)
   {
    uint32 count = AtomicLoad<moRelaxed>(&this->futex.Data);
    if(count > 1)
     {
      AtomicSub<moRelaxed>(&this->futex.Data, uint32(1));
      return 0;  // Still owned
     }
   }
  
  uint32 expected = Pack(tid, STATE_LOCKED);
  if(AtomicCmpExch<moRelease,moRelaxed,false>(&this->futex.Value, &expected, STATE_UNLOCKED)) return 0;  // Fast path: no waiters
  
  uint32 prev = AtomicExch<moRelease>(&this->futex.Value, STATE_UNLOCKED);     // Slow path: there are waiters
  if(GetState(prev) == STATE_WAITERS) NAPI::futex_wake(&this->futex, 1, PX::fxfTypeMutex);
  return 1;           // We rely on overwriting recursion counter on next lock - please do not inspect it
 }
};

// ============================================================================
// SEMAPHORE - Counting semaphore
// ============================================================================

class CSemaphore
{
 PX::futex_t futex;  // Current count
 //------------------------------------------------------------------------------

public:
 CSemaphore(uint32 initial_value=0) { this->Init(initial_value); }
 ~CSemaphore() { this->Destroy(); }     // BEWARE of lost Unlocks!
 //------------------------------------------------------------------------------
 void Init(uint32 initial_value=0)
 {
  this->futex = {}; 
  this->futex.Value = initial_value;
 }
 //------------------------------------------------------------------------------
 void Destroy() { }   // Nothing to do
 //------------------------------------------------------------------------------ 
 // Get current value (non-blocking)
 uint32 GetValue(void) { return AtomicLoad<moAcquire>(&this->futex.Value); }
 //------------------------------------------------------------------------------
 // Increment semaphore (signal/post)
 void Signal(void)
 {
  AtomicAdd<moRelease>(&this->futex.Value, uint32(1));
  NAPI::futex_wake(&this->futex, 1, PX::fxfTypeSemaphore);  // Wake one waiter
 }
 //------------------------------------------------------------------------------
 // Wait until semaphore > 0, then decrement (blocking)  // Optionally with timeout (milliseconds)
 // Return values:
 //  1 = Success
 //  0 = Would block (NoWait or timeout_ms==0)
 //  Negative values = Errors (e.g., -ETIMEDOUT)
 template<bool NoWait = false> sint32 Wait(PX::timeout_t timeout_ms=PX::WAIT_INFINITE)
 {
  PX::timespec  start_time;
  PX::timeout_t rem_timeout = timeout_ms;
  bool have_timeout = timeout_ms;
  bool need_elapsed = have_timeout && (timeout_ms != PX::WAIT_INFINITE); 
  if(need_elapsed) NAPI::gettime(&start_time, PX::CLOCK_MONOTONIC);
  for(;;)
   {
    sint32 val = sint32(AtomicLoad<moAcquire>(&this->futex.Value));
    while(val > 0)   // Fast path: Try to decrement without blocking if positive
     {
      if(AtomicCmpExch<moAcquire,moRelaxed,false>((sint32*)&this->futex.Value, &val, val - 1)) return 1;  // Success  // NOTE: CmpExch should not care about signness 
     }
    
    if constexpr (NoWait) return 0;     // Would block 
    if(!have_timeout) return 0;         // Would block (zero timeout = non-blocking poll). Otherwise Futex returns immediately with ETIMEDOUT. A zero timeout poll should return 0 (would block), not -ETIMEDOUT (timeout expired)

    if(need_elapsed)  // Calculate remaining timeout
     {
      rem_timeout = NDT::CalcRemainingTimeout<PX::ETimeUnits::tuMilliseconds>(start_time, timeout_ms);
      if(rem_timeout == 0) return -PX::ETIMEDOUT;    // futex_wait would return ETIMEDOUT with (rem_timeout == 0) anyway.
     }

    uint32 expected = uint32(val);
    sint32 res = PX::futex_wait(&this->futex, expected, rem_timeout, PX::fxfTypeSemaphore);  // Wait for value to become positive    
    if(res == -PX::ETIMEDOUT) return res;  // Timeout   // Retrying spurious wake ups with initial timeout?   
   }
 }
};

// ============================================================================
// EVENT - Manual/Auto reset event
// ============================================================================
// Manual reset: use  the ResetEvent function to set the event state to nonsignaled otherwise the event is automatically resets the state to nonsignaled after a single waiting thread has been released.

template<bool manual_reset = false> class CEvent
{
 PX::futex_t futex;  // 0 = not signaled, 1 = signaled
 SCVR uint32 FutexFlags = (manual_reset ? (PX::fxfTypeEvent | PX::fxfNoWakeFIFO) : PX::fxfTypeEvent);

public:
 CEvent() { this->Init(); }
 ~CEvent() { this->Destroy(); }     // BEWARE of lost Unlocks!
 //------------------------------------------------------------------------------
 void Init(bool initial_state=false) // If initial_state is true, the initial state is signaled
 {
  this->futex = {}; 
  this->futex.Value = initial_state; // 1/0
 }
 //------------------------------------------------------------------------------
 void Destroy(void) { }   // Nothing to do
 //------------------------------------------------------------------------------
 void Set(void)
 {
  AtomicStore<moRelease>(&this->futex.Value, 1);   
  NAPI::futex_wake(&this->futex, (manual_reset ? PX::WAKE_ALL : 1), FutexFlags); 
 }
 //------------------------------------------------------------------------------
 void Reset(void)
 {
  AtomicStore<moRelease>(&this->futex.Value, 0);
 }
 //------------------------------------------------------------------------------
 template<bool NoWait = false> sint32 Wait(PX::timeout_t timeout_ms=PX::WAIT_INFINITE)       // Return value ???
 {
  PX::timespec  start_time;
  PX::timeout_t rem_timeout = timeout_ms;
  bool have_timeout = timeout_ms;
  bool need_elapsed = have_timeout && (timeout_ms != PX::WAIT_INFINITE); 
  if(need_elapsed) NAPI::gettime(&start_time, PX::CLOCK_MONOTONIC);
  for(;;)
   {
    uint32 val = AtomicLoad<moAcquire>(&this->futex.Value); 
    if(val == 1)
     {  
      if constexpr (!manual_reset)  // Auto-reset: atomically consume signal
       {
        if(AtomicCmpExch<moAcquire,moRelaxed,false>(&this->futex.Value, &val, uint32(0))) return 1;   // Success
        continue;  // Race - retry
       }
      return 1;    // Success ( Manual reset )
     }

    if constexpr (NoWait) return 0;     // Would block 
    if(!have_timeout) return 0;         // Would block (zero timeout = non-blocking poll). Otherwise Futex returns immediately with ETIMEDOUT. A zero timeout poll should return 0 (would block), not -ETIMEDOUT (timeout expired)
    if(need_elapsed)  // Calculate remaining timeout
     {
      rem_timeout = NDT::CalcRemainingTimeout<PX::ETimeUnits::tuMilliseconds>(start_time, timeout_ms);
      if(rem_timeout == 0) return -PX::ETIMEDOUT;    // futex_wait would return ETIMEDOUT with (rem_timeout == 0) anyway.
     }
 
    sint32 res = NAPI::futex_wait(&this->futex, 0, rem_timeout, FutexFlags);   // Wait for signal
    if(res == -PX::ETIMEDOUT) return res;
   }
 }
};

// ============================================================================
// CONDITION VARIABLE - Signal/wait coordination with mutex
// ============================================================================

class CCondVar
{
 PX::futex_t futex;  // Sequence number for broadcast detection
 
 template<bool Shared, typename T> static _finline void DoLock(T* lock)
 {
  if constexpr (requires { lock->LockRead(); })  // CRWLock
   {
    if constexpr (Shared) lock->LockRead();
    else lock->LockWrite();
   }
  else lock->Lock(); // CMutex (recursive or non-recursive)
 }

public:
 CCondVar() { this->Init(); }
 ~CCondVar() { this->Destroy(); }     // BEWARE of lost Unlocks!
 //------------------------------------------------------------------------------
 void Init(void)
 {
  this->futex = {}; 
 }
 //------------------------------------------------------------------------------
 void Destroy(void) { }  // Nothing to do
 //------------------------------------------------------------------------------
 // Signal one waiting thread
 void Signal(void)
 {
  AtomicAdd<moRelease>(&this->futex.Value, uint32(1));
  NAPI::futex_wake(&this->futex, 1, PX::fxfTypeCondVar);     // Wake one waiter
 }
 //------------------------------------------------------------------------------
 // Signal all waiting threads
 void Broadcast(void)
 {
  AtomicAdd<moRelease>(&this->futex.Value, uint32(1));
  NAPI::futex_wake(&this->futex, PX::WAKE_ALL, PX::fxfTypeCondVar);  // Wake all
 }
 //------------------------------------------------------------------------------
 // Wait on condition with mutex and timeout
 // Condition variables don't really have a non-blocking mode that makes sense. The lock is already unlocked when you check the condition. If you want non-blocking behavior, just check the condition without calling Wait().
 template<bool SharedRWL=false, typename LockType> sint32 Wait(LockType* lock, PX::timeout_t timeout_ms=PX::WAIT_INFINITE)   
 {
  PX::timespec  start_time;
  PX::timeout_t rem_timeout = timeout_ms;
  bool have_timeout = timeout_ms;         // With condition variables, timeout_ms == 0 is a bit unusual. 
  bool need_elapsed = have_timeout && (timeout_ms != PX::WAIT_INFINITE);
  
  sint32 seq = AtomicLoad<moAcquire>(&this->futex.Value);
  lock->Unlock();  // Uses moRelease (proper ordering with seq)  // Release lock before wait
  
  if(!have_timeout)    // Handle zero timeout (non-blocking poll)
   {
    DoLock<SharedRWL>(lock);  // Re-acquire lock
    return 0;  // Would block (didn't wait at all)
   }
  
  if(need_elapsed) NAPI::gettime(&start_time, PX::CLOCK_MONOTONIC);
  for(;;)
   {
    if(need_elapsed)
     {
      rem_timeout = NDT::CalcRemainingTimeout<PX::ETimeUnits::tuMilliseconds>(start_time, timeout_ms);
      if(rem_timeout == 0)
       {
        DoLock<SharedRWL>(lock);
        return -PX::ETIMEDOUT;
       }
     }
    
    sint32 res = NAPI::futex_wait(&this->futex, seq, rem_timeout, PX::fxfTypeCondVar);
    if(AtomicLoad<moAcquire>(&this->futex.Value) != seq)   // Check if sequence changed (condition signaled)
     {
      DoLock<SharedRWL>(lock);
      return 0;  // Success
     }
    
    if(res == -PX::ETIMEDOUT)
     {
      DoLock<SharedRWL>(lock);
      return res;
     }
    // Spurious wakeup - continue loop
   }
 }
};

// ============================================================================
// READ-WRITE LOCK - Multiple readers OR single writer       // DONE
// ============================================================================
// CRWLock - Reader/Writer Lock
// 
// Template parameter:
//   starve_writers=true:  Reader-preferring (high read throughput, writers may starve)
//   starve_writers=false: Writer-fair (prevents writer starvation)
//
// Return values:
//   1  = Success (lock acquired)
//   0  = Would block (NoWait mode)
//   -1 = Too many readers (overflow)

template<bool starve_writers=true> struct CRWLock
{
 PX::futex_t futex;  // High bit = writer lock, bit 30 = writer pending, low bits = reader count
 
 SCVR uint32 FutexFlags     = PX::fxfTypeRWLock | PX::fxfNoWakeFIFO;
 SCVR uint32 WRITER_LOCK    = 0x80000000;
 SCVR uint32 WRITER_PENDING = starve_writers ? 0 : 0x40000000;
 SCVR uint32 READER_MASK    = starve_writers ? 0x7FFFFFFF : 0x3FFFFFFF;
 
public:
 CRWLock() { this->Init(); }
 ~CRWLock() { this->Destroy(); }     // BEWARE of lost Unlocks!
 //------------------------------------------------------------------------------
 void Init(void)
 {
  this->futex = {}; 
 }
 //------------------------------------------------------------------------------
 void Destroy(void) { }   // Nothing to do
 //------------------------------------------------------------------------------
 // Lock for reading (multiple readers allowed)
 template<bool NoWait = false> sint32 LockRead(void)    // Shared lock
 {
  for(;;)
   {
    uint32 val = AtomicLoad<moAcquire>(&this->futex.Value);
    if(val & (WRITER_LOCK | WRITER_PENDING))    // Block if writer active OR writer pending (when not starving writers)
     {
      if constexpr (NoWait) return 0;     // Would block 
      PX::futex_wait(&this->futex, val, PX::WAIT_INFINITE, FutexFlags);
      continue;
     }

    if((val & READER_MASK) == READER_MASK) return -1;  // Too many readers   
    if(AtomicCmpExch<moAcquire,moRelaxed,false>(&this->futex.Value, &val, val + 1)) return 1;  // Try to increment reader count  // Returns if Success
   }
 }
 //------------------------------------------------------------------------------
 // Lock for writing (exclusive)
 template<bool NoWait = false> sint32 LockWrite(void)   // Exclusive lock
 {
  for(;;)
   {
    uint32 val = AtomicLoad<moAcquire>(&this->futex.Value);  // FIXED: Load current value
    if( !(val & (WRITER_LOCK | READER_MASK)) )    // Try to acquire writer lock when no readers and no writer
     {
      if(AtomicCmpExch<moAcquire,moRelaxed,false>(&this->futex.Value, &val, WRITER_LOCK)) return 1;  // ReturnsSuccess
      continue;  // CAS failed, retry
     }
     
    if constexpr (!starve_writers)   // Set writer pending flag (only when not starving writers)
     {
      if(!(val & WRITER_PENDING))
       {
        AtomicCmpExch<moAcquire,moRelaxed,false>(&this->futex.Value, &val, (val | WRITER_PENDING));
        val = AtomicLoad<moAcquire>(&this->futex.Value);  // Reload after setting flag
       }
     }
    
    if constexpr (NoWait) return 0;
    PX::futex_wait(&this->futex, val, PX::WAIT_INFINITE, FutexFlags);  // FIXED: Wait on current value
   }
 }
 //------------------------------------------------------------------------------
 // Unlock (for both readers and writers)
 void Unlock(void)
 {
  uint32 val = AtomicLoad<moRelaxed>(&this->futex.Value);
  if(val & WRITER_LOCK)  // Writer unlock
   {
    AtomicStore<moRelease>(&this->futex.Value, 0);  // Clears everything
    NAPI::futex_wake(&this->futex, PX::WAKE_ALL, FutexFlags);    
    return;
   }

 
  uint32 new_val;
  do {                 // Reader unlock - just decrement, don't touch WRITER_PENDING
      val = AtomicLoad<moRelaxed>(&this->futex.Value);
      new_val = val - 1;
  } while(!AtomicCmpExch<moRelease,moRelaxed,false>(&this->futex.Value, &val, new_val));
  
  if(!(new_val & READER_MASK)) 
   {
    uint32 cur = AtomicLoad<moRelaxed>(&this->futex.Value);
    if(cur & WRITER_PENDING) NAPI::futex_wake(&this->futex, 1, FutexFlags);
      else NAPI::futex_wake(&this->futex, PX::WAKE_ALL, FutexFlags); 
   }
 }
};

// ============================================================================
// BARRIER - Synchronize N threads at a point
// ============================================================================

struct CBarrier
{
 PX::futex_ext futex;      // Current count of waiting threads

 SCVR uint32 TreshOffs = 16;
 //uint32      threshold;  // Data[16-31]: Number of threads to wait for
 //uint32      generation; // Data[0 -15]: Generation counter to handle reuse
 
public:
 CBarrier(uint16 count) { this->Init(count); }
 ~CBarrier() { this->Destroy(); }     // BEWARE of lost Unlocks!
 //------------------------------------------------------------------------------
 void Init(uint16 count)
 {
  this->futex = {}; 
  this->futex.Data = uint32(count) << TreshOffs;
 }
 //------------------------------------------------------------------------------
 void Destroy(void) { }   // Nothing to do
 //------------------------------------------------------------------------------
 // Wait at barrier until all threads arrive
 sint32 Wait(void)
 {
  uint16 gen   = uint16(AtomicLoad<moAcquire>(&this->futex.Data));
  uint32 thrsh = AtomicLoad<moRelaxed>(&this->futex.Data) >> TreshOffs;           // Set by Init, not changed. Should not be affected by generation increment (low 16 bits) unles overfllown
  uint32 count = AtomicAdd<moAcqRel, false>(&this->futex.Value, uint32(1));       // Increment arrival count   // Returns new value
  
  if(count >= thrsh)     // Last thread to arrive - reset and wake all
   {
    AtomicStore<moRelease>(&this->futex.Value, 0);
    AtomicAdd<moRelease>(&this->futex.Data, uint32(1));  // New generation   // Unlikely to overflow uint16 here?
    NAPI::futex_wake(&this->futex, PX::WAKE_ALL, PX::fxfNoWakeFIFO);
    return 1;  // Leader thread
   }

  while(uint16(AtomicLoad<moAcquire>(&this->futex.Data)) == gen)   // Wait for barrier to be released
   {
    uint32 expected = count;
    NAPI::futex_wait(&this->futex, expected, PX::WAIT_INFINITE, PX::fxfNoWakeFIFO);
    count = AtomicLoad<moAcquire>(&this->futex.Value);
   }
  return 0;  // Follower thread
 }
};

// ============================================================================
// ONE-TIME INITIALIZATION - Thread-safe once
// ============================================================================

struct COnce       // Useless
{
 PX::futex_t futex;  // 0 = not started, 1 = in progress, 2 = done
 
public:
 COnce() { this->Init(); }
 ~COnce() { this->Destroy(); }   
 //------------------------------------------------------------------------------
 void Init()
 {
  this->futex = {}; 
 }
 //------------------------------------------------------------------------------
 void Destroy(void) { }   // Nothing to do
 //------------------------------------------------------------------------------
 template<typename F> void Call(F&& func)
 {
  uint32 val = AtomicLoad<moAcquire>(&this->futex.Value);
  if(val == 2) return;  // Already done
  
  // Try to become the initializer
  val = 0;
  if(AtomicCmpExch<moAcquire,moRelaxed,false>(&this->futex.Value, &val, 1))
   {
    func();   // We won - do initialization
    AtomicStore<moRelease>(&this->futex.Value, 2);
    PX::futex_wake(&this->futex, PX::WAKE_ALL, PX::fxfNoWakeFIFO);       // Mark as done and wake waiters
   }
  else
   {
    while(AtomicLoad<moAcquire>(&this->futex.Value) != 2)    // Wait for initialization to complete
     {
      uint32 expected = 1;
      PX::futex_wait(&this->futex, expected, PX::WAIT_INFINITE, PX::fxfNoWakeFIFO);
     }
   }
 }
};
//============================================================================================================
//};  // SYNC

/*
struct alignas(8) futex_t  // Emulated
{
 uint32 val;  // Linux futex uses only 32-bit variables
 uint32 cnt;  // Waiter count  // Allows fast path for FUTEX_WAKE // Windows will work better with this (although zero timeout should prevent blocking anyway)
};

// On success, FUTEX_WAIT returns 0 if the caller was woken up. callers should always conservatively assume that a return value of 0 can mean a spurious wake-up, and use the futex word's value
static sint32 PXCALL futex_wait(futex_t* addr, uint32 expected, sint64 millisecs);
// On success, FUTEX_WAKE returns the number of waiters that were woken up
static sint32 PXCALL futex_wake(futex_t* addr, uint32 count);
int futex_wake(int *futex) {return _umtx_op(futex, UMTX_OP_WAKE, 1, 0, 0);}

int futex_wait(int *futex, int val, struct timespec *ts) {
	int err = _umtx_op(futex, UMTX_OP_WAIT_UINT, val, 0, (void *)ts);
	if (err != 0) {
		if (errno == ETIMEDOUT)return FUTEX_TIMEDOUT;	
		 else if (errno == EINTR)return FUTEX_INTERRUPTED; // XXX: unsure if umtx can be EINTR'd.	
	}
	return err;
} */

/*
// https://locklessinc.com/articles/keyed_events/
// https://medium.com/windows-os-internals/windows-internals-thread-synchronization-primitives-0b222b71f0ce
// https://joeduffyblog.com/2006/11/28/windows-keyed-events-critical-sections-and-new-vista-synchronization-features/
// With 'waiters count' fast path it should be comparable in performance with WaitOnAddress/SRW locks
// On Windows XP, keyed event wait list is a linked list, so finding and setting a key required an O(n) traversal. 
// //  '\KernelObjects\CritSecOutOfMemoryEvent' because it is always present
// https://medium.com/windows-os-internals/windows-internals-thread-synchronization-primitives-0b222b71f0ce
//
FUNC_WRAPPERFI(PX::futex_wait,  futex_wait  )    
{   
 PX::futex_t* addr      = GetParFromPk<0>(args...);
 uint32       expected  = GetParFromPk<1>(args...);
 sint64       millisecs = GetParFromPk<2>(args...);
 if(AtomicLoad(&addr->val) != expected) return 0;   // Check value BEFORE incrementing waiters (avoid increment if value changed)
 AtomicAdd(&addr->cnt, 1);
 if(AtomicLoad(&addr->val) != expected) { AtomicSub(&addr->cnt, 1); return 0;}      // Double-check after incrementing (classic pattern)
 NT::LARGE_INTEGER timeout = (millisecs * 10000LL) / 100LL;         // Milliseconds to 100-ns intervals
 sint32 result = SAPI::NtWaitForKeyedEvent((NT::HANDLE)fwsinf.hKeyedEvent, &addr->val, 0, &timeout);   // Use &addr->val as the keyed event key
 AtomicSub(&addr->cnt, 1);
 return -NTX::NTStatusToLinuxErr(result);     // STATUS_SUCCESS, STATUS_TIMEOUT
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::futex_wake,  futex_wake  )    
{  
 PX::futex_t* addr = GetParFromPk<0>(args...);
 uint32      count = GetParFromPk<1>(args...);
 if(AtomicLoad(&addr->cnt) == 0) return 0;      // Fast path - no waiters, skip syscall entirely!     
 NT::LARGE_INTEGER timeout = 0;   // Don't wait at all
 int woken = 0;  
 for(uint32 i = 0; i < count; i++)   // Wake waiters
  {
   sint32 result = SAPI::NtReleaseKeyedEvent((NT::HANDLE)fwsinf.hKeyedEvent, addr, 0, &timeout);
   if(NT::IsError(result)) return -NTX::NTStatusToLinuxErr(result); 
   if(result == NT::STATUS_TIMEOUT) break;      // There was no waiter available at that moment.
   woken++;                                     // STATUS_SUCCESS: Successfully paired with a waiting thread and woke it
  }
 return woken;
}

*/
