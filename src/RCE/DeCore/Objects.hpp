
//===========================================================================
SCVR size_t InvalidIdx = (size_t)-1;    // Can't be uint64 on x32
template<typename T> static bool _finline IsValidIndex(T idx){return ~idx;}   // Invalid is -1

#pragma pack(push, 1)
// Used to find specific list class and number of them and total number of them (Max 256 items per list)   
// Or just to store 1-4 extra props (takes space of 2 inplace props)
//
template<typename LT, uint32 LN> struct __attribute__ ((packed)) UPListDesc    // Should be packed  // We really need some sort of in place 'packas'
{
 LT      BaseIndex;       // List1  // Index in global RefsList
 uint8   ListSizes[LN];   // List2 From FirstIndex;  List3 from List2;  List4 from List3;  ListEnd from List4
 //uint64  Value;            // This allows to easily memcmp an entire list    // Makes alignment requirement to be 8 bytes?

//------------------------------------
 LT GetSubList(uint32 idx, uint32& Size)      // No bounds check!    // Must be inlined
 {
  LT CurIdx  = this->BaseIndex;
  uint32 ctr = 0;
  for(;ctr < idx;ctr++)CurIdx += this->ListSizes[ctr];    // NOTE: Should be unrolled
  Size = this->ListSizes[ctr]; 
  return CurIdx;
 }
//------------------------------------
//Used to get range of several sub list to interpret them as one
//
 uint32 GetSize(uint32 from, uint32 to)      // No bounds check!    // Must be inlined
 {
  uint32 CurLen = 0;
  uint32 ctr = from;
  for(;ctr <= to;ctr++)CurLen += this->ListSizes[ctr];    // NOTE: Should be unrolled
  return CurLen;
 }
}; 
#pragma pack(pop)
//---------------------------------------------------------------------------
// NOTE: The Item size should be large enough to fit an item index (single linked list of deleted items)
// TODO: Make sure it is non-blocking and thread safe
// The array can allocate N requested items but only indexes are guranteed to be sequential not the items themselves
// When deleting N items the group does to bucket for that N.  
//
template<typename T, uint32 MaxLBkt=16, uint32 IdxLen=sizeof(uint32), uint32 Flags=0> class SSeqArr          // Use MaxLBkt=0 to meke it a single item array
{
 using TI = TSV<IdxLen <= sizeof(uint32),uint32,size_t>::T;        // No 64-bit indexes on x32 platforms!
 static_assert(sizeof(T) >= (sizeof(TI)*(1+(bool)MaxLBkt)));  // Need two slots for bucket 0 in ItemList mode
 struct SBucket {TI Bucs[256];}


 NALC::SBlkAlloc<65536,NALC::afGrowLin,T> Array;
 NALC::SBlkAlloc<4096, NALC::afGrowUni,SBucket> Buckets;    // Buckets of groups of deleted items to avoid enumeration when reusing
 TI EndIndex;
 TI DelChain[MaxLBkt+1];    // Buckets of items with contiguous indexes    // Bucket 0 is for all other sizes 
//------------------------------------
TI AllocNew(uint32 Num)     // The indexes will be always sequential
{
 int res = this->Array.Expand((this->EndIndex+Num)-1);      // TODO: Need locking inside SBlkAlloc if allocating a new block  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 if(res < 0)return (TI)InvalidIdx;
 TI idx = this->EndIndex;
 this->EndIndex += Num;     // TODO: Exchange/add and return prev !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 return idx;
}
//------------------------------------
TI AllocOld(uint32 Num)    // From deleted      // TODO: Needs locking   !!!!!!!!!!!!!!!!!!!!!!!!!
{
 if constexpr (MaxBkt)     // Look up as lists of 'num' items
  {
   if(Num > MaxBkt)        // In MISC bucket   // No splitting and moving between buckets, the 'Num' must match exactly  
    {
     TI From   = this->DelChain[0];
     if(!IsValidIndex(From))return (TI)InvalidIdx;   // No deleted lists
     auto iter = this->Array.ElmFrom(From);
     TI*  Last = &this->DelChain[0];
     while(iter)
      {
       T* itm  = *iter;
       TI Next = *(TI*)itm;
       TI ICnt = ((TI*)itm)[1];
       if((uint32)ICnt == Num)  // Found
        {
         *Last = Next;
         return From;  
        }
       Last = (TI*)itm;
       From = Next;
       iter = Next;    // Set next item for the iterator (It will handle block boundaries)
      }
    }
     else  // In size buckets
      {
       TI Idx = this->DelChain[Num];
       if(!IsValidIndex(Idx))return (TI)InvalidIdx;   // No deleted lists for this size
       T* elm = this->Array.GetElm(Idx); 
       if(!elm)return (TI)InvalidIdx;
       this->DelChain[Num] = *(TI*)elm;    // Next becomes First   // Locking needed?
       return Idx;
      }
  }
 else  // Just make sure that we have a sequential chain of 'Num' items (not in memory, indexes only) (Do not expect to find it if 'Num' is variable - fragmentation will occur)  // Best if 'Num' is 1
  {
   TI From   = this->DelChain[0];
   if(!IsValidIndex(From))return (TI)InvalidIdx;
   auto iter = this->Array.ElmFrom(From);
   uint32 Total = 0;
   TI* Last  = &this->DelChain[0];
   while(iter)              // Each item have index of a next item written at its offset 0, even if they are sequential
    {
     T* itm  = *iter;
     TI Next = *(TI*)itm;
     if(++Total >= Num)  // Found enough with sequential indexes
      {
       *Last = Next;
       return From;       // TODO: Test with more than one item!!!!!!!!!!!!!
      }
     if(!IsValidIndex(Next))return (TI)InvalidIdx;   // No more deleted items
     if(Next != (From+1)){Total = 0; From = Next;}   // Reset the sequence
     Last = (TI*)itm;
     iter = Next;    // Set next item for the iterator (It will handle block boundaries)
    }
  }
 return (TI)InvalidIdx;
}
//------------------------------------
public:
//------------------------------------
// Does not guarantees the entire list to be contiguous in the same block(use iterator to access the items)
// When adding 'Num' of items as a list(MaxLBkt != 0), all their indexes will be sequential (to use in UPListDesc)
// In Item mode adds 'Num' separate items with sequential indexes
// The 'Num' is not stored anywhere. You must store it and pass to the iterator request later
// 
TI Add(uint32 Num=1)
{
 TI fidx = this->AllocOld(Num); 
 if(!IsValidIndex(fidx))fidx = this->AllocNew(Num); 
 return fidx;
}
//------------------------------------
// In list mode, deletes a list of 'Num' items which starts at 'From'
// In Item mode, deletes 'Num' items with sequential indexes, starting at 'From'
// Returns number of deleted sequential items. TODO!!!!!!!!!!!!!!!!
//
sint32 Delete(TI From, uint32 Num=1)        // No one is expected to use the item at this point     // Must lock
{
 Num += (bool)!Num;  // Must be at least 1
 if constexpr (MaxBkt)  // Delete a list of 'Num' items
  {
   uint32 Total = Num;
   T* elm = this->Array.GetElm(From); 
   if(!elm)return -1;
   if(Num > MaxBkt)     // Need two index slots      
    {
     ((TI*)elm)[1] = Num;   // Save the list size in second field of its first item
     Num = 0;   // Put into misc group // Assigning default values is faster than using 'else' branch which usually generates additional unconditional JMP instruction  
    }
   *(TI*)elm = this->DelChain[Num];  // = From;  // TODO: thread safe exchange somehow    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   this->DelChain[Num] = From;  // Exchange(*(TI*)elm, this->DelChain[idx])
   return (sint32)Total;
  }
   else  // Delete 'Num' of separate items (With sequential indexes only)
    {
     auto iter = this->Array.ElmFrom(From); 
     if(!iter)return -1;
     TI* Prev  = nullptr;
     T*  itm   = nullptr;
     uint32 Total = 0;
     for(;iter && (Total < Num);Total++)
      {
       itm   = *iter;
       *Prev = (size_t)iter;  // Get its index
       Prev  = (TI*)itm;     
      }
     *(TI*)itm = (TI)InvalidIdx;  // End of chain
     return (sint32)Total;
    }
 return 0;
} 
//------------------------------------
// TODO: Iterator for requested range (and indexes must be sequential in list mode)
// TODO: ToStream, FromStream
};
/*struct SBaseObj
{
 uint16 Type;        // Limits to 16 mask bits!
 uint16 Flags;       // Limits to 16 mask bits!
};
//---------------------------------------------------------------------------
struct SBaseHint     // Not for MicroOperations - waste of memory
{
 uint32 NumHint;     // An User defined Hint value - Can be used as an index of some struct or flags
 uint32 StrHint;     // Index, str 0 is "" // Can be used by a code generator if present (Important: Store this string in some list with a dups exclusion)     // Strings are ref counted but to actually free tthem - index rebuild is needed
}; */
//---------------------------------------------------------------------------
// High byte is used as an index of micro-instruction, if the initial CPU instruction is too complex(Always 0 for jump/call targets)
//
union UIAddress
{
 static_assert(sizeof(TCFG::AddrSize) <= MaxAddrBits);

 SCVR int AMaxWidth = sizeof(uint64) * 8;
 SCVR int RemBits   = AMaxWidth - TCFG::AddrSize;
 SCVR int MILen     = (RemBits > 8)?((RemBits > 9)?8:7):6;  // Min 64 micro instructions(6 bits)
 SCVR int SGLen     = RemBits - MILen;

// NOTE: Check the memory layout, the 'Addr' should be most efficient
 struct {
 uint64 Addr : TCFG::AddrSize;   // The linear address part
 uint64 SSeg : SGLen;            // Software segment index (different linear address spaces) (at least 1 bit)
 uint64 MIdx : MILen;};          // Micro instruction index
 uint64 Value;                   // For fast comparison

 uint64 GetLinAddr(void) {return (this->Addr >> (TCFG::AddrSize - 1)) ? (this->Addr | (uint64(-1) << TCFG::AddrSize)) : this->Addr;}   // Does appropriate sign extension. For printing mostly
};
static_assert(sizeof(UIAddress) == sizeof(uint64));      // Packed, at least for now
//---------------------------------------------------------------------------
union UOAddress
{
 static_assert(sizeof(TCFG::AddrSize) <= MaxAddrBits);

 SCVR int AMaxWidth = sizeof(uint64) * 8;
 SCVR int OffsLen   = TCFG::AddrSize + 3;  // +3 for being a bit offset (used a lot in flag accesses)
 SCVR int BSLen     = 4;    // Max is (1 << 15)   // Cannot be 3, (1 << 7) gives max 128 bit argument size
 SCVR int SGLen     = AMaxWidth - (OffsLen + BSLen);

// NOTE: Check the memory layout, the 'Addr' should be most efficient
 struct {
 uint64 Offs : OffsLen;
 uint64 SSeg : SGLen;            // Software segment index (defined by a disassembler (like tracking of register banks switching?)) (at least 1 bit)
 uint64 BLen : BSLen;};          // Gives the operand size in bits (1 << BLen)  (1,2,4,8,16,32,64,128,256,... bits)   Unlikely there will be instructions that access memory not in pow2 granularity of bits
 uint64 Value;                   // For fast comparison

 uint64 GetLinAddr(void) {uint64 badddr = this->Offs >> 3; return (badddr >> (TCFG::AddrSize - 1)) ? (badddr | (uint64(-1) << TCFG::AddrSize)) : badddr;} 
};
//---------------------------------------------------------------------------
// Big floating point constants are stored in memory. The decompiler can fetch them when necessary, no need for bigger than 64bit operand values directly.
// Access size is required too: 'mov bl, [eax]' uses a register operand of 32 bits for memory access of 8 bits
//   The operand may be a constant but if marked as a pointer should include asscess size too
// Operand Ref is used to attribute an operand usage per instruction. 
//  Anything that can be changed manually later, Like if it is an memory address
//   or to which custom segment it does the memory access.
// Separate flag if a disassembler was sure that the operand is a memory address (like 'mov edx, [eax]')
// Requirement to be able to edit some values prevents operand lists from being deduplicated when storing
// Jump/Call instructions store their targets in Right operand list (Multiple may be added if the transfer is indirect)
//
struct SOperand    // 16 bytes per operand (pow2 lookup is fast!)
{
 union {
 uint64     Value;      // Constants have values directly 
 UOAddress  Address;};  // Registers and flags are mapped to virtual ranges  // What about setting bits at setb '#33, 3'?  This will definently reference a memory by a single bit!!
 uint32     Hint;       // A hint from a dissassembler to a decompiler
 uint16     Flags;
 uint8      SegIdx;     // For pointer types    // Default is 0. Can be changed to split overlapping stuctures (manually)
 uint8      ALen : 4;   // Size of memory access if this operand is a pointer
 uint8      Type : 4;   // Enumeration type of the operand
//-----------------------
SOperand* InitAsVar(uint64 Addr, uint32 Hint, uint16 Flags, uint8 Type, uint8 BitSize, uint8 BitOffs=0, uint8 SSeg=0)
{
 return this;
}
//-----------------------
SOperand* InitAsConst(uint64 Val, uint32 Hint, uint16 Flags, uint8 Type)
{
 return this;
}
//-----------------------
SOperand* MakePtr(uint8 TgtLen, uint8 SSeg)
{
 return this;
}
//-----------------------
SOperand* UndoPtr(void)
{
 return this;
}
//-----------------------
};
static_assert(sizeof(SOperand) == 16);
//---------------------------------------------------------------------------
// Refs and Decompilation extra info 
struct SExtCtx   // 32 bytes 
{
 uint32  EntryIdx;   // For a first instruction of a Entry Point points to that Entry Point info structure (For a fast access to EP by a CALL instructions)
 TLstIdx OpDstRefs;  // The list is the number of references to a destination operand (Result) of this instruction (An assignment instruction can be excluded if Refs is 0)
 TLstIdx BranRefLst; // List of all instructions which referense to this Instruction (JUMPs and CALLs (if this an entry point instruction))
 TLstIdx BranDefLst; // If this instruction is a branch/CALL then the List contains all Definitions at that point (Useful for detecting an arguments of subroutine)   // Loops/switches/backjumps may add definitions and go to different paths with them - handle this somehow (unly unrolling and copy will help?)
 TLstIdx OpTakenLst; // List of instructions from which the associated operands were taken
 uint32  ReservedA;
 uint32  ReservedB;
 uint32  Hint;       // Taken from SInstruction
};
static_assert(sizeof(SExtCtx) == 32);
//---------------------------------------------------------------------------
// All unique micro operations or SysCalls/ExtCalls (same refs)
// Do not expect to feed here some x86 DOS real mode segmented code as is
// All weird memory models must be unwrapped by a disassembler/tracer first into linear 48-56 bit address.
// Jump target is a list. Useful in case of indirect jumps in switches
// If an operand an address or not should be made avaliable to decide later.
//   Because some address wrapping tricks are used by optimizers, especially on x32 systems.
//---------------------------------------------------------------------------
// 
using TOpList = UPListDesc<TOpdIdx,3>;

struct SInstruction  // 32 bytes    // WARNING! Will create an instance for EVERY!! different constant!!!
{
 UIAddress Address;
 TInsIdx   Prev;
 TInsIdx   Next;
 TOpList   OpdLst;     // Args, Rets   // Watch for alignment holes!    // The list is sorted as {Dst,Left,Right}    
 uint8     Type; 
 uint32    Flags;
 union {
 uint32    Hint;          // A hint from a dissassembler to a decompiler
 uint32    ExCtx;};       // Extra context, if needed. The hint is in it then
};
static_assert(sizeof(SInstruction) == 32);
//---------------------------------------------------------------------------
