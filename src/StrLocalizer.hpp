
/*
https://stackoverflow.com/questions/71156078/will-friend-injections-be-ill-formed

 While there is no persistent constexpr vector in C++ to store hashes and indexes there is no way to exclude duplicate messages
 And it seems that there is no way to gather all string descriptors in one place at compile time so they could be dumped in a file at run time

InstantiateFunction:
  "NFWK::NCTM::CtCtr<NFWK::SLOC>::Next<0, NFWK::NCTM::CtCtr<NFWK::SLOC>::(lambda at COMMON/FRAMEWORK/Platforms/CompileTime.hpp:439:37){}>"
  "NFWK::NCTM::CtCtr<NFWK::SLOC>::Next<0, NFWK::NCTM::CtCtr<NFWK::SLOC>::(lambda at COMMON/FRAMEWORK/Platforms/CompileTime.hpp:439:37){}>"

 Repeats instantiation in a recursive loop for all existing values of a counter +1 every time when 'Next' is used!

 GCC/Clang automatically create symbols for custom sections:
extern "C" {
    extern SLOC::SLTxt __start_locidx;  // Note: not a pointer!
    extern SLOC::SLTxt __stop_locidx;
}

void DumpStrings() {
    auto* begin = &__start_locidx;
    auto* end = &__stop_locidx;
    ...
}

 TODO:
  Try to store SLTxt together with the strings in a separate section (needs string size(Can pass it as a template argument in a string literal class?))
  Do not use indexes, 'auto U = []{}' should be enough for uniquiness
*/

struct SLOC         // template<char... ns="Framework">      // []{return "Hello world!";}
{
 using CtStrIdx = NCTM::CtCtr<SLOC>;   // NOTE: Clang 15 will crash when compiling 'SLOC::CtStrIdx::Next()'  // Global, for now (no separate namespaces for localization if ever needed)

struct SLTxt   // String literal (Useful for templates)       template<sint num>
{
 const achar* str;
 uint32 hash;
 uint16 len;
 uint16 idx;      // Indexes help to avoid looking for messages by their hash every time (But must be uniquely incremented at compile time)

// This produces and unique object per call site.
 template<sint N, typename T=SLOC::CtStrIdx, auto U = []{}> consteval SLTxt(const achar (&strng)[N], int num = T::template Next<0,1,U>())   // Increment the counter at each unique construction site  // NOTE: SLTxt will be unique for each string  // NOTE: Using same unique lambda placeholder for 'NEXT'
  {
   this->str  = strng;
   this->len  = N;
   this->idx  = num;
   this->hash = NCRYPT::CRC32(strng);
  }
};

//void Replace(const SLOC* loc){*(const SLOC**)&this = loc;}    // To replace a default 'static inline SLOC& LS' if needed (Watch out for an optimizer! Compiler does not expect references to be replaced.)
// Default namespace is "Framework"?  // Either searched in a separate file with that name or in a section // Namespace must belong to a localizer instance, no way to attach it to a message
const achar* operator() (const SLTxt&& str)
{
 //SStr vv{str};
 //static const SLTxt stored __attribute__((section(".locidx"), used, aligned(4))) = str;
 DBGMSG("%u,%u,%08X: %s",str.idx,str.len,str.hash,str.str);
 return str.str;
}

};

static inline SLOC LocNone;   // A polaceholder that just passes messages as is

using SLR = CRRef<SLOC>;    // NOTE: Ref wrapper makes code generation more complicated with all that forwarding   // 'static inline SLR LS = LocNone;'   // By storing it as ref its operators can be used directly   // No ordinary ref is suitable - need a way to overwrite it later  // May be 'static inline volatile SLOC& LS' ???
//static inline SLR LS = LocNone;
#define LSTR (LocNone)  //(*LS)   //.Str<NCRYPT::CRC32(s), SRC_LINE, decltype(*LS)::GICtr::next()>
//#define LOCS(str) Localize(makeGlobal<SLTxt>(str))
