
// Use this as Main C++ file for an app

#if !__has_include ("Framework.hpp")  // May be already included by a compilation script
#include "Framework.hpp"
#endif

//using namespace NCMN;
//using namespace NFWK::NGenericTypes;
using namespace NFWK;   // TODO: Configurable


// NOTE: CD is set to $PRJROOT which may have all of its sources under SRC
// NOTE: Handling of '../' with directory junctions is not reliable across all platforms (May be resolved to an actual parent directory, not where it is linked to. - Clang's implementation decision)
// No several projects in a folder - Dumb '__has_include' implementation is not affected by macro substitution! 
     
#if __has_include ("AppMain.hpp")
#define _APPENTRYPT
#include "AppMain.hpp"     
#else
#pragma message(">>> No AppMain.hpp is found - expecting ModuleMain as entry point!")  // ModuleMain must be globally visible 
#endif

#include "Platform/EntryPoints.hpp"     // Specify 'AppEntryPoint' to the linker
