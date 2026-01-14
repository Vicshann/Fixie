
#pragma once

//---------------------------------------------------------------------------
// NOTE: This array will not split items if they don't fit in underlying unit (not Pow2 item sizes)
// ItmLen: Size of bit unit (1 bit by default)
template<uint ItmLen=1, bool IsBitOrderMSB=true, bool IsByteOrderBE=true, bool AutoGrow=true> class CBitArr
{
 using UT = uint64;     // Unit type. Should be as large as possible. Its bit-side defines max item bit-size
 using VT = TypeForSizeT<ItmLen/8, true>;    // Value unit type

 static_assert(ItmLen <= (sizeof(UT)*8));    // A better idea?

 SCVR uint BitsPerUT  = sizeof(UT) * 8;      // Number of bits in the underlying array unit
 SCVR uint BitsPerVT  = sizeof(VT) * 8;      // Max number of bits in the value
 SCVR uint ItemsPerUT = BitsPerUT / ItmLen;  // How many ItmLen-bit units fit in one UT  (No splitting between units - may left some unused bits)

 CArr<UT> UnitArr;    // TODO: Replace?
 usize    NumItems;   // Required to keep track of actual number of items in incomplete units
 usize    ItmFree;    // Number of free items in last unit  (Faster than recalculate every time)

//-----------------------------------------------------
// Helper to convert to/from big-endian (bit stream is in byte order)
static _finline UT CnvUnitByteOrder(UT val) { return ChangeByteOrder<UT, IsByteOrderBE>(val); }
//-----------------------------------------------------
// Create a mask for ItmLen bits at the given position
// bitPos is right-to-left from D0
static _finline UT MakeMask(uint bitPos) 
{
 UT mask = ((UT(1) << ItmLen) - 1);   // Create mask of ItmLen bits
 return mask << bitPos;
}
//----------------------------------------------------- 
// bitPos is right-to-left from D0
static _finline UT MakeUnit(const VT val, uint32 bitPos)
{
 return (UT(val) << bitPos);
}
//----------------------------------------------------- 
static _finline VT ValFromUnit(const UT unit, uint32 bitPos)
{
 UT mask = ((UT(1) << ItmLen) - 1);   // Create mask of ItmLen bits
 return VT((unit >> bitPos) & mask);
}
//----------------------------------------------------- 
// Assumes that numFree is not MaxItemsInUnit
template<bool Val=false> static _finline UT SetUnitFreeSpaceTo(const UT unit, uint32 numFree)
{
 if(!numFree)return unit;
 const uint32 BitsFree = numFree * ItmLen;
 UT ValMask = IsBitOrderMSB ? (UT(-1) << BitsFree) : ((UT(1) << BitsFree) - 1);  // Constexpr?   // Mask to keep value bits
 UT BitVal  = Val ? UT(-1) : UT(0);
 return Val ? (unit | ~ValMask) : (unit & ValMask);
} 
//-----------------------------------------------------
_finline UT GetUnit(usize at)
{
 return CnvUnitByteOrder(this->UnitArr[at]);
}
//----------------------------------------------------- 
_finline void SetUnit(const UT unit, usize at)
{
 this->UnitArr[at] = CnvUnitByteOrder(unit);
}
//----------------------------------------------------- 
// Calculate which UT element and bit position within it
template<bool ForceGrow=false, bool NoGrow=false> _finline bool GetPosition(usize at, usize& utIdx, uint32& bitPos)
{
 if(at >= this->Count())      // Out of bounds
 {
  if constexpr (!NoGrow && (AutoGrow||ForceGrow)) { if(this->Resize(at + 1) < 0)return false; }   // Ensure array is large enough
    else return false;  // Should return errors instead?
 }
 usize idx  = at / ItemsPerUT;     // Which UT element           // TODO.OPT: Check that it does not divides twice here.    // TODO: Optimize for 1-bit items?
 usize pos  = at % ItemsPerUT;     // Number of remaining items
 usize bpos = pos * ItmLen;        // Bit position within that UT
 if constexpr(IsBitOrderMSB) bpos = (BitsPerUT - ItmLen) - bpos;
 utIdx  = idx;
 bitPos = bpos;    // Bit position right-to-left from D0
 return true;  
}
//-----------------------------------------------------
// Pos is from D0, bitCnt is the total number of bits to fill
// Fills bitCnt bits starting at bitPos with the pattern of val repeated
static _finline UT FillUnit(const UT unit, const VT Val, uint32 bitPos, uint32 bitCnt)    //  (toBitPos - fromBitPos) / ItmLen     // TODO: Simple for 1-bit units
{
 UT Mask = ((UT(-1) >> (BitsPerUT - bitCnt)) << bitPos);  
 UT resv = unit & ~Mask;
 if constexpr (ItmLen > 1)
  {
   UT vv = 0;
   for(int num=bitCnt;num > 0;num -= ItmLen)    // Build the fill pattern by repeating val for each item position
    {
     vv <<= ItmLen;
     vv  |= Val;
    }
    resv |= (vv << bitPos) & Mask;
  }
   else if(Val) resv |= Mask;  // Fill entire range with 1s  // resv |= (Val ? Mask : 0);   // Just apply the mask itself
 return resv;
}
//-----------------------------------------------------
// Returns number of bits for the item range, including bits of ItmBitPosFirst and ItmBitPosLast
template<bool bHaveFirst=true, bool bHaveLast=true> static _finline void UnitItemRangeToBits(uint32 ItmBitPosFirst, uint32 ItmBitPosLast, uint32& bitPos, uint32& bitCnt)
{
 if constexpr (!bHaveFirst)
  {
   if constexpr (IsBitOrderMSB) ItmBitPosFirst = BitsPerUT - ItmLen;   // First bit    // MSB: 64 - 0
     else ItmBitPosFirst = 0;
  }
 if constexpr (!bHaveLast)
  {
   if constexpr (IsBitOrderMSB) ItmBitPosLast = 0;   // Last bit
     else ItmBitPosLast = BitsPerUT - ItmLen;
  }
 if constexpr (IsBitOrderMSB)
  {
   bitCnt = (ItmBitPosFirst - ItmBitPosLast) + ItmLen; 
   bitPos = ItmBitPosLast;  // Start filling from lowest bit position
  }
   else 
    {
     bitCnt = (ItmBitPosLast - ItmBitPosFirst) + ItmLen; 
     bitPos = ItmBitPosFirst;  // Start filling from lowest bit position
    }
}
//-----------------------------------------------------
 
public:
 CBitArr(void){this->NumItems = 0; this->ItmFree = 0;}
 CBitArr(usize numItems){this->NumItems = 0; this->ItmFree = 0; this->Resize(numItems);}
//-----------------------------------------------------
usize Size(void) const   // In bytes
{
 return this->UnitArr.Size();
}
//-----------------------------------------------------
usize Count(void) const 
{
 return this->NumItems;  //(this->ByteArr.Size() * 8) / ItmLen;  // Total number of ItmLen-bit units
}
//-----------------------------------------------------
// Clear all bits
void Clear(void)
{
 this->UnitArr.Clear();  //for(usize idx=0, tot=this->ByteArr.Count(); idx < tot; idx++)this->ByteArr[idx] = 0;      // TODO: memset
 this->NumItems = 0;
 this->ItmFree  = 0;
}
//-----------------------------------------------------
// Resize to hold 'numItems' of ItmLen-bit values
// Return value?
ssize Resize(usize NewNumItems)   
{
 if(NewNumItems > this->NumItems)  // Grow
  {
   usize ItmToGrow = NewNumItems - this->NumItems;
   if(ItmToGrow <= this->ItmFree)
    {
     this->ItmFree  -= ItmToGrow;
     this->NumItems += ItmToGrow;
     return NewNumItems;
    }
  }
 else  // Shrink
  {
   if(NewNumItems == this->NumItems)return NewNumItems;
   usize ItmInLastUnit = ItemsPerUT - this->ItmFree;
   usize ItmToShrink   = this->NumItems - NewNumItems;
   if(ItmToShrink <= ItmInLastUnit)  // Just release the items from last unit  // Later can reuse those or shrink more without even touching the underlying array of units
    {
     this->ItmFree  += ItmToShrink;
     this->NumItems -= ItmToShrink;
     return NewNumItems;
    }
  }
// Resize number of units 
 usize UnitsWhole  = NewNumItems / ItemsPerUT;             // TODO: Optimize for 1-bit items?
 usize ItemsExtra  = NewNumItems % ItemsPerUT;
 usize UnitsNeeded = UnitsWhole + bool(ItemsExtra);

 this->ItmFree  = ItemsPerUT - ItemsExtra;
 this->NumItems = NewNumItems;
 if(ssize res = this->UnitArr.Resize(UnitsNeeded);res < 0)return res;
 return NewNumItems;  
}
//-----------------------------------------------------
sint Grow(ssize numItems)  // Grow or Shrink by numItems
{
 if((numItems < 0) && (-numItems >= this->NumItems)){this->Clear(); return 0;}   // Shrink to Empty
 return this->Resize(this->NumItems + numItems);
}
//-----------------------------------------------------
VT Get(usize at)
{
 usize  utIdx;
 uint32 bitPos;
 if(!this->GetPosition(at, utIdx, bitPos))return 0;
 return this->ValFromUnit(this->GetUnit(utIdx), bitPos);
}
//-----------------------------------------------------
// Set an item to 1
bool Set(usize at)
{
 usize   utIdx;
 uint32  bitPos;
 if(!this->GetPosition(at, utIdx, bitPos))return false;
 this->SetUnit(this->GetUnit(utIdx) | MakeUnit((VT(1) << ItmLen)-1, bitPos), utIdx);    // Sets bits from the 'val' at position (Does logical OR, no masking)
 return true;
}
//-----------------------------------------------------
// Set an item to 0
bool Clr(usize at)
{
 usize   utIdx;
 uint32  bitPos;
 if(!this->GetPosition(at, utIdx, bitPos))return false;
 this->SetUnit(this->GetUnit(utIdx) & ~MakeMask(bitPos), utIdx);    // Sets bits from the 'val' at position (Does logical OR, no masking)
 return true;
}
//-----------------------------------------------------
// Puts the specified value
bool Put(const VT val, usize at)  
{
 usize   utIdx;
 uint32  bitPos;
 if(!this->GetPosition(at, utIdx, bitPos))return false;
 this->SetUnit((this->GetUnit(utIdx) & ~MakeMask(bitPos)) | MakeUnit(val, bitPos), utIdx);    // Sets bits from the 'val' at position (Does logical OR, no masking)
 return true;
}  
//-----------------------------------------------------
// Apply bit operation   // NOTE: It is not cheap to return a previous value(~,&,>>) and it is not needed in most cases
bool Mod(const VT val, usize at, EBitOp op)
{
 usize   utIdx;
 uint32  bitPos;
 if(!this->GetPosition(at, utIdx, bitPos))return false;
 if(op == boOr)this->SetUnit(this->GetUnit(utIdx) | MakeUnit(val, bitPos), utIdx);   // Masking is excessive for OR
 UT mask   = MakeMask(bitPos);    
 UT oldVal = this->GetUnit(utIdx);
 UT newVal = SBitLogic<UT>::ModBit(op, oldVal, MakeUnit(val, bitPos)) & ~mask;      // No masking of 'val' - it should not contain any extra bits  
 this->SetUnit((oldVal & mask) | newVal, utIdx);       
 return true;  
}
//-----------------------------------------------------
bool Add(const VT val)
{
 usize   utIdx;
 uint32  bitPos;
 if(!this->GetPosition<true>(this->Count(), utIdx, bitPos))return false;    // !!!!!!!!!!!!!
 this->SetUnit((this->GetUnit(utIdx) & ~MakeMask(bitPos)) | MakeUnit(val, bitPos), utIdx);         // No masking of 'val' - it should not contain any extra bits
 return true;
}
//-----------------------------------------------------
bool IsZero(void)  // true if all 0
{
 usize TotItems   = this->Count();
 if(!TotItems)return true;
 usize UnitsWhole = TotItems / ItemsPerUT;
 usize ItemsExtra = TotItems % ItemsPerUT;
 for(usize idx=0; idx < UnitsWhole; idx++)
   if(this->UnitArr[idx])return false;
 if(ItemsExtra)
  {
   UT last  = SetUnitFreeSpaceTo<false>(this->GetUnit(UnitsWhole), ItemsPerUT-ItemsExtra);
   if(last)return false;
  }
 return true;
}
//-----------------------------------------------------
bool IsOne(void)   // True if all 1
{
 usize TotItems   = this->Count();
 if(!TotItems)return false;
 usize UnitsWhole = TotItems / ItemsPerUT;
 usize ItemsExtra = TotItems % ItemsPerUT;
 for(usize idx=0; idx < UnitsWhole; idx++)
   if(this->UnitArr[idx] != UT(-1))return false;
 if(ItemsExtra)
  {
   UT last  = SetUnitFreeSpaceTo<true>(this->GetUnit(UnitsWhole), ItemsPerUT-ItemsExtra);
   if(last != UT(-1))return false;
  }
 return true;
}
//-----------------------------------------------------
// Set a range of items to 1
bool Set(usize From, usize Num)
{
 return this->Fill(VT(-1), From, Num);
}
//-----------------------------------------------------
// Clear a range of items to 0
bool Clr(usize From, usize Num)
{
 return this->Fill(VT(0), From, Num);
}
//-----------------------------------------------------
bool Fill(const VT val, usize From, usize Num)        
{
 if(!Num)return true;   // Nothing to do
 usize To = From + Num;
 
 usize  FstUtIdx,  LstUtIdx;
 uint32 FstBitPos, LstBitPos;
 
 if(!this->GetPosition(From,   FstUtIdx, FstBitPos))return false;
 if(!this->GetPosition(To - 1, LstUtIdx, LstBitPos))return false;

 uint32 BitPos, BitCnt;
 if(FstUtIdx == LstUtIdx)  // Range is within a single unit
  {
   UnitItemRangeToBits<true,true>(FstBitPos, LstBitPos, BitPos, BitCnt);
   this->SetUnit(FillUnit(this->GetUnit(FstUtIdx), val, BitPos, BitCnt), FstUtIdx);
   return true;
  }

 usize UnitRange = LstUtIdx - FstUtIdx;    // Range spans multiple units  
 UnitItemRangeToBits<true,false>(FstBitPos, 0, BitPos, BitCnt);   
 this->SetUnit(FillUnit(this->GetUnit(FstUtIdx), val, BitPos, BitCnt), FstUtIdx);    // Fill first unit      
 UT ValUnit = FillUnit(0, val, 0, BitsPerUT);
 while(++FstUtIdx < LstUtIdx)this->SetUnit(ValUnit, FstUtIdx);
 UnitItemRangeToBits<false,true>(0, LstBitPos, BitPos, BitCnt);
 this->SetUnit(FillUnit(this->GetUnit(LstUtIdx), val, BitPos, BitCnt), LstUtIdx);    // Fill last  unit      
 return true;
}
//-----------------------------------------------------
// Returns the matching unit position, negative if not found. 
// Start looking from unit at 'from'
/*ssize Find(const VT val, usize from=0)
{
 usize TotItems = this->Count();
 for(usize idx = from; idx < TotItems; idx++) 
  if(this->Get(idx) == val)return ssize(idx);
 return -1;  // Not found
}
//-----------------------------------------------------
ssize RFind(const VT val, usize from=Count()-1)
{
 if(!this->Count())return -1;
 for(ssize idx = ssize(from); idx >= 0; idx--)
  if(this->Get(idx) == val)return idx;
 return -1; // Not found
}  */
//-----------------------------------------------------
ssize ToStream(auto* strm, usize size=usize(-1), usize from=0)     // TODO: Fast path
{
 return 0;
}
//-----------------------------------------------------
// Complicated:
//   CountItems  at     // Forward/backward
//-----------------------------------------------------
//---------------------------------------------------------------------------
// Bidirectional iterator for efficient traversal
/*class SIterator
{
 CBitArr* arr;        // When you declare a nested class inside CBitArr, the name CBitArr inside that nested class automatically refers to the enclosing template instantiation.
 ssize    curUtIdx;   // Current unit index (signed for reverse iteration)???  (< 0 if the iterator is invalid)
 usize    curAbsIdx;  // Current absolute item index (signed for reverse)???
 UT       curUnit;    // Cached current unit value
 uint32   curItmPos;  // Current item position within unit (0 to ItemsPerUT-1)
 
 //------------------------------------------------------ 
 _finline void CopyFrom(const SIterator* itr)
 {

 }
//------------------------------------------------------ 
 _finline void Reset(void)      // Makes the iterator invalid  ('At' can restore it)
 {
  this->curAbsIdx = this->curUtIdx  = -1;    // invalid
  this->curUnit   = this->curItmPos = 0;
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
 SIterator(CBitArr* a, ssize idx) : arr(a), curAbsIdx(idx)
 {
  if(idx >= 0 && usize(idx) < arr->Count())
  {
   this->curUtIdx  = idx / ItemsPerUT;
   this->curItmPos = idx % ItemsPerUT;
   this->curUnit   = this->arr->GetUnit(this->curUtIdx);
  }
  else if(idx >= ssize(this->arr->Count()))  // End iterator
  {
   this->curUtIdx  = this->arr->UnitArr.Count();
   this->curItmPos = 0;
   this->curUnit   = 0;
  }
  else  // Before begin (idx < 0)
  {
   this->curUtIdx  = -1;
   this->curItmPos = 0;
   this->curUnit   = 0;
  }
 }
 //------------------------------------------------------ 
 inline Ty* At(size_t ItmIdx)      // Set the position in items and retrieves one from it
 {

 }
 //------------------------------------------------------ 
 // Dereference - extract value from cached unit
 VT operator*() const
 {
  uint32 bitPos;
  if constexpr(IsBitOrderMSB) bitPos = (BitsPerUT - ItmLen) - (this->curItmPos * ItmLen);
    else bitPos = this->curItmPos * ItmLen;
  return this->arr->ValFromUnit(this->curUnit, bitPos);
 }
 //------------------------------------------------------
 // Pre-increment (forward)
 SIterator& operator++()
 {
  this->curAbsIdx++;
  this->curItmPos++;
  if(this->curItmPos >= ItemsPerUT)  // Move to next unit
  {
   this->curItmPos = 0;
   this->curUtIdx++;
   if(this->curUtIdx < ssize(this->arr->UnitArr.Count()))this->curUnit = this->arr->GetUnit(this->curUtIdx);   
  }
  return *this;
 }
 //------------------------------------------------------
 // Pre-decrement (backward)
 SIterator& operator--()
 {
  this->curAbsIdx--;
  if(this->curItmPos == 0)  // Move to previous unit
  {
   this->curUtIdx--;
   if(this->curUtIdx >= 0)
   {
    this->curItmPos = ItemsPerUT - 1;
    this->curUnit   = this->arr->GetUnit(this->curUtIdx);
   }
  }
  else this->curItmPos--;
  return *this;
 }
//------------------------------------------------------ 
// Postfix
 _finline SIterator operator++(int) { SIterator tmp = *this; ++(*this); return tmp; }     
 _finline SIterator operator--(int) { SIterator tmp = *this; --(*this); return tmp; }    

 //_finline operator bool() const { return (bool)this->CPtr; }   // Checks if this iterator is valid/not reached out-of-bounds 

 // Equality comparison
 friend _finline bool operator== (const SIterator& a, const SIterator& b) { return return a.curAbsIdx == b.curAbsIdx; }   // bool operator==(const Iterator& other) const
 friend _finline bool operator!= (const SIterator& a, const SIterator& b) { return a.curAbsIdx != b.curAbsIdx; }                               // bool operator!=(const Iterator& other) const

 // Get current absolute index
 usize Index() const { return curAbsIdx; }
 //-----------------------------------------------------
 inline operator usize(void) const             // Get the position in objects
 {
  return this->Index();  //this->EIdx + (this->CPtr - this->BPtr);
 }
 //-----------------------------------------------------
 inline void operator= (const SIterator& itr)
 {
  MOPR::CopyObj(this, itr);
 }
 //------------------------------------------------------
 inline void operator= (usize ItmIdx)    // Set the position in objects      // Fast if in boundaries of the current block
 {
  this->At(ElmIdx);
 }
 //------------------------------------------------------
 inline void operator+= (usize Num)
 {
  //if(!this->CPtr)return;
  Ty* Ptr = this->CPtr + Num;
  if(Ptr >= this->EPtr)this->At(this->Index() + Num);
    else this->CPtr = Ptr;
 }
 //------------------------------------------------------
 inline void operator-= (usize Num)
 {
  //if(!this->CPtr)return;
  Ty* Ptr = this->CPtr - Num;
  if(this->CPtr < this->BPtr)this->At(this->Index() - Num);
    else this->CPtr = Ptr;
 }
 //------------------------------------------------------

};
//-----------------------------------------------------
// Iterator access methods
Iterator begin() { return Iterator(this, 0); }
Iterator end()   { return Iterator(this, this->Count()); }
//-----------------------------------------------------
// Optimized Find using iterator
ssize Find(const VT val, usize from=0)
{
 for(auto it = Iterator(this, from); it != end(); ++it)
  if(*it == val) return it.Index();
 return -1;
}
//-----------------------------------------------------
// Optimized reverse find
ssize RFind(const VT val, usize from)
{
 if(!this->Count()) return -1;
 if(from >= this->Count()) from = this->Count() - 1;
 
 auto endIt = Iterator(this, -1);  // Before beginning
 for(auto it = Iterator(this, from); it != endIt; --it)
  if(*it == val) return it.Index();
 return -1;
}
//-----------------------------------------------------
// Overload for default parameter
ssize RFind(const VT val)
{
 return RFind(val, this->Count() > 0 ? this->Count() - 1 : 0);
}
  */
//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
