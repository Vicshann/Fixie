
/*
https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords/613132#613132
https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c
https://www.reddit.com/r/ProgrammingLanguages/comments/15o42wx/how_to_implement_an_order_free_parser_with_angle/
https://en.cppreference.com/w/cpp/language/escape
https://en.cppreference.com/w/cpp/language/charset
https://www.quut.com/c/ANSI-C-grammar-l-2011.html

https://en.wikipedia.org/wiki/Operators_in_C_and_C%2B%2B
https://www.open-std.org/jtc1/sc22/WG14/www/docs/n1256.pdf#%5B%7B%22num%22%3A148%2C%22gen%22%3A0%7D%2C%7B%22name%22%3A%22XYZ%22%7D%2C-27%2C816%2Cnull%5D

https://en.cppreference.com/w/cpp/language/operator_alternative

c++ casting a negative integer to smaller size. What will be the result????
*/

/*
<: :>   []
<% %>   {}
%:      #
%:%:    ##


[](){}.->
++ -- & * + - ~ !
/%<< >><><= >= == !=^|&& ||
?:;...
= *= /= %= += -= <<= >>= &= ^= |=
,###
<: :> <% %> %: %:%:
*/


// C++ parsing example
class CParseCpp
{
 enum EChrStates    // Sorted by occurrence probability(may save some memory)   // 256 bytes per state record
  {
   csBase = 0,
   csWhitespc,
   csTSpecial,
   csInTkName,  
   csInNumExt,
//   csInNumSep,   // Just allow 0b1001101'; for now
   csInDecNum,
//   csInHexNum,
   csInHexVal,
   csInOctNum,
  // csInBinNum,  
   csInBinVal,
  // csInHexFlt,
   csInDecFlt,
   csInHFlVal,
   csInFltExp,
 //  csInFltExV,
   csInFlEVal,
   csInCmnBeg,
   csInCmntSL,
   csInCmntML,
   csInCmntBegML,
   csInCmntEndML,
   csInSQString,
   csInDQString,
   csInSQStrEsc,
   csInDQStrEsc,
   csInPossRStr,
   csInSQRawStr,
   csInDQRawStr,
   csInSQRawStrBeg,
   csInDQRawStrBeg,
   csInSQRawStrEnd,
   csInDQRawStrEnd,
   csInBOM1,
   csInBOM2,
  };
//-----------------------------------------------------------------------
// Used flags instead of enum so they can be merged during collapsing of tokens later
 enum ETknType          // Passed to the Lexer
  {
   ttNone,
   ttName       = 0x00000001,
   ttNumber     = 0x00000002,  // Unknow numberic (A token starts with 0) 
   ttDecNum     = 0x00000004,
   ttHexNum     = 0x00000008,
   ttOctNum     = 0x00000010,
   ttBinNum     = 0x00000020,
   ttFltNum     = 0x00000040,  // Dec or Hex form
   ttFltExp     = 0x00000080,  // Float number with an exponent
   ttIndent     = 0x00000100,  // Python-like indent scoping
   ttWhSpace    = 0x00000200,
   ttSpecial    = 0x00000400, 
   ttComment    = 0x00000800, 
   ttSQString   = 0x00001000,  // Single quoted, escaped. Packed string - converts to an integer or an array of integers. Packing is platform specific // Always attempted to be embedded in the code without rdata placement
   ttDQString   = 0x00002000,  // Double quoted, escaped.
   ttSQRawStr   = 0x00004000,  // Single quoted, raw.
   ttDQRawStr   = 0x00008000,  // Double quoted, raw.
   ttInCurlyBr  = 0x00010000,  // ???
   ttInRoundBr  = 0x00020000,  // ???
   ttInSquareBr = 0x00040000,  // ???
  };

enum EScpGroup
{
 sgNone,
 sgIndent,
 sgCmntML,
 sgBrCurly,
 sgBrRound,
 sgBrSquare,
 sgSQString,
 sgDQString,
 sgSQRawStr,
 sgDQRawStr,
};
//------------------------------------------------------------------------------------------------------------
NLEX::CLexer<STRM::CStrmBuf> lex;
NLEX::SRangeLst Ranges;  // May be Shared 
//------------------------------------------------------------------------------------------------------------
void RegRanges_WS(void)
{
 // Whitespaces          
 this->Ranges.Add(csBase|(csWhitespc << 8), 0x01,0x20, csWhitespc, sgNone, ttWhSpace, ttNone, NLEX::tfWhtspc);  // Whitespace: 01-20, 7F  // No messing up text parsing/display - treat all special chars as whitespaces (If you put them in your source code you are responsible if some text editor(or a terminal emulator) will choke on it)
 this->Ranges.Add(csBase|(csWhitespc << 8), 0x7F,0x7F, csWhitespc, sgNone, ttWhSpace, ttNone, NLEX::tfWhtspc);

 this->Ranges.Add(csBase,   0xEF,0xEF, csInBOM1,   sgNone, ttWhSpace, ttNone, NLEX::tfWhtspc);
 this->Ranges.Add(csInBOM1, 0xBB,0xBB, csInBOM2,   sgNone, ttWhSpace, ttNone, NLEX::tfWhtspc);
 this->Ranges.Add(csInBOM2, 0xBF,0xBF, csWhitespc, sgNone, ttWhSpace, ttNone, NLEX::tfWhtspc);
}
//------------------------------------------------------------------------------------------------------------
// https://en.cppreference.com/w/cpp/language/operator_alternative
// Trigraphs are removed in C++17
// There are alternative operqator tokens like 'xor_eq' which reqire to be separated by any valid delimiter. If required, such operators must be registered as keywords
//
void RegRanges_Names(void)
{
 // For names     // Must add to all prefixes and keywords that is handled
 this->Ranges.Add(csBase|(csInTkName << 8)|(csInPossRStr << 16),   'a','z', csInTkName, sgNone, ttName);
 this->Ranges.Add(csBase|(csInTkName << 8)|(csInPossRStr << 16),   'A','Z', csInTkName, sgNone, ttName);
 this->Ranges.Add(csBase|(csInTkName << 8)|(csInPossRStr << 16),   '_','_', csInTkName, sgNone, ttName);
 this->Ranges.Add(csBase|(csInTkName << 8)|(csInPossRStr << 16), 0x80,0xFF, csInTkName, sgNone, ttName);   // UTF-8 multi-byte chars  // Too much to manage - just allow any of this to be in names as 'a - z'  // Means no special chars in extended codepoints will be supported (Too slow to parse)  // Aliasing will solve this at the Lexer level
                                   
 this->Ranges.Add(csInTkName|(csInPossRStr << 8), '0','9', csInTkName, sgNone, ttNone  );
} 
//------------------------------------------------------------------------------------------------------------
void RegRanges_Specials(void)          // TODO: Use @ for aliasing by default
{
 // Specials (Split by Lexer)
 this->Ranges.Add(csBase|(csTSpecial << 8), 0x21,0x2F, csTSpecial, sgNone, ttSpecial, ttNone);  // !"#$%&'()*+,-./
 this->Ranges.Add(csBase|(csTSpecial << 8), 0x3A,0x40, csTSpecial, sgNone, ttSpecial, ttNone);  // :;<=>?@       // TODO: ';' as special ?
 this->Ranges.Add(csBase|(csTSpecial << 8), 0x5B,0x5E, csTSpecial, sgNone, ttSpecial, ttNone);  // [\]^
 this->Ranges.Add(csBase|(csTSpecial << 8), 0x60,0x60, csTSpecial, sgNone, ttSpecial, ttNone);  // ` 
 this->Ranges.Add(csBase|(csTSpecial << 8), 0x7B,0x7E, csTSpecial, sgNone, ttSpecial, ttNone);  // {|}~
                                                  
// Scopes
 this->Ranges.Add(csBase|(csTSpecial << 8), '{','{', csBase, sgBrCurly,  ttInCurlyBr,  ttNone, NLEX::tfScopeOpn|NLEX::tfTknRSplit|NLEX::tfTknLSplit);
 this->Ranges.Add(csBase|(csTSpecial << 8), '}','}', csBase, sgBrCurly,  ttInCurlyBr,  ttNone, NLEX::tfScopeCse|NLEX::tfTknRSplit|NLEX::tfTknLSplit);
 
 this->Ranges.Add(csBase|(csTSpecial << 8), '(','(', csBase, sgBrRound,  ttInRoundBr,  ttNone, NLEX::tfScopeOpn|NLEX::tfTknRSplit|NLEX::tfTknLSplit);
 this->Ranges.Add(csBase|(csTSpecial << 8), ')',')', csBase, sgBrRound,  ttInRoundBr,  ttNone, NLEX::tfScopeCse|NLEX::tfTknRSplit|NLEX::tfTknLSplit);
 
 this->Ranges.Add(csBase|(csTSpecial << 8), '[','[', csBase, sgBrSquare, ttInSquareBr, ttNone, NLEX::tfScopeOpn|NLEX::tfTknRSplit|NLEX::tfTknLSplit);
 this->Ranges.Add(csBase|(csTSpecial << 8), ']',']', csBase, sgBrSquare, ttInSquareBr, ttNone, NLEX::tfScopeCse|NLEX::tfTknRSplit|NLEX::tfTknLSplit);
  
// Comments  (Multiline comments are nestable)
 this->Ranges.Add(csBase|(csTSpecial << 8),           '/','/', csInCmnBeg,      sgNone, ttNone,     ttNone, NLEX::tfTknRSplit);    // Always starts a new token (Div or a comment)
 this->Ranges.Add(csInCmnBeg,                         '/','/', csInCmntSL,      sgNone, ttComment,  ttNone, NLEX::tfComment); 
 this->Ranges.Add(csInCmnBeg|(csInCmntBegML << 8),    '*','*', csInCmntML,    sgCmntML, ttComment,  ttNone, NLEX::tfComment|NLEX::tfScopeOpn);    // Increase scope depth and continue the comment
                                                                                  
 this->Ranges.Add(csInCmntSL,                       0x01,0xFF, csInCmntSL,      sgNone, ttComment,  ttNone, NLEX::tfComment);
 this->Ranges.Add(csInCmntSL,                       '\n','\n', csWhitespc,      sgNone, ttComment,  ttNone, NLEX::tfWhtspc|NLEX::tfTknRSplit);    // Done at EOL   // '\r' (if present) will still be left at the end of a comment
                                                                                   
 this->Ranges.Add(csInCmntML|(csInCmntEndML << 8),  0x01,0xFF, csInCmntML,      sgNone, ttComment,  ttNone, NLEX::tfComment); 
 this->Ranges.Add(csInCmntML,                         '/','/', csInCmntBegML,   sgNone, ttComment,  ttNone, NLEX::tfComment); 
                                                                                 
 this->Ranges.Add(csInCmntML,                         '*','*', csInCmntEndML,   sgNone, ttComment,  ttNone, NLEX::tfComment); 
 this->Ranges.Add(csInCmntEndML,                      '/','/', csInCmntML,    sgCmntML, ttComment,  ttNone, NLEX::tfComment|NLEX::tfScopeCse|NLEX::tfLSplitOnSDZ|NLEX::tfBRstIfSplit);  

// In C/C++ \ is used as line continuation mark so defining macros in multi-line is possible. It is also possible to change a single line comment to multi-line with it. And any tokens can be split too.
// Just consume and ignore anything up to '\n' includin it. Anything on a next line will continue the token
// This would mean that we must add such behaviour in EVERY state(Every name, number and every composed token) and it MUST be an unique state for proper continuation!
// I am too lazy to implement that ugly nonsence!    // May be allow a char to be assigned globally to skip rest of the line(Same as '\n' is works globally)?

////////
 this->Ranges.Add(csBase, '.','.', csBase, sgIndent, ttWhSpace, ttNone, NLEX::tfDepthIndent);   // |(csWhitespc << 8)
}                                            
//------------------------------------------------------------------------------------------------------------
void RegRanges_Strings(void) 
{
// Strings: "" '' raw        // Parse escapes here or later?      // Store quotes as separate tokens?
 this->Ranges.Add(csBase|(csTSpecial << 8),         '\'','\'', csInSQString, sgSQString, ttSQString, ttNone, NLEX::tfEscString|NLEX::tfScopeOpn|NLEX::tfTknLSplit);   // Raw format for this type of string too?
 this->Ranges.Add(csInSQString,                     0x01,0xFF, csInSQString, sgSQString, ttSQString, ttNone, NLEX::tfEscString);
 this->Ranges.Add(csInSQString,                     '\\','\\', csInSQStrEsc, sgSQString, ttSQString, ttNone, NLEX::tfEscString);   // Escape a char (including ')
 this->Ranges.Add(csInSQString,                     '\'','\'', csBase,       sgSQString, ttSQString, ttNone, NLEX::tfEscString|NLEX::tfScopeCse|NLEX::tfTknRSplit|NLEX::tfTknLSplit);   // Close the scope
 this->Ranges.Add(csInSQStrEsc,                     0x01,0xFF, csInSQString, sgSQString, ttSQString, ttNone, NLEX::tfEscString);   // Consume a char and return to the string state     //TODO: Escapes can be multichar
 
 this->Ranges.Add(csBase|(csTSpecial << 8),         '\"','\"', csInDQString, sgDQString, ttDQString, ttNone, NLEX::tfEscString|NLEX::tfScopeOpn|NLEX::tfTknLSplit);   // Raw format for this type of string too?
 this->Ranges.Add(csInDQString,                     0x01,0xFF, csInDQString, sgDQString, ttDQString, ttNone, NLEX::tfEscString);
 this->Ranges.Add(csInDQString,                     '\\','\\', csInDQStrEsc, sgDQString, ttDQString, ttNone, NLEX::tfEscString);   // Escape a char (including ')
 this->Ranges.Add(csInDQString,                     '\"','\"', csBase,       sgDQString, ttDQString, ttNone, NLEX::tfEscString|NLEX::tfScopeCse|NLEX::tfTknRSplit|NLEX::tfTknLSplit);   // Close the scope
 this->Ranges.Add(csInDQStrEsc,                     0x01,0xFF, csInDQString, sgDQString, ttDQString, ttNone, NLEX::tfEscString);   // Consume a char and return to the string state

// Raw strings prefix
 this->Ranges.Add(csBase,            'R','R', csInPossRStr,        sgNone, ttNone,     ttNone);      // NOTE: Slows down parsing of every 'R' 

// Raw single-quoted string
 this->Ranges.Add(csInPossRStr,    '\'','\'', csInSQRawStrBeg, sgSQRawStr, ttSQRawStr, ttNone, NLEX::tfScpMrkBegin); 
 this->Ranges.Add(csInSQRawStrBeg, 0x01,0xFF, csInSQRawStrBeg, sgSQRawStr, ttSQRawStr, ttNone);
 this->Ranges.Add(csInSQRawStrBeg,   '(','(', csInSQRawStr,    sgSQRawStr, ttNone,     ttNone, NLEX::tfRawString|NLEX::tfTknLSplit|NLEX::tfScopeOpn|NLEX::tfScpMrkEnd);
 this->Ranges.Add(csInSQRawStr,    0x01,0xFF, csInSQRawStr,    sgSQRawStr, ttSQRawStr, ttNone, NLEX::tfRawString);
 
 this->Ranges.Add(csInSQRawStr,      ')',')', csInSQRawStrEnd, sgSQRawStr, ttSQRawStr, ttNone, NLEX::tfRawString|NLEX::tfTknMemStore|NLEX::tfScpMrkBegin);    // Remember a possible finished string
 this->Ranges.Add(csInSQRawStrEnd, 0x01,0xFF, csInSQRawStrEnd, sgSQRawStr, ttSQRawStr, ttNone, NLEX::tfRawString);                                // The string continues
 this->Ranges.Add(csInSQRawStrEnd,   ')',')', csInSQRawStrEnd, sgSQRawStr, ttSQRawStr, ttNone, NLEX::tfRawString|NLEX::tfTknMemStore|NLEX::tfScpMrkBegin);    // Repeat // Remember a possible scope closing token
// this->Ranges.Add(csInSQRawStrEnd, '\\','\\', csInSQRawStrEnd,    sgSQRawStr, ttSQRawStr, ttNone,  NLEX::tfBadToken);      // Disabled: we can`t be sure if it is actually in a closing marker now
// this->Ranges.Add(csInSQRawStrEnd, '/','/',   csInSQRawStrEnd,    sgSQRawStr, ttSQRawStr, ttNone,  NLEX::tfBadToken); 
 this->Ranges.Add(csInSQRawStrEnd, '\'','\'', csBase,          sgSQRawStr, ttSQRawStr, ttNone, NLEX::tfRawString|NLEX::tfScopeCse|NLEX::tfLSplitOnSDZ|NLEX::tfMemSplitOnSDZ|NLEX::tfScpMrkEnd|NLEX::tfOChStateOnSDZ); 

// Raw double-quoted string
 this->Ranges.Add(csInPossRStr,    '\"','\"', csInDQRawStrBeg, sgDQRawStr, ttDQRawStr, ttNone, NLEX::tfScpMrkBegin); 
 this->Ranges.Add(csInDQRawStrBeg, 0x01,0xFF, csInDQRawStrBeg, sgDQRawStr, ttDQRawStr, ttNone);
 this->Ranges.Add(csInDQRawStrBeg,   '(','(', csInDQRawStr,    sgDQRawStr, ttNone,     ttNone, NLEX::tfRawString|NLEX::tfTknLSplit|NLEX::tfScopeOpn|NLEX::tfScpMrkEnd);
 this->Ranges.Add(csInDQRawStr,    0x01,0xFF, csInDQRawStr,    sgDQRawStr, ttDQRawStr, ttNone, NLEX::tfRawString);

 this->Ranges.Add(csInDQRawStr,      ')',')', csInDQRawStrEnd, sgDQRawStr, ttDQRawStr, ttNone, NLEX::tfRawString|NLEX::tfTknMemStore|NLEX::tfScpMrkBegin);    // Remember a possible finished string
 this->Ranges.Add(csInDQRawStrEnd, 0x01,0xFF, csInDQRawStrEnd, sgDQRawStr, ttDQRawStr, ttNone, NLEX::tfRawString);                                // The string continues
 this->Ranges.Add(csInDQRawStrEnd,   ')',')', csInDQRawStrEnd, sgDQRawStr, ttDQRawStr, ttNone, NLEX::tfRawString|NLEX::tfTknMemStore|NLEX::tfScpMrkBegin);    // Repeat // Remember a possible scope closing token
// this->Ranges.Add(csInDQRawStrEnd, '\\','\\', csInDQRawStrEnd,    sgDQRawStr, ttDQRawStr, ttNone,  NLEX::tfBadToken);      // Disabled: we can`t be sure if it is actually in a closing marker now
// this->Ranges.Add(csInDQRawStrEnd, '/','/',   csInDQRawStrEnd,    sgDQRawStr, ttDQRawStr, ttNone,  NLEX::tfBadToken); 
 this->Ranges.Add(csInDQRawStrEnd, '\"','\"', csBase,          sgDQRawStr, ttDQRawStr, ttNone, NLEX::tfRawString|NLEX::tfScopeCse|NLEX::tfLSplitOnSDZ|NLEX::tfMemSplitOnSDZ|NLEX::tfScpMrkEnd|NLEX::tfOChStateOnSDZ); 
}
//------------------------------------------------------------------------------------------------------------
//Check that you can have inputs like +++-+++-+-+-++3+-+-+-+5 and they are valid. 
// Unary operators should be given more precedence than the binary arithmetic ones, 
// to allow expressions like 5 * - 3 or -5 * +3 (to be interpreted as 5 * (-3) and (-5) * (+3)) or at least more than the adding operators + and - (in that case, de second -5 * +3 would be incorrect)
// And do not forget -(123) is same as -123 bit it is known later
// Ex: something-5    // Since a language can be whitespace free, does this mean that -5 is an argument to 'something' or it is an expression like 'int x = something - 5' ?
// 
void RegRanges_Numbers(void)  // NOTE: It is diffucult to determine negative numbers!   // TODO: Is it possible to attach a leading sign without actually knowing if it an operator or a part of the number?
{
// For all numbers
 this->Ranges.Add(csBase,        '0','0', csInNumExt, sgNone, ttNumber, ttNone, NLEX::tfNumeric);    // 0OCTAL, 0x,0b    // Decimal 0, if alone
 this->Ranges.Add(csBase,        '1','9', csInDecNum, sgNone, ttDecNum, ttNone, NLEX::tfNumeric);    // Or float
 this->Ranges.Add(csInDecNum,    '0','9', csInDecNum, sgNone, ttDecNum, ttNone, NLEX::tfNumeric);    // May be turned into float
 this->Ranges.Add(csInDecNum,  '\'','\'', csInDecNum, sgNone, ttNone,   ttNone, NLEX::tfIgnore|NLEX::tfNoTerm);     // Just ignore those, do not store in the number string
// Turning dec numbers into dec floats           
 this->Ranges.Add(csInDecNum,    '.','.', csInDecFlt, sgNone, ttFltNum);    // Can have dec floats like 'float x = 34.;'
 this->Ranges.Add(csInDecFlt,    '0','9', csInDecFlt, sgNone, ttNone,   ttNone, NLEX::tfNumeric);
 this->Ranges.Add(csInDecFlt,  '\'','\'', csInDecFlt, sgNone, ttNone,   ttNone, NLEX::tfIgnore|NLEX::tfNoTerm);    // Just ignore those, do not store in the number string
 this->Ranges.Add(csInDecFlt,    'e','e', csInFltExp, sgNone, ttFltExp, ttNone, NLEX::tfNoTerm);    
 this->Ranges.Add(csInDecFlt,    'E','E', csInFltExp, sgNone, ttFltExp, ttNone, NLEX::tfNoTerm);
// Exponent (Dec/Hex floats)                                               
 this->Ranges.Add(csInFltExp,    '0','9', csInFlEVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);    // E5
 this->Ranges.Add(csInFltExp,    '+','+', csInFlEVal, sgNone, ttNone,   ttNone, NLEX::tfNoTerm );    // E+5   // 3.250000e+004
 this->Ranges.Add(csInFltExp,    '-','-', csInFlEVal, sgNone, ttNone,   ttNone, NLEX::tfNoTerm );    // E-5
 this->Ranges.Add(csInFlEVal,    '0','9', csInFlEVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric); 
 this->Ranges.Add(csInFlEVal,  '\'','\'', csInFlEVal, sgNone, ttNone,   ttNone, NLEX::tfIgnore|NLEX::tfNoTerm);    // Just ignore those, do not store in the number string
// Octal numbers                                                
 this->Ranges.Add(csInNumExt,    '0','8', csInOctNum, sgNone, ttOctNum, ttNone, NLEX::tfNumeric);    // 00067
 this->Ranges.Add(csInOctNum,    '0','8', csInOctNum, sgNone, ttNone,   ttNone, NLEX::tfNumeric);    // Only 0-8 is valid
 this->Ranges.Add(csInOctNum,  '\'','\'', csInOctNum, sgNone, ttNone,   ttNone, NLEX::tfIgnore|NLEX::tfNoTerm);    // Just ignore those, do not store in the number string
// Binary numbers                                              
 this->Ranges.Add(csInNumExt,    'b','b', csInBinVal, sgNone, ttBinNum, ttNone, NLEX::tfNoTerm);    
 this->Ranges.Add(csInNumExt,    'B','B', csInBinVal, sgNone, ttBinNum, ttNone, NLEX::tfNoTerm); 
 this->Ranges.Add(csInBinVal,    '0','1', csInBinVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);    // This can be separated
 this->Ranges.Add(csInBinVal,  '\'','\'', csInBinVal, sgNone, ttNone,   ttNone, NLEX::tfIgnore|NLEX::tfNoTerm);    // Just ignore those, do not store in the number string
// Hex numbers                                                    
 this->Ranges.Add(csInNumExt,    'x','x', csInHexVal, sgNone, ttHexNum, ttNone, NLEX::tfNoTerm);     // Hex/Float
 this->Ranges.Add(csInNumExt,    'X','X', csInHexVal, sgNone, ttHexNum, ttNone, NLEX::tfNoTerm);     // Hex/Float
 this->Ranges.Add(csInHexVal,    '0','9', csInHexVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);
 this->Ranges.Add(csInHexVal,    'a','f', csInHexVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);
 this->Ranges.Add(csInHexVal,    'A','F', csInHexVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);
 this->Ranges.Add(csInHexVal,  '\'','\'', csInHexVal, sgNone, ttNone,   ttNone, NLEX::tfIgnore|NLEX::tfNoTerm);    // Just ignore those, do not store in the number string
// Turning hex numbers into hex floats                                              
 this->Ranges.Add(csInHexVal,    '.','.', csInHFlVal, sgNone, ttFltNum, ttNone, NLEX::tfNoTerm);     // Different state to allow only one '.'     (Cannot have hex floats like 'float x = 0x34.;'  - Why?)                                                 
 this->Ranges.Add(csInHFlVal,    '0','9', csInHFlVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);
 this->Ranges.Add(csInHFlVal,    'a','f', csInHFlVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);
 this->Ranges.Add(csInHFlVal,    'A','F', csInHFlVal, sgNone, ttNone,   ttNone, NLEX::tfNumeric);
 this->Ranges.Add(csInHFlVal,  '\'','\'', csInHFlVal, sgNone, ttNone,   ttNone, NLEX::tfIgnore|NLEX::tfNoTerm);    // Just ignore those, do not store in the number string                                                 
 this->Ranges.Add(csInHFlVal,    'p','p', csInFltExp, sgNone, ttFltExp, ttNone, NLEX::tfNoTerm);     // Same format of exponent as dec float
 this->Ranges.Add(csInHFlVal,    'P','P', csInFltExp, sgNone, ttFltExp, ttNone, NLEX::tfNoTerm);
}
//------------------------------------------------------------------------------------------------------------
void RegRanges_Scopes(void)
{

}
//------------------------------------------------------------------------------------------------------------
public:
void Initialize(void)
{
 //tkn.Initialize();
 this->RegRanges_WS();
 this->RegRanges_Specials();  // Must be after WS and before everything else
 this->RegRanges_Strings();
 this->RegRanges_Scopes();
 this->RegRanges_Names();
 this->RegRanges_Numbers();
}
//------------------------------------------------------------------------------------------------------------
sint ParseFile(const achar* Path)
{
 CArr<achar> Text;
 Text.FromFile(Path);
 if(Text.Length() < 1)return -9;
 //NLEX::SCtx<STRM::CStrmBuf> lctx{};
 this->Initialize();
 this->lex.Stream.Init(Text.Data(), Text.Size());
 this->lex.Init(&this->Ranges, 0);
 sint pres = this->lex.Tokenize();
 if(pres < 0)
  {
   achar* ErrVal = this->lex.GetErrVal();
   LOGMSG("Parsing failed: %i, State=%u, Val=%02X('%.*s') at {Line=%u, Pos=%u} in {Line=%u, Pos=%u}",-pres, this->lex.ErrCtx.State, *ErrVal, this->lex.ErrCtx.Size, ErrVal,  this->lex.ErrCtx.CurPos.Line+1, this->lex.ErrCtx.CurPos.Pos+1,  this->lex.ErrCtx.PrvPos.Line+1, this->lex.ErrCtx.PrvPos.Pos+1);
   return pres;
  }
   else {LOGMSG("Parsing OK: %i",pres);}
 return pres;
}
//------------------------------------------------------------------------------------------------------------
};