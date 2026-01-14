
// https://www.unicode.org/Public/UNIDATA/
// https://www.unicode.org/Public/UCD/latest/ucd/
// https://www.w3.org/TR/charmod-norm/
// https://jrgraphix.net/research/unicode_blocks.php      // Incomplete
// https://unicode-explorer.com/blocks
// 
// https://www.b-list.org/weblog/2018/feb/11/usernames/
// 
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2728r0.html
// 
// http://utf8everywhere.org/
// https://www.unicode.org/reports/tr9/
// https://learn.microsoft.com/en-us/globalization/fonts-layout/text-directionality
// 
// Case conversions and composite chars support
// https://www.b-list.org/weblog/2018/nov/26/case/
// https://www.unicode.org/Public/16.0.0/ucd/CaseFolding.txt
// https://stackoverflow.com/questions/36897781/how-to-uppercase-lowercase-utf-8-characters-in-c
// Complete implementation in GO:
//    https://tip.golang.org/src/unicode/graphic.go
//    https://tip.golang.org/src/unicode/letter.go
//    https://tip.golang.org/src/unicode/tables.go
// Case folding is just a way to normalize to disregard case. Whether it's more like upper-casing or lower-casing doesn't matter.
// https://stackoverflow.com/questions/16467479/normalizing-unicode
// NFC, or 'Normal Form Composed' returns composed characters, NFD, 'Normal Form Decomposed' gives you decomposed, combined characters.
// Using either NFKC or NFKD form, in addition to composing or decomposing characters, will also replace all 'compatibility' characters with their canonical form.
// TOO BIG TABLES:
//   https://docs.python.org/3/library/unicodedata.html#unicodedata.normalize
//   https://github.com/python/cpython/blob/main/Modules/unicodedata.c
//   https://github.com/python/cpython/blob/main/Modules/unicodedata_db.h
//   https://github.com/python/cpython/blob/main/Modules/unicodename_db.h
// 
// https://www.reddit.com/r/programming/comments/b09c0j/when_zo%C3%AB_zo%C3%AB_or_why_you_need_to_normalize_unicode/
// the recommendation when normalizing strings for comparison, especially if those strings are intended for use as identifiers, should be to normalize to NFKC, not NFC
// 
// http://juliastrings.github.io/utf8proc/
// https://github.com/JuliaStrings/utf8proc
// 
// Too many macros:
//   https://github.com/GNOME/glib/blob/main/glib/gunidecomp.c
//   https://github.com/GNOME/glib/blob/main/glib/guniprop.c
// 
// Too complete:
//  https://github.com/libogonek/ogonek
// 
// https://github.com/artichoke/focaccia/blob/trunk/src/folding/mapping/lookup.rs
// 
// https://unicode.org/ucd/
// http://www.unicode.org/Public/UNIDATA/UnicodeData.txt
// HEADER: Code_Point;Name;General_Category;Canonical_Combining_Class;Bidi_Class;Decomposition_Type_and_Decomposition_Mapping;Numeric_Type;Numeric_Value_for_Type_Digit;Numeric_Value_for_Type_Numeric;Bidi_Mirrored;Unicode_1_Name;ISO_Comment;Simple_Uppercase_Mapping;Simple_Lowercase_Mapping;Simple_Titlecase_Mapping
// 
// http://0x80.pl/notesen/2025-02-02-utf32-change-case.html   // Complete research!    // SRC: https://github.com/WojciechMula/toys/tree/master/utf32-change-case
// 
// https://hjlebbink.github.io/x86doc/html/VPGATHERDD_VPGATHERQD.html
// https://kevinboone.me/overlong.html
// 
// https://www.compart.com/en/unicode/category/Lt
// 
// Normalization:
//  canonical composition is strictly limited to two-code-point pairs only.
//  No direct 3+ code point compositions exist in the standard. Multi-mark sequences compose stepwise: e.g., A + acute + grave → (A+acute) + grave → Á + grave → final form.s
//  Decompositions can be longer than 2 (e.g., Hangul syllables decompose to 2-3 jamo), but composition is always the reverse: iteratively reducing adjacent pairs back to length 2 → 1.
// NFD: stop after decomposition
// NFD does not run any composition step.
//    The final result stays as fully decomposed, ordered code points.​
//    Example: precomposed é (U+00E9) becomes e + ◌́ (U+0065 U+0301) and remains that way in NFD.
// 
// NFC: decompose, then recompose eligible pairs
//   NFC adds a canonical composition phase:​
//   After decomposition + reordering, it scans left‑to‑right.
//   Whenever it sees a (starter, combining) pair that:
//   Has a canonical composition mapping, and
//   Passes the blocking / CCC rules,it replaces the pair with the single composed code point.
// 
// So for the same input é:
// Decomposition step (same as NFD): 00E9 → 0065 0301.
// Composition step: sees (0065, 0301) has a composed form, replaces with 00E9.
// Final NFC output: U+00E9 again, i.e., precomposed where possible.​
// If no composition mapping exists (or composition is blocked by combining classes), NFC leaves the decomposed sequence as‑is, so NFC and NFD can sometimes match.
//
// NFKC and NFKD use compatibility decomposition instead of just canonical, then follow the same compose/decompose pattern as NFC/NFD.
// The key difference: compatibility decomposition
// NFC/NFD: Only canonical decompositions (semantically identical, like é → e + acute).​
// NFKC/NFKD: Canonical + compatibility decompositions (includes "similar but not identical" mappings).​
// Compatibility mappings expand things like:
// Ligatures: ﬁ → f + i
// Full-width: Ａ → A
// Superscripts: ² → 2
// Circled numbers: ① → 1
// Variant forms (small kana, etc.)​
// NFKD: compatibility decompose only
// Full compatibility decomposition on every code point.
// Canonical reordering of combining marks.
// No composition step. 
// 
// Use NFC for preserving semantic/visual distinctions; use NFKC when matching, searching, or normalizing diverse user input where compatibility variants should be treated as the same.
// Windows uses case folding (Unicode case folding) + NFKC normalization for case-insensitive file path comparisons on NTFS.
// Use Case	Recommended Form	Reason
// General text storage	NFC	Compact, preserves visual appearance
// Accent-insensitive search	NFD then strip marks	Easy to remove combining characters
// Full-text search	NFKC	Matches variant representations
// Security (username comparison)	NFKC	Prevents homograph attacks
// Preserving formatting	NFC	Keeps ligatures and special forms 
// 
// https://tonsky.me/blog/unicode/
//  F0000 - 10FFFF = Private use
// 
// NFD tries to explode everything to the smallest possible pieces, and also sorts pieces in a canonical order if there is more than one.
// NFC tries to combine everything into pre-composed form if one exists.
// NFKD tries to explode everything and replaces visual variants with default ones.
// NFKC tries to combine everything while replacing visual variants with default ones.
// 
// 
// https://www.unicode.org/reports/tr15/
// https://github.com/JuliaStrings/utf8proc
// https://github.com/blackwinter/unicode
// https://github.com/railgunlabs/judo  // JSON5 
// 
// Main decomposition tags. When no tag is present the mapping is treated as <canonical>, used by NFC/NFD and NFKC/NFKD
// Unicode defines a fixed set of compatibility formatting tags that can appear in angle brackets before the decomposition mapping, such as:​
//    <initial> – Arabic initial presentation form
//    <medial> – Arabic medial presentation form
//    <final> – Arabic final presentation form
//    <isolated> – Arabic isolated presentation form
//    <circle> – encircled form
//    <square> – CJK squared form
//    <vertical> – vertical layout form
//    <wide> / <narrow> – fullwidth / halfwidth compatibility forms
//    <small> – small variant form
//    <super> / <sub> – superscript / subscript forms
//    <fraction> – precomposed vulgar fractions
//    <noBreak> – non-breaking forms
//    <compat> – "other" compatibility mapping when no more specific tag fits
// 
// TODO: Attach 16-bit mask to each codepoint specifying which mappings to it exist
// 
// Case folding statuses
// Case folding has these status letters in CaseFolding.txt:​
//   C – common mapping (shared by simple and full folding).
//   F – full mapping (may expand to multiple code points).
//   S – simple mapping (1‑to‑1, when it differs from full).
//   T – special handling for dotted/dotless I; optional.
// 
// So there isn’t one single "category" there either, but a small set of status values, separate from the normalization tags. All code points not listed in CaseFolding.txt are implicitly status C and fold to themselves
//------------------------------------------------------------------------------------------------------------
// Implementation notes:
//   No transcoding to UTF-32
//   Case folding and normalization is used only for unit comparison (Strings are compared unit by unit)
//   Case conversions preserve UTF-8 code point size even if output an incorrect result (Because UTF-8 strings must be case converted inplace)
// 
// CP_SIZE -> Tables 0-2 [sizes: 2,3,4] [Table 0 is 32 entries, Table 1,2 is 64] (Table 2 skips first byte - no alterable data bits in it, always 0xf0)
// ------------------
// 00 - Not present
// 01 - uint8
// 10 - uint16
// 11 - uint32
// xx
// xx
// xx
// xx
//------------------------------------------------------------------------------------------------------------
//struct NUNI
//{
//------------------------------------------------------------------------------------------------------------
// The stream starts from the first VarUint RLE counter
// IsVarInt: Treat the value as a variable integer and ulen as size of units VarInt counts
// ulen: any number of bits up to 64 (Or 128 if i128 is supported)
// 00,01,10,110, 11100, 11101, 11110, 111110, ...
/*template<uint ulen=8, bool IsVarInt=false, bool Signed=false> class CBitUStrm
{
 static_assert(!IsVarInt || (ulen >= 8));
 using UType = ... // uint8,uint16,uint32,uint64 unit representation to read/write
 uint8* Data;
 size_t Size;
 uint BitOffs;

 template<typename T=size_t, bool sign=false> ReadVarInt(void) { ...To Do... }
 template<typename T=size_t, bool sign=false> WriteVarInt(void) { ...To Do... }

public:
 CBitUStrm(void){}
 void   Init(vptr Stream, size_t Length){this->Data = (uint8*)Stream; this->Size = Length; this->Reset();}
 void   Reset(void) {this->BitOffs = 0; ...To Do... }
 auto   ReadUnit(void){ ...To Do... }
 void   WriteUnit(UType Values){ ...To Do... }
 size_t ReadUnits(UType* Values, size_t Count){ ...To Do... }
 size_t WriteUnits(const UType* Values, size_t Count) { ...To Do... }
};   */
//------------------------------------------------------------------------------------------------------------
// Composition aware (one unit may be composed of several UTF-8 code points)
static int UnitSizeUtf8(const achar* chr)
{
 // TODO
 return false;
}
//------------------------------------------------------------------------------------------------------------
// Case aware, if required
static bool IsUnitsEqualUtf8(const achar* chra, const achar* chrb, bool cins=false, int* lena=nullptr, int* lenb=nullptr)
{
 // TODO
 return false;
}  
//------------------------------------------------------------------------------------------------------------
//};
/*
// CP_SIZE tables (precomputed!)
// Table 0 (32 entries): 2-byte units  
static constexpr uint16 cp_size2[32] = { lead byte ? unit size  };
// Table 1 (64 entries): 3-byte units  
static constexpr uint16 cp_size3[64] = {  lead byte ? unit size  };
// Table 2 (64 entries): 4-byte units (always 0xF0 lead) 
static constexpr uint16 cp_size4[64] = {  second byte ? unit size };

static int UnitSizeUtf8(const achar* chr) {
    uint8 lead = uint8(chr[0]);
    
    if (lead < 0x80) return 1;  // ASCII = 1 unit
    
    int tsize = clz<true>(~lead) - 1;  // 1?2byte, 2?3byte, 3?4byte
    if (tsize == 1) return 2 + cp_size2[lead >> 3][1];      // Table 0
    if (tsize == 2) return 3 + cp_size3[lead >> 2][2];      // Table 1  
    return 4 + cp_size4[chr[1]][3];                         // Table 2
}

static bool IsUnitsEqualUtf8(const achar* chra, const achar* chrb, bool cins, int* lena, int* lenb) {
    int sizea = UnitSizeUtf8(chra);
    int sizeb = UnitSizeUtf8(chrb);
    
    if (sizea != sizeb) return false;
    if (lena) *lena = sizea; if (lenb) *lenb = sizeb;
    
    // Case-insensitive: simple fold lead byte (ASCII + lead bits only)
    if (cins) {
        uint8 lowa = tolower_fold(chra[0]);
        uint8 lowb = tolower_fold(chrb[0]);
        if (lowa != lowb) return false;
    } else if (chra[0] != chrb[0]) {
        return false;
    }
    
    // Fast unit compare (preserves UTF-8 structure)
    return memcmp(chra, chrb, sizea) == 0;
}
Table generation ideas (precompute offline):

text
CP_SIZE[0][lead>>3] = {
    0: "LATIN_A?GREEK_ALPHA" ? +1 (combining unit)  
    1: "Cyrillic_A" ? 0 (standalone)
    2: "Hangul_L" ? +2 (needs V+T for full syllable)
    ...
}
*/
//------------------------------------------------------------------------------------------------------------
