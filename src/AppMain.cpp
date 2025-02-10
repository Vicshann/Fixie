
// Use this as Main C++ file for an app

#include "Framework.hpp"

//using namespace NCMN;
//using namespace NFWK::NGenericTypes;
using namespace NFWK;   // TODO: Configurable

#if __has_include ("AppMain.hpp")
#define _APPENTRYPT
#include "AppMain.hpp"         // The app implementation (When fs link is to this file)
/*#elif __has_include ("src/AppMain.hpp")
#define _APPENTRYPT
#include "src/AppMain.hpp"      // The app implementation (When fs link is to /COMMON/FRAMEWORK)
#elif __has_include ("../AppMain.hpp")
#define _APPENTRYPT
#include "../AppMain.hpp"      // The app implementation (When fs link is to /COMMON/FRAMEWORK)
#elif __has_include ("../src/AppMain.hpp")
#define _APPENTRYPT
#include "../src/AppMain.hpp"      // The app implementation (When fs link is to /COMMON/FRAMEWORK)
#elif __has_include ("../../AppMain.hpp")
#define _APPENTRYPT
#include "../../AppMain.hpp"   // The app implementation (When fs link is to /COMMON)
#elif __has_include ("../../src/AppMain.hpp")
#define _APPENTRYPT
#include "../../src/AppMain.hpp"   // The app implementation (When fs link is to /COMMON)
#elif __has_include ("../../../AppMain.hpp")
#define _APPENTRYPT
#include "../../../AppMain.hpp"   // The app implementation (When fs link is to /COMMON)
#elif __has_include ("../../../src/AppMain.hpp")
#define _APPENTRYPT
#include "../../../src/AppMain.hpp"   // The app implementation (When fs link is to /COMMON)
#elif __has_include ("../../../../AppMain.hpp")
#define _APPENTRYPT
#include "../../../../AppMain.hpp"   // The app implementation (When fs link is to /COMMON)
#elif __has_include ("../../../../src/AppMain.hpp")
#define _APPENTRYPT
#include "../../../../src/AppMain.hpp"   // The app implementation (When fs link is to /COMMON)    */  // The Compilation Script will respect links in the path and will add the correct path to Includes
#else
#pragma message(">>> No AppMain.hpp is found - expecting ModuleMain as entry point!")
#endif
#include "Platforms/EntryPoints.hpp"     // Specify 'AppEntryPoint' to the linker
