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
struct MRGN       // Memory Region
{
enum EAllocFlags {
 afNone        = 0,
 afNoMove      = 0x0010,   // For afSequential. Allocate only from an initially reserved single block of memory(BlkLen). No moving is attempted (delete, extend) and pointers are always preserved. An attempt to grow may fail. // May be not enough memory to relocate (On Windows, especially x32)
 afSequential  = 0x0020,   // The data in memory must always be sequential to be accessed by a raw pointer. Will use reallocation and copy, cannot guarantee pointer preservation. No free slot map, just a used/allocated sizes
 afObjAlign    = 0x0040,   // Waste some memory by expanding (Pow2/Mul2) size of objects for better access performance
 afAlignPow2   = 0x0080,   // Align obj/hdr size to nearest Pow2 size
 afLimitHdrAl  = 0x0100,   // Use some limited header alignment (64 bytes) instead of same as the object size (aligned)
 afSinglePtr   = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere).  // Sometimes an array is a member of some struct that is allocated in vast numbers but not all of them will have some data
 afBlkTrcOwner = 0x0400,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)
//afSmallMem    = 0x0800,   // Do not preallocate early and keep preallocation as small as possible (More conditions while allocating/accessing and more syscalls to grow the allocated block)
//afSparseIndex = 0x1000,   // Allocate blocks if their pointers are NULL otherwise report errors on access by unallocateed index.
 afBlkCtxInIdx  = 0x2000,   // If the block context is present, store it in the block index table instead of beginning of each block (Will use more stack memory) // Allows block contexts to be preserved even for deleted blocks

 afGrowMask    = 0x000F,
 afGrowUni     = 0x0000,   // NextSize = BaseSize               // Uniform allocation growth (All blocks of same size (BaseSize))
 afGrowLin     = 0x0001,   // NextSize = PrevSize + BaseSize    // Linear allocation growth
 afGrowExp     = 0x0002,   // NextSize = PrevSize + PrevSize    // Exponential allocation growth (Size = Size * 2)
};

SCVR usize DefMaxAlign = 64;
//============================================================================================================

#include "ArenaSeq.hpp"
#include "ArenaSeg.hpp"
//------------------------------------------------------------------------------------------------------------
// MinLen: In bytes. Will be rounded up to the system`s PageSize at least
// TCIfo: Per context (allocator) user defined info
// TBIfo: Per block user defined info
//
template<usize MinLen, uint32 Flg, typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemProvBase> class CMemRegion: protected TSW<(Flg & afSequential), CSeqArena<MinLen,Flg,Ty,TCIfo,TBIfo,MP>, CSegArena<MinLen,Flg,Ty,TCIfo,TBIfo,MP> >::T
{
 using Base = TSW<(Flg & afSequential), CSeqArena<MinLen,Flg,Ty,TCIfo,TBIfo,MP>, CSegArena<MinLen,Flg,Ty,TCIfo,TBIfo,MP> >::T;

//------------------------------------------------------------------------------------------------------------
public:

//------------------------------------------------------------------------------------------------------------
inline CMemRegion(MP* mp=(MP*)nullptr): Base(mp) {}    
inline ~CMemRegion(){}      
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
// MoveItems
// Save
// Load
//-------------------------------------------------------------------
};
//============================================================================================================
//                                           TESTS
//------------------------------------------------------------------------------------------------------------
#ifdef DBGBUILD
template<typename S> static bool TestPolicyBlk(uint32 From=0)
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
