
//------------------------------------------------------------------------------------------------------------
struct WSYN
{
 enum ENodeFlg: uint32                    
  {
   nfLkExclusive  = 0,   // Exclusive SRW lock
   nfStillAwake   = 1,   // This prevents the classic lost wakeup problem: Wake arrives after enqueue but before sleep. Without this flag, thread would miss the wake and sleep forever. With flag: thread sees wake happened, skips sleep.
   nfDetached     = 2,   // Detached    // This node is not part of the list anymore and its waiter proc is safe to exit (The node is on its stack)
  };

 struct alignas(16) SWaitNode   // We steal low four bits     // Used for SRW Locks too
 {
  SWaitNode* Next; 
  SWaitNode* Prev; 
  SWaitNode* Back; 
  usize      ThreadID;  
  sint32     LockState;
  uint32     Flags;
  vptr       LockObj;     // PRTL_SRWLOCK
 };
 static_assert(alignof(SWaitNode) == 16);
//============================================================================================================
template<uint32 Features, typename SO> class CWaitList;

class SDummySyncObj    // Just an interface 
{
 template <uint32 Features, typename SO> friend class CWaitList;
protected:
// This means: "Don't wake this thread now. Instead, transfer it to the lock's wait queue so it gets woken when the lock is released."
// Direct wake would be wrong. the thread would: wake up, immediately attempt to run, but it does NOT own the SRW lock yet.
// That violates this invariant: A thread returning from SleepConditionVariableSRW must hold the SRW lock.
// Lock queueing preserves FIFO ordering
//
 bool QueueWaitNode(SWaitNode* Node, uint32 Count){return false;}
public:
 int  Acquire(bool shared=false) {return 0;}
 int  Release(bool shared=false) {return 0;}
};
//============================================================================================================
static void RtlBackoff(uint32* aCounter)
{
 uint32 vCtr = *aCounter;
 if( vCtr )
  {
   if( vCtr < 0x1FFF )vCtr *= 2;
  }
 else
  {
   if( NT::NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors == 1 )return;
   vCtr = 64;
  }
 *aCounter = vCtr;
 uint64 vTCtr = __rdtsc();   // TODO: ARM
 uint32 vIdx  = 0;
 uint32 vYCtr = ((vCtr - 1) & vTCtr) + vCtr;   // Win10:  vYCtr = 10 * (((vCtr - 1) & (unsigned int)vTCtr) + vCtr) / MEMORY[0x7FFE02D6];// CyclesPerYield (1903 and higher)
 if ( vYCtr )
  {
    do
     {
      YieldCPU();
      ++vIdx;
     }
      while( vIdx < vYCtr );
  }
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Win7 keyed events wait on the node pointer itself, but NtWaitForAlertByThreadId wait on Node->LockObj
// NOTE: We will have some performance overhead for keyed events because they are already managed by a linked list(in the kernel)
// Can multiple waiters use the same Sync object?
static NT::NTSTATUS DoWait(SWaitNode* Node, uint64 TimeoutMs)    
{
 NT::LARGE_INTEGER  nstimeout;
 NT::LARGE_INTEGER* ptimeout = nullptr;
 // Convert milliseconds → relative NT timeout (100-ns units, negative)
 if(TimeoutMs != uint64(-1))    // 1 ms = 10,000 * 100 ns
  {
   nstimeout = -(TimeoutMs * uint64(10'000));
   ptimeout  = &nstimeout;
  }
 if(!fwsinf.hKeyedEvent) 
  {
   NT::NTSTATUS res = SAPI::NtWaitForAlertByThreadId(Node, ptimeout);
   return (res == NT::STATUS_ALERTED) ? NT::STATUS_SUCCESS : res;
  }
   else return SAPI::NtWaitForKeyedEvent((NT::HANDLE)fwsinf.hKeyedEvent, Node, 0, ptimeout);	// Not alertable (By APC)
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS DoWake(SWaitNode* Node)    
{
 if(!fwsinf.hKeyedEvent)return SAPI::NtAlertThreadByThreadId(Node->ThreadID); 
   else return SAPI::NtReleaseKeyedEvent((NT::HANDLE)fwsinf.hKeyedEvent, Node, 0, nullptr);	   // Not alertable 
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS DoSync(uint32* Flags, uint8 BitIdx, uint32 SpinCnt=100)  // Multi-processor support
{
 //if( MEMORY[0x7FFE036A] > 1u )                // NumberOfProcessors (SMP)
  {
   /*if( MEMORY[0x7FFE0297] )                   // flHaveMonitor
    {
      v14 = __rdtsc();
      v15 = v14 + (unsigned int)SpinCnt;
      while ( 1 )
      {
        __asm { monitorx rax, rcx, rdx }
        if ( (Node->Flags & 2) == 0 ) break;
        v16 = v14;
        v17 = __rdtsc();
        v14 = v17;
        if ( v17 <= v16 || v17 >= v15 ) break;
        __asm { mwaitx  rax, rcx, rbx }
      }
    }
   else  */
    {
      for (uint32 i = 0; (*Flags & (1 << BitIdx)) != 0 /*&& i != SpinCnt / (unsigned int)MEMORY[0x7FFE02D6]*/; ++i )
      {
        YieldCPU();
      }
    }
  }
/*
  v15 = 0LL;
  v18 = 0LL;
  v17 = 2;
  UniqueThread = NtCurrentTeb()->ClientId.UniqueThread;
  while ( 1 )
  {
    v7 = (unsigned __int64)&v13 | v6 & 0xF;
    v13 = v6 & 0xFFFFFFFFFFFFFFF0uLL;
    if ( (v6 & 0xFFFFFFFFFFFFFFF0uLL) != 0 )
    {
      v14 = 0LL;
      v7 |= 8uLL;
    }
    else
    {
      v14 = &v13;
    }
    v8 = _InterlockedCompareExchange64(aCondVar, v7, v6);
    if ( v6 == v8 )
      break;
    v6 = v8;
  }
  RtlLeaveCriticalSection(aCritSec);
  if ( (((unsigned __int8)v6 ^ (unsigned __int8)v7) & 8) != 0 )
    RtlpOptimizeConditionVariableWaitList(aCondVar, v7);
  for ( i = ConditionVariableSpinCount; i; --i )
  {
    if ( (v17 & 2) == 0 )
      break;
    _mm_pause();
  }
*/
 return 0;
}
//============================================================================================================
struct SRW
{
 usize HeadDesc;

//------------------------------------------------------------------
void UpdateClonedSRWLock(bool aShared)
{
 this->HeadDesc = aShared ? 17 : 1;
}
//------------------------------------------------------------------
bool TryAcquireSRWLockExclusive(void)
{
 return !AtomicBitSet(&this->HeadDesc, 0);    // _interlockedbittestandset64
}
//------------------------------------------------------------------
void AcquireReleaseSRWLockExclusive(void)
{
// signed __int32 v2[10]; // [rsp+0h] [rbp-28h] BYREF
// _InterlockedOr(v2, 0);
 if( (this->HeadDesc & 1) )
 {
 // this->AcquireSRWLockExclusive();
 // this->ReleaseSRWLockExclusive();
 }
}
//------------------------------------------------------------------
bool ConvertSRWLockExclusiveToShared(void)
{
 usize vPtrAndFlags = 1; 
 if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vPtrAndFlags,  17)) return true;                
//  if ( (vPtrAndFlags & 1) == 0 )RtlRaiseStatus(0xC0000264);
 if(!AtomicBitSet(&this->HeadDesc, 2))      // _interlockedbittestandset64
  {
  // this->WakeSRWLock(vPtrAndFlags | 4, 1);
   return true;
  }
 return false;
}
//------------------------------------------------------------------
bool TryConvertSRWLockSharedToExclusiveOrRelease(void)
{
 usize vPtrAndFlags = this->HeadDesc;
 //if( (vPtrAndFlags & 1) == 0 ) RtlRaiseStatus(0xC0000264LL);
 while( (vPtrAndFlags & 2) == 0 )
  {
   if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vPtrAndFlags, usize(vPtrAndFlags - 16))) return (vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) == 16;
  }
 if( (vPtrAndFlags & 8) != 0 )
  {
   SWaitNode *vPrvNode;
   for(SWaitNode* vNode = (SWaitNode *)(vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL); ; vNode = vNode->Next )
    {
     vPrvNode = vNode->Prev;
     if( vPrvNode ) break;
    }
   if( AtomicAdd<moSeqCst,true>(&vPrvNode->LockState, 0xFFFFFFFF) > 1 ) return 0;      // _InterlockedExchangeAdd
   AtomicAdd<moSeqCst,true>(&this->HeadDesc, 0xFFFFFFFFFFFFFFF8uLL);         // _InterlockedExchangeAdd64
  }
 return 1;
}
//------------------------------------------------------------------
/*char __fastcall RtlTryAcquireSRWLockShared(volatile signed __int64 *aSrwLock)
{
  char v1; // r9
  volatile signed __int64 *vSrwLock; // r10
  unsigned __int64 vPtrAndFlags; // rax
  __int64 vFlag; // r8
  signed __int64 v6; // rcx
  unsigned int aCounter; // [rsp+30h] [rbp+8h] BYREF

  v1 = 0;
  vSrwLock = aSrwLock;
  aCounter = 0;
  vPtrAndFlags = _InterlockedCompareExchange64(aSrwLock, 17LL, 0LL);
  if ( !vPtrAndFlags )
    return 1;
  while ( 1 )
  {
    vFlag = (vPtrAndFlags >> 1) & 1;
    if ( (vPtrAndFlags & 1) != 0 && (vFlag || (vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) == 0) )
      break;
    v6 = (vPtrAndFlags | 1) + 16;
    if ( vFlag )
      v6 = vPtrAndFlags | 1;
    if ( vPtrAndFlags == _InterlockedCompareExchange64(vSrwLock, v6, vPtrAndFlags) )
      return 1;
    RtlBackoff(&aCounter);
    _m_prefetchw((const void *)vSrwLock);
    vPtrAndFlags = *vSrwLock;
  }
  return v1;
}
//------------------------------------------------------------------
void __fastcall RtlpOptimizeSRWLockList(volatile signed __int64 *aSRWLock, signed __int64 aPtrAndFlags)
{
  signed __int64 vPtrAndFlags; // rax
  SWaitNode *vCurNode; // r8
  SWaitNode *v5; // rcx
  SWaitNode *vNext; // rcx
  signed __int64 v7; // rtt

  vPtrAndFlags = aPtrAndFlags;
  if ( (aPtrAndFlags & 1) != 0 )
  {
    while ( 1 )
    {
      vCurNode = (SWaitNode *)(vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL);
      if ( !*(_QWORD *)((vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) + 8) )
      {
        do
        {
          v5 = vCurNode;
          vCurNode = vCurNode->Next;
          vCurNode->Back = (__int64)v5;
          vNext = vCurNode->Prev;
        }
        while ( !vNext );
        if ( vCurNode != (SWaitNode *)(vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) )
          *(_QWORD *)((vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) + 8) = vNext;
      }
      v7 = vPtrAndFlags;
      vPtrAndFlags = _InterlockedCompareExchange64(aSRWLock, vPtrAndFlags - 4, vPtrAndFlags);
      if ( v7 == vPtrAndFlags )
        break;
      if ( (vPtrAndFlags & 1) == 0 )
        goto LABEL_8;
    }
  }
  else
  {
LABEL_8:
    RtlpWakeSRWLock(aSRWLock, vPtrAndFlags, 0);
  }
}
//------------------------------------------------------------------
bool __fastcall RtlpQueueWaitBlockToSRWLock(SWaitNode *aNode, size_t **aSrwLock, __int64 aCount)
{
  SWaitNode *vResult; // r9
  unsigned __int64 vSrwHeadAndFlags; // rax
  char vSharedSRW; // bl
  size_t **vSrwLock; // r11
  SWaitNode *vNode; // r10
  bool vNeedOptimize; // r8
  __int64 v10; // rdx
  signed __int64 vNewTaggedPtr; // rdx
  unsigned int aCounter; // [rsp+30h] [rbp+8h] BYREF

  vResult = 0LL;
  vSrwHeadAndFlags = (unsigned __int64)*aSrwLock;
  vSharedSRW = aNode->Flags & 1;
  aCounter = 0;
  vSrwLock = aSrwLock;
  vNode = aNode;
  while ( (vSrwHeadAndFlags & 1) != 0
       && (vSharedSRW || (vSrwHeadAndFlags & 2) != 0 || (vSrwHeadAndFlags & 0xFFFFFFFFFFFFFFF0uLL) == 0) )
  {
    vNode->Back = (__int64)vResult;
    vNeedOptimize = (char)vResult;
    if ( (vSrwHeadAndFlags & 2) != 0 )
    {
      vNode->SrwState = -1;
      vNode->Prev = vResult;
      vNode->Next = (SWaitNode *)(vSrwHeadAndFlags & 0xFFFFFFFFFFFFFFF0uLL);
      vNewTaggedPtr = (unsigned __int64)vNode | vSrwHeadAndFlags & 8 | 7;
      vNeedOptimize = (vSrwHeadAndFlags & 4) == 0;
    }
    else
    {
      v10 = 11LL;
      vNode->Prev = vNode;
      vNode->SrwState = vSrwHeadAndFlags >> 4;
      if ( (int)(vSrwHeadAndFlags >> 4) <= 1 )
        v10 = 3LL;
      vNewTaggedPtr = (unsigned __int64)vNode | v10;
      if ( !(unsigned int)(vSrwHeadAndFlags >> 4) )
        vNode->SrwState = -2;
    }
    if ( vSrwHeadAndFlags == _InterlockedCompareExchange64(
                               (volatile signed __int64 *)vSrwLock,
                               vNewTaggedPtr,
                               vSrwHeadAndFlags) )
    {
      if ( vNeedOptimize )
        RtlpOptimizeSRWLockList((volatile signed __int64 *)vSrwLock, vNewTaggedPtr);
      LOBYTE(vResult) = 1;
      return (char)vResult;
    }
    RtlBackoff(&aCounter);
    _m_prefetchw(vSrwLock);
    vSrwHeadAndFlags = (unsigned __int64)*vSrwLock;
  }
  return (char)vResult;
}
//------------------------------------------------------------------
void __fastcall RtlpWakeSRWLock(volatile signed __int64 *aSRWLock, signed __int64 aPtrAndFlags, char aFlag)
{
  SWaitNode *vCurrNode; // r10
  SWaitNode *vNext; // r8
  __int64 Back; // rax
  bool v7; // zf
  signed __int64 v8; // rax
  SWaitNode *v9; // rbx
  __int64 ThreadID; // rcx
  signed __int64 v11; // rax
  SWaitNode *v12; // rax

  while ( aFlag || (aPtrAndFlags & 1) == 0 )
  {
LABEL_3:
    vCurrNode = (SWaitNode *)(aPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL);
    vNext = *(SWaitNode **)((aPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) + 8);
    if ( !vNext )
    {
      do
      {
        v12 = vCurrNode;
        vCurrNode = vCurrNode->Next;
        vCurrNode->Back = (__int64)v12;
        vNext = vCurrNode->Prev;
      }
      while ( !vNext );
      if ( vCurrNode != (SWaitNode *)(aPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) )
        *(_QWORD *)((aPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) + 8) = vNext;
    }
    if ( (vNext->Flags & 1) != 0 )
    {
      if ( aFlag )
      {
        _InterlockedAnd64(aSRWLock, 0xFFFFFFFFFFFFFFFBuLL);
        return;
      }
      Back = vNext->Back;
      if ( Back )
      {
        *(_QWORD *)((aPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) + 8) = Back;
        vNext->Back = 0LL;
        _InterlockedAnd64(aSRWLock, 0xFFFFFFFFFFFFFFFBuLL);
        do
        {
LABEL_8:
          v9 = (SWaitNode *)vNext->Back;
          ThreadID = vNext->ThreadID;
          _interlockedbittestandset(&vNext->Flags, 2u);
          if ( !_interlockedbittestandreset(&vNext->Flags, 1u) )
            ZwAlertThreadByThreadId(ThreadID);
          vNext = v9;
        }
        while ( v9 );
        return;
      }
    }
    v8 = _InterlockedCompareExchange64(aSRWLock, aFlag != 0 ? 0x11 : 0, aPtrAndFlags);
    v7 = aPtrAndFlags == v8;
    aPtrAndFlags = v8;
    if ( v7 )
      goto LABEL_8;
  }
  while ( 1 )
  {
    v11 = _InterlockedCompareExchange64(aSRWLock, aPtrAndFlags - 4, aPtrAndFlags);
    v7 = aPtrAndFlags == v11;
    aPtrAndFlags = v11;
    if ( v7 )
      break;
    if ( (v11 & 1) == 0 )
      goto LABEL_3;
  }
}
//------------------------------------------------------------------
void __fastcall RtlReleaseSRWLockExclusive(volatile signed __int64 *aSrwLock)
{
  signed __int64 vPtrAndFlags; // rax
  signed __int64 v2; // r8
  __int64 v3; // rdx
  signed __int64 v4; // rdx
  signed __int64 v5; // rtt

  vPtrAndFlags = _InterlockedCompareExchange64(aSrwLock, 0LL, 1LL);
  if ( vPtrAndFlags != 1 )
  {
    do
    {
      v2 = vPtrAndFlags & 6;
      v3 = 3LL;
      if ( v2 != 2 )
        v3 = -1LL;
      v4 = vPtrAndFlags + v3;
      v5 = vPtrAndFlags;
      vPtrAndFlags = _InterlockedCompareExchange64(aSrwLock, v4, vPtrAndFlags);
    }
    while ( v5 != vPtrAndFlags );
    if ( v2 == 2 )
      RtlpWakeSRWLock(aSrwLock, v4, 0);
  }
}
//------------------------------------------------------------------
void __fastcall RtlReleaseSRWLockShared(volatile signed __int64 *aSrwLock)
{
  signed __int64 vPtrAndFlags; // rax
  signed __int64 v2; // r9
  signed __int64 v3; // rtt
  __int64 v4; // r8
  __int64 v5; // rdx
  signed __int64 v6; // r9
  signed __int64 v7; // rdx
  signed __int64 v8; // rtt
  SWaitNode *vNode; // rdx
  SWaitNode *i; // r9

  vPtrAndFlags = _InterlockedCompareExchange64(aSrwLock, 0LL, 17LL);
  if ( vPtrAndFlags == 17 )
    return;
  if ( (vPtrAndFlags & 1) == 0 )
    RtlRaiseStatus(0xC0000264LL);
  if ( (vPtrAndFlags & 2) != 0 )
  {
LABEL_9:
    if ( (vPtrAndFlags & 8) != 0 )
    {
      vNode = (SWaitNode *)(vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL);
      for ( i = *(SWaitNode **)((vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) + 8); !i; i = vNode->Prev )
        vNode = vNode->Next;
      if ( _InterlockedExchangeAdd(&i->SrwState, 0xFFFFFFFF) > 1 )
        return;
      v4 = -9LL;
    }
    else
    {
      v4 = -1LL;
    }
    do
    {
      v5 = v4 + 4;
      v6 = vPtrAndFlags & 6;
      if ( v6 != 2 )
        v5 = v4;
      v7 = vPtrAndFlags + v5;
      v8 = vPtrAndFlags;
      vPtrAndFlags = _InterlockedCompareExchange64(aSrwLock, v7, vPtrAndFlags);
    }
    while ( v8 != vPtrAndFlags );
    if ( v6 == 2 )
      RtlpWakeSRWLock(aSrwLock, v7, 0);
    return;
  }
  while ( 1 )
  {
    v2 = 0LL;
    if ( (vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) != 0x10 )
      v2 = vPtrAndFlags - 16;
    v3 = vPtrAndFlags;
    vPtrAndFlags = _InterlockedCompareExchange64(aSrwLock, v2, vPtrAndFlags);
    if ( v3 == vPtrAndFlags )
      break;
    if ( (vPtrAndFlags & 2) != 0 )
      goto LABEL_9;
  }
}
//------------------------------------------------------------------
void __fastcall RtlAcquireSRWLockExclusive(unsigned __int64 *aSrwLock)
{
  unsigned __int64 vPtrAndFlags; // rdi
  bool vNeedOpt; // cl
  __int64 v4; // rdx
  unsigned __int64 vNewHdrAndFlg; // rdx
  bool v6; // zf
  signed __int64 v7; // rax
  int i; // edx
  unsigned __int64 v10; // r8
  unsigned __int64 v11; // r9
  unsigned __int64 v12; // rcx
  unsigned __int64 v13; // rax
  SWaitNode vSrwBlock; // [rsp+20h] [rbp-48h] BYREF
  unsigned int aCounter; // [rsp+70h] [rbp+8h] BYREF

  aCounter = 0;
  if ( _interlockedbittestandset64((volatile signed __int32 *)aSrwLock, 0LL) )
  {
    vPtrAndFlags = *aSrwLock;
    while ( 1 )
    {
      if ( (vPtrAndFlags & 1) != 0 )
      {
        if ( RtlpWaitCouldDeadlock() )
          ZwTerminateProcess(-1LL, 0xC000004BLL);
        vSrwBlock.ThreadID = (__int64)NtCurrentTeb()->ClientId.UniqueThread;
        vNeedOpt = 0;
        vSrwBlock.Flags = 3;
        vSrwBlock.Back = 0LL;
        if ( (vPtrAndFlags & 2) != 0 )
        {
          vSrwBlock.Prev = 0LL;
          vSrwBlock.SrwState = -1;
          vSrwBlock.Next = (SWaitNode *)(vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL);
          vNewHdrAndFlg = (unsigned __int64)&vSrwBlock | vPtrAndFlags & 8 | 7;
          vNeedOpt = (vPtrAndFlags & 4) == 0;
        }
        else
        {
          v4 = 11LL;
          vSrwBlock.Prev = &vSrwBlock;
          vSrwBlock.SrwState = vPtrAndFlags >> 4;
          if ( vSrwBlock.SrwState <= 1 )
            v4 = 3LL;
          vNewHdrAndFlg = (unsigned __int64)&vSrwBlock | v4;
          if ( !(unsigned int)(vPtrAndFlags >> 4) )
            vSrwBlock.SrwState = -2;
        }
        v7 = _InterlockedCompareExchange64((volatile signed __int64 *)aSrwLock, vNewHdrAndFlg, vPtrAndFlags);
        v6 = vPtrAndFlags == v7;
        vPtrAndFlags = v7;
        if ( !v6 )
          goto LABEL_13;
        if ( vNeedOpt )
          RtlpOptimizeSRWLockList((volatile signed __int64 *)aSrwLock, vNewHdrAndFlg);
        if ( MEMORY[0x7FFE036A] > 1u )
        {
          if ( MEMORY[0x7FFE0297] )
          {
            v10 = __rdtsc();
            v11 = v10 + (unsigned int)SRWLockSpinCycleCount;
            while ( 1 )
            {
              __asm { monitorx rax, rcx, rdx }
              if ( (vSrwBlock.Flags & 2) == 0 )
                break;
              v12 = v10;
              v13 = __rdtsc();
              v10 = v13;
              if ( v13 <= v12 || v13 >= v11 )
                break;
              __asm { mwaitx  rax, rcx, rbx }
            }
          }
          else
          {
            for ( i = 0; (vSrwBlock.Flags & 2) != 0 && i != SRWLockSpinCycleCount / (unsigned int)MEMORY[0x7FFE02D6]; ++i )
              _mm_pause();
          }
        }
        if ( _interlockedbittestandreset(&vSrwBlock.Flags, 1u) )
        {
          do
            NtWaitForAlertByThreadId(aSrwLock, 0LL);
          while ( (vSrwBlock.Flags & 4) == 0 );
        }
      }
      else
      {
        if ( vPtrAndFlags == _InterlockedCompareExchange64(
                               (volatile signed __int64 *)aSrwLock,
                               vPtrAndFlags + 1,
                               vPtrAndFlags) )
          return;
LABEL_13:
        RtlBackoff(&aCounter);
        _m_prefetchw(aSrwLock);
        vPtrAndFlags = *aSrwLock;
      }
    }
  }
}
//------------------------------------------------------------------
void __fastcall RtlAcquireSRWLockShared(volatile signed __int64 *aSrwLock)
{
  volatile unsigned __int64 vPtrAndFlags; // rdi
  __int64 vFlag; // rbx
  signed __int64 vMarkedHdr; // rcx
  bool vNeedOpt; // cl
  char *vNewHdrAndFlg; // rdx
  bool v7; // zf
  signed __int64 v8; // rax
  int i; // edx
  unsigned __int64 v11; // r8
  unsigned __int64 v12; // r9
  unsigned __int64 v13; // rcx
  unsigned __int64 v14; // rax
  SWaitNode vSrwBlock; // [rsp+20h] [rbp-38h] BYREF
  unsigned int aCounter; // [rsp+60h] [rbp+8h] BYREF

  aCounter = 0;
  vPtrAndFlags = _InterlockedCompareExchange64(aSrwLock, 17LL, 0LL);
  if ( vPtrAndFlags )
  {
    while ( 1 )
    {
      vFlag = (vPtrAndFlags >> 1) & 1;
      if ( (vPtrAndFlags & 1) != 0 && (vFlag || (vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL) == 0) )
      {
        if ( RtlpWaitCouldDeadlock() )
          ZwTerminateProcess(-1LL, 0xC000004BLL);
        vSrwBlock.ThreadID = (__int64)NtCurrentTeb()->ClientId.UniqueThread;
        vNeedOpt = 0;
        vSrwBlock.Flags = 2;
        vSrwBlock.Back = 0LL;
        if ( vFlag )
        {
          vSrwBlock.Prev = 0LL;
          vSrwBlock.SrwState = -1;
          vSrwBlock.Next = (SWaitNode *)(vPtrAndFlags & 0xFFFFFFFFFFFFFFF0uLL);
          vNewHdrAndFlg = (char *)((unsigned __int64)&vSrwBlock | vPtrAndFlags & 8 | 7);
          vNeedOpt = (vPtrAndFlags & 4) == 0;
        }
        else
        {
          vSrwBlock.SrwState = -2;
          vSrwBlock.Prev = &vSrwBlock;
          vNewHdrAndFlg = (char *)&vSrwBlock.Next + 3;
        }
        v8 = _InterlockedCompareExchange64(aSrwLock, (signed __int64)vNewHdrAndFlg, vPtrAndFlags);
        v7 = vPtrAndFlags == v8;
        vPtrAndFlags = v8;
        if ( !v7 )
          goto LABEL_14;
        if ( vNeedOpt )
          RtlpOptimizeSRWLockList(aSrwLock, (signed __int64)vNewHdrAndFlg);
        if ( MEMORY[0x7FFE036A] > 1u )
        {
          if ( MEMORY[0x7FFE0297] )
          {
            v11 = __rdtsc();
            v12 = v11 + (unsigned int)SRWLockSpinCycleCount;
            while ( 1 )
            {
              __asm { monitorx rax, rcx, rdx }
              if ( (vSrwBlock.Flags & 2) == 0 )
                break;
              v13 = v11;
              v14 = __rdtsc();
              v11 = v14;
              if ( v14 <= v13 || v14 >= v12 )
                break;
              __asm { mwaitx  rax, rcx, rbx }
            }
          }
          else
          {
            for ( i = 0; (vSrwBlock.Flags & 2) != 0 && i != SRWLockSpinCycleCount / (unsigned int)MEMORY[0x7FFE02D6]; ++i )
              _mm_pause();
          }
        }
        if ( _interlockedbittestandreset(&vSrwBlock.Flags, 1u) )
        {
          do
            NtWaitForAlertByThreadId((HANDLE)aSrwLock, 0LL);
          while ( (vSrwBlock.Flags & 4) == 0 );
        }
      }
      else
      {
        vMarkedHdr = (vPtrAndFlags | 1) + 16;
        if ( vFlag )
          vMarkedHdr = vPtrAndFlags | 1;
        if ( vPtrAndFlags == _InterlockedCompareExchange64(aSrwLock, vMarkedHdr, vPtrAndFlags) )
          return;
LABEL_14:
        RtlBackoff(&aCounter);
        _m_prefetchw((const void *)aSrwLock);
        vPtrAndFlags = *aSrwLock;
      }
    }
  }
}
*/
};
//============================================================================================================
// TODO: Faster path for single-waiter cases
// NOTE: Return codes are from PX
// NOTE: Heavy use of moSeqCst everywhere may be overly conservative. Some operations might work with weaker orderings (Acquire/Release), improving performance on ARM/weak-memory architectures.
//
enum EWLFeatures: uint32
{
 wlfWakeInFIFO = 1,    // Fairly wake waiters using FIFO (Requires backling management) 
 wlfCheckVar   = 2     // Check conditional variable, Futex semantics.  (Adds 8 bytes to the object for the variable)
};

template<uint32 Features=uint32(-1), typename SO=SDummySyncObj> class CWaitList: TSW<bool(Features & wlfCheckVar), PX::futex_t, PX::futex_not>::T    // Size: 8/16 bytes
{
 SCVR uint32 SyncMsSMP = 100;

 enum EPTagFlg: usize   
  {                     
   tfInProggress = 3,   // A magic flag, it sticks until the head is replaced   // Wait path: List has multiple waiters, backlinks needed. Wake path: Wake operation in progress.
   tfValIncCtr   = 1,   // Wait() too holds tfInProggress when adds no non empty list while it optimizes the list.
   tfValInProggr = 1 << tfInProggress,
   tfPtrTagMask  = usize(0x0F),     // Bits 0,1,2,3
   tfWakeCtrMask = usize(7),        // Bits 0,1,2    // Max counter
   tfPtrMask     = ~tfPtrTagMask    // To extract pointer from a tagged pointer
  };

//------------------------------------------------------------------
/*
Before Optimization (single waiter, bit 3 = 0):
 - Simple singly-linked list (only Next pointers)
 - Head points to most recent waiter
 - No easy way to find the tail (oldest waiter)
 - To wake FIFO-style: must walk entire list O(n)

After Optimization (multiple waiters, bit 3 = 1):
 - head->Prev now points directly to the TAIL
 - Backward links built using Back field
 - Waking from tail becomes O(1)

 This ensures FIFO fairness - threads that waited longest get woken first.
 FIFO fairness prevents starvation: New threads push onto the head (LIFO insertion); Waking pulls from the tail (FIFO removal); Without this, newer threads could keep "cutting in line";

 Before optimization: Head -> newest -> older -> older -> ... -> oldest
 Since tfInProggress is set by the caller
*/
sint32 OptimizeWaitList(usize aNodePtrAndFlags)     // RebuildWakeList (multiple waiters, Prev (backward) linked list)
{
 for(;;)           // NOTE: No initial check of Wake counter. Supposed to be 0 
  {
   SWaitNode* vHeadNode = (SWaitNode*)(aNodePtrAndFlags & tfPtrMask);     // Not supposed to be NULL
   SWaitNode* vCurNode  = vHeadNode;
   while(!vCurNode->Prev)   // Rebuild back links   // The head node's Prev is temporarily null during races?
    {
     SWaitNode* prv = vCurNode;
     vCurNode = vCurNode->Next;
     vCurNode->Back = prv;
    }
   vHeadNode->Prev = vCurNode->Prev;      // Link to the head's Prev
   if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &aNodePtrAndFlags, vHeadNode)) return 0; // Clears entire tag, including tfInProggress  // No one has changed the head - set the new head and exit
   if(aNodePtrAndFlags & tfWakeCtrMask)   // The counter is non zero (Have pending wake requests)  // On CAS failure, aNodePtrAndFlags is updated by someone
    {
     return this->WakeMultiple(aNodePtrAndFlags, 0);    // Handle any pending wake requests from other threads
    }
  }
} 
//------------------------------------------------------------------
// Wakes one or more waiters  
// 'aCount == 0' - Consume all pending wake requests from other threads
// Bad Scenario: High-frequency condition variable   ???
// 
// Thread pool with 100 threads waiting:
// - 100 waiters enqueued
// - First wake: walks 100 nodes to rebuild backlinks
// - Wake 1 thread
// - Add another waiter (101 total)
// - Second wake: walks 101 nodes AGAIN!
// - Wake 1 thread
//
sint32 WakeMultiple(usize aNodePtrAndFlags, usize aCount)    // RtlpWakeConditionVariable
{
 SWaitNode*  vFrom = nullptr;    // List head for waking
 SWaitNode** pNext = &vFrom;
 for(usize vIndex  = 0;;)        // NOTE: This loop may be skipped if FIFO waking is not required (just do 'vFrom = (SWaitNode *)(aNodePtrAndFlags & tfPtrMask)')
  {
   SWaitNode* vHeadNode = (SWaitNode*)(aNodePtrAndFlags & tfPtrMask);   // NOTE: Wake() checks that aNodePtrAndFlags is not NULL
   usize      WakeCtr   = (aNodePtrAndFlags & tfWakeCtrMask);
   if(WakeCtr == tfWakeCtrMask)    // The wake counter is saturated - detach the head and proceed to waking everyone
    {  
     *pNext = (SWaitNode*)(AtomicExch(&this->HeadDesc, usize(0)) & tfPtrMask);      // May change vFrom    // May be AtomicCmpExch??? if(!AtomicCmpExch<false,moSeqCst,moSeqCst>(&this->HeadDesc, &aNodePtrAndFlags, usize(0)))continue;  // Retry
     if constexpr (!(Features & wlfWakeInFIFO)) aCount = usize(-1);  // Max counter
     break;
    }
   aCount += WakeCtr;        // Multiple concurrent wakes batch together
   SWaitNode* vCurNode = vHeadNode;

   if constexpr (Features & wlfWakeInFIFO) {
     while(!vCurNode->Prev)    // Rebuild back links 
      {
       SWaitNode* prv = vCurNode;    
       vCurNode = vCurNode->Next;
       vCurNode->Back = prv;
      }   
     for(SWaitNode* vPrevNode = vCurNode->Prev; aCount > vIndex; vIndex++)     // Reverse order of (aCount - vIndex) waiters in the list?
      {
       SWaitNode* Back = (SWaitNode*)vPrevNode->Back;
       if(!Back)    // No back link (End of the list?)
        {
         if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &aNodePtrAndFlags, usize(0)))   // Steal the head if it has not changed
          {
           *pNext = vPrevNode;   // May change vFrom   // Will start waking from this node
           vPrevNode->Next = nullptr;     // This node will be last to wake
           goto LblBreak;    // Could be OuterLoop:break
          }
         goto LblCont;       // Could be OuterLoop:continue     // Someone has changed the head
        }
       *pNext = vPrevNode;          // May change vFrom
       vPrevNode->Next = nullptr;   // This node will be last to wake
       pNext  = &vPrevNode->Next;   // pNext won't point to vFrom anymore   // We will be setting some node's Next to a node taken from Prev (Reversing the list)
       vHeadNode->Prev = Back;
       vPrevNode  = Back;
       Back->Next = nullptr;
      } 
    }
    else    // Can safely detach the entire list - the list is already locked by Wake ??? Wake() and WakeAll() already lock the list - no Wake should happen while we are here?
    {
     vFrom     = vHeadNode;
     aCount   -= vIndex;
     vIndex    = aCount;
     vHeadNode = nullptr;     // To detach the list

  /*   for(;aCount > vIndex; vIndex++)     // Is counting correct for walking Next?
      {
       pNext = &vCurNode->Next;
       vCurNode = *pNext;
       if(!vCurNode)    // No next node (End of the list, can detach it)
        {
         if(AtomicCmpExch<false,moSeqCst,moSeqCst>(&this->HeadDesc, &aNodePtrAndFlags, usize(0)))   // Detach the head if it has not changed
          {
           vFrom = vHeadNode;  // Will start waking from this node
           goto LblBreak;    // Could be OuterLoop:break
          }
         goto LblCont;       // Could be OuterLoop:continue      // Someone has changed the head
        }
      }
     if(vCurNode != vHeadNode) 
      {
       vFrom = vHeadNode;   // Need a   // Will start waking from this node
       vHeadNode = *pNext;     // Rest of the nodes will be new head
      }  */
    }
   // If aCount was 0, then this will just store aNodePtrAndFlags back but without its tag flags.  Why do this?  // It will ALWAYS wake at least one node that way!
   if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &aNodePtrAndFlags, (usize)vHeadNode)) break;  // Break if the head has not changed (Sets HeadDesc to vHeadNode, tag flags will be 0) otherwise get the changed head into aNodePtrAndFlags and retry
/*    {
     if constexpr (!(Features & wlfWakeInFIFO)) 
       if(vFrom) *pNext = nullptr;  // Safe to do after the head was replaced
     break;
    } */
LblCont:
  }

LblBreak: 
 sint32 Result = 0;      
 while(vFrom)    // Wake the entire list from vFrom (Head -> Tail)
  {
   if constexpr (!(Features & wlfWakeInFIFO)) 
     if(!aCount--) break;

   SWaitNode* Next = vFrom->Next;
   if(!AtomicBitClr(&vFrom->Flags, nfStillAwake))   // If nfStillAwake was zero already - wake   // Wait() sets it to 0 before going to sleep      // _interlockedbittestandreset
    {
     if( !vFrom->LockObj || !((SO*)(vFrom->LockObj))->QueueWaitNode(vFrom, aCount) )
      {
       AtomicBitSet<moSeqCst>(&vFrom->Flags, nfDetached);     // AtomicOr(&vFrom->Flags, 4u);  // _InterlockedOr
       DoWake(vFrom);
       Result++;            // The queue state ignores any NtAPI failures anyway     // TODO: increment Result on success only?  
      }
    }
   vFrom = Next;
  }

 if constexpr (!(Features & wlfWakeInFIFO)) 
  {
   if(vFrom)   // This node should be the new head
    {
     SWaitNode* Head  = vFrom;
     SWaitNode* Last  = Head;
     while((vFrom=Last->Next))Last = vFrom;    // Walk to last node
     aNodePtrAndFlags = this->HeadDesc;
     for(;;) {
        Last->Next = (SWaitNode*)(aNodePtrAndFlags & tfPtrMask);  // Will continue at the current head (The old piece of the list will come first)
        usize NewPtrAndFlags = (usize)Head | (aNodePtrAndFlags & tfPtrTagMask);
        if( AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &aNodePtrAndFlags, NewPtrAndFlags) ) break;  // break if we won the race and replaced the head
      }     
     // Preserve tags
    }
  }
 return Result;
}
//------------------------------------------------------------------
// Used by Wait() to remove self after time out   // Why 'Wake()' uses 'WakeMultiple(1)' instead?
// Used to remove Self from the list. Looks up for aNode by walking the list
// Not quite optimal to actually wake a single node. WakeMultiple(1) will do this better
//
bool WakeSingle(SWaitNode* aNode)
{
 usize vCurrPtrAndFlags;
 usize vHeadPtrAndFlags = this->HeadDesc;
 do { for(;;)
   {
    if(!vHeadPtrAndFlags || IsSetFlags(vHeadPtrAndFlags, tfWakeCtrMask)) return false;   // There is no waiter nodes or someone is already waking everyone
    if(!IsSetFlag(vHeadPtrAndFlags, tfInProggress)) break;    // The head is not busy - proceed to attempt to take its ownership
    if(IsSetFlag(aNode->Flags, nfDetached)) return true;      // ADDED by me     // To wake aNode on the busy queue means to wake everyone!
    if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadPtrAndFlags,  AddFlags(vHeadPtrAndFlags, tfWakeCtrMask))) return false;  // The head is busy, saturate the counter to wake everyone and exit if the head hasn't changed
   }
  vCurrPtrAndFlags = vHeadPtrAndFlags + tfValInProggr;   // Why Add? Should not leak into the pointer because we know that tfInProggress flag is not set here      
  if(IsSetFlag(aNode->Flags, nfDetached)) return true;   // ADDED by me  // Already removed, nothing to do // Fast path: check if already detached before taking ownership  // Redundant? Check if it ever hits
 }
   while( !AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadPtrAndFlags, vCurrPtrAndFlags) );  // If the head hasn't changed - take ownership

 bool vResult = false;
 for(SWaitNode* vCurNode = (SWaitNode*)(vCurrPtrAndFlags & tfPtrMask), *vLstNode = vCurNode, *vNode = nullptr;;)    // Walk from the head
  {
   if(!vCurNode)
    {
     if(vLstNode) vLstNode->Prev = vNode;
     if(!vResult) AtomicBitSet<moSeqCst>(&aNode->Flags, nfStillAwake);      // AtomicOr(&aNode->Flags, 2u);  // _InterlockedOr
     break;
    }
   SWaitNode* Next = vCurNode->Next;
   if( vCurNode != aNode )   // No match: Update back link and continue
    {
     vCurNode->Back = vNode;
     vNode    = vCurNode;
     vCurNode = Next;
     continue;
    }
   if( vNode )     // Have match (but not at the head)
    {
     AtomicBitSet<moSeqCst>(&vCurNode->Flags, nfDetached);      // moSeqCst ?
     vResult = true;
     vNode->Next = Next;
     if constexpr (!(Features & wlfWakeInFIFO)) break; 
     if( Next ) Next->Back = vNode;
     vCurNode = Next;
     continue;
    }
   usize vPrevPtrAndFlags = vCurrPtrAndFlags;
   vCurrPtrAndFlags = (usize)Next;    // No flags here? List pointers are untagged
   if( Next ) vCurrPtrAndFlags = vCurrPtrAndFlags ^ ((vPrevPtrAndFlags ^ vCurrPtrAndFlags) & tfPtrTagMask);      // Preserve flags but take the pointer from 'Next'?   // vCurrPtrAndFlags = ((usize)Next & tfPtrMask) | (vPrevPtrAndFlags & tfPtrTagMask);
   if( AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vPrevPtrAndFlags, vCurrPtrAndFlags) )  // If the head hasn't changed, set Next node as new head (Preserving the tag) 
    {
     AtomicBitSet<moSeqCst>(&vCurNode->Flags, nfDetached);     // moSeqCst ?    // Mark the removed (old head) node as detached
     if( !Next ) return true;         // New head is 0 (no more waiters)
     vResult = true;
     if constexpr (!(Features & wlfWakeInFIFO)) break; 
    }
     else vCurrPtrAndFlags = vPrevPtrAndFlags;    // The head was changed by someone - repeat

   vLstNode = vCurNode = (SWaitNode *)(vCurrPtrAndFlags & tfPtrMask);
   vNode    = nullptr;
  }

 this->WakeMultiple(vCurrPtrAndFlags, 0);     // While we held tfInProggress other threads may have queued wake requests - process them. Those wake requests were counted in tfWakeCtrMask
 return vResult ? vResult : !AtomicBitClr(&aNode->Flags, nfStillAwake);
}
//------------------------------------------------------------------

public:
//------------------------------------------------------------------
// Wake one waiter. Also can try to wake multiple waiters (Will wake everyone if the queue is busy and wake counter becomes saturated)
sint32 Wake(uint32 aCount=1)    // WakeConditionVariable
{
 if(aCount == uint32(-1)) return this->WakeAll();
 for(usize vHeadPtrAndFlags = this->HeadDesc;vHeadPtrAndFlags;)   // Loop until the head is detached  
  {
   if(!IsSetFlag(vHeadPtrAndFlags, tfInProggress))     // May be AtomicBitSet on 'this->HeadDesc'? - If it wasn't set before then we own it now. 
    {
     usize vMarkedHead = vHeadPtrAndFlags + tfValInProggr;   // Why Add? Should not leak into the pointer because we know that tfInProggress flag is not set here 
     if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadPtrAndFlags, vMarkedHead))    // If the head has not changed - update it with tfInProggress and do wake
      {   
       return this->WakeMultiple(vMarkedHead, aCount);       // Was 1
      }
    }
     else   
      {
       if(IsSetFlags(vHeadPtrAndFlags, tfWakeCtrMask)) return 0;    // The wake counter is saturated already (Everyone will be woken)
       if(aCount > 1)  
        {
         usize NewCtr = (vHeadPtrAndFlags & tfWakeCtrMask) + aCount;
         if(NewCtr > tfWakeCtrMask)NewCtr = tfWakeCtrMask;    // saturate the counter to wake everyone
         if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadPtrAndFlags, (vHeadPtrAndFlags & ~tfWakeCtrMask)|NewCtr)) return 0;  // If the head hasn't changed(waking is still in proggress) - set the new wake counter
        }
         else if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadPtrAndFlags, vHeadPtrAndFlags + tfValIncCtr)) return 0;    // If the head hasn't changed(waking is still in proggress) - just increment the pending wake coulter - someone else will do the waking
      }
  }
 return 0;
}
//------------------------------------------------------------------
sint32 WakeAll(void)
{
 for(usize vHeadAndFlags = this->HeadDesc; vHeadAndFlags && !IsSetFlags(vHeadAndFlags, tfWakeCtrMask);)    // If the wake counter is saturated then someone is expected to already be in proggress of waking everyone
  {
   if(!IsSetFlag(vHeadAndFlags, tfInProggress))   
    {
     if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadAndFlags, usize(0)))   // Detach the list if its state has not changed
      {
       uint32 Result = 0;
       for(SWaitNode* vCurNode=(SWaitNode*)(vHeadAndFlags & tfPtrMask); vCurNode;)   // Iterate all nodes and wake them
        {
         SWaitNode* Next = vCurNode->Next;
         AtomicBitSet<moSeqCst>(&vCurNode->Flags, nfDetached);     // _interlockedbittestandset     // moSeqCst ?
         if(!AtomicBitClr(&vCurNode->Flags, nfStillAwake))          // If nfStillAwake was zero already - wake 
          {
           DoWake(vCurNode);  // _interlockedbittestandreset    AtomicBitClr(&vCurNode->Flags, 1u)
           Result++;          // The queue state ignores any NtAPI failures anyway     // TODO: increment Result on success only?  
          }
         vCurNode = Next;
        }
       return Result;   // Number woken waiters
      } 
    }
     else if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadAndFlags, AddFlags(vHeadAndFlags, tfWakeCtrMask))) return 0;  // If the head has not changed - saturate the wake counter and exit (someone else will continue with waking all waiters)
  } 
 return 0;
}
//------------------------------------------------------------------
// https://github.com/microsoft/STL/issues/4462
// The shared mode is problematic because:
//  - Condition variables typically need to modify shared state when signaled
//  - Waiting on a condition usually implies you'll need exclusive access when woken
//  - The shared mode support is rarely used correctly
// 
// May return STATUS_TIMEOUT or 0
// 
// 1. **Double-check pattern**:
//   - Check **before** enqueueing (fast path - avoid queue operations if value changed)
//   - Check **after**  enqueueing (prevent lost wakeup)
// 
// NOTE: Waiting on an event does not requires aSyncObj or aExpected. Just suspends the thread until a notification (WakeAll)
// 
// UPDATES aExpected to a last detected changed value
// Timeout is in milliseconds
//
sint32 Wait(uint64 aTimeout=uint64(-1), sint32* aExpected=nullptr, SO* aSyncObj=nullptr, bool SyncObjShared=false)     // RtlSleepConditionVariableCS / RtlSleepConditionVariableSRW
{
 if constexpr (Features & wlfCheckVar)                
   if(sint32 CurrVal;aExpected && ((CurrVal=AtomicLoad<moAcquire>(&this->Value)) != AtomicLoad<moAcquire>(aExpected))) {*aExpected=CurrVal; return PX::EAGAIN;}  // Value already changed, don't wait  // Check BEFORE enqueueing - no lock needed  // AtomicLoad<moRelaxed>(aExpected) ???
//_m_prefetchw(this);
 SWaitNode vStackNode;     // Must be 16-byte aligned
 usize vNewHeadAndFlags;
 usize vHeadAndFlags = this->HeadDesc;
 bool NeedExtraVChk  = aExpected && !aSyncObj;
 vStackNode.Back     = nullptr;
 vStackNode.LockObj  = aSyncObj;  
 vStackNode.ThreadID = NT::NtCurrentTeb()->ClientId.UniqueThread; 
 if(aSyncObj && !SyncObjShared) SetFlags(vStackNode.Flags, MakeFlag<uint32>(nfStillAwake) | MakeFlag<uint32>(nfLkExclusive));
   else SetFlag(vStackNode.Flags, nfStillAwake);  
 
 for(;;)    // CAS loop to put the new node as the head
  {   
   vNewHeadAndFlags = (usize)&vStackNode | (vHeadAndFlags & tfPtrTagMask);   // Preserve the tag 
   SWaitNode* Head  = (SWaitNode*)(vHeadAndFlags & tfPtrMask);
   vStackNode.Next  = Head;
   if constexpr (Features & wlfCheckVar)  
     if(sint32 CurrVal;NeedExtraVChk && ((CurrVal=AtomicLoad<moAcquire>(&this->Value)) != AtomicLoad<moAcquire>(aExpected))) {*aExpected=CurrVal; return PX::EAGAIN;}  // Value already changed, don't wait   // nfStillAwake will take care about the rest  // FUTEX_WAKE does not care about the value.
   if( Head )   // List is NOT empty
    {
     vStackNode.Prev = nullptr;
     SetFlag(vNewHeadAndFlags, tfInProggress);   // Multiple waiters, need optimization. OptimizeWaitList() will clear this flag (and the entire tag). Any Wake() will wait until this flag is cleared
    }
     else vStackNode.Prev = &vStackNode;   // List IS empty (first waiter)  // Prev is circular but Next stays NULL
   if(AtomicCmpExch<moSeqCst,moSeqCst,false>(&this->HeadDesc, &vHeadAndFlags, vNewHeadAndFlags)) break;   // Set the new head and break, if it hasn't changed
  }

 // Less efficient than holding an external CriticalSection/SRWLock ?
 // Is this not much worse than having an actual SRW lock to protect the variable check? 
 if constexpr (Features & wlfCheckVar) {
 if(sint32 CurrVal;NeedExtraVChk && ((CurrVal=AtomicLoad<moAcquire>(&this->Value)) != AtomicLoad<moAcquire>(aExpected)))  // SECOND CHECK after enqueueing - prevent lost wakeup  // This handles: check passes -> other thread modifies value and wakes -> we enqueue
  {
   if(!this->WakeSingle(&vStackNode))   // Value changed while we were enqueueing // Try to remove ourselves from the queue
    { 
     for(int spin = 0; spin < 100; spin++) {     // Someone is waking us - spin briefly before syscall
         if(AtomicBitTst(&vStackNode.Flags, nfDetached)) return PX::EAGAIN;  // Got detached during spin  // moAcquire by default
         if(spin < 10) continue;      // Tight spin for first 10
         YieldCPU();  // CPU pause hint
     }
     do DoWait(&vStackNode, 0);     // Failed to remove (someone is waking us), wait for detachment  // Pass the node itself, not aSyncObj  
     while(!AtomicBitTst(&vStackNode.Flags, nfDetached));
    }
   *aExpected = CurrVal;
   return PX::EAGAIN;  // Don't sleep, value changed
  } }
 if(aSyncObj)aSyncObj->Release(SyncObjShared);   // RtlLeaveCriticalSection(aCritSec);   // RtlReleaseSRWLockShared/RtlReleaseSRWLockExclusive

 if constexpr (Features & wlfWakeInFIFO)
   if( IsSetFlag(vHeadAndFlags ^ vNewHeadAndFlags, tfInProggress) ) this->OptimizeWaitList(vNewHeadAndFlags);  // Only if one of the heads have the flag, not both
 DoSync(&vStackNode.Flags, nfStillAwake);

 NT::NTSTATUS vRes = 0;
 if( AtomicBitClr(&vStackNode.Flags, nfStillAwake) ) vRes = DoWait(&vStackNode, aTimeout);       // The previous value was 1 (unchanged) - goto sleep  
   else AtomicBitSet<moSeqCst>(&vStackNode.Flags, nfDetached);     // Wake processed the node first! We're already woken, don't sleep  //  moSeqCst ?  _InterlockedOr
 if( (vRes == NT::STATUS_TIMEOUT) || !IsSetFlag(vStackNode.Flags, nfDetached) )     // vStackNode.Flags & 4
  {
   if(!this->WakeSingle(&vStackNode))   // Wake self (just remove from the list)
    {
     do DoWait(&vStackNode, 0);
     while( !IsSetFlag(vStackNode.Flags, nfDetached) );    // Loop until someone removes us?    // (vStackNode.Flags & 4) 
    }     
  }
 if(aSyncObj)aSyncObj->Acquire(SyncObjShared);   // RtlEnterCriticalSection(aCritSec);   // RtlAcquireSRWLockShared/RtlAcquireSRWLockExclusive
 return -NTX::NTStatusToLinuxErr(vRes);          // May return: ETIMEDOUT, EINTR
} 
//------------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
};



/* https://locklessinc.com/articles/keyed_events/

#pragma comment (linker, "/defaultlib:ntdll.lib")

typedef union fast_mutex fast_mutex;
union fast_mutex
{
	unsigned long waiters;
	unsigned char owned;
};

#define FAST_MUTEX_INITIALIZER {0}

HANDLE mhandle;

// Allows up to 2^23-1 waiters 
#define FAST_M_WAKE		256
#define FAST_M_WAIT		512

void init_fast_mutex_handle(void)
{
	NtCreateKeyedEvent(&mhandle, -1, NULL, 0);
}

void fast_mutex_init(fast_mutex *m)
{
	m->waiters = 0;
}

void fast_mutex_lock(fast_mutex *m)
{
	// Try to take lock if not owned 
	while (m->owned || _interlockedbittestandset(&m->waiters, 0))
	{
		unsigned long waiters = m->waiters | 1;
		
		// Otherwise, say we are sleeping 
		if (_InterlockedCompareExchange(&m->waiters, waiters + FAST_M_WAIT, waiters) == waiters)
		{
			// Sleep 
			NtWaitForKeyedEvent(mhandle, m, 0, NULL);
			
			// No longer in the process of waking up 
			_InterlockedAdd(&m->waiters, -FAST_M_WAKE);
		}
	}
}

int fast_mutex_trylock(fast_mutex *m)
{
	if (!m->owned && !_interlockedbittestandset(&m->waiters, 0)) return 0;
	
	return EBUSY;
}

void fast_mutex_unlock(fast_mutex *m)
{
	// Atomically unlock without a bus-locked instruction 
	_ReadWriteBarrier();
	m->owned = 0;
	_ReadWriteBarrier();
	
	// While we need to wake someone up 
	while (1)
	{
		unsigned long waiters = m->waiters;
		
		if (waiters < FAST_M_WAIT) return;
		
		// Has someone else taken the lock? 
		if (waiters & 1) return;
		
		// Someone else is waking up 
		if (waiters & FAST_M_WAKE) return;
		
		// Try to decrease wake count 
		if (_InterlockedCompareExchange(&m->waiters, waiters - FAST_M_WAIT + FAST_M_WAKE, waiters) == waiters)
		{
			// Wake one up 
			NtReleaseKeyedEvent(mhandle, m, 0, NULL);
			return;
		}
		
		YieldProcessor();
	}
}


typedef struct fast_cv fast_cv;
struct fast_cv
{
	fast_mutex lock;
	unsigned long wlist;
};

#define FAST_CV_INITIALIZER	{0, 0}

void fast_cv_init(fast_cv *cv)
{
	fast_mutex_init(&cv->lock);
	cv->wlist = 0;
}

void fast_cv_wait(fast_cv *cv, fast_mutex *m)
{
	fast_mutex_lock(&cv->lock);
	fast_mutex_unlock(m);
	cv->wlist++;
	fast_mutex_unlock(&cv->lock);
	
	NtWaitForKeyedEvent(mhandle, &cv->wlist, 0, NULL);

	fast_mutex_lock(m);
}

void fast_cv_signal(fast_cv *cv)
{
	fast_mutex_lock(&cv->lock);
	if (cv->wlist)
	{
		cv->wlist--;
		NtReleaseKeyedEvent(mhandle, &cv->wlist, 0, NULL);
	}
	fast_mutex_unlock(&cv->lock);
}

void fast_cv_broadcast(fast_cv *cv)
{
	fast_mutex_lock(&cv->lock);
	while (cv->wlist)
	{
		cv->wlist--;
		NtReleaseKeyedEvent(mhandle, &cv->wlist, 0, NULL);
	}
	fast_mutex_unlock(&cv->lock);
}
*/
//------------------------------------------------------------------------------------------------------------
