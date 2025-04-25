/*
  Copyright (c) 2024-present Victor Sheinmann

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/
#pragma once
//============================================================================================================
struct NALC
{
enum EAllocFlags {
 afNone        = 0,
 afNoMove      = 0x0010,   // For afSequential. Allocate only from an initially reserved single block of memory(BlkLen). No moving is attempted (delete, extend) and pointers are always preserved. An attempt to grow may fail. // May be not enough memory to relocate (On Windows, especially x32)
 afSequential  = 0x0020,   // The data in memory must always be sequential to be accessed by a raw pointer. Will use reallocation and copy, cannot guarantee pointer preservation. No free slot map, just a used/allocated sizes
 afObjAlign    = 0x0040,   // Waste some memory by expanding (Pow2/Mul2) size of objects for better access perfomance
 afAlignPow2   = 0x0080,   // Align obj/hdr size to nearest Pow2 size
 afLimitHdrAl  = 0x0100,   // Use some limited header alignment (64 bytes) instead of same as the object size (aligned)
 afSinglePtr   = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere).  // Sometimes an array is a member of some struct that is allocated in vast numbers but not all of them will have some data
 afBlkTrcOwner = 0x0400,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)
//afSmallMem    = 0x0800,   // Do not preallocate early and keep preallocation as small as possible (More conditions while allocating/accessing and more syscalls to grow the allocated block)
//afSparseIndex = 0x1000,   // Allocate blocks if their pointers are NULL otherwise report errors on access by unallocateed index.
 afBlkCtxInIdx  = 0x2000,   // If the block context is present, store it in the block intex table instead of beginning of each block (Will use more stack memory) // Allows block contexts to be preserved even for deleted blocks

 afGrowMask    = 0x000F,
 afGrowUni     = 0x0000,   // NextSize = BaseSize               // Uniform allocation growth (All blocks of same size (BaseSize))
 afGrowLin     = 0x0001,   // NextSize = PrevSize + BaseSize    // Linear allocation growth
 afGrowExp     = 0x0002,   // NextSize = PrevSize + PrevSize    // Exponential allocation growth (Size = Size * 2)
};
SCVR usize DefMaxAlign = 64;
//============================================================================================================
struct alignas(usize) SMemPrvBase      // Base memory provider  // Min size is 4096 and min alignment is 4096  // TODO: The interface verification
{
 vptr Alloc(usize len)  // For block allocations,   // May return a handle instead of actual memory
 {
  vptr BPtr = (vptr)NPTM::NAPI::mmap(nullptr, len, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   // Executable?  // Some platforms may refuse allocating of RWX memory
  if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
  return BPtr;
 }
 bool Free(vptr ptr, usize len){return NPTM::NAPI::munmap(ptr, len) >= 0;}    // Size is optional
 vptr Lock(vptr ptr, usize len, usize offs=0){return ptr;}            // Size is optional  // Returns actual memory pointer   // NOTE: Do not expect to store any contexts or headers in memory that requires this
 bool UnLock(vptr ptr, usize len, usize offs=0){return true;}         // Size is optional
 vptr ReAlloc(vptr optr, usize olen, usize nlen, bool maymove=true)   // May return a handle   // TODO: Implement mremap syscall  // NOTE: may fail if MayMove is false and ptr is not a handle
 {
  if(!optr)return this->Alloc(nlen);   // Should just allocate if ptr is NULL
  vptr BPtr = (vptr)NPTM::NAPI::mremap(optr, olen, nlen, !maymove?PX::MREMAP_FIXED:0, nullptr);
  if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
  return nullptr; 
 } 
};
//============================================================================================================

#include "AllocatorSeq.hpp"
#include "AllocatorBlk.hpp"
//#include "AllocatorBkt.hpp"
//#include "AllocatorBin.hpp"
//#include "AllocatorTLSF.hpp"
//------------------------------------------------------------------------------------------------------------
// MinLen: In bytes. Will be rounded up to the system`s PageSize at least
// TCIfo: Per context (allocator) user defined info
// TBIfo: Per block user defined info
//
template<usize MinLen, uint32 Flg, typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemPrvBase> class CGenAlloc: protected TSW<(Flg & afSequential), SSeqAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP>, SBlkAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP> >::T
{
 using Base = TSW<(Flg & afSequential), SSeqAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP>, SBlkAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP> >::T;

//------------------------------------------------------------------------------------------------------------
public:

//------------------------------------------------------------------------------------------------------------
inline CGenAlloc(MP* mp=(MP*)nullptr): Base(mp) {}    
inline ~CGenAlloc(){}      
//------------------------------------------------------------------------------------------------------------
using Base::end;
using Base::begin;

using Base::ElmFrom;
// ElmNext
using Base::ElmLast;
using Base::ElmFirst;
     
using Base::Expand;
using Base::Shrink;
using Base::Release;

using Base::GetCtxInfo;  // NOTE: Use 'auto' to assign
using Base::IsElmExist;
using Base::GetElmMax;

// Duplicate
// MemoryUsed
// Save
// Load
//-------------------------------------------------------------------
};
//============================================================================================================
// 'free' is working - have to know allocation sizes for iterators to work
// if MinUnits != MaxUnits - one unit is reserved for the count and all requests return a next unit which contains  actual data (WASTE!!!)
// We may have PrevNode and NextNode and some way to determine number of units used
// Index ranges are extendable somehow and processed in a loop?
//
template<usize MinLen, usize MinUnits, usize MaxUnits, uint32 Flg, typename Ty=uint32, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemPrvBase> class CUnitAlloc
{
static_assert(MaxUnits >= MinUnits);
using UDefType = uint32;
using UType  = TSW<sizeof(Ty) < sizeof(UDefType), UDefType, Ty>::T;
SCVR usize UnitLen = sizeof(UType);
SCVR usize UnitRange = MaxUnits - MinUnits;
SCVR bool SingleFChain = MaxUnits == MinUnits;

}; 
//============================================================================================================
//                                           TESTS
//------------------------------------------------------------------------------------------------------------
#ifdef DBGBUILD
template<typename S> static bool TestStrategyBlk(uint32 From=0)
{
 constexpr S str;   //  constexpr NALC::SBASGeom<128, 4096, 64, 0> str;  
 for(uint idx=From;idx < 0xFFFFFFFF;idx++)
  {
   usize ClcIdx, BrtIdx;
   usize ClcBlk = str.CalcForIndex(idx, ClcIdx);
   usize BrtBlk = str.BruteForIndex(idx, BrtIdx);
   if((ClcBlk != BrtBlk)||(ClcIdx != BrtIdx))
    {
     return false;
    }
   if(!BrtIdx && (str.BlkIdxToObjIdx(BrtBlk) != idx))   // Every first unit must match a calculated one
    {
     return false;
    }
  }
 return true; 
}
#endif
//------------------------------------------------------------------------------------------------------------

};
//------------------------------------------------------------------------------------------------------------
/*
{
   NALC::CGenAlloc<4096,4096,NALC::afObjAlloc|NALC::afSinglePtr> alc;
     bool t1 = alc.IsElmExist(56);
   volatile vptr xx = alc.GetBlock(5);
     bool t2 = alc.IsElmExist(67);
      xx = alc.GetBlock(8);
        alc.DeleteBlk(5);
        xx = alc.GetBlock(43);
    vptr yy = alc.FindBlock(nullptr);
    auto dd = alc.GetBlkData(8, 4);
    alc.SetBlkTag(8, 9); 
    uint rr = alc.GetBlkTag(8);
    uint TotalElm1 = 0;
    uint TotalElm2 = 0;
    uint TotalElm3 = 0;
    for (auto it = alc.begin(), end = alc.end(); it != end; ++it) 
     {
      *it = 6;
      TotalElm1++;
     }
    for (auto it = alc.begin(), end = alc.end(); it ; ++it) 
     {
      *it = 6;
      TotalElm2++;
     }
    for (auto& el : alc) 
     {
      el = 6;
      TotalElm3++;
     }
   //int v = AlignToP2Dn(5);
   //v++;
   return 11;
  }

*/
