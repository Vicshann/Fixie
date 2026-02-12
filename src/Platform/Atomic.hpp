
#pragma once

//============================================================================================================
/*
The x86 architecture supports additional memory ordering modifiers to mark critical sections for hardware lock elision. These modifiers can be bitwise or’ed with a standard memory order to atomic intrinsics.

__ATOMIC_HLE_ACQUIRE
Start lock elision on a lock variable. Memory order must be __ATOMIC_ACQUIRE or stronger.

__ATOMIC_HLE_RELEASE
End lock elision on a lock variable. Memory order must be __ATOMIC_RELEASE or stronger.

When a lock acquire fails, it is required for good performance to abort the transaction quickly. This can be done with a _mm_pause.

#include <immintrin.h> // For _mm_pause

int lockvar;

// Acquire lock with lock elision
while (__atomic_exchange_n(&lockvar, 1, __ATOMIC_ACQUIRE|__ATOMIC_HLE_ACQUIRE))
    _mm_pause(); // Abort failed transaction 
...
// Free lock with lock elision 
__atomic_store_n(&lockvar, 0, __ATOMIC_RELEASE|__ATOMIC_HLE_RELEASE);

*/

//struct NATM   // Part of common functionality
//{
/* Memory Ordering Cheat Sheet:
 * 
 * Relaxed:  Just make it atomic, no ordering (counters, stats)
 * Acquire:  Load barrier - use when READING flags/locks
 * Release:  Store barrier - use when WRITING flags/locks
 * AcqRel:   Both - use for read-modify-write (fetch_add, exchange)
 * SeqCst:   Slowest, total order - rarely needed
 * 
 * Common pattern:
 *   Writer: data = x; flag.Store(1, Release);
 *   Reader: while(!flag.Load(Acquire)); use(data);
 
The following built-in functions approximately match the requirements for the C++11 memory model. They are all identified by being prefixed with ‘__atomic’ and most are overloaded so that they work with multiple types.

These functions are intended to replace the legacy ‘__sync’ builtins. The main difference is that the memory order that is requested is a parameter to the functions. New code should always use the ‘__atomic’ builtins rather than the ‘__sync’ builtins.

Note that the ‘__atomic’ builtins assume that programs will conform to the C++11 memory model. In particular, they assume that programs are free of data races. See the C++11 standard for detailed requirements.

The ‘__atomic’ builtins can be used with any integral scalar or pointer type that is 1, 2, 4, or 8 bytes in length. 16-byte integral types are also allowed if ‘__int128’ (see 128-bit Integers) is supported by the architecture.

The four non-arithmetic functions (load, store, exchange, and compare_exchange) all have a generic version as well. This generic version works on any data type. It uses the lock-free built-in function if the specific data type size makes that possible; otherwise, an external call is left to be resolved at run time. This external call is the same format with the addition of a ‘size_t’ parameter inserted as the first parameter indicating the size of the object being pointed to. All objects must be the same size. 
 
An atomic operation can both constrain code motion and be mapped to hardware instructions for synchronization between threads (e.g., a fence). To which extent this happens is controlled by the memory orders, which are listed here in approximately ascending order of strength.
*/

/*
__ATOMIC_RELAXED: 
    // "I don't care about ordering, just make it atomic"
    // Use for: counters where you only care about final value
    // RELAXED: No ordering guarantees, only atomicity
    // ✓ Use for: stats counters, reference counts (when dtor doesn't access data)
    // ✗ Don't use for: flags protecting data, locks
    // Example: atomic_stats.FetchAdd(1, Relaxed);    

__ATOMIC_ACQUIRE:    
    // "I'm reading a guard/flag, block loads until I see it"
    // Use for: loading a flag before loading data it protects
    // ACQUIRE: Barrier for loads - "don't move later ops before this load"
    // ✓ Use for: reading locks, loading "data ready" flags
    // Pairs with: Release stores
    // Example: while (!ready.Load(Acquire)) {} // then safe to read data    

 __ATOMIC_RELEASE:
    // "I'm writing a guard/flag, flush stores before others see it"
    // Use for: storing data then setting a "ready" flag
    // RELEASE: Barrier for stores - "don't move earlier ops after this store"
    // ✓ Use for: releasing locks, setting "data ready" flags
    // Pairs with: Acquire loads
    // Example: data = x; ready.Store(true, Release);
    
__ATOMIC_ACQ_REL:    
    // "I'm doing read-modify-write with synchronization"
    // Use for: locks, fetch_add when order matters
    // SEQ_CST: Total global ordering (slowest, most restrictive)
    // ✓ Use for: when you need to reason about all threads seeing same order
    // ✗ Usually overkill - Acquire/Release is almost always enough
    // Example: rarely needed in practice
   
    
__ATOMIC_SEQ_CST:
    // "I need total global ordering" (slowest, rarely needed)
    // Use for: when you need to reason about global order
    // CONSUME: Like Acquire but weaker (compiler treats as Acquire anyway)
    // Just use Acquire instead
*/

enum EMemoryOrder: int 
{
 moRelaxed = __ATOMIC_RELAXED,   // 0: Implies no inter-thread ordering constraints.
 moConsume = __ATOMIC_CONSUME,   // 1:  This is currently implemented using the stronger __ATOMIC_ACQUIRE memory order because of a deficiency in C++11’s semantics for memory_order_consume.
 moAcquire = __ATOMIC_ACQUIRE,   // 2: Creates an inter-thread happens-before constraint from the release (or stronger) semantic store to this acquire load. Can prevent hoisting of code to before the operation.
 moRelease = __ATOMIC_RELEASE,   // 3: Creates an inter-thread happens-before constraint to acquire (or stronger) semantic loads that read from this release store. Can prevent sinking of code to after the operation.
 moAcqRel  = __ATOMIC_ACQ_REL,   // 4: Combines the effects of both __ATOMIC_ACQUIRE and __ATOMIC_RELEASE.
 moSeqCst  = __ATOMIC_SEQ_CST,   // 5: Enforces total ordering with all other __ATOMIC_SEQ_CST operations.

// Better names:
//   moUnordered  = __ATOMIC_RELAXED,    // No synchronization - reorderable, fastest
//   moLoadSync   = __ATOMIC_ACQUIRE,    // Load: prevent later reads/writes from moving before this
//   moStoreSync  = __ATOMIC_RELEASE,    // Store: prevent earlier reads/writes from moving after this
//   moFullSync   = __ATOMIC_ACQ_REL,    // Both: full synchronization for read-modify-write
//   moSequential = __ATOMIC_SEQ_CST,    // Strongest: global ordering of all seq-cst operations


// For hardware lock elision (X86 only)  //  XACQUIRE and XRELEASE instruction prefixes used for Hardware Lock Elision (HLE) on TSX CPUs
// Intel’s own documentation states that HLE is removed from all Intel products released in 2019 and later, and many 10th‑gen (Comet Lake, Ice Lake) client parts do not support TSX at all, neither HLE nor RTM.
// moAcquireLock = __ATOMIC_ACQUIRE | __ATOMIC_HLE_ACQUIRE,
// moReleaseLock = __ATOMIC_RELEASE | __ATOMIC_HLE_RELEASE

 moValueMask   = 0xFFFF,
 moNone        = moValueMask,              // Not atomic
 moTypeBitTest = 0x00800000,               // Used with FlagSet/FlagClr 
 moDefBitTest  = moTypeBitTest|moSeqCst,   // Default for bit test 
 moNBitTest    = moTypeBitTest|moNone 
};

// ============================================================================
// Low-level atomic operations
// ============================================================================
template<EMemoryOrder Order=moAcquire, typename T> _finline static T AtomicLoad(const T* ptr) 
{
 return __atomic_load_n(ptr, Order);
}
//-----------------------------------------------------------------------------
template<EMemoryOrder Order=moRelease, typename T> _finline static void AtomicStore(T* ptr, auto&& val) 
{
 __atomic_store_n(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
// __atomic_*_fetch return the result of the operation. 
// __atomic_fetch_* return the value that had previously been in *ptr. 
//
template<EMemoryOrder Order=moAcqRel, bool RetOld=true, typename T> _finline static T AtomicAdd(T* ptr, auto&& val)   // 'auto&&' prevents deduction of 'T' from 'val'
{
 if constexpr(RetOld) return __atomic_fetch_add(ptr, T(val), Order);    // _InterlockedExchangeAdd
   else return __atomic_add_fetch(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
template<EMemoryOrder Order=moAcqRel, bool RetOld=true, typename T> _finline static T AtomicSub(T* ptr, auto&& val) 
{
 if constexpr(RetOld) return __atomic_fetch_sub(ptr, T(val), Order);
   else return __atomic_sub_fetch(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
template<EMemoryOrder Order=moAcqRel, bool RetOld=true, typename T> _finline static T AtomicAnd(T* ptr, auto&& val) 
{
 if constexpr(RetOld) return __atomic_fetch_and(ptr, T(val), Order);   // NOTE: Most of 'atomic_fetch_*' will produce CAS loop if the return value is actually used
   else return __atomic_and_fetch(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
template<EMemoryOrder Order=moAcqRel, bool RetOld=true, typename T> _finline static T AtomicOr(T* ptr, auto&& val) 
{
 if constexpr(RetOld) return __atomic_fetch_or(ptr, T(val), Order);
   else return __atomic_or_fetch(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
template<EMemoryOrder Order=moAcqRel, bool RetOld=true, typename T> _finline static T AtomicXor(T* ptr, auto&& val) 
{
 if constexpr(RetOld) return __atomic_fetch_xor(ptr, T(val), Order);
   else return __atomic_xor_fetch(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
// NAND is NOT a native atomic instruction on most architectures
template<EMemoryOrder Order=moAcqRel, bool RetOld=true, typename T> _finline static T AtomicNAnd(T* ptr, auto&& val) 
{
 if constexpr(RetOld) return __atomic_fetch_nand(ptr, T(val), Order);
   else return __atomic_nand_fetch(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
// Toggle bit, return prev/new value
//__attribute__((optimize("O3")))  // GCC
template<EMemoryOrder Order=moSeqCst, bool RetOld=true, typename T> _finline bool AtomicBitFlip(T* ptr, uint8 bit_index)
{
 const T mask = T(1) << bit_index;
 T val  = AtomicXor<RetOld,Order>(ptr, mask); // Force RetOld=true    
 return bool(val & mask);
}
//---------------------------------------------------------------------------  
template<EMemoryOrder Order=moAcquire, typename T> _finline bool AtomicBitTst(T* ptr, uint8 bit_index)  
{   
 return bool(AtomicLoad<Order>(ptr) & (T(1) << bit_index));
} 
//---------------------------------------------------------------------------
// _interlockedbittestandset     // NOTE: The code is giant without at least O1
//__attribute__((optimize("O3")))  // GCC
/*template<EMemoryOrder Order=moSeqCst, typename T> _finline bool AtomicBitSet(T* ptr, uint8 bit_index)   // X86: BTS
{
 const T mask = T(1) << bit_index;
 T old  = AtomicOr<true,Order>(ptr, mask);  // Force RetOld=true    
 return bool(old & mask);
}
//---------------------------------------------------------------------------  
// _interlockedbittestandreset   // NOTE: The code is giant without at least O1
//__attribute__((optimize("O3")))  // GCC
template<EMemoryOrder Order=moSeqCst, typename T> _finline bool AtomicBitClr(T* ptr, uint8 bit_index)   // X86: BTR
{
 const T mask = T(1) << bit_index;
 T old  = AtomicAnd<true,Order>(ptr, ~mask);  // Force RetOld=true    
 return bool(old & mask);
} */
//--------------------------------------------------------------------------- 
template<EMemoryOrder order=moDefBitTest, typename T> _finline constexpr auto AtomicBitSet(T* Value, uint8 bit_index)
{
 if constexpr ((order & moValueMask) != moNone)
  {
   if constexpr (order & moTypeBitTest) 
    {
#if __has_builtin(_interlockedbittestandset) || defined(COMP_MSVC)       // This will allow BTS instruction even in O0 builds on X86
     if constexpr (sizeof(T) > 4)return _interlockedbittestandset64((__int64*)Value, bit_index);
       else return _interlockedbittestandset((long*)Value, bit_index);
#else
     constexpr T mask = T(1) << bit_index;
     T old  = AtomicOr<EMemoryOrder(order & moValueMask), true>(ptr, mask);  // Force RetOld=true       // Order?
     return bool(old & mask); 
#endif
    }
     else AtomicOr<EMemoryOrder(order & moValueMask), false>(Value, (T(1) << bit_index));
  }
 else    // Not atomic
  {
   if constexpr (order & moTypeBitTest)
    {
     constexpr T mask = T(1) << bit_index;
     bool old  = *Value & mask;   
     *Value |= mask;
     return old;
    }
     else *Value |= (T(1) << bit_index);
  }
}
//-----------------------------------------------------------------------------
template<EMemoryOrder order=moDefBitTest, typename T> _finline constexpr auto AtomicBitClr(T* Value, uint8 bit_index)
{
 if constexpr ((order & moValueMask) != moNone)
  {
   if constexpr (order & moTypeBitTest) 
#if __has_builtin(_interlockedbittestandreset) || defined(COMP_MSVC)       // This will allow BTS instruction even in O0 builds on X86
     if constexpr (sizeof(T) > 4)return _interlockedbittestandreset64((__int64*)Value, bit_index);
       else return _interlockedbittestandreset((long*)Value, bit_index);
#else
     constexpr T mask = T(1) << bit_index;
     T old  = AtomicAnd<EMemoryOrder(order & moValueMask), true>(ptr, ~mask);  // Force RetOld=true        // Order?
     return bool(old & mask);
#endif
     else AtomicAnd<EMemoryOrder(order & moValueMask), false>(Value, ~(T(1) << bit_index));
  }
 else    // Not atomic
  {
   if constexpr (order & moTypeBitTest)
    {
     constexpr T mask = T(1) << bit_index;
     bool old  = *Value & mask;   
     *Value &= ~mask;
     return old;
    }
     else *Value &= ~(T(1) << bit_index);
  }
}
//-----------------------------------------------------------------------------
template<EMemoryOrder Order=moAcqRel, typename T> _finline static T AtomicExch(T* ptr, auto&& val) 
{
 return __atomic_exchange_n(ptr, T(val), Order);
}
//-----------------------------------------------------------------------------
// This built-in function implements an atomic compare and exchange operation. 
// This compares the contents of *ptr with the contents of *expected. If equal, the operation is a read-modify-write operation that writes desired into *ptr. 
// If they are not equal, the operation is a read and the current contents of *ptr are written into *expected. weak is true for weak compare_exchange, which may fail spuriously, 
// and false for the strong variation, which never fails spuriously. Many targets only offer the strong variation and ignore the parameter.
// InShort: Writes the previous value into *expected on failure (and also on success, but you don’t care then). Returns TRUE if exchange happened.
//    "Dear memory, are you still X?"
//       yes -> "Cool, become Y" (success = true)
//       no  -> "Nope, you're Z now" (success = false, expected = Z)
//
template<EMemoryOrder Success=moAcqRel, EMemoryOrder Failure=moAcquire, bool Weak=false, typename T> _finline static bool AtomicCmpExch(T* ptr, T* expected, auto&& desired) 
{
 return __atomic_compare_exchange_n(ptr, expected, T(desired), Weak, Success, Failure);    // Not weak
}
//-----------------------------------------------------------------------------
/*template<typename T> _finline static bool AtomicCmpExchPtr(volatile vptr* value, vptr exchange, vptr comperand)     // Useless?
{
 return __sync_bool_compare_and_swap(value, comperand, exchange);     // Old, deprecated
} */
//-----------------------------------------------------------------------------
// This built-in function acts as a synchronization fence between threads based on the specified memory order.
template<EMemoryOrder Order=moSeqCst> _finline static void AtomicThreadFence(void) 
{
 __atomic_thread_fence(Order);
}
//-----------------------------------------------------------------------------
// This built-in function acts as a synchronization fence between a thread and signal handlers based in the same thread.
template<EMemoryOrder Order=moSeqCst> _finline static void AtomicSignalFence(void) 
{
 __atomic_signal_fence(Order);
}
//-----------------------------------------------------------------------------
// No memory operand will be moved across the operation, either forward or backward. 
// Further, instructions will be issued as necessary to prevent the processor from speculating loads across the operation and from queuing stores after the operation.
_finline static void AtomicMemBarrier(void)
{
 __sync_synchronize();  // Causes -Watomic-implicit-seq-cst  // roughly equivalent to: __atomic_thread_fence(__ATOMIC_SEQ_CST);
}
//-----------------------------------------------------------------------------
// __atomic_always_lock_free (size_t size, void *ptr)
// This built-in function returns true if objects of size bytes always generate lock-free atomic instructions for the target architecture. size must resolve to a compile-time constant and the result also resolves to a compile-time constant.
// if (__atomic_always_lock_free (sizeof (long long), 0))

// ============================================================================
// Safe wrapper class
// ============================================================================

template<typename T, bool OpRetOld=false> class CAtomic 
{
protected:
 alignas(sizeof(T)) mutable T Value;  // volatile: The atomic operations already have the right semantics. volatile doesn't help with threading and can actually prevent optimizations.
    
public:
  CAtomic() = default;
  CAtomic(T v) : Value(v) {}
  
  CAtomic(const CAtomic&) = delete;
  CAtomic& operator=(const CAtomic&) = delete;
  
  template<EMemoryOrder Order=moAcquire>  T    Load() const { return AtomicLoad<Order>(&this->Value); }
  template<EMemoryOrder Order=moRelease> void Store(T v) { AtomicStore<Order>(&this->Value, v); }   
  template<EMemoryOrder Order=moAcqRel>   T    Exch(T v) { return AtomicExch<Order>(&this->Value, v); }
  template<EMemoryOrder Success=moAcqRel, EMemoryOrder Failure=moAcquire, bool Weak=false> bool CmpExch(T& expected, auto&& desired) { return AtomicCmpExch<Success,Failure,Weak>(&this->Value, &expected, desired); }

// Return modified this->Value (the result)
  template<EMemoryOrder Order=moAcqRel> T Add(T v) { return AtomicAdd<Order,OpRetOld>(&this->Value, v); }
  template<EMemoryOrder Order=moAcqRel> T Sub(T v) { return AtomicSub<Order,OpRetOld>(&this->Value, v); }
  template<EMemoryOrder Order=moAcqRel> T And(T v) { return AtomicAnd<Order,OpRetOld>(&this->Value, v); }
  template<EMemoryOrder Order=moAcqRel> T Xor(T v) { return AtomicXor<Order,OpRetOld>(&this->Value, v); }
  template<EMemoryOrder Order=moAcqRel> T Or(T v)  { return AtomicOr<Order,OpRetOld>(&this->Value,  v); }

// Compound assignment operators (return new this->Value, match C++ semantics)
  T operator+=(T v) { return AtomicAdd<moAcqRel,false>(&this->Value, v); }
  T operator-=(T v) { return AtomicSub<moAcqRel,false>(&this->Value, v); }
  T operator&=(T v) { return AtomicAnd<moAcqRel,false>(&this->Value, v); }
  T operator|=(T v) { return AtomicOr<moAcqRel,false>(&this->Value, v); }
  T operator^=(T v) { return AtomicXor<moAcqRel,false>(&this->Value, v); }
  
// Prefix operators (return modified this->Values)
  T operator++() { return AtomicAdd<moAcqRel,false>(&this->Value, 1); }
  T operator--() { return AtomicSub<moAcqRel,false>(&this->Value, 1); }
// Postfix operators (return original this->Values)
  T operator++(int) { return AtomicAdd<moAcqRel,true>(&this->Value, 1); }
  T operator--(int) { return AtomicSub<moAcqRel,true>(&this->Value, 1); }
  
  operator T() const { return this->Load(); }   // With default order
  void operator= (T v) { this->Store(v); }
};
//============================================================================================================

// ============================================================================
// Atomic bitfield operation helpers
// These compile to efficient atomic RMW operations on both x86 and ARM
// NOTE: Casting int to bool is not undefined behavior
// NOTE: No bit range checks is intentional
// TODO: Check memory order defaults
// ============================================================================

template<typename T, bool OpRetOld=false> class CAtomicBits: public CAtomic<T, OpRetOld>   // Defaults are set for use with synchronization primitives
{
// CAtomic<T>& Value;
 static T MakeRangeMsk(uint8 bit_start, uint8 bit_count) { return ((T(1) << bit_count) - 1) << bit_start; }   

public:
// explicit CAtomicBits(CAtomic<T>& value) : this->Value(value) {}
  CAtomicBits() = default;
  CAtomicBits(T v) : CAtomic<T, OpRetOld>(v) { }
  
  CAtomicBits(const CAtomicBits&) = delete;
  CAtomicBits& operator=(const CAtomicBits&) = delete;

 // ========================================================================
 // Single bit operations (bit index 0-31)
 // ========================================================================
 
 // Set bit to 1, return new/previous value of that bit
 template<EMemoryOrder Order=moSeqCst> bool BitSet(uint8 bit_index)
 {
  const T mask = T(1) << bit_index;
  T val  = this->Or<Order>(mask);
  return bool(val & mask);
 }
//---------------------------------------------------------------------------    
 // Clear bit to 0, return new/previous value of that bit
 template<EMemoryOrder Order=moSeqCst> bool BitClr(uint8 bit_index)
 {
  const T mask = T(1) << bit_index;
  T val  = this->And<Order>(~mask);
  return bool(val & mask);
 }
//---------------------------------------------------------------------------    
 // Test bit value
 template<EMemoryOrder Order=moAcquire> bool BitTst(uint8 bit_index) const
 {
  const T mask = T(1) << bit_index;
  return bool(this->template Load<Order>() & mask);
 }
//---------------------------------------------------------------------------
// Toggle bit, return previous value
 template<EMemoryOrder Order=moSeqCst> bool BitFlip(uint8 bit_index)
 {
  T mask = T(1) << bit_index;
  T old  = AtomicXor<Order,true>(&this->Value, mask);  // this->Xor<true,Order>(mask);  // Force RetOld=true    
  return bool(old & mask);
 }
//---------------------------------------------------------------------------   
// Compare-and-swap on single bit
// Returns true if bit was expected_value and was changed to new_value
 template<EMemoryOrder Success=moSeqCst, EMemoryOrder Failure=moAcquire> bool BitCAS(uint8 bit_index, bool expected_value, bool new_value)
 {
  T mask = T(1) << bit_index;
  T current = this->template Load<moRelaxed>();
  for(;;) {
    bool current_bit = bool(current & mask);
    if(current_bit  != expected_value) return false;  // Bit is not expected value
    T desired = new_value ? (current | mask) : (current & ~mask);
    if(this->CmpExch<true,Success,Failure>(current, desired)) return true;
    // current was updated by compare_exchange_weak, retry
   }
 }

 // ========================================================================
 // Counter operations (using bit range)
 // ========================================================================
 
 // Get counter value from bit range [bit_start, bit_start + bit_count)
 template<EMemoryOrder Order=moSeqCst> T CtrGet(uint8 bit_start, uint8 bit_count) const
 {
  T mask = MakeRangeMsk(bit_start, bit_count); 
  return (this->template Load<Order>() & mask) >> bit_start;
 }
//---------------------------------------------------------------------------    
// Set counter value in bit range
 template<EMemoryOrder Order=moSeqCst> T CtrSet(uint8 bit_start, uint8 bit_count, T new_value)
 {
  T mask = MakeRangeMsk(bit_start, bit_count); 
  T value_shifted = (new_value << bit_start) & mask;
  T current = this->template Load<moRelaxed>();
  T desired;
  do {
   desired = (current & ~mask) | value_shifted;
  } while (!this->CmpExch<true,Order,moRelaxed>(current, desired));
  return (current & mask) >> bit_start;  // Return old value
 }
//---------------------------------------------------------------------------    
// Increment counter in bit range, return old value    // Clamp?
 template<EMemoryOrder Order=moSeqCst> T CtrInc(uint8 bit_start, uint8 bit_count, T delta = 1)
 {
  T mask = MakeRangeMsk(bit_start, bit_count); 
  T current = this->template Load<moRelaxed>();
  T desired;      
  do {
   T counter = (current & mask) >> bit_start;  
   T new_counter = counter + delta;
   desired = (current & ~mask) | ((new_counter << bit_start) & mask);
  } while (!this->CmpExch<true,Order,moRelaxed>(current, desired));
  return (current & mask) >> bit_start;
 }
//---------------------------------------------------------------------------    
// Decrement counter in bit range, return old value     // Clamp?
 template<EMemoryOrder Order=moSeqCst> T CtrDec(uint8 bit_start, uint8 bit_count, T delta = 1)
 {
  T mask = MakeRangeMsk(bit_start, bit_count); 
  T current = this->template Load<moRelaxed>();
  T desired;
  do {
   T counter = (current & mask) >> bit_start;   
   T new_counter = counter - delta;
   desired = (current & ~mask) | ((new_counter << bit_start) & mask);
  } while (!this->CmpExch<true,Order,moRelaxed>(current, desired)); 
  return (current & mask) >> bit_start;
 }
//---------------------------------------------------------------------------    
// Compare-and-swap on counter in bit range
 template<EMemoryOrder Success=moAcqRel, EMemoryOrder Failure=moAcquire> bool CtrCAS(uint8 bit_start, uint8 bit_count, T expected_value, T new_value)
 {
  T mask = MakeRangeMsk(bit_start, bit_count); 
  T expected_shifted = (expected_value << bit_start) & mask;
  T new_shifted = (new_value << bit_start) & mask;
  T current = this->template Load<moRelaxed>();
  for(;;) {
   if((current & mask) != expected_shifted) return false;  // Counter doesn't match expected
   T desired = (current & ~mask) | new_shifted;
   if(this->CmpExch<true,Success,Failure>(current, desired)) return true;     
  }
 }
//---------------------------------------------------------------------------
};
//};
//============================================================================================================

 /*
https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html

   // Such things increase compilation time
#ifdef _X64BIT
 static_assert((sizeof(N) == sizeof(char))&&(sizeof(N) == sizeof(short))&&(sizeof(N) == sizeof(long))&&(sizeof(N) == sizeof(long long)), "Operand size mismatch");
#else
 static_assert((sizeof(N) == sizeof(char))&&(sizeof(N) == sizeof(short))&&(sizeof(N) == sizeof(long)), "Operand size mismatch");
#endif

https://stackoverflow.com/questions/286629/what-is-a-memory-fence
The Linux kernel uses a gcc extension (asm __volatile__("": : :"memory")) to create a full compiler optimization barrier.
(.NET CLR) volatile reads are acquire fences, writes are release fences. Interlocked ops are full as is the MemoryBarrier method.

https://www.albahari.com/threading/part4.aspx#_NonBlockingSynch

std::atomic

Early AMD64 processors lacked the CMPXCHG16B instruction.

https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html

These built-in functions perform the operation suggested by the name, and returns the value that had previously been in memory.
Built-in Function: type __sync_fetch_and_add (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_sub (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_or (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_and (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_xor (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_nand (type *ptr, type value, ...)

These built-in functions perform the operation suggested by the name, and return the new value.
Built-in Function: type __sync_add_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_sub_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_or_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_and_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_xor_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_nand_and_fetch (type *ptr, type value, ...)     // GCC 4.4 and later implement __sync_nand_and_fetch as *ptr = ~(*ptr & value) instead of *ptr = ~*ptr & value.

warning: implicit use of sequentially-consistent atomic may incur stronger memory barriers than necessary [-Watomic-implicit-seq-cst]    // NOTE: All above are have 'seq-cst' memory model implicitly

// These built-in functions perform an atomic compare and swap. That is, if the current value of *ptr is oldval, then write newval into *ptr.
// The `bool` version returns true if the comparison is successful and newval is written. The `val` version returns the contents of *ptr before the operation.

https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html  // C++11 memory model
// Note that the `__atomic` builtins assume that programs will conform to the C++11 memory model. In particular, they assume that programs are free of data races. See the C++11 standard for detailed requirements.
// The `__atomic` builtins can be used with any integral scalar or pointer type that is 1, 2, 4, or 8 bytes in length. 16-byte integral types are also allowed if `__int128` (see 128-bit Integers) is supported by the architecture.
          __atomic_add_fetch (&Ptr[2], 4, 0); //__sync_fetch_and_add(&Ptr[2], 4);

https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html

Built-in Function: type __atomic_add_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_sub_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_and_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_xor_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_or_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_nand_fetch (type *ptr, type val, int memorder)

Built-in Function: type __atomic_fetch_add (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_sub (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_and (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_xor (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_or (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_nand (type *ptr, type val, int memorder)

Built-in Function: type __atomic_load_n (type *ptr, int memorder)
This built-in function implements an atomic load operation. It returns the contents of *ptr.

The valid memory order variants are __ATOMIC_RELAXED, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE, and __ATOMIC_CONSUME.

Built-in Function: void __atomic_load (type *ptr, type *ret, int memorder)
This is the generic version of an atomic load. It returns the contents of *ptr in *ret.

Built-in Function: void __atomic_store_n (type *ptr, type val, int memorder)
This built-in function implements an atomic store operation. It writes val into *ptr.

The valid memory order variants are __ATOMIC_RELAXED, __ATOMIC_SEQ_CST, and __ATOMIC_RELEASE.

Built-in Function: void __atomic_store (type *ptr, type *val, int memorder)
This is the generic version of an atomic store. It stores the value of *val into *ptr.

Built-in Function: type __atomic_exchange_n (type *ptr, type val, int memorder)
This built-in function implements an atomic exchange operation. It writes val into *ptr, and returns the previous contents of *ptr.

All memory order variants are valid.

Built-in Function: void __atomic_exchange (type *ptr, type *val, type *ret, int memorder)
This is the generic version of an atomic exchange. It stores the contents of *val into *ptr. The original value of *ptr is copied into *ret.

Built-in Function: bool __atomic_compare_exchange_n (type *ptr, type *expected, type desired, bool weak, int success_memorder, int failure_memorder)
This built-in function implements an atomic compare and exchange operation. This compares the contents of *ptr with the contents of *expected. If equal, the operation is a read-modify-write operation that writes desired into *ptr. If they are not equal, the operation is a read and the current contents of *ptr are written into *expected. weak is true for weak compare_exchange, which may fail spuriously, and false for the strong variation, which never fails spuriously. Many targets only offer the strong variation and ignore the parameter. When in doubt, use the strong variation.

If desired is written into *ptr then true is returned and memory is affected according to the memory order specified by success_memorder. There are no restrictions on what memory order can be used here.

Otherwise, false is returned and memory is affected according to failure_memorder. This memory order cannot be __ATOMIC_RELEASE nor __ATOMIC_ACQ_REL. It also cannot be a stronger order than that specified by success_memorder.

Built-in Function: bool __atomic_compare_exchange (type *ptr, type *expected, type *desired, bool weak, int success_memorder, int failure_memorder)
This built-in function implements the generic version of __atomic_compare_exchange. The function is virtually identical to __atomic_compare_exchange_n, except the desired value is also a pointer.

*/
//============================================================================================================
