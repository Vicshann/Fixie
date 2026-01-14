
#pragma once


struct NMATH
{

//======================================================================================================================
// https://forum.arduino.cc/t/divmod10-a-fast-replacement-for-10-and-10-unsigned
// NOTE: Generic UDiv10, much slower than Mult with a magic constant
// Checking X at runtime will make it slower?  ( X may take less bytes and rest of 'q = (q>>8) + x' will be redundant )
// Best solution on ARM?
//
template<typename T> constexpr _finline static T Div10U(T in) requires (IsUnsigned<T>::V)
{
 T x = (in|1) - (in>>2); // div = in/10 <~~> div = 0.75*in/8
 T q = (x>>4) + x;
 if constexpr ((T)-1 > 0xFF)      // NOTE: sizeof(T) may incorrectly detect size of a class as larger if it contains anything besides the type itself
  {
   x = q;
   q = (q>>8) + x;
   q = (q>>8) + x;
   if constexpr ((T)-1 > 0xFFFF)
    {
     q = (q>>8) + x;
     q = (q>>8) + x;
     if constexpr ((T)-1 > 0xFFFFFFFF)
      {
       q = (q>>8) + x;
       q = (q>>8) + x;
       q = (q>>8) + x;
       q = (q>>8) + x;
      }
    }
  }
 return q >> 3;     // We need to lose 3 low bits
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T DivMod10U(T in, auto&& mod) requires (IsUnsigned<T>::V)
{
 T d = Div10U(in);
 mod = in - ((d << 3) + (d << 1));
 return d;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T ModDiv10U(T in, auto&& div) requires (IsUnsigned<T>::V)
{
 T d = div = Div10U(in);
 return in - ((d << 3) + (d << 1));
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T Mod10U(T in) requires (IsUnsigned<T>::V)
{
 return ModDiv10U(in, T());
}
//----------------------------------------------------------------------------------------------------------------------
//======================================================================================================================
// https://github.com/OP-TEE/optee_os/blob/master/lib/libutils/isoc/arch/arm/
// Software DIV
template<typename T> constexpr _ninline static T DivModU(T num, T den, auto&& mod) requires (IsUnsigned<T>::V)  // Returns quotient. remainder is in 'mod'
{
 T i = 1, q = 0;
 if(!den){mod=0; return 0;}  // What to return?             //{this->r = (T)-1;	return;} // division by 0

 while(!(den >> ((sizeof(T)*8)-1)))  // 31/63 // Until highest bit set // Very slow on X32! // TODO: Optimize with hardware (clz)
  {
   i = i << 1;	  // count the max division steps
   den = den << 1;   // increase p until it has maximum size
  }

 while(i > 0)
  {
   q = q << 1;	 // write bit in q at index (size-1)
   if(num >= den)
    {
     num -= den;
     q++;
    }
   den = den >> 1; 	// decrease p
   i = i >> 1; 	// decrease remaining size in q
  }
 mod = num;
 return q;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T ModDivU(T num, T den, auto&& div) requires (IsUnsigned<T>::V)   // Returns remainder. quotient is in 'div'
{
 T rem;
 div = DivModU(num, den, rem);
 return rem;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _ninline static T DivModS(T num, T den, auto&& mod) requires (!IsUnsigned<T>::V)
{
 using M = decltype(TypeToUnsigned<T>());
 bool qs = 0, rs = 0;
 if(((num < 0) && (den > 0)) || ((num > 0) && (den < 0)))qs = true;	// quotient shall be negate
 if(num < 0){num = -num; rs = true;} 	// remainder shall be negate
 if(den < 0)den = -den;
 M r;  // Unsigned T
 T q = DivModU<M>(num, den, r);
 if(qs)q = -q;
 if(rs)r = -r;
 mod = r;
 return q;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T ModDivS(T num, T den, auto&& div) requires (!IsUnsigned<T>::V)   // Returns remainder. quotient is in 'div'
{
 T rem;
 div = DivModS(num, den, rem);
 return rem;
}
//----------------------------------------------------------------------------------------------------------------------
// extract hi and lo 32-bit words from 64-bit value
static _finline uint32 Arith64lo(uint64& v) {return ((uint32*)&v)[(bool)IsBigEndian];}
static _finline uint32 Arith64hi(uint64& v) {return ((uint32*)&v)[(bool)!IsBigEndian];}    // BIG_ENDIAN: hi,lo

// Negate a if b is negative, via invert and increment.
static _finline sint64 ArithNeg64(uint64 a, sint64 b)   // ((a ^ ((sint64(b) >= 0) - 1)) + (sint64(b) < 0))  // TODO: Test it
{
 uint32 b_lz = (b < 0);     // uint32 and hope it will be put into a register
 uint32 b_ge = !b_lz;
 return ((a ^ ((sint64)b_ge - 1)) + (sint64)b_lz);
}
static _finline uint64 ArithAbs64(sint64 a) {return ArithNeg64(a, a);}
// Calculate both the quotient and remainder of the unsigned division of a by b.
// The return value is the quotient, and the remainder is placed in variable pointed to by c (if it's not NULL).
//
static inline uint64 DivModQU(uint64 num, uint64 den, auto&& mod)   // __divmoddi4
{
 if(den > num)                                  // divisor > numerator?
  {
   mod = num;                                   // remainder = numerator
   return 0;                                    // quotient = 0
  }
 if(!Arith64hi(den))                            // divisor is 32-bit
  {
   if(den == 0)return 0;                        // Never fail! (optional?)   // divide by 0   {volatile char x = 0; x = 1 / x;}     // force an exception
   if(den == 1)                                 // divide by 1
    {
     mod = 0;                                   // remainder = 0
     return num;                                // quotient = numerator
    }
  if(!Arith64hi(num))                           // numerator is also 32-bit
   {
    mod = Arith64lo(num) % Arith64lo(den);      // use generic 32-bit operators
    return Arith64lo(num) / Arith64lo(den);
   }
 }

 // let's do long division
 sint8 bits = clz(uint64(den)) - clz(uint64(num)) + 1;  // number of bits to iterate (a and b are non-zero)
 uint64 rem = num >> bits;                              // init remainder
 num <<= 64 - bits;                                     // shift numerator to the high bit
 uint64 wrap = 0;                                       // start with wrap = 0
 while(bits-- > 0)                                      // for each bit
  {
   rem  = (rem << 1) | (num >> 63);                     // shift numerator MSB to remainder LSB
   num  = (num << 1) | (wrap & 1);                      // shift out the numerator, shift in wrap
   wrap = ((sint64)(den - rem - 1) >> 63);              // wrap = (b > rem) ? 0 : 0xffffffffffffffff (via sign extension)
   rem -= den & wrap;                                   // if (wrap) rem -= b
  }
 mod = rem;                                             // maybe set remainder
 return (num << 1) | (wrap & 1);                        // return the quotient
}
//----------------------------------------------------------------------------------------------------------------------
static _finline int64 DivModQS(sint64 num, sint64 den, sint64& mod)
{
 uint64 r;
 uint64 q = DivModQU(ArithAbs64(num), ArithAbs64(den), r);
 mod = ArithNeg64(r, num);
 return ArithNeg64(q, num^den); // negate q if a and b signs are different
}
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/664852/which-is-the-fastest-way-to-get-the-absolute-value-of-a-number
/*
 __builtin_abs()
__builtin_constant_p()
__builtin_expect()
__builtin_fabs()
__builtin_fabsf()
__builtin_frame_address()
__builtin_labs()
__builtin_llabs()
__builtin_sqrt()
__builtin_sqrtf()
*/
// Check for *MAX undefined behaviour?
template<typename T> constexpr _finline static T Abs(T num)
{
 if(num < 0)num = -num;
 return num;
}
//----------------------------------------------------------------------------------------------------------------------
// Calculate greatest common divisor (std::gcd)
template<typename T> constexpr static T gcd(T u, T v)
{
 using M = decltype(TypeToUnsigned<T>());
 if(u == 0)return v;
 if(v == 0)return u;
 int shift = ctz(u | v);
 u >>= ctz(u);
 do {
   v >>= ctz(v);
   M m = (v ^ u) & -(v < u);  // unsigned of same size as T
   u ^= m;
   v ^= m;
   v -= u;
 } while (v != 0);
 return u << shift;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T sqrt(T v)
{
 if constexpr (sizeof(T) <= sizeof(float))return (T)__builtin_sqrtf(float(v));
   else if constexpr (sizeof(T) > sizeof(double))return (T)__builtin_sqrtl(double(v));
 return (T)__builtin_sqrt((double)v);    // NOTE: Clang`s optimizer may have troubles recognizing end of a function without explicit return if other branches are in 'if constexpr'.
}
//----------------------------------------------------------------------------------------------------------------------
};


/*
 sint32 r = 99999;
 sint32 q = NFWK::NMATH::DivModS(4, 54, r);
   sint32 q1 = 4 / 54;
   sint32 r1 = 4 % 54;
 return q+q1+r1;

The framework should work with -ffast-math

https://kristerw.github.io/2021/10/19/fast-math/?trk=article-ssr-frontend-pulse_little-text-block
 //  (MSVC: /fp:fast (overrides /clang: -ffast-math))  /clang: -fno-math-errno (part of -ffast-math)    // PLT_WASM: https://github.com/llvm/llvm-project/issues/92698


----  Treating subnormal numbers as 0.0
 #define MXCSR_DAZ (1<<6)    // Enable denormals are zero mode
#define MXCSR_FTZ (1<<15)   // Enable flush to zero mode

unsigned int mxcsr = __builtin_ia32_stmxcsr();
mxcsr |= MXCSR_DAZ | MXCSR_FTZ;
__builtin_ia32_ldmxcsr(mxcsr);
----

float SquareRootAI(float num)
    {
        float guess = num / 2;
        float prevGuess = guess + 1;
        while(guess != prevGuess)
        {
            prevGuess = guess;
            guess = (num / guess + guess) / 2;
        }
        return guess;
    }
----------------------
https://stackoverflow.com/questions/4930307/fastest-way-to-get-the-integer-part-of-sqrtn


// bit_width can be evaluated in constant time and the loop will iterate at most ceil(bit_width / 2) times. So even for a 64-bit integer, this will be at worst 32 iterations of basic arithmetic and bitwise operations.
//
// C++20 also provides std::bit_width in its <bit> header
unsigned char bit_width(unsigned long long x) {
    return x == 0 ? 1 : 64 - __builtin_clzll(x);
}

template <typename Int, std::enable_if_t<std::is_unsigned<Int, int = 0>> Int sqrt(const Int n)
{
    unsigned char shift = bit_width(n);
    shift += shift & 1; // round up to next multiple of 2

    Int result = 0;

    do {
        shift -= 2;
        result <<= 1; // make space for the next guessed bit
        result |= 1;  // guess that the next bit is 1
        result ^= result * result > (n >> shift); // revert if guess too high
    } while (shift != 0);

    return result;
}

Performance
I have benchmarked my methods against float-bases ones by generating inputs uniformly. Note that in the real world most inputs would be much closer to zero than to std::numeric_limits<...>::max().

for uint32_t this performs about 25x worse than using std::sqrt(float)
for uint64_t this performs about 30x worse than using std::sqrt(double)
Accuracy
This method is always perfectly accurate, unlike approaches using floating point math.

Using sqrtf can provide incorrect rounding in the [228, 232) range. For example, sqrtf(0xffffffff) = 65536, when the square root is actually 65535.99999.
Double precision doesn't work consistently for the [260, 264) range. For example, sqrt(0x3fff...) = 2147483648, when the square root is actually 2147483647.999999.
The only thing that covers all 64-bit integers is x86 extended precision long double, simply because it can fit an entire 64-bit integer.

Alternative Approach Using Newton's Method:

template <typename Int, std::enable_if_t<std::is_unsigned_v<Int>, int> = 0> Int sqrt_guess(const Int n)
{
    Int log2floor = bit_width(n) - 1;
    // sqrt(x) is equivalent to pow(2, x / 2 = x >> 1)
    // pow(2, x) is equivalent to 1 << x
    return 1 << (log2floor >> 1);
}

template <typename Int, std::enable_if_t<std::is_unsigned_v<Int>, int> = 0>
Int sqrt_newton(const Int n)
{
    Int a = sqrt_guess(n);
    Int b = n;

    // compute unsigned difference
    while (std::max(a, b) - std::min(a, b) > 1) {
        b = n / a;
        a = (a + b) / 2;
    }

    // a is now either floor(sqrt(n)) or ceil(sqrt(n))
    // we decrement in the latter case
    // this is overflow-safe as long as we start with a lower bound guess
    return a - (a * a > n);
}
------------------------
// Gives errors after a while:
int sqrti(int x)
{
    union { float f; int x; } v;

    // convert to float
    v.f = (float)x;

    // fast aprox sqrt
    //  assumes float is in IEEE 754 single precision format
    //  assumes int is 32 bits
    //  b = exponent bias
    //  m = number of mantissa bits
    v.x  -= 1 << 23; // subtract 2^m
    v.x >>= 1;       // divide by 2
    v.x  += 1 << 29; // add ((b + 1) / 2) * 2^m

    // convert to int
    return (int)v.f;
}
------------------------
// This one works but how many iterations it may take?
uint64_t Sqrt64(uint64_t xx)
{
  if (xx <= 1) return xx;
  uint64_t z = xx >> 2;
  uint64_t r2 = 2 * Sqrt64(z);
  uint64_t r3 = r2 + 1;
  return (xx < r3 * r3) ? r2 : r3;
}
--------------------
// http://www.codecodex.com/wiki/Calculate_an_integer_square_root
class IntSqrt
{
public:
  IntSqrt(int n): _number(n) {}

  int operator()() const
  {
    int remainder = _number;
    if (remainder < 0) { return 0; }

    int place = 1 <<(sizeof(int)*8 -2);

    while (place > remainder) { place /= 4; }

    int root = 0;
    while (place)
    {
      if (remainder >= root + place)
      {
        remainder -= root + place;
        root += place*2;
      }
      root /= 2;
      place /= 4;
    }
    return root;
  }

private:
  int _number;
};

// http://en.wikipedia.org/wiki/Fast_inverse_square_root
class FastSqrt
{
public:
  FastSqrt(int n): _number(n) {}

  int operator()() const
  {
    float number = _number;

    float x2 = number * 0.5F;
    float y = number;
    long i = *(long*)&y;
    //i = (long)0x5fe6ec85e7de30da - (i >> 1);
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;

    y = y * (1.5F - (x2*y*y));
    y = y * (1.5F - (x2*y*y)); // let's be precise

    return static_cast<int>(1/y + 0.5f);
  }

private:
  int _number;
};
Results: Hardware SQRT is faster anyway ;)
--------------------------------------
inline int8_t log2(size_t value)
{
    static constexpr int8_t table[64] =
    {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5
    };
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return table[((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

inline size_t next_power_of_two(size_t i)
{
    --i;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i |= i >> 32;
    ++i;
    return i;
}
*/
