
#pragma once

//===========================================================================

//---------------------------------------------------------------------------
enum EPropType: uint8 {ptNone, ptPointer, ptString, ptUint32, ptUint64};

struct SPropDesc    // Which field order is more optimized?    // Empty value is 0
{
 uint32 Id    : 4;    // ID is used to retrieve a specific prop (which all may have the same type)
 uint32 Type  : 4;    // Type is used to find the factory 
 uint32 Index : 24;   // Max 16777215 unique properties of each type in a factory (Need more? Will have to use UINT64)  // Index of
};
//---------------------------------------------------------------------------
class CPropIndex      // Multi-block array of SPropDesc     // Pointer to this will be discoverable from any object through GetCtxInfo of the allocator
{
 struct SPropRef
 {
  uint32    Next;  // Is 0xFFFFFFFF enough? Should it be a relative to current index(Then in SBaseObj it should be relative too, but cannot)?  // To form a per object chain of properties    // 0 Means no next prop  
  SPropDesc Prop;  
 };
 NALC::SBlkAlloc<1024*64,NALC::afGrowLin,SPropRef> Array;      // 1023 to reserve the rest for block headers
//---------------------------------------------------------------------------
uint32 Append(uint32 Index, uint32 Type, SPropRef* Prev)
{
 return 0;
}
//---------------------------------------------------------------------------
}; 
//===========================================================================
// NOTES: 
// - All Enum elements used as bit masks for fast comparision
// - Allocator of IStream should NOT split single allocations across memory blocks!
// - A Entities must NOT contain any objects, only pointers to an objects are allowed!
// - Anything in EFlags may be combined and may be combined with ETypes. Anything in ETypes should be combined only with EFlags 
// Allocation: {SBaseItem+Derived+Alignment},uint32,uint32,{uint32,uint32}
//
template<typename D> struct SBaseItem   // Size: 8 bytes     // Redefine here operators new and delete if there are BUGS in originals
{
 uint16 Type;
 uint16 Flags;      // Limit is 16 mask bits!
 uint32 PropIdx;    // Index of a first property, if any // When an object is created, it created with fixed    // Is 0xFFFFFFFF enough? 
 
//---------------------------------------------------------------------------
void Initialize(uint32 flags)
{
 this->Type    = flags >> 8;
 this->Flags   = (uint16)flags;
 this->PropIdx = 0;
}
//---------------------------------------------------------------------------
template<typename P> P* FindProp(uint8 Id)  // Returns NULL if not found  
{
 return nullptr;
}
//---------------------------------------------------------------------------
template<typename P> P* GetProp(uint8 Id)   // Creates one, if not found
{
 return nullptr;
}
//---------------------------------------------------------------------------
SCtx* GetContext(void)
{
 return nullptr;
}
//---------------------------------------------------------------------------
// TODO: Operator [] for props   
//---------------------------------------------------------------------------
/*D* ToStream(IStream* Strm)            // From stream?
{

}
//---------------------------------------------------------------------------
D* FindSame(IStream* Strm)      // A binary tree will be much faster for duplicate search?
{

} */
//---------------------------------------------------------------------------

public:
SBaseItem(void) = delete; // will never be generated    // Constructors are meaningless here. Those obgects are created in stream and must be gurantieed not to be split
//---------------------------------------------------------------------------
/*D* Clone(IStream* DstStrm)     // TODO: REdirect to its factory
{
 size_t Len = 0; 
 size_t Off = Strm->GetPos();
 Strm->Write(this, this->GetSize());            // Writes entire derived object there
 return (D*)Strm->Data(Off, &Len);
} */
//---------------------------------------------------------------------------
bool IsExist(void){return (bool)this->Type;}  // Type is a bitmask
//---------------------------------------------------------------------------

 bool IsSame(const D* item){if(item == this)return true; else return !memcmp(this,item,sizeof(D));}    // Returns TRUE if equal     // memcmp is Much faster but you must leave no garbage between members of this struct
 bool IsRelated(const D* item){return this->IsSame(item);}   // TODO: Do full relationship check, not just memory compare!

 //void  operator =  (const D &ent) {this->Clone(&ent);}     // Move?
 //void  operator =  (const D *ent) {this->Clone(ent);}
 bool  operator == (const D &ent) {return this->IsSame(&ent);}
 bool  operator == (const D *ent) {return this->IsSame(ent);}

};
//===========================================================================
// NOTES: 
//
struct SIECondition : public SBaseItem<SIECondition>  // <, >, ==, !=, ...
{                  
enum EFlags {fNone=0,            // Ignore FlagMask
             fNot     = 0x0001,  // (Same as Z flag)
             fLess    = 0x0002,
             fGreat   = 0x0004,
             fEqual   = 0x0008,
             fSpecial = 0x0010,  // Some unknow type of condition. The check can be done by a custom analyzer using FlagMask field
};   // Combination
enum ETypes {tNone=0,            // Opposite to 'Not', cannot be used in combination (Same as NZ flag)
             tAnd     = 0x0100,  // Perfom a bitwise AND on FlagMask
             tXor     = 0x0200,  // Perfom a bitwise XOR on FlagMask
             tOr      = 0x0400,  // Perfom a bitwise OR  on FlagMask
             tCompare = 0x0800,  // Compare FlagMask
};   // One of 

uint32 FlagMask;   // Mask of checked flags, must be related to operation`s resulting FlagMask (After bitwise AND must remain unchanged)
//---------------------------------------------------------------------------
static SIECondition* Init(uint32 flags, uint32 fmask=0)       // !!!!!!!!!!!!!!!!!!!!!!!!!!!!! How to avoid adding the same object???????????????????????????
{
 this->Initialize(flags);
 this->FlagMask = fmask;

 /*CStream<uint8, CBufStatic<(sizeof(SIECondition)+(sizeof(uint32)*8))> > TmppObj;
 UINT s_hint  = CountReqSlotsForVal(hint); 
 UINT s_fmask = CountReqSlotsForVal(fmask);
 SIECondition* Temp = SIECondition::Allocate(&TmppObj, s_hint+s_fmask);   
 Temp->Flags  = flags;                                                                 
 Temp->InitValue(&hint,  0, s_hint);            // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&fmask, 1, s_fmask);           // Mask of checked flags, must be related to operation`s resulting FlagMask (After bitwise AND must remain unchanged)   // 32 bits should be enough for most architectures
 SIECondition* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;*/
 return this;
}
//---------------------------------------------------------------------------
size_t GetHint(void){return this->GetValue<size_t>(0);}
//---------------------------------------------------------------------------
uint32 GetMask(void){return this->GetValue<uint32>(1);}       
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//---------------------------------------------------------------------------
void SetMask(uint32 Val){this->SetValue<uint32>(1, Val);}       
//---------------------------------------------------------------------------

};
//===========================================================================
// NOTES:
// - Set 'Numberic' field to some uniqe value  
// - All registers and uniqe memory addresses will be stored as separate SIEOperand instances
//
struct SIEOperand : public SBaseItem<SIEOperand>       // EAX, @R18, #0BE34, [046C7], ...
{                       
enum EOpdFlags {fNone=0,
                fNumber    = 0x0001,   // The 'Numberic' field is valid, use it if needed  (Real number, not a virtual as for register A, for example, and can be used by a code generator)
                fPointer   = 0x0002,   // The Value is used for addressing a memory location (Any assignments must be preserved, not collapsed)  (BitSize is size of data pointed to and need to check if it is part of something(register bank, for example))
                fSigned    = 0x0004,   // Hint from a disassembler, force display the number as signed
                fBitValue  = 0x0008,   // The operand is a bit(s) // For bit addressing set BitSize to number of bits, BitOffs to index of lowest bit and Numberic to identify a memory or register location
                fStack     = 0x0010,   // Stack addressing, use instead of 'Pointer' as a hint for stack tracing
                fLocal     = 0x0020,   // The variable is local for subroutine(Stack, mostly) and can be optimized out, if needed  (Make sence only with a 'Pointer' flag for a stack or memory variables)
                fGlobal    = 0x0040,   // The variable is global (interprocedural) and cannot be optimized out (Make sence only with a 'Pointer' flag for a stack or memory variables)
                fTemporary = 0x0080,   // Must stay invisible in decompiled code an be fully collapsed at the end of decompilation with no assignments left  // The parser may declare a Temp variables for suboperations
                fForceKeep = 0x0100,   // If the variable assignment has been done the always keep it and use for linking, do not collapse onto expressions
//                fFullHEX   = 0x0200,   // Hint from a disassembler, display as fill sized HEX value, don`t trim to highest nonzero byte (0x000000FF instead 0xFF)
//                fHexFormat = 0x0400,   // Hint from a disassembler, force display the number as HEX
//                fDecFormat = 0x0800,   // Hint from a disassembler, force display the number as DEC  
};   // Combination
enum EOpdTypes {tNone=0,               // Not present ??? Use 'Excluded' flag instead
                tStatic    = 0x1000,   // The name is just copied unchanged, no collapsing used  (i.e String)
                tConstant  = 0x2000,   // Can be used in operations but cannot be a destination operand
                tVariable  = 0x4000,   // Can be used in operations and be a destination operand
};   // One of

//---------------------------------------------------------------------------
static SIEOperand* Create(IStream* Strm, UINT flags, size_t hint, uint64 numb=0, uint32 size=0, uint32 offs=0)
{
 CStream<uint8, CBufStatic<(sizeof(SIEOperand)+(sizeof(uint32)*8))> > TmppObj;
 UINT s_OpTr = 0;
 UINT s_hint = CountReqSlotsForVal(hint); 
 UINT s_numb = CountReqSlotsForVal(numb);
 UINT s_size = CountReqSlotsForVal(size);
 UINT s_offs = CountReqSlotsForVal(offs);
 if(flags & fBitValue)s_OpTr = sizeof(void*) / sizeof(uint32);
 SIEOperand* Temp = SIEOperand::Allocate(&TmppObj, s_hint+s_numb+s_size+s_offs+(s_OpTr*3));   
 Temp->Flags = flags;                                                              
 Temp->InitValue(&hint, 0, s_hint);           // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&numb, 1, s_numb);           // Used for a operands distinction when collapsing, must be uniqe for same operands (register numbers or memory locations) // Faster, than comparing by names  // For registers this is a Virtual Offset in register bank, real or not
 Temp->InitValue(&size, 2, s_size);           // Size in bits of a numberic operand. Used for assignments size control and register access (Helps treat AL as part of EAX, for example)
 Temp->InitValue(&offs, 3, s_offs);           // Offset of lowest bit of operand inside of Parent operand type
 if(flags & fBitValue)
  {
   void* Val = nullptr;
   Temp->InitValue(&Val, 4, s_OpTr);    // Pointer to any first operand contained (overlapped) with current, other accessible trough 'SiblingOp' field (i.e AL in EAX)
   Temp->InitValue(&Val, 5, s_OpTr);    // Parent operand pointer, i.e. EAX for AL or AH   // !!! Do relationchip analysis BEFORE operand collapsing  // TODO: Operand relationchip !!!!!!!!!!!!
   Temp->InitValue(&Val, 6, s_OpTr);    // i.e. AH for AL // All 3 pointers must be assigned(If needed) after all operands created for a target machine  // !!! Or just check later bit overlapping (BitSize,BitOffs) which must be set appropriate for registers and calculated later for a memory/array addresses?
  }
 SIEOperand* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;
}
//---------------------------------------------------------------------------
size_t      GetHint(void){return this->GetValue<size_t>(0);}
//---------------------------------------------------------------------------
uint64      GetNumberic(void){return this->GetValue<uint64>(1);}
//---------------------------------------------------------------------------
uint32      GetBitSize(void){return this->GetValue<uint32>(2);}  
//---------------------------------------------------------------------------
uint32      GetBitOffs(void){return this->GetValue<uint32>(3);}
//---------------------------------------------------------------------------
SIEOperand* GetChildOp(void){return this->GetValue<SIEOperand*>(4);}
//---------------------------------------------------------------------------
SIEOperand* GetParentOp(void){return this->GetValue<SIEOperand*>(5);}
//---------------------------------------------------------------------------
SIEOperand* GetSiblingOp(void){return this->GetValue<SIEOperand*>(6);}
//--------------------------------------------------------------------------- 
//---------------------------------------------------------------------------
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//---------------------------------------------------------------------------
void SetNumberic(uint64 Val){this->SetValue<uint64>(1, Val); }  // Cannot set value wider than original 
//---------------------------------------------------------------------------
void SetBitSize(uint32 Val){this->SetValue<uint32>(2, Val);}       
//---------------------------------------------------------------------------
void SetBitOffs(uint32 Val){this->SetValue<uint32>(3, Val);}       
//---------------------------------------------------------------------------
void SetChildOp(SIEOperand* Val){this->SetValue<SIEOperand*>(4, Val);}
//---------------------------------------------------------------------------
void SetParentOp(SIEOperand* Val){this->SetValue<SIEOperand*>(5, Val);}
//---------------------------------------------------------------------------
void SetSiblingOp(SIEOperand* Val){this->SetValue<SIEOperand*>(6, Val);}
//---------------------------------------------------------------------------
bool IsGlobal(void){return (this->Flags & fGlobal);}
bool IsVariable(void){return (this->Flags & tVariable);}
bool IsPointerTo(void){return (this->Flags & fPointer);}
bool IsMustKeepDef(void){return (this->Flags & (fGlobal|fPointer|fForceKeep));}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
// NOTES:
// - A conditional operations, separate from a conditions checks and usage of various flags for conditions make impossible to correctly decompile such operations in some situations.
//
struct SIEOperation : public SBaseItem<SIEOperation>      // +, -, =, *, ...
{                 
enum EFlags {fNone=0,          
             fModFlags = 0x0001,   // Changes the machine specific flags (FlagMask is valid)
};   // Combination
enum ETypes {tNone=0,              // Not present, has no meaning and can be skipped by the Decompiler
             tStatic   = 0x0100,   // The name is just copied unchanged, no collapsing used
             tAssign   = 0x0200,   // Assignment operation (Starting direct assignment, meaning ValA = ValB) Any other operations (opStandart) is mostly mean ValA = ValX op ValY
             tStandart = 0x0400,   // A standart operation (+,-,*,...) with Left and Right operands, UHint contains info for a Code Generator about it
             tSpecial  = 0x0800,   // Has no meaning for the Decompiler, but not same as 'opNone'.  Use it for a special uniqe operations
             tJump     = 0x1000,   // Jump to address or index
             tCall     = 0x2000,   // Subroutine call, the arguments list must be determined after decompilation of this(all?) subroutine
             tReturn   = 0x4000,   // Return from the subroutine (Take Value from Right value of top nearest 'otTempVar' assignment)
};   // One of 
  
//---------------------------------------------------------------------------
static SIEOperation* Create(IStream* Strm, UINT flags, size_t hint, uint32 fmask=0)
{
 CStream<uint8, CBufStatic<(sizeof(SIEOperation)+(sizeof(uint32)*8))> > TmppObj;
 UINT s_hint  = CountReqSlotsForVal(hint); 
 UINT s_fmask = CountReqSlotsForVal(fmask);
 SIEOperation* Temp = SIEOperation::Allocate(&TmppObj, s_hint+s_fmask);   
 Temp->Flags  = flags;                                                             
 Temp->InitValue(&hint,  0, s_hint);            // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&fmask, 1, s_fmask);           // Resulting FlagMask of the operation  (Possibly Affected flags)
 SIEOperation* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;
}
//---------------------------------------------------------------------------
size_t GetHint(void){return this->GetValue<size_t>(0);}
//---------------------------------------------------------------------------
uint32 GetMask(void){return this->GetValue<uint32>(1);}       
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//---------------------------------------------------------------------------
void SetMask(uint32 Val){this->SetValue<uint32>(1, Val);}       
//---------------------------------------------------------------------------

};
//===========================================================================
// Formats: LeftValue Operation RightValue
//          LeftValue
// 
// NOTES:
// - Operands with a different numberic values(constants and addresses) can result in creation of large amount of SIEInstruction objects
//
struct SIEInstruction : public SBaseItem<SIEInstruction>
{                           
enum EFlags {fNone=0,
             fSOpIndex   = 0x0001,    // Operand of jump is actually index of SubOperation not an address (To one address can be assigned whole list of suboperations)
             fBranDirect = 0x0002,    // The instruction is a direct Jump/Call (Any conrol transfer instruction where target address may be calculated from the operands) // This flag saves from analyzing argument flags
             fBreakFlow  = 0x0004,    // The instrucrion Exits from the control flow (To unknow direction with no return if this is not a RET operation)  // Any RET or decided by the Analyzer/User
             fNotSep     = 0x0008,    // Hint from a disassembler, do not separate by spaces Left and Right operand from the Operation 'Value' (Used for some Static operands)
};
enum ETypes {tNone=0};     

enum EProps {pNone,
             pHint,        // Optional, for debugging
             pOper,        // The operation descriptor  // Read only, refers to existing    // No ref counting? All entities must be kept?
             pCond,        // A condition descriptor
             pResOp,       // A list of affected operands (registers, memory)
             pSrcOp,       // A list of source operands (Left, Right, ...)
}; 
//---------------------------------------------------------------------------
static SIEInstruction* Create(uint32 flags)
{
 this->Flags = flags;
 this->Type  = flags >> 16;

 CStream<uint8, CBufStatic<(sizeof(SIEInstruction)+(sizeof(uint32)*8))> > TmppObj;
 UINT s_hint  = CountReqSlotsForVal(hint); 
 UINT s_oper  = CountReqSlotsForVal(oper);
 UINT s_cond  = CountReqSlotsForVal(cond);
 UINT s_dest  = CountReqSlotsForVal(dest);
 UINT s_left  = CountReqSlotsForVal(left);
 UINT s_right = CountReqSlotsForVal(right);                                                           
 SIEInstruction* Temp = SIEInstruction::Allocate(&TmppObj, s_hint+s_oper+s_cond+s_dest+s_left+s_right);  
 Temp->Flags  = flags;                                                                 
 Temp->InitValue(&hint,  0, s_hint);            // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&oper,  1, s_oper);            // Current operation description
 Temp->InitValue(&cond,  2, s_cond);            // Condition for this operation, if used // Executed only if the condition match (Just take Left and Right values from top nearest operation with 'ModFlags', no other solution exist!)
 Temp->InitValue(&dest,  3, s_dest);            // Pointer to a Destination operand instance (i.e OpDest-OpLeft-Operation-OpRight is 'A = B + C' or 'A = A + B')
 Temp->InitValue(&left,  4, s_left);            // Pointer to a Left operand instance
 Temp->InitValue(&right, 5, s_right);           // Pointer to a Right operand instance (i.e. NULL for assignments like A = 5)
 SIEInstruction* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;
}
//...........................................................................
size_t        GetHint(void){return this->GetValue<size_t>(0);}
//...........................................................................
SIEOperation* GetOper(void){return this->GetValue<SIEOperation*>(1);}      // Using pointers will increase memory consumption on x64 and harder to serialize 
//...........................................................................
SIECondition* GetCond(void){return this->GetValue<SIECondition*>(2);}
//...........................................................................
SIEOperand*   GetDest(void){return this->GetValue<SIEOperand*>(3);}
//...........................................................................
SIEOperand*   GetLeft(void){return this->GetValue<SIEOperand*>(4);}
//...........................................................................
SIEOperand*   GetRight(void){return this->GetValue<SIEOperand*>(5);}
//...........................................................................
//...........................................................................
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//...........................................................................
void SetOper(SIEOperation* Val){this->SetValue<SIEOperation*>(1, Val);}       
//...........................................................................
void SetCond(SIECondition* Val){this->SetValue<SIECondition*>(2, Val);}
//...........................................................................
void SetDest(SIEOperand* Val){this->SetValue<SIEOperand*>(3, Val);}
//...........................................................................
void SetLeft(SIEOperand* Val){this->SetValue<SIEOperand*>(4, Val);}
//...........................................................................
void SetRight(SIEOperand* Val){this->SetValue<SIEOperand*>(5, Val);}
//...........................................................................

};
//===========================================================================
//
class CInstruction : public SBaseItem<CInstruction>
{
 ULONG Index;                   // Sequental index, must be uniqe (Used in a suboperational jumps calculation) // Has no meaning for analysis and may become unsorted   // Useful for debugging - graph dumping
 ULONG Address;                 // Must be uniqe (Used in an operational jumps calculation)  // Has no meaning for analysis and may become unsorted   // Useful for debugging - graph dumping
 ULONG InstrOffs;               // Instruction instance offset (SIEInstruction)

};
//===========================================================================
