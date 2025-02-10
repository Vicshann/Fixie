

// Used instead of a HashSet to get unique string IDs (Any byte sequences in fact)
// Consumes ~8 times more memory than original data(Without tags)!  ( But it is sparse and very fast at finding matches )  (Have no limit on a word size and consumes much less memory than a char tree would do)
// NOTE: Any hashing algo would require bucketing if you don`t want to take chances with collisions.
// This is a flattened bit tree, no ability to remove records.
// TODO: Compare with:
//          https://www.nedprod.com/programs/portable/nedtries/
//          https://github.com/ned14/quickcpplib/blob/master/include/quickcpplib/algorithm/bitwise_trie.hpp
//          https://github.com/ned14/nedtries
//          https://judy.sourceforge.net/
// 
// TODO: Multi-threaded sliding matcher (on all branches) for exact/best match
// TODO: Callback to compare accumulated units at their boundaries (sliding matcher)  // Useful for fuzzy matching
// NOTE: Sliding matcher will not produce unique word end points (a small and big word which include small one as its end will have same endpoint)
//------------------------------------------------------------------------------------------------------------
// UnitLen: A tag will be stored per each UnitLen (Like per char if UnitLen is 8). Leave it Zero if no tags is needed.
//          Unit len is in bits, pow2: 0,2,4,8,16,32,64,...  // If not 0, uint32 tag is added to each unit.  // Use 8 to tag strings which is also stored elsewhere
// BlkLen:  Block len is from 0x10000: 65536, 131072, 262144, 524288, 1048576 and is used for the stream growth allocation. Should be not less than the system memory page size
// 
// BeBig:   Expecting the database become quite big (megabytes) so use allocation strategy that will use allow blocks to grow large and reduce memory allocation syscalls. 
//
template<uint BlkLen=0x10000, uint UnitLen=0, int MemMode=0> class CByteJam  // CFlatTrie?  // CBitJam? // Something like a Trie (Prefix Tree) for bits: https://en.wikipedia.org/wiki/Trie
{
enum EBFlags         // Distance can be 268435455 to 1073741820 bytes for a nonterminal bit
{    
 flInfBits  = 2,           // Number of info bits for each bit desc                               
 flOffsMsk  = uint32((uint32)-1 >> (flInfBits * 2)),  // 0x0FFFFFFF     // Mask out info bits (for both bits) in desc
 flFlagMsk  = uint32((uint32)-1 << ((sizeof(uint32)*8) - flInfBits)),   // 0xC0000000  Mask out info bits (single bit) in desc

 flWordEnd  = 0x80000000,  // This bit ends some word
 flHaveIdx  = 0x40000000,  // An offset field is present for this bit

 flExLenMsk = (flHaveIdx | (flHaveIdx >> 2))
};

struct SSerHdr
{
 uint32 Sign;     // BJAM
 uint32 ElmCnt;
};
//==================================================================================
struct SBitNode   // Bit node
{
 struct SBit
 {
  uint32 Offs;
  uint32 Flags;    // Includes offset bits
  uint8  Idx;
  uint8  Padding[7];  // Makes this struct 16 bytes so shift will be used for index access instead of mul
 
  bool IsTerm(void){return !this->Offs;}
  SBitNode* GetOwner(void){return (SBitNode*)(((SBit*)this) - this->Idx);}
 } Bit[2];
 uint32 MaxOffs;
 uint32 NBitPos;    // Next bit pos (for tagged units)
 uint32 UnitTag[2]; // Each bit path requires its own tag slot

 SBitNode(void){this->Bit[0].Idx = 0; this->Bit[1].Idx = 1;}
 uint8 GetSize(void)     // In positions: 1, 2 or 3 (for a tagged bit)
 {
  uint8  len  = 1;
  uint32 Desc = (this->Bit[0].Flags | (this->Bit[1].Flags >> flInfBits));
  len += ((Desc & flExLenMsk) == flExLenMsk);
  if constexpr (UnitLen)len += size_t((int)IsTaggedBit(this) << 1); // Using tagged units
  return len;
 }
};
using SBit = SBitNode::SBit;
//==================================================================================
struct SBits
{
 const uint8* Data;
 uint ByteLen;
 uint BitLen;
 uint BitOffs;
 const uint8* CurPtr;
 const uint8* EndPtr;

//----------------------------------------------------------------------------------
SBits(const uint8* Word, uint Len)
{
 this->Data    = Word;
 this->ByteLen = Len;
 this->BitLen  = Len * 8;
 this->EndPtr  = &this->Data[this->ByteLen];
 this->Reset();
}
//----------------------------------------------------------------------------------
uint GetBitPos(void){return this->BitOffs;}    // Of a next bit
uint GetBitLen(void){return this->BitLen;}
void Reset(void){this->CurPtr = Data; this->BitOffs = 0;}
//----------------------------------------------------------------------------------
bool Next(void)       // Bit order: BYTE0{D0-D7}, BYTE1{D0-D7}, ...
{
 sint8 val = (*this->CurPtr >> (this->BitOffs++ & 7)) & 1;
 if(!(this->BitOffs & 7))   
  {
   if(++this->CurPtr >= this->EndPtr)this->Reset();   // No more bits - wrap around
  }
 return val;
}
//----------------------------------------------------------------------------------
};
//==================================================================================
// afGrowUni should have fastest access by index, but does more memory alloc syscalls. 
// afGrowExp have a bit slower access by index, much less alloc syscalls but may waste more memory in last block if it is partially used.

 NALC::SBlkAlloc<BlkLen, ((MemMode == 2)?NALC::afGrowExp:((MemMode == 1)?NALC::afGrowLin:NALC::afGrowUni)), uint32> Stream;        // NOTE: BlkLen=0x100000,MemMode=0 is the best for large dictionaries
 uint32 ElmCount;

//------------------------------------------------------------------------------------------------------------
static _finline bool IsTaggedBit(SBitNode* bitn){return !(bitn->NBitPos & (UnitLen - 1)) && bitn->NBitPos;}  // UnitLen must be Pow2
//------------------------------------------------------------------------------------------------------------
static void ReadBitNode(auto Iter, SBitNode* bitn)  // Works on a copy of an iterator (the end index is useless) // Returns index of a next bit (if any)
{  
 uint32 Desc = (*Iter);  
 bool  ExLen = ((Desc & flExLenMsk) == flExLenMsk);  // For a terminal bit position (Both bits require big offsets)
 bitn->Bit[0].Flags = Desc;  // & flFlagMsk;     // Leaving offset bits in the flags - should not cause any problems (UpdateBit* will mask out any garbage anyway)
 bitn->Bit[1].Flags = (Desc << flInfBits);  // & flFlagMsk; 
 if(ExLen)  // Both bits have flHaveIdx set      // 30 bits for offset
  {
   uint32 OffsBit1   = (*++Iter); 
   uint32 OffsBit0   = (Desc & flOffsMsk) | ((OffsBit1 & flFlagMsk) >> 2);  // Two high bits of extra offset belong to bit`s 0 offset    // <<< NOTE: Must be adjusted if number of info bits changed
   bitn->Bit[1].Offs = OffsBit1 & ~flFlagMsk;   
   bitn->Bit[0].Offs = OffsBit0;                
   bitn->MaxOffs = ~flFlagMsk;  // 30 bits
  }
  else  // Only one bit requires offset, the other one is adjacent   // 28 bits for offset
   {
    bool BitIdx = (Desc & flHaveIdx);    // Check if the index belongs to bit 0
    bitn->Bit[!BitIdx].Offs = Desc & flOffsMsk;  // The index belong to bit 0 or bit 1
    bitn->MaxOffs = flOffsMsk;  // 28 bits
    if constexpr (UnitLen)bitn->Bit[BitIdx ].Offs = (IsTaggedBit(bitn))?3:1;             // Next bit is adjacent  // 3 = 1+(2 Tag Slots)
     else bitn->Bit[BitIdx ].Offs = 1;             // Next bit is adjacent
   }
 if constexpr (UnitLen)  // Using tagged units
  {
   if(IsTaggedBit(bitn)){ bitn->UnitTag[0] = (*++Iter); bitn->UnitTag[1] = (*++Iter); }   
     else bitn->UnitTag[1] = bitn->UnitTag[0] = 0;
  }     
}
//------------------------------------------------------------------------------------------------------------
// A terminal bit(i.e. last bit of a byte (high)) takes 8 bytes (ExLen) because both bit nodes require far offsets
// A tagged bit takes another 8 bytes (4 bytes for each bit path)
// TODO: For multithreading, should have a separate WriteIndex and advance it atomically
//
static void WriteBitNode(auto& Iter, SBitNode* bitn)   // Works on a ref of an iterator       // Use only to store new nodes   // TODO: Multithread safety    // flExOffs must be already set accordingly (Only terminal bits and not adjacent ones will have it)
{
 uint32 Desc = (bitn->Bit[0].Flags | (bitn->Bit[1].Flags >> flInfBits)); // & flOffsMsk; No need to mask out - the offset bits should be 0
 bool  ExLen = ((Desc & flExLenMsk) == flExLenMsk);
 if(ExLen)
  {
   uint32 OffsBit1 = bitn->Bit[1].Offs;  // & ~flFlagMsk;   // The offsets is initially 0 and trimming them will not save from data corruption anyway (random index is NOT always points to a bit record)
   uint32 OffsBit0 = bitn->Bit[0].Offs;  // & ~flFlagMsk;
   (*Iter)   = OffsBit0 | Desc;             // (OffsBit0 & flOffsMsk) | Desc;
   (*++Iter) = OffsBit1 | ((OffsBit0 << 2) & flFlagMsk);     // + 2 highs bits for bit 0 offset    // <<< NOTE: Must be adjusted if number of info bits changed
  }
   else (*Iter) = Desc | bitn->Bit[bool(!(Desc & flHaveIdx))].Offs;  // .Offs & flOffsMsk)  // Bit0 or bit1
// if constexpr (UnitLen)  // Using tagged units
//  {
//   if(IsTaggedBit(bitn)){ (*++Iter) = bitn->UnitTag[0]; (*++Iter) = bitn->UnitTag[1]; };    // Probably not needed to do here since it is not passed from anywhere and always set to 0 
//  }
// ++Iter;
 if constexpr (UnitLen)
  {
   if(IsTaggedBit(bitn))Iter += 3;
     else ++Iter;
  }
   else ++Iter; 
}
//------------------------------------------------------------------------------------------------------------
static void UpdateBitFlags(auto& Iter, SBit* bit)      // Use to update bits in nodes  // Used only to set flWordEnd 
{
 uint32 CfgOr = (bit->Flags & flFlagMsk);   
 CfgOr >>= (bit->Idx << 1);    // bit->Idx * 2   // Shift by 0 (bit is 0) or by 2 (bit is 1)   // <<< NOTE: Must be adjusted if number of info bits changed      
 (*Iter) |= CfgOr;          // TODO: Check thread safety (Interlocked OR ?)   
} 
//------------------------------------------------------------------------------------------------------------
static void UpdateBitOffset(auto Iter, SBit* bit)     // Original offset expected to be 0  
{
 SBitNode* bitn = (SBitNode*)(bit - bit->Idx);
 if((bitn->Bit[0].Flags & bitn->Bit[1].Flags) & flHaveIdx)   // Have extended offs fields (flHaveIdx is set for both bits)       
  {
   if(bit->Idx)(*++Iter) |= (bit->Offs & ~flFlagMsk);
    else {(*Iter) |= (bit->Offs & flOffsMsk); (*++Iter) |= ((bit->Offs << 2) & flFlagMsk);}      // <<< NOTE: Must be adjusted if number of info bits changed    // TODO: OR as UINT64 for thread safety
  }
  else (*Iter) |= (bit->Offs & flOffsMsk);    // Assuming that this is a terminal bit which owns the offs field 
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Does not advances the iterator from the last bit so it can be updated later
//
static sint8 MatchWordAt(auto& Iter, SBits& Word, SBitNode* Bitn, SBit** LastBit)    // NOTE: Do not call on an empty stream or an empty word!   // Returns: 1 if the word is longer than a path; -1 if the word is shorter or equal the path but no end marker; 0 if match 
{
 SBit*   bv = nullptr;      
 uint WBits = Word.BitLen;
 Word.Reset();       
 for(;;)      
  {
   sint8 val = Word.Next();
   if constexpr (UnitLen)Bitn->NBitPos = Word.GetBitPos();   // Required only for tagged units 
   ReadBitNode(Iter, Bitn);              // TODO: Avoid copying the iterator!!!!!!!!!!!!!!!!!!!
   bv = &Bitn->Bit[val];
   WBits--;           // Counting the bits is faster 
   if(!WBits)break;   // This bit is last in the word
   if(bv->IsTerm()){*LastBit = bv; return 1;}     // This bit is last on the path  // The word is longer than the path  
   Iter += size_t(bv->Offs); 
  }
 *LastBit = bv;
 if(bv->Flags & flWordEnd)return 0;  // Match - No more bits in the word and the end marker is present
 return -1;  // No end marker
} 
//------------------------------------------------------------------------------------------------------------
// NOTE: Terminal bit record index is same for both bits so we need to add the actual bit value (0 or 1) to discriminate them and use thes position as ID
// Pointing to AFTER the terminal bit allows access tags easier (No need to check any flags)
//
sint8 StoreWordAt(auto& Iter, SBits& Word, SBitNode* Bitn, SBit** LastBit)    // Just returns same offset if the word was already stored   
{
 SBit* bv = nullptr;
 if(this->ElmCount)
  {
   sint8 mv = this->MatchWordAt(Iter, Word, Bitn, &bv);     // The resulting iterator points to the terminal bit and can be used to retrieve its tag
   if(!mv){ *LastBit = bv; return 0; }      // Already present
   if((mv < 0) && bv)     // Just set the end marker
    {
     bv->Flags |= flWordEnd;
     this->UpdateBitFlags(Iter, bv);     // Match at the same position. Just mark the second bit as terminal
     *LastBit = bv;   // Last bit value to add to its array index (Which takes two positions since it is a terminal bit)
     return 1;           
    }
  }
// Continue storing the word at updated by MatchWordAt position (The word is longer than the path) 
 uint   WBits = Word.GetBitLen() - Word.GetBitPos();   // Number of bits left to store
 uint32 NewAt = this->ElmCount;     // Where we can write a new records
 if(bv)    // Last present bit is terminal and distance to a next bit must be set
  { 
   uint32 NextOffs = NewAt - Iter.Index();   // Relative to current bit record
   if(NextOffs > Bitn->MaxOffs){LOGERR("Offset is too big!"); return -2;} 
   bv->Offs = NextOffs;
   this->UpdateBitOffset(Iter, bv);    // Failed to write, the bit has been changed by another thread - must repeat StoreWordAt   // TODO: For multithreading, should do this only when all new bits are already written so that reading
  }                                                                                                                               //  Probably requires locking or some other thread may start adding same bits here // May be lock only this node while writing it
 uint WBEx = 1;       // +1 for terminal bit`s extended offset
 if constexpr (UnitLen)
  {
//   bitn.UnitTag[1] = bitn.UnitTag[0] = 0;
   WBEx += ((WBits / UnitLen) + bool(WBits % UnitLen)) << 1;  // Add number of tags required (2 slots per tag)
  }
 this->ElmCount += WBits + WBEx;
 this->Stream.Expand(this->ElmCount-1);  //ElmAdd(WBits+WBEx);   // TODO: Thread safety    // Iterators are not corrupted by this   // The actual block will contain more records

 Iter = size_t(NewAt);       // Reinit the iterator
//  LOGMSG("Max at: %08X (%u)",this->Stream.Count(),this->Stream.Count());
 while(WBits-- > 0)    // Store new bits
  {
   sint8 val = Word.Next();
          bv = &Bitn->Bit[val];
   SBit*  bz = &Bitn->Bit[!val];  // Not on the path
   bz->Flags = flHaveIdx;
   bv->Flags = (WBits)?(0):(flHaveIdx|flWordEnd);   // This bit position is terminal else 0        // NOTE: conditional move us used here even with no optimization
   bv->Offs  = bz->Offs = 0;     // No next bit or it is adjacent
   if constexpr (UnitLen)Bitn->NBitPos = Word.GetBitPos();   // Required only for tagged units (see IsTaggedBit)
   this->WriteBitNode(Iter, Bitn);   // NewAt =     // NOTE: Advances the iterator beyond the last bit
  }
 Iter -= size_t(Bitn->GetSize()); // Better to move the iterator back here than doing anything inside the loop above
 *LastBit = bv;
//  LOGMSG("End at: %08X (%u)",NewAt,NewAt);
 return 2;
}
//------------------------------------------------------------------------------------------------------------
public:
CByteJam(void): ElmCount(0) {}
//------------------------------------------------------------------------------------------------------------
void   Clear(void){this->ElmCount = 0; this->Stream.Release();}
bool   Duplicate(CByteJam& jam){jam.Clear(); jam.ElmCount = this->ElmCount; return this->Stream.Duplicate(jam.Stream);}
size_t MemoryUsed(void){return this->Stream.MemoryUsed();}
//------------------------------------------------------------------------------------------------------------
size_t Save(auto& Strm)  //TODO: Optional LZ4 stream before target stream
{
// Strm.Reset();       // Prevents composition!
 if(!this->ElmCount)return 0;   // Nothing to save
 uint8 sig[] = {'B','J','A','M'};
 SSerHdr hdr;
 hdr.Sign   = *(uint32*)&sig;
 hdr.ElmCnt = this->ElmCount;
 size_t res = Strm.Write(&hdr, sizeof(hdr));
 if(Strm.IsFail(res))return res;
 return this->Stream.Save(Strm, this->ElmCount);
}
//------------------------------------------------------------------------------------------------------------
size_t Load(auto& Strm)
{
// Strm.Rewind();    // Prevents composition!
 this->Clear();
 SSerHdr hdr;
 size_t res = Strm.Read(&hdr, sizeof(hdr));
 if(Strm.IsFail(res))return res;
 uint8 sig[] = {'B','J','A','M'};
 if(hdr.Sign != *(uint32*)&sig)return -2;
 this->ElmCount = hdr.ElmCnt;
 return this->Stream.Load(Strm);
}
//------------------------------------------------------------------------------------------------------------
// NOTE: This is slower than accessing tags by a pointer returned by MatchBytes and StoreBytes
// NOTE: Will return invalid values if IDx is not actually an index of a tag record (And you will corrupt the structure by writing it)
//
uint32* GetTagPtr(uint32 IDx)  // 'IDx' is returned from MatchBytes or StoreBytes
{
 if constexpr (UnitLen)return this->Stream.GetElm(IDx); 
  else return nullptr;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Returned 'EndAt' can be used as unique ID of stored data (Like a hash)
//
bool MatchBytes(const uint8* Data, uint Size, uint32* pIDx=nullptr, uint32** pTag=nullptr)     // // TODO: Make sure that words that differ by one last bit are returning different EndAt
{
 if(!Size || !this->ElmCount)return false;
 SBitNode Bitn;
 auto iter = this->Stream.ElmFirst();
 SBit*  bv = nullptr;
 SBits wrd(Data, Size);
 if(this->MatchWordAt(iter, wrd, &Bitn, &bv) != 0)return false;      // No match             // If tags are used, ID should be index of tag itself (Which is unique too)
 if constexpr (UnitLen)      // Have tags  // Use index of a tag as ID
  {
   size_t TagIdx = 2 + bv->Idx;   // A terminal bit takes 2 slots
   if(pIDx)*pIDx = iter.Index() + TagIdx;     // Use tag index as unique ID
   if(pTag){iter += TagIdx; *pTag = &*iter;}
  }
   else
    {
     if(pIDx)*pIDx = iter.Index() + bv->Idx;  // For bit 1 will point to its extended offset field    // This value is unique for each unique byte sequence
     if(pTag)*pTag = nullptr;   // No tags
    }
 return true;  // Match
}
//------------------------------------------------------------------------------------------------------------
sint8 StoreBytes(const uint8* Data, uint Size, uint32* pIDx=nullptr, uint32** pTag=nullptr)
{
 if(!Size)return false;
 SBitNode Bitn;
 auto iter = this->Stream.ElmFirst();
 SBit*  bv = nullptr;
 SBits wrd(Data, Size);
 sint8 res = this->StoreWordAt(iter, wrd, &Bitn, &bv); 
 if constexpr (UnitLen)      // Have tags  // Use index of a tag as ID
  {
   size_t TagIdx = 2 + bv->Idx;   // A terminal bit takes 2 slots
   if(pIDx)*pIDx = iter.Index() + TagIdx;     // Use tag index as unique ID
   if(pTag){iter += TagIdx; *pTag = &*iter;}
  }
   else
    {
     if(pIDx)*pIDx = iter.Index() + bv->Idx;  // For bit 1 will point to its extended offset field    // This value is unique for each unique byte sequence
     if(pTag)*pTag = nullptr;   // No tags
    }
 return res;
}
//------------------------------------------------------------------------------------------------------------
// TODO: Extract list of possible paths for autocomplete
//------------------------------------------------------------------------------------------------------------
//#ifdef _DEBUG
// https://github.com/dwyl/english-words
//
// Results:
//   ~10mb per 100000 words
//
static void DoTest(const achar* SrcFile=nullptr, NSTM::CStrmBase* Strm=nullptr) 
{
 CByteJam<BlkLen, UnitLen, MemMode> jam1;
 CByteJam<BlkLen, UnitLen, MemMode> jam2;

  /*  uint32 EndAt;
    uint32* pTag;
    uint8 Vals[] = {('B'|0x80),'C',('B'|0x00)};
    jam.StoreBytes((uint8*)&Vals[0], 1, &EndAt, &pTag);   // 9: Inconsistent, points to AFTER terminal bit (Writer only?)
    jam.StoreBytes((uint8*)&Vals[1], 1, &EndAt, &pTag);
    jam.StoreBytes((uint8*)&Vals[2], 1, &EndAt, &pTag);   // 7(,8) Terminal bit for 'B'  //
   */
 CArr<achar> arr;      // TODO: Make some decent allocator!
 arr.FromFile(SrcFile);
 if(arr.Size() < 1){LOGMSG("Empty or failed to load: %s",SrcFile); return;}
 LOGMSG("Loaded: %s",SrcFile);
 achar* WrdList = arr.Data();
 achar* WBeg = WrdList;
 uint WrdIdx = 0;
 PX::timeval  tv_before;
 NPTM::NAPI::gettimeofday(&tv_before, nullptr);
 for(;;WrdList++)
  {
   if(*WrdList < 0x20)      // Line break
    {
     if(!*WrdList)break;
     if(!WBeg)continue;  
     uint32 EndAt;
     uint WLen = WrdList - WBeg;
     sint8 res = jam1.StoreBytes((uint8*)WBeg, WLen, &EndAt);
     uint recs = jam1.ElmCount;
    // DBGMSG("Res=%i, WrdIdx=%08X, Count=%08X, Size=%08X, EndAt=%08X: %.*s",res,WrdIdx,recs,recs*sizeof(uint32),EndAt,WLen,WBeg);
     WBeg = nullptr;
     WrdIdx++;
    }
     else if(!WBeg)WBeg = WrdList;  
  }

 PX::timeval  tv_after;
 NPTM::NAPI::gettimeofday(&tv_after, nullptr);
 PX::timeval  tv_diff;
 NDT::timeval_diff(&tv_diff, &tv_after, &tv_before);
 LOGMSG("Storing %u words took %lu.%lu seconds", WrdIdx, tv_diff.sec, tv_diff.usec);
 LOGMSG("Memory used: %lu", jam1.MemoryUsed());

 NPTM::NAPI::gettimeofday(&tv_before, nullptr);
 jam1.Duplicate(jam2);
 NPTM::NAPI::gettimeofday(&tv_after, nullptr);
 NDT::timeval_diff(&tv_diff, &tv_after, &tv_before);
 LOGMSG("Duplication took %lu.%lu seconds", tv_diff.sec, tv_diff.usec);

 if(Strm)
  {
   NPTM::NAPI::gettimeofday(&tv_before, nullptr);
   size_t sres = jam2.Save(*Strm);
   NPTM::NAPI::gettimeofday(&tv_after, nullptr);
   NDT::timeval_diff(&tv_diff, &tv_after, &tv_before);
   LOGMSG("Saving %i took %lu.%lu seconds", sres, tv_diff.sec, tv_diff.usec);
   
   NPTM::NAPI::gettimeofday(&tv_before, nullptr);
   Strm->Rewind();
   sres = jam2.Load(*Strm);
   NPTM::NAPI::gettimeofday(&tv_after, nullptr);
   NDT::timeval_diff(&tv_diff, &tv_after, &tv_before);
   LOGMSG("Loading %i took %lu.%lu seconds", sres, tv_diff.sec, tv_diff.usec);
  }

 WrdIdx  = 0;
 WrdList = arr.Data();
 WBeg    = WrdList;
 NPTM::NAPI::gettimeofday(&tv_before, nullptr);
 for(;;WrdList++)
  {
   if(*WrdList < 0x20)      // Line break
    {
     if(!*WrdList)break;
     if(!WBeg)continue;  
     uint32 EndAt;
     uint WLen = WrdList - WBeg;
     bool res  = jam2.MatchBytes((uint8*)WBeg, WLen, &EndAt);
     uint recs = jam2.ElmCount;
     if(!res){LOGMSG("No match at word %u",WrdIdx);}
    // DBGMSG("Res=%i, WrdIdx=%08X, Count=%08X, Size=%08X, EndAt=%08X: %.*s",res,WrdIdx,recs,recs*sizeof(uint32),EndAt,WLen,WBeg);
     WBeg = nullptr;
     WrdIdx++;
    }
     else if(!WBeg)WBeg = WrdList;  
  }

 NPTM::NAPI::gettimeofday(&tv_after, nullptr);
 NDT::timeval_diff(&tv_diff, &tv_after, &tv_before);
 LOGMSG("Matching %u words took %lu.%lu seconds", WrdIdx, tv_diff.sec, tv_diff.usec);
 LOGMSG("Memory used: %lu", jam2.MemoryUsed());

 if(SrcFile)
  {
   achar dmpbuf[512];
   NSTR::StrCopy(dmpbuf, SrcFile);
   NSTR::StrCopy(NPTM::GetFileExt(dmpbuf), "jam");
   //jam2.SaveToFile(dmpbuf);
   LOGMSG("Saved: %s",&dmpbuf);
  }
}
//#endif
//------------------------------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
// TODO: Policy based memory allocator
