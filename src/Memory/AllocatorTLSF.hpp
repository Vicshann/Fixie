

// First  Level (FL): Powers of 2 (exponential): [16-31], [32-63], [64-127], [128-255], [256-511], etc.
// Second Level (SL): Linear subdivisions within each FL range (Each first-level range is divided into (1 << SL_INDEX_COUNT_LOG2) equal subdivisions)
//     More  subdivisions (higher SL): Better fit (less fragmentation), but more memory for bitmaps and free list arrays
//     Fewer subdivisions (lower  SL): Less overhead, but more internal fragmentation
//============================================================================================================
template<int AlignAB=16, typename MP=SMemProvBase> class CTLSF
{
 SCVR usize AlignLOG2   = BitWidth(BitCeil(AlignAB < 16 ? 16 : AlignAB));  // 4;   // Align to 16 (to keep allocations SIMD compatible)          // AlignSizeLOG2
 SCVR usize BlkAlign    = usize(1) << AlignLOG2;               // All allocations are in 16-byte units (aligned to 16)
 SCVR usize FL_IdxMax   = 30 + (2 * int(sizeof(void*) > 4));    // Max 1Gb for x32 and 4GB for X64             
 SCVR usize MinBlkLen   = sizeof(vptr)*4;   // 16/32 bytes is min per block.  // Must contain SBlkHdr which is 'sizeof(vptr)*3'
 SCVR usize MaxBlkLen   = usize(1) << FL_IdxMax;
 SCVR usize MinHdrLen   = sizeof(uint32);   // SizeAndFlg
 SCVR usize SL_IdxCntLOG2 = 5;   // 5 = 32 subdivisions per FL range (Fits into uint32)   // log2 of number of linear subdivisions of block sizes values of 4-5 typical, so there will be 2^5 or 32 subdivisions.  
 SCVR usize SL_IdxCount = (usize(1) << SL_IdxCntLOG2);
 SCVR usize FL_IdxShift = (SL_IdxCntLOG2 + AlignLOG2);
 SCVR usize FL_IdxCount = (FL_IdxMax - FL_IdxShift + 1);
 SCVR usize SmallBlkLen = (usize(1) << FL_IdxShift);     // Blocks less than that will use fast indexing path (FL only)   // Below 512 bytes: You get 32 size classes (512 / 16 = 32), perfectly filling one FL row  //  At 512 bytes and above: You switch to exponential (FL) + linear subdivision (SL)

 using BM = uint32;   // Bitmap type

 enum EHdrFlags: usize
 {
  flFree     = 1,   // This block is free
  flPrvFree  = 2,   // Previous block is free(It is safe to read its footer)
  flRgnLast  = 4,   // This block is last  in the memory region
  flRgnFirst = 8,   // This block is first in the memory region  // This flag always stays in first block (SRgnHdr.Blk.SizeAndFlg)
  flSizeMsk  = usize(~0x7)
 };

 struct SRgnHdr;
 
 struct SRgnHdrBase                          
 {
  SRgnHdr* Prev;
  SRgnHdr* Next;
  vptr     This;      // TLSF instance ptr
  usize    Size;
  usize    Data;      // Unused, need for alignment // Some user data? 
 };

 // This header is at the end of a block and describes next block
 struct SBlkHdr        // Must fit into MinBlkLen
 {
  uint32 PrevSize;     // Valid only if flags have flPrvFree
  uint32 SizeAndFlg;   // Includes flags in four low bits     // The size is aligned and includes space for SBlkHdr of a next block   // Max possible is 8589934576

  usize GetSize(void) const {return usize(this->SizeAndFlg & flSizeMsk) + BlkAlign;}
  void  SetSize(usize len) {this->SizeAndFlg = uint32((len - BlkAlign) | (this->SizeAndFlg & ~flSizeMsk));}     // As one unit less to fit 4GB into 32 bits
  usize GetPrevSize(void) const {return this->PrevSize << AlignLOG2;}        // Valid only if flags have flPrvFree
  void  SetPrevSize(usize len) {this->PrevSize = len >> AlignLOG2;}          // Valid only if flags have flPrvFree   // Low bits are unused and we can pack larger than 2G size into uint32
  bool  IsEmptyRgn(void) const {return (this->SizeAndFlg & (flPrvFree|flRgnLast|flRgnFirst)) == (flPrvFree|flRgnLast|flRgnFirst);}    // This block is the only one in the region now and it is free
  vptr  GetDataPtr(void) {return (uint8*)this + sizeof(SBlkHdr);}   // Returns actual data pointer (after this header) when this header describes a free block
  SBlkHdr* GetNext(void) {return (SBlkHdr*)((uint8*)this + this->GetSize());}  // Getting at exactly (BlkSize-sizeof(SBlkHdr))  // NOTE: Only SizeAndFlg is guranteed to be valid
  SBlkHdr* GetPrev(void) {return (SBlkHdr*)((uint8*)this - this->GetPrevSize());}    // Valid only if flags have flPrvFree
  SRgnHdr* GetRgnFromFirstBlk(void) {return (SRgnHdr*)((uint8*)this - sizeof(SRgnHdrBase));}     // This is expected to be a block, marked with flRgnFirst
  static SBlkHdr* BlkPtrToHdr(vptr ptr) {return ((uint8*)ptr) - sizeof(SBlkHdr);}
 };

 struct SFreeHdr // Stored in the data space when a block is free
 {
  SFreeHdr* Next;   
  SFreeHdr* Prev;   

  SBlkHdr*  GetHdr(void) {return SBlkHdr::BlkPtrToHdr(this);}
  SBlkHdr*  GetNexHdr(void) {SBlkHdr* hdr = this->GetHdr(); return (uint8*)hdr + hdr->GetSize();}       // Invalid for a last block in a region 
 // static SFreeHdr* FromNext(SBlkHdr* NxtHdr) {return (uint8*)NxtHdr - (NxtHdr->PrevSize - sizeof(SBlkHdr));}   // PrevSize is expected to be valid (flPrvFree is set)
 };
 static_assert((sizeof(SBlkHdr)+sizeof(SFreeHdr)) <= MinBlkLen);

 struct SRgnHdr: SRgnHdrBase  
 { 
  SBlkHdr  Blk;       // This belongs to a first block

  bool IsUnmanaged(void) {return this->Data & 1;}    // This block is large and is not handled by TLSF bitmaps
  static SRgnHdr* RgnFromFirstHdr(SBlkHdr* hdr){ return (SBlkHdr*)((uint8*)hdr - sizeof(SRgnHdrBase)); }
 };
 static_assert((sizeof(SRgnHdr) % BlkAlign) == 0);

 SRgnHdr*  Pools;            // Linked list of memory pools
 SRgnHdr*  LstEmptyPool;      // Last region which was detected as empty. It will be released on next 'Free' request or after 'Alloc' if nothing was allocated in it.    
 SFreeHdr* FreeBlocks[FL_IdxCount][SL_IdxCount];  // Linked Lists of free blocks for each size class

 BM FL_Bitmap;                // First  level allocated free ranges are marked with 1
 BM SL_Bitmap[FL_IdxCount];   // Second level allocated free ranges are marked with 1

 MP* MemProv;    // Null in most cases and it is OK ('this' is usually not used)

//-----------------------------------------------------
// Map size to FL index (first level - power of 2 range)
static uint8 MapFL(usize size) { return (sizeof(usize) * 8 - 1) - clz<false>(size); }   // Or: return clz(1) - clz(size);    // tlsf_fls_sizet
//-----------------------------------------------------
// Map size to SL index (second level - linear subdivision)
static uint8 MapSL(usize size, uint8 fl) { return (size >> (fl - (SL_IdxCntLOG2 + AlignLOG2))) - (usize(1) << SL_IdxCntLOG2); }     // Orig: (size >> (fl - SL_IdxCntLOG2)) ^ (1 << SL_IdxCntLOG2);
//-----------------------------------------------------
// Map size to both indices
static void MapForInsert(usize size, uint8& fl, uint8& sl) 
{
 if (size < SmallBlkLen) {
     fl = 0;                  // Put everything in
     sl = size >> AlignLOG2;  // Direct mapping: 16->1, 32->2, 48->3, etc.   // Orig: size / (SmallBlkLen / SL_IdxCount)
 } else {                     // TLSF switches from direct indexing to two-level segregated indexing
     uint8 ValFL = MapFL(size);
     sl = MapSL(size, ValFL);    
     fl = ValFL - (FL_IdxShift - 1);
 }
}
//-----------------------------------------------------
// Similar for search (allocations). May round up to the next block size
static void MapForSearch(usize size, uint8& fl, uint8& sl)
{
 if (size < SmallBlkLen) {
     fl = 0;                  // Put everything in
     sl = size >> AlignLOG2;  // Direct mapping: 16->1, 32->2, 48->3, etc.   // Orig: size / (SmallBlkLen / SL_IdxCount)
 } else {                     // TLSF switches from direct indexing to two-level segregated indexing
     size += ((1 << (MapFL(size) - SL_IdxCntLOG2)) - 1);  // Must round the size for allocations  
     uint8 ValFL = MapFL(size);                           // Doing 'MapFL' again. Is this the optimal way?
     sl = MapSL(size, ValFL);    
     fl = ValFL - (FL_IdxShift - 1);
 }
}
//-----------------------------------------------------
// Find first available block >= requested size
bool FindSuitableBlock(uint8& fl, uint8& sl) 
{   
 BM sl_map = this->SL_Bitmap[fl] & (BM(-1) << sl);  // Search in current SL row first
 if(!sl_map)   // Search next FL level
  {    
   BM fl_map = this->FL_Bitmap & (BM(-1) << (fl + 1));
   if(!fl_map)return false;
   fl = ctz<false>(fl_map);
   sl_map = this->SL_Bitmap[fl];    // Nonzero, some range is present
  }
 sl = ctz<false>(sl_map);
 return true;
}
//-----------------------------------------------------
void MarkRange(uint8 fl, uint8 sl) 
{
 this->FL_Bitmap     |= (BM(1) << fl);
 this->SL_Bitmap[fl] |= (BM(1) << sl);
}
//-----------------------------------------------------
void UnmarkRange(uint8 fl, uint8 sl) 
{
 BM ValSL = this->SL_Bitmap[fl] & ~(BM(1) << sl);
 if(!ValSL) this->FL_Bitmap &= ~(BM(1) << fl);   // Unmark FL range if all SL ranges for it is empty
 this->SL_Bitmap[fl] = ValSL;
}
//-----------------------------------------------------  
void InsertFreeBlock(SBlkHdr* blk, uint8 fl, uint8 sl) 
{
 SFreeHdr* head = this->FreeBlocks[fl][sl];
 SFreeHdr* fblk = (SFreeHdr*)blk->GetDataPtr();
 fblk->Next = head;
 fblk->Prev = nullptr;
 if(head) head->Prev = fblk;
 this->FreeBlocks[fl][sl] = fblk;  // Place as a new head
 this->MarkRange(fl, sl);          // This free block size range is available for allocation now
 blk->SizeAndFlg |= flFree; 
}
//-----------------------------------------------------
// Remove the block from linked list of free blocks
SBlkHdr* RemoveFreeBlock(SFreeHdr* blk, uint8 fl, uint8 sl) 
{
 SFreeHdr* prev = blk->Prev;
 SFreeHdr* next = blk->Next;
 if(next) next->Prev = blk->Prev;
 if(prev) prev->Next = blk->Next;
 else {
   this->FreeBlocks[fl][sl] = next;     // This block is the head - replace with its 'next'
   if(!next)UnmarkRange(fl, sl);        // This block is NULL, remove the size class chain from available
 }
 SBlkHdr* hdr = blk->GetHdr();
 hdr->SizeAndFlg &= ~flFree;
 return hdr; 
}
//-----------------------------------------------------
// Should use ForBlkSize, fl, and sl for best pooll size heuristics
SRgnHdr* AllocatePool(usize ForBlkSize, uint8 fl, uint8 sl)
{
 usize MinLen = ForBlkSize + sizeof(SRgnHdr);
 if(MinLen > MaxBlkLen)return nullptr;      // Too big block 



 return nullptr;  // TODO
}
//-----------------------------------------------------
// Returns next region
SRgnHdr* ReleasePool(SRgnHdr* Rgn)
{
 SRgnHdr* Next = Rgn->Next;
 if(Rgn->Prev)Rgn->Prev->Next = Rgn->Next;  
   else this->Pools = Rgn->Next;   // This region was Head
 if(Next)Next->Prev = Rgn->Prev;
 this->MemProv->Free(Rgn, Rgn->Size);  
 return Next; 
}
//-----------------------------------------------------
SBlkHdr* MergeWithPrev(SBlkHdr* blk)
{
 usize prevBlkSize  = blk->GetPrevSize(); 
 SBlkHdr*  prevBlk  = (SBlkHdr*)((uint8*)blk - blk->GetPrevSize());   // Navigate to previous block using stored PrevSize
 SFreeHdr* prevFree = (SFreeHdr*)prevBlk->GetDataPtr();     // Get the free header to remove from list
 
 // Remove previous block from free list
 uint8 fl, sl;
 MapForInsert(prevBlkSize, fl, sl);
 this->RemoveFreeBlock(prevFree, fl, sl);
 
 usize newSize = prevBlkSize + blk->GetSize();    // Merge sizes
 prevBlk->SetSize(newSize);
 
 if(blk->SizeAndFlg & flRgnLast)prevBlk->SizeAndFlg |= flRgnLast;     // Preserve current block's region flags
  else        // Update next block's PrevSize
   {
    SBlkHdr* nextBlk     = prevBlk->GetNext();      // Should reach a block after 'blk' if size calculation isn't broken
    nextBlk->PrevSize    = newSize - BlkAlign;
    nextBlk->SizeAndFlg |= flPrvFree;  // Previous is now free
   }
 //if(blk->SizeAndFlg & flRgnFirst)prevBlk->SizeAndFlg |= flRgnFirst;  // Should never happen 
 //prevBlk->SizeAndFlg |= flFree;    // Preserve free flag from prevBlk (Should never happen - it was already free)
 return prevBlk;
}
//-----------------------------------------------------
// NOTE: This function is supplementary to 'Free' 
void MergeWithNext(SBlkHdr* blk, SBlkHdr* nextBlk)
{
 usize     nextSize = nextBlk->GetSize();
 SFreeHdr* nextFree = (SFreeHdr*)nextBlk->GetDataPtr();    // Get the free header to remove from list
 
 // Remove next block from free list
 uint8 fl, sl;
 MapForInsert(nextSize, fl, sl);
 this->RemoveFreeBlock(nextFree, fl, sl);
 
 usize newSize = blk->GetSize() + nextSize;    // Merge sizes
 blk->SetSize(newSize);
 
 // Preserve next block's flRgnLast flag if present
 if(nextBlk->SizeAndFlg & flRgnLast) blk->SizeAndFlg |= flRgnLast;
   else           // Update the block after nextBlk (if exists)
    {
     SBlkHdr*  afterNext = blk->GetNext();     // Should reach a block after 'blk' if size calculation isn't broken
     afterNext->SetPrevSize(newSize);
     afterNext->SizeAndFlg |= flPrvFree;
    }
}
//-----------------------------------------------------
// NOTE: This function is supplementary to 'Alloc' at specific call site (depends on the state)
void SplitBlock(SBlkHdr* blk, usize UsedLen, usize FullLen)
{
 usize RemainLen = FullLen - UsedLen;
 blk->SetSize(UsedLen);    // Update current block's size    // UsedLen should already include size of SizeAndFlg for next block
 
 // Create new free block from the remainder
 SBlkHdr* newBlk = blk->GetNext();   // Do this after SetSize   // (SBlkHdr*)((uint8*)blk + UsedLen);
 newBlk->SizeAndFlg = flFree;  
 newBlk->SetSize(RemainLen);
 //newBlk->PrevSize = 0;   // Prev block is used so this field is just invalid
 
 if(blk->SizeAndFlg & flRgnLast)  // This new free block may become last in the pool region
  {
   newBlk->SizeAndFlg |= flRgnLast;     // New block is now last
   blk->SizeAndFlg    &= ~flRgnLast;    // Current block is no longer last
  }
 else   // Update the block after newBlk 
  {
   SBlkHdr* nextBlk = newBlk->GetNext();
   nextBlk->SizeAndFlg |= flPrvFree;
   nextBlk->SetPrevSize(RemainLen); 
  }
 // Insert the new free block into free lists
 uint8 fl, sl;
 MapForInsert(RemainLen, fl, sl);
 this->InsertFreeBlock(newBlk, fl, sl);
}
//-----------------------------------------------------
_finline bool FreePendingPool(void)
{
 if(!this->LstEmptyPool)return false;
 if(LstEmptyPool->Blk.IsEmptyRgn())
  {
   this->ReleasePool(this->LstEmptyPool);   // Have a pending empty region to free
   this->LstEmptyPool = nullptr;
  }
 return false;
}
//-----------------------------------------------------

public:
_finline CTLSF(MP* mp=(MP*)nullptr){this->Init(mp);}   // Not all memory providers use contexts so nullptr as default is OK
_finline ~CTLSF(){this->Release();}
//-----------------------------------------------------
inline void Init(MP* mp=(MP*)nullptr)
{
 MOPR::ZeroObj(this);
 this->MemProv = mp;
}
//-----------------------------------------------------
void Release(void)
{
 for(SRgnHdr* Rgn=this->Pools;Rgn;)    // Rgn = this->ReleasePool(Rgn);  // Release allocated memory regions   // Disabled: No need to maintain the list
  {
   SRgnHdr* Cur = Rgn;
   Rgn = Rgn->Next;
   this->MemProv->Free(Cur, Cur->Size);  
  }
 this->Pools = nullptr;
}
//-----------------------------------------------------
// Updates the size to actually allocated
vptr Alloc(usize* size) 
{
 usize BlkLen  = *size + MinHdrLen;           // Overhead is only 4 bytes, rest is the alignment to 16
 if(BlkLen < MinBlkLen)BlkLen = MinBlkLen;
   else BlkLen = AlignFrwdP2(BlkLen, BlkAlign);
 if(BlkLen > MaxBlkLen) return nullptr;
 
 uint8 fl, sl;
 MapForSearch(BlkLen, fl, sl);
 while(!FindSuitableBlock(fl, sl))         // Expected to loop only once
  { 
   if(!this->AllocatePool(BlkLen, fl, sl)) return nullptr;     // No suitable block, expand memory
  }
 
 SFreeHdr* fblk = this->FreeBlocks[fl][sl];
 SBlkHdr*  blk  = RemoveFreeBlock(fblk, fl, sl);               
 if(usize FullLen=blk->GetSize();FullLen < (BlkLen + MinBlkLen))  // Update the next block's flags
  {
   if(!(blk->SizeAndFlg & flRgnLast))blk->GetNext()->SizeAndFlg &= ~flPrvFree;   // Update next block's flPrvFree
  }
   else this->SplitBlock(blk, BlkLen, FullLen);    // Split if block too large and return its tail to the Free list (Turns the blocks tail into a new block)
      
 this->FreePendingPool();    

 *size = BlkLen - MinHdrLen;      // SizeAndFlg of a next block must be preserved
 return blk->GetDataPtr();
}
//-----------------------------------------------------
void Free(vptr ptr) 
{
 this->FreePendingPool(); 

 SBlkHdr* blk = SBlkHdr::BlkPtrToHdr(ptr);
 blk->SizeAndFlg |= flFree;
 
 if(blk->IsEmptyRgn())     // Probably a large block, not handled by bitmaps
  {
   SRgnHdr* rgn = blk->GetRgnFromFirstBlk();
   if(rgn->IsUnmanaged())
    {
     this->ReleasePool(rgn);      // Immediately release large unmanaged allocations?
     return;
    }
  }
 
 if(blk->SizeAndFlg & flPrvFree)blk = this->MergeWithPrev(blk);   // Coalesce with previous if free
 if(!(blk->SizeAndFlg & flRgnLast))    // Coalesce with next if free
  {
   SBlkHdr* nxt = blk->GetNext();
   if(nxt->SizeAndFlg & flFree) this->MergeWithNext(blk, nxt);    // Should propagate first/Last flags
  }
 // Check if entire region is free
 if(blk->IsEmptyRgn())this->LstEmptyPool = blk->GetRgnFromFirstBlk();     // Queue the region for release but still put it into free list to give a chance to next call to Alloc to reclaim it

 // Insert into appropriate free list
 uint8 fl, sl;
 MapForInsert(blk->GetSize(), fl, sl);
 this->InsertFreeBlock(blk, fl, sl);   
}
//-----------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
