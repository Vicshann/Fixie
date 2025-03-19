
/*
 https://www.utf8-chartable.de/
 https://en.wikipedia.org/wiki/List_of_Unicode_characters
 https://en.wikipedia.org/wiki/UTF-32
 https://en.wikipedia.org/wiki/Plane_(Unicode)#Basic_Multilingual_Plane
 https://en.wikipedia.org/wiki/Graphic_character
 https://www.techtarget.com/whatis/definition/logic-gate-AND-OR-XOR-NOT-NAND-NOR-and-XNOR

 NOTE: A state machine + utility flags are used
 TODO: Test streams with 4K local and allocated buffer, and a callback for byte reading. Use a callback if not slower for big sources
*/
//============================================================================================================
struct NLEX        // It is shorter than CTokenizer (flags are namespaced)
{
// TODO: Propagate to token stream (STkn) only useful flags
enum EToolFlags {   // Tokenizer flags, not a token flags  // Token flags or range flags?   // Only 8 bits for token flags!   // This is for Range/Token records, not the token stream itself     // TODO: Sort the flags
  tfNone          = 0, 
  tfWhtspc        = 0x00000001,   // This range is a whitespace (At least one empty position is occupied)
  tfIgnore        = 0x00000002,   // Ignore the char range, like ' in numbers. Do not change the state?
  tfNoTerm        = 0x00000004,   // The char range is a part of a token but cannot be last in it
  tfBadToken      = 0x00000008,   // Treat unspecified chars as an error instead of fallback to state 0
  tfScopeOpn      = 0x00000010,   // Both flags mean that same symbols will open and close their scope (i.e.: ' " )   They will use a separate scope stack ???          //rfScopeOpn + rfScopeCse for counted scopes like ''' or '"'   // Note Do not allow scope close if it is not opened
  tfScopeCse      = 0x00000020,   // Can we have nested scopes of same char?  NO!: "1 "2 "3 "?[We want to close the last one]    // So, the flat only: "O "C "O "C
  tfNumeric       = 0x00000040,   // No lexing. Store in a separate list with associated struct of converted values to do conversion only once
  tfTknTerm       = 0x00000080,   // This token terminates/separates an expression (i.e. ';')    // <<<TODO: Remove to type flags ( have meaning only for an AST walker )
// Not transferable to token records
  tfTknRSplit     = 0x00000100,   // Split current token when encountered // Will become part of a next token, not the current one (Not same as tfTknTerm?)
  tfTknLSplit     = 0x00000200,   // Split current token when encountered // Will become part of the current token
  tfRawString     = 0x00000400,   // A string with no escape processing
  tfEscString     = 0x00000800,   // An escaped string
  tfComment       = 0x00001000,   // <<<TODO: Remove to type flags (Looks like no special processing is required)
  tfLSplitOnSDZ   = 0x00002000,   // Split if scope depth of GroupID becomes zero after tfScopeCse (Does tfTknLSplit)
  tfBRstIfSplit   = 0x00004000,   // Reset current state to base if split
  tfScpTknMirror  = 0x00008000,   // This token is mirrored when used as a scope marker (Store the token to compare on closing the scope)  // Format: [Prefix]anything[Postfix] Prefix/Postfix is controlled by ranges and 'anything' is stored on scope stack to compare on close   // C++: R "delimiter( raw_characters )delimiter"     // Delimiter is optional and it can be a character except () / or whitespaces
  tfTknMemStore   = 0x00010000,   // Remember a token accumulated before the current char (type flags accumulation will continue)
  tfTknMemSplit   = 0x00020000,   // Split at a remembered token  // Useful when we can't be sure if the current token should be a token, so the decision is postponed with tfTknMemStore    (AAAAABBBBB => AAAAA, BBBBB)  
  tfScpMrkBegin   = 0x00040000,   // Next char will start a scope marker
  tfScpMrkEnd     = 0x00080000,   // This char ends a scope marker(Not becoming a part of it if not marked with tfScpMrkBegin too)
  tfMemSplitOnSDZ = 0x00100000,   // Split a remembered token if scope depth of GroupID becomes zero after tfScopeCse
  tfOChStateOnSDZ = 0x00200000,   // Only change state if the scope depth of GroupID becomes zero after tfScopeCse
  tfAddTkTypeLeft = 0x00400000,   // Add current char`s type flags to a previous token if splitting (Useful to skip expression separators like ';' and avoid wasting memory for them)
 // tfParseEscape   = 0x00800000,   // Parse some predefined escape sequences starting from a next char   ???????????????????
  tfCharReplace   = 0x01000000,   // Replace the current char     // Any use? (Adds a condition for each char storing)     // !!!!!!!! ?????????????
  tfDepthIndent   = 0x02000000,   // Indentantion with a sequence of those chars sets the scope depth explicitly (Python like scopes). Splits on a next char that does not have this flag and sets Depth value for a next token.
  tfNonSpaced     = 0x04000000,   // This byte is part of a non printable char (Those chars that usually do not take any space in text editors)
};
//============================================================================================================
// TODO: Optionally split on change state to base (R or L ?)     // And split on group change to < or > or both
enum ELexFlags         // For 'ParseFlg'
{
 lfNone,
 lfKeepWhtspc  = 0x01,   // Keep whitespace ranges as tokens                // Not keeping those can save a lot of memory  // Whitespaces in strings and comments are always kept (when tokenizing them)
 lfKeepBraces  = 0x02,   // Keep braces (which are useless for AST)
 lfKeepScpTkn  = 0x04,   // Keep scoping tokens (i.e. braces and brackets)  // Scoping tokens are useful only for printing. Each token record contains scope info   // Not keeping those can save a lot of memory
 lfKeepTrmTkn  = 0x08,   // Keep termination tokens (i.e. ';' in c++)       // Not keeping those can save some of memory
 lfTknStrings  = 0x10,   // Tokenize strings (Spams the Lexer)
 lfTknComments = 0x20,   // Tokenize comments (Spams the Lexer)
};
//------------------------------------------------------------------------------------------------------------
enum ETLErrors     // And warnings: tlw*
{
 leNoMoreData=1,     // No more data in the stream (continuable)
 leBadChar,          // The char is declared inappropriate at this place
 leUnexpChar,        // The char is in unregistered range
 leUnexpEOF,         // Unexpected end of file (Incomplete token)
 leScopeTooDeep,     // Max scope depth reached for a specfic scope type
 leUnexpScpClse,     // Closing a scope that was not opened before                // Unexpected brace
 leWrongScpClse,     // Closing a scope that was not opened at this depth level   // Wrong brace type
 leUnclosedScpAtEOF, // There are some unclosed scopes at EOF
 leChrCantBeLast,    // The char is forbidden to be last in the token
 leScpMrkMismatch,   // Scope marker mismatch
 leCallback,         // Some error code, returned by a callback function (Must be last here, create your codes starting from it)
};
//============================================================================================================
struct SPos     // usize is max 4Gb on x32. Should it be able to handle more than 4Gb on x32? 
{
 usize Offs;    // Source offset (In bytes)
 usize Size;    // Size of the token (In bytes)
 usize COffs;   // Source offset (In chars)
 usize CSize;   // Size of the token (In chars)
 usize Pos;     // Position on the line       // An entire source file could be a single line, this is normal
 usize Line;    // Line of a source file      // One token stream per file 

_finline SPos(void){this->Reset();}
_finline SPos(const SPos* pos){this->Set(pos);}
_finline SPos(usize line, usize pos, usize offs, usize len, usize coffs, usize clen){this->Set(line, pos, offs, len, coffs, clen);}

void _finline Set(const SPos* pos){*this = *pos;}
void _finline Set(usize line, usize pos, usize offs, usize len, usize coffs, usize clen)
{
 this->Offs  = offs;
 this->Size  = len;
 this->COffs = coffs;
 this->CSize = clen;
 this->Pos   = pos;
 this->Line  = line;
}
void _finline Reset(void){Offs=Size=COffs=CSize=Line=Pos=0;}
};
//============================================================================================================
struct STknInf  // Base token description struct for 'ProcessToken'
{
 SPos   Pos;
 uint32 Offs;  // In the buffer                                             
 uint32 EPos;  // Actual size of the token (In the buffer) is EPos - Offs
 uint32 CSize; // Size in chars (In the buffer)
 uint32 Types; // Accumulated, Set/Clr controlled
 uint32 Group; // The Last one assignment will be used
 uint32 Flags; // The Last one assignment will be used

uint32 _finline Size(void){return EPos - Offs;}      // In bytes
void   _finline Reset(void){Offs=EPos=CSize=Types=Group=Flags=0;Pos.Reset();}
};
//============================================================================================================
struct SErrCtx
{ 
 uint32  Size;    // in the token buffer
 uint32  Offs;    // in the token buffer
 sint32  Code;
 uint32  State;
 uint32  Extra;
 SPos    CurPos;
 SPos    PrvPos;  // For scopes
//----------------------------------
sint Set(sint32 code, uint32 state, uint32 vlen, uint32 voffs, auto&& cpos, auto&& ppos, uint32 extra=0)   // Had to use auto instead of SPos to accept L and R refs
{
 this->Size   = vlen;
 this->Offs   = voffs;
 this->Code   = code;
 this->State  = state;
 this->Extra  = extra;
 this->CurPos.Set(&cpos);
 this->PrvPos.Set(&ppos);
 return -code;     // Prepare it to be returned as an error code
}
//----------------------------------
void Reset(void){Code=Size=Offs=State=Extra=0;CurPos.Reset();PrvPos.Reset();}  //  memset(this,0,sizeof(*this));}    // !!! My memset is BAD !!!
//----------------------------------
}; 
//============================================================================================================
//                                      Byte Range implementation
//------------------------------------------------------------------------------------------------------------
// Main container of states for the Tokenizer
// Can be used to gather number constants, including complex ones, like floats
// Not useful for scope counting because scopes can be defined by a simple words like 'begin' and 'end' in Pascal (Register keywords with the Lexer for that)
//
struct SRangeLst              // Shared with all tokenizing instances
{
 SCVR uint MaxRanges = 256;   // Max unique ranges for all states   // Using uint8 for RangeMap saves a lot of memory  // (Cannot be changed without breaking everything (for now))

struct SState                 // Indexed by a 'State' into StateArr // Even with only one range per state we can have max 256 states. More ranges - less states possible because the range list is global(To index it by a byte from RangeMap)
{
 uint8 RangeMap[MaxRanges];   // Stores indexes into RangeArr. Indexed by a 'Char'  // 0 is for an empty slot (max 255 ranges)  // Range indexes for chars  // Max 256 ranges (Meaning that we can declare each char separatedly)
};

struct SRange
{
 uint8  First;      // First byte in range (UTF-8 is split in multi-depth byte ranges)
 uint8  Last;       // Last byte in range
 uint8  Repl;       // Replace with this value if required
 uint8  State;      // Max 256 states (Max uint8 value 00-FF), 64K memory (Allocated by 4K?)
 uint32 Group;      // Useful for Scopes (Max 256 scope types)   // Actual group value is uint8, higher bytes are arbitrary
 uint32 Flags;      // ETokenFlags  // By default all unassigned char ranges cause fallback to state 0. Invalid char ranges must be defined to break parsing if necessary 
 uint32 FlgSet;     // Set (OR) the accumumulated token flags for the Lexer (Low part)
 uint32 FlgClr;     // Clear(~AND) the accumumulated token flags for the Lexer (Low part)
 
 bool IsValid(void){return (First|Last);}  // 0 char is invalid
};

 CArr<SRange> RangeArr;    // TODO: Optimize allocations    // Max is MaxRanges. Indexes are in RangeMap
 CArr<SState> StateArr;    // A map to find SRange by char index and not by search in RangeArr by State+First+Last // TODO: Optionally disable to save some memory (RangeArr needs to be sorted by State)
//------------------------------------------------------
SRangeLst(void)
{
 SRange rec{};
 this->RangeArr.Append(&rec);  // Add an empty zero-idx range which will mark entire range as unexpected chars 
}
//------------------------------------------------------
// Type is flags that accumulated by 'Parse' loop for current token
// NOTE: State, Group, Type are arbitrary user defined values
// Can add additionally 3 nonbase target states
// TgtStates (states to which to add this range) allows up to 8 target states. (Because each state must have a copy of affected char range)
// Check return value explicitly as -1 in case of an error if using multiple target states
// Actual group value is uint8, higher bytes are arbitrary and can be used as custom flags or IDs
// FlgSet is the type to w
//
sint64 Add(uint64 TgtStates, uint16 First, uint8 Last, uint8 NextState, uint32 Group=0, uint32 FlgSet=0, uint32 FlgClr=0, uint32 Flags=0)    // NOTE: Replacement char is shifted <<8 in First
{
 SRange rng{.First=(uint8)First, .Last=Last, .Repl=uint8(First>>8), .State=NextState, .Group=Group, .Flags=Flags, .FlgSet=FlgSet, .FlgClr=FlgClr};
 uint64 status = 0;
 do
  {
   sint res = this->Add(rng, (uint8)TgtStates);
   status <<= 8;     // Last return value will be lowest
   status  |= (uint8)res;
   if(res < 0)break;
   TgtStates >>= 8;
  }
   while(TgtStates);
 return (sint64)status;
}
//------------------------------------------------------------------------------------------------------------
sint Add(SRange& rec, uint8 TgtState)      // Returns range index(Can change after next add because of sorting)
{
 sint res = this->RangeArr.Count();     // Max is 254
 if(res >= (sint)MaxRanges)return -1;   // Already full   // At least one free slot is required
 this->RangeArr.Append(&rec);
 SRange* ranges = this->RangeArr.Data();       // Why there is a warning that Ranges overshadows the CTokenizer`s "Ranges" member variable if it is not static and not accessible as a function could be? 
 if(uint cnt=this->StateArr.Count();TgtState >= cnt)    // Make sure that states up to 'TgtState' are allocated    // TODO: Precalculate and memset as a single block
  {
   uint num = (TgtState - cnt) + 1;
   this->StateArr.Append(nullptr, num);
   memset(&this->StateArr.Data()[cnt],0,num*sizeof(SState));   // ??? required?  // The array should handle zero initialization?
 }
 uint8* RangeMap = this->StateArr.Data()[TgtState].RangeMap;
 for(uint idx=rec.First;idx <= rec.Last;idx++)    // Add in array and update char map (Shorter ranges overlap larger ones)
  {   
   if(uint ridx = RangeMap[idx];ridx)   // Already belongs to some range   // We can write ranges inside of ranges but without overlapped borders
    {
     SRange* Rng = &ranges[ridx];
     if(rec.First < Rng->First)continue;    // Do not overwrite ranges with intersecting borders
     if(rec.Last  > Rng->Last )continue;       
    }
   RangeMap[idx] = res;   // This slot is free
  }
 return res;
}
//------------------------------------------------------
SRange* Get(uint8 chr, uint8 State)  
{
// if(State >= this->StateArr.Count())return nullptr;     // Slow but safe?
 uint8* RangeMap = this->StateArr.Data()[State].RangeMap;
 return &this->RangeArr.Data()[RangeMap[chr]];
}
//------------------------------------------------------
};
//============================================================================================================
struct STknBuf     // Scratch buffer. Can contain more than just a token. Grows as needed. Positioning is handled externally. DO NOT FORGET to free reserved ranges!
{
 SCVR uint MinBlk = NPTM::MEMPAGESIZE;
 CArr<uint8> arr; 
 uint8* beg;
 uint8* end;
 uint32 Offset;    // Modified to reserve some space    // TODO: Move to context !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//------------------------------------------------------   
 STknBuf(void)
 {
  this->Offset = 0;
  this->Expand();
 }
//------------------------------------------------------
 uint8* Put(uint8 Val, usize At)   // Returns ptr at
  {
   uint8* Addr = &this->beg[At];
   if(Addr >= this->end){this->Expand(); Addr = &this->beg[At];}
   *Addr = Val; 
   return Addr;
  }
//------------------------------------------------------
 usize  Len(void){return this->end - this->beg;}        // Meaningless (May have several ranges)
 uint8* Ptr(void)const {return (uint8*)this->beg;}        // Absolute ptr
 operator  uint8*() const {return (uint8*)this->beg;}    
 operator  achar*() const {return (achar*)this->beg;} 
 void Reset(void){this->Offset = 0;}       // ?????
//------------------------------------------------------
 void Expand(void)     // Sequential write only!
 {
  this->arr.Append(nullptr, MinBlk);
  this->beg = this->arr.c_data();
  this->end = this->beg + this->arr.Size();
 }
//------------------------------------------------------
};
//============================================================================================================
//                                   Scope Stack implementation
//------------------------------------------------------------------------------------------------------------
// FIFO is used to catch something like this: ({)}
// Some scopes closed by same char (use \ to hide)
//
struct SScopeLst              // Part of the current tokenizing context
{
 SCVR uint MaxScopes = 256;   // Max nested scopes (Depth) of specific type  // uint8 sized DepthState
 SCVR uint MaxGroups = 256;   // Max group indexes for scopes (tokens)

struct SScpMrk
{
 uint32 Offs;     // In buffer
 sint32 Size;     // In buffer
};

struct SScope
{
 SPos    Pos;       // 4/8 bytes
 SScpMrk Mrk;
 uint32  BufOffs;
 uint8   GroupId;
 uint8   Flags;     // From ETokenFlags
};          
 uint32 NextPosInFIFO;          // If ScopeArr could be an undeallocating allocator this would be unnecessary
 CArr<SScope> ScopeArr;         // Used as a stack for nestable scopes    // Will not shrink
 uint8 DepthState[MaxGroups];   // Max depth is 256 for each scope type(group)  // States of a flat scope markers (Like ' or ")    // 1-Opened scope, 0-Closed scope // Transitions: 0->1, 1->0;  1->1 or 0->0 is an error
//------------------------------------------------------
SScopeLst(void)
{
 this->Reset();
}
//------------------------------------------------------
void Reset(void)
{
 this->NextPosInFIFO = 0;
 memset(&DepthState,0,sizeof(DepthState));
}
//------------------------------------------------------
sint Update(STknBuf& Buffer, uint8 GroupId, uint8 Flags, const SScpMrk& ScpMrk, SPos&& TknPos, SPos* ScOpnPos=nullptr)
{
 uint8* DepthPtr = &this->DepthState[GroupId];
 if((Flags & (tfScopeOpn|tfScopeCse)) == (tfScopeOpn|tfScopeCse))   // Flat scope Open or Close
  {
   if(*DepthPtr)Flags &= ~tfScopeOpn;  // Already opened, close it
  }
 if(Flags & tfScopeOpn)
  {
   if(*DepthPtr == (MaxScopes-1))return -leScopeTooDeep;
   this->NextPosInFIFO++;
   (*DepthPtr)++;
   if(this->NextPosInFIFO > this->ScopeArr.Count())this->ScopeArr.Append(nullptr);  // Allocate an additional FIFO entry 
   SScope* Last   = this->ScopeArr.Get(this->NextPosInFIFO-1);
   Last->Pos.Set(&TknPos);
   Last->Mrk      = ScpMrk;      // May be incorrect, checked on tfScopeCse (Markers must match if one of them is valid)
   Last->BufOffs  = Buffer.Offset;
   Last->GroupId  = GroupId;
   Last->Flags    = Flags;
   if(ScpMrk.Size)Buffer.Offset = ScpMrk.Offs + ScpMrk.Size;      // Preserve from overwriting by a new token
   return *DepthPtr;
  }
 if(Flags & tfScopeCse)
  {
   if(!this->NextPosInFIFO)return -leUnexpScpClse;   // The FIFO is empty!
   SScope* Last = this->ScopeArr.Get(this->NextPosInFIFO-1);
   if(Last->GroupId != GroupId)
    {
     if(ScOpnPos)ScOpnPos->Set(&Last->Pos); // Useful for the error context 
     return -leWrongScpClse;
    }
   if(ScpMrk.Size != Last->Mrk.Size)return -leScpMrkMismatch;   // Marker mismatch error
   if(ScpMrk.Size)   // Have some marker
    {
     const uint8* MrkA = &Buffer.Ptr()[ScpMrk.Offs];         
     const uint8* MrkB = &Buffer.Ptr()[Last->Mrk.Offs];        
     const uint8* EndA = &MrkA[ScpMrk.Size];
     for(;MrkA < EndA;MrkA++,MrkB++)    // Memcmp? Nees something simple and fast              
        if(*MrkA != *MrkB)return -leScpMrkMismatch;   // Should be able to continue a raw string if this is encountered
     Buffer.Offset = Last->BufOffs;   // Release the reserved region
    }
   this->NextPosInFIFO--;
   if(!*DepthPtr)return -leUnexpScpClse;   // The depth counter is 0!
   (*DepthPtr)--;
  }
 return *DepthPtr;
}
//------------------------------------------------------
};
//============================================================================================================
//                                    The Lexer implementation
//------------------------------------------------------------------------------------------------------------
template<typename TStrm, uint8 LBR='\n'> class CLexer  // SLexerCtx  // NOTE: Not shared between threads
{
public:   // TODO private !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 uint32     Flags;
 uint32     Offset;   // Offset in bytes
 uint32     ChOffs;   // Offset in chars ( including non printable ones )
 uint32     LineNum;  // Line number
 uint32     LinePos;  // Position on the line (In spaced chars chars)
 uint32     PrvSplitFlg;
 uint32     LstDepth;
 uint8      State; 
 uint8      ChrLen;   // Size of current UTF-8 char
 uint8      ChBLeft;  // Bytes left in thecurrent UTF-8 char
 bool       FullChar;
 SScopeLst::SScpMrk smrk;   // For an associated scope marker 
 SPos       MemPos;   
 SPos       PrvPos;       // For error reporting
 STknInf    CurTkn;
 STknInf    MemTkn;
 SErrCtx    ErrCtx;  // TODO: Remember last up to 256(?) states for error reporting (With position of state changes) as a circular buffer and a pointer in it
 SScopeLst  Scopes;  // Context only (current source)
 STknBuf    Buffer;  // achar     TokenBuf[MaxTokenLen];   // Buffer     // Allocate?  // One page?   // Useful only to store tokens without skipped chars?   or with replaced chars  // TODO: Use pointer to original token if not modified!
 TStrm      Stream;  // Not affected by reset
 SRangeLst* Ranges; 
 vptr       CbData;  

using TCallback = sint (_fcall *)(achar* Text, uint32 ByteLen, STknInf* Token, vptr Data);
TCallback pOnChar;       // Does not called for each byte of multi-byte UTF-8 chars (Text points to first byte of a multi-byte char)
TCallback pOnToken;	
//------------------------------------------------------------------------------------------------------------
sint Reset(void)  //const achar* Data, uint Size)     // TODO: Use uint8 instead of achar?
{
 this->Offset      = this->Stream.Offset();  // TODO: Init from stream (Where reading will start)
 this->ChOffs      = 0;  // Relative to Offset (No way to recalculate the size in chars)
 this->ChrLen      = 0;  // Size of current UTF-8 char
 this->ChBLeft     = 0;
 this->LineNum     = 0;  // Line number
 this->LinePos     = 0;  // Position on the line
 this->State       = 0;  // Initial state is always 0 
 this->FullChar    = false;
 this->LstDepth    = 0;
 this->PrvSplitFlg = 0;
 this->smrk.Offs = this->smrk.Size = 0;

 this->ErrCtx.Reset();
 this->Scopes.Reset();
 this->Buffer.Reset(); // Reset TokenBuf (it have the actual size of the token)

 this->MemPos.Reset();
 this->PrvPos.Reset();
 this->ResetCurrToken();
 return 0;
}
//------------------------------------------------------------------------------------------------------------
void UpdateCurrPos(void)
{
 this->Offset++;  // In bytes!
 this->ChOffs  += this->FullChar;  
 this->LinePos += this->FullChar && !(this->CurTkn.Flags & tfNonSpaced); 
}
//------------------------------------------------------------------------------------------------------------
void ResetCurrToken(void)
{
 this->CurTkn.Offs  = this->CurTkn.EPos  = this->Buffer.Offset;
 this->CurTkn.CSize = this->CurTkn.Types = this->CurTkn.Group = this->CurTkn.Flags = 0;      // Reset the token
 this->CurTkn.Pos.Set(this->LineNum,this->LinePos,this->Offset,0,this->ChOffs,0);    
 this->MemTkn.Reset();      // Not related now
}
//------------------------------------------------------------------------------------------------------------
sint UpdateCurrToken(uint8 Chr, SRangeLst::SRange* Rng)
{
 this->CurTkn.Group  = Rng->Group;
 this->CurTkn.Flags  = Rng->Flags;
 this->CurTkn.Types &= ~Rng->FlgClr;  // First clear the flags
 this->CurTkn.Types |= Rng->FlgSet;   // then set them
 this->CurTkn.Pos.Size++;
 this->CurTkn.Pos.CSize += this->FullChar;   // This byte is last in the char                               
 if(Rng->Flags & tfIgnore)return 0;
 this->Buffer.Put((Rng->Flags & tfCharReplace)?(Rng->Repl):(Chr), this->CurTkn.EPos);     // Accumulate a token     // Is the 'replace' feature just useless slowdown?   
 this->CurTkn.EPos++;
 this->CurTkn.CSize += this->FullChar;
 if(this->pOnChar && this->FullChar) 
   if(sint res=this->pOnChar((achar*)&this->Buffer.Ptr()[this->CurTkn.EPos-this->ChrLen], this->ChrLen, &this->CurTkn, this->CbData);res)return this->ErrCtx.Set(res, State, this->CurTkn.Size(), this->CurTkn.Offs, SPos(this->LineNum,this->LinePos,this->Offset,this->CurTkn.Pos.Size,this->ChOffs,this->CurTkn.Pos.CSize), this->PrvPos);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// TODO: Some preregistered keywords should be able to alter current state too
//
sint ProcessToken(bool TknMem=false)   
{
 STknInf* Token = TknMem ? &this->MemTkn : &this->CurTkn;
     // achar* xxx = (achar*)&this->Buffer.Ptr()[Token->Offs];
 uint32 TknSize = Token->Size();
 if(!TknSize)return 0;

 sint Result = 0;
 this->PrvPos.Set(&Token->Pos);                            // Remember this token position (For error context, ScopeOpen)
 if(Token->Flags & tfNoTerm)Result = leChrCantBeLast;         // A last char brought this flag and it is not allowed to be last
   else
    {
     if(Token->Flags & tfNonSpaced)return 0;        // No tokens with non graphic chars allowed!
     if((Token->Flags & tfWhtspc) && !(this->Flags & lfKeepWhtspc))return 0;  // Ignore whitespaces 
     uint32 Depth = this->LstDepth;       
     if(Token->Flags & (tfScopeOpn|tfScopeCse)) 
      {    
       if(!(this->Flags & lfKeepBraces))return 0;                  
       if(Token->Flags & tfScpMrkEnd)    // Makes braces belong to outer level   
        {
         if(Token->Flags & tfScopeCse)Depth--;        // Raw strings require are different for some reason
        }
         else if(Token->Flags & tfScopeOpn)Depth--; 
      }
       else if((Token->Flags & tfDepthIndent) && !Token->Pos.Pos)Depth = Token->CSize;  // Only relative to the line start is useful ??????????????????
     achar* TknStr = (achar*)&this->Buffer.Ptr()[Token->Offs];
     DBGMSG("Token {%4u, %4u, %08X} D=%4u, S=%3u, G=%08X, T=%08X, F=%08X: %.*s", Token->Pos.Line,Token->Pos.Pos,Token->Pos.Offs,  Depth, this->State,  Token->Group, Token->Types, Token->Flags,   TknSize, TknStr);
     if(this->pOnToken)Result = this->pOnToken(TknStr, TknSize, Token, this->CbData);
    }
 if(Result)
  {
   if(TknMem)Result = this->ErrCtx.Set(Result, State, this->MemTkn.Size(), this->MemTkn.Offs, this->MemTkn.Pos, this->MemTkn.Pos);
     else Result = this->ErrCtx.Set(Result, State, this->CurTkn.Size(), this->CurTkn.Offs, SPos(this->LineNum,this->LinePos,this->Offset,this->CurTkn.Pos.Size,this->ChOffs,this->CurTkn.Pos.CSize), this->PrvPos);
  }
 return Result;
}
//------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/48176431/reading-utf-8-characters-from-console
//
sint32 NextByte(void)   // Returns 0 on EndOfStream (EOF) // NOTE: Reset it by resetting the stream
{
 uint8 val;
 ssize res = (ssize)this->Stream.Read(&val, 1);
 if(res <= 0)        // Have to decode UTF-8 to know exact token position on a line if there are some UTF-8 chars on it     // TODO: No need to check for NULL char?
  {
   if(!res)return -leNoMoreData;       // NoData needs special handling to allow continuation
   return this->Stream.IsEOF(res)?0:sint32(res - 0x100);  
  }
 if(val == LBR)        // '\r' does not matter
  {
   this->LineNum++;
   this->LinePos = 0;    
   this->ChBLeft = this->ChrLen = 1;     
  }
   else if(!this->ChBLeft)this->ChBLeft = this->ChrLen = NUTF::GetCharSize(val);    // High bits of a first determine size of UTF-8 char in bytes? ??? Check to replace table access with shift 
 this->ChBLeft--;
 this->FullChar = !this->ChBLeft;    // If this bytes is last it makes the char full
 return val;  // NOTE: Offset is not updated here to report errors correctly
}
//------------------------------------------------------------------------------------------------------------
public:
achar* GetErrVal(void)
{
 achar* Buf = this->Buffer;
 usize  Len = this->Buffer.Len();
 if(!this->ErrCtx.Size)  // Something useful may still be in the buffer
  {
   Buf += Len-1;
   *Buf = 0;
   return Buf;
  }
 Buf[this->ErrCtx.Size] = 0;
 return &Buf[this->ErrCtx.Offs];
}
//------------------------------------------------------------------------------------------------------------
achar* GetTknVal(void)
{
 achar* Buf = this->Buffer;
 if(!this->CurTkn.Size())  // Something useful may still be in the buffer
  {
   usize  Len = this->Buffer.Len();
   Buf += Len-1;
   *Buf = 0;
   return Buf;
  }
 Buf[this->CurTkn.EPos] = 0;
 return &Buf[this->CurTkn.Offs];
}
//------------------------------------------------------------------------------------------------------------
sint Init(SRangeLst* Rng, uint32 Flg=0)  // TODO: Do better  // SLexerCtx* Ctx
{
 this->Reset();
 this->Ranges   = Rng;
 this->Flags    = Flg;
 this->CbData   = nullptr;
 this->pOnChar  = nullptr;	     
 this->pOnToken = nullptr;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: ErrPos and ErrLine are counted from 0
//  Can continue to parse a next token(An error may break in a middle of current token, a warning will not)
// NOTE: In case of an error it is possible to extract expected char ranges from the current state
// NOTE: An error position reporting may be slightly broken
//
sint Tokenize(void)  // TODO: Continuable (Done?) // TODO: Break on warnings too (Done?)    // SLexerCtx* Ctx
{
 sint32 Val = 0;
 for(;;)   // TODO: Test if it is actually continuable  (should work from console input)  
  {
   Val = this->NextByte();
   if(Val <= 0)break;

   SRangeLst::SRange* Rng;
   uint32 SplitFlg = this->PrvSplitFlg;
   while((Rng=this->Ranges->Get(Val, this->State),!Rng->IsValid()))   // Detect and fallback to a base state 0 if there is no valid defined range for the char in current state    // If a range is not defined it is a empty record with NextState is 0 (Base state)
    {
     if(!this->State)return this->ErrCtx.Set(leUnexpChar, this->State, 1, this->Buffer.Put(Val, this->CurTkn.Size()) - this->Buffer.Ptr(), SPos(this->LineNum,this->LinePos,this->Offset,1,this->ChOffs,1), this->CurTkn.Pos);    // Nowhere to fallback, already in state 0 (All usable ASCII ranges must be defined in state 0)
     this->State = 0;              // Rng->State (An invalid rec is empty and its state is 0 too)
     SplitFlg   |= tfTknRSplit;    // The char will start a new token (Same as tfTknRSplit)
    }
   this->LstDepth = this->Scopes.NextPosInFIFO;   // NOTE: Will reset depth for

// We have some valid range in Rng now   // This range in this state is marked as inappropriate at this point (Saves the Lexer from processing some bad number pieces like '0b1010120101;' which would be just split as '0b10101','20101;' otherwise - just add range 2-9 as bad for binary numbers, and 'a-z' too, probably)
   if(Rng->Flags & tfBadToken)return this->ErrCtx.Set(leBadChar, this->State, 1, this->Buffer.Put(Val, this->CurTkn.Size()) - this->Buffer.Ptr(), SPos(this->LineNum,this->LinePos,this->Offset,1,this->ChOffs,1), this->CurTkn.Pos);  
 
   SplitFlg |= Rng->Flags;          // RSplit first stores current token then inits a new one; LSplit stores adds the char, stores the token and then inits a new token  // LSplit is for scopes and usually set manually so should have higher priority
   uint8 NxtState = Rng->State;
   if(SplitFlg & tfScpMrkBegin)this->smrk.Offs = this->CurTkn.EPos + 1;       // Starting from a next char      // Begin a marker, like R"marker(test)marker" 
   if(SplitFlg & tfScpMrkEnd)this->smrk.Size = this->CurTkn.EPos - this->smrk.Offs;    // Should come with tfScopeOpn  // it is incorrect to set tfScpMrkEnd and tfScpMrkBegin on a same char    // Usually raw strings will be used without markers, like R"(test)"
   if(SplitFlg & (tfScopeOpn|tfScopeCse))   // Can do scoping even in the middle of a token (Scope closing does not imply splitting)    // TODO: Apply only to an fully accumulated token  // How? We need splitting to be done for that
    {
     sint res = this->Scopes.Update(this->Buffer, Rng->Group, Rng->Flags, this->smrk, SPos(this->LineNum,this->LinePos,this->Offset,this->CurTkn.Pos.Size,this->ChOffs,this->CurTkn.Pos.CSize), &this->PrvPos);
     if((res < 0) && !(res == -leScpMrkMismatch))  //  !((res == -tleScpMrkMismatch)&&(SplitFlg & tfRawString)))    // At this point a last token char is not yet appended and even CurTkn may be a last token  // Mismatched raw string close marker, should continue 
      {
       if(SplitFlg & tfTknRSplit){this->ProcessToken(); this->UpdateCurrToken(Val, Rng);}   // Store last token to update the error position
       return this->ErrCtx.Set(-res, this->State, this->CurTkn.Size(), this->CurTkn.Offs, SPos(this->LineNum,this->LinePos,this->Offset,this->CurTkn.Pos.Size,this->ChOffs,this->CurTkn.Pos.CSize), this->PrvPos);    // Scoping error     // NOTE: only scope closing may return an error   
      }
       else if(SplitFlg & tfScopeCse)  //if(!res && (SplitFlg & (tfScopeCse|tfLSplitOnSDZ)) == (tfScopeCse|tfLSplitOnSDZ))SplitFlg |= tfTknLSplit;     // LSplit
        {
         if(!res)
          {
           if(SplitFlg & tfLSplitOnSDZ)SplitFlg |= tfTknLSplit;     // LSplit
           if(SplitFlg & tfMemSplitOnSDZ)SplitFlg |= tfTknMemSplit;  
          }
           else if(SplitFlg & tfOChStateOnSDZ)NxtState = this->State;   // Keep the current state
        }
//     if(this->smrk.Size && (SplitFlg & tfScopeCse))this->smrk.Offs = this->smrk.Size = 0;   // Reset scope marker ?      // @@@@@@@@@@@ TODO: Flag to merge current token`s state with an accumutated token (Use add termination flags to it instead of storing ';')
    }

   if((SplitFlg & tfTknMemSplit) && this->MemTkn.Size())     // Split a remembered token   // NOTE: Must be done before any other 'ProcessToken'  // And after scope processing (Because it may remove tfTknMemSplit as a hack)
    {                                                    // Buffer: MemTkn|CurTkn  (Same scratch region)      // !!!! Scopes: How to rollback scratch buffer if there are always a token accumulated in it?
     if(sint res=this->ProcessToken(true);res < 0)return res;   // Is this correct pos?  // Check size before the call?
     this->MemPos.Size = this->CurTkn.Pos.Size - this->MemTkn.Pos.Size;  
     this->CurTkn.Offs = this->MemTkn.EPos;
     this->CurTkn.Pos.Set(&this->MemPos);
     this->MemTkn.Reset();    
     this->MemPos.Reset();     // Required?
    }

   if((SplitFlg & (tfTknLSplit|tfTknRSplit)) == (tfTknLSplit|tfTknRSplit))  // Magic combination to force a next char to start a new token   // Also makes sure that the only one of these flags is set
    {
     this->PrvSplitFlg =  tfTknRSplit;    // Split at next char
     SplitFlg         &= ~tfTknLSplit;
    }
     else this->PrvSplitFlg = 0;

   if(SplitFlg & tfTknMemStore){this->MemTkn = this->CurTkn; this->MemPos.Set(this->LineNum,this->LinePos,this->Offset,0,this->ChOffs,0);}   // The current char will not be a part of the remembered token   // Is here a better place for this?
   if(SplitFlg & tfTknLSplit)       // The current char will be last char of the current token
    {
     if(sint res=this->UpdateCurrToken(Val, Rng);res < 0)return res;   // Consume the current char
     if(sint res=this->ProcessToken();res < 0)return res;   // Check size before the call?
     this->UpdateCurrPos();
     this->ResetCurrToken();                    // Disabled: This will start a new token on the position of an already processed byte! 
    }
     else
      {
       if(SplitFlg & tfTknRSplit)        // Store the accumulated token  // The current char will start a new token
        {
         if(SplitFlg & tfAddTkTypeLeft)       // Append type flags of current char(which will start a new token) to a previous token before processing it   // TODO: Test it
          {
           this->CurTkn.Types &= ~Rng->FlgClr;  // First clear the flags
           this->CurTkn.Types |= Rng->FlgSet;   // then set them
          }
         if(sint res=this->ProcessToken();res < 0)return res;   // Check size before the call?
         this->ResetCurrToken();        // Current byte starts a new token
        }     
       if(sint res=this->UpdateCurrToken(Val, Rng);res < 0)return res; 
       this->UpdateCurrPos();
      }
   this->State = ((SplitFlg & tfBRstIfSplit) && (SplitFlg & (tfTknLSplit|tfTknRSplit))) ? 0 : this->State = NxtState;
  }

 if(Val < 0)return sint(Val);  // Not EOF
 // May be an incomplete token, unexpected EOF or just a whitespace 
 if(sint res=this->ProcessToken();res < 0)return res;  // Process a last token
 this->ResetCurrToken();
 if(this->Scopes.NextPosInFIFO)    // Check any unclosed scopes
  {
   SPos pos2;
   if(this->Scopes.NextPosInFIFO > 1)pos2.Set(&this->Scopes.ScopeArr.Get(this->Scopes.NextPosInFIFO-2)->Pos);     // TODO: Check correctness of all returned error positions
   return this->ErrCtx.Set(leUnclosedScpAtEOF, 0, 0, 0, SPos{this->Scopes.ScopeArr.Get(this->Scopes.NextPosInFIFO-1)->Pos}, pos2, this->Scopes.NextPosInFIFO);    // TODO: Take two last unclosed scopes pos
  }
 return this->Offset;   // Return size of processed text (In bytes)
}
//------------------------------------------------------------------------------------------------------------
}; 
//------------------------------------------------------------------------------------------------------------ 
}; 
