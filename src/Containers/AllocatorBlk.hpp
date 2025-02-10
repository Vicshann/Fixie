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
//                                      Block access strategies
//------------------------------------------------------------------------------------------------------------
// NOTE: PageLen is better to be multiple of MEMGRANSIZE (Even better if it is Pow2 sized amount)
// Both 64-bit Linux and 64-bit Windows support 48-bit addresses, which translates to 256TB of virtual address space
// Android will use memory tagging, so no pointer compression worth the implementation effort anyway
// Allocating 64k for page index is not a problem on linux. On Windows it must be 64k or a memory hole will be created.
//
private:
template<size_t PageLen, size_t UnitLen, size_t FHdrLen, size_t NHdrLen, size_t RMin, size_t RMax, typename Der> struct SBASBase
{
 SCVR size_t TagMask   = ((size_t)NPTM::MEMPAGESIZE - 1);   // Do not use PageSize to keep it consistent with other strategies
 SCVR size_t PtrMask   = ~TagMask;
 SCVR size_t RangeMin  = RMin;
 SCVR size_t RangeMax  = RMax;      // Arbitrary
 SCVR size_t FHdrSize  = FHdrLen;   // To make them available from outside
 SCVR size_t NHdrSize  = NHdrLen;
 SCVR size_t UnitSize  = UnitLen;
 SCVR size_t PageSize  = PageLen;    // Base/First page size

//-------------------------------------------------------------
static inline uint BlkIdxToHdrLen(uint32 BlkIdx)        // TODO: Branchless     // TODO: Check that any padding is in the geader
{
 if constexpr (FHdrSize)
  {
   if constexpr (FHdrSize != NHdrSize)return BlkIdx ? NHdrSize : FHdrSize;
     else return FHdrSize;
  }
 else if constexpr (NHdrSize)   // FHdrSize is 0     // NOTE: It is most likely useless to have NHdr without FHdr
  {
   if(BlkIdx)return NHdrSize;
  }
 return 0;
}
//-------------------------------------------------------------
static inline uint64 BlkIdxToDLen(uint32 BlkIdx) {return Der::BlkIdxToSize(BlkIdx) - BlkIdxToHdrLen(BlkIdx);}
//-------------------------------------------------------------
static inline size_t BlkIdxToOffset(uint32 BlkIdx) {return Der::BlkIdxToObjIdx(BlkIdx) * UnitSize;}     // Returns offset as in a contiguous block of units
//-------------------------------------------------------------
static inline size_t BlkIdxToObjNum(uint32 BlkIdx) {return BlkIdxToDLen(BlkIdx) / UnitSize;}
//-------------------------------------------------------------
static inline size_t BlkIdxToObjInf(uint32 BlkIdx, size_t& Offs, size_t& Size)
{
 uint64 BLen = Der::BlkIdxToSize(BlkIdx);
 size_t HLen = BlkIdxToHdrLen(BlkIdx);
 Offs = HLen;     // Offset of data
 Size = BLen;     // Size of the block
 return (BLen - HLen) / UnitSize;  // Number of items in the block
}
//-------
};
public:
//============================================================================================================
// Size = Size   // Uniform allocation (Every block size is the same)
// NOTE: Do not use pages of MEMPAGESIZE on Windows or the entire address space will be full of unreclaimable holes of 60k in size!
//
template<size_t UnitLen, size_t PageLen=NPTM::MEMGRANSIZE, size_t FHdrLen=0, size_t NHdrLen=0, size_t LBlkMin=0> class SBASUni: public SBASBase<AlignP2Frwd(((PageLen - Max(FHdrLen, NHdrLen)) < UnitLen)?(UnitLen+Max(FHdrLen, NHdrLen)):(PageLen),NPTM::MEMGRANSIZE), UnitLen, FHdrLen, NHdrLen, (LBlkMin?LBlkMin:14), (size_t)-1, SBASUni<UnitLen,PageLen,FHdrLen,NHdrLen> >
{
 DEFINE_SELF   // To fix dependent name lookup - not going to repeat that looong base class template type!

 SCVR bool   NeedCorr  = self::ObjInFBlk != self::ObjInNBlk;
 SCVR size_t ObjInFBlk = ((self::PageSize - self::FHdrSize) / self::UnitSize);
 SCVR size_t ObjInNBlk = ((self::PageSize - self::NHdrSize) / self::UnitSize);

public:
//-------------------------------------------------------------
static inline uint64 BlkIdxToSize(uint32 BlkIdx) {return self::PageSize;}   // Force inline?  // Sise of entire block, including any headers/padding
//-------------------------------------------------------------
static size_t BlkIdxToObjIdx(size_t BlkIdx)
{
 size_t ObjIdx = BlkIdx * ObjInNBlk;
 if constexpr (NeedCorr)
  {
   SCVR sint FPOLess = ObjInNBlk - ObjInFBlk;    // +/-
   ObjIdx -= FPOLess * (ObjIdx >= ObjInFBlk);
  }
 return ObjIdx;
}
//-------------------------------------------------------------
static uint CalcForIndex(size_t ObjIdx, size_t& Idx)      // TODO: Check if ObjIdx is outside of valid range? (For now this function is branchless)
{
 if constexpr (NeedCorr)      // First block contains different header
  {
   SCVR sint FPOLess = ObjInNBlk - ObjInFBlk;    // +/-
   ObjIdx += FPOLess * (ObjIdx >= ObjInFBlk);
  }
 Idx =  ObjIdx % ObjInNBlk;    // Object index in the associated block  // TODO: Make sure that the Mul is used with a magic constant. Or use LibDiv
 return ObjIdx / ObjInNBlk;    // Returns a block index for this ObjIdx
}
//-------------------------------------------------------------
#ifdef DBGBUILD
static uint BruteForIndex(size_t ObjIdx, size_t& Idx)  // For testing
{
 size_t Ps   = 0;
 size_t HLen = self::FHdrSize;
 for(int idx=0;idx < self::RangeMax;idx++)
  {
   size_t Lx = Ps;
   size_t On = ((self::PageSize - HLen) / self::UnitSize);
   Ps  += On;
   if(Ps > ObjIdx){Idx = ObjIdx - Lx; return idx;}
   HLen = self::NHdrSize;
  }
 Idx = -1;
 return -1;
}
#endif
//-------------------------------------------------------------
};
//============================================================================================================
// Size = Size + BaseSize    // Linear allocation growth   // BlkSize=PageSize*(BlkIdx+1)
// NOTE: Do not use pages of MEMPAGESIZE on Windows or the entire address space will be full of unreclaimable holes of 4-60k in size!
//
template<size_t UnitLen, size_t PageLen=NPTM::MEMGRANSIZE, size_t FHdrLen=0, size_t NHdrLen=0, size_t LBlkMin=0> class SBASLin: public SBASBase<AlignP2Frwd(((PageLen - Max(FHdrLen, NHdrLen)) < UnitLen)?(UnitLen+Max(FHdrLen, NHdrLen)):(PageLen),NPTM::MEMGRANSIZE), UnitLen, FHdrLen, NHdrLen, (LBlkMin?LBlkMin:6), (size_t)-1, SBASLin<UnitLen,PageLen,FHdrLen,NHdrLen> >
{
 DEFINE_SELF   // To fix dependent name lookup - not going to repeat that looong base class template type!

 SCVR sint   LeftOnFPage  = (self::PageSize-self::FHdrSize) % self::UnitSize;
 SCVR sint   LeftOnNPage  = (self::PageSize-self::NHdrSize) % self::UnitSize;
 SCVR sint   FirstBlkDiff = ((LeftOnNPage+self::NHdrSize)-(LeftOnFPage+self::FHdrSize));   // Because in the arr correction is for NHdrSize
 SCVR size_t BlkLftRange  = self::UnitSize / ((size_t)1 << ctz(self::UnitSize));  // Leftover bytes sequence repeat range (in blocks)   // Same as "UnitSize / NMATH::gcd(PageSize, UnitSize)" when page size is pow2

 struct SApArr
 {
  uint32 Arr[BlkLftRange+1];     // Biggest value is TotalInRange which is not known yet. uint32 should be enough
  consteval SApArr(void)         // Be aware how BlkLftRange depends on UnitSize
   {
    uint32 Total = 0;
    this->Arr[0] = Total;
    for(size_t i=1; i <= BlkLftRange; i++)       // Assume that the header is padded so that rest of the block contains whole number of units
     {
      Total += ((self::PageSize * i)-self::NHdrSize) % self::UnitSize;          // Must assume same header size for all blocks (correction for thr first block is done later)
      Total += self::NHdrSize;      // Known not to be the part of units array
      this->Arr[i] = Total;         // Cumulative bytes
     }
   }
 };
 SCVR TSW<(bool)LeftOnNPage,SApArr,ETYPE>::T CorrArr;    // Allocated in rdata section

public:
//-------------------------------------------------------------
static inline uint64 BlkIdxToSize(uint32 BlkIdx) {return self::PageSize * ++BlkIdx;}
//-------------------------------------------------------------
static size_t BlkIdxToObjIdx(size_t BlkIdx)
{
 size_t cblen;
 if constexpr (LeftOnNPage)
  {
   SCVR uint32 TotalInRange = CorrArr.Arr[BlkLftRange];
   size_t aidx  = (BlkIdx % BlkLftRange);
   size_t corra = ((BlkIdx / BlkLftRange) * TotalInRange);    // Is it possible to avoid doing DivMod again?  // Optimized to mul and mulx, looks better than manual optimization to test+add
   size_t corrb = (corra + CorrArr.Arr[aidx]);
   corrb -= FirstBlkDiff * (bool)corrb;                // Mul by 0/1 to avoid branching       // Needed only when we have a full block  // Mul by +/-1: 1 - (signed)( (unsigned)(a+1) ^ (unsigned)(b+1) ); or am array: {0,val}
   cblen  = (self::PageSize * ((BlkIdx * (BlkIdx + 1)) / 2)) - corrb;
  }
   else
    {
     SCVR size_t Correction = LeftOnFPage + self::FHdrSize;
     cblen = (self::PageSize * ((BlkIdx * (BlkIdx + 1)) / 2));
     if constexpr(self::FHdrSize)cblen -= Correction * (bool)cblen;
    }
 return (cblen / self::UnitSize);         // NOTE: Bad for BlkIdxToOffset
}
//-------------------------------------------------------------
static uint CalcForIndex(size_t ObjIdx, size_t& Idx)
{
 size_t cblen, cbidx;
 size_t uoffs = ObjIdx * self::UnitSize;          // GetOffsetForUnit(UnitIndex);
 if constexpr (LeftOnNPage)   // Will need corrections not only for the first block
  {
   SCVR uint32 TotalInRange = CorrArr.Arr[BlkLftRange];       // Last one is the total range (Should be constant to precalc magic numbers for div)
          cbidx = ((size_t)NMATH::sqrt((8 * (uoffs / self::PageSize)) + 1) - 1) / 2;   // FindBlockForOffset(uoffs);    // In a single block - Ignoring headers and leftover bytes (will correct for them later)
   size_t aidx  = (cbidx % BlkLftRange);                      // NOTE: cbidx is not correct at this point (good only for calculation of an actual cbidx)
   size_t corra = ((cbidx / BlkLftRange) * TotalInRange);
   size_t corrb = corra + CorrArr.Arr[1+aidx];         // Compensate difference between default LeftOnNPage and LeftOnFPage
   uoffs += corrb - FirstBlkDiff;
   cbidx  = ((size_t)NMATH::sqrt((8 * (uoffs / self::PageSize)) + 1) - 1) / 2;         // FindBlockForOffset(uoffs);    // The square root again :(   // cbidx will usually be the same
   aidx   = (cbidx % BlkLftRange);
   corra  = ((cbidx / BlkLftRange) * TotalInRange);    // Is it possible to avoid doing DivMod again?  // Optimized to mul and mulx, looks better than manual optimization to test+add
   corrb  = (corra + CorrArr.Arr[aidx]);
   corrb -= FirstBlkDiff * (bool)corrb;                // Mul by 0/1 to avoid branching       // Needed only when we have a full block  // Mul by +/-1: 1 - (signed)( (unsigned)(a+1) ^ (unsigned)(b+1) ); or am array: {0,val}
   cblen  = (self::PageSize * ((cbidx * (cbidx + 1)) / 2)) - corrb;
  }
   else
    {
     SCVR size_t Correction = LeftOnFPage + self::FHdrSize;
     if constexpr(self::FHdrSize)uoffs += Correction;
     cbidx = ((size_t)NMATH::sqrt((8 * (uoffs / self::PageSize)) + 1) - 1) / 2;   // FindBlockForOffset(uoffs);   // In a single block - Ignoring headers and leftover bytes (will correct for them later)
     cblen = (self::PageSize * ((cbidx * (cbidx + 1)) / 2));
     if constexpr(self::FHdrSize)cblen -= Correction * (bool)cblen;
    }
 Idx = ObjIdx - (cblen / self::UnitSize);
 return cbidx;
}
//-------------------------------------------------------------
#ifdef DBGBUILD
static uint BruteForIndex(size_t ObjIdx, size_t& Idx)  // For testing
{
 size_t Ps   = 0;
 size_t HLen = self::FHdrSize;
 for(int idx=0;idx < self::RangeMax;idx++)
  {
   size_t Lx = Ps;
   size_t On = (((self::PageSize*(idx+1)) - HLen) / self::UnitSize);
   Ps += On;
   if(Ps > ObjIdx){Idx = ObjIdx - Lx; return idx;}
   HLen = self::NHdrSize;
  }
 Idx = -1;
 return -1;
}
#endif
//-------------------------------------------------------------
};
//============================================================================================================
// Size = Size + Size  // Grow geometrically (geometric progression with base 2)
// PageLen is rounded up to nearest Pow2
// RMin: 32 Should be enough for x32 and x64 even if base page size is 4096 (16TB max) (RangeNum times : PageSize = (PageSize * 2))
//
template<size_t UnitLen, size_t PageLen=NPTM::MEMPAGESIZE, size_t FHdrLen=0, size_t NHdrLen=0, size_t LBlkMin=0> class SBASExp: public SBASBase<AlignToP2Up(((PageLen - Max(FHdrLen, NHdrLen)) < UnitLen)?(UnitLen+Max(FHdrLen, NHdrLen)):(PageLen)), UnitLen, FHdrLen, NHdrLen, 32, 32, SBASExp<UnitLen,PageLen,FHdrLen,NHdrLen> >
{
 DEFINE_SELF   // To fix dependent name lookup - not going to repeat that looong base class template type!

 struct SApArr     // Is there another way to initialize an array at compile time by a consteval function?
 {
  size_t Arr[self::RangeMin];
  consteval SApArr(void)
   {
    for(size_t i=0,v=0,h=self::FHdrSize;i < self::RangeMin;i++)
     {
      this->Arr[i] = v;
      v += (((self::PageSize << i)-h) / self::UnitSize);
      h  = self::NHdrSize;
     }
   }
 };
 SCVR bool NoCorr = !self::NHdrSize && IsPowOfTwo(self::UnitSize);
 SCVR TSW<NoCorr,ETYPE,SApArr>::T RangeArr;    // Allocated in rdata section

public:
//-------------------------------------------------------------
static inline uint64 BlkIdxToSize(uint32 BlkIdx) {return (uint64)self::PageSize << BlkIdx;}   // Force inline?   // NOTE: May be too much for x32  // BlkIdx &= (RangeNum - 1);  // Max is RangeNum
//-------------------------------------------------------------
static size_t BlkIdxToObjIdx(size_t BlkIdx)
{
 if constexpr (NoCorr)
 {
  SCVR size_t ObjsOnFPage   = self::PageSize / self::UnitSize;      // No FHdr, Pow2 only
  SCVR size_t ObjBaseBitIdx = ctz(ObjsOnFPage);
  size_t ObjIdx = ((size_t)1 << (BlkIdx + ObjBaseBitIdx)) - ObjsOnFPage;
  if constexpr (self::FHdrSize)
   {
    SCVR size_t FPOLess = ObjsOnFPage - ((self::PageSize-self::FHdrSize) / self::UnitSize);
    ObjIdx  -=  FPOLess * (ObjIdx >= (ObjsOnFPage - FPOLess));
   }
  return ObjIdx;
 }
  else return RangeArr.Arr[BlkIdx];    // Precalculated for all 32 blocks
}
//-------------------------------------------------------------
static uint CalcForIndex(size_t ObjIdx, size_t& Idx)      // TODO: Check if ObjIdx is outside of valid range? (For now this function is branchless)
{
 if constexpr (NoCorr)   // Number of objects in a block is Pow2 as the block itself (No tail bytes, no headers in each block)  // If NHdrSize is not NULL then use of the table is faster than calculations
  {
   SCVR size_t ObjsOnFPage   = self::PageSize / self::UnitSize;      // No FHdr, Pow2 only
   SCVR size_t ObjBaseBitIdx = ctz(ObjsOnFPage);
   if constexpr (self::FHdrSize)      // First block contains the header       // FHdrSize != NHdrSize ?????
    {
     SCVR size_t FPOLess = ObjsOnFPage - ((self::PageSize-self::FHdrSize) / self::UnitSize);     // How many objects we lose because of the header
     ObjIdx  +=  FPOLess * (ObjIdx >= (ObjsOnFPage - FPOLess));          // This one is probably better since FPOLess may happen to be Pow2 and optimized to aleft shift // !OPTIMIZE!: Will it be brancless on ARM too?   // if(Index >= (ObjsOnFPage - FPOLess))Index += FPOLess;
    }
   size_t ratio = (ObjIdx >> ObjBaseBitIdx) + 1;     // Reduce and round-up
   size_t BIdx  = BitWidth(ratio >> 1);
   size_t ISub  = ((size_t)1 << (BIdx + ObjBaseBitIdx)) - ObjsOnFPage;
   Idx = ObjIdx - ISub;
   return BIdx;   // The Block index
  }
 else  // Uncomputable number of objects per block (Wasted tails, headers in each block)
  {
   SCVR uint32 PageBitIdx = (BitSize<size_t>::V - clz(self::PageSize));  // - 1;  PageSize itself is index 0    // Must be done at compile time somehow // Initial page size will be block index 0
   uint64 ObjByteOffs  = ObjIdx * self::UnitSize;                  // As in a contigous memory block
   uint32 ApproxGrpIdx = (BitSize<uint64>::V - clz(ObjByteOffs >> PageBitIdx));     // log2    // Actual page-aligned address   // Approximate (there are some wasted bytes at end of each block because an object cannot be split between blocks. But the index calculation is done as if for a single contiguous block of memory)
   ApproxGrpIdx += (ObjIdx >= RangeArr.Arr[1 + ApproxGrpIdx]);  // The error will not cross more than one block because each next block is twice as large than a previous one
   Idx = (ObjIdx - RangeArr.Arr[ApproxGrpIdx]);     // Sometimes means rereading from the same array position
   return ApproxGrpIdx;  // The Block index
  }
}
//-------------------------------------------------------------
#ifdef DBGBUILD
static uint BruteForIndex(size_t ObjIdx, size_t& Idx)  // For testing
{
 size_t Ps   = 0;
 size_t HLen = self::FHdrSize;
 for(int idx=0;idx < self::RangeMin;idx++)
  {
   size_t Nx = (((self::PageSize << idx)-HLen)/self::UnitSize);
   size_t Lx = Ps;
   Ps += Nx;
   if(ObjIdx < Ps){Idx = ObjIdx-Lx; return idx;}
   HLen = self::NHdrSize;
  }
 Idx = -1;
 return -1;
}
#endif
//-------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
template<uint32 StratType, size_t LBlkMin=0, size_t UnitLen=0x100, size_t PageLen=0x1000, size_t FHdrLen=0, size_t NHdrLen=0> struct SSel
{
 STASRT(StratType <= afGrowExp, "Wrong grow strategy!");
 using T = TSW<StratType==afGrowExp, SBASExp<UnitLen,PageLen,FHdrLen,NHdrLen,LBlkMin>, typename TSW<StratType==afGrowLin, SBASLin<UnitLen,PageLen,FHdrLen,NHdrLen,LBlkMin>, SBASUni<UnitLen,PageLen,FHdrLen,NHdrLen,LBlkMin> >::T>::T;
};
//============================================================================================================
//                                          The Allocator
//------------------------------------------------------------------------------------------------------------
// NOTE: Allocations may fail depending on available memory and address space.
//
template<size_t PageLen, uint32 Flg, typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemPrvBase, size_t MaxAlign=DefMaxAlign, size_t LBlkMin=0> class SBlkAlloc   // Multiple linear ranges    // Alignment 64 is enough even for AVX512
{
SCVR bool   EmptyMP   = sizeof(MP) <= 1;    // MP will have 1 size without any data members    // The MP size should be either 0 or > 1
SCVR bool   EmptyTB   = sizeof(TBIfo) <= 1;
SCVR bool   EmptyTC   = sizeof(TCIfo) <= 1;
SCVR uint32 StratType = Flg & afGrowMask;

using TBStrat = SSel<StratType>::T;    // To get access to range constants

SCVR bool SepBlkIdx = TBStrat::RangeMax != TBStrat::RangeMin;    // Block index array is a separate allocation (For strategies that require large indexes for blocks)
SCVR bool MetaInBlk = (Flg & afBlkTrcOwner);                     // Metadata is added to every block (To find the block owner)  // Forces context to be in the first block
SCVR bool CtxInFBlk = MetaInBlk || (Flg & afSinglePtr);          // The context is in the first block (Forced by MetaInBlk)
SCVR bool BCtxInIdx = !EmptyTB && (Flg & afBlkCtxInIdx);         // The context will not be stored in each block but in index table instead

struct SBlkPtrA
{
 vptr Ptr;        // Pointer to the allocated block
};
struct SBlkPtrB
{
 vptr  Ptr;        // Pointer to the allocated block
 TBIfo Inf;
};
using SBlkPtr = TSW< BCtxInIdx, SBlkPtrB, SBlkPtrA >::T; //   // May also include TBIfo

struct SBlkArrI    // Only for SBASExp
{
 size_t  BlkNum;    // Max number of blocks that ever been allocated     // uint32 + padding?
 SBlkPtr BlkArr[TBStrat::RangeMin];

 static _finline uint GetArrMax(void) {return sizeof(BlkArr)/sizeof(vptr);}
};

struct SBlkArrP    // SepBlkIdx: Not SBASExp
{
 size_t   BlkNum;  // No allocated blocks is expected starting from this one. But in the range some may be NULL
 size_t   BlkLen;  // Number of pointers total to fit in the array
 SBlkPtr* BlkArr;  // Separate allocation to be able to grow.
 SBlkPtr  Local[TBStrat::RangeMin];   // BlkArr Should point here initially   // TODO: Fine tune size in different strategies

 _finline uint  GetArrMax(void) const {return BlkLen;}
};

struct SMFHdr { vptr Self; };  // Points to beginning of the block(This SHdr)       // Only if MetaInBlk and CtxInFBlk

struct SFHdr: TSW< MetaInBlk, SMFHdr, SEmptyType >::T     // For a first block // Cannot be part of the allocator if afBlkMetadata is specified because the allocator may be moved // Can be member of SObjAlloc, at beginning of block index or at beginning of the first block(bad idea). Should not be moved(Pointer to it may be stored in each block)
{
 MP* MProv;
 TSW< SepBlkIdx, SBlkArrP, SBlkArrI >::T Blocks;
 TCIfo  CtxInfo[!EmptyTC];    // If not [] then will consume memory even if empty but [0] fixes that
 _finline static vptr   ExtractPtr(vptr ptr) { return vptr((size_t)ptr & Strat::PtrMask); }
 _finline static uint32 ExtractTag(vptr ptr) { return uint32((size_t)ptr & Strat::TagMask); }   // Max 4095 for 4K aligned pointers

 inline uint     GetArrLen(void) const {return this->Blocks.BlkNum;}
 inline uint     GetArrMax(void) const {return this->Blocks.GetArrMax();}
 inline SBlkPtr* GetBlkArr(void) {return this->Blocks.BlkArr;}
 inline vptr     GetBlkPtr(size_t BlkIdx) {return ExtractPtr(this->GetBlkArr()[BlkIdx].Ptr);}
 inline uint32   GetBlkTag(size_t BlkIdx) {return ExtractTag(this->GetBlkArr()[BlkIdx].Ptr);}
 inline void     SetBlkTag(size_t BlkIdx, uint32 Tag)      // It is important to have no checks if it is NULL or not
 {
  vptr*  Ptr  = &this->GetBlkArr()[BlkIdx].Ptr;
  size_t Addr = (size_t)*Ptr & Strat::PtrMask;
  *Ptr = vptr(Addr | (Tag & Strat::TagMask));
 }
//-------------------------------------------------------------
};

struct SFHdrEx: SFHdr  // If the context is in the first block
{
 TBIfo  BlkInfo[(!EmptyTB && !BCtxInIdx)];
};

struct SMNHdr: SMFHdr { SFHdr* Ctx; };
struct SNHdr: TSW< MetaInBlk, SMNHdr, SEmptyType >::T     // If no MetaInBlk and no CtxInFBlk then this header is used in first block too
{
 TBIfo  BlkInfo[(!EmptyTB && !BCtxInIdx)];
};

struct SSerBlk
{
 uint64 DLen;     // 0 for deleted blocks
 uint32 DOffs;    // Useless?  // Actual header offsets is max 65536
 uint32 Tag;
};
struct SSerHdr  // 32 bytes    // For Save/Load
{
 uint8  SigH;   // 'B'
 uint8  SigL;   // 'Q'
 uint8  Strat;  // Strat type
 uint8  Flags;  // Unused
 uint16 LenCI;
 uint16 LenBI;
 uint16 NHdrLen;
 uint16 FHdrLen;
 uint32 UnitLen;
 uint64 BlkNum;
 uint64 PageSize;  // Initial block size
};
static_assert(sizeof(SSerBlk) == 16);
static_assert(sizeof(SSerHdr) == 32);

// Is this the most appropriate alignment strategy?
SCVR size_t AlUnitLen = (Flg & afObjAlign)  ? ( (Flg & afAlignPow2)?(AlignToP2Up(sizeof(Ty))):(AlignP2Frwd(sizeof(Ty), sizeof(size_t))) ):(sizeof(Ty));   // May align to nearest Pow@ or by pointer size, depending what is the best for the current strategy
SCVR size_t AlNHdrLen = (sizeof(SNHdr) > 1) ? ( (!(Flg & afLimitHdrAl) || (AlUnitLen <= MaxAlign))?(AlignFrwd(sizeof(SNHdr),AlUnitLen)):(AlignP2Frwd(sizeof(SNHdr),MaxAlign)) ):0;              // May align to UnitSize
SCVR size_t AlFHdrLen = CtxInFBlk ? ( (!(Flg & afLimitHdrAl) || (AlUnitLen <= MaxAlign))?(AlignFrwd(sizeof(SFHdrEx),AlUnitLen)):(AlignP2Frwd(sizeof(SFHdrEx),MaxAlign)) ): AlNHdrLen;

using Strat = SSel<StratType, AlUnitLen, PageLen, AlFHdrLen, AlNHdrLen>::T;
//static_assert(size_t(Strat::UnitSize) != size_t(0), "UnitSize is zero!");

TSW< CtxInFBlk, SFHdr*, SFHdr >::T Context;
//-------------------------------------------------------------
static bool IsCtxMP(SFHdr* ptr) {if constexpr (CtxInFBlk)return (size_t)ptr & 1; return false;}   // NOTE: High unused bits may be actually used by the system for some tagging
//-------------------------------------------------------------
inline MP* GetMP(void)
{
 if constexpr (!EmptyMP)
  {
   if constexpr (CtxInFBlk)
    {
     if((size_t)this->Context & 1)return (MP*)((size_t)this->Context - 1);     // MProv is in Context pointer (Temporarily)
      else return this->Context->MProv;
    }
     else return ((SFHdr*)&this->Context)->MProv;
  }
   else return (MP*)nullptr;   // no data members
}
//-------------------------------------------------------------
_finline SFHdr* GetCtx(void)  // _ninline
{
 if constexpr (CtxInFBlk)return this->Context;     // May be NULL if not initialized yet (GrowBlkIdxArrFor will do that)      // Return nullptr if bit 1 is set (ptr to MP)?
 return (SFHdr*)&this->Context;     // NOTE: Clang will not detect correctly all exit points in constexpr if-else (when inlined) and will go insane
}
//-------------------------------------------------------------
// Will expand index table but will not allocate blocks between(except block 0) - leaves the array sparse
// NOTE: No check if the block is already allocated
//
vptr AllocBlock(size_t BlkIdx)
{
 size_t BSize = (size_t)Strat::BlkIdxToSize(BlkIdx);
 vptr   BPtr  = this->GetMP()->Alloc(BSize);
 if(!BPtr)return nullptr;
 if constexpr (CtxInFBlk)
  {
   if(!this->GetCtx())   // No context is allocated yet
    {
     if(BlkIdx)          // Must make sure that the first block exist  // No need to allocate index for block 0 anyway (Stored locally)
      {
       if(!this->AllocBlock(0))return nullptr;
       // new (BPtr) SNHdr();   // Construct BlockInfo
      }
       else
        {
         this->Context = (SFHdr*)BPtr;
        // new (BPtr) SFHdrEx();  // Construct ContextInfo and BlockInfo // Whatever it is, the user probably expects it to be constructed properly     // TODO: ???
        }
    }
  }
 SFHdr* ctx = this->GetCtx();   // Will be available now
 if constexpr (SepBlkIdx)   // Init/Grow the index array if required
  {
   SBlkArrP* ablk = (SBlkArrP*)&ctx->Blocks;
   if(!ablk->BlkArr){ablk->BlkArr = ablk->Local; ablk->BlkLen = TBStrat::RangeMin;}
   if(BlkIdx >= ablk->GetArrMax())
    {
     uint NMax = AlignP2Frwd(BlkIdx+1, NPTM::MEMPAGESIZE/sizeof(vptr));
     vptr IPtr = (vptr)NPTM::NAPI::mmap(nullptr, NMax*sizeof(vptr), PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);    // TODO: afObjAlign ???????????????   // TODO: Alloc/Free holder class as an argument (Assume that we wat allocation on GPU or from some pool)
     if(NPTM::GetMMapErrFromPtr(IPtr)){ this->GetMP()->Free(BPtr, BSize); return nullptr; }    // Tolal failure, especially on the first block

     vptr   OPtr = (vptr)ablk->BlkArr;
     size_t OLen = ablk->GetArrMax()*sizeof(vptr);
     NMOPS::MemCopy(IPtr, OPtr, OLen);

     ablk->BlkArr = (SBlkPtr*)IPtr;    // Separate array
     ablk->BlkLen = NMax;     // Max number of block slots available
     if(OPtr != ablk->Local)NPTM::NAPI::munmap(OPtr, OLen);
    }
  }
 if constexpr(MetaInBlk)    // MetaInBlk Forces CtxInFBlk
  {
   if(BlkIdx)
    {
     SNHdr* hdr = (SNHdr*)BPtr;
     hdr->Self  = BPtr;
     hdr->Ctx   = ctx;
    }
     else ctx->Self = vptr((size_t)BPtr | 1);    // First block, mark it
  }
 vptr*  Adr = &ctx->GetBlkArr()[BlkIdx].Ptr;
 size_t Tag = (size_t)*Adr & Strat::TagMask;     // Preserve some old tag
 *Adr = vptr((size_t)BPtr | Tag);     // Store the block ptr in the index array
 if(BlkIdx >= ctx->Blocks.BlkNum)ctx->Blocks.BlkNum = BlkIdx+1;
 return BPtr;   // Return Offset to beginning or to data?
}
//-------------------------------------------------------------
public:
//-------------------------------------------------------------
//SBlkAlloc(void) = delete;

_finline SBlkAlloc(MP* mp=(MP*)nullptr){this->Init(mp);}   // Not all memory providers use contexts so nullptr as default is OK
_finline ~SBlkAlloc(){this->Release();}
//-------------------------------------------------------------
inline void Init(MP* mp=(MP*)nullptr)
{
 //NMOPS::ZeroObj(this);
 if constexpr (CtxInFBlk)this->Context = nullptr;    // The context on 'this' is a single pointer
   else NMOPS::ZeroObj(this);
 if constexpr (!EmptyMP)
  {
   if constexpr (CtxInFBlk)this->Context = (SFHdr*)((size_t)mp | 1);     // In most cases MP is zero    // NOTE: This will make the GetCtx to return an invalid context pointer until first call to AllocBlock
     else this->Context.MProv = mp;
  }
}
//-------------------------------------------------------------
int Release(void)               // Free everything, including tables and metadata if any
{
 SFHdr* ctx    = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return 0; }      // Not initialized
 size_t BASize = ctx->GetArrMax();
 vptr   BAPtr  = (vptr)ctx->GetBlkArr();   // May be in a separate block
 MP*    mpov   = this->GetMP();
 int    res    = 0;
 for(sint bidx=ctx->GetArrLen()-1;bidx >= 0;bidx--)  // Last block to free may contain the context
  {
   vptr   blk  = ctx->GetBlkPtr(bidx);
   if(!blk)continue;
   size_t blen = Strat::BlkIdxToSize(bidx);
   if(!mpov->Free(blk, blen))res--;
  }
 if constexpr(SepBlkIdx)
   if(BAPtr && (BASize > Strat::RangeMin))NPTM::NAPI::munmap(BAPtr, BASize * sizeof(vptr));  // The block index was a separate allocation     //  && !mpov->Free(BAPtr, BASize * sizeof(vptr)))res -= 100;  The memory provider is not for index?
 //if constexpr (CtxInFBlk)this->Context = nullptr;
 //  else NMOPS::ZeroObj(&this->Context);
 this->Init(mpov);
 return res;
}
//-------------------------------------------------------------
// Will not preserve access to NULL pointers and tags of last deleted blocks (Decrements the counter)
//
int DeleteBlk(size_t BlkIdx, size_t BlkCnt=1)     // Deallocates a single block
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return -0x10000000; }
 if(BlkIdx >= ctx->Blocks.BlkNum)return -0x20000000;  // Out of range
 if constexpr (CtxInFBlk)  // The context is in first block
   if(!BlkIdx)return -0x30000000;     // Cannot deallocate the first block
 int    res    = 0;
 vptr*  BAPtr  = ctx->GetArrPtr();
 size_t EndIdx = BlkIdx + BlkCnt;
 if(EndIdx > ctx->Blocks.BlkNum)EndIdx = ctx->Blocks.BlkNum;
 for(;BlkIdx < EndIdx;BlkIdx++)
  {
   vptr blk = vptr((size_t)BAPtr[BlkIdx] & Strat::PtrMask);
   if(blk)      // Skip already deleted blocks
    {
     BAPtr[BlkIdx] = (vptr)this->GetBlkTag(BlkIdx);     // Set the pointer to NULL but Leave the tag intact
     size_t blen = Strat::BlkIdxToSize(BlkIdx);
     if(!this->GetMP()->Free(blk, blen))res--;
    }
  }
 if(EndIdx == ctx->Blocks.BlkNum)ctx->Blocks.BlkNum = BlkIdx;   // It is last of non NULL pointers
 return res;
}
//-------------------------------------------------------------
int Ensure(size_t ElmIdx)        // Allocates the block that must contain
{
 return this->Expand();
}
//-------------------------------------------------------------
int Expand(size_t ElmIdx)        // Allocate blocks so that ElmIdx will exist (Will not reallocate already deleted blocks. Only allocates new blocks up to ElmIdx)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return -0x10000000; }
 size_t BlkIdx  = Strat::CalcForIndex(ElmIdx, ElmIdx);
 size_t CurrIdx = ctx->Blocks.BlkNum;
 int    res     = 0;
 if(BlkIdx >= CurrIdx)
  {
   for(;CurrIdx <= BlkIdx;CurrIdx++)              // Allocate new blocks up to BlkIdx  (Make optional?)
    if(!this->AllocBlock(CurrIdx))res--;
  }
   else   // The block is already in range
    {
     if(!ctx->GetBlkPtr(BlkIdx))   // This block was deleted
      if(!this->AllocBlock(BlkIdx))res--;
    }
 return res;
}
//-------------------------------------------------------------
// TODO: Change the strategy so that items before ElmIdx in the same block are preserved?
//
int Shrink(size_t ElmIdx)     // Removes all blocks after and including the one with ElmIdx (And any other items before the ElmIdx which is in the same block)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return -0x10000000; }
 size_t BlkIdx = Strat::CalcForIndex(ElmIdx, ElmIdx) + 1;
 size_t Total  = ctx->GetArrLen();
 if(BlkIdx >= Total)return -0x20000000;     // Will not grow
 return this->DeleteBlk(BlkIdx, Total-BlkIdx);
}
//-------------------------------------------------------------
inline vptr GetBlock(size_t BlkIdx)   // Allocates the block if it is not allocated yet
{
 if constexpr (!CtxInFBlk)     // The context is always available
  {
   SFHdr* ctx = this->GetCtx();
   if(BlkIdx < ctx->GetArrMax())           // In bounds
     if(vptr ptr=ctx->GetBlkPtr(BlkIdx);ptr)return ptr;      // Already allocated  // Useless if GrowBlkIdxArrFor returns 0 (Has grown) which is rare
  }
 else if(SFHdr* ctx=this->GetCtx();ctx)    // May be not initialized yet
  {
   if(BlkIdx < ctx->GetArrMax())           // In bounds
     if(vptr ptr=ctx->GetBlkPtr(BlkIdx);ptr)return ptr;
  }
 return this->AllocBlock(BlkIdx);
}
//-------------------------------------------------------------
// If we would allocated blocks of same Pow2 size and at addresses that are only multiples of that size then it would be easy to find base of any such block.
// But it is too complicated to get from OS an allocation at specific address granularity, especially on Linux
//
vptr FindBlkBase(Ty* ElmAddrInBlk)   // NOTE: More grown the block - Slower this function!    // The element must know type of its container   // NOTE: Will crash if an invalid address is passed!
{
 if constexpr(MetaInBlk)   // Can't iterate if there is no metadata at beginning of each block
  {
   for(size_t pbase=AlignP2Bkwd((size_t)ElmAddrInBlk, NPTM::MEMGRANSIZE);;pbase -= NPTM::MEMGRANSIZE)   // Cannot step by Strat::PageSize because allocation address is NPTM::MEMGRANSIZE aligned
    {
     SMFHdr* hdr = (SMFHdr*)pbase;
     if(((size_t)hdr->Self & Strat::PtrMask) == pbase)return (vptr)pbase;     // Nothing else to check here
    }
  }
 return nullptr;
}
//-------------------------------------------------------------
inline bool IsBlkExist(size_t BlkIdx)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return false;}
 return (bool)ctx->GetBlkPtr(BlkIdx);
}
//-------------------------------------------------------------
inline bool IsElmExist(size_t ElmIdx)     // Checks if a block for the element is allocated
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return false;}
 return (bool)ctx->GetBlkPtr(Strat::CalcForIndex(ElmIdx, ElmIdx));
}
//-------------------------------------------------------------
Ty* GetElm(size_t ElmIdx)   // Returns NULL if the element is not present
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return nullptr;}
 size_t BEIdx;
 size_t BlkIdx = Strat::CalcForIndex(ElmIdx, BEIdx);
 size_t blkp   = (size_t)ctx->GetBlkPtr(BlkIdx);
 if(!blkp)return nullptr;
 size_t HSize  = Strat::BlkIdxToHdrLen(BlkIdx);
 return ((Ty*)(blkp + HSize)) + BEIdx;
}
//-------------------------------------------------------------
Ty* GetElm(size_t ElmIdx, size_t* EndSeqIdx)   // Returns NULL if the element is not present
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return nullptr;}
 size_t BEIdx;
 size_t BlkIdx = Strat::CalcForIndex(ElmIdx, BEIdx);
 size_t blkp   = (size_t)ctx->GetBlkPtr(BlkIdx);
 if(!blkp)return nullptr;
 size_t HSize  = Strat::BlkIdxToHdrLen(BlkIdx);
 *EndSeqIdx = (((size_t)Strat::BlkIdxToSize(BlkIdx) - HSize) / Strat::UnitSize);
 return ((Ty*)(blkp + HSize)) + BEIdx;
}
//-------------------------------------------------------------
inline Ty* GetBlkData(size_t BlkIdx, size_t BElmIdx)
{
 size_t Ptr = (size_t)this->GetBlock(BlkIdx); // May allocate the block (do not pass some random numbers as BlkIdx)
 if(!Ptr)return nullptr;                      // The block is not allocated
 size_t EOffs = BElmIdx * Strat::UnitSize;
 size_t HSize = Strat::BlkIdxToHdrLen(BlkIdx);
 size_t DSize = (size_t)Strat::BlkIdxToSize(BlkIdx) - HSize;   // NOTE: May trim the size on x32, no checks
 if(EOffs >= DSize)return nullptr;            // The element is out of range for the block
 return (Ty*)(Ptr + HSize + EOffs);
}
//-------------------------------------------------------------
// ElmIdx: In=Index of an element, Out=Index of that element in the block
// Size: Full size of the block. Offs: Offset to first element in the block
//
static inline uint GetRange(size_t ElmIdx, size_t& BlkIdx, size_t& Size, size_t& Offs)   // Used by iterators, should be branchless
{
 BlkIdx = Strat::CalcForIndex(ElmIdx, ElmIdx);   // NOTE: No ElmIdx check. CalcForIndex may return nonsense!
// if(BlkIdx >= ctx->Blocks.BlkNum)return -1;  // Out of range (The index part itself may be not allocated yet)
 Size = (size_t)Strat::BlkIdxToSize(BlkIdx);    // NOTE: May trim the size on x32, no checks
 Offs = Strat::BlkIdxToHdrLen(BlkIdx);
 return ElmIdx;
}
//-------------------------------------------------------------
static inline uint GetBlkSize(size_t BlkIdx) {return (size_t)Strat::BlkIdxToSize(BlkIdx);}  // NOTE: May trim the size on x32, no checks
static inline uint GetElmOffs(size_t BlkIdx) {return Strat::BlkIdxToHdrLen(BlkIdx);}
static inline uint GetBlkEIdx(size_t& ElmIdx) {size_t BlkIdx = Strat::CalcForIndex(ElmIdx, ElmIdx); return BlkIdx;}
//-------------------------------------------------------------
inline bool   IsBlkInRange(size_t BlkIdx) {return BlkIdx < this->GetCtx()->GetArrLen();}  // If the block is in the block index range   // For iterators
inline void   EndBlkPtr(size_t BlkIdx){}      // Finish any work with the ptr and release it     // TODO: Mark all functions that may require this!!!!!!!!!!!!!!!!!!
inline vptr   GetBlkPtr(size_t BlkIdx) {return vptr((size_t)this->GetCtx()->GetBlkPtr(BlkIdx));}       // TODO: Call some provided locking function? The memory may belong to some device and the allocation function may return a handle instead of a pointer to it
inline uint32 GetBlkTag(size_t BlkIdx) {return this->GetCtx()->GetBlkTag(BlkIdx);}
inline void   SetBlkTag(size_t BlkIdx, uint32 Tag) {return this->GetCtx()->SetBlkTag(BlkIdx, Tag);}      // It is important to have no checks if it is NULL or not
//-------------------------------------------------------------
inline TCIfo* GetCtxInfo(void)
{
 if constexpr(!EmptyTC)return &this->GetCtx()->CtxInfo;
  else return nullptr;
}
//-------------------------------------------------------------
// Get base context by any block
//
static inline TCIfo* GetCtxInfo(vptr BlkBase)         // NOTE: Use FindBlock to slowly find the BlkBase
{
 if constexpr(!EmptyTC && MetaInBlk && CtxInFBlk)
  {
   if((size_t)((SMFHdr*)BlkBase)->Self & 1)return &((SFHdr*)BlkBase)->CtxInfo;  // CtxInFBlk
     else return &((SMNHdr*)BlkBase)->Ctx->CtxInfo;       // MetaInBlk
  }
  else return nullptr;
}
//-------------------------------------------------------------
inline TBIfo* GetBlkInfo(size_t BlkIdx)        // NOTE: No range checks!
{
 if constexpr(!EmptyTB)
  {
   if constexpr (!BCtxInIdx)
    {
     if constexpr(CtxInFBlk)
      {
       if(!BlkIdx)return &((SFHdrEx*)this->GetCtx())->BlkInfo;     // First block (0)
      }
     SFHdr* ctx = this->GetCtx();
//     if(!ctx)return nullptr;
     vptr ptr = ctx->GetBlkPtr(BlkIdx);
     if(!ptr)return nullptr;          // The block may be deleted
     return &((SNHdr*)ptr)->BlkInfo;
    }
     else    // Block contexts are in index table
      {
       SFHdr* ctx = this->GetCtx();
//       if(!ctx)return nullptr;
       return &(ctx->GetBlkArr())[BlkIdx].Inf;   // NOTE: No bounds check!
      }
  }
  else return nullptr;
}
//-------------------------------------------------------------
inline TBIfo* GetBlkInfo(vptr BlkBase)          // Does not require MetaInBlk or CtxInFBlk
{
 if constexpr(!EmptyTB)
  {
   if constexpr (!BCtxInIdx)
    {
     if constexpr(MetaInBlk)
      {
       if((size_t)((SMFHdr*)BlkBase)->Self & 1)return &((SFHdrEx*)BlkBase)->BlkInfo;    // This block is first  CtxInFBlk      // 'Self' is only present if MetaInBlk
      }
       else if constexpr (CtxInFBlk){ if(BlkBase == this->GetBlkPtr(0))return &((SFHdrEx*)BlkBase)->BlkInfo; }   // This block is first  CtxInFBlk
     return &((SMNHdr*)BlkBase)->BlkInfo;
    }
     else    // Block contexts are in index table    // SLOOOOW! - Need to find this block in array
      {
       SFHdr* ctx = this->GetCtx();
       SBlkPtr* arr = ctx->GetBlkArr();
       for(uint idx=0,tot=ctx->GetArrLen();idx < tot;idx++)
        {
         SBlkPtr* elm = &arr[idx];
         if(ctx->ExtractPtr(elm->Ptr) == BlkBase)return &elm->Inf;
        }
      }
  }
  else return nullptr;
}
//-------------------------------------------------------------
size_t GetElmMax(void)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return 0;}
 uint len = ctx->GetArrLen();
 if(!len)return 0;
 return Strat::BlkIdxToObjNum(len-1);
}
//-------------------------------------------------------------
// It is able to copy blocks, preserving indexes (Target must have same block header sizes)
// NOTE: Original memory provider is preserved
// NOTE: Works! But why it takes twice time of storing or matching???
//
bool Duplicate(SBlkAlloc& to)  // NOTE: Types must match - duplicated by blocks
{
// static_assert(to.AlUnitLen == this->AlUnitLen);     // None of such checks are doable in GCC mode
// static_assert(to.AlNHdrLen == this->AlNHdrLen);
// static_assert(to.AlFHdrLen == this->AlFHdrLen);
 to.Release();

 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return false; }      // Not initialized
 for(uint idx=0,tot=ctx->GetArrLen();idx < tot;idx++)  // Last block to free may contain the context   // Should not use AllocBlock because it may reallocate indexes
  {
   vptr  SrcBlk = this->GetBlkPtr(idx);
   if(!SrcBlk)continue;
   vptr  TgtBlk = to.AllocBlock(idx);          // It may reallocate index blocks when become full
   if(!TgtBlk)return false;
   TBIfo* TgtTB = to.GetBlkInfo(TgtBlk);
   TBIfo* SrcTB = this->GetBlkInfo(SrcBlk);
   if(TgtTB && SrcTB)memcpy(TgtTB, SrcTB, sizeof(*SrcTB));   // Copy per block user info

   size_t Size = (size_t)Strat::BlkIdxToSize(idx);
   size_t Offs = Strat::BlkIdxToHdrLen(idx);   // Most likely aligned to pointer size or even more
   for(uint64* Dst=(uint64*)((uint8*)TgtBlk + Offs),*Src=(uint64*)((uint8*)SrcBlk + Offs),*DEnd=(uint64*)((uint8*)TgtBlk + Size); Dst < DEnd;Dst++,Src++)*Dst = *Src;  // Three times faster than storing or matching!
   //memcpy((uint8*)TgtBlk + Offs, (uint8*)SrcBlk + Offs, Size - Offs);    // Very slow, twice of storing, even in RELEASE !!!!!!!!!!!!!!!!!!!!!!! // Slightly better without VectorizeMemOps    // Too many conditional jumps!!!
  }
 for(uint idx=0,tot=ctx->GetArrLen();idx < tot;idx++)to.SetBlkTag(idx, this->GetBlkTag(idx));  // Transfer tags to allocated/deleted block list (Requires full index list block)
 TCIfo* TgtTC = to.GetCtxInfo();
 TCIfo* SrcTC = this->GetCtxInfo();
 if(TgtTC && SrcTC)memcpy(TgtTC, SrcTC, sizeof(*SrcTC));    // Copy main user info
 return true;
}
//-------------------------------------------------------------
size_t MemoryUsed(void)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return 0; }      // Not initialized
 size_t len = 0;
 for(uint idx=0,tot=ctx->GetArrLen();idx < tot;idx++)  // Last block to free may contain the context
  {
   vptr blk = ctx->GetBlkPtr(idx);
   if(blk)
    {
     size_t blen = Strat::BlkIdxToSize(idx);
     len += blen;
//     DBGMSG("Block %u of size: %u",idx,blen);
    }
  }
 if constexpr(SepBlkIdx)
  {
   size_t BASize = ctx->GetArrMax();
   auto   BAPtr  = ctx->GetBlkArr();   // May be in a separate block
   if(BAPtr && (BASize > Strat::RangeMin))
    {
     size_t ilen = AlignP2Frwd((BASize * sizeof(vptr)), NPTM::MEMPAGESIZE);
     len += ilen;
//     DBGMSG("Index block Size: %u",ilen);
    }
  }
 return len;
}
//-------------------------------------------------------------
// TODO: Must avoid saving any padding data????
// MaxItems: just a hint. Can't know if a block is almost empty, MaxItems may help to avoild saving unused remaining of a last block (It is max expected size of data in all blocks(excluding headers and padding))
//
size_t Save(auto& Strm, size_t MaxItems=(size_t)-1)         // Not insisting on use of specific stream class
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return -1; }      // Not initialized
 SSerHdr hdr;
 hdr.SigH     = 'B';
 hdr.SigL     = 'Q';
 hdr.Strat    = StratType;
 hdr.Flags    = bool(Flg & afBlkCtxInIdx);   // NOTE: Only one flag for now
 hdr.LenCI    = sizeof(TCIfo);
 hdr.LenBI    = sizeof(TBIfo);
 hdr.NHdrLen  = AlNHdrLen;
 hdr.FHdrLen  = AlFHdrLen;
 hdr.UnitLen  = AlUnitLen;
 hdr.BlkNum   = ctx->GetArrLen();
 hdr.PageSize = PageLen;
 size_t res = Strm.Write(&hdr, sizeof(SSerHdr));
 if(Strm.IsFail(res))return res;

 // Saving blocks info  // Final deleted blocks and their tags are not preserved
 size_t DataSize = 0;    // Total actual data
 size_t DataMax  = (MaxItems != (size_t)-1)?(MaxItems*AlUnitLen):((size_t)-1);
 uint32   BrcIdx = 0;
 SCVR int MaxBRC = 32;
 SSerBlk BlkRecs[MaxBRC];
 for(uint idx=0,tot=hdr.BlkNum,lst=tot-1;idx < tot;idx++)  // Last block to free may contain the context
  {
   SSerBlk* brec = &BlkRecs[BrcIdx];
   brec->Tag = this->GetBlkTag(idx);
   vptr  blk = ctx->GetBlkPtr(idx);
   if(blk)
    {
     size_t hlen = Strat::BlkIdxToHdrLen(idx);
     size_t blen = Strat::BlkIdxToSize(idx) - hlen;
     size_t dlen = blen - (blen % AlUnitLen);         // May be leftover bytes at end of blocks
     brec->DOffs = hlen;
     DataSize   += dlen;
     if((idx == lst) && (DataSize > DataMax))dlen -= (DataSize - DataMax);   // Skip overallocated items in the last block
     brec->DLen  = dlen;
    }
     else brec->DLen = brec->DOffs = 0;   // Deleted block   // Storing descriptors of deleted blocks allows to preserve indexes to data items
   BrcIdx++;
   if(BrcIdx >= MaxBRC)
    {
     res = Strm.Write(&BlkRecs, sizeof(SSerBlk) * BrcIdx);
     if(Strm.IsFail(res))return res;
     BrcIdx = 0;
    }
  }
 if(BrcIdx)
  {
   res = Strm.Write(&BlkRecs, sizeof(SSerBlk) * BrcIdx);
   if(Strm.IsFail(res))return res;
  }

 if(!EmptyTC)
  {
   TCIfo* tc = this->GetCtxInfo();
   res = Strm.Write(tc, sizeof(TCIfo));
   if(Strm.IsFail(res))return res;
  }
 if(!EmptyTB)
  {
   for(uint idx=0,tot=hdr.BlkNum;idx < tot;idx++)
    {
     TBIfo* tb = this->GetBlkInfo(idx);
     if(!tb)continue;
     res = Strm.Write(tb, sizeof(TBIfo));    // May be quite many calls (StratUNI)  // Use some caching?
     if(Strm.IsFail(res))return res;
    }
  }
 Strm.Write(nullptr,0);    // Inform the stream writer that we are done writing headers

 // Saving bulk of actual items (your stream writer may apply some transformation/compression)
 DataSize = 0;
 for(uint idx=0,tot=hdr.BlkNum,lst=tot-1;idx < tot;idx++)  // Last block to free may contain the context
  {
   vptr blk = ctx->GetBlkPtr(idx);
   if(!blk)continue;   // Deleted
   size_t hlen = Strat::BlkIdxToHdrLen(idx);
   size_t blen = Strat::BlkIdxToSize(idx) - hlen;
   size_t dlen = blen - (blen % AlUnitLen);         // May be leftover bytes at end of blocks
   size_t dsiz = DataSize + dlen;
   if((idx == lst) && (dsiz > DataMax)){dlen -= (dsiz - DataMax);}   // Skip overallocated items in the last block
   DataSize += dlen;
   res = Strm.Write((uint8*)blk + hlen, dlen);
   if(Strm.IsFail(res) || (res != dlen))return res;
  }
 Strm.Write(nullptr,0);  // Done writing data
 return DataSize;     // Return size of saved items (no headers)
}
//-------------------------------------------------------------
// NOTE: Will refuse to load blocks of incompatible sizes (alloc strategy) or with incompatible header sizes
//
size_t Load(auto& Strm)
{
 this->Release();
 SSerHdr hdr;
 size_t res = Strm.Read(&hdr, sizeof(SSerHdr));
 if(Strm.IsFail(res))return res;
 if(!hdr.BlkNum)return 0;      // No data
 if((hdr.SigH != 'B')||(hdr.SigL != 'Q'))return -10;
 if(hdr.Strat    != StratType)return -11;
 if(hdr.Flags    != bool(Flg & afBlkCtxInIdx))return -12;    // NOTE: Only one flag for now
 if(hdr.LenCI    != sizeof(TCIfo))return -13;
 if(hdr.LenBI    != sizeof(TBIfo))return -14;
 if(hdr.NHdrLen  != AlNHdrLen)return -15;
 if(hdr.FHdrLen  != AlFHdrLen)return -16;
 if(hdr.UnitLen  != AlUnitLen)return -17;
 if(hdr.PageSize != PageLen)return -18;

 size_t DataSize  = 0;
 size_t LastDLen  = 0;
 SCVR int MaxBRC  = 32;
 uint32   BrcIdx  = MaxBRC;
 SSerBlk BlkRecs[MaxBRC];
 if(!this->AllocBlock(hdr.BlkNum-1))return -1;  // Alloc last and first(if needed) blocks
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return -2; }      // Not initialized
 for(uint idx=0,tot=hdr.BlkNum,lst=tot-1;idx < tot;idx++)
  {
   if(BrcIdx >= MaxBRC)
    {
     uint num = MaxBRC;
     uint rem = tot - idx;
     if(rem < num)num = rem;
     res = Strm.Read(&BlkRecs, num * sizeof(SSerBlk));
     if(Strm.IsFail(res))return res;
     BrcIdx = 0;
    }
   SSerBlk* brec = &BlkRecs[BrcIdx++];
   size_t   hlen = Strat::BlkIdxToHdrLen(idx);
   size_t   blen = Strat::BlkIdxToSize(idx) - hlen;
   size_t   dlen = blen - (blen % AlUnitLen);         // May be leftover bytes at end of blocks
   if((dlen < brec->DLen)||((brec->DLen != dlen)&&(idx != lst)))return -3;    // The block size mismatch
   LastDLen  = brec->DLen;
   DataSize += LastDLen;

   ctx->SetBlkTag(idx, brec->Tag);        // Full block index table is already present
   if(!brec->DLen)continue;         // This block is deleted
   if(!ctx->GetBlkPtr(idx) && !this->AllocBlock(idx))return -1;
  }

 if(!EmptyTC)
  {
   TCIfo* tc = this->GetCtxInfo();
   res = Strm.Read(tc, sizeof(TCIfo));
   if(Strm.IsFail(res))return res;
  }
 if(!EmptyTB)
  {
   for(uint idx=0,tot=hdr.BlkNum;idx < tot;idx++)
    {
     TBIfo* tb = this->GetBlkInfo(idx);
     if(!tb)continue;
     res = Strm.Read(tb, sizeof(TBIfo));    // May be quite many calls (StratUNI)  // Use some caching?
     if(Strm.IsFail(res))return res;
    }
  }
 Strm.Read(nullptr,0);    // Inform the stream reader that we are done reading headers

 for(uint idx=0,tot=hdr.BlkNum,lst=tot-1;idx < tot;idx++)
  {
   vptr blk = ctx->GetBlkPtr(idx);
   if(!blk)continue;
   size_t hlen = Strat::BlkIdxToHdrLen(idx);
   size_t dlen = LastDLen;
   if(idx != lst)
    {
     size_t blen = Strat::BlkIdxToSize(idx) - hlen;
     dlen = blen - (blen % AlUnitLen);         // May be leftover bytes at end of blocks
    }
   res = Strm.Read((uint8*)blk + hlen, dlen);
   if(Strm.IsFail(res) || (res != dlen))return res;
  }
 Strm.Read(nullptr,0);  // Done reading data
 return DataSize;     // Return size of loaded items (no headers)
}
//-------------------------------------------------------------
class SIterator    // I do not like clumsy ranged FOR but i need iterator object to hide iteration between blocks. :: for (auto it = alc.begin(), end = alc.end(); it != end; ++it) {const auto i = *it; ... }
{
 Ty*     CPtr;     // Data ptr in the current block (Points to first object, after a header if present)
 Ty*     BPtr;     // Points to first element in current block(after headers) (In elements) (Aligned to 'Ty')   // TODO: Use offsets because those pinters will involve MUL operations
 Ty*     EPtr;     // End of current block (In elements)      // TODO: Must be aligned to 'Ty'
 SFHdr*  Ctx;      // Allocator`s context
 size_t  BIdx;     // Index of current block;
 size_t  EIdx;     // Index of first unit of the block (To avoid doing costly Strat::CalcForIndex again)
 //------------------------------------------------------
 void NextBlock(void)
 {
  auto APtr = this->Ctx->GetBlkArr();
  for(sint idx=(sint)this->BIdx+1,tot=this->Ctx->GetArrLen(); idx < tot; idx++)  // Iterate in case null blocks are present
   {
    if(size_t blk = (size_t)this->Ctx->ExtractPtr(APtr[idx].Ptr); blk)    // (size_t)APtr[idx] & Strat::PtrMask
     {
      size_t BSize = (size_t)Strat::BlkIdxToSize(idx);     // NOTE: May trim the size on x32, no checks
      size_t HSize = Strat::BlkIdxToHdrLen(idx);
      this->EIdx  += this->EPtr - this->BPtr;    // Add number of units in the previous block
      this->BIdx   = idx;
      this->CPtr   = this->BPtr = (Ty*)(blk + HSize);
      this->EPtr   = this->BPtr + ((BSize - HSize) / Strat::UnitSize);     // NOTE: May trim the size on x32, no checks
      return;
     }
   }
  //this->BIdx = this->EIdx = -1;    // reset to END
  //this->EPtr = (Ty*)-1;             // Why -1?????????????
  //this->CPtr = this->BPtr = nullptr;
  this->Reset();
 }
 //------------------------------------------------------
 void PrevBlock(void)
 {
  vptr* APtr = this->Ctx->GetArrPtr();
  for(sint idx=(sint)this->BIdx-1; idx <= 0; idx--)  // Iterate in case null blocks are present
   {
    if(size_t blk = (size_t)APtr[idx] & Strat::PtrMask; blk)
     {
      size_t BSize = (size_t)Strat::BlkIdxToSize(idx);     // NOTE: May trim the size on x32, no checks
      size_t HSize = Strat::BlkIdxToHdrLen(idx);
      this->BIdx   = idx;
      this->BPtr   = (Ty*)(blk + HSize);
      this->CPtr   = this->EPtr = this->BPtr + ((BSize - HSize) / Strat::UnitSize);     // NOTE: May trim the size on x32, no checks
      this->EIdx  -= this->EPtr - this->BPtr;    //Subtract number of units in the current block
      this->CPtr--;
      return;
     }
   }
  //this->BIdx = this->EIdx = -1;    // reset to END
  //this->CPtr = this->BPtr = this->EPtr = nullptr;
  this->Reset();
 }
 //------------------------------------------------------
 void SetBlockFor(size_t ElmIdx)      // NOTE: Will not check if the block is NULL     // TODO: Test it!!!
 {
  size_t BEIdx;
  size_t BlkIdx = Strat::CalcForIndex(ElmIdx, BEIdx);
  size_t BSize  = (size_t)Strat::BlkIdxToSize(BlkIdx);     // NOTE: May trim the size on x32, no checks
  size_t HSize  = Strat::BlkIdxToHdrLen(BlkIdx);
  auto    APtr  = this->Ctx->GetBlkArr();
  size_t  blkp  = (size_t)this->Ctx->ExtractPtr(APtr[BlkIdx].Ptr);      // (size_t)APtr[BlkIdx] & Strat::PtrMask;
  this->EIdx = ElmIdx - BEIdx;
  this->BIdx = BlkIdx;  // ElmIdx - BEIdx;
  this->BPtr = (Ty*)(blkp + HSize);
  this->EPtr = this->BPtr + ((BSize - HSize) / Strat::UnitSize);
  this->CPtr = this->BPtr + BEIdx;
 }
 //------------------------------------------------------
 _finline void CopyFrom(const SIterator* itr)
 {
  this->CPtr = itr->CPtr;
  this->BPtr = itr->BPtr;
  this->EPtr = itr->EPtr;
  this->Ctx  = itr->Ctx;
  this->BIdx = itr->BIdx;
  this->EIdx = itr->EIdx;
 }
 //------------------------------------------------------
 _finline void Reset(void)
 {
  this->BIdx = this->EIdx = -1;    // reset to END
  this->CPtr = this->BPtr = this->EPtr = nullptr;
 }
 //------------------------------------------------------
public:

 inline SIterator(const SIterator& itr)   // Copy constructor
 {
  this->CopyFrom(&itr);   //NMOPS::CopyObj<const SIterator>(this, &itr);
 }
 inline SIterator(SIterator&& itr)  // Move constructor
 {
  //NMOPS::CopyObj<SIterator>(this, &itr);
  //NMOPS::ZeroObj(&itr);
  this->CopyFrom(&itr);
  itr.Reset();
 }
 //------------------------------------------------------
 inline SIterator(SFHdr* ctx)       // NULL iterator constructor
 {
  //this->BIdx = this->EIdx = -1;    // reset to END
  //this->EPtr = (Ty*)-1;
  //this->CPtr = this->BPtr = nullptr;
  this->Reset();
  this->Ctx = ctx;
 }
 //------------------------------------------------------
 inline SIterator(vptr BlkPtr, size_t BlkIdx, size_t ElmIdx, size_t ElmIdxInBlk, SFHdr* ctx)
 {
  size_t BSize = (size_t)Strat::BlkIdxToSize(BlkIdx);     // NOTE: May trim the size on x32, no checks
  size_t HSize = Strat::BlkIdxToHdrLen(BlkIdx);
  this->EIdx = ElmIdx;   // First element of the block
  this->BIdx = BlkIdx;
  this->BPtr = (Ty*)((size_t)BlkPtr + HSize);   // Expected to be aligned to 'Ty'
  this->EPtr = this->BPtr + ((BSize - HSize) / Strat::UnitSize);   // NOTE: Must be in whole units (Points either to leftover bytes or beyond the current block)
  this->CPtr = this->BPtr + ElmIdxInBlk;
  this->Ctx  = ctx;
 }
 //------------------------------------------------------
 inline Ty* At(size_t ElmIdx)
 {
  if((ElmIdx >= this->EIdx)&&(ElmIdx < (this->EIdx + (this->EPtr - this->BPtr))))this->CPtr = this->BPtr + (ElmIdx - this->EIdx);   // NOTE: '(this->EPtr - this->BPtr)' may use DIV and '(ElmIdx - this->EIdx)' may be used with MUL
    else this->SetBlockFor(ElmIdx);
  return this->CPtr;
 }
 //------------------------------------------------------
 inline size_t Index(void) const
 {
  return this->EIdx + (this->CPtr - this->BPtr);   // NOTE: May be used DIV.  // Not frequently needed  // Alternative is to ise indexes instead of pointers and to have MUL in retrieving the item's pointer (operator*)
 }
 //------------------------------------------------------
 inline void operator= (const SIterator& itr)
 {
  NMOPS::CopyObj(this, itr);
 }
 //------------------------------------------------------
 inline void operator= (size_t ElmIdx)    // Set the position in objects      // Fast if in boundaries of the current block
 {
  this->At(ElmIdx);
 }
 //------------------------------------------------------
 inline void operator+= (size_t Num)
 {
  //if(!this->CPtr)return;
  Ty* Ptr = this->CPtr + Num;
  if(Ptr >= this->EPtr)this->At(this->Index() + Num);
    else this->CPtr = Ptr;
 }
 //------------------------------------------------------
 inline void operator-= (size_t Num)
 {
  //if(!this->CPtr)return;
  Ty* Ptr = this->CPtr - Num;
  if(this->CPtr < this->BPtr)this->At(this->Index() - Num);
    else this->CPtr = Ptr;
 }
 //------------------------------------------------------
 inline operator size_t(void) const             // Get the position in objects
 {
  return this->Index();  //this->EIdx + (this->CPtr - this->BPtr);
 }
 //------------------------------------------------------
 // Prefix
 inline SIterator& operator++(void)
 {
  this->CPtr++;  // += (bool)this->CPtr;   // Do nothing if the iterator is NULL   // NOTE: Doing '(bool)this->CPtr;' is not so cheap  // No any CMP or MUL with just '++' (Adds a constant)
  if(this->CPtr >= this->EPtr)this->NextBlock();
  return *this;
 }
 //------------------------------------------------------
 inline SIterator& operator--(void)
 {
  this->CPtr--;  // -= (bool)this->CPtr;   // Do nothing if the iterator is NULL
  if(this->CPtr < this->BPtr)this->PrevBlock();
  return *this;
 }
 //------------------------------------------------------
 // Postfix
 inline SIterator operator++(int) { SIterator tmp = *this; ++(*this); return tmp; }     // NOTE: copying the iterator is not cheap - avoid postfix increment/decrement
 inline SIterator operator--(int) { SIterator tmp = *this; --(*this); return tmp; }     // Not used by ranged FOR (Clumsy, i don`t like them anyway)

 inline Ty& operator*(void) const { return *this->CPtr; }    // Access by a pointer - the cheapest operation
 inline Ty* operator->(void) { return this->CPtr; }

 inline operator bool() const { return (bool)this->CPtr; }      // to check if this iterator is NULL    // More efficient when iterating not in ranged 'for' loop than comparing equality

 friend inline bool operator== (const SIterator& a, const SIterator& b) { return !((a.BIdx ^ b.BIdx)|((size_t)a.CPtr ^ (size_t)b.CPtr)); }  // Do not compare pointers, +1 pointer may be in a next block with a random element index
 friend inline bool operator!= (const SIterator& a, const SIterator& b) { return ((a.BIdx ^ b.BIdx)|((size_t)a.CPtr ^ (size_t)b.CPtr)); }   // Can it be done simpler?  // Just replace with '(bool)a->CPtr' to work efficiently in ranged 'for' loops?
};
//-------------------------------------------------------------
SIterator ElmFrom(size_t ElmIdx)    // Returns iterator for the ElmIdx if it exist    // Will not add new blocks
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return SIterator(ctx);}
 if(!ctx->GetArrLen())return SIterator(ctx);  // Empty
 size_t BEIdx;
 size_t BlkIdx  = Strat::CalcForIndex(ElmIdx, BEIdx);
 size_t ElmOffs = BEIdx * Strat::UnitSize;
 vptr ptr = ctx->GetBlkPtr(BlkIdx);
 if(ptr)return SIterator(ptr, BlkIdx, ElmIdx - BEIdx, ElmOffs, ctx);
 return SIterator(ctx);  // Empty
}
//-------------------------------------------------------------
// Iterates in allocated ranges (Not aware if any valid data is present there or not)   // Unallocated blocks are skipped
SIterator ElmNext(size_t ElmIdx)    // Returns iterator for closest allocated ElmIdx
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return SIterator(ctx);}
 size_t BEIdx;
 size_t BlkIdx  = Strat::CalcForIndex(++ElmIdx, BEIdx);
 size_t ElmOffs = BEIdx * Strat::UnitSize;
 for(uint idx=BlkIdx,tot=ctx->GetArrLen();idx < tot;idx++)
  {
   vptr ptr = ctx->GetBlkPtr(BlkIdx);
   if(ptr)return SIterator(ptr, BlkIdx, ElmIdx - BEIdx, ElmOffs, ctx);
   ElmOffs  = 0;   // Any of next blocks will start from 0
  }
 return SIterator(ctx);  // Empty
}
//-------------------------------------------------------------
SIterator ElmLast(void)   // Backward only from here      // Cannot know which UnitIndex this is  // NOTE: No tracking of MaxUnits (The Blocks array is sparse)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return SIterator(ctx);}
 for(sint idx=ctx->GetArrLen()-1;idx >= 0;idx--)
  {
   vptr ptr = ctx->GetBlkPtr(idx);
   if(ptr)return SIterator(ptr, idx, Strat::BlkIdxToObjIdx(idx), Strat::BlkIdxToObjNum(idx) - 1, ctx);   // TODO: Calc offset for last index
  }
 return SIterator(ctx);
}
//-------------------------------------------------------------
inline SIterator ElmFirst(void)   // Forward only from here
{
 return this->ElmFrom(0);
}
//-------------------------------------------------------------
inline SIterator begin(void)      // NOTE: No way to avoid iteration over deleted objects if an ownerhave them!
{
 return this->ElmFirst();
}
//-------------------------------------------------------------
inline SIterator end(void)        // Iterates up until last non NULL block (NULL blocks are skipped)
{
 return SIterator(this->GetCtx());     // NULL iterator (There is no possible valid comparable END pointer for blocks)
}
//-------------------------------------------------------------
};
//============================================================================================================
