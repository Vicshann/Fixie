
// For a limited MaxSize item  // Alignment is UnitSize  // Free lists is managed by a separate stream
// Same blockchain for items, created ititially to fit MaxItemSize 
// Iterator can iterate only in From-To and you must know this range. 
// The allocator can't know if two different requests of same size is for group of 8byte items and a group of twice less 16byte items.
// Requested allocations are always contiguous in memory - no splitting between blocks
// This allocator is for allocation of items of different size. Internally, indexind is done by UnitSize which is 4 by default.
// TODO: Use strategy type to determine allocation scheme. All blocks is PageLen and no larger items can be allocated or use allocation of expanded blocks and PageLen is a minimal size.
//  May fragment memory more or overallocate if PageLen is not of a fixed size. (Because if a block of appropriate size is full then we can get only bigger one)
// 
// afSequential prevents splitting of larger blocks and allows to keep same sized allocations relatively sequential but leads to less deleted memory being reused. (splitting is a little bit slower and may grow index more)
//============================================================================================================
// Supports item deletion
//          // 0x00010000
template<size_t PageLen, uint32 Flg, typename Ty=uint32, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemPrvBase, size_t MaxAlign=DefMaxAlign, size_t LBlkMin=0> class SBktAlloc    
{
SCVR bool   EmptyMP   = sizeof(MP) <= 1;    // MP will have 1 size without any data members    // The MP size should be either 0 or > 1 
SCVR bool   EmptyTB   = sizeof(TBIfo) <= 1;     // Should be external?
SCVR bool   EmptyTC   = sizeof(TCIfo) <= 1;     // Should be external?

 using BlockSize = PageLen;  // TODO: Align pow2
 using TUnit = uint32;     // Min unit size is 4   // All requested allocations are rounded to the unit size
 SCVR bool CtxInFBlk  = (Flg & afSinglePtr); 
 SCVR size_t UnitSize = 4;   // Min is 4 (Pow2 only, ensure that)
 // Size >> Pow2(UnitSize)          // High bit of uint32 index specifies if this is index of actual memory stream for the objects. if 0 then this is stream, if 1 then another index
struct SVvlNode    // 8 bytes
{
 uint32 UnitIdx;   // Index of a next free record  (The record may be not allocated yet)
 uint32 NextLvl;   // Index of a record for next size branch  
};
struct SLvlBranch
{
 SVvlNode Nodes[16];  // Indexes of associated block desc (-1 is NONE)  ReqSize: (1 << (0-15))     // Each ingex group is (16 x uint32)
};
struct SFreeHdr   // Min unit size is 8 bytes
{
 uint32 Next;  // In units
};

struct SSizeInf
{
 size_t Allocated;    // Number of allocated units in the block. If it is 0 then the block can be removed to free its memory
};   

struct SCtx      // Takes ~4K       // TODO: Use flag to decide if it is separate                   // 4 high bits of index id the bin index                        
{
 NALC::SBlkAlloc<NPTM::MEMPAGESIZE, Flg|NALC::afGrowUni, SLvlBranch, SEmptyType, SEmptyType, SMemPrvBase, DefMaxAlign, 8> SizeTree;   // 13 blocks should be ~4K for all 16 bins  // TODO: Check that it is not too mach pages are allocated   // TODO: Check that afGrowUni on Windows uses reserved memory to allocate a next page or just use MEMGRANSIZE (4k allocs will leave holes in address space)
 NALC::SBlkAlloc<BlockSize, (Flg & ~afSinglePtr)|NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SEmptyType, TBIfo, MP, MaxAlign, LBlkMin> UnitStream;   
};
static_assert(!CtxInFBlk || (sizeof(SCtx) <= (BlockSize / 8)));

TSW<(CtxInFBlk), SCtx*, SCtx >::T Context;
// uint32 EndIndex; // An index that is beyond allocated blocks
 uint32 ElmCount;
//--------------------------------------------------------
static size_t SizeBytesToUnits(size_t Size)  // Size in bytes to size in units    // Aligns to unit size
{
 return AlignFrwdP2(Size, UnitSize) / UnitSize;  // TODO: Make sure that shift is used instead DIV
}
//--------------------------------------------------------
SVvlNode* SizeUnitsToNode(uint32 Units)     // Alignment is always same as the size?????????????     !!!! TODO: AllocatorBlk - Optionally store BlkCtx in index table
{
 auto iter = this->SizeTree.ElmFirst(); //  SLvlBranch* Branch = FirstBranch;
 for(;;)   // Max 8 iterations // Unroll?
  {
   uint8 idx = Units & 0x0F;
   Units >>= 4;
   if(!Units)return &iter->Nodes[idx];   // No deeper levels
   iter = size_t(iter->Nodes[idx].NextLvl);    // TODO: Check that it is &0x0FFFFFFF
  }
}
//--------------------------------------------------------
_finline void Init(MP* mp)
{
 NMOPS::ZeroObj(this); 
 if constexpr (!EmptyMP)
  {
   if constexpr (CtxInFBlk)this->Context = (SFHdr*)((size_t)mp | 1);     // In most cases MP is zero    // NOTE: This will make the GetCtx to return an invalid context pointer until first call to AllocBlock
     else this->Context.MProv = mp;
  }
}
//------------------------------------------------------------- 
public:
inline SBinAlloc(MP* mp=(MP*)nullptr){this->Init(mp);}   // Not all memory providers use contexts so nullptr as default is OK
inline ~SBinAlloc(){this->Release();}
//--------------------------------------------------------
// Alignment is rounded to nearest pow2
// It is possible to allocate smaller objects at bigger alignment, like 4 bytes at alignment 16
// NOTE: it will try to not waste the alignment bytes but alignments greater than UnitSize will require the allocation to be made from a bin ... ????????
//
vptr Alloc(size_t size, size_t* IdxOut)   
{

}
//--------------------------------------------------------
void Free(size_t Idx, size_t size)     // NOTE: The size must be same as the one that was passed to Alloc or everything will break!
{

}
//--------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
/*
static constexpr int SL_INDEX_COUNT = (1 << SL_INDEX_COUNT_LOG2);
static constexpr int FL_INDEX_SHIFT = (SL_INDEX_COUNT_LOG2 + ALIGN_SIZE_LOG2);
static constexpr int FL_INDEX_COUNT = (FL_INDEX_MAX - FL_INDEX_SHIFT + 1);
static constexpr int SMALL_BLOCK_SIZE = (1 << FL_INDEX_SHIFT);

static constexpr std::size_t BLOCK_SIZE_MAX = static_cast<std::size_t>(1) << FL_INDEX_MAX;

 SCVR int FL_INDEX_COUNT;
 SCVR int SL_INDEX_COUNT;
/// 
 // Block sizes are always a multiple of 4. 
 // The two least significant bits of the size field are used to store the block status
 //  bit 0: whether the block is busy or free
 //  bit 1: whether the previous block is busy or free
 // 
static constexpr std::size_t BLOCK_HEADER_FREE_BIT = 1 << 0;
static constexpr std::size_t BLOCK_HEADER_PREV_FREE_BIT = 1 << 1;


//  @brief The only overhead exposed during usage is the size field. The previous_phys_block field is technically stored 
//  inside the previous block.

static constexpr std::size_t BLOCK_HEADER_OVERHEAD = sizeof(std::size_t);

#ifdef TLSF_64BIT
// all allocation sizes are aligned to 8 bytes
static constexpr int ALIGN_SIZE_LOG2 = 3;
static constexpr int FL_INDEX_MAX = 32; //this means the largest block we can allocate is 2^32 bytes or about 2.1 GB
#else
// all allocation sizes are aligned to 4 bytes
static constexpr int ALIGN_SIZE_LOG2 = 2;
static constexpr int FL_INDEX_MAX = 30;
#endif

static constexpr int ALIGN_SIZE = (1 << ALIGN_SIZE_LOG2);

// log2 of number of linear subdivisions of block sizes
// values of 4-5 typical, so there will be 2^5 or 32 subdivisions.
static constexpr int SL_INDEX_COUNT_LOG2 = 5;

/**
 * Allocations of sizes up to (1 << FL_INDEX_MAX) are supported. Because we linearly subdivide the second-level lists 
 * and the minimum size block granularity is N bytes, it doesn't make sense to create first-level lists for sizes 
 * smaller than SL_INDEX_COUNT * N or (1 << (SL_INDEX_COUNT_LOG2 + log(N))) bytes, as we will be trying to split size
 * ranges into more slots than we have available. 
 * 
 * N is 4 or 8 bytes depending on whether the system is 32-bit or 64-bit, respectively.
 * 
 * We calculate the minimum threshold size, and place all blocks below that size into the 0th first-level list. 
 */

    /*
        unsigned int fl_bitmap;
        unsigned int sl_bitmap[detail::FL_INDEX_COUNT];

SBlkHdr* blocks[detail::FL_INDEX_COUNT][detail::SL_INDEX_COUNT];
 detail::block_header block_null;

*/
//------------------------------------------------------------------------------------------------------------