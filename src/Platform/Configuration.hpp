
#pragma once

// ??? Should duplicate defines as NCFG values?  // It is convenient to use different CFG defines with the same config file
// NOTE: Put here all available configs 
//#define FWK_RCE_PROTECT
// FWK_OLD_UBOOT
//#define FCFG_FORCE_DBGMSG
/* 
Modules:
  FWK_MOD_ALL
  FWK_MOD_HOOK
  FWK_MOD_PARSE
  FWK_MOD_CRYPTO
  FWK_MOD_PROTECT
  FWK_MOD_LOCALIZE
Not all platforms:
  FWK_MOD_WINDOWING
  FWK_MOD_GRAPHICS
  FWK_MOD_AUDIO
  FWK_MOD_INPUT
Specific:
  FWK_MOD_EXTRA
*/

/*  CONFIGS:
FWK_CFG_ENCSYSC   // Encrypt linux syscalls
*/

// inline (C++17+) - Allows the definition in a header without ODR violations. Each translation unit gets the same instance. 
// 'static' at global scope gives internal linkage (each TU gets its own copy)
struct NCFGType_ 
{      
 long MemPageSize     = 0;      // 0 - Platform default if not redefined for an app
 long MemGranSize     = 0;
 long DefStkSize      = (sizeof(void*) > 4)?0x200000:0x100000;  // 2Mb : 1Mb    // 0x100000 1Mb ?    // Or read those defaults from the Exe header?
 long DefTlsSize      = 0;
 long DefConState     = 0;          // Normal mode;
 bool InitCon         = false;      // Init console (console app) at early initialization stage
 bool InitSignals     = true;       // Init signal handling at early initialization stage
 bool InitSyscalls    = true;       // Initialize syscalls to be ready at compile time, if possible
 bool VectorizeMemOps = false;      // Makes memcopy slower(Misalignment?) // TODO: Investigate
} static constexpr const NCFG {   
#if __has_include("AppCfg.hpp")
#include "AppCfg.hpp"
#else
#pragma message(">>> No AppCfg.hpp is found - using default config!")
#endif   
};

// Example: AppCfg.hpp (only in projects that need overrides)
// #define FWK_MOD_CRYPTO 6
//  .flag2 = 999,
//  .flag3 = 777,
// #define FWK_MOD_U 9
