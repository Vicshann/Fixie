
#pragma once

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
namespace NCFG
{
static constexpr long MemPageSize = 0;      // 0 - Platform default if not redefined for an app
static constexpr long MemGranSize = 0;
static constexpr long DefStkSize  = (sizeof(void*) > 4)?0x20000:0x10000;  // 128k : 64K    // 0x100000 1Mb ?    // Or read those defaults from the Exe header?
static constexpr long DefTlsSize  = 0;
static constexpr bool InitCon = false;      // Init console (console app) at early initialization stage
static constexpr bool IsBigEnd = false;     // Since the Framework is for x86 and ARM its base operation mode is LittleEndian  // __BIG_ENDIAN__ // __BYTE_ORDER == __BIG_ENDIAN // __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static constexpr bool InitSyscalls = true;  // Initialize syscalls to be ready at compile time, if possible
static constexpr bool VectorizeMemOps = false; // Makes memcopy slower(Misalignment?) // TODO: Investigate
}
