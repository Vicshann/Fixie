
struct Test_Unicode
{
// Test sequence with various UTF-32 codepoints
// Each comment shows: codepoint value, character/description, UTF-8 byte length
// UTF-32 BE exists! Just like UTF-16, UTF-32 comes in three variants:
//    UTF-32 (with BOM to specify endianness)
//    UTF-32BE (explicit big-endian)
//    UTF-32LE (explicit little-endian)
// UTF-32 uses the BOM byte sequence 00 00 FE FF for big-endian or FF FE 00 00 for little-endian
//
static const inline uint32 test_set_utf32[] = {
    // 1-byte UTF-8 (ASCII range: 0x00-0x7F)
    0x00000041,  // 'A' - 1 byte
    0x0000007A,  // 'z' - 1 byte
    0x00000020,  // ' ' (space) - 1 byte
    
    // 2-byte UTF-8 (range: 0x80-0x7FF)
    0x000000A9,  // '©' (copyright) - 2 bytes: C2 A9
    0x000000F1,  // 'ñ' - 2 bytes: C3 B1
    0x000003B1,  // 'α' (alpha) - 2 bytes: CE B1
    0x000007FF,  // Maximum 2-byte - 2 bytes: DF BF
    
    // 3-byte UTF-8 (range: 0x800-0xFFFF)
    0x00000800,  // Minimum 3-byte - 3 bytes: E0 A0 80
    0x00002764,  // '❤' (heart) - 3 bytes: E2 9D A4
    0x00003042,  // 'あ' (Hiragana A) - 3 bytes: E3 81 82
    0x00004E2D,  // '中' (Chinese: middle) - 3 bytes: E4 B8 AD
    0x0000FFFD,  // '�' (replacement char) - 3 bytes: EF BF BD
    
    // 4-byte UTF-8 (range: 0x10000-0x10FFFF)
    0x00010000,  // Minimum 4-byte - 4 bytes: F0 90 80 80
    0x0001F600,  // '😀' (grinning face) - 4 bytes: F0 9F 98 80
    0x0001F4A9,  // '💩' (pile of poo) - 4 bytes: F0 9F 92 A9
    0x0001F680,  // '🚀' (rocket) - 4 bytes: F0 9F 9A 80
    0x0010FFFF,  // Maximum valid Unicode - 4 bytes: F4 8F BF BF
    
    // Invalid codepoints (> 0x10FFFF) - should map to replacement char
    0x00110000,  // Just beyond valid range
    0x00200000,  // Way beyond valid range
    0xFFFFFFFF,  // Maximum uint32

    0      // END
};


// Expected UTF-16 encoding (Little Endian - Windows default) //  UTF-16 big-endian is the default when there's no Byte Order Mark (BOM) according to RFC 2781
// BMP characters (U+0000 to U+FFFF) encode as single 16-bit units
// Supplementary characters (U+10000 to U+10FFFF) encode as surrogate pairs
static const inline uint16 test_set_utf16[] = {      // LE
    // 1-byte UTF-8 / BMP (ASCII range)
    0x0041,                    // 'A'
    0x007A,                    // 'z'
    0x0020,                    // ' '
    
    // 2-byte UTF-8 / BMP
    0x00A9,                    // '©'
    0x00F1,                    // 'ñ'
    0x03B1,                    // 'α'
    0x07FF,                    // Maximum 2-byte UTF-8
    
    // 3-byte UTF-8 / BMP
    0x0800,                    // Minimum 3-byte UTF-8
    0x2764,                    // '❤'
    0x3042,                    // 'あ'
    0x4E2D,                    // '中'
    0xFFFD,                    // '�'
    
    // 4-byte UTF-8 / Surrogate pairs (high surrogate, low surrogate)
    0xD800, 0xDC00,            // U+10000 (minimum 4-byte)
    0xD83D, 0xDE00,            // '😀' U+1F600
    0xD83D, 0xDCA9,            // '💩' U+1F4A9
    0xD83D, 0xDE80,            // '🚀' U+1F680
    0xDBFF, 0xDFFF,            // U+10FFFF (maximum valid Unicode)
    
    // Invalid codepoints → replacement character
    0xFFFD,                    // 0x110000 → '�'
    0xFFFD,                    // 0x200000 → '�'
    0xFFFD,                    // 0xFFFFFFFF → '�'

    0      // END
};

// Expected UTF-8 encoding (assuming replacement char U+FFFD = EF BF BD for invalid)
static const inline uint8 test_set_utf8[] = {
    // 1-byte sequences
    0x41,                    // 'A'
    0x7A,                    // 'z'
    0x20,                    // ' '
    
    // 2-byte sequences
    0xC2, 0xA9,              // '©'
    0xC3, 0xB1,              // 'ñ'
    0xCE, 0xB1,              // 'α'
    0xDF, 0xBF,              // Maximum 2-byte
    
    // 3-byte sequences
    0xE0, 0xA0, 0x80,        // Minimum 3-byte
    0xE2, 0x9D, 0xA4,        // '❤'
    0xE3, 0x81, 0x82,        // 'あ'
    0xE4, 0xB8, 0xAD,        // '中'
    0xEF, 0xBF, 0xBD,        // '�'
    
    // 4-byte sequences
    0xF0, 0x90, 0x80, 0x80,  // Minimum 4-byte
    0xF0, 0x9F, 0x98, 0x80,  // '😀'
    0xF0, 0x9F, 0x92, 0xA9,  // '💩'
    0xF0, 0x9F, 0x9A, 0x80,  // '🚀'
    0xF4, 0x8F, 0xBF, 0xBF,  // Maximum valid Unicode
    
    // Invalid codepoints → replacement character
    0xEF, 0xBF, 0xBD,        // 0x110000 → '�'
    0xEF, 0xBF, 0xBD,        // 0x200000 → '�'
    0xEF, 0xBF, 0xBD,        // 0xFFFFFFFF → '�'

    0      // END
};
//------------------------------------------------------------------------------------
static int DoTests(void)
{
 LOGMSG("Begin: UTF-8 to UTF-32");
 uint32 BufU32[128];         
 MOPR::MemFill<uint32>(BufU32,sizeof(BufU32),0);
 NUTF::Utf8To32(BufU32, test_set_utf8);
 RT_TEST(memcmp(BufU32, test_set_utf32, sizeof(test_set_utf32)-(4*4)), == 0,"Failed: UTF-8 decoding")

 LOGMSG("Begin: UTF-32 to UTF-8");
 uint8 BufU8[128];
 MOPR::MemFill<uint8>(BufU8,sizeof(BufU8),0);
 NUTF::Utf32To8(BufU8, test_set_utf32);
 RT_TEST(memcmp(BufU8, test_set_utf8, sizeof(test_set_utf8)-10), == 0,"Failed: UTF-8 encoding")

 LOGMSG("Done");

 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 SCVR uint MaxLoop = 100000;   // 200000

 CArr<uint8> bytes;
 achar FilePath[NPTM::PATH_MAX];  
 NSTR::StrCopy(FilePath, NTST::FwkRoot);
 NSTR::StrCnat(FilePath, "/Data/Unicode/utf-8-demo.txt");
 bytes.FromFile(FilePath);      
 uint dlen = bytes.Length();
 if(!dlen) {LOGMSG("Failed to open the file"); return -1;}

 NPTM::NDBG::STElapsed et;
 et.Reset();
 CArr<uint32> chars; 
 volatile uint8* Src = bytes.Data();
 volatile int ulen = 0;
 LOGMSG("Begin: calc Len8To32");
 for(uint ctr=0;ctr < MaxLoop;ctr++)ulen = NUTF::Len8To32(Src, dlen);
 et.Measure();

 chars.Resize(ulen+4);
 volatile uint32* Dst = chars.Data();

 LOGMSG("Begin: UTF-8 decoding");
 //*Src = 0xFF;
 for(uint ctr=0;ctr < MaxLoop;ctr++)NUTF::Utf8To32(Dst, Src, ulen);
 et.Measure();

 LOGMSG("Begin: calc Len32To8");
 for(uint ctr=0;ctr < MaxLoop;ctr++)ulen = NUTF::Len32To8(Dst, ulen);
 et.Measure();

 LOGMSG("Begin: UTF-8 encoding");
 et.Reset();
 for(uint ctr=0;ctr < MaxLoop;ctr++)NUTF::Utf32To8(Src, Dst, ulen);
 et.Measure();

 LOGMSG("Done");

 return 0;
}
//------------------------------------------------------------------------------------
};
