
#pragma once
/*
  Copyright (c) 2021 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// https://tip.golang.org/src/unicode/utf8/utf8.go
// http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
// https://github.com/BobSteagall/utf_utils
// https://chromium.googlesource.com/breakpad/breakpad/src/+/chrome_49/common/convert_UTF.c
// https://github.com/CppCon/CppCon2018/blob/master/Presentations/fast_conversion_from_utf8_with_cpp_dfas_and_sse_intrinsics/fast_conversion_from_utf8_with_cpp_dfas_and_sse_intrinsics__bob_steagall__cppcon_2018.pdf
// Specifically, the first byte holds two or more uppermost set bits,
// a zero bit, and some payload; the second and later bytes each start with
// their uppermost bit set, the next bit clear, and six bits of payload.
// Payload parcels are in big-endian order.  All bytes must be present in a
// valid sequence; i.e., low-order sezo bits must be explicit.  UTF-8 is
// self-synchronizing on input as any byte value cannot be both a valid
// first byte or trailing byte.
//
// 0xxxxxxx - 7 bit ASCII                                                           0x00000000 - 0x0000007F
// 110xxxxx 10xxxxxx - 11-bit value                                                 0x00000080 - 0x000007FF
// 1110xxxx 10xxxxxx 10xxxxxx - 16-bit value                                        0x00000800 - 0x0000FFFF
// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx - 21-bit value                               0x00010000 - 0x0010FFFF  // Standard UTF-8 caps at 0x10FFFF, but structurally could encode up to 0x1FFFFF
// - Explicitly forbidden in RFC 3629 (2003)  ( Unicode was limited to U+10FFFF)    
// 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx - 26-bit value                      0x00200000 - 0x03FFFFFF
// 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx - 31-bit value             0x04000000 - 0x7FFFFFFF
// 11111110 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx - 36-bit value    0x80000000 - 0xFFFFFFFF  // U32 Overflow   (Max 32 bits, wastes a byte to get +1 bit)
// 
// The History:
//   The beautiful mathematical elegance (simple bit counting) was deliberately broken for practical reasons:
//      Security hardening
//      Matching Unicode's artificial ceiling
//      Faster validation (don't need to handle 5-6 byte sequences)
//   // TODO: Size is {Invert and count leading zeroes}
// #define UNICODE_VALID(Char)  ((Char) < 0x110000 && (((Char) & 0xFFFFF800) != 0xD800))
//===========================================================================
struct NUTF
{    // NOTE: Streams should emulate array access
enum EUtf8Err
{
 u8eTruncatedAt1           = -1, // Byte 2 not a continuation
 u8eTruncatedAt2           = -2, // Byte 3 not a continuation
 u8eTruncatedAt3           = -3, // Byte 4 not a continuation
 u8eOverlong               = -4,
 u8eOutOfRange             = -5,
 u8eTooLongCodepoint       = -6,
 u8eUnExContinuation       = -7,
 };

// For UTF-16
SCVR uint32 highBegin   = 0xD800;     // D800 - DB7F : High Surrogates
SCVR uint32 highEnd     = 0xDBFF;
SCVR uint32 lowBegin    = 0xDC00;     // DC00 - DFFF : Low Surrogates
SCVR uint32 lowEnd      = 0xDFFF;
SCVR uint32 maxbmp      = 0x0000FFFF;
SCVR uint32 base        = 0x00010000;
SCVR uint32 mask        = 0x3FF;      // For 10 bits
SCVR int    shift       = 10;

SCVR uint32 MaxUtf32    = 0x0010FFFF;
SCVR uint32 IllegalChar = 0x0000FFFD; 
//---------------------------------------------------------------------------
static uint CpSizeUtf8(uint8 LeadChr)
{
 return (LeadChr >= 0xC0)?(clz<true>(uint8(~LeadChr))):(1);    // Will threat continuation bytes as 1 in size
}    
//---------------------------------------------------------------------------
// Tests only the UTF-8 data stream validity with basic RFC constraints
// Return: Positive - Codepoint size; Negative - Error code
//
template<typename TSrc> int CpValidateUtf8(TSrc Src, uint SrcIdx=0)
{
 uint8 c1 = uint8(Src[SrcIdx]);     // Must not do 'SrcIdx++' here or it will be precomputed into a return value and make the first case (case 4) slow because of extra index calculations                                      
 if(sint8(c1) < 0)  [[unlikely]]            
  {     
   uint8 elen = clz<true>(uint8(~c1));        // 2 - 7      // Note C1 may be 0xFF and fail with BSR
   uint32 chr = c1 & (0xFF >> elen);   // First byte's bits 
   switch(elen)    // Compose UTF-32 character   // Intentional fallthrough to accumulate bits  // Put [[fallthrough]] on every case? Nah, it should be applicable to the entire switch too.
    {
     default: UNREACHABLE();      // This removes unnecessary out-of-range check
    [[unlikely]] case 8:   // 0xFF is not UTF-8 at all?              // Those cases should be [[unlikely]] or the IllegalChar will be put before the 'switch' wasting a register and performance
    [[unlikely]] case 7:   // Unused in UTF-8 
    [[unlikely]] case 6:   // Unused in UTF-8 
    [[unlikely]] case 5:   // Unused in UTF-8 
        return u8eTooLongCodepoint;
    [[unlikely]] case 1:   // This is an unexpected continuation byte 
    [[unlikely]] case 0:   // Should not get here (the char was 0)  
        return u8eUnExContinuation;
     case 4:     // Four-byte is max valid UTF-8 by RFC 
        c1 = uint8(Src[++SrcIdx]);
        if((c1 >> 6) != 2)return u8eTruncatedAt1 - (elen-4);
        chr = (chr << 6) | (c1 & 0x3F); 
     case 3:
        c1 = uint8(Src[++SrcIdx]);
        if((c1 >> 6) != 2)return u8eTruncatedAt1 - (elen-3);
        chr = (chr << 6) | (c1 & 0x3F); 
     case 2:
        c1 = uint8(Src[++SrcIdx]);
        if((c1 >> 6) != 2)return u8eTruncatedAt1 - (elen-2);
        chr = (chr << 6) | (c1 & 0x3F);
    }  
   if(chr > MaxUtf32)return u8eOutOfRange;
   SCVR uint32 CpLimits[] = {0x0, 0x0, 0x80, 0x800, 0x10000};    
   if(chr < CpLimits[elen])return u8eOverlong;   
  }
   else [[likely]] { return 1; }    // ASCII - fast path    
}
//---------------------------------------------------------------------------
// Convert UTF-8 character to UTF-32
// Input:  1 - 4(6?) UTF-8 chars
// Output: 1 UTF-32 char
// Return: Updated SrcIdx (Number of source uint8 read if initial SrcIdx is 0)
// NOTE: No continuation validation (Only treating the first byte as IllegalChar if it is a continuation byte)           
// DO NOT MODIFY: Carefully optimized to tame the wild CodeGen
//                                    
template<typename TDst, typename TSrc> static uint CpUtf8To32(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)   
{
 using DType = typename RemoveRef<decltype(Dst[0])>::T;  // ???: static_assert on size of this type because it is expected to be of uint8 size? Or allow Dst to expand   // What type if multiple 'operator[]' is present?
 uint8 c1 = uint8(Src[SrcIdx]);     // Must not do 'SrcIdx++' here or it will be precomputed into a return value and make the first case (case 4) slow because of extra index calculations                                      
 if(sint8(c1) < 0)  [[unlikely]]            
  {     
   uint8 elen = clz(uint8(~c1));        // 2 - 7      // Nonzero - safe with __builtin_clz
   uint32 chr = c1 & (0xFF >> elen);   // First byte's bits 
// Final: 2.3s  
   switch(elen)    // Compose UTF-32 character   // Intentional fallthrough to accumulate bits  // Put [[fallthrough]] on every case? Nah, it should be applicable to the entire switch too.
    {
     default: UNREACHABLE();      // This removes unnecessary out-of-range check
    [[unlikely]] case 8:   // 0xFF is not UTF-8 at all              // Those cases should be [[unlikely]] or the IllegalChar will be put before the 'switch' wasting a register and performance
    [[unlikely]] case 7:   // Unused in UTF-8 
    [[unlikely]] case 6:   // Unused in UTF-8 
    [[unlikely]] case 5:   // Unused in UTF-8 
    [[unlikely]] case 1:   // This is an unexpected continuation byte 
    [[unlikely]] case 0:   // Should not get here (the char was 0)  
        Dst[DstIdx] = DType(IllegalChar);   
        return ++SrcIdx;
/*    
     case 7:   // IllegalChar
        chr = (chr << 6) | (uint8(Src[SrcIdx++]) & 0x3F); 
     case 6:   // IllegalChar
        chr = (chr << 6) | (uint8(Src[SrcIdx++]) & 0x3F);      
     case 5:   // IllegalChar             // Had to enable this case
        chr = (chr << 6) | (uint8(Src[SrcIdx++]) & 0x3F);  */ 
     case 4:     // Four-byte is max valid UTF-8 by RFC      
        chr = (chr << 6) | (uint8(Src[++SrcIdx]) & 0x3F); 
     case 3:
        chr = (chr << 6) | (uint8(Src[++SrcIdx]) & 0x3F); 
     case 2:
        chr = (chr << 6) | (uint8(Src[++SrcIdx]) & 0x3F);
    }     
   Dst[DstIdx] = DType(chr); 
  }
   else [[likely]] { Dst[DstIdx] = DType(c1); }    // ASCII - fast path     
 return ++SrcIdx;
}
//---------------------------------------------------------------------------   //  -flto  
// Convert UTF-32 character to UTF-8
// Input:  1 UTF-32 char
// Output: 1 - 4 UTF-8 chars
// Return: Updated DstIdx (Number of destination uint8 written if initial DstIdx is 0)
//
template<typename TDst, typename TSrc> static uint CpUtf32To8(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)
{
 SCVR uint32 contmrk = 0x80;
 SCVR uint32 contmsk = 0x3F;
 using DType = typename RemoveRef<decltype(Dst[0])>::T;  // ???: static_assert on size of this type because it is expected to be of uint8 size? Or allow Dst to expand   // What type if multiple 'operator[]' is present?
 uint32  chr = uint32(Src[SrcIdx]);   // No 0 detection here!
 if(chr >= 0x80)  [[unlikely]]    
  {
//  if(chr > MaxUtf32)chr = IllegalChar;      // NOTE: This check is not free
  int len = (chr >= 0x800) + (chr >= 0x10000) + 1;    // Max 4 bytes  
  switch(len)
   {
    default: UNREACHABLE();      // This removes unnecessary out-of-range check
    case 3:    // 3 extra bytes   
//      if(chr > MaxUtf32){chr = IllegalChar; goto BadChar;}       
      Dst[DstIdx++] = DType(uint8((chr >> 18) | 0xF0));  
      Dst[DstIdx++] = DType(uint8(((chr >> 12) & contmsk) | contmrk));
      Dst[DstIdx++] = DType(uint8(((chr >> 6) & contmsk) | contmrk));
      Dst[DstIdx++] = DType(uint8((chr & contmsk) | contmrk));
      break;
    case 2:    // 2 extra bytes   
//      if(chr > MaxUtf32)chr = IllegalChar; 
//BadChar:     
      Dst[DstIdx++] = DType(uint8((chr >> 12) | 0xE0));  
      Dst[DstIdx++] = DType(uint8(((chr >> 6) & contmsk) | contmrk));
      Dst[DstIdx++] = DType(uint8((chr & contmsk) | contmrk));
      break;
    case 1:    // 1 extra byte       // NOTE: Do not mark with [[likely]] - slower
    case 0:    // will not happen (just keeps the first jump table slot occupied)
      Dst[DstIdx++] = DType(uint8((chr >> 6) | 0xC0));  
      Dst[DstIdx++] = DType(uint8((chr & contmsk) | contmrk));
      break;
   }   
  /* int sft = len * 6; //    // Start from highest 6 bits
   uint8 msk = (0xFF << (7-len));    
   Dst[DstIdx++] = ((chr >> sft) | msk);   // The first byte
   switch(len)
    {
     default: UNREACHABLE();      // This removes unnecessary out-of-range check
     case 3:    // 3 extra bytes         
       Dst[DstIdx++] = DType(uint8(((chr >> 12) & contmsk) | contmrk)); 
     case 2:    // 2 extra bytes        
       Dst[DstIdx++] = DType(uint8(((chr >> 6) & contmsk) | contmrk));
     case 1:    // 1 extra byte
     case 0:    // will not happen (just keeps the first jump table slot occupied)
       Dst[DstIdx++] = DType(uint8((chr & contmsk) | contmrk));  // Low 6 bits
    };  */
  }
   else [[likely]] { Dst[DstIdx++] = DType(chr); }    // ASCII fast path       
 return DstIdx;
}
//---------------------------------------------------------------------------  
// Return: Number of destination uint8 written
template<typename TSrc> static uint CpLen32To8(TSrc Src, uint SrcIdx=0)
{
 uint32 c = uint32(Src[SrcIdx]);   // No 0 detection here!
 uint DstLen = 0;
 if(c < 0x80) [[likely]] DstLen++;   // 1 byte         // Removed 'switch' with backward indexing to make it stream friendly        //  0     1     2     3     4     5     6
 else if(c < 0x800)DstLen += 2;  // 2 bytes
 else if(c < 0x10000)DstLen += 3;  // 3 bytes
 else if(c <= MaxUtf32)DstLen += 4;   // 4 bytes
 else DstLen += 3;  // 3 bytes, IllegalChar
 return DstLen;
}
//---------------------------------------------------------------------------
// Convert UTF-32 character to UTF-16
// Input:  1 UTF-32 char
// Output: 1 - 2 UTF-16 chars
// Return: Updated DstIdx (Number of destination uint16 written if initial DstIdx is 0)
// 
// NOTE: Validations are disabled due to frequent use on Windows (File system API)
//
// Unicode's High Surrogates block, covering the codepoints D800 to DB7F, is reserved for use in UTF-16 encoding to help represent characters 
// outside the Basic Multilingual Plane (i.e., characters with codepoints above U+FFFF). These codepoints do not represent actual characters directly. 
// Instead, each high surrogate is paired with a low surrogate (from DC00 to DFFF) to encode a single supplementary character by combining their values in a surrogate pair.
//
template<typename TDst, typename TSrc> static uint CpUtf32To16(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)
{
 uint32 chr = uint32(Src[SrcIdx]);    // No 0 detection here!
 if(chr <= maxbmp)      // Single 16-bit codepoint  
  {
//   if((chr >= highBegin) && (chr <= lowBegin))Dst[DstIdx++] = IllegalChar;    // D800 - DB7F : High Surrogates  (Should not be present in Utf32 chars)
//      else 
   Dst[DstIdx++] = uint16(chr);
  }
 //  else if(chr > MaxUtf32)Dst[DstIdx++] = IllegalChar;      // Not helpful
     else
      {
       chr -= base;
       Dst[DstIdx++] = uint16((chr >> shift) + highBegin);
       Dst[DstIdx++] = uint16((chr & mask) + lowBegin);
      }
 return DstIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-16 characters to UTF-32
// Input:  1 - 2 UTF-16 chars
// Output: 1 UTF-32 char
// Return: Updated SrcIdx (Number of source uint16 read if initial SrcIdx is 0)
//
// NOTE: Validations are disabled due to frequent use on Windows (File system API)
//
template<typename TDst, typename TSrc> static uint CpUtf16To32(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)   // TODO: Optimize (It is called in a loop)
{
 const uint32 c1 = uint16(Src[SrcIdx++]);
 if((c1 >= highBegin) && (c1 <= highEnd))
  {
   const uint32 c2 = uint16(Src[SrcIdx++]);   // Take another uint16. 0 will be invalid here
  // if((c2 >= lowBegin) && (c2 <= lowEnd))
   Dst[DstIdx] = ((c1 - highBegin) << shift) + (c2 - lowBegin) + base;    // Merge the pair
  //   else Dst[DstIdx] = IllegalChars[!(bool)c2];   // Write 0 if c2 is 0   ???
  }
 //  else if((c1 >= lowBegin) && (c1 <= lowEnd))Dst[DstIdx] = IllegalChar;   // Not 0
      else Dst[DstIdx] = c1;   // May be 0
 return SrcIdx;         
}
//---------------------------------------------------------------------------
// Convert UTF-8 character to UTF-16
// Input:  1 - 4(6?) UTF-16 chars
// Output: 1 - 2 UTF-16 chars
// Return: Original char value
template<typename TDst, typename TSrc> static _finline uint32 CpUtf8To16(TDst Dst, TSrc Src, uint& DstIdx, uint& SrcIdx)
{
 uint32 Val;
 SrcIdx = CpUtf8To32(&Val, Src, 0, SrcIdx);
 DstIdx = CpUtf32To16(Dst, &Val, DstIdx, 0);
 return Val;
}
//---------------------------------------------------------------------------
// Convert UTF-16 character to UTF-8
// Input:  1 - 2 UTF-16 chars
// Output: 1 - 4(6?) UTF-16 chars
// Return: Original char value
template<typename TDst, typename TSrc> static _finline uint32 CpUtf16To8(TDst Dst, TSrc Src, uint& DstIdx, uint& SrcIdx)
{
 uint32 Val;
 SrcIdx = CpUtf16To32(&Val, Src, 0, SrcIdx);
 DstIdx = CpUtf32To8(Dst, &Val, DstIdx, 0);
 return Val;
}
//===========================================================================
//---------------------------------------------------------------------------
// Return: Number of destination uint16 chars
//
// NOTE: Validations are disabled due to frequent use on Windows (File system API)
//
template<typename TSrc> static uint CpLen32To16(TSrc Src, uint SrcIdx=0)
{
 uint32 c = uint32(Src[SrcIdx]);    // No 0 detection here!
 uint DstLen = 0;
 if(c <= maxbmp)
  {
 //  if((c >= highBegin) && (c <= lowBegin))DstLen++;
 //     else 
   DstLen++;
  }
 //  else if(c > MaxUtf32)DstLen++;
     else DstLen += 2;
 return DstLen;
}
//---------------------------------------------------------------------------
/*template<typename TSrc> static uint CpLen16To32(TSrc Src, uint SrcIdx=0)
{
 return 1;
}
//---------------------------------------------------------------------------
template<typename TSrc> static uint CpLen8To32(TSrc Src, uint SrcIdx=0)
{
 return 1;
}
//---------------------------------------------------------------------------
template<typename TSrc> static uint32 CpLen8To16(TSrc Src, uint& SrcIdx)
{
 uint32 Val;
 SrcIdx += CpLen8To32(Src, SrcIdx);
 DstIdx += 
 return CpLen32To16(&Val);
}
//---------------------------------------------------------------------------
template<typename TSrc> static uint32 CpLen16To8(TSrc Src, uint& SrcIdx)
{
 uint32 Val;
 SrcIdx = CpUtf16To32(&Val, Src, 0, SrcIdx);
 DstIdx = CpUtf32To8(Dst, &Val, DstIdx);
 return Val;
}*/
//===========================================================================
// NOTE: Streams expected to return 0 when reading beyond their end and no more data is available
//---------------------------------------------------------------------------
// Convert UTF-32 to UTF-16
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf32To16(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val = Src[SrcIdx++];
   if(!Val)break;
   DstIdx = CpUtf32To16(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-16 to UTF-32
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf16To32(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx = CpUtf16To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   Dst[DstIdx++] = Val;
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-32 to UTF-8
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf32To8(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val = Src[SrcIdx++];
   if(!Val)break;
   DstIdx = CpUtf32To8(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-8 to UTF-32
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf8To32(TDst Dst, TSrc Src, size_t DstChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;DstChrCnt;DstChrCnt--)
  {
   uint32 Val;
   SrcIdx = CpUtf8To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;        // Not writing the 0?
   Dst[DstIdx++] = Val;
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-16 to UTF-8
// Return: Number of chars written to Dst (May be more DST bytes than SRC chars)
template<typename TDst, typename TSrc> static size_t Utf16To8(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx = CpUtf16To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   DstIdx = CpUtf32To8(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// NOTE: DstLen/SrcLen is max number of chars to wrire/read not actual sizes of Dst/Src (which may even be streams with operator[] (Please don't do such loops on streams directly, do it on their buffer iterators))
template<typename TDst, typename TSrc> static size_t Utf16To8(TDst Dst, TSrc Src, size_t DstLen, size_t SrcLen, uint& DstIdx, uint& SrcIdx)    // TODO: Make more of this kind (DstLen/SrcLen)
{
 uint vDstIdx = DstIdx;    // Can we tell the compiler that it is OK to cache such references?
 uint vSrcIdx = SrcIdx;
 uint DstEnd  = DstIdx + DstLen;
 uint SrcEnd  = SrcIdx + SrcLen;
 while((vSrcIdx < SrcEnd)&&(vDstIdx < DstEnd))
  {
   uint32 Val;
   vSrcIdx = CpUtf16To32(&Val, Src, 0, vSrcIdx);
   if(!Val)break;
   vDstIdx = CpUtf32To8(Dst, &Val, vDstIdx, 0);
  }
 size_t ODIdx = DstIdx;
 DstIdx = vDstIdx;
 SrcIdx = vSrcIdx;
 return vDstIdx - ODIdx;    // Number of DST bytes
}
//---------------------------------------------------------------------------
// Convert UTF-8 to UTF-16
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf8To16(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx = CpUtf8To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   DstIdx = CpUtf32To16(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//===========================================================================
//---------------------------------------------------------------------------
template<typename TSrc> static size_t Len32To16(TSrc Src, size_t SrcChrCnt=(size_t)-1, uint SrcIdx=0)
{
 uint DstLen = 0;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val = Src[SrcIdx++];
   if(!Val)break;
   DstLen += CpLen32To16(&Val);
  }
 return DstLen;
}
//---------------------------------------------------------------------------
template<typename TSrc> static size_t Len16To32(TSrc Src, size_t SrcChrCnt=(size_t)-1, uint SrcIdx=0)
{
 uint DstLen = 0;
 for(;SrcChrCnt;SrcChrCnt--,DstLen++)
  {
   uint32 Val;
   SrcIdx = CpUtf16To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
  }
 return DstLen;
}
//---------------------------------------------------------------------------
template<typename TSrc> static size_t Len32To8(TSrc Src, size_t SrcChrCnt=(size_t)-1, uint SrcIdx=0)
{
 uint DstLen = 0;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val = Src[SrcIdx++];
   if(!Val)break;
   DstLen += CpLen32To8(&Val);
  }
 return DstLen;
}
//---------------------------------------------------------------------------
template<typename TSrc> static size_t Len8To32(TSrc Src, usize SrcByteCnt=(size_t)-1, usize SrcIdx=0)
{
 if(!SrcByteCnt)return 0;
 uint DstLen = 0;
 for(;;)
  {
   uint8 val = Src[SrcIdx++];
   if(!val)break;      // Null char
   uint len = CpSizeUtf8(val);
   if(len >= SrcByteCnt){DstLen += (len == SrcByteCnt); break;}  // No more complete codepoints to read
   SrcByteCnt -= len;          // Overload to avoid all of this when not needed?
   DstLen++;
  }
 return DstLen;
}
//---------------------------------------------------------------------------
template<typename TSrc> static size_t Len16To8(TSrc Src, size_t SrcChrCnt=(size_t)-1, uint SrcIdx=0)
{
 uint DstLen = 0;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx = CpUtf16To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   DstLen += CpLen32To8(&Val);
  }
 return DstLen;
}
//---------------------------------------------------------------------------
template<typename TSrc> static size_t Len8To16(TSrc Src, size_t SrcChrCnt=(size_t)-1, uint SrcIdx=0)
{
 uint DstLen = 0;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx = CpUtf8To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   DstLen += CpLen32To16(&Val);
  }
 return DstLen;
}
//---------------------------------------------------------------------------
#include "Unicode.hpp"  // Unicode normalization and case conversion (Directly in UTF-8 encoding)
};
//---------------------------------------------------------------------------
