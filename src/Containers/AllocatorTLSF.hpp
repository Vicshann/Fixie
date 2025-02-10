
// For a limited MaxSize item  // Alignment is UnitSize  // Free lists is managed by a separate stream
// Same blockchain for items, created ititially to fit MaxItemSize 
//============================================================================================================
/*
   NOTE: This allocator is not intended to make bad designs easier to implement. Please do not allocate 1,2,3,4 bytes with it!
   NOTE: No per allocation metadata. You must pass the SAME size for 'free' that was passed for 'malloc'

   UnitSize: Default is size_t. Specifies size of unit to which allocations are aligned and to which requested sizes are aligned. 
             TLSF ranges consist of groups of such units. But splitting a requested range across several blocks is not allowed. 

First level | Second level
(+4+1)      |   0       1       2       3       4       5       6       7
--------------------------------------------------------------------------
0 |    32   |   0       4       8      12      16      20      24      28
1 |    64   |  32      36      40      44      48      52      56      60
2 |   128   |  64      72      80      88      96     104     112     120
3 |   256   | 128     144     160     176     192     208     224     240
4 |   512   | 256     288     320     352     384     416     448     480
5 |  1024   | 512     576     640     704     768     832     896     960       // Not good - malloc is usually used for blocks >2k

 Block bins start from 64K:
   65536
   131072
   262144
   524288
   1048576  0x00100000 1mb

        65536  // For <= 65536

       131072  // > 65536
       524288  // > 131072
      2097152  // > 524288
      8388608  // > 2097152
     33554432  // > 8388608
    134217728  // > 33554432
    536870912  // > 134217728
   2147483648  // > 536870912


  In units:  (Max 8 levels deep iteration)   // Shift right by 4 until zero and try to acceess the group as 0-15 by UnitIdx to alloc from it
  1    17
  2    18
  3    19
  4    20
  5    21
  6    22
  7    23
  8    24
  9    25
 10    26
 11    27
 12    28
 13    29
 14    30
 15    31
 16    32

Alloc/Free requests use SIZE to navigate the tree:

*/
// Supports items deletion
//
class SBinAlloc     // AllocatorBin
{
 using TUnit = uint32;

SCVR size_t UnitSize = 4;   // Min is 4 (Pow2 only, ensure that)
 // Size >> Pow2(UnitSize)          // High bit of uint32 index specifies if this is index of actual memory stream for the objects. if 0 then this is stream, if 1 then another index
struct SVvlNode    // 8 bytes
{
 uint32 UnitIdx;        // Index of a next free record  (The record may be not allocated yet)
 uint32 BinIdx  : 4;    // Indxes of stream for this UnitIdx        // NOTE: Must be high bits
 uint32 NextLvl : 28;   // Index of a record for next size branch   // NOTE: Must be low bits
};
struct SLvlBranch
{
 SVvlNode Nodes[16];  // Indexes of associated block desc (-1 is NONE)  ReqSize: (1 << (0-15))     // Each ingex group is (16 x uint32)
};
struct SFreeHdr   // Min unit size is 8 bytes
{
 uint32 Next;  // In units
 uint32 Size;  // In units // This is needed because we cannot have a bin for easch multiple of units
};

struct SSizeInf
{
 size_t Allocated;
};   

struct SCtx      // Takes ~4K       // TODO: Use flag to decide if it is separate                   // 4 high bits of index id the bin index                        
{
 NALC::SBlkAlloc<NPTM::MEMPAGESIZE, NALC::afGrowUni, SLvlBranch, SEmptyType, SEmptyType, SMemPrvBase, DefMaxAlign, 8> SizeTree;   // 13 blocks should be ~4K for all 16 bins  // TODO: Check that it is not too mach pages are allocated   // TODO: Check that afGrowUni on Windows uses reserved memory to allocate a next page or just use MEMGRANSIZE (4k allocs will leave holes in address space)

 NALC::SBlkAlloc<0x00010000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin0;   // 13 blocks should be ~4K for all 16 bins     // TODO: Check that afGrowUni on Windows uses reserved memory to allocate a next page or just use MEMGRANSIZE (4k allocs will leave holes in address space)
 NALC::SBlkAlloc<0x00020000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin1;
 NALC::SBlkAlloc<0x00040000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin2;
 NALC::SBlkAlloc<0x00080000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin3;
 NALC::SBlkAlloc<0x00100000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin4;
 NALC::SBlkAlloc<0x00200000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin5;
 NALC::SBlkAlloc<0x00400000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin6;
 NALC::SBlkAlloc<0x00800000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin7;
 NALC::SBlkAlloc<0x01000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin8;
 NALC::SBlkAlloc<0x02000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin9;
 NALC::SBlkAlloc<0x04000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin10;
 NALC::SBlkAlloc<0x08000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin11;
 NALC::SBlkAlloc<0x10000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin12;
 NALC::SBlkAlloc<0x20000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin13;
 NALC::SBlkAlloc<0x40000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin14;
 NALC::SBlkAlloc<0x80000000, NALC::afBlkCtxInIdx|NALC::afGrowUni, TUnit, SSizeInf, SSizeInf, SMemPrvBase, DefMaxAlign, 12> Bin15;
};
static_assert(sizeof(SCtx) <= NPTM::MEMPAGESIZE);


TSW<(CtxInFBlk), SFHdr*, SFHdr >::T Context;
// uint32 EndIndex; // An index that is beyond allocated blocks
 uint32 ElmCount;
//--------------------------------------------------------
static size_t SizeBytesToUnits(size_t Size)  // Size in bytes to size in units    // Aligns to unit size
{
 return AlignP2Frwd(Size, UnitSize) / UnitSize;  // TODO: Make sure that shift is used instead DIV
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