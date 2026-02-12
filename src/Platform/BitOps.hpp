
#pragma once

// std::bit ( #include <bit> )
// NOTE: Duplicates in ArbiNum
//===========================================================================
// Count Leading Zeros
// 
// https://github.com/nemequ/portable-snippets
// https://github.com/mackron/refcode
//
// https://en.cppreference.com/w/cpp/numeric/countl_zero
// https://stackoverflow.com/questions/53443249/do-all-cpus-which-support-avx2-also-support-sse4-2-and-avx
// X86: MSVC: __lzcnt and __lzcnt64 (SSE4.2 2009) which is turned into LZCNT. LZCNT differs from BSR. For example, LZCNT will produce the operand size when the input operand is zero. It should be noted that on processors that do not support LZCNT, the instruction byte encoding is executed as BSR.
//      lzcnt counts, bsr returns the index or distance from bit 0 (which is the lsb)
//      GCC:  __builtin_clz and __builtin_clzll which expanded into bsr+xor 31/64 if no __attribute__((target("lzcnt"))) or -mlzcnt is specified
// ARM: (Ver 5) The CLZ instruction counts the number of leading zeros in the value in Rm and returns the result in Rd . The result value is 32 if no bits are set in the source register, and zero if bit 31 is set.
//      (defined(_M_ARM) && _M_ARM >= 5) || defined(__ARM_FEATURE_CLZ)
// NOTE: Use of LZCNT makes it slower for some reason
// NOTE: __builtin_clz with 0 input will return 32 when using LZCNT and 31 when using BSR on x86. On ARM it returns 32 for 0 input too.
// NOTE: With Clang (Not GCC) __builtin_clz( 0 ) is not valid compile time expression! They decided to base their implementation on behavior of a legacy X86 instruction (BSR) instead just adding this check when emitting BSR there if LZCNT is not enabled for the target.
// WARNING: Clang makes peculiar assumptions about values that originate from __builtin_clz and, for example, even with O1 generate 8 entries switch table for 9 cases!
// C:\WORKFOLDER\_MISC_SRC\LLVM_repo\llvm-project-main\llvm\lib\Analysis\ValueTracking.cpp (ref 'range metadata', 'Intrinsic::ctlz', PossibleLZ)
//
template<bool MayBeZero=true> constexpr static _finline unsigned int clz(auto Num)   // Won't inline with just 'inline' for some reason
{                 
 using T = decltype(Num);
#ifdef REAL_MSVC       // TODO: Remove this ugly mess?
 unsigned long Index;   // Is it changed by _BitScanForward in case of Zero input? If not we can store (sizeof(Num)*8) in it
 unsigned char res;
 if constexpr (sizeof(T) > sizeof(uint32))
  {
   if constexpr (__has_builtin(_BitScanReverse64))res = _BitScanBitScanReverse64(&Index, (unsigned long long)Num);    // ARM and AMD64
    else
     {
      res = _BitScanReverse(&Index, (unsigned long)(Num >> 32));
      if(!res)res = _BitScanReverse(&Index, (unsigned long)Num);
      Index += 32;
     }
  }
   else 
    {
     if constexpr (sizeof(T) < sizeof(uint32))res = _BitScanReverse(&Index, (unsigned long)Num) - ((sizeof(uint32)*8) - (sizeof(T)*8));  // Those bits must be aclually leading
       else res = _BitScanReverse(&Index, (unsigned long)Num);
    }
 if(res)return (unsigned int)UnbindVal(Index);    // Found 1 at some position
  else return (unsigned int)(sizeof(T)*8);  // Num is zero, all bits is zero
#else
 // NOTE: We must detect if building below SSE4.2 on x86 because the compiler will use BSR which requires special handling for 0 values.
#if defined(CPU_X86) && !defined(__LZCNT__)   // AVX2 is close to SSE4.2 and usually will support POPCNT and LZCNT (BSR will be emitted for __AVX__)  // NOTE: __SSE4_2__ may be defined but still no __LZCNT__
if constexpr (MayBeZero) { if(!Num)return BitSize<T>::V; } // Every bit is zero
#else     // This handling is always required. GCC have inconsistency in its runtime and constexpr __builtin_clz and Clang just refuses to compile __builtin_clz(0) as constexpr
if(IsConstexprCtx() && !Num)return BitSize<T>::V;  // clang will not accept __builtin_clz(0) in constexpr  // Should be optimized away when IsConstexprCtx is false // This will produce JE before BSR and it is faster than CMOV after
#endif
 if constexpr (sizeof(T) > sizeof(uint32))
  {
   if constexpr (sizeof(T) > sizeof(uint64))
    {
     if constexpr ((sizeof(T) > sizeof(u128)) || !IsScalar128<T>)  // Process as vector type in array
      {
       using N = decltype(Num[0]);
       int res = 0;
       for(int idx = 0; idx < (sizeof(T)/sizeof(N));idx++)  
        {
         N v = Num[idx];
         if(v)return clz<MayBeZero>((uint64)Num) + res; // Have some ONEs
          else res += (sizeof(N) * 8);
        }
       return res;
      }
       else    // Scalar i128
        {
         uint64 hi = (uint64)(Num >> 64);   
         return hi?(clz<MayBeZero>(hi)):(clz<MayBeZero>((uint64)Num) + 64);
        }
    }   
     else return (unsigned int)UnbindVal(__builtin_clzll((uint64)Num));  // T is 64bit // X64 CPUs only?   // Returns the number of leading 0-bits in x, starting at the most significant bit position. If x is 0, the result is undefined
  }
   else 
    {
     if constexpr (sizeof(T) < sizeof(uint32))return (unsigned int)UnbindVal(__builtin_clz((uint32)Num) - ((sizeof(uint32)*8) - (sizeof(T)*8)));   // Must drop extra bits     
       else return (unsigned int)UnbindVal(__builtin_clz((uint32)Num));
    }
#endif
/* else   // TODO: optimize?  // Rely on the optimizer's pattern matcher?
  {
    int i = 0;
    T v = 1 << ((sizeof(T) * 8)-1);
	for(;!(Num & v); v >>= 1, i++);
	return i;
  }
 return 0; */
}
//------------------------------------------------------------------------------------
// Count Trailing Zeros
// 
// X86: TZCNT counts the number of trailing least significant zero bits in source operand (second operand) and returns the result in destination operand (first operand). TZCNT is an extension of the BSF instruction. The key difference between TZCNT and BSF instruction is that TZCNT provides operand size as output when source operand is zero while in the case of BSF instruction, if source operand is zero, the content of destination operand are undefined. On processors that do not support TZCNT, the instruction byte encoding is executed as BSF.
// ARM: Uses 63/31 - CLZ(x). 0 is handled with a condition. 
// Note: Use '-mbmi' to force TZCNT
//
template<bool MayBeZero=true> constexpr static inline unsigned int ctz(auto Num)
{
 using T = decltype(Num);
#ifdef REAL_MSVC          // TODO: Remove this ugly mess?
 unsigned long Index;   // Is it changed by _BitScanForward in case of Zero input? If not we can store (sizeof(Num)*8) in it
 unsigned char res;
 if constexpr (sizeof(T) > sizeof(uint32))
  {
   if constexpr (__has_builtin(_BitScanForward64))res = _BitScanForward64(&Index, (unsigned long long)Num);    // ARM and AMD64
    else
     {
      res = _BitScanForward(&Index, (unsigned long)Num);
      if(!res)res = _BitScanForward(&Index, (unsigned long)(Num >> 32));
      Index += 32;
     }
  }
   else res = _BitScanForward(&Index, (unsigned long)Num);
 if(res)return (unsigned int)UnbindVal(Index);    // Found 1 at some position
  else return (unsigned int)(sizeof(T)*8);  // Num is zero, all bits is zero
#else
 // NOTE: We must detect if building below SSE4.2 on x86 because the compiler may use BSF which requires special handling for 0 values. BSF or TZCNT 
#if defined(CPU_X86) && !defined(__BMI__) && !defined(__LZCNT__) // && !defined(__AVX2__)   // AVX2 is close to SSE4.2 and usually will support POPCNT and LZCNT 
if constexpr (MayBeZero) { if(!Num)return BitSize<T>::V; }   // Every bit is zero
#endif
 if constexpr (sizeof(T) > sizeof(uint32))
  {
   if constexpr (sizeof(T) > sizeof(uint64))
    {
     if constexpr ((sizeof(T) > sizeof(u128)) || !IsScalar128<T>)  // Process as vector type in array
      {
       using N = decltype(Num[0]);
       int res = 0;
       for(int idx = (sizeof(T)/sizeof(N))-1; idx >= 0 ;idx--)    
        {
         N v = Num[idx];
         if(v)return ctz<MayBeZero>((uint64)Num) + res; // Have some ONEs    // TODO: Must pattern match bt optimizer for vectorization
          else res += (sizeof(N) * 8);
        }
       return res;
      }
       else   // Scalar i128
        {
         uint64 lo = (uint64)Num;    
         return lo?(ctz<MayBeZero>(lo)):(ctz<MayBeZero>(uint64(Num >> 64)) + 64);
        }
    }   
     else return (unsigned int)UnbindVal(__builtin_ctzll((uint64)Num));  // X64 CPUs only?
  }
   else return (unsigned int)UnbindVal(__builtin_ctz((uint32)Num));
#endif
/* else   // TODO: optimize?
  {
    int i = 0;
	for(;!(Num & 1u); Num >>= 1, i++);
	return i;
  }
 return 0; */
}
//------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/3849337/msvc-equivalent-to-builtin-popcount
// https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
// https://stackoverflow.com/questions/15736602/fastest-way-to-count-number-of-1s-in-a-register-arm-assembly
// http://dalkescientific.com/writings/diary/popcnt.c
// Counts the number of 1 bits (population count) in a 16-, 32-, or 64-bit unsigned integer.     // GCC: -mpopcnt (also implied by -msse4.2)   // AArch64: -march=armv8-a+cssc
// MSVC-X86: __popcnt16() / __popcnt() / __popcnt64()                     // BSF?
//
template<typename T> constexpr static inline int PopCnt(T Num)
{
 if constexpr (__has_builtin(__builtin_popcount) && (sizeof(T) <= sizeof(uint32)))
  {
   return __builtin_popcount((unsigned long)Num);
  }
 else if constexpr (__has_builtin(__builtin_popcountll))
  {
   return __builtin_popcountll((unsigned long long)Num);
  }
 else // UNTESTED!!!  // up to 128 bits // Should be as fast as MSVC's/CLang's intrinsic.  // MSVC does not have '__builtin_popcount' alternative for ARM
 {
  using V = MakeUnsigned<T>::T;
  V Val = V(Num);
  Val = Val - ((Val >> 1) & (V)~(V)0/3);        // TODO: Must cast the T(make_unsigned) to unsigned of same size an copy Num to it
  Val = (Val & (V)~(V)0/15*3) + ((Val >> 2) & (V)~(V)0/15*3);
  Val = (Val + (Val >> 4)) & (V)~(V)0/255*15;
  return (V)(Val * ((V)~(V)0/255)) >> (sizeof(V) - 1) * 8;  // 8 is number of bits in char
 }
}
//---------------------------------------------------------------------------
// TODO: Use refs (L,R val)
// NOTE: Can`t sift, should work on any types AND arrays(?)
//
template<typename T> constexpr _finline static auto SwapBytes(T value) 
{
 using UT = MakeUnsignedT<T>;
 if constexpr (sizeof(UT) == 1) return (UT)value;
#ifdef REAL_MSVC     // Expected to be constexpr evaluated
 if constexpr (sizeof(UT) == 2)return (UT)_byteswap_ushort((uint16)value);
 else if constexpr (sizeof(UT) == 4)return (UT)_byteswap_ulong((uint32)value);
 else if constexpr (sizeof(UT) == 8)return (UT)_byteswap_uint64((uint64)value);
#else
 if constexpr (sizeof(UT) == 2)return (UT)__builtin_bswap16((uint16)value);
 else if constexpr (sizeof(UT) == 4)return (UT)__builtin_bswap32((uint32)value);
 else if constexpr (sizeof(UT) == 8)return (UT)__builtin_bswap64((uint64)value);
//#  ifdef HAS_INT128
// else if constexpr (sizeof(UT) == 16)return (UT)__builtin_bswap128((uint128)value);    // Not present yet
//#  endif
#endif

 auto bytes = __builtin_bit_cast(SWrapArray<sizeof(UT),uint8>, value);      // TODO: Make sure that it is recognized and optimized or use the single array version instead
 SWrapArray<sizeof(UT),uint8> swapped;
 for(int id=0, is=sizeof(UT)-1; id < sizeof(UT); ++id,--is)swapped[id] = bytes[is];    
 return __builtin_bit_cast(UT, swapped);
/*
for(uint i = 0; i < sizeof(UT) / 2; ++i) {
    uint8 temp = bytes[i];
    bytes[i] = bytes[sizeof(UT) - 1 - i];
    bytes[sizeof(UT) - 1 - i] = temp;
}
*/
}
//------------------------------------------------------------------------------------------------------------
// NOTE: This is fallback function with no intrinsics (expected to be recognized and optimized into BSWAP instruction)
// 0: AABBCCDDEEFFGGHH <7,>7 (S-1)  AABBCCDD <3,>3 (S-1)  AABB <1,>1 (S-1)  AA
// 1: HHBBCCDDEEFFGGAA <5,>5 (S-3)  DDBBCCAA <1,>1 (S-3)  BBAA
// 2: HHGGCCDDEEFFBBAA <3,>3 (S-5)  DDCCBBAA
// 3: HHGGFFDDEECCBBAA <1,>1 (S-7)
// 4: HHGGFFEEDDCCBBAA
template<typename T> constexpr _finline static T RevByteOrder(T Value)   // Can be used at compile time  // What if T is signed?
{
 if constexpr (sizeof(T) > 1)
  {
   T Result = ((Value & 0xFF) << ((sizeof(T)-1)*8)) | ((Value >> ((sizeof(T)-1)*8)) & 0xFF);  // Exchange edge 1
   if constexpr (sizeof(T) > 2)
    {
     Result |= ((Value & 0xFF00) << ((sizeof(T)-3)*8)) | ((Value >> ((sizeof(T)-3)*8)) & 0xFF00); // Exchange edge 2
     if constexpr (sizeof(T) > 4)
      {
       Result |= ((Value & 0xFF0000) << ((sizeof(T)-5)*8)) | ((Value >> ((sizeof(T)-5)*8)) & 0xFF0000); // Exchange edge 3
       Result |= ((Value & 0xFF000000) << ((sizeof(T)-7)*8)) | ((Value >> ((sizeof(T)-7)*8)) & 0xFF000000); // Exchange edge 4
      }
    }
   return Result;
  }
 return Value;
}
//------------------------------------------------------------------------------------
// NOTE: Slow, use only as fallback
template <class T> constexpr T RevBits(T n)
{
 short bits = BitSize<T>::V;
 T mask = ~T(0); // equivalent to uint32_t mask = 0b11111111111111111111111111111111;
 while(bits >>= 1)
  {
   mask ^= mask << (bits); // will convert mask to 0b00000000000000001111111111111111;
   n = (n & ~mask) >> bits | (n & mask) << bits; // divide and conquer
  }
 return n;
}
/*
template<typename T> T RevBits( T n )
{
    // we force the passed-in type to its unsigned equivalent, because C++ may
    // perform arithmetic right shift instead of logical right shift, depending
    // on the compiler implementation.
    typedef typename std::make_unsigned<T>::type unsigned_T;
    unsigned_T v = (unsigned_T)n;

    // swap every bit with its neighbor
    v = ((v & 0xAAAAAAAAAAAAAAAA) >> 1)  | ((v & 0x5555555555555555) << 1);

    // swap every pair of bits
    v = ((v & 0xCCCCCCCCCCCCCCCC) >> 2)  | ((v & 0x3333333333333333) << 2);

    // swap every nybble
    v = ((v & 0xF0F0F0F0F0F0F0F0) >> 4)  | ((v & 0x0F0F0F0F0F0F0F0F) << 4);
    // bail out if we've covered the word size already
    if( sizeof(T) == 1 ) return v;

    // swap every byte
    v = ((v & 0xFF00FF00FF00FF00) >> 8)  | ((v & 0x00FF00FF00FF00FF) << 8);
    if( sizeof(T) == 2 ) return v;

    // etc...
    v = ((v & 0xFFFF0000FFFF0000) >> 16) | ((v & 0x0000FFFF0000FFFF) << 16);
    if( sizeof(T) <= 4 ) return v;

    v = ((v & 0xFFFFFFFF00000000) >> 32) | ((v & 0x00000000FFFFFFFF) << 32);

    // explictly cast back to the original type just to be pedantic
    return (T)v;
}
*/

//---------------------------------------------------------------------------
// __builtin_arm_rbit(unsigned int val) (for 32-bit)
// __builtin_arm_rbitll(unsigned long long val) (for 64-bit)
//
template<typename T> constexpr _finline static auto SwapBits(T value) 
{
 using UT = MakeUnsignedT<T>;

#ifdef CPU_ARM         // __has_builtin(__builtin_arm_rbit)  __has_builtin(__builtin_arm_rbitll)
 if constexpr (sizeof(UT) == 4)return (UT)__builtin_arm_rbit((uint32)value);
#  ifdef ARCH_X64 
 if constexpr (sizeof(UT) == 8)return (UT)__builtin_arm_rbitll((uint64)value);
#  endif 
#endif

 SCVR uint8 MaskA8 = 0x55;
 SCVR uint8 MaskB8 = 0x33;
 SCVR uint8 MaskC8 = 0x0F;

 auto DoMask = [] (auto v, auto mask1, auto mask2, auto mask3) -> auto {
     v = ((v >> 1) & mask1) | ((v & mask1) << 1);
     v = ((v >> 2) & mask2) | ((v & mask2) << 2);
     v = ((v >> 4) & mask3) | ((v & mask3) << 4);
     return v;
    };

 if constexpr (sizeof(UT) == 1)return DoMask((UT)value, MaskA8, MaskB8, MaskC8);

 auto bswapped = SwapBytes(value);
 SCVR uint16 MaskA16 = MaskA8 | (uint16(MaskA8) << 8);
 SCVR uint16 MaskB16 = MaskB8 | (uint16(MaskB8) << 8);
 SCVR uint16 MaskC16 = MaskC8 | (uint16(MaskC8) << 8);
 
 if constexpr (sizeof(UT) == 2)return DoMask(bswapped, MaskA16, MaskB16, MaskC16); 

 SCVR uint32 MaskA32 = MaskA16 | (uint32(MaskA16) << 16);
 SCVR uint32 MaskB32 = MaskB16 | (uint32(MaskB16) << 16);
 SCVR uint32 MaskC32 = MaskC16 | (uint32(MaskC16) << 16);

 if constexpr (sizeof(UT) == 4)return DoMask(bswapped, MaskA32, MaskB32, MaskC32); 

 SCVR uint64 MaskA64 = MaskA32 | (uint64(MaskA32) << 32);
 SCVR uint64 MaskB64 = MaskB32 | (uint64(MaskB32) << 32);
 SCVR uint64 MaskC64 = MaskC32 | (uint64(MaskC32) << 32);

 if constexpr (sizeof(UT) == 8)return DoMask(bswapped, MaskA64, MaskB64, MaskC64);
#ifdef HAS_INT128
 if constexpr (sizeof(UT) == 16)
  {
   SCVR uint128 MaskA128 = MaskA64 | (uint128(MaskA64) << 64);
   SCVR uint128 MaskB128 = MaskB64 | (uint128(MaskB64) << 64);
   SCVR uint128 MaskC128 = MaskC64 | (uint128(MaskC64) << 64); 
   return DoMask(bswapped, MaskA128, MaskB128, MaskC128);
  }
#endif

 auto bytes = __builtin_bit_cast(SWrapArray<sizeof(UT),uint8>, bswapped);                        // TODO: Test it
 for(int idx=0; idx < sizeof(UT); ++idx)bytes[idx] = DoMask(bytes[idx], MaskA8, MaskB8, MaskC8);    
 return __builtin_bit_cast(UT, bytes);
}
//---------------------------------------------------------------------------
// 1) For 32-bit x86 and x86-64, shift amounts are interpreted mod 32 for operand widths of 32 bits and lower, and mod 64 for 64-bit operands. So right-shifting a 64-bit value by 64 will yield the same as shifting by 0, i.e. a no-op.
// 2) In 32-bit ARM (A32/T32 instruction sets), shift amounts are taken mod 256. Right-shifting a 32-bit value by 32 (or 64) will hence yield 0, as will right-shifting it by 255, but right-shifting it by 256 will leave the value untouched.
// 3) In 64-bit ARM (A64 ISA), shift amounts are taken mod 32 for 32-bit shifts and mod 64 for 64-bit shifts (essentially the same as x86-64).
// 4) RISC-V also follows the same rule: 32-bit shifts distances are mod 32, 64-bit shift distances are mod 64.
// 5) For POWER/PowerPC, 32-bit shifts take the shift amount mod 64 and 64-bit shifts take the shift amount mod 128.
// 
// To make matters even more confusing, in most of these instruction sets with SIMD extensions, the SIMD integer instructions have different out-of-range shift behaviorthan the non-SIMD instructions. 
//
template <typename T> constexpr _finline static auto RotL(T Value, unsigned int Shift)
{
 using UT = MakeUnsignedT<T>;
 SCVR int MaxBits = BitSize<T>::V;
 Shift &= (MaxBits - 1);   // Normalize shift to [0, MaxBits)
 //if(!Shift)return Value;  // Costly
#ifdef REAL_MSVC     // Expected to be constexpr evaluated    
 if constexpr (sizeof(UT) == 1)return (UT)_rotl8(uint8(Value), uint8(Shift)); 
 else if constexpr (sizeof(UT) == 2)return (UT)_rotl16(uint16(Value), uint8(Shift));
 else if constexpr (sizeof(UT) == 4)return (UT)_rotl(uint32(Value), Shift);
 else if constexpr (sizeof(UT) == 8)return (UT)_rotl64(uint64(Value), Shift);        
#else
 if constexpr (sizeof(UT) == 1)return (UT)__builtin_rotateleft8(uint8(Value), Shift);
 else if constexpr (sizeof(UT) == 2)return (UT)__builtin_rotateleft16(uint16(Value), Shift);
 else if constexpr (sizeof(UT) == 4)return (UT)__builtin_rotateleft32(uint32(Value), Shift);
 else if constexpr (sizeof(UT) == 8)return (UT)__builtin_rotateleft64(uint64(Value), Shift);
#endif   
 return UT((UT(Value) << Shift) | (UT(Value) >> (MaxBits - Shift)));   // Fallback   // Was recognized and replaced with inlined intrinsic anyway
}
//---------------------------------------------------------------------------
template <typename T> constexpr _finline static auto RotR(T Value, unsigned int Shift)
{
 using UT = MakeUnsignedT<T>;
 SCVR int MaxBits = BitSize<T>::V; 
 Shift &= (MaxBits - 1); // Normalize shift to [0, MaxBits)
 //if(!Shift)return Value;  // Costly
#ifdef REAL_MSVC     // Expected to be constexpr evaluated
 if constexpr (sizeof(UT) == 1)return (UT)_rotr8(uint8(Value), uint8(Shift));
 else if constexpr (sizeof(UT) == 2)return (UT)_rotr16(uint16(Value), uint8(Shift));
 else if constexpr (sizeof(UT) == 4)return (UT)_rotr(uint32(Value), Shift);
 else if constexpr (sizeof(UT) == 8)return (UT)_rotr64(uint64(Value), Shift);
#else
 if constexpr (sizeof(UT) == 1)return (UT)__builtin_rotateright8(uint8(Value), Shift);
 else if constexpr (sizeof(UT) == 2)return (UT)__builtin_rotateright16(uint16(Value), Shift);
 else if constexpr (sizeof(UT) == 4)return (UT)__builtin_rotateright32(uint32(Value), Shift);
 else if constexpr (sizeof(UT) == 8)return (UT)__builtin_rotateright64(uint64(Value), Shift);
#endif
 return UT((UT(Value) >> Shift) | (UT(Value) << (MaxBits - Shift)));   // Fallback   // Was recognized and replaced with inlined intrinsic anyway
}
//---------------------------------------------------------------------------
// https://en.cppreference.com/w/cpp/numeric/bit_width
//
template<typename T> constexpr _finline static int BitWidth(T v)  // Numberic only   // Signed? Lose the sign?
{
 return BitSize<T>::V - clz(v);
}
//------------------------------------------------------------------------------------
template<typename T> constexpr _finline static int MSB(T v)  // Numeric only   // Signed? Lose the sign?
{
 return (v)?((BitSize<T>::V-1) - clz<false>(v)):(0);
}
//------------------------------------------------------------------------------------
//  int __builtin_ffs (unsigned int x) Returns one plus the index of the least significant 1-bit of x, or if x is zero, returns zero
// __builtin_ffs is a function of the GCC built-in, gets a value: from the low position, the first 1 appears, such as 0x11, returned to 1,0x00 returned 0, 0x02, return 2.
// It is also known as FFS (find first set) or CTZ (count trailing zero). __builtin_ctz(x) returns the number of trailing 0-bits in x, starting at the least significant bit position.
// Returns one plus the index of the least significant 1-bit of x, or if x is zero, returns zero.
// ffs(x) = w - clz(x & -x)
// ffs(x) = popcount(x ^ ~-x)
// 
// __builtin_ffs(0)  returns: 0
// __builtin_ffs(1)  returns: 1
// __builtin_ffs(20) returns: 3
// 
// __builtin_ffsl(unsigned long x)
// __builtin_ffsll
//
template<typename T> constexpr _finline static int FFS(T v)   // Find First Significant  
{
//#if __has_builtin(__builtin_ffs)    // DISABLED: __builtin_ffs is suboptimal wrapper of 'ctz'
//  __builtin_ffs  // What type?
//#endif
 return (v)?(1 + ctz<false>(v)):(0);
}
//---------------------------------------------------------------------------
// https://en.cppreference.com/w/cpp/numeric/bit_ceil
// Calculates the smallest integral power of two that is not smaller than x
//
template<typename T> constexpr _finline static T BitCeil(T v)   // Signed? Lose the sign?   // AlignP2Up
{
 return (v > 1u)?(T(1) << BitWidth(T(v - 1))):(T(1));
}
//------------------------------------------------------------------------------------
// https://en.cppreference.com/w/cpp/numeric/bit_floor
// If x is not zero, calculates the largest integral power of two that is not greater than x. If x is zero, returns zero.
//
template<typename T> constexpr _finline static T BitFloor(T v)  // Signed? Lose the sign?  // AlignP2Dn
{
 return (v)?(T(1) << (BitWidth(v) - 1)):(v);
}
//------------------------------------------------------------------------------------
template<typename T, bool NeedBE> static _finline constexpr T ChangeByteOrder(T val)
{
 if constexpr (sizeof(val) == sizeof(uint8))return val;
 if constexpr (NeedBE)
  {
   if constexpr (IsBigEndian)return val;   
     else return SwapBytes(val);
  }
 else
  {
   if constexpr (IsBigEndian)return SwapBytes(val); 
     else return val;   
  }
}
//------------------------------------------------------------------------------------
template<typename T> _finline constexpr T MakeFlag(uint8 FlagIdx) {return (T(1) << FlagIdx);}
_finline constexpr auto AddFlag(auto Value, uint8 FlagIdx) {return (Value | ((decltype(Value))(1) << FlagIdx));}
_finline constexpr auto AddFlags(auto Value, auto FlagMsk) {return (Value | FlagMsk);}
_finline constexpr void SetFlag(auto& Value, uint8 FlagIdx) {Value |= ((RemoveRefT<decltype(Value)>)(1) << FlagIdx);}
_finline constexpr void SetFlags(auto& Value, auto FlagMsk) {Value |= FlagMsk;}
_finline constexpr void RemoveFlags(auto& Value, auto FlagMsk) {Value &= ~FlagMsk;}
_finline constexpr bool IsSetFlag(auto Value, uint8 FlagIdx) {return bool(Value & ((decltype(Value))(1) << FlagIdx));}
_finline constexpr bool IsSetFlags(auto Value, auto FlagMsk) {return ((Value & FlagMsk) == FlagMsk);}    // TRUE If all flags is set
_finline constexpr bool IsSetAnyFlag(auto Value, auto FlagMsk) {return bool(Value & FlagMsk);}
//====================================================================================
// This one is used in several bit-manipulating classes, like bit array or ArbiNum
enum EBitOp: uint8
{
 boNone,
// Base logic gates
 boOr,
 boAnd,
 boNot,
// Combined logic gates
 boXOr,
 boNOr,
 boXNOr,
 boNAnd
};
//------------------------------------------------------------------------------------
// https://www.techtarget.com/whatis/definition/logic-gate-AND-OR-XOR-NOT-NAND-NOR-and-XNOR
template<typename T=bool> struct SBitLogic
{
static _finline T OpNOT(T V1){return !V1;}
static _finline T OpOR(T V1, T V2){return V1|V2;}
static _finline T OpAND(T V1, T V2){return V1&V2;}
static _finline T OpXOR(T V1, T V2){return V1^V2;}

static _finline T OpNOR(T V1, T V2){return OpNOT(OpOR(V1,V2));}
static _finline T OpXNOR(T V1, T V2){return OpNOT(OpXOR(V1,V2));}
static _finline T OpNAND(T V1, T V2){return OpNOT(OpAND(V1,V2));}

static _finline T ModBit(EBitOp Op, T ValA, T ValB=0)
{
 switch(Op)
  {
   case boNot:  {return OpNOT( ValA); break;}      // Input ValB is ignored
   case boOr:   {return OpOR(  ValA, ValB); break;}
   case boAnd:  {return OpAND( ValA, ValB); break;}
   case boXOr:  {return OpXOR( ValA, ValB); break;}
   case boNOr:  {return OpNOR( ValA, ValB); break;}
   case boXNOr: {return OpXNOR(ValA, ValB); break;}
   case boNAnd: {return OpNAND(ValA, ValB); break;}
  }
 return 0;
}
//----------------------------
};
//====================================================================================
// https://en.cppreference.com/w/cpp/numeric/countl_one
// https://en.cppreference.com/w/cpp/numeric/countr_one
// https://en.cppreference.com/w/cpp/numeric/has_single_bit      // return (popcount(v) == 1)
//===========================================================================
