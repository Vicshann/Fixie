
#include "../FRAMEWORK/src/Parsers/ExampleParseCpp.hpp"
//#include "SDL3/SDL.h"
// UI: https://github.com/nakst/gf  // D:\_DOWNLOADS_\_SRC\_GUI\luigi

template <typename T, size_t N> struct XCEStr {
    template <typename V>
    consteval XCEStr(const T(&str)[N]) {
        // Constructor body
    }
};
template <typename T, size_t N> XCEStr(const T(&)[N]) -> XCEStr<T, N>;
//------------------------------------------------------------------------------------------------------------
EXPORT_SYM char* _ccall realpath(const char* path, char* resolved_path)   
{
 DBGMSG("Not resolving links on path: %s",path);
 if(!resolved_path){DBGMSG("cannot alloc memory - no access to malloc!"); return nullptr;}
 NPTM::CanonicalizePath(resolved_path, path, NPTM::PATH_MAX);
 DBGMSG("Canonicalized path: %s",resolved_path);
 return resolved_path;
}
//------------------------------------------------------------------------------------------------------------
class CAppMain: public CApplication
{
static inline SLOC* LS = &LocNone; 

               //  static-pie
/*
static int MyProc(const SStrLit&& str)
{
 LOGERR("%s", &str.value);
 return sizeof(str.value);
}  */

/*static_assert(NFWK::NCTM::unique_id() == 0);
static_assert(NFWK::NCTM::unique_id() == 1);
static_assert(NFWK::NCTM::unique_id() == 2);
static_assert(NFWK::NCTM::unique_id() == 3);
static_assert(NFWK::NCTM::unique_id() == 4);
static_assert(NFWK::NCTM::unique_id() == 5);
static_assert(NFWK::NCTM::unique_id() == 6);
static_assert(NFWK::NCTM::unique_id() == 7);
        */
static ssize_t MyThread(NPTM::NTHD::SThCtx* ctx)
{
 DBGMSG("Thread Exiting!");
 //NPTM::SAPI::NtClose((NPTM::NT::HANDLE)ctx->ThreadHndl); 
 DBGMSG("Thread Exiting 1!");
 //NPTM::SAPI::NtTerminateThread(NPTM::NT::NtCurrentThread, 78);
 DBGMSG("Thread Exiting 2!");
 return 56;
}

public:
_ninline sint Initialize(vptr ArgA=0, vptr ArgB=0, vptr ArgC=0) 
{   
 LOGERR(LSTR("Hello localize init 1"));
 return 1;
}
//------------------------------------------------------------------------------------------------------------
/*
  stop the generation of global jump tables


https://stackoverflow.com/questions/50126786/how-to-prevent-clang-llvm-compile-local-variables-to-global

 const char*  Test1 =  "TestA";
 const char*  Test2 =  "TestB";
 const auto   Test3 = &"TestC";
 const auto   Test4 = {"TestD"};
 const auto   Test5 = {"TestE1","TestE2","TestE3"};
 const char*  Test6[] = {"TestF"};

.text:0000000000001462                 lea     rax, aTesta     ; "TestA"
.text:000000000000146D                 lea     rax, aTestb     ; "TestB"
.text:0000000000001478                 lea     rax, aTestc     ; "TestC"
.text:0000000000001483                 lea     rax, aTestd     ; "TestD"

.text:00000000000014A8                 mov     rax, cs:off_E250 ; "TestE1"
.text:00000000000014B6                 mov     rax, cs:off_E258 ; "TestE2"
.text:00000000000014C4                 mov     rax, cs:off_E260 ; "TestE3"

.text:00000000000014F2                 mov     rax, cs:off_E268 ; "TestF"
*/

_ninline sint Execute(void) 
{


 {
 uint8 v6[200];// = {

  v6[0] = 111;
  v6[1] = 129;
  v6[2] = 66;
  v6[3] = 62;
  v6[4] = 67;
  v6[5] = 60;
  v6[6] = 61;
  v6[7] = 62;
  v6[8] = 131;
  v6[9] = 111;
  v6[10] = 129;
  v6[11] = 127;
  v6[12] = 124;
  v6[13] = 131;
  v6[14] = 113;
  v6[15] = 114;
  v6[16] = 110;
  v6[17] = 112;
  v6[18] = 125;
  v6[19] = 113;
  v6[20] = 114;
  v6[21] = 112;
  v6[22] = 113;
  v6[23] = 110;
  v6[24] = 125;
  v6[25] = 114;
  v6[26] = 112;
  v6[27] = 113;
  v6[28] = 114;
  v6[29] = 125;
  v6[30] = 110;
  v6[31] = 113;
  v6[32] = 112;
  v6[33] = 120;
  v6[34] = 108;
  v6[35] = 130;
  v6[36] = 109;
  v6[37] = 120;
  v6[38] = 108;
  v6[39] = 128;
  v6[40] = 120;
  v6[41] = 130;
  v6[42] = 109;
  v6[43] = 108;
  v6[44] = 128;
  v6[45] = 120;
  v6[46] = 130;
  v6[47] = 109;
  v6[48] = 119;
  v6[49] = 120;
  v6[50] = 119;
  v6[51] = 130;
  v6[52] = 120;
  v6[53] = 109;
  v6[54] = 128;
  v6[55] = 119;
  v6[56] = 117;
  v6[57] = 125;
  v6[58] = 112;
  v6[59] = 116;
  v6[60] = 110;
  v6[61] = 117;
  v6[62] = 125;
  v6[63] = 112;
  v6[64] = 117;
  v6[65] = 110;
  v6[66] = 125;
  v6[67] = 116;
  v6[68] = 112;
  v6[69] = 129;
  v6[70] = 111;
  v6[71] = 121;
  v6[72] = 115;
  v6[73] = 127;
  v6[74] = 124;
  v6[75] = 111;
  v6[76] = 115;
  v6[77] = 129;
  v6[78] = 124;
  v6[79] = 129;
  v6[80] = 67;
  v6[81] = 61;
  v6[82] = 62;
  v6[83] = 66;
  v6[84] = 60;
  v6[85] = 67;
  v6[86] = 61;
  v6[87] = 58;
  v6[88] = 66;
  v6[89] = 62;
  v6[90] = 58;
  v6[91] = 60;
  v6[92] = 67;
  v6[93] = 61;
  v6[94] = 62;
  v6[95] = 10;
  v6[96] = 102;
  v6[97] = 109;
  v6[98] = 121;
  v6[99] = 120;
  v6[100] = 112;
  v6[101] = 115;
  v6[102] = 113;
  v6[103] = 102;
  v6[104] = 111;
  v6[105] = 120;
  v6[106] = 56;
  v6[107] = 130;
  v6[108] = 119;
  v6[109] = 118;
  v6[110] = 10;
  v6[111] = 10;
  v6[112] = 102;
  v6[113] = 109;
  v6[114] = 121;
  v6[115] = 120;
  v6[116] = 112;
  v6[117] = 115;
  v6[118] = 113;
  v6[119] = 102;
  v6[120] = 111;
  v6[121] = 120;
  v6[122] = 56;
  v6[123] = 122;
  v6[124] = 120;
  v6[125] = 113;
  v6[126] = 10;
  v6[127] = 10;
  v6[128] = 102;
  v6[129] = 109;
  v6[130] = 121;
  v6[131] = 120;
  v6[132] = 112;
  v6[133] = 115;
  v6[134] = 113;
  v6[135] = 102;
  v6[136] = 115;
  v6[137] = 126;
  v6[138] = 56;
  v6[139] = 130;
  v6[140] = 119;
  v6[141] = 118;
  v6[142] = 10;
  v6[143] = 10;
  v6[144] = 102;
  v6[145] = 109;
  v6[146] = 121;
  v6[147] = 120;
  v6[148] = 112;
  v6[149] = 115;
  v6[150] = 113;
  v6[151] = 102;
  v6[152] = 115;
  v6[153] = 126;
  v6[154] = 56;
  v6[155] = 122;
  v6[156] = 120;
  v6[157] = 113;
  v6[158] = 10;
  v6[159] = 10;
  v6[160] = 102;
  v6[161] = 109;
  v6[162] = 121;
  v6[163] = 120;
  v6[164] = 112;
  v6[165] = 115;
  v6[166] = 113;
  v6[167] = 102;
  v6[168] = 111;
  v6[169] = 125;
  v6[170] = 56;
  v6[171] = 130;
  v6[172] = 119;
  v6[173] = 118;
  v6[174] = 10;
  v6[175] = 10;
  v6[176] = 102;
  v6[177] = 109;
  v6[178] = 121;
  v6[179] = 120;
  v6[180] = 112;
  v6[181] = 115;
  v6[182] = 113;
  v6[183] = 102;
  v6[184] = 111;
  v6[185] = 125;
  v6[186] = 56;
  v6[187] = 122;
  v6[188] = 120;
  v6[189] = 113;
  v6[190] = 10;


  v6[190] = 0;




   int v3 = 0;
   do
    v6[v3++] -= 10;
  while ( v3 < 0xBF );




 //0};
 // for(int x=0;xxx[x];x++)xxx[x] -= 5;//8;//0x0C;
 v6[1] = 6;
 }


// Tokenizer test
  {
   CParseCpp pex;
       LOGMSG("Number: %7u test",2342);
   int zx   = 00067;
   int yy   = 0xADF'CE;
   float rr = 34.45e+00'56;
   float rx = 0x1.4p03;
   int bb   = 0b1001'11001;       // invalid digit '2' in binary constant  // invalid digit 'a' in binary constant // invalid suffix 'g001' on integer constant   //  0b100111!001; - error : expected ';' at end of declaration
   char* vb = " " "  ";
   char* rs = R"(help " me)";
   //float ty = 0.345E +5;
      
//int sdf=8; doll8:help: int yu = 8;              // cond?ns::val:0;
//goto help;
   pex.ParseFile("D:/TEST/TestTokenizer.txt");
   return 0;
  }

 /*
 NPTM::NTHD::SHDesc dsk{1};
 size_t ValB = dsk.TrHd;
 size_t ValC = dsk.PrHd;

 //static __thread int zx0 = 1;
 char*  Test1 =  "TestA";
 const char*  Test2 =  "TestB";
 const auto   Test3 = &"TestC";
 const char*   Test4 = {"TestD"};
 const char*  Test5[] = {"TestE1","TestE2","TestE3", Test1};
 const char*  Test6[] = {"TestF"};
 LOGMSG("Hello World", Test1, &Test2, Test3, Test4, Test5, Test6);
 */
 /*   int64 max = 0x8000000000000000;
    int64 *end = beg + len;
    while (beg < end) {
        if (*beg > max) {
            max = *beg;
        }
        beg++;
    }   */


 achar buf[512];

 achar path[] = {".\\GLOBAL??\\C:\\Hello\\.\\World\\..\\..\\..\\Finish\\"};
 achar dstbuf[512];
 int res = NPTM::NAPI::getcwd(buf, sizeof(buf));
 LOGMSG("getcwd %i: %s",res,buf);
 //NPTM::NAPI::chdir("/tmp");
 //  res = NPTM::NAPI::getcwd(buf, sizeof(buf));
 //LOGMSG("getcwd %i: %s",res,buf);
            //  zx0++;
 sint llen = NPTM::CanonicalizePath(dstbuf, path, sizeof(dstbuf));
  LOGMSG("CanonicalizePath %i: %s",llen,dstbuf);
 //return 0;
 //uint rrr = NPTM::NormalizePathNt(path,path); 
  //NPTM::NAPI::sleep(5,0);
  //LOGMSG("Done waiting 5s");                                                                                                 // (const achar*[]){"AAA=BBB",nullptr}     
 //static_assert(sizeof(size_t) == sizeof(unsigned long long));
 // const achar* args[] ={"-Hello","World","123",nullptr};
 // LOGMSG("args %p: %p %p",args,args[0],args[1]);                              (const achar*[]){"-Hello","World","123",TERMARGLST}                                                                                  // CfgSetTgtCurDir
// ssize_t res2 = NPTM::NAPI::spawn("./cdisp", (const achar*[]){"-Hello","World","123",TERMARGLST}, nullptr, (size_t[]){NPTM::NTHD::CfgPsCurDir, (size_t)"/tmp", TERMFLGLST});
 ssize_t res2 =0;// NPTM::NAPI::spawn(IsSysWindows?"C:/Windows/System32/calc.exe":"/usr/bin/ark", nullptr, nullptr, (size_t[]){NPTM::NTHD::CfgPsCurDir, (size_t)"/tmp", TERMFLGLST});  // NPTM::NTHD::CfgSetTgtCurDir, NPTM::NTHD::CfgSetTgtCurDir,
 //int res2 = NPTM::NAPI::spawn("/bin/ls", (const achar*[]){"-l",nullptr}, nullptr, (size_t[]){NPTM::NTHD::CfgPsCurDir, (size_t)"/tmp", 0});  
 LOGMSG("spawn result: %016llX",res2);
 sint status = 0;
 sint res3 = NPTM::NAPI::spawn_wait(res2, 10000, &status);
 LOGMSG("wait result: %i, %i",res3, status);
 // res3 = NPTM::NAPI::spawn_wait(res2, 10000, &status);
 //LOGMSG("second wait result: %i, %i",res3, status);
       //InitFilePathBuf(const achar* Path, uint plen, EPathType ptype, wchar* buf_path)
 
 return 0;
// ByteJam test
  {                    // TODO: Profile memcpy
   //NSTM::CStrmFile fs;
   //fs.Open("D:/TMP/savetst.jam");
   //CByteJam<0x100000,0,0>::DoTest("D:/TMP/WordsRandom.txt",&fs); 
   return 0;
  }

/*
  {
   CArr<uint8>  file;
   file.FromFile("C:\\WORKFOLDER\\_PAYED_\\LiliPop\\8\\1\\Seg_B1000.bin");
   uint8* ptr = file.Data();
   uint8* end = ptr + file.Size();
   for(;ptr < end;ptr++)
    {
     uint8 b = *ptr;
     //b = ~b;
     b = b - 0x55; 
     if(b & 0x80)b+=128;
     *ptr = b;
    }
   file.IntoFile("C:\\WORKFOLDER\\_PAYED_\\LiliPop\\8\\1\\Seg_B1000.dec.bin");
  }
  */
/* {
  NPTM::SMemRange Range;
  uint8 buf[8192];
  uint8 tmp[2048];

  Range.FPathLen = sizeof(buf);
  Range.FPath    = (achar*)&buf;
  NPTM::SMemMap* MappedRanges = (NPTM::SMemMap*)&buf; 
  int res = NPTM::NPFS::FindMappedRangeByAddr(0, 0x7f95d59042, &Range);

  memset(MappedRanges,0,sizeof(buf));
  MappedRanges->TmpBufLen  = sizeof(tmp);
  MappedRanges->TmpBufOffs = 0;
  MappedRanges->TmpBufPtr  = tmp;
  MappedRanges->RangesCnt  = 0;
  MappedRanges->NextAddr   = 0;
  res = NPTM::NPFS::FindMappedRangesByPath(0, 0, "/aarch64-linux-gnu/libnss_files-2.28.so", MappedRanges, sizeof(buf));

  memset(MappedRanges,0,sizeof(buf));
  MappedRanges->TmpBufLen  = sizeof(tmp);
  MappedRanges->TmpBufOffs = 0;
  MappedRanges->TmpBufPtr  = tmp;
  MappedRanges->RangesCnt  = 0;
  MappedRanges->NextAddr   = 0;
  res = NPTM::NPFS::ReadMappedRanges(0, 0, -1, MappedRanges, sizeof(buf));  
  MappedRanges = 0;
 }  */
  /*
  {
   NALC::TestStrategyBlk< NALC::SBASExp<32, 4096, 0, 0> >(0);     // <26, 4096, 30, 0> >(0);   156, 469
   volatile size_t iimm = 10;   
  }
   */
  //{
  // NALC::SBlkAlloc<4096, NALC::afGrowUni, uint32> Stream; 

  //}


  {
   //NALC::CGenAlloc<4096,0> alc;         // NALC::afSinglePtr
    /* bool t1 = alc.IsElmExist(56);
   volatile vptr xx = alc.GetBlock(5);
     bool t2 = alc.IsElmExist(67);
      xx = alc.GetBlock(8);
        alc.DeleteBlk(5);
        xx = alc.GetBlock(43);
    vptr yy = alc.FindBlock(nullptr);
    auto dd = alc.GetBlkData(8, 4);
    alc.SetBlkTag(8, 9); 
    uint rr = alc.GetBlkTag(8);  */
   /* uint TotalElm1 = 0;
    uint TotalElm2 = 0;
    uint TotalElm3 = 0;
    for (auto it = alc.begin(), end = alc.end(); it != end; ++it) 
     {
      *it = 6;
      TotalElm1++;
     }
    for (auto it = alc.begin(), end = alc.end(); it ; ++it) 
     {
      *it = 6;
      TotalElm2++;
     }
    for (auto& el : alc) 
     {
      el = 6;
      TotalElm3++;
     }
   //int v = AlignToP2Dn(5);
   //v++;
   return 11;   */
  }
  


  /*{
   sint hSrcFile = NPTM::NAPI::open("D:/TMP/Test1.txt",NPTM::PX::O_RDWR,0);
   if(hSrcFile < 0)return -1;

   uint8* DmpBuf = (uint8*)NPTM::NAPI::mmap(nullptr, 65536, NPTM::PX::PROT_READ|NPTM::PX::PROT_WRITE, NPTM::PX::MAP_SHARED, -1, 0); 
   DmpBuf[8] = 1;
   NPTM::NAPI::munmap(DmpBuf, 0);
  }*/


  /*{
   NPTM::NT::PEB* pe =  NPTM::NT::NtCurrentPeb(); 
   NPTM::NTX::NativeCreateProcess("C:/WORKFOLDER/_PUBLIC_/FrameworkTestApp/BUILD/FrmwTest/Debug64/FrmwTest.exe", nullptr, nullptr, true, nullptr, nullptr);
  }*/

  {
   sint id = NPTM::NAPI::thread(&MyThread, nullptr, 0, nullptr);
   DBGMSG("Thread created: %i",id);
   NPTM::NTHD::SThCtx* th = NPTM::GetThreadByID(id);
   /*for(uint x=0;th &&( x < 256);x++)
    {
     //if(!th)break;
     DBGMSG("Loop %u: %i",x,th->ThreadID);
    } */
   int res = NPTM::NAPI::thread_wait(id, -1, nullptr);
   DBGMSG("Wait result: %i",res);
   sint stat = NPTM::NAPI::thread_status(id);
   DBGMSG("Thread status: %i",stat);
  }


 NDT::DT dt = {};
 PX::timeval  tv = {};
 PX::timezone tz = {};

 NPTM::NAPI::gettimeofday(&tv, &tz);
 tv.sec += tz.utcoffs;
 NDT::UnixTimeToDateTime(&tv, &dt);

 LOGERR(LSTR("Hello local 1"));
 LOGERR(LSTR("Hello local 22"));
 LOGERR(LSTR("Hello local 333"));
 LOGERR(LSTR("Hello local 332"));
 LOGERR(LSTR("Hello local 343"));
 LOGERR(LSTR("Hello local 333"));
 LOGERR(LSTR("Hello local 373"));
 LOGERR(LSTR("Hello local 1335"));
  {
    CMiniIni ini;
    ini.Load("D:/TEST/testout1.ini");
    ini.Save("D:/TEST/testout2.ini"); 
  }   

  {
   CMiniIni ini;
   ini.Format |= CMiniIni::efSecBeforeNL|CMiniIni::efNameValSpacing|CMiniIni::efSecSpacedNames  |CMiniIni::efCmntSpacing|CMiniIni::efCmntIndenting;
  // CMiniIni::SValRec val = ini.AddSection("Section 1").SetValue("Val1","0x55");              

  // bool res_bool = val.AsBool();
  // sint res_sint = val.AsInt();
   ini.GetSection("Section 1").GetSection("Section 2").SetValue("Val2","0x66");
   ini.GetSection("Section 1").GetSection("Section 2").SetValue("Multiline","Hello\r\nTo\r\nAll\r\nMy\r\nFriends");   

  // ini.GetValue("Section 1", "Name 1", "DefValue 1", &ValLen); 

  /* uint ValLen = 0;
   ini.GetValue("Section 1", "Name 1", "DefValue 1", &ValLen);  
   ini.GetValue("Section 1", "Name 2", "DefValue 2", &ValLen); 
   ini.GetValue("Section 1", "Name 3", "DefValue 3", &ValLen); 

   ini.GetValue("Section 2", "Name 1", "DefValue 1", &ValLen);  
   ini.GetValue("Section 2", "Name 2", "DefValue 2", &ValLen); 

  
 //ini.AddEmptyLines(3);

      ini..AddComment("First comment");
   CMiniIni::SSecRec sec = ini.AddSection("Section 1");
     ini.AddComment("Hello sec comment 1",0,nullptr,true);
   ini.AddValue("Name 1", "Value 1");
    ini.AddComment("Hello comment 1",0,nullptr,true);
   ini.AddValue("Name 2", "Value 2");
       ini.AddComment("Hello norm comment 1");
   ini.AddValue("Name 3", "Value 3");
  // ini.AddEmptyLines(5);

 //  ini.AddValue(nullptr, "Value 1");
 //  ini.AddValue(nullptr, "Value 2");
 //  ini.AddValue(nullptr, "Value 3");

 //  ini.AddSection("Section 1");
 //  ini.AddSection("Section 2");
  // ini.AddSection("Section 3");

    ini.AddValue("Name 4", "Value 4", 0, 0, &sec);

     CMiniIni::SSecRec sec2 = ini.AddSection("SubSec 1", 0, &sec);
         ini.AddComment("Hello sub comment 0");
       CMiniIni::SValRec val = ini.AddValue("Sub Nam 1", "Sub Val 1", 0, 0, &sec2);
       ini.AddComment("Hello sub comment 1");
      ini.AddValue("Sub Nam 2", "Sub Val 2", 0, 0, &sec2);
       ini.AddSection("SubSec 2", 0, &sec2);
       
       ini.RemoveRec(&sec);     */
   ini.Save("D:/TEST/testout1.ini"); 
  }


 {
/*  CMiniIni ini;
  CArr<achar> DstData;
  sint res = ini.Load("D:/TEST/test.ini");
  DBGMSG("ini load result: %i",res);
 // ini.Flags |= CMiniIni::efUseDisabledSec;
  CMiniIni::SSecRec sec = ini.FindSection("belkas");
  CMiniIni::SValRec rec = ini.FindValue(&sec, "help");
  uint Size = 0;
  const achar* nam = rec.GetName(Size);
  const achar* val = rec.GetValue(Size);
  int rr = 6;  */
 // ini.Save("D:/TEST/testout.ini");
 // ini.Save(DstData);
//  DstData.IntoFile("D:/TEST/testout2.ini");
 }


 /*
 CArr<uint8> arr;
 arr.FromFile("C:/WORKFOLDER/_MISC_/TestTZ/Yekaterinburg");
 sint64 offs = NPTM::STZF::GetTimeZoneOffset(arr.Data(),0);

 PX::timeval tv2 = {(ssize_t)NDT::FileTimeToUnixTime(NPTM::NTX::GetLocalTime()),0};
 
 NDT::UnixTimeToDateTime(&tv2, &dt);

 NDT::DT dt_bias = {};
 PX::timeval tv_bias = {NPTM::GetTZOffsUTC(),0};
 NDT::UnixTimeToDateTime(&tv_bias, &dt_bias);

 {
  NPTM::PX::SFStat sti;
  int res = NPTM::NAPI::stat("I:/x64.syscall.sh.pdf", &sti);     // "/tmp/test"
  DBGMSG("sta: %i: mode=%08X",res,sti.mode);
 }


 {
  uint8 DirEntBuf[2048];    
  int hDir = NPTM::NAPI::open("C://TST", PX::O_DIRECTORY|PX::O_RDONLY, 0666);    // C:/TST     "/run"
  DBGMSG("hDir: %i",hDir);
  if(hDir > 0)
   {
    for(sint Total=0,Offset=0;;)
     {       
      if(Offset >= Total)
       {
        Total  = NPTM::NAPI::getdents(hDir, DirEntBuf, -sizeof(DirEntBuf));
        if(Total <= 0){ DBGMSG("No more: %i",Total); break;}    // No more or an error
        Offset = 0;
       }
      NPTM::PX::SDirEnt* ent = (NPTM::PX::SDirEnt*)&DirEntBuf[Offset];
      Offset += ent->reclen;
      LOGMSG("FILE: ino=%016llX, off=%016llX, reclen=%04X, type=%02X, name=%s",ent->ino, ent->off, ent->reclen, ent->type, &ent->name);
     }
    NPTM::NAPI::close(hDir);
   }
  return 0;
 }

 {
  NPTM::PX::SFStat sti;
  int res = NPTM::NAPI::stat("/tmp/test", &sti);     // "/tmp/test"    "I:/DOWNLOADS/_PROGR/1/x64.syscall.sh.pdf"
  DBGMSG("sta: %i: mode=%08X",res,sti.mode);
 }


 {
  uint8 DirEntBuf[2048];    
  int hDir = NPTM::NAPI::open("/run", PX::O_DIRECTORY|PX::O_RDONLY, 0666);    //  "/run"  "C:/WORKFOLDER/_PAYED_/LiliPop/_PROJECT_/WHMUpdPkgTool/TST"    
  DBGMSG("hDir: %i",hDir);
  if(hDir > 0)
   {
    for(sint Total=0,Offset=0;;)
     {       
      if(Offset >= Total)
       {
        Total  = NPTM::NAPI::getdents(hDir, DirEntBuf, -sizeof(DirEntBuf));
        if(Total <= 0){ DBGMSG("No more: %i",Total); break;}    // No more or an error
        Offset = 0;
       }
      NPTM::PX::SDirEnt* ent = (NPTM::PX::SDirEnt*)&DirEntBuf[Offset];
      Offset += ent->reclen;
      LOGMSG("FILE: ino=%016llX, off=%016llX, reclen=%04X, type=%02X, name=%s",ent->ino, ent->off, ent->reclen, ent->type, &ent->name);
     }
    NPTM::NAPI::close(hDir);
   }
  return 0;
 } */

 //volatile achar xbuf[512];
 //uint64 ss = 0x1122334455667788;
 //StoreAsBytes(&ss, (void*)&xbuf[0]);
 //return NCRYPT::CRC32(xbuf);

 /*volatile uint8 xbuf[512] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
 uint64 vv = 0;
 LoadAsBytes(&vv, (void*)&xbuf[0]);
 return vv;*/

 /*volatile uint8 xbuf[512] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
 volatile achar tbuf[512];
 MemCopySync((char*)&tbuf, (char*)&xbuf, sizeof(tbuf));
 return NCRYPT::CRC32(tbuf); */

 volatile uint8 xbuf[512] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
 int ofs = 0;
 NMOPS::MemMove((void*)&xbuf[12], (void*)&xbuf[0], 56); 
 NMOPS::MemMove((void*)&xbuf[2], (void*)&xbuf[12], 56);  
 NMOPS::MemFill((void*)&xbuf[0], 56, (uint8)0xDD);  
// MemMove((void*)&xbuf[ofs], (void*)&xbuf[0], sizeof(xbuf)-ofs);    //   MemCopySync((char*)&tbuf, (char*)&xbuf, sizeof(tbuf));
 return NCRYPT::CRC32((achar*)&xbuf); 
}
//------------------------------------------------------------------------------------------------------------

};
//------------------------------------------------------------------------------------------------------------


