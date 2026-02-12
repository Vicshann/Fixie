
#pragma once

// NOTE: This module does not declare its own name space
// NOTE: The Framework is not compatible with MSVC compiler because it uses some GCC style assebbler

//#pragma warning(disable:4800)   // forcing value to bool 'true' or 'false' (performance warning)

// template<[template-parameter-list]> using [your-alias] = [original-type];

// https://github.com/graphitemaster/incbin/blob/main/incbin.h

// MSVC(_MSC_VER), CLANG(__clang__), GCC(__GNUC__), ICC(__INTEL_COMPILER), ICX(__INTEL_LLVM_COMPILER)

// REF: https://github.com/chakra-core/ChakraCore/blob/master/lib/Common/CommonPal.h

// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// __builtin_bit_cast
// __builtin_trap (void)
// 
// https://github.com/DaemonEngine/Daemon/blob/master/src/common/Compiler.h
//------------------------------------------------------------------------------------------------------------

// NOTE: It is still possible to take an address of a 'constexpr' variable (which will make it actually allocated).
// Can't be 'inline' instead of 'static' - won't compile in function bodies.
#define SCVR static constexpr const

#ifdef __GNUC__              // CLANG defines it too
#define COMP_AGCC __GNUC__   // May be GCC, ICC, or something else
#endif
#ifdef __clang__              // Must be last here!
#define COMP_CLNG __clang__   // May define __GNUC__ or _MSC_VER
#pragma message(">>> Compiler is CLANG")
static constexpr bool IsMSVC = false;
#undef COMP_AGCC
#endif
#ifdef _MSC_VER
#define COMP_MSVC _MSC_VER
#pragma message(">>> Compiler is MSVC")
static constexpr bool IsMSVC = true;       // TODO: Test it with ClangCL   // Should be real MSVC only
#undef COMP_CLNG   // CLANG pretends to be MSVC
#undef COMP_AGCC
#endif


#define _ENTRY_POINT_NAME Mod_Enter_Proc_Main
#define _EXIT_POINT_NAME  Mod_Exit_Proc_Main

// https://abseil.io/docs/cpp/platforms/macros
// https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
//#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__i486__) || defined(__i586__) || defined(__i686__)    // None of those is recognized by CLANG
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#define CPU_ARM
#pragma message(">>> CPU is ARM")
static constexpr bool IsCpuARM = true;
static constexpr bool IsCpuX86 = false;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(__i386__) || defined(_M_X86)
static constexpr bool IsCpuARM = false;
#define CPU_X86
#pragma message(">>> CPU is X86")
static constexpr bool IsCpuX86 = true;
#else
#pragma message(">>> CPU is UNKNOWN")
static constexpr bool IsCpuARM = false;
static constexpr bool IsCpuX86 = false;
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(__amd64)
#define ARCH_X64
#pragma message(">>> Architecture is X64")
static constexpr bool IsArchX32 = false;
static constexpr bool IsArchX64 = true;
#else
#define ARCH_X32
#pragma message(">>> Architecture is X32")
static constexpr bool IsArchX32 = true;
static constexpr bool IsArchX64 = false;
#endif

// NOTE: MSVC just hardcodes LittleEndian anyway
#if (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)) || defined(__BIG_ENDIAN__) || defined(_big_endian__) || defined(_BIG_ENDIAN)
#define _BTORD_BE
static constexpr bool IsBigEndian = true;
#pragma message(">>> Byte order is Big endian")
#else
#define _BTORD_LE
static constexpr bool IsBigEndian = false;     // Since the Framework is for x86 and ARM its base operation mode is LittleEndian  // __BIG_ENDIAN__ // __BYTE_ORDER == __BIG_ENDIAN // __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#pragma message(">>> Byte order is Little endian")
#endif

// https://stackoverflow.com/questions/3378560/how-to-disable-gcc-warnings-for-a-few-lines-of-code
// TODO: Implement

//#define CPU_X86
//#define CPU_ARM
//#define ARCH_X32
//#define ARCH_X64 

// gnu::sysv_abi  
// gnu::regcall   __regcall 
// __vectorcall
//__attribute__((flag_enum))
// [[gnu::randomize_layout]] [[gnu::no_randomize_layout]] (C structure)  -frandomize-layout-seed=    -frandomize-layout-seed-file=
// [[clang::trivial_abi]]
// __attribute__((using_if_exists)) using empty_namespace::does_not_exist; // no error!
// [[no_unique_address]]  // On MSVC targets, [[no_unique_address]] is ignored; use [[msvc::no_unique_address]] instead.
// WASM: __attribute__((export_name(<name>)))   __attribute__((import_name(<name>))) 
// __attribute__((alloc_align(<alignment>))
// void *my_malloc(int a) __attribute__((alloc_size(1)));
// void *my_calloc(int a, int b) __attribute__((alloc_size(1, 2)));
// [[gnu::always_inline]]
// void *a() __attribute__((assume_aligned (32)));     // The returned pointer value has 32-byte alignment.
// void *b() __attribute__((assume_aligned (32, 4)));  // The returned pointer value is 4 bytes greater than an address having 32-byte alignment.
// __attribute__((clang_builtin_alias(__builtin_rvv_vadd_vv_i8m1)))  // ARM
// __attribute__((cpu_dispatch(ivybridge, atom, sandybridge)))
// __attribute__((cpu_specific(ivybridge, atom)))
// [[clang::disable_tail_calls]]
// int isdigit(int c) __attribute__((enable_if(c <= -1 || c > 255, "chosen when 'c' is out of range"))) __attribute__((unavailable("'c' must have the value of an unsigned char or EOF")));
// [[gnu::flatten]]       __attribute__((flatten))               // https://awesomekling.github.io/Smarter-C++-inlining-with-attribute-flatten/
// __attribute__ ((force_align_arg_pointer))
// [[gnu::malloc]]
// clang::min_vector_width
// clang::minsize
// gnu::no_caller_saved_registers 
// gnu::no_split_stack   //  -fsplit-stack
// gnu::no_stack_protector   // __attribute__((no_stack_protector)) and Microsoft style __declspec(safebuffers)      -fstack-protector compiler option
// clang::not_tail_called
// gnu::nothrow
// clang::nouwtable
// clang::optnone
// gnu::patchable_function_entry
// __attribute__((target("arch=atom"))) __attribute__((target("default")))   __attribute__((target_clones("arch=atom,avx2","arch=ivybridge","default")))
// gnu::zero_call_used_regs
//#pragma clang loop      [loop]
//#pragma nounroll
//#pragma unroll(16)      [unroll(4)]
// [[likely]]
// [[unlikely]]
// musttail  clang::musttail    // The target function must have the same number of arguments as the caller.
// [[clang::nomerge]] 
// __ptr32, __ptr64  //  -fms-extensions
// __sptr , __uptr
//  typedef double * aligned_double_ptr __attribute__((align_value(64)));
// gnu::alias       // void __f (){}  //  void f () __attribute__ ((weak, alias ("__f")));
// clang::amdgpu_kernel
// gnu::naked
// gnu::mode         // typedef unsigned int pointer __attribute__((mode(pointer)));  typedef unsigned int word __attribute__((mode(word)));  //  allow to explicitly specify a size for a type without depending on compiler or machine semantics, such as the size of 'long' or 'int'.
// gnu::packed
// gnu::returns_twice
//  [[clang::code_align(16)]] for (int i = 0; i < 10; ++i) var++;
// int x __attribute__((cleanup(foo)));    static void foo (int *) { ... }
//__attribute__((init_priority(101))) SomeType Obj2;
// void maybeundeffunc(int __attribute__((maybe_undef))param);
// [[maybe_unused]] void f([[maybe_unused]] bool thing1, [[maybe_unused]] bool thing2)
// gnu::model
// clang::no_destroy      -fno-c++-static-destructors
// void nonescapingFunc(__attribute__((noescape)) int *p) 
// __attribute__((pass_object_size(0)))  // __builtin_object_size(p, 0)
// [[clang::no_destroy]]  __attribute__((no_destroy))

#define _MaySkip [[maybe_unused]]


// TODO: Check that we have compatible PLT_* definitions
#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WINDOWS__) || defined(__NT__)  // Clang, GCC mode at least   // MSVC: _WIN64 - Defined as 1 when the compilation target is 64-bit ARM (Windows only?)or x64.
#pragma message(">>> OS is Windows")
#define SYS_WINDOWS
static constexpr bool IsSysWindows = true;
#else
static constexpr bool IsSysWindows = false;
#endif

#ifdef __ANDROID__        // Implies Linux, so check it first
#pragma message(">>> OS is Android")
#define SYS_ANDROID
static constexpr bool IsSysAndroid = true;
#else
static constexpr bool IsSysAndroid = false;
#endif

#ifdef __linux__    // Clang, GCC mode at least
#pragma message(">>> OS is Linux")
#define SYS_LINUX
static constexpr bool IsSysLinux = true;
#else
static constexpr bool IsSysLinux = false;
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)    // Linux too  // Clang, GCC mode at least
#pragma message(">>> OS is Unix")   // Linux too!
#define SYS_UNIX
#endif

#if defined(__APPLE__) && defined(__MACH__)    // __APPLE__  // Clang, GCC mode at least
#pragma message(">>> OS is MacOS")
#define SYS_MACOS                          // TODO: Replace with SYS_XNU  (MacOS, iOS)
static constexpr bool IsSysMacOS = true;
#else
static constexpr bool IsSysMacOS = false;
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)        // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#pragma message(">>> OS is BSD")
#define SYS_BSD
static constexpr bool IsSysBSD = true;
#else
static constexpr bool IsSysBSD = false;
#endif


#if (defined(__DEBUG) || defined(_DEBUG) || defined(DEBUG)) && !defined(NDEBUG) && !defined(_NDEBUG) && !defined(__NDEBUG)
#define DBGBUILD
#pragma message(">>> Building as DEBUG")
static constexpr bool IsDbgBuild = true;
#else
#pragma message(">>> Building as RELEASE")
static constexpr bool IsDbgBuild = false;
#endif

#if defined(PLT_UBOOT) || defined(PLT_WEBASM) || defined(PLT_SDL3)
#define SYS_ISOLATED
#endif

#define IFDBG if constexpr(IsDbgBuild)

// https://clang.llvm.org/docs/AttributeReference.html
#ifdef COMP_MSVC  // MSVC/CLANG_MSVC                            // __has_cpp_attribute

#if defined(ARCH_X32) && !defined(CPU_ARM)
#define _scall  __stdcall
#define _ccall  __cdecl
#define _fcall  __fastcall 
#define PXCALL  __cdecl
#else
#define _scall       // Not required
#define _ccall
#define _fcall
#define PXCALL
#endif

#define _RST
#define NOALIAS __declspec(noalias)
#pragma intrinsic(_ReturnAddress)
#pragma intrinsic(_AddressOfReturnAddress)  // Instead of #include <intrin.h>

#define UNREACHABLE() __assume(false)
#define GETSTKFRAME() _AddressOfReturnAddress()  // ARM? On ARM RetAddr is in LR register, not on stack (assuming first in the stack frame according to ABI) // Will not include address of some previous stack frame (from 'push rbp' at proc addr)    // CLANG:MSVC also supports __builtin_frame_address
#define GETRETADDR() _ReturnAddress()
//#define SETRETADDR(addr) (*(void**)_AddressOfReturnAddress() = (void*)(addr))    // Not on ARM? LR is usually pushed on stack
#define EXPORT_SYM __declspec(dllexport) extern "C" __attribute__((used)) 
#define RTL_SYM  extern "C" __attribute__((used))   // [[gnu::retain]]             // Prevents RTL functions from being removed by LTO 

#define _pinline __forceinline              // No 'flatten' with MSVC?      // inline void foo();  // Not between the return value and the function name (The compiler may give some obscure errors otherwise)
#define _ninline _declspec(noinline)
#define _finline __forceinline               // At least '/Ob1' is still required     [[msvc::forceinline]] ??
#define _minline inline                      // May be inlined
#define _naked __declspec(naked)
#define _noret __attribute__((noreturn))     // Clang/GCC only
#define _used  __attribute__((used))         // Without this attr the compiler will remove almost all records from SysApi array for Windows system when -O2 is enabled
//#pragma code_seg(".xtext")
#pragma section(".xtxt",execute,read)        // 'execute' will be ignored for a data declared with _codesec if the section name is '.text', another '.text' section will be created, not executable
#define _codesec _declspec(allocate(".xtxt"))
#define _codesecn(n) _declspec(allocate(".xtxt"))
#pragma comment(linker,"/MERGE:.xtxt=.text")     // Without this SAPI struct won`t go into executable '.text' section
#else   // CLANG/GCC   (Even if building for Windows)

#if defined(ARCH_X32) && !defined(CPU_ARM)
#define _scall  __attribute__((stdcall))
#define _ccall  __attribute__((cdecl))
#define _fcall  __attribute__((fastcall))
#define PXCALL  __attribute__((cdecl))
#else
#define _scall       // Not required
#define _ccall
#define _fcall
#define PXCALL
#endif

#define _RST __restrict           // void fr (float * __restrict dest)   // https://stackoverflow.com/questions/1965487/does-the-restrict-keyword-provide-significant-benefits-in-gcc-g
#define NOALIAS __attribute__((noalias))
#define UNREACHABLE() __builtin_unreachable()
// https://gcc.gnu.org/onlinedocs/gcc/Return-Address.html
// NOTE: __builtin_frame_address does not return the frame address as it was at a function entry. It points at the current function`s frame, including all its local variables
#define GETSTKFRAME() __builtin_frame_address(0)  // NOTE: MINGW implements _AddressOfReturnAddress() with it  // Anything except 0 will use a stack frame register according to specified ABI // TODO: Rework! // On ARM there is no RetAddr on stack  // Should not include address of some previous stack frame (from 'push rbp' at proc addr)
#define GETRETADDR() __builtin_extract_return_addr(__builtin_return_address(0))
//#define SETRETADDR(addr) (*(void**)__builtin_frame_address(0) = __builtin_frob_return_addr((void*)(addr)))  // ARM?
// TODO: _oinline - inlining for obfuscation when enabled (_finline is for optimization)
#define RTL_SYM  extern "C" __attribute__((used))  // [[gnu::retain]]             // Prevents RTL functions from being removed by LTO 

#define _pinline __attribute__((flatten))         // Inlines everything inside the marked function
#define _ninline __attribute__((noinline))
#define _finline __attribute__((always_inline))
#define _minline inline                      // May be inlined
#define _naked   __attribute__((naked))
#define _noret  __attribute__((noreturn))      // [[noreturn]] is ignored for some reason
#define _used __attribute__((used))       // Without it Clang will strip every reference to syscall stubs when building for ARM
#ifdef SYS_MACOS
#define _codesec __attribute__ ((section ("__TEXT,__text")))             // [[gnu::section(".xtxt")]]  // ???
#define _codesecn(n) __attribute__ ((section ("__TEXT,__text" #n )))
#define EXPORT_SYM extern "C" __attribute__((__visibility__("default")))  __attribute__((used))   // [[gnu::retain]]      // Will __visibility__ work for MACHO  too?
#elifdef SYS_WINDOWS   // WINDOWS with GCC driver
#define _codesec __attribute__ ((section (".xtxt")))     // NOTE: For PE exe sections any static inline MEMBER(not a global data) will go into a separate data section named as '.text'  // Either merge sections by linker or put such data outside of any struct/class
#define _codesecn(n) __attribute__ ((section (".xtxt." #n )))   // ELF section format: 'secname.index' goes to secname
#pragma comment(linker,"/MERGE:.xtxt=.text")      // This will work with '-fms-extensions' otherwise a linker script will be needed to merge these two sections
#define EXPORT_SYM __declspec(dllexport) extern "C" __attribute__((used))      // Why __declspec in GCC mode?
#else  // LINUX-ELF
#define _codesec __attribute__ ((section (".text")))
#define _codesecn(n) __attribute__ ((section (".text." #n )))
#define EXPORT_SYM extern "C" __attribute__((__visibility__("default")))  __attribute__((used))   // [[gnu::retain]]      // __visibility__ is ELF specific
#endif
#endif

// On x86 compiler expects return address on stack and calculates stack alignment based on this
// But system entry point(from kernel)does not stores any return address and keeps the stack aligned to 16
//
#ifdef CPU_X86
#define _SYSENTRY extern "C" __attribute__((force_align_arg_pointer))     //  __attribute__ ((section ("entry")))
#else
#define _SYSENTRY extern "C"
#endif

//#if defined(CPU_X86)
//#define YieldCPU() _mm_pause    //  __asm { rep nop }
//#elif defined(CPU_ARM)
//#define YieldCPU() __yield      // asm volatile("yield")
//#endif

#ifdef SYS_LINUX
// Will create .init_array (dlopen on modern linux will not use old method)  // init_array does not use relative offets and will cause a reloc section creation
#define MODERN_INIT __attribute__((constructor))   // Adds DT_INIT_ARRAY+DT_INIT_ARRAYSZ // Needed bacause MUSL may be compiled without DT_INIT support (NO_LEGACY_INITFINI)
#define MODERN_FINI __attribute__((destructor))
#else
#define MODERN_INIT
#define MODERN_FINI
#endif

//#define _NOMANGL extern "C" __attribute__((internal_linkage))    // Removes mangling and marks as internal again (Not working with memset and others)

// For exported functions
#ifdef FWK_NO_EXTERN
#define _EXTERNC
#else
#define _EXTERNC extern "C"
#endif

// Works on both Clang and GCC (automatically defined to 16 when __int128 is supported as a real scalar type)
#ifdef __SIZEOF_INT128__
#define HAS_INT128 1
static constexpr bool HaveScalarI128 = true;
#else 
static constexpr bool HaveScalarI128 = false;
#endif

// NOTE: Stack will be restored at function's end even if it is inlined (Clang)
// https://stackoverflow.com/questions/55177353/constructing-a-function-pointer-to-alloca-causes-linker-errors
// C++ is not a data friendly language :(
#define StkAlloc(x) __builtin_alloca(x)
/*
#if defined(PLT_WIN_USR) || defined(PLT_WIN_KRN)

#ifdef CPU_X86

#ifdef ARCH_X64
#define SYSDESCPTR ((void*)__readgsqword(0x60))   // PEB
#elif defined(ARCH_X32)
#define SYSDESCPTR ((void*)__readfsdword(0x30))   // PEB
#else
#error "SYSDESCPTR Architecture Unsupported"
#endif

#elif defined(CPU_ARM)
#error "SYSDESCPTR Windows ARM Unsupported"
#else
#error "SYSDESCPTR CPU Unsupported"
#endif

#else
#define SYSDESCPTR  &((void**)__builtin_frame_address(0))[1]  // A pointer to 'ELF Auxiliary Vectors'  // First size_t is nullptr for a return addr
#endif
*/
//#define STKFRAMEPTR  &((void**)__builtin_frame_address(0))[1]  // A pointer to 'ELF Auxiliary Vectors'  // First size_t is nullptr for a return addr


//#ifdef FWK_DEBUG
#define STASRT(...) static_assert(__VA_ARGS__)
//#else
//#define STASRT(cond,txt)
//#endif

#ifndef COMP_MSVC
// Another injected struct without injected members:(
constexpr const char* My__builtin_FUNCSIG(auto* rt = __builtin_source_location()) noexcept   // Cannot use the name '__builtin_FUNCSIG' even if it is available only for MSVC compatibility mode
{
 return static_cast<const std::source_location::__impl*>(rt)->_M_function_name;
}
#endif

// These three work at a call site
#define SRC_LINE __builtin_LINE()
#define SRC_FILE __builtin_FILE()        // Full path included
#define SRC_FUNC __builtin_FUNCTION()    // Only the name itself, no arguments or a return type (like __func__)  // __builtin_FUNCSIG() ?? // TODO: Use __builtin_source_location() to get full signatures

#define FUNC_NAME __func__

// To retrieve a function signature including all argument types
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#define FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define FUNC_SIG __FUNC__
#else
#define FUNC_SIG __func__   // Useless, because no types are provided  (Part of C++ 11 standart)
#endif

// Allowed to force inline
#ifdef FWK_INLINE_ALL
#define FWMAYINL _finline
#define FWINLINE _finline
#else
#define FWMAYINL
#define FWINLINE
#endif

// NOTE: [no_unique_address] implies that a next member will take that space(Previous too, with some compilers). i.e. If four or less [no_unique_address] are followed by 'int' then the struct size will be 4.
//       Add another empty member and another alignment amount will be added to struct size
//       But ICC behaves differently and allocates space if there are more than one empty member in a sequence.
#if defined(_MSC_VER)
#define NO_UNIQUE_ADDR [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDR [[no_unique_address]]
#endif

// https://gist.github.com/graphitemaster/494f21190bb2c63c5516
#define OffsetOf(type, member) __builtin_offsetof(type, member)
// MSVC: #define offsetof(s,m) ((::size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
//template<typename Base, typename Class> constexpr std::size_t offsetOf( Base Class::*r ){return (size_t)&(((Class *)nullptr)->*r);}    // offsetOf( &X::b )   // The code can be compiled without any problems with g++ and clang but not with MSVC.

#define UNUSED(expr) do { (void)(expr); } while (0)

// Interface validator macros
#define API_VALIDATE(fname,rettype,args...) static_assert(SameType<typename RemovePtr<decltype(static_cast<rettype(*)(args)>(fname))>::T, rettype (args)>::V, "");    // This will work with overloaded functions, including func(void)
//#define API_VALIDATE(fname,rettype,args...) static_assert(SameType<decltype(fname), rettype (args)>::V, "");
#define API_MUSTMATCH(orig,impl)         // No overloads!


// This helps to create a function wrapper without copying its declared signature (If signatures is changed frequently during redesign it is a big hassle to keep the interfaces in sync)
// Setting SAPI here as default helps to get rid of lambdas which generate too much assignment code:
//   {return [&]<typename T=SAPI>() _finline {if constexpr(requires { typename T::unlink; })return T::unlink(args...); else return T::unlinkat(PX::AT_FDCWD, args..., 0);}();}
//   if constexpr(requires { Y::rename; })return Y::rename(args...); else return [](achar* oldpath, achar* newpath) _finline {return Y::renameat(PX::AT_FDCWD,oldpath,PX::AT_FDCWD,newpath);}(args...);
//
#define FUNC_WRAPPERFI(Func,Name) template<typename Y=SAPI, typename... Args> static _finline auto Name(Args ... args) -> decltype(((decltype(Func)*)nullptr)(args...))  // Name(Args&& ... args) ? - refs create too much code! // Argument errors is more verbose when using nullptr decltype instead of direct type here        // template<typename Func, typename... Args> auto Name(Args&& ... args) -> decltype(((Func*)nullptr)(args...)) {}
#define FUNC_WRAPPERNI(Func,Name) template<typename Y=SAPI, typename... Args> static _ninline auto Name(Args ... args) -> decltype(((decltype(Func)*)nullptr)(args...))  // Name(Args&& ... args) ? - refs create too much code! // Argument errors is more verbose when using nullptr decltype instead of direct type here        // template<typename Func, typename... Args> auto Name(Args&& ... args) -> decltype(((Func*)nullptr)(args...)) {}
#define FUNC_WRAPWOW64(Name) template<typename Y=WOW64, typename... Args> static _ninline auto Name(Args ... args) -> decltype(((decltype(NT::Name)*)nullptr)(args...))

#define CALL_IFEXISTR(fchk,falt,farg,faarg) if constexpr(requires { Y::fchk; })return Y::fchk farg; else return Y::falt faarg;
#define CALL_IFEXISTRN(fchk,falt,anams,farg,faarg) if constexpr(requires { Y::fchk; })return Y::fchk farg; else return anams::falt faarg;
#define CALL_IFEXISTRP(fchk,falt,farg,faarg,apar) if constexpr(requires { Y::fchk; })return Y::fchk farg; else return [] apar _finline {return Y::falt faarg;} farg;
#define CALL_IFEXISTRPC(fchk,falt,cond,farg,faarg,apar) if constexpr((cond))return Y::fchk farg; else return [] apar _finline {return Y::falt faarg;} farg;

#define CALL_RIFEXISTR(fchk,falt,farg,faarg) if constexpr(requires { Y::fchk; })res = Y::fchk farg; else res = Y::falt faarg;

struct SEmptyType {};    // [[no_unique_address]]  ?
using ETYPE = SEmptyType[0];   // As a member // This works plain and simple with ICC, GCC, CLANG (You can get a completely empty struct with such members and no annoying padding rules) but MSVC reports an error 'error C2229: struct has an illegal zero-sized array'
// MSVC: sizeof(ETYPE) : error C2070: 'ETYPE': illegal sizeof operand
// CLANG: With '--target=x86_64-windows-msvc' you cannot get an empty struct with such members, its size will be at least 1 byte

// Conclusion: [no_unique_address] feature is a mess :(  Forget about it and use ETYPE instead. At least it works somehow and it is predictable.
//------------------------------------------------------------------------------------------------------------
template<bool UseTypeA, typename A, typename B> struct TSW;                   // Usage: TSW<MyCompileTimeCondition, MyStructA, MyStructB>::T Val;   // Same as std::conditional
template<typename A, typename B> struct TSW<true, A, B> {using T = A;};
template<typename A, typename B> struct TSW<false, A, B> {using T = B;};

// Helper alias to match std::conditional_t
template<bool V, typename A, typename B> using TSWT = typename TSW<V, A, B>::T;

template<bool UseTypeA, typename A, typename B> constexpr static auto TypeSwitch(void)       // Usage: struct SMyStruct { decltype(TypeSwitch<MyCompileTimeCondition, MyStructA, MyStructB>()) Val; }
{
 if constexpr (UseTypeA) {A val{0}; return val;}
   else {B val{0}; return val;}
}
//------------------------------------------------------------------------------------------------------------
//template<typename T, typename U> struct SameType {enum { V = false };};   // Cannot be a member of a local class or a function
//template<typename T> struct SameType<T, T> {enum { V = true };};

template<typename> struct TrueTypeT {static constexpr const bool V = true;};       // inline constexpr bool  
template<typename> struct FalseTypeT {static constexpr const bool V = false;};

using TrueType  = TrueTypeT<void>;
using FalseType = FalseTypeT<void>;

template<typename T, typename U> struct SameType : FalseType {};   // Cannot be a member of a local class or a function
template<typename T> struct SameType<T, T> : TrueType {};

template<typename A, typename B> constexpr _finline static bool IsSameType(A ValA, B ValB)
{
 UNUSED(ValA); UNUSED(ValB);
 return SameType<A, B>::V;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> consteval static auto TypeToSigned(T Val=0)
{
 if constexpr (1 == sizeof(T))return (signed char)Val;
 else if constexpr (2 == sizeof(T))return (signed short)Val;
 else if constexpr (4 == sizeof(T))return (signed int)Val;
 else if constexpr (8 == sizeof(T))
  {
   if constexpr (SameType<T, unsigned long>::V)return (signed long)Val;     // X64 mostly   // decltype(sizeof(void*)) is 'unsigned long' there
     else return (signed long long)Val;
  }
#ifdef HAS_INT128
 else if constexpr (16 == sizeof(T))return (signed __int128)Val;
#endif
}
//------------------------------------------------------------------------------------------------------------
template<typename T> consteval static auto TypeToUnsigned(T Val=0)
{
 if constexpr (1 == sizeof(T))return (unsigned char)Val;
 else if constexpr (2 == sizeof(T))return (unsigned short)Val;
 else if constexpr (4 == sizeof(T))return (unsigned int)Val;
 else if constexpr (8 == sizeof(T))
  {
   if constexpr (SameType<T, unsigned long>::V) return (unsigned long)Val;
     else return (unsigned long long)Val;
  }
#ifdef HAS_INT128
 else if constexpr (16 == sizeof(T))return (unsigned __int128)Val;
#endif
}
//------------------------------------------------------------------------------------------------------------
template<typename V> struct MakeSigned { using T = decltype(TypeToSigned<V>()); };
template<typename V> struct MakeUnsigned { using T = decltype(TypeToUnsigned<V>()); };
template<typename Ty> using MakeUnsignedT = typename MakeUnsigned<Ty>::T;
//------------------------------------------------------------------------------------------------------------
template<typename T> consteval static auto ChangeTypeSign(T Val=0)  // Should be compiled faster than a template specialization?
{
 if constexpr (T(-1) < T(0))return TypeToUnsigned<T>(Val);   // IsSigned
   else return TypeToSigned<T>(Val);
}
//------------------------------------------------------------------------------------------------------------

constexpr static bool Is64BitBuild(void){return sizeof(void*) == 8;}   // To be used in constexpr expressions instead of __amd64__ macro
//------------------------------------------------------------------------------------------------------------
static _finline void YieldCPU()
{
#if defined(CPU_X86)
 _mm_pause();    //  __asm { rep nop }
#elif defined(CPU_ARM)
 __yield();      // asm volatile("yield")
#else
 //  __asm__ volatile("" ::: "memory");
#endif
}
//------------------------------------------------------------------------------------------------------------
// NOTE: You must use these types if you want code randomization to be applied
namespace NGT   // NGenericTypes (Shortenrd to have shorter type dumps) // You should do 'using' for it yourselves if you want to bring these types to global name space   // If this is not Namespace than this would not be possible: 'using namespace NFWK::NGenericTypes;'
{
#ifdef FWK_DEBUG
 static_assert(1 == sizeof(unsigned char), "Unsupported size of char!");
 static_assert(2 == sizeof(unsigned short), "Unsupported size of short!");
 static_assert(4 == sizeof(unsigned int), "Unsupported size of int!");
 static_assert(8 == sizeof(unsigned long long), "Unsupported size of int64!");
 static_assert(4 == sizeof(float), "Unsupported size of float!");
 static_assert(8 == sizeof(double), "Unsupported size of double!");
 static_assert(sizeof(void*) == sizeof(decltype(sizeof(void*))), "Unsupported size of size_t!");
#endif

/* https://unix.org/version2/whatsnew/lp64_wp.html
Datatype	LP64	ILP64	LLP64	ILP32	LP32
char	    8	    8	    8	    8	    8
short	    16	    16	    16	    16	    16
_int32		32
int	        32	    64	    32	    32	    16
long	    64	    64	    32	    32	    32
long long			64
pointer	    64	    64	    64	    32	    32
*/

// Trying to sort out the whole history of type mess (If some platform does not support any of these, its compiler should implement missing operations)
 using achar   = char;      // Different platforms may use different sizes for it(?). Since C++11: u8"Hello" to define a UTF-8 string of chars
 using wchar   = wchar_t;   // Different platforms may use different sizes for it
 using charb   = char8_t;   // u8"" // cannot be signed or unsigned
 using charw   = char16_t;  // u""  // cannot be signed or unsigned
 using chard   = char32_t;  // U""  // cannot be signed or unsigned
#ifdef SYS_WINDOWS           // Default char, used on target system (achar, except Windows)
 using syschar = wchar;
#else
 using syschar = achar;
#endif
 using uint8   = unsigned char;      // 'char' can be signed or unsigned by default
 using uint16  = unsigned short int;
 using uint32  = unsigned int;       // Expected to be 32bit on all supported platforms  // NOTE: int is 32bit even on x64 platforms, meaning that using 'int' everywhere is not architecture friendly
 using uint64  = unsigned long long; // 'long long unsigned int' or 'long unsigned int' ???  // See LP64, ILP64, ILP32 data models on different architectures  // On x64 may be not compatible with size_t (unsigned long)

 using sint8   = signed char;
 using sint16  = signed short int;
 using sint32  = signed int;
 using sint64  = signed long long;   // __int64_t

#ifdef ARCH_X64          // Not present on X32
 using uint128 = unsigned __int128;
 using sint128 = __int128;
#else
// using int128_t = int __attribute__((mode(TI)));   // !!! https://gcc.gnu.org/onlinedocs/gccint/Machine-Modes.html
using uint128 = decltype(sizeof(void*)) __attribute__((__vector_size__(16), __aligned__(16), __may_alias__));
using sint128 = decltype(sizeof(void*)) __attribute__((__vector_size__(16), __aligned__(16), __may_alias__));
#endif

 using int8    = sint8;
 using int16   = sint16;
 using int32   = sint32;
 using int64   = sint64;   // __int64_t

 using uint    = decltype(sizeof(void*));   // These 'int' are always platform-friendly (same size as pointer type, replace size_t) // "The result of sizeof and sizeof... is a constant of type std::size_t"
 using sint    = decltype(ChangeTypeSign(sizeof(void*)));   // NOTE: use of arch`s default type size by default is better for ARM(no easy way to get/set half of a register) and worse for X86(wasted register halves could be used for something else)
 using vptr    = void*;
 using bptr    = uint8*;
 using wptr    = uint16*;
 using dptr    = uint32*;
 using qptr    = uint64*;
 using vcptr   = const void*;
 using bcptr   = const uint8*;
 using wcptr   = const uint16*;
 using dcptr   = const uint32*;
 using qcptr   = const uint64*;
 using usize   = uint;    // Should replace size_t (altough just using 'uint' is more intuitive (If size is not specified then it is of the current arch's default size))
 using ssize   = sint;    // isize?   // ui,si,u8,u16,u32,u64 (too short, hard to notice)? 

 using time_t    = int64; // Modern time_t is 64-bit
 using size_t    = uint;  // To write a familiar type convs
 using ssize_t   = sint;  // TODO: Stop spreading this '_t' naming ?
 using uintptr_t = uint;
 using nullptr_t = decltype(nullptr);

 // Add definitions for floating point types?
 using flt32   = float;
 using flt64   = double;

};

static_assert(sizeof(NGT::uint) == sizeof(void*));   // Make sure that the compiler behaves properly

using PTRTYPE64  = typename NGT::uint64;
using PTRTYPE32  = typename NGT::uint32;
using PTRCURRENT = typename NGT::uint;

using namespace NGT;   // For 'Framework.hpp'   // You may do the same in your code if you want
//------------------------------------------------------------------------------------------------------------

static constexpr unsigned int StkAlign = Is64BitBuild()?8:4;   // X86,ARM     // ARM is 8, x86 16(32 is better)
static constexpr unsigned int PtrBIdx  = Is64BitBuild()?3:2;

#ifndef va_start
typedef __builtin_va_list va_list;        // Requires stack alignment by 16
#if defined __has_builtin    // GCC/CLANG
#  if __has_builtin (__builtin_va_start)
#    define va_start(ap, param) __builtin_va_start(ap, param)
#    define va_end(ap)          __builtin_va_end(ap)
#    define va_arg(ap, type)    __builtin_va_arg(ap, type)
#  endif
#endif

#endif
//------------------------------------------------------------------------------------------------------------

#if defined(SYS_MACOS) || defined(SYS_BSD)    // Proprocessor should be faster than 'if constexpr' (probably)
 static consteval auto VSLB(_MaySkip auto vLINUX, auto vBSD_MAC) { return vBSD_MAC; }   // Value select Linux/BSD(MacOS)
#else
 static consteval auto VSLB(auto vLINUX, _MaySkip auto vBSD_MAC) { return vLINUX; }
#endif

#if defined(CPU_ARM)   // Proprocessor should be faster than 'if constexpr' (probably)
 static consteval auto VSIA(auto vARM, _MaySkip auto vX86) { return vARM; }    // Value select ARM/x86
#else
 static consteval auto VSIA(_MaySkip auto vARM, auto vX86) { return vX86; }
#endif

//------------------------------------------------------------------------------------------------------------
// NOTE:  Dump here any implementations that should be accessible early and have no personal HPP yet >>>

//------------------------------------------------------------------------------------------------------------
// Some type traits
// https://github.com/mooncollin/CPPAdditions/tree/main

//https://stackoverflow.com/questions/64060929/why-does-the-implementation-of-declval-in-libstdc-v3-look-so-complicated
//  template<typename _Tp, typename _Up = _Tp&&>  _Up __declval(int);
//  template<typename _Tp> _Tp __declval(long);
//  template<typename _Tp> auto reclval() noexcept -> decltype(__declval<_Tp>(0));
// Ihis is fine... for now
template <typename T> T declval();        // Not really useful to use an overloaded functions with decltype
//template<class T> T&& declval() noexcept;   // *only declared, never defined*

// Check if two types are variants of same template  // static_assert(SameKind<decltype(v1), std::vector>::V, "no match");
template<typename Test, template<typename...> class Ref> struct SameKind : FalseType {};        // is_specialization
template<template<typename...> class Ref, typename... Args> struct SameKind<Ref<Args...>, Ref> : TrueType {};

//template<class Type, template<typename...> class Template> inline constexpr bool SameKindV = SameKind<Type, Template>::V;

/*template<typename A, typename B> constexpr _finline static bool IsSameKind(A ValA, B ValB)
{
 UNUSED(ValA); UNUSED(ValB);
 return SameKind<A, Base>::V && SameKind<B, Base>::V;     // !!! How to get the 'Base' from A and B???    // https://godbolt.org/z/vnbfa6v7W
}*/

template<class T> struct IsIntegralLike 
{
private:
 // Primary overload chosen only if "declval<T>() & 1" is a valid unevaluated expression
 template<class U, class = decltype( (declval<U>() & 1), void() )> static TrueTypeT<U> test(int);
 template<class> static FalseTypeT<void> test(...);
public:
 static constexpr bool V = decltype(test<T>(0))::V;
};

template<class T> concept IsIntegral   = IsIntegralLike<T>::V;
template<class T> concept CanDoBitOps  = requires(T x) { x & int(1); };

// the compiler is required to treat T(1) / T(0) for T = int as ill-formed even in an unevaluated constant-expression context, not just "substitution failure", so it cannot be turned into a nice SFINAE check.
//template<class T> concept CanDivByZero = requires(T x) { []() constexpr {constexpr T v = T(1) / T(0);(void)v;}(); }    //constexpr const y = x / int(1); };    // uint32(1) >> 32;

template<class T> concept IsFltPoint   = (!CanDoBitOps<T>); 
template<class T> concept IsArithmetic = (IsIntegral<T> || IsFltPoint<T>); 

template<class T> concept SubscriptableAsArray = requires(T x) { x[0]; };
template<class T> concept IsScalar128 = (sizeof(T) == 16) && (!SubscriptableAsArray<T>);  // is it a 128-bit scalar?
template<class T> concept IsVectorExt = SubscriptableAsArray<T>;  // is it a vector extension type (any size)?

template<typename T> struct BitSize {using ST = decltype(sizeof(T)); static constexpr const ST V = sizeof(T) * ((ST)8);};
template<typename T> constexpr T GetBitSize(T x){return BitSize<T>::V;}
//------------------------------------------------------------------------------------
//template<typename T> struct IsSigned : TSW<(T(-1) < T(0)), TrueTypeT<T>, FalseTypeT<T>>::T {};   // Add IsArithmetic check?

template<typename T> struct IsSigned { static const bool V = (T(-1) < 0); };
template<typename T> struct IsUnsigned { static const bool V = (T(-1) >= 0); };     // template<typename T> constexpr _finline static bool IsPositive(void){return (T(-1) >= 0);}    // constinit
template<typename T> constexpr bool IsSignedV = IsSigned<T>::V;
template<typename T> constexpr bool IsUnsignedV = IsUnsigned<T>::V;

template<bool condition> struct warn_if{};
template<> struct [[deprecated]] warn_if<false>{constexpr warn_if() = default;};     // NOTE: MSVC have problems with this!
#define static_warn(x, ...) ((void) warn_if<x>())
//------------------------------------------------------------------------------------------------------------
template<typename Ty> struct RemoveRef { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&> { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&&> { using T = Ty; };
template<typename Ty> using  RemoveRefT = typename RemoveRef<Ty>::T;

template<typename T> RemoveRefT<T>& rvalcast(RemoveRefT<T> &&t){return static_cast<RemoveRefT<T> &>(t);}
template<typename T> auto __inline rvptr(T&& v){return &v;}
//------------------------------------------------------------------------------------------------------------
template <typename Ty> struct TyIdent { using T = Ty; };

// https://stackoverflow.com/questions/17644133/function-that-accepts-both-lvalue-and-rvalue-arguments
// NOTE: Use this only to pass an unused temporaries as unneeded return values of a function
// EXAMPLE: ARef<typename RemoveRef<typename TyIdent<T>::T>::T> res
// NOTE: The compiler will make a copy if a passed type is not same as type of REF (i.e. you pass an uint when type of REF is sint) (FIXED?)
/*template <typename Ref> struct ARef       // Universal ref wrapper  // NOTE: DO NOT USE! Use 'auto&&' and ignore any possible mistakes (It is an universal reference and will behave like actual return value by implicit conversion)
{
 Ref& ref;

// constexpr _ninline explicit ARef(Ref&& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }   // RValue     // Using the class` type Ref leaves us wuthout Universal Reference. But making the constructor template will break type conversion on assignment
// constexpr _ninline explicit ARef(Ref& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }    // LValue

// constexpr _ninline ARef(const auto& arg): ref(arg) {} //: ref((typename RemoveRef<Ref>::T&&)arg) { }    // LValue
 constexpr _ninline ARef(auto&& arg): ref(arg) {}  // Universal reference  //: ref((typename RemoveRef<Ref>::T&&)arg) { }   // RValue     // Using the class` type Ref leaves us wuthout Universal Reference. But making the constructor template will break type conversion on assignment
// constexpr _ninline explicit ARef(volatile Ref& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }    // LValue for a volatile storage (Can removing the 'volatile' break some use cases of it bacause of optimization?)  // Can we pass a type to ARef without losing its volatility?

// constexpr _ninline Ref& operator=(ARef<Ref> const& v){ref = v; return ref; }
// constexpr _ninline Ref& operator=(auto& v){ref = v; return ref; }
 constexpr _ninline Ref& operator=(auto&& v){ref = v; return ref; }   // Universal reference
 constexpr _ninline operator Ref& () const & { return ref; }
 constexpr _ninline operator Ref&& () const && { return (typename RemoveRef<Ref>::T&&)ref; }
 constexpr _ninline Ref& operator*() const { return ref; }
 constexpr _ninline Ref* operator->() const { return &ref; }
};

// EXAMPLE: XRef<T> res
template<typename T> using XRef = ARef<typename RemoveRef<typename TyIdent<T>::T>::T>;    // Template Alias
*/
//--------------
template<typename Ty> struct RemovePtr { using T = Ty; };
template<typename Ty> struct RemovePtr<Ty*> { using T = Ty; };
template<typename Ty> struct RemovePtr<Ty* const> { using T = Ty; };
template<typename Ty> struct RemovePtr<Ty* volatile> { using T = Ty; };
template<typename Ty> struct RemovePtr<Ty* const volatile> { using T = Ty; };

template<typename Ty> using RemovePtrT = typename RemovePtr<Ty>::T;
//--------------

template <int SizeInBytes> struct TypeForSizeU
{
 using T = typename TSW<(SizeInBytes < 8), typename TSW<SizeInBytes < 4, typename TSW<SizeInBytes < 2, uint8, uint16>::T, uint32>::T, uint64>::T;    // without 'typename' the compiler refuses to look for TSW type
};

template <int SizeInBytes> struct TypeForSizeS
{
 using T = typename TSW<(SizeInBytes < 8), typename TSW<SizeInBytes < 4, typename TSW<SizeInBytes < 2, sint8, sint16>::T, sint32>::T, sint64>::T;    // without 'typename' the compiler refuses to look for TSW type
};

template<int SizeInBytes, bool Unsign=true> using TypeForSizeT = typename TSW<Unsign, typename TypeForSizeU<SizeInBytes>::T, typename TypeForSizeS<SizeInBytes>::T>::T;
//------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/46404503/can-i-implement-maxa-maxb-maxc-d-using-fold-expressions

template <unsigned int MinVal=sizeof(void*), typename ... Ts> constexpr _finline size_t SizeOfMaxInPPack(const Ts&... args)
{
 size_t ret { MinVal };
 if constexpr (sizeof...(args)){( (ret = (sizeof(Ts) > ret ? sizeof(Ts) : ret)), ... );}    // Max
 return ret;
}

template <unsigned int MinVal=sizeof(void*), typename ... Ts> constexpr _finline size_t SizeOfMaxInTPack(void)
{
 size_t ret { MinVal };
 return ( (ret = (sizeof(Ts) > ret ? sizeof(Ts) : ret)), ... );
}

// TODO: C++23 replace  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2662r3.pdf    // Pack Indexing (Clang 19)
// Generates a lot of back and forth local variable moves of all values in the pack on each value request!!! (O0)
template<int at, int idx=0> static constexpr _finline auto GetParFromPk(const auto first, const auto... rest)    // NOTE: With auto& it generates more complicated code (Even when inlined there is a lot of taking refs into local vars)
{
 if constexpr(idx == at)return first;
  else return GetParFromPk<at,idx+1>(rest...);
}

/*static volatile __m128 me = {7,7,7,7};
static_assert(SizeOfMaxInPPack((int)1, (char)2, (double)3.0, (short)4, me) == 16);
static_assert(SizeOfMaxInPPack((int)1, (char)2, (double)3.0, (short)4) == 8);
static_assert(SizeOfMaxInPPack((char)1, (float)2) == sizeof(float));
static_assert(SizeOfMaxInPPack((int)1, (char)2) == 4); */

//------------------------------------------------------------------------------------------------------------
template<typename T> struct IsRef : FalseTypeT<T> {};      // Primary template - assumes not a reference
template<typename T> struct IsRef<T&> : TrueTypeT<T> {};   // Specialization for lvalue references
template<typename T> struct IsRef<T&&> : TrueTypeT<T> {};  // Specialization for rvalue references
template<typename T> inline constexpr bool IsRefV = IsRef<T>::V;   // C++17 variable template helper
//------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/3177686/how-to-implement-is-pointer

// These are useful with 'auto' arguments
//template <typename T> static consteval bool IsPointer(T const &t) {return false;}
//template <typename T> static consteval bool IsPointer(T *t) {return true;}
//template<typename T> struct IsPtrType { static constinit bool V = false; };
//template<typename T> struct IsPtrType<T*> { static constinit bool V = true; };


template <typename Ty> struct RemoveConst {typedef Ty T;};
template <typename Ty> struct RemoveConst<const Ty> {typedef Ty T;};
template <typename Ty> struct RemoveVolatile {typedef Ty T;};
template <typename Ty> struct RemoveVolatile<volatile Ty> {typedef Ty T;};
template <typename Ty> struct RemoveCV : RemoveConst<typename RemoveVolatile<Ty>::T> {};
template <typename Ty> struct IsUPtrType {enum { V = false };};
template <typename Ty> struct IsUPtrType<Ty*> {enum { V = true };};
template <typename Ty> struct IsPtr : IsUPtrType<typename RemoveCV<Ty>::T> {};
template <typename Ty> inline constexpr bool IsPtrV = IsPtr<Ty>::V;   // C++17 variable template helper
template <typename Ty> static consteval bool IsPointer(const Ty&){return IsPtr<Ty>::V;}   // Is this one better?

consteval auto ConstEval(auto v){return v;}     // Most constexpr functions prefer to not be evaluated at compile time, even if they can

template<typename Ty> concept NoPtr = !IsPtr<Ty>::V;
template<typename Ty> concept NoRef = !IsRef<Ty>::V;
template<typename Ty> concept Plain = !IsPtr<Ty>::V && !IsRef<Ty>::V;
template<typename Ty> concept Integral = IsIntegral<Ty>;
//------------------------------------------------------------------------------------------------------------
//template<typename T> struct is_array_helper : FalseType {};
//template<typename T, size_t N> struct is_array_helper<T[N]> : TrueType {};
//template<typename T> struct is_array_helper<T[]> : TrueType {};
//template<typename T, size_t N> struct is_array_helper<T(&)[N]> : TrueType {};
//template<typename T> struct is_array_helper<T(&)[]> : TrueType {};
//
//template<typename T> constexpr bool IsArray = is_array_helper<RemoveRefT<T>>::V;
//------------------------------------------------------------------------------------------------------------

//template<typename T> constexpr _finline static int32 AddrToRelAddr(T CmdAddr, unsigned int CmdLen, T TgtAddr){return -((CmdAddr + CmdLen) - TgtAddr);}         // x86 only?
//template<typename T> constexpr _finline static T     RelAddrToAddr(T CmdAddr, unsigned int CmdLen, int32 TgtOffset){return ((CmdAddr + CmdLen) + TgtOffset);}  // x86 only?

template <typename T> constexpr _finline static T Min(T ValA, T ValB){return (ValA < ValB)?ValA:ValB;}
template <typename T> constexpr _finline static T Max(T ValA, T ValB){return (ValA > ValB)?ValA:ValB;}

template <typename T> constexpr _finline static T Abs(T Val){return (Val >= 0) ? Val : -Val;}

template<typename N, typename M> constexpr _finline static M NumToPerc(N Num, M MaxVal){return M(((Num)*100)/(MaxVal));}               // NOTE: Can overflow!
template<typename P, typename M> constexpr _finline static M PercToNum(P Per, M MaxVal){return M(((Per)*(MaxVal))/100);}               // NOTE: Can overflow!

template<typename N> constexpr _finline static N AlignFrwd(N Value, N Alignment){return ((Value/Alignment)+(bool(Value%Alignment)))*Alignment;}    // NOTE: Slow but works with any Alignment value
template<typename N> constexpr _finline static N AlignBkwd(N Value, N Alignment){return (Value/Alignment)*Alignment;}                              // NOTE: Slow but works with any Alignment value

// 2,4,8,16,...
//template<typename N> constexpr _finline static bool IsPowerOf2(N Value){return Value && !(Value & (Value - 1));}
template<typename T> constexpr _finline static bool IsPowOfTwo(T v){return !(v & (v - 1));}   // Will return TRUE for v=0 which may be undesirable (but actually consistent)

// TODO: Cast pointer types to size_t
template<typename N> constexpr _finline static N AlignFrwdP2(N Value, unsigned int Alignment){return (Value+((N)Alignment-1)) & ~((N)Alignment-1);}    // NOTE: Result is incorrect if Alignment is not power of 2
template<typename N> constexpr _finline static N AlignBkwdP2(N Value, unsigned int Alignment){return Value & ~((N)Alignment-1);}                       // NOTE: Result is incorrect if Alignment is not power of 2

template<typename T> constexpr _finline static bool IsRangesIntersect(T OffsA, T SizeA, T OffsB, T SizeB) {return (OffsA < (OffsB + SizeB)) && (OffsB < (OffsA + SizeA));}  
//------------------------------------------------------------------------------------------------------------

template<typename T> consteval size_t countof(T& a){return (sizeof(T) / sizeof(*a));}         // Not for array classes or pointers!  // 'a[0]' ?


template<typename T> constexpr _finline static int SizeOfP2Type(void)
{
 if constexpr (SameType<T,void>::V)return 0;
  else
   {
    static_assert(IsPowOfTwo(sizeof(T)), "Only Pow2 types allowed!");
    return sizeof(T);
   }
}
//------------------------------------------------------------------------------------------------------------
/*template<typename ...TA> struct find_overload
{
template<typename TObj, typename TR> using member_func_t = TR(TObj::*)(TA...);
template<typename TObj, typename TR> static constexpr auto get_address(member_func_t<TObj, TR> func) -> member_func_t<TObj, TR> { return func; }
};*/
template<typename... A, typename T, typename R> constexpr auto find_overload(R(T::*f)(A...)) { return f; }
template<typename... A, typename R> constexpr auto find_overload(R(*f)(A...)) { return f; }
//------------------------------------------------------------------------------------------------------------
constexpr _finline bool IsConstexprCtx(void) noexcept    // WARNING: Always 'true' in 'if constexpr' or if declared as 'consteval' here
{
 return __builtin_is_constant_evaluated();    //  MSVC,GCC,Clang
}
//------------------------------------------------------------------------------------------------------------
// Makes the pointer 'arbitrary', like it came from some malloc
// Helps the optimizer to 'forget' about where this pointer came from
// NOTE: May cause the ptr to lose relative addressing? (Arm32)
//auto _finline UnbindPtr(auto* ptr) // __builtin_launder (std::launder)  // UnbindVal replaces it?
//{
// volatile size_t tmp = (size_t)ptr;
// return (decltype(ptr))tmp;
//}
//---------------------------------------------------------------------------
// Removes unwanted metadata from a variable
// Needed to suppress assumptions about return values from some intrinsics (like  __builtin_clz )
// Should force compiler to use registers to store constants in code directly instead of putting them into RDATA
template<typename T> constexpr static T _finline UnbindVal(T value) noexcept     // LoadFromRegister  // Actually prevents values from being allocated into RDATA?
{
#if defined(__clang__) || defined(__GNUC__)
 if(!IsConstexprCtx())
  {
   asm volatile ("" : "+r"(value));  // asm("" : "+r"(value) : : );  // asm("" : "=r"(value) : "0"(value) : );
   return value;
  }
   else return value;     // Useless in constexpr
#else       // MSVC fallback
 volatile T reg = value;
 return reg;
#endif
}

//#include "Intrinsic.hpp"   // <<<< Should it be included here?
//---------------------------------------------------------------------------
// No way to return an array from a 'consteval' directly?
//
template<typename T, uint N> struct SDHldr
{
 T data[N];

// static_assert(sizeof(SDHldr<T,N>) == (sizeof(T)*N)); // incomplete type // Should work as array

 constexpr _finline operator T* () { return &this->data[0]; }
 constexpr _finline T& operator[] (const uint idx) {return this->data[idx];}              //lvalue
 constexpr _finline const T& operator[] (const uint idx) const {return this->data[idx];}  //rvalue
};
//------------------------------------------------------------------------------------------------------------
struct pchar
{
 union
  {
   achar* av;
   charb* bv;
   const achar* cav;
   const charb* cbv;
  } val;

_finline constexpr pchar(achar* v){this->val.av = v;}
_finline constexpr pchar(charb* v){this->val.bv = v;}
_finline constexpr pchar(const achar* v){this->val.cav = v;}
_finline constexpr pchar(const charb* v){this->val.cbv = v;}

_finline uint8 operator* () const {return (uint8)*this->val.cav;}    // *Ptr ?

_finline operator achar* (void) const {return this->val.av;}
_finline operator charb* (void) const {return this->val.bv;}
};
//------------------------------------------------------------------------------------------------------------
// The pointer proxy to use inside of a platform dependant structures (i.e. NTDLL)
// Accepts a pointer to type T and stores it as type H
// On x86-x32 if a system api returns a pointer and it is wrapped in SPTR then compiler actually returns it by a hidden pointer, passes it as a first argument!!! (Except MSVC)
// https://www.sco.com/developers/devspecs/abi386-4.pdf   // It is SystemV ABI for X86-X32!
// This kills ALL wrapper classes!    // As usual, data management is at the bottom!   // Flexie note: Never do that for internal functions, only for explicitly marked with SystemV ABI
// So, it is useless!!!
template<typename T, typename H=uint> struct alignas(H) SPTR
{
 using R = typename RemoveConst<typename RemoveRef<T>::T>::T;   // Base type
 STASRT(SameType<H, uint>::V || SameType<H, uint32>::V || SameType<H, uint64>::V, "Unsupported pointer type!");
 H Value;
 _finline constexpr SPTR(void) = default;    //{this->Value = 0;}          // Avoid default constructors in POD (SPTR will replace many members in POD structures)!
 _finline constexpr SPTR(H v){this->Value = v;}
 _finline constexpr SPTR(T* v) requires (!SameType<T, const char>::V) {this->Value = (H)v;}
 _finline constexpr SPTR(int v){this->Value = (H)v;}                   // For '0' values
 _finline constexpr SPTR(pchar v){this->Value = (H)v.val.av;}
 //_finline SPTR(unsigned int v){this->Value = (H)v;}          // For '0' values
 _finline constexpr SPTR(long long v){this->Value = (H)v;}             // For '0' values
 //_finline SPTR(unsigned long long v){this->Value = (H)v;}    // For '0' values
// template<typename X, int N> _finline  SPTR(const X(&v)[N]){this->Value = (H)v;}   // for arrays
 _finline constexpr SPTR(const char* v){this->Value = (H)v;}           // For string pointers
 _finline constexpr SPTR(nullptr_t v){this->Value = (H)v;}             // For 'nullptr' values
 template<typename X> _finline constexpr SPTR(X&& v) requires (!SameType<R, pchar>::V) {this->Value = (H)((T*)v);}   // For some classes that cannon convert themselves implicitly
 template<typename X> _finline constexpr SPTR(X&& v) requires (SameType<R, pchar>::V) {this->Value = (H)((const achar*)v);}

 _finline void operator= (H val){this->Value = val;}
 _finline void operator= (T* val){this->Value = (H)val;}     // May truncate or extend the pointer
// _finline void operator= (SPTR<T,H> val){this->Value = val.Value;}   // -Wdeprecated-copy-with-user-provided-copy
 template<typename X> _finline operator X* (void) const {return (X*)this->Value;}
// _finline operator auto* (void) const requires (!SameType<T, void>::V) {return (T*)this->Value;}
// _finline operator T* (void) const {return (T*)this->Value;}        // Must be convertible to current pointer type
 _finline operator H (void) const {return this->Value;}             // Raw value

 _finline T* operator* () const {return (T*)this->Value;}   // Should be T&
 _finline T* operator-> () const {return (T*)this->Value;}

//constexpr _finline Self& operator+=(const auto& rhv) {; return *this;}
//constexpr _finline Self& operator-=(const auto& rhv) {; return *this;}
}; 
//template<typename T, typename H=uint> using SVAL = T;    // For the future, may be   // Should be separate from SPTR anyway
//template<typename T, typename H=uint> using SPTR = T*;   // Just a simple alias
//using SPTRN  = SPTR<uint>;
//using SPTR32 = SPTR<uint32>;
//using SPTR64 = SPTR<uint64>;
//------------------------------------------------------------------------------------------------------------
// Error returning:
// 0 is always a success code
// If a sint is the return type, it is a POSIX error code
//
//
//
//------------------------------------------------------------------------------------------------------------
// Reassignable reference
template<typename T> class CRRef    // https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper
{
 T* ptr;

public:
 CRRef(const T& v){this->ptr = (T*)&v;}
 CRRef(const CRRef& v){this->ptr = (T*)v.ptr;}
 CRRef& operator=(const T& v){this->ptr = (T*)&v;}
 CRRef& operator=(const CRRef& v){this->ptr = (T*)v.ptr;}

 constexpr operator T& () const noexcept { return *this->ptr; }
 constexpr T& get() const noexcept { return *this->ptr; }

 template<typename... Args> constexpr auto operator() (Args& ... args){return (*this->ptr)(args...);}   // Forward any 'operator()'

};
template<typename T> CRRef(T&) -> CRRef<T>;    // Deduction guides
//------------------------------------------------------------------------------------------------------------
template<size_t N> struct SStrLit   // String literal (Useful for templates where a string need to be passes as a nontype argument)
{
 achar value[N];        // Still no way to use it with a custom string class passed as a function argument    // Probably constexpr allocation and consteval counstructor would help somehow to get the size from the constructor?
                        // Looks like user defined string literals are introduced to "hide" this problem ("argument deduction not allowed in function prototype")
 constexpr SStrLit(const achar (&str)[N]) { for(int z=0;z < N;z++)value[z] = str[z]; }        // std::copy_n(str, N, value);   // constexpr cstring(char const* data)
 auto operator<=>(const SStrLit&) const = default;
 bool operator==(const SStrLit&) const  = default;
};
//template <std::size_t N> cstring(char const (&data)[N]) -> cstring<N>;
/*struct String {                // Thus class can be a function argument
  __attribute__((always_inline)) inline String(size_t size) {
     bytes= static_cast<char*>(alloca( size ));// alloca() memory gets allocated here
  }
  char* bytes;
}; */

/*
template <int N> struct ConstString
{
    char str[N];
    consteval ConstString(const char (&new_str)[N])
    {
     for(int idx=0;idx < N;idx++)str[idx] = new_str[idx];
    }
};
template <int N> ConstString(const char (&arr)[N]) -> ConstString<N>;

template <ConstString S> struct Axx {};
*/
//template <cstring file_name> struct event {
//    static constexpr char const* const file_name_ = file_name.data_;
//};
//------------------------------------------------------------------------------------------------------------
namespace SelfType    // NOTE: Stateful metaprogramming trick
{
 inline void GetSelfType() {}
 template <typename T> struct Reader { friend auto GetSelfType(Reader<T>); };
 template <typename T, typename U> struct Writer {friend auto GetSelfType(Reader<T>){return U{};} };
 template <typename T> using Read = RemovePtr<decltype(GetSelfType(Reader<T>{}))>::T;
}

#define DEFINE_SELF \
    struct _self_type_tag {}; \
    constexpr auto _self_type_helper() -> decltype(SelfType::Writer<_self_type_tag, decltype(this)>{}); \
    using self = SelfType::Read<_self_type_tag>;

// Example: struct A { DEFINE_SELF };     
//------------------------------------------------------------------------------------------------------------
// Clang`s codegen is inconsistent with GCC (starting from early versions)

#define STR_NULL NullStr()
#define STR_EMPTY EmptyStr()
// Kills that annoying optimization of 'char*' arrays that places them in global memory
#define SNC(v) UnbindPtr(v)

consteval _finline auto EmptyStr(void) {return "";}
// Helps to prevent local char* arrays from being placed in static memory (for null terminated arrays like args)
constexpr _finline auto NullStr(void) 
{
 volatile usize ptr = 0;     // Must not be computable to a constant
 return (const char*)ptr;
}
//------------------------------------------------------------------------------------------------------------
// Byte array wrapper - REQUIRED for BitCast because arrays can't be returned/assigned directly. 
// Example auto bytes = __builtin_bit_cast(SWrapArray<sizeof(T), uint8>, value);
template<usize N, typename T=uint8> struct SWrapArray 
{
 T data[N];
 constexpr T& operator[](usize i) { return data[i]; }
 constexpr const T& operator[](usize i) const { return data[i]; }
};
//------------------------------------------------------------------------------------------------------------
template<typename To, typename From> constexpr To BitCast(const From& src) noexcept 
{
 static_assert(sizeof(To) == sizeof(From), "Size mismatch");
 return __builtin_bit_cast(To, src);
}
//------------------------------------------------------------------------------------------------------------

// Generic version with auto return type based on string length (Slow monstrosity)
/*template<int N> consteval auto StrToCC(const achar (&str)[N]) noexcept 
{
// static_assert(N >= 2, "String must be at least 1 character + null terminator");
 constexpr int CharCount = N - 1;  // Exclude null terminator
 using RetType =    // Determine return type based on character count
     TSW_t<(CharCount == 8), uint64,  
     TSW_t<(CharCount == 4), uint32, 
     TSW_t<(CharCount == 2), uint16,    
     TSW_t<(CharCount == 1), uint8,  
     ETYPE>>>>;  // Error case for > 8 chars
 static_assert(sizeof(RetType) > 0, "The string can't map to the type");
 RetType result = 0;
 if constexpr (!NCFG::IsBigEnd) { for(int idx = 0; idx < CharCount; idx++) result |= (RetType)(uint8(str[idx])) << (idx << 3); }  // Little-endian: first char at LSB     // * 8
 else { for(int idx = 0; idx < CharCount; idx++) { result<<=8; result |= (RetType)(uint8(str[idx]));} } // Big-endian: first char at MSB  
 return result;
} */

/*consteval uint64 MakeCC(const achar* sv, usize len)
{
 uint64 result = 0;
 for(usize i = 0;i < 8; ++i)result |= uint64(uint8(sv[i])) << (i * 8);
 return result;
}
consteval uint64 operator""_cc(const achar* s, usize len){ return MakeCC(s, len); } // C++23 user-defined literal (optional, very convenient)  */

consteval uint32 cc4(const char (&s)[5])    // exactly 4 chars + '\0'  // NOTE: Compare the result with unswapped uint32 as it was read from memory
{
 if constexpr (!IsBigEndian)return uint32((uint8)s[0]) | (uint32((uint8)s[1]) << 8) | (uint32((uint8)s[2]) << 16) | (uint32((uint8)s[3]) << 24);
   else return uint32((uint8)s[3]) | (uint32((uint8)s[2]) << 8) | (uint32((uint8)s[1]) << 16) | (uint32((uint8)s[0]) << 24);
}
//------------------------------------------------------------------------------------------------------------
// This handles all cases correctly:
// NumBits == 0:        returns 0 (the & 0xFFFF... zeros it)
// NumBits == MaxBits:  shift by 0, returns T(-1)
// 0 < NumBits < Max:   returns proper mask
template<typename T> static _finline constexpr T MakeBitMask(uint32 NumBits)   // TODO: Optimize
{
 SCVR uint32 MaxBits = sizeof(T) * 8;
 return (T(-1) >> ((MaxBits - NumBits) & (MaxBits - 1))) & -T(NumBits != 0);   // bool?
} 
//------------------------------------------------------------------------------------------------------------
