
#pragma once

#ifndef _FRAMEWORK_    // These macro are used to check which header file included
#define _FRAMEWORK_    // Now we just use __has_include 	- a preprocessor operator to check whether an inclusion is possible. // C++17  // __has_include requires include search directories to be passed to the compiler

// Try to keep the code as simple and readable as possible. Not so future generations will appreciate that ;)
/*
 Some useful links:
   https://graphics.stanford.edu/~seander/bithacks.html

*/
#if !defined(_MSC_VER) && !__has_include (<source_location>)
namespace std
{
 struct source_location
  {
   struct __impl      // Hardcoded
    {
      const char* _M_file_name;
      const char* _M_function_name;
      unsigned _M_line;
      unsigned _M_column;
    };
  };
}
#endif

#define _HIDENT(x) x
#define _HXSTR(x) #x
#define _HSTR(x) _HXSTR(x)
#define _JOIN_PATH(x,y) _HSTR(_HIDENT(x)_HIDENT(y))

#if __has_include ("./COMPILER/include/intrin.h")      
#define _INC_PATH ./COMPILER/include/
#elif __has_include ("../COMPILER/include/intrin.h")
#define _INC_PATH ../COMPILER/include/
#elif __has_include ("../../COMPILER/include/intrin.h")
#define _INC_PATH ../../COMPILER/include/
#elif __has_include ("../../../COMPILER/include/intrin.h")
#define _INC_PATH ../../../COMPILER/include/
#elif __has_include ("../../../../COMPILER/include/intrin.h")
#define _INC_PATH ../../../../COMPILER/include/
#else
//#pragma message(">>> No compiler`s include path is found!")
#endif

#ifdef _INC_PATH
#pragma message(">>> Compiler`s include path is " _HSTR(_INC_PATH))
#endif

// ******* ARE  THOSE ACTUALLY USABLE ? *******
//#include _JOIN_PATH(_INC_PATH,intrin.h)   // Works but intrin.h includes some files too which is not found
// Must be outside any namespace. This is the only external dependancy. It is header-only and very compiler specific
// It is probably better to reimplement necessary parts of those
//#include <stdarg.h>
//#include <intrin.h>         // Only for windows?  // Unfortunately <intrin.h> is a mess  // Clang`s intrin.h redirects to next intrin.h when building not with MSVC driver
//#include <emmintrin.h>      // for _mm_storeu_si128
//#include <immintrin.h>
//#include <xmmintrin.h>
//------------------------------------------------------------------------------------------------------------
// Some STD stuff
#if !__has_include (<initializer_list>)
#include "Platforms/InitList.hpp"
#endif

//extern "C" void*  __cdecl memmove(void* _Dst, const void* _Src, size_t _Size);
//extern "C" void*  __cdecl memset(void* _Dst, int _Val, size_t _Size)

// Format: 'N'+'Four letters of namespace'
// NOTE: It is impossible to put a 'namespace' inside of a 'class'
namespace NFWK      // Must be a namespace because we are adding some namespaces and configs in it
{
// NOTE: Keep it consistent with AppMain.cpp
// NOTE: On Linux, Clang will not respect links on the path and will resolve them!
#if __has_include ("AppCfg.hpp")
#include "AppCfg.hpp"
/*#elif __has_include ("src/AppCfg.hpp")
#include "src/AppCfg.hpp"
#elif __has_include ("../AppCfg.hpp")
#include "../AppCfg.hpp"
#elif __has_include ("../src/AppCfg.hpp")
#include "../src/AppCfg.hpp"
#elif __has_include ("../../AppCfg.hpp")
#include "../../AppCfg.hpp"
#elif __has_include ("../../src/AppCfg.hpp")
#include "../../src/AppCfg.hpp"
#elif __has_include ("../../../AppCfg.hpp")
#include "../../../AppCfg.hpp"
#elif __has_include ("../../../src/AppCfg.hpp")
#include "../../../src/AppCfg.hpp"
#elif __has_include ("../../../../AppCfg.hpp")
#include "../../../../AppCfg.hpp"
#elif __has_include ("../../../../src/AppCfg.hpp")
#include "../../../../src/AppCfg.hpp"   */   // The Compilation Script will respect links in the path and will add the correct path to Includes
#else
#include "Platforms/DefaultCfg.hpp"
#pragma message(">>> No AppCfg.hpp is found - using default config!")
#endif

#include "Platforms/Common.hpp"       // Contains type definitions, must be in namespace to allow their inclusion with 'using namespace'
#include "Platforms/Intrin.hpp"       // Must be included after definition of NGenericTypes
#include "Platforms/BitOps.hpp"
#include "Platforms/MemOps.hpp"
namespace NCRYPT
{
#include "Crypto/Crc32.hpp"
#include "Crypto/Random.hpp"
}
#include "Platforms/CompileTime.hpp"  // Have access only to Common.hpp, everything else is 'invisible'
#include "Platforms/ArbiNum.hpp"

#include "Math.hpp"
#include "UTF.hpp"
#include "NumCnv.hpp"
#include "DateTime.hpp"
#include "StrUtils.hpp"
#include "StrFmt.hpp"
#if defined(CPU_X86)
#include "RCE/HDE.hpp"
#endif
#include "Platforms/Platform.hpp"     // All API is defined here and inaccessible before!

using PX   = NPTM::PX;
using PX64 = NPTM::PX64;

// Now we have access to SAPI+NAPI
// TODO: CStr which can be initialized from a pointer and contains string utilities


#include "Containers/Allocators.hpp"
#include "Streams/Streams.hpp"

//#include "MemUtils.hpp"
//#include "MemStorage.hpp"
//#include "StrStorage.hpp"
//#include "StrPool.hpp"
#include "Arrays.hpp"
#include "Parsers/MiniIni.hpp"

namespace NCRYPT     // https://github.com/abbbaf/Compile-time-hash-functions
{
#include "Crypto/TEA.hpp"
#include "Crypto/RC4.hpp"
#include "Crypto/MD5.hpp"
#include "Crypto/SHA1.hpp"
#include "Crypto/Rijndael.hpp"
};

//--- Extra
#include "StrLocalizer.hpp"
#include "Containers/BitTrie.hpp"
#include "Parsers/Tokenizer.hpp"
//---
#ifdef FWK_CFG_USE_RCE
#include "RCE/HDE.hpp"
//#include "RCE/DeCore/DeCore.hpp"

//#include "RCE/UniHook/UniHook.hpp"     // TODO: X86!!!
#endif

#ifdef FWK_CFG_USE_MISC

#endif
//---

#include "AppDef.hpp"

};
//------------------------------------------------------------------------------------------------------------
using NFWK::NCTM::operator""_ps;
using NFWK::NCTM::operator""_es;

#include "Platforms/RTL.hpp"    // Must be in global namespace for the compiler to find it // Comment this out if you linking with a default Run Time Library

#endif

/*
 https://aras-p.info/blog/2019/01/12/Investigating-compile-times-and-Clang-ftime-report/
 MSVC: /clang:-ftime-trace=C:/ctrace.jsn
 chrome://tracing

 ftime-report

 -ast-dump <filename.c>

 -Xclang -ast-print -fsyntax-only
---------------
https://dorotac.eu/posts/input_broken/
*/
