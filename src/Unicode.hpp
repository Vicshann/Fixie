
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
//------------------------------------------------------------------------------------------------------------
// Implementation notes:
//   No transcoding to UTF-32
//   Case folding and normalization is used only for unit comparision (Strings are compared unit by unit)
//   Case conversions preserve UTF-8 code point size even if output an incorrect result (Because UTF-8 strings must be case converted inplace)
// 
// CP_SIZE -> Tables 0-2 [sizes: 2,3,4] [Table 0 is 32 entries, Table 1,2 is 64] (Table 2 skips first byte - no alterable data bits in it, always 0xf0)
//------------------------------------------------------------------------------------------------------------
//struct NUNI
//{

//------------------------------------------------------------------------------------------------------------
// Composition aware (one unit may be composed of several UTF-8 code points)
static int UnitSizeUtf8(const achar* chr)
{
 // TODO
}
//------------------------------------------------------------------------------------------------------------
// Case aware, if required
static bool IsUnitsEqualUtf8(const achar* chra, const achar* chrb, bool cins=false, int* lena=nullptr, int* lenb=nullptr)
{
 // TODO
}
//------------------------------------------------------------------------------------------------------------
//};
//------------------------------------------------------------------------------------------------------------