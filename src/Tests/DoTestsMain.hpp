
#pragma once

#define FWKSRCDIR "FWKSRCDIR"

#define TST_LINE __LINE__
// Tests result of compile-time evaluation
#define CT_TEST(oper,expr,...)  SCVR auto _HJOIN(ctTst_, TST_LINE) = oper;  \
                                static_assert(_HJOIN(ctTst_, TST_LINE) expr, "Test: '" _HSTR(oper) "' - failed" __VA_OPT__( ": " ) __VA_ARGS__ ); 

// Tests result of runtime evaluation     // TODO: Check the var type to print expected non-integers
#define RT_TEST_(oper,expr,...)  auto _HJOIN(ctTst__, TST_LINE) = oper;  \
                                if(!((_HJOIN(ctTst__, TST_LINE)) expr)){LOGERR("Failed: '%s %s' at %u, got %i" __VA_OPT__(": %s"),_HSTR(oper),_HSTR(expr),TST_LINE,ssize_t(_HJOIN(ctTst__, TST_LINE)) __VA_OPT__(, __VA_ARGS__)); return -(TST_LINE);} 

#define RT_TEST(oper,expr,...)  { RT_TEST_(oper,expr,__VA_ARGS__) }

// Does both tests and check consistency of not exact match(<;>) tests
#define CTRT_TEST(oper,expr,...) { CT_TEST(oper,expr,__VA_ARGS__) \
                                   RT_TEST_(oper,expr,__VA_ARGS__) \
                                   if((_HJOIN(ctTst_, TST_LINE)) != (_HJOIN(ctTst__, TST_LINE))){LOGERR("Mismatch: '%s' at %u, got %i:%i",_HSTR(oper),TST_LINE,ssize_t(_HJOIN(ctTst_, TST_LINE)),ssize_t(_HJOIN(ctTst__, TST_LINE))); return -(TST_LINE);} }


#define DO_TEST(name)   if(int res = name::DoTests();res < 0){LOGERR("Failed: %s at %u",_HSTR(name),TST_LINE);return -(TST_LINE);}
#define DO_BENK(name)   if(int res = name::DoBenchmarks();res < 0){LOGERR("Failed: %s at %u",_HSTR(name),TST_LINE);return -(TST_LINE);}

struct NTST
{
 static inline achar TmpPath[NPTM::PATH_MAX];
 static inline achar FwkRoot[NPTM::PATH_MAX];      // FWKSRCDIR=C:\_SYNC\COMMONSRC\FRAMEWORK

#include "TestBitOps.hpp"
#include "TestUnicode.hpp"
#include "TestStreams.hpp"
#include "TestBitStream.hpp"
#include "TestBitArray.hpp"
#include "TestRLE.hpp"
//------------------------------------------------------------------------------------
static int DoTests(void)         // TODO: Test/Benchmark optional
{
 if(NPTM::GetEnvVar("TEMP", TmpPath, sizeof(TmpPath)) < 0){LOGERR("Missing EVars!"); return -998;} 
 if(NPTM::GetEnvVar(FWKSRCDIR, FwkRoot, sizeof(FwkRoot)) < 0){LOGERR("Missing EVars!"); return -999;} 

 LOGMSG("Testing...\n");

 DO_TEST(Test_BitOps) 
 DO_TEST(Test_Unicode) 
 DO_TEST(Test_Streams) 
 DO_TEST(Test_BitStream) 
 //DO_TEST(Test_BitArray) 
 DO_TEST(Test_RLE) 

 LOGMSG("All passed!\n");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void)  
{
 if(NPTM::GetEnvVar(FWKSRCDIR, FwkRoot, sizeof(FwkRoot)) < 0){LOGERR("Missing EVars!"); return -999;} 
 LOGMSG("Profiling...\n");

 DO_BENK(Test_BitOps) 
 DO_BENK(Test_Unicode) 
 DO_BENK(Test_Streams) 
 DO_BENK(Test_BitStream) 
 DO_BENK(Test_BitArray) 
 DO_BENK(Test_RLE)

 LOGMSG("All done!\n");
 return 0;
}
//------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------
