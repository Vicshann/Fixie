
/*
 https://en.wikipedia.org/wiki/Argument-dependent_name_lookup
 https://probablydance.com/2015/02/16/ideas-for-a-programming-language-part-3-no-shadow-worlds/
 https://stackoverflow.com/questions/36668466/bitfields-and-alignment

 TODO: Store numeric strings alongside their positive AND negative representations

*/

struct CParser
{
//------------------------------------------------------------------------------------------------------------
// NOTE: For interpreting/code generation it is better to rearrange some expressions. To avoid memory moving just store with each token index of a next token, relative to current one.
//      And beginning of an expression will have index of first token in the expression
struct STkn      // There will be a great amount of such records, preferable in a contiguous memory for CPU cache sake  // TODO TODO TODO
{
 uint16 StrIdx;  // Index of the token SStr in CStrIn
 uint16 Index;   // Index of a lexical template (SLex)  // 0 - undefined   // Required for scope tokens to produce a valid sequence  // If stays 0 when executing - report the UnknownToken error
 uint16 Extra;   // Extra info index (uint64 index, arbitrary sized records) (Scope size, target from, or jump to) For a first token - Total token records in this scope (To skip to a next scope without enumeration) (Includes subscopes) else - Scope depth (Controlled by scoping tokens (Must be defined first))   // May be too small (namespaces) // Is there are reason to skip a scope?
 uint8  Scope;   // Scope depth level (256 should be enough)
 uint8  Flags;   // ??? (i.e. to disable the token)  //ScopeFirst, ScopeLast    // Created automatically from ETokenFlags
};
//------------------------------------------------------------------------------------------------------------
struct SPos      // Indeed, part of 'struct-of-arrays' concept but only because it may be optional and useful only for error reporting or an IDE  // Text related info for error reporting  (Move to optional array?)   // Moved out of STkn to make it more compack in cache (altough having some cache misses when writing but that happens only once)
{
}
//------------------------------------------------------------------------------------------------------------
struct STokenLst           // Parsed from a file
{
// enum EFlg {flNone=0, flStorePos=0x01};

 CArr<STkn> Tokens;
 CArr<SPos> Positions;
// uint32 LastScope = 0;   // 0 is the root scope
// uint32 Flags = flNone;

//------------------------------------------------------
// Need NoLex flag?
//
sint Add(uint8 CurState, uint32 Group, uint32 Type, uint32 Flags, uint TknLen, const achar* TknVal, SPos&& TknPos, SPos& PrvPos) //     uint16 StrIdx, sint LexIdx, sint ExtIdx, uint8 slflg, uint32 AbsPos, uint16 SrcLin, uint16 SrcPos)
{
/* STkn tkn;
 if(slflg & tfScopeBeg)this->LastScope++;
 if(this->LastScope > MaxScope){LOGERR(LSTR("Token scope too deep at %u:%u:%u"), AbsPos, SrcLin, SrcPos);}

 tkn.StrIdx = StrIdx;
 tkn.Scope  = (uint8)this->LastScope;
 tkn.Flags  = slflg;   // tfBaseLex, tfScopeBeg, tfScopeEnd
 if(LexIdx >= 0){tkn.Index = LexIdx; tkn.Flags |= tfHaveLex;}   // If < 0 then the token is detected by default separation algo, not by Lex template
   else tkn.Index = 0;
 if(ExtIdx >= 0){tkn.Extra = ExtIdx; tkn.Flags |= tfHaveExt;}
   else tkn.Extra = 0;

 if(slflg & tfScopeEnd)this->LastScope--;
 this->Tokens.Append(&tkn, 1);

 if(this->Flags & flStorePos)
  {
   SPos ps {.AbsPos=AbsPos, .SrcLin=SrcLin, .SrcPos=SrcPos};
   this->Positions.Append(&ps, 1);
  }
*/
 return 0;
}
//------------------------------------------------------
};
//============================================================================================================
STokenLst Tokens;  // Shared
//------------------------------------------------------------------------------------------------------------
};