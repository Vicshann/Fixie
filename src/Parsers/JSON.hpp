/*
https://www.tbray.org/ongoing/When/201x/2017/12/14/rfc8259.html
https://datatracker.ietf.org/doc/html/rfc6901
https://github.com/Tencent/rapidjson/blob/v1.1.0/doc/pointer.md
https://github.com/Tencent/rapidjson/blob/v1.1.0/doc/internals.md




*/
//------------------------------------------------------------------------------------------------------------
class CJSON
{

enum EChrStates    // Sorted by occurrence probability(may save some memory)   // 256 bytes per state record
{
 csBase = 0,
 csWhitespc,


 csInBOM1,        // BOM IS EVIL: https://stackoverflow.com/questions/2223882/whats-the-difference-between-utf-8-and-utf-8-with-bom
 csInBOM2,
};

NLEX::CLexer<STRM::CStrmBuf> lex;
NLEX::SRangeLst Ranges;  // May be Shared 
//------------------------------------------------------------------------------------------------------------
void RegisterRanges(void)
{

}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

};
//------------------------------------------------------------------------------------------------------------