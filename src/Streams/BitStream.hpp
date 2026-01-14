
#pragma once
//============================================================================================================
// https://github.com/Cyan4973/FiniteStateEntropy/blob/dev/lib/bitstream.h
// https://fgiesen.wordpress.com/2018/02/19/reading-bits-in-far-too-many-ways-part-1/
// 
// For performance reasons boundary checks are minimal
// The bit stream is MSB-first by default 
// 
// NOTE: JPEG uses MSB-first bit packing for its bitstream, and DEFLATE (zip) uses LSB-first
// UNIMPLEMENTED: LSB mode treats CacheBPos as "how many bits are already used" starting from bit 0, while MSB treats it as "how many bits are free" counting down from the top.  
//                (CacheBPos is always number of free bits for now)
//
template<typename TS=CStrmBase, uint CacheLen=32, bool IsBitOrderMSB=true, bool IsByteOrderBE=true> class CBitStream
{
 enum EFlags {
   flDidSeek    = 0x80,      // Seek has been done (Need special Flush handling)
   flBufRead    = 0x40,      // Marks the buffer as being read into (CacheULen + CacheURem). // If 'flDirtyBuf' then should seek back and write the cache
   flDirtyBuf   = 0x20,      // Marks the buffer as containing a recently written data. 
   flDirtyCache = 0x10,      // Marks the cache unit as being written.
 };

 //SCVR bool  BitsMSB = IsBitOrderMSB;
 //SCVR bool  BytesBE = IsByteOrderBE;
 SCVR uint8 Mode = uint8(IsBitOrderMSB) | (uint8(IsByteOrderBE) << 1);
 SCVR bool  IsBadCmb   = (!IsBitOrderMSB && IsByteOrderBE) || (IsBitOrderMSB && !IsByteOrderBE);  // (LSB+BE)/(MSB+LE): it is impossible to correctly store last incomplete unit because byte order will be written as '00 00 00 AB' in memory for 32-bit units and the actual number of bytes is one ('AB').
 using ChT  = TSWT<IsBadCmb,uint8,usize>;  // Cache unit type  // LSB + Big-Endian is non-existent?  NOTE: Seeking is still will be weird
 SCVR uint  CacheSize  = CacheLen ? CacheLen : 32;
 SCVR uint  CacheUBits = (sizeof(ChT)*8);
 SCVR uint  CacheUnits = (CacheSize / sizeof(ChT)) + bool(CacheSize % sizeof(ChT));   // ??? Is Shift+AND will compile faster

 TS*    Stream;       // Supposed to be a byte stream  // NOTE: Do not cache any information about it because it may be updated by another stream (Flushing  before giving up control is enough)
 uint8  CacheBPos;    // Bit position (1-based) of a next bit in the current cache unit  // Left to right: First bit is 64, last is 1, 0 - no bits in the cache left
 uint8  CacheUPos;    // Position of a next unit in the buffer
 uint8  CacheULen;    // Number of units currently in the buffer (Including a partial one if there was EOF)  // Reading uses it (May read less than 'CacheUnits')
 uint8  CacheURem;    // Number of bytes which is not present in the last unit in the buffer (Incomplete read at EOF)
 uint8  Flags;
 ChT    Cache;        // Native-Endian current cache unit
 ChT    Buffer[CacheUnits];  // alignas(16) // Big-Endian piece of the underlying stream

//-----------------------------------------------------
// Helper to convert to/from big-endian (bit stream is in byte order)
static _finline ChT CnvUnitByteOrder(ChT val) { return ChangeByteOrder<ChT, IsByteOrderBE>(val); }
//-----------------------------------------------------
void _finline ResetCachePos(void)
{
 //if constexpr (IsBitOrderMSB) 
this->CacheBPos = CacheUBits; // This is correct only for 'Write', for 'Read' it must be 0 to force the cache load (Hence extra check in 'Read')
//   else this->CacheBPos = 0;  // LSB: bits filled so far
}
//-----------------------------------------------------
//bool _finline IsCachePosReset(void)  // UNUSED
//{
// return (this->CacheBPos == CacheUBits);
//}
//-----------------------------------------------------
bool _finline IsConsumesCache(uint32 Cnt)
{
//if constexpr (IsBitOrderMSB) 
return Cnt >= this->CacheBPos;  // MSB: CacheBPos is more like number of 'free' bits. MSB: NextBitPos is CacheBPos+1. LSB should be: NextBitPos is CacheBPos 
// else return Cnt > (CacheUBits - this->CacheBPos);  // LSB: comparing against remaining free space      
}
//-----------------------------------------------------
void _finline AdvanceBitCache(uint32 Cnt)
{
// if constexpr (IsBitOrderMSB) 
this->CacheBPos -= Cnt;  // MSB: decrease free bits
//   else this->CacheBPos += Cnt;  // LSB: increase filled bits   
}
//-----------------------------------------------------
uint32 _finline BitsInCache(void)  // Bits used in current unit
{
// if constexpr (IsBitOrderMSB) 
return CacheUBits - this->CacheBPos;  // MSB: total minus free
//   else return this->CacheBPos;  // LSB: directly the filled count
}
//-----------------------------------------------------
uint32 _finline BytesPreread(void)
{
 return (this->CacheULen * sizeof(ChT)) - this->CacheURem;
}
//-----------------------------------------------------
uint8 _finline FullUnitsPreread(void)
{
 return this->CacheULen - bool(this->CacheURem);
}
//-----------------------------------------------------
bool _finline IsCacheBufEndW(void){return this->CacheUPos >= CacheUnits;}       // Can write past 'CacheULen'
bool _finline IsCacheBufEndR(void){return this->CacheUPos >= this->CacheULen;}  // Also prevents reading beyond writing position on unread buffer
//-----------------------------------------------------
void _finline LoadCacheUnit(void)        // Read
{
 this->Cache = CnvUnitByteOrder(this->Buffer[this->CacheUPos]); 
 this->ResetCachePos();  // this->CacheBPos = CacheUBits;   // We have a full unit (The buffer is unit-aligned) 
 RemoveFlags(this->Flags, flDirtyCache);  
} 
//-----------------------------------------------------       
void _finline SaveCacheUnit(void)    // Write
{
 this->Buffer[this->CacheUPos] = CnvUnitByteOrder(this->Cache);
 this->Flags |= flDirtyBuf;  
 RemoveFlags(this->Flags, flDirtyCache);   //|flBufRead);   // flBufRead ????????????? 
 //this->CacheBPos = 0;   // Not needed
}    
//-----------------------------------------------------
ChT  MergeCacheWithUnit(ChT Unit)
{
 if constexpr (IsBitOrderMSB) 
  {
   const ChT Mask = MakeBitMask<ChT>(this->CacheBPos);  //  ChT(-1) >> (CacheUBits - this->CacheBPos); //  ~(ChT(-1) << this->CacheBPos);
   return (this->Cache & ~Mask) | (Unit & Mask);
  }
 else 
  {
   const ChT Mask = MakeBitMask<ChT>(CacheUBits - this->CacheBPos); //  (ChT(1) << this->CacheBPos) - 1;
   return (this->Cache & Mask) | (Unit & ~Mask); 
  }
}
//-----------------------------------------------------
// MSB: New bits fill from the left (high bits), we take high bits from the value first
// LSB: New bits fill from where we left off (from the right), we take low bits from the value first
// 'Cnt' is expected to be '> 0' (Checked by a caller)
template<bool LastBits, typename T> void GetCacheBits(T& Val, uint32& Cnt, uint32& Num)  // NOTE: No position check
{
 if constexpr (LastBits)  // Cnt is less than CacheBPos (and less than size of ChT in bits) and fits in the cache
  {
   if constexpr (IsBitOrderMSB)  // AAABBCCCCDDDEEE
    {
     const uint32 BitsN = Cnt;
     this->CacheBPos   -= BitsN;
     const uint32 Shift = this->CacheBPos;
     const ChT   Mask   = ChT(-1) >> (CacheUBits - BitsN);  // ~(ChT(-1) << BitsN);   // Safe: 'const ChT Mask = ChT(-1) >> (CacheUBits - Num)' but Num is expected to be less than CacheBPos anyway
     Val = (Val << BitsN) | ((this->Cache >> Shift) & Mask);     // Take last (low) bits from the cache
    }
   else    // LSB: EEEDDDCCCCBBAAA - bits are consumed from right to left
    {
     const uint32 BitsN = Cnt;    // Not expected to be 0 !!!!
     const uint32 BitsP = CacheUBits - this->CacheBPos;
     const ChT    Mask  = ChT(-1) >> (CacheUBits - BitsN);  //(ChT(1) << BitsN) - 1;
     Val = (Val >> BitsN) | (T((this->Cache >> BitsP) & Mask) << (Num-BitsN));
     this->CacheBPos   -= BitsN;
    }
  }
 else  // Cnt is greater than number of data bits in the cache unit at current position  // (Cnt >= CacheBPos)
  {
   if constexpr (IsBitOrderMSB)
    {
     const uint32 Shift = this->CacheBPos;    // Not expected to be 0 !!!!
     const ChT    Mask  = ChT(-1) >> (CacheUBits - Shift);    // UB: 'const ChT   Mask  = ~(ChT(-1) << Shift);'   
     Val  = (Val << Shift) | (this->Cache & Mask);      
     Cnt -= Shift; 
    }
   else    // LSB: Take all remaining bits from current position
    {
     const uint32 BitsN = this->CacheBPos;    // Not expected to be 0 !!!!
     const uint32 BitsP = CacheUBits - BitsN;
     const ChT    Mask  = ChT(-1) >> (CacheUBits - BitsN);  //(ChT(1) << BitsN) - 1;
     Cnt -= BitsN;
     Val  = (Val >> BitsN) | (T((this->Cache >> BitsP) & Mask) << (Num-BitsN));
    }
  }
}
//-----------------------------------------------------
// 'Cnt' is expected to be '> 0' (Checked by a caller)
template<bool LastBits, typename T> void SetCacheBits(T& Val, uint32& Cnt)  // NOTE: No position check
{
 if constexpr (LastBits)  // Cnt is less than CacheBPos (and less than size of ChT in bits) and fits in the cache
  {
   const uint32 BitsN = Cnt;
   if constexpr (IsBitOrderMSB)
    {
     this->CacheBPos   -= BitsN;    // Will have several bits left
     const uint32 Shift = this->CacheBPos;      // TODO: Check a fast path 'if cnt == 0'
     const ChT  Mask    = ChT(-1) >> (CacheUBits - BitsN);  // (ChT(1) << BitsN) - 1; - broken when BitsN is 'sizeof(ChT)*8'
     this->Cache        = (this->Cache & ChT(~(Mask << Shift))) | ((ChT(Val) & Mask) << Shift);  // We always assume overwriting so must mask out any existing bits first
    }
   else   // LSB: Write bits at current position (from right)
    {  
     const uint32 Shift  = CacheUBits - this->CacheBPos;   // Actual position of bits to write   // NOTE: Have to subtract bacause CacheBPos is not from 0 as it shouldd be for LSB
     const ChT    Mask   = ChT(-1) >> (CacheUBits - BitsN);  // (ChT(1) << BitsN) - 1; - broken when BitsN is 'sizeof(ChT)*8'
     this->Cache         = (this->Cache & ChT(~(Mask << Shift))) | ((ChT(Val) & Mask) << Shift);     // Shift in low bits
     this->CacheBPos    -= BitsN; 
     Val >>= BitsN;   // Remove consumed bits
    }
  }
 else   // Cnt is greater than number of free bits in the cache unit  // (Cnt >= CacheBPos)
  {
   const uint32 BitsN = this->CacheBPos;      // Number of free bits (consume all)  // Not expected to be 0 !!!!
   Cnt -= BitsN;    // Fill in all bits
   if constexpr (IsBitOrderMSB)
    {
     const uint32 Shift = Cnt;   // To get high bits
     const ChT Mask     = ChT(-1) >> (CacheUBits - BitsN);    // UB: 'const ChT Mask = (ChT(1) << this->CacheBPos) - 1;'
     this->Cache        = (this->Cache & ChT(~Mask)) | (ChT(Val >> Shift) & Mask);  
    }
   else   // LSB: Fill remaining space in cache
    {     
     const uint32 Shift  = CacheUBits - BitsN;   // Actual position of bits to write
     const ChT    Mask   = ChT(-1) >> Shift;  // (ChT(1) << BitsN) - 1; - broken when BitsN is 'sizeof(ChT)*8'
     this->Cache         = (this->Cache & ChT(~(Mask << Shift))) | ((ChT(Val) & Mask) << Shift);     // Shift in low bits
     Val >>= BitsN;   // Remove consumed bits
    }
  }
}
//-----------------------------------------------------
// Always fetch the entire cache block if the current bit offset is outside of it
// The current cache unit is represented of the current slot in it (With converted endianess)
// Seek, then write partially(uncomplete unit!), then reading - no bits in the buffer!
// Note: The cache is slower when mixing Reads and Writes in a single stream instance
// NOTE: Next 'this->Cache' is expected to be Get/Set after this
//
usize CacheBufFlush(usize WrPart=0)
{
 if(!(this->Flags & flDirtyBuf))return 0;   // If nothing has been written then nothing to save 
 if(this->Flags & flBufRead)  // The buffer was initially requested by 'Read' but then 'Write' has been done into it    // Have to seek back and write it (All of it, no track of dirty units for now)  // TODO: Test the performance
  {
   ssize Offs = this->BytesPreread();   // The last   // CacheURem may come only from 'CacheBufLoad'
   usize res = this->Stream->Seek(-Offs, SEEK_CUR);   // No other choice when mixing reading with writing(doing overwrites)
   if(TS::IsFail(res))return res;
  }
 usize Length = (Max(this->FullUnitsPreread(), this->CacheUPos) * sizeof(ChT)) + WrPart;     // Write by unit granularity here. 'Flush' will handle any byte tails
 usize res = this->Stream->Write(&this->Buffer, Length);     // Flush the buffer   
 if(TS::IsFail(res))return res;
 // NOTE: No handling if was written less than should be
 this->CacheURem = 0;
 this->CacheULen = 0;  // CacheUnits;  // Ready to write (Considered empty)   // Can't read beyond that - no data
 this->CacheUPos = 0; 
 MOPR::ZeroObj(this->Buffer);    // The buffer is saved but 'this->Cache' may still contain some unsaved bits if 'flDirtyCache'
 RemoveFlags(this->Flags, flDirtyBuf);   // |flBufRead);    // 'flBufRead' will stick until Seek or Flush, marks that we do reading from this stream
 return res; 
}
//-----------------------------------------------------
// We expect reading, so we will read the piece of stream into the cache
// If the buffer was only written to then 'CacheULen' was specifying the number of WHOLE written units and then it was saved above
//
usize CacheBufAdvance(void)     
{
 usize res = this->CacheBufFlush();
 if(TS::IsFail(res))return res;    // Will it be optimized away if CacheBufFlush is inlined?
 bool BufCleared = bool(res);

 if(!BufCleared)MOPR::ZeroObj(this->Buffer);    // Remove the condition? Mixed Read/Write will be slower without it?  // TODO: Test performance
 res = this->Stream->Read(&this->Buffer, sizeof(this->Buffer));     // Attempt the full buffer read
 if(TS::IsFail(res))
  {
   if(TS::IsEOF(res))
    {
     RemoveFlags(this->Flags, flBufRead);      // No data in the buffer 
     this->ResetCachePos();       //  this->CacheBPos = CacheUBits;    // Should drop any further processing on EOF
     this->CacheURem = this->CacheULen = this->CacheUPos = 0;    
     this->Cache = 0;
    }
   return res;
  }  
 usize rem = res & (sizeof(ChT)-1);      // Incomplete unit bytes 
 this->CacheURem = (sizeof(ChT) - rem) & (sizeof(ChT)-1);  // Number of missing bytes in a last unit  (Needed for resync)
 this->CacheULen = (res / sizeof(ChT)) + bool(rem);   // Total units actually in the buffer now   // TODO: Use shift instead of DIV (precalc the shift)
 this->CacheUPos = 0;     // Already done by 'CacheBufSave'       // From the beginning   
 return res;   // What to return?
}
//-----------------------------------------------------

public:
CBitStream(TS* Strm){this->Init(Strm);}
CBitStream(void){this->Init(nullptr);}
~CBitStream(void){this->Flush();}   // Ignoring possible errors, but well...
//-----------------------------------------------------
void Init(TS* Strm)
{
 this->Stream = Strm; 
 this->CacheUPos = 0;     // CacheWrite will check Flags too
 this->CacheULen = 0;
 this->CacheURem = 0;
 this->Flags = 0;
 this->Cache = 0;
 this->ResetCachePos();   // this->CacheBPos = CacheUBits;  // This is correct only for 'Write', for 'Read' it must be 0 to force the cache load (Hence extra check in 'Read')  
 MOPR::ZeroObj(this->Buffer);   
}
//-----------------------------------------------------
usize Rewind(void) 
{
 this->Flush();
 this->Init(this->Stream); 
 return this->Stream->Rewind();   // Just do rewind, nothing is in the cache yet after 'Flush' (First bit, first byte - cache unit alignment)
}
//-----------------------------------------------------
usize Reset(void)
{
 this->Flush();
 this->Init(this->Stream);
 return this->Stream->Reset();  
}
//-----------------------------------------------------
usize Discard(void)
{
 this->Init(this->Stream);
 return this->Stream->Discard(); 
}
//-----------------------------------------------------
uint64 Size(void)  // Returns size in bits (Not precise, may be Byte aligned(Seek/Read))
{
 uint64 offs = this->Offset();            // This will give only byte-aligned size
 if(TS::IsFail(offs))return offs;
 uint64 size = this->Stream->Size();      // This gives the current bit-precise position but it gives valid size only when writing last 
 if(TS::IsFail(size))return size;
 size <<= 3;  // *8          
 return Max(offs, size);
}
//-----------------------------------------------------
uint64 Offset(void)  // Current position in bits   // TODO: Test it
{
 uint64 Offs = this->Stream->Offset();    // Current byte position in the stream
 if(TS::IsFail(Offs))return Offs;
 usize ChLen = this->BytesPreread();  // Number of preread bytes in the cache
 usize ChPos = (this->CacheUPos * sizeof(ChT));  // R/W position in the cache // May be greater than CacheLen when writing
 Offs  -= ChLen;
 Offs  += ChPos;
 Offs <<= 3;  // *8
 Offs  += this->BitsInCache();  // Current unit
 return Offs;
}
//-----------------------------------------------------
// Resets the entire context
// Returns current byte offset of underlying stream
// NOTE: In LSB mode bit order inside values will be different from bit order in the stream - seek only on known value boundaries
// All seeking is done at unit granularity to be CPU cache friendly for underlying memory streams
//
uint64 Seek(sint64 offs, STRM::ESeek wh)             
{ 
 switch(wh)
  {
   case STRM::SEEK_SET:
    if(offs < 0)return -PX::ERANGE;   // EINVAL
    break;
   case STRM::SEEK_CUR:   // NOTE: Should not request current byte position - this operation may be slow (file streams). So just expect the position to be unit-aligned already. 'Flush' should update it.
   case STRM::SEEK_END:   // NOTE: May be incorrect because of padding bits (Byte aligned)!
    {
     uint64 bpos = (wh == STRM::SEEK_END)?(this->Size()):(this->Offset());      // Must keep it unit-aligned
     if(TS::IsFail(bpos))return bpos;
     if((offs < 0) && (-offs > bpos))return -PX::ERANGE;   // EINVAL
     offs = bpos + offs;       // +/-  // Seeking beyond stream's end may be unsupported 
     wh   = STRM::SEEK_SET;    // NOTE: Losing range precision to MAX_INT64
    }
    break;
  }
   
 this->Flush();    // Flush pending writes  // 'Flush' is expected to reset the entire context despite any errors
 this->ResetCachePos();   // 'Write' will expect this but read have to check 'CacheULen' and reset CacheBPos to 0. Otherwise we would have to preload cache after each Flush/Seek even if we will only write the stream
 this->Cache = 0;        

 const uint64 boffs    = (offs < 0)?-offs:offs;
 const uint64 ByteOffs = boffs >> 3;   // / 8  // Convert bit offset - byte + bit
 const uint   ExBytes  = ByteOffs & (sizeof(ChT)-1);
 const uint   BitOff   = (boffs & 7) + (ExBytes << 3);  // Incomplete bytes 
 const sint64 RealOffs = ByteOffs - ExBytes; 

 uint64 ResOffs = this->Stream->Seek(RealOffs, wh);       // Seek the underlying stream  // With SEEK_CUR, even if RealOffs is 0 it still returns actual stream offset
 if(TS::IsFail(ResOffs))return ResOffs;
 if(BitOff)   // If not byte-aligned, preload one cache unit and advance BitPos   // TODO: Check if the bit offset correct for LSB
  {
   usize res = this->Stream->Read(&this->Buffer, sizeof(ChT));     // Read only one cache unit here or the entire cache buffer? (For writing one is enough but for reading the entire cache will be more efficient (if need more than one unit))  
   if(TS::IsFail(res))return res;  // NOTE: IsFail includes EOF
   usize rem = res & (sizeof(ChT)-1);      // Incomplete unit bytes 
   this->CacheURem = (sizeof(ChT) - rem) & (sizeof(ChT)-1);  // Number of missing bytes in a last unit  (Needed for resync)
   this->LoadCacheUnit(); 
   this->AdvanceBitCache(BitOff);  // this->CacheBPos -= BitOff;   // Is this enough for Read and Write to work properly? (The cache buffer is still in reset state)
   this->CacheULen++;              // Is enough for 'Write'. Will read the full buffer when needed by 'Read'
   this->Flags |= flBufRead;
   ResOffs += res;
  }
 this->Flags |= flDidSeek;
 return ResOffs;  
}
//-----------------------------------------------------
// Flushes all cache but does not reset the stream position
// Can't know what will be done after this. If read, then the cache should be fetched. If write then only misaligned bits are needed in cache.
//
usize Flush(void)        // Call this when done writing bits to keep the underlying stream updated
{
 usize Flushed;
 if(IsSetFlags(this->Flags, flDidSeek|flDirtyCache) && ((this->CacheUPos >= this->CacheULen) || !(this->Flags & flBufRead))) // May need merging   
  {   
   Flushed = this->CacheBufFlush();    // Flush the buffer first
   if(TS::IsFail(Flushed))return Flushed;
   ChT LastUnit = 0;
   usize res = this->Stream->Read(&LastUnit, sizeof(ChT));    
   if(!TS::IsFail(res))   // May fail if we at EOF already
    {
     res = this->Stream->Seek(-ssize(res), SEEK_CUR);   // Seek back so we can overwrite
     if(TS::IsFail(res))return res;
    }
     else if(!TS::IsEOF(res))return res;
   LastUnit = CnvUnitByteOrder(this->MergeCacheWithUnit(CnvUnitByteOrder(LastUnit)));   
   usize Length = (this->BitsInCache() + 7) >> 3;    // Round up to byte      
   res = this->Stream->Write(&LastUnit, Length);     // Flush the last unit   // !!! Writing an entire unit but must preserve the bit position somehow !!!
   if(TS::IsFail(res))return res;  
   RemoveFlags(this->Flags, flDirtyCache);
   Flushed += res;
  }
 else    // Can just write the cache unit into the stream without merging
  {
   usize WrExtra = 0;
   if(this->Flags & flDirtyCache)  // LoadCacheUnit -> Write  // The current cache unit has been written to - save it before reading the buffer
    {
     this->SaveCacheUnit();        // Write the cache unit back into the buffer  // Assuming this unit is complete since we read it completely
     if(this->CacheUPos >= this->FullUnitsPreread())WrExtra = (this->BitsInCache() + 7) >> 3;  // Round up to byte    // Flush the tail but do not count it as full unit in the buffer  
    }
   Flushed = this->CacheBufFlush(WrExtra); 
   if(TS::IsFail(Flushed))return Flushed;
   if constexpr (sizeof(ChT) )
     if(WrExtra)     // We must keep the stream unit-aligned
      {
       usize res = this->Stream->Seek(-ssize(WrExtra), SEEK_CUR);   // Seek back to keep the stream unit-aligned  // ????????????????????????
       if(TS::IsFail(res))return res;     
      }
  }
 if(!Flushed)  // Clear any read data (Not in case of a error or if already done when some data has been written)
  {
   this->CacheURem = 0;
   this->CacheULen = 0;  // CacheUnits;  // Ready to write (Considered empty)   // Can't read beyond that - no data
   this->CacheUPos = 0;
   MOPR::ZeroObj(this->Buffer);  
  }
 //this->ResetCachePos();  // this->CacheBPos = CacheUBits;  // 'Write' will expect this but read have to check 'CacheULen' and reset CacheBPos to 0. Otherwise we would have to preload cache after each Flush/Seek even if we will only write the stream
 //this->Cache = 0;        // Do not reset cache unit and current bit position in it
 return Flushed;
}
//-----------------------------------------------------
// NOTE: Can read what wasn't written yet (Empty cached bits after 'CacheBPos') because bit-precise EOF tracking is expensive (requires to know the current stream position and size)
// Returns number of bytes the underlying stream has advanced or a stream error: ( < 0) = Error/EOF; (>= 0) = Success  // Check !STRM::IsError(res)
template<bool DoCntChk=false> usize Read(auto* Val, uint32 Cnt)
{
 static_assert(!IsRefV< RemoveRefT<decltype(*Val)> >);
 using VT = MakeUnsignedT< RemoveRefT<decltype(*Val)> >;  // decltype(*Val): Dereferencing a pointer always yields an lvalue reference type
 SCVR  uint32 VBitLen = (sizeof(VT)*8);
 uint32 BitNum = Cnt;       // For LSB
 if constexpr (DoCntChk)
  {
   if(!Cnt){*Val = 0; return 0;}
   if(Cnt > VBitLen)Cnt = VBitLen;
  }
 VT    Dst = 0;
 usize Loaded = 0;
 if(!this->CacheULen)goto NoBits;  // this->CacheBPos = 0; // !!!!! For Read CacheBPos should be set to 0 after seek/Flush but for Write it should be set to Max unit bits
 while(this->IsConsumesCache(Cnt))     // Need more bits than currently available in the cache (or equal)   // NOTE: We may attempt to read after a write and the position will be at yet not cached bits
  {
   this->GetCacheBits<false>(Dst, Cnt, BitNum);   // Take the remaining bits from current cache unit
NoBits:
   if(this->Flags & flDirtyCache)  // The current cache unit has been written to - save it before reading the buffer
    {
     this->SaveCacheUnit();        // Assuming this unit is complete since we read it completely   // ?????????? Merging?????????????????????  // Write->Seek>Write->Read
    }
   this->CacheUPos++;              // Need next unit  
   this->Flags |= flBufRead;       // TODO: CacheBufAdvance must handle dirty cache 
   if(this->IsCacheBufEndR())      // Check the buffer first (It may be empty)  Initial 'CacheUPos' and 'CacheULen' is set to 0
    { 
     usize res = this->CacheBufAdvance();     // Returns number of bytes read    // BEWARE: The underlying stream will be advanced by the cache amount (Flushing will resync to byte granularity)
     if(TS::IsFail(res))
      {
       if(Cnt){*Val=0; return res;}       // Do not clear a complete value on Error/EOF (An attempt to preload more units while the Value is already complete)
         else {*Val = VT(Dst); return STRM::SEOF;}  // Return a complete value (no unread bits left) and EOF
      }    
     Loaded += res;
    }
   this->LoadCacheUnit();   // Fetch the current unit from the buffer (may be not 0 and contain some cached data) 
  }
 // Rest of the bits which fit into available cache bits
 if(Cnt)
  {
   this->GetCacheBits<true>(Dst, Cnt, BitNum);
  }
 *Val = VT(Dst);
 return Loaded;
}
//-----------------------------------------------------
// It is able to overwrite in the stream (Whole cache units)
// Returns number of bytes the underlying stream has advanced or a stream error: ( < 0) = error; (>= 0) = success
template<bool DoCntChk=false, bool DoSrcMsk=false> usize Write(NoRef auto Val, uint32 Cnt)
{
//static_assert(!IsRefV<decltype(Val)>);
 using VT = MakeUnsignedT<decltype(Val)>;  // TODO: RemoveRef
 SCVR uint32 VMaxBits = (sizeof(VT)*8);
 if constexpr (DoCntChk)
  {
   if(!Cnt)return 0;
   if(Cnt > VMaxBits)Cnt = VMaxBits;   // Skip that safeguard?
  }
 VT    Src = VT(Val); 
 usize Saved = 0;
 if constexpr(DoSrcMsk) Src &= ((VT(1) << Cnt) - 1);   // Should mask out any extra high bits?  // NOTE: Optional
 while(this->IsConsumesCache(Cnt))  // Have enough bits to form a complete cache unit and store it in the buffer    // Low bits will be left for another unit
  {
   this->SetCacheBits<false>(Src, Cnt);    // No need to mask, adds high bits and full cache units only
   this->SaveCacheUnit();       // Store the entire cache unit at current position
   this->CacheUPos++;     
   if(this->IsCacheBufEndW())                    // Check the buffer first: 'CacheUPos' may be left overflown by 'CacheRead'
    { 
     usize res = this->CacheBufFlush();   // Returns number of bytes written 
     if(TS::IsFail(res))return res; 
     Saved += res;
    }
   this->LoadCacheUnit();   // Fetch the current unit from the buffer (may be not 0 and contain some cached data) // this->Buffer[this->CacheUPos++] = CnvUnitByteOrder(this->Cache);
  }
 // Rest of the bits which fit into size of 'CacheBPos'  // Low bits will be zeroes in the current unit if there is more free bits
 if(Cnt)   // Either we check here or the next time the 'While' above will enter with 'cnt' and 'CacheBPos' both are being 0
  {  
   this->SetCacheBits<true>(Src, Cnt); 
   this->Flags |= flDirtyCache;
  }
 return Saved;
}
//-----------------------------------------------------
// Copy to a bit stream
// Slow but can convert between different stream types (MSB/LSB, BE/LE)
// NOTE: If destination stream have different bit order(MSB/LSB) then multi-bit values will have inverted bit order
// Returns number of bits transferred
//
uint64 ToStream(auto* strm, uint64 size=uint64(-1), uint64 from=0) 
{
// static_assert(strm->Mode == this->Mode);       // Enforce 'strm' to be CBitStream of the same mode
 uint64 sres; 

 if(from != STRM::etsFromCurrPos)
  {
   sres = this->Seek(from, STRM::SEEK_SET);
   if(TS::IsFail(sres))return sres; 
  }
   else this->Flush();   // We must flush to underlying byte stream first  // Will use 'from' to properly seek later

 if constexpr (this->Mode == strm->Mode)   // Fast path if both streams are of the same mode
  {
   uint16 SrcCacheBits = this->BitsInCache();
   uint16 DstCacheBits = strm->BitsInCache();
   if(!(SrcCacheBits & 7) && !(DstCacheBits & 7))  // Both streams are byte-aligned
    {
     uint64 SrcBitOffs = this->Offset();
     if(TS::IsFail(SrcBitOffs))return SrcBitOffs;
     uint64 DstBitOffs = strm->Offset();
     if(TS::IsFail(DstBitOffs))return DstBitOffs;
     strm->Flush();  // Flush the destination bit stream to its underlying byte stream 

     usize SrcByteOffs = SrcBitOffs >> 3;  // /8
     usize DstByteOffs = DstBitOffs >> 3;  // /8
     usize BytesToCopy = size >> 3;        // /8 // Will be still large enough to mean MAX bytes if it was -1 initially
     sres = this->Stream->ToStream(&strm->Stream, BytesToCopy, SrcByteOffs); 
     if(TS::IsFail(sres))return sres;

     // Resync both bit streams to their byte streams
     usize BitsWritten = BytesToCopy << 3;  //*8
     sres = this->Seek(SrcBitOffs+BitsWritten, STRM::SEEK_SET);
     if(TS::IsFail(sres))return sres;
     sres = strm->Seek(DstBitOffs+BitsWritten, STRM::SEEK_SET);
     if(TS::IsFail(sres))return sres;
     sres = BitsWritten;   // And we will continue to copy rest of bits bit-by-bit
    }
     else sres = 0;
  }
   else sres = 0;

// Copy bit-by-bit
 for(;sres < size;sres++)
  {
   ChT Val; 
   usize res = this->Read(&Val, 1);
   if(TS::IsFail(res))
    {
     if(TS::IsEOF(res))break;    // NOTE: Try to not rely on copying until EOF because it may read some extra alignment bits at the end
     return res;
    }
   res = strm->Write(Val, 1);
   if(TS::IsFail(res))return res;
  }
 return sres;
}
//-----------------------------------------------------
};

template<typename TS=CStrmBase, uint CacheLen=32> using CBitStreamMSB = CBitStream<TS, CacheLen, true,  true>;
template<typename TS=CStrmBase, uint CacheLen=32> using CBitStreamLSB = CBitStream<TS, CacheLen, false, false>;
//============================================================================================================
