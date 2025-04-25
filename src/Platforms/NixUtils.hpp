
#pragma once

struct UNIX
{
// AUX vector types    
enum EAFlags
{
// Legal values for a_type (entry type).
 AT_NULL              = 0,    // End of vector
 AT_IGNORE            = 1,    // Entry should be ignored
 AT_EXECFD            = 2,    // File descriptor of program
 AT_PHDR              = 3,    // Program headers for program     // &phdr[0]            // <<<<<
 AT_PHENT             = 4,    // Size of program header entry    // sizeof(phdr[0])
 AT_PHNUM             = 5,    // Number of program headers       // # phdr entries
 AT_PAGESZ            = 6,    // System page size                // getpagesize(2)      // <<<<<
 AT_BASE              = 7,    // Base address of interpreter     // ld.so base addr     // <<<<<
 AT_FLAGS             = 8,    // Flags                           // processor flags
 AT_ENTRY             = 9,    // Entry point of program          // a.out entry point   // <<<<<
 AT_NOTELF            = 10,   // Program is not ELF
 AT_UID               = 11,   // Real uid
 AT_EUID              = 12,   // Effective uid
 AT_GID               = 13,   // Real gid
 AT_EGID              = 14,   // Effective gid
 AT_CLKTCK            = 17,   // Frequency of times()
// Some more special a_type values describing the hardware.
 AT_PLATFORM          = 15,   // String identifying platform.
 AT_HWCAP             = 16,   // Machine-dependent hints about processor capabilities.
// This entry gives some information about the FPU initialization performed by the kernel.
 AT_FPUCW             = 18,   // Used FPU control word.
// Cache block sizes.
 AT_DCACHEBSIZE       = 19,   // Data cache block size.
 AT_ICACHEBSIZE       = 20,   // Instruction cache block size.
 AT_UCACHEBSIZE       = 21,   // Unified cache block size.
// A special ignored value for PPC, used by the kernel to control the interpretation of the AUXV. Must be > 16.
 AT_IGNOREPPC         = 22,   // Entry should be ignored.
 AT_SECURE            = 23,   // Boolean, was exec setuid-like?
 AT_BASE_PLATFORM     = 24,   // String identifying real platforms.
 AT_RANDOM            = 25,   // Address of 16 random bytes.                // <<<<<
 AT_HWCAP2            = 26,   // More machine-dependent hints about processor capabilities.
 AT_EXECFN            = 31,   // Filename of executable.                      // <<<<<
// Pointer to the global system page used for system calls and other nice things.
 AT_SYSINFO           = 32,
 AT_SYSINFO_EHDR      = 33,   // The base address of the vDSO           // <<<<<
// Shapes of the caches.  Bits 0-3 contains associativity; bits 4-7 contains log2 of line size; mask those to get cache size.
 AT_L1I_CACHESHAPE    = 34,
 AT_L1D_CACHESHAPE    = 35,
 AT_L2_CACHESHAPE     = 36,
 AT_L3_CACHESHAPE     = 37,
// Shapes of the caches, with more room to describe them. *GEOMETRY are comprised of cache line size in bytes in the bottom 16 bits and the cache associativity in the next 16 bits.
 AT_L1I_CACHESIZE     = 40,
 AT_L1I_CACHEGEOMETRY = 41,
 AT_L1D_CACHESIZE     = 42,
 AT_L1D_CACHEGEOMETRY = 43,
 AT_L2_CACHESIZE      = 44,
 AT_L2_CACHEGEOMETRY  = 45,
 AT_L3_CACHESIZE      = 46,
 AT_L3_CACHEGEOMETRY  = 47,
 AT_MINSIGSTKSZ       = 51,   // Stack needed for signal delivery (AArch64).
};
//------------------------------------------------------------------------------------------------------------
struct SAuxVecRec
{
 size_t type;       // Entry type
 union
  {
   size_t val;      // Integer value
   vptr   ptr;      // Pointer value
  };
};
//------------------------------------------------------------------------------------------------------------
static int UpdateTZOffsUTC(sint64 CurTimeUTC)  // Returns timezone offset in seconds
{
 int df = NAPI::open("/etc/localtime",PX::O_RDONLY,0);
 //DBGDBG("TZFILE %lli open: %i",CurTimeUTC,df);
 if(df < 0)
  {
   if(-df == PX::ENOENT)return ReadTZOffsFromFile();
   return df;
  }
 sint64 flen = NAPI::lseek(df, 0, PX::SEEK_END);     // Avoiding unreliable fstat
 if(flen < 0)return (int)flen;
 NAPI::lseek(df, 0, PX::SEEK_SET);
 uint8* filedata = (uint8*)StkAlloc(flen+8);
 sint rlen = NAPI::read(df, filedata, flen);
 NAPI::close(df);
 if(rlen < flen)return -PX::ENODATA;     // sizeof(SHdrTZ)
 sint32 offs = STZF::GetTimeZoneOffset(filedata,CurTimeUTC);
// DBGDBG("TZFILE %u offs: %i",rlen,offs);
 if(offs < 0)return -PX::EINVAL;
 fwsinf.UTCOffs = offs;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: On mobile phones and other small devices that run Linux, the time zone is stored differently. It is written in /etc/TZ
// In the POSIX timezone format, the 3 letters are the timezone abbreviation (which is arbitrary) and the number is the number
// of hours the timezone is behind UTC. So UTC-8 means a timezone abbreviated "UTC" that is -8 hours behind the real UTC, or UTC + 8 hours.
//
static int ReadTZOffsFromFile(void)  // Returns timezone offset in seconds
{
 int df = NAPI::open("/etc/TZ",PX::O_RDONLY,0);
 if(df < 0)return df;
 achar buf[128];
 sint rlen = NAPI::read(df, buf, sizeof(buf)-1);
 NAPI::close(df);
 sint32 offs = 0;
 if(rlen)
  {
   buf[rlen] = 0;
//   DBGDBG("TZFILE %u: %s",rlen,&buf);
   for(sint idx=0;idx < rlen;idx++)
    {
     achar val = buf[idx];
     if((val >= '0')&&(val <= '9'))     // Most likely incorrect  // CST-8, UTC-8  // How to actually parse
      {
       offs = NCNV::DecStrToNum<int32>(&buf[idx]);   // Hours?
       offs = offs * 3600;  // Hours to seconds
       if(buf[idx-1] != '-')offs = -offs;   // hours behind
       break;
      }
    }
  }
 fwsinf.UTCOffs = offs;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static PX::fdsc_t OpenDevNull(void)
{
 return NPTM::NAPI::open("/dev/null",PX::O_RDWR,0);
}
//------------------------------------------------------------------------------------------------------------
static PX::fdsc_t OpenDevRand(void)
{
 return NPTM::NAPI::open("/dev/random",PX::O_RDONLY,0);
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// AddrOnStack is any address on stack because __builtin_frame_address(0) is unreliable
// NOTE: ArgC may be 0
// NOTE: This is not 100% reliable but should help with finding the startup info even if the process started by an loader
// TEST: Test it with 0 ArgC
//
// Detect:
//   ArgC
//   ArgPtrs...
//   Null
//   EVarPtrs...
//   Null
//
//
//
static vptr FindStartupInfo(vptr AddrOnStack)
{
 constexpr const size_t MaxArgC  = 0x1000;   // Max is 4k of arguments
 constexpr const size_t StkAlgn  = 0x1000;   // Max is 4k forward   // NOTE: Bottom of the stack is unknown
 size_t* CurPtr = (size_t*)AddrOnStack;
 size_t* EndPtr = (size_t*)AlignFrwdP2((size_t)AddrOnStack, StkAlgn);
 enum EState {stZero,stArgC,stArgs,stEnvs,stAuxV,stDone};
 EState state = stZero;
 if(((uint8*)EndPtr - (uint8*)CurPtr) < 0x100)EndPtr += StkAlgn;  // Too small, another page must exist
 size_t* TblEndPtr = nullptr;
 size_t* InfoPtr = nullptr;
 size_t ArgC  = 0;    // Counts everything
 size_t ZeroC = 0;
// DBGDBG("Scanning range: %p - %p",CurPtr,EndPtr);

 for(;;)
  {
   if(CurPtr >= EndPtr)
    {
     if(state != stArgC)    // Stop only on ArgC search
      {
       if(ArgC < 8)    // Min reliable counter
        {
         state  = stArgC;
         CurPtr = InfoPtr;
        }
         else {EndPtr += StkAlgn; continue;} // Another page is expected   // DBGDBG("PageAdded: EndPtr=%p",EndPtr);
      }
       else break;
    }
   else if(state == stZero)     // There may be only zeroes pushed on stack, they should not be assumed as zero args and evars
    {
     size_t val = *CurPtr;
     if(val)
      {
//       DBGDBG("Nonzero at: %p - %p",CurPtr, (vptr)val);
       if((val > MaxArgC) && (ZeroC >= 2))CurPtr -= 2;    // In case of actually zero ArgC     // Empty ArgV is unlikely
       state = stArgC;
       continue;      // Avoid CurPtr++
      }
       else ZeroC++;
    }
   else if(state == stArgC)
    {
     ArgC = *CurPtr;
     if(ArgC < MaxArgC)
      {
       state = stArgs;
       InfoPtr = CurPtr;
       TblEndPtr = &CurPtr[ArgC+1];    // Expected to contain NULL but may happen to be outside of our EndPtr or completely wrong if the ArgC is a mistake
//       DBGDBG("ArgC at: %p",CurPtr);
      }
    }
   else if(state == stArgs)
    {
     if(ArgC)
      {
//       DBGDBG("ArgRec at: %p",CurPtr);
       if((uint8*)*CurPtr <= (uint8*)TblEndPtr){state = stArgC; CurPtr = InfoPtr;}    // The string address is wrong - reset    // Pointers should point forward (The strings are closer to the stack`s top)
        else ArgC--;
      }
     else if(!*CurPtr)state = stEnvs;
           else {state = stArgC; CurPtr = InfoPtr;}     // Unexpected non-null - reset
    }
   else if(state == stEnvs)
    {
//     DBGDBG("EnvRec at: %p",CurPtr);
     if(!*CurPtr)state = stAuxV;
     else if((uint8*)*CurPtr <= (uint8*)TblEndPtr){state = stArgC; CurPtr = InfoPtr;}  // The string address is wrong - reset    // TblEndPtr points not at the end of ENVs but should be OK
          else ArgC++;
    }
   else if(state == stAuxV)   // Only ways out of this state is success or reaching EndPtr
    {
//     DBGDBG("AuxRec at: %p",CurPtr);
     SAuxVecRec* Rec = (SAuxVecRec*)CurPtr;
     if(Rec->type == AT_NULL){state = stDone; break;}
      else {CurPtr++; ArgC++;}
    }
   CurPtr++;
  }
// DBGDBG("Finished with: %u",state);
 if(state != stDone)return nullptr;   // Not found
 return InfoPtr;
}
//------------------------------------------------------------------------------------------------------------
static vptr FindStartupInfoByAuxV(vptr AddrOnStack)
{
 constexpr const size_t MaxArgC  = 0x1000;   // Max is 4k of arguments
 constexpr const size_t StkAlgn  = 0x1000;   // Max is 4k forward   // NOTE: Bottom of the stack is unknown
 size_t* CurPtr = (size_t*)AddrOnStack;
 size_t* EndPtr = (size_t*)AlignFrwdP2((size_t)AddrOnStack, StkAlgn);
 size_t* AuxEndNull = nullptr;
// DBGDBG("Starting from: %p",CurPtr);
 for(uint ARep=0;!AuxEndNull && (ARep < 2);ARep++)  // Alignment correction
  {
 for(sint MatchCtr=0,Idx=ARep,pidx=-((bool)!((size_t)CurPtr & (StkAlgn-1)));(Idx < 256)&&(pidx < 2);Idx+=2)
  {
   size_t num = CurPtr[Idx];
   size_t val = CurPtr[Idx+1];
 //  DBGDBG("Aux %p: %p %p, %u",&CurPtr[Idx],(vptr)num,(vptr)val, MatchCtr);
   if(!((size_t)&CurPtr[Idx] & (StkAlgn-1)))pidx++;
    else if(!((size_t)&CurPtr[Idx+1] & (StkAlgn-1)))pidx++;
   if(!num)
    {
     if(MatchCtr > 6){AuxEndNull=&CurPtr[Idx]; break;}  // Probably end of AuxV
     MatchCtr = 0;
     continue;
    }
// Most popular AUXV pointer values
   if((num == AT_PHDR)&&(val >= StkAlgn))MatchCtr++;
   else if((num == AT_BASE)&&(val >= StkAlgn))MatchCtr++;
   else if((num == AT_ENTRY)&&(val >= StkAlgn))MatchCtr++;
   else if((num == AT_EXECFN)&&(val >= StkAlgn))MatchCtr++;
   else if((num == AT_RANDOM)&&(val >= StkAlgn))MatchCtr++;
   else if((num == AT_SYSINFO_EHDR)&&(val >= StkAlgn))MatchCtr++;
// Most popular AUXV  integer values
   else if((num == AT_PAGESZ)&&(val <= StkAlgn))MatchCtr++;
   else if((num == AT_PHNUM)&&(val <= StkAlgn))MatchCtr++;
   else if((num == AT_PHENT)&&(val <= StkAlgn))MatchCtr++;
   else if((num == AT_UID)&&(val <= StkAlgn))MatchCtr++;
   else if((num == AT_GID)&&(val <= StkAlgn))MatchCtr++;
   else if((num == AT_EUID)&&(val <= StkAlgn))MatchCtr++;
   else if((num == AT_EGID)&&(val <= StkAlgn))MatchCtr++;
  }
  }
// DBGDBG("AuxVec end: %p",AuxEndNull);
 if(!AuxEndNull)return nullptr;
 size_t* VPtr = AuxEndNull;
 bool NullTbl = false;
 for(;;) // Count AuxV
  {
   VPtr -= 2;
   if(!VPtr[0] && !VPtr[1]){NullTbl=true;break;}  // End of the AuxV table (Prev tables is null)  // NOTE: No way to know what tables are null there (Most likely EnvP)
   if(!VPtr[1] && (VPtr[0] >= StkAlgn) && (VPtr[0] >= (size_t)AuxEndNull))break;   // The ID is actually a valid pointer in a table above
  }
// DBGDBG("Ptr AuxV: %p ",VPtr);
 if(!NullTbl)
  {
 for(;;)  // Count EnvP
  {
   VPtr--;
   if(!*VPtr)break;  // End of the EnvP table
   if(*VPtr < StkAlgn)return nullptr; // Not a pointer
   if((size_t*)*VPtr <= AuxEndNull)return nullptr;   // The data is not after the table
  }
  }
// DBGDBG("Ptr EnvP: %p ",VPtr);
 for(;;)  // Count ArgV
  {
   VPtr--;
   if(!*VPtr)break;  // End of the ArgV table (No records)
   if(*VPtr < StkAlgn)break; // Probably ArgC is reached
   if((size_t*)*VPtr <= AuxEndNull)return nullptr;   // The data is not after the table
  }
// DBGDBG("Ptr Final: %p ",VPtr);
 return VPtr;
}
//------------------------------------------------------------------------------------------------------------
// Reads a string from a null-terminated string array such as Args or EVars
static sint GetStrFromArr(sint* AIndex, const achar** Array, achar* DstBuf, uint BufLen=uint(-1))
{
 if(DstBuf)*DstBuf = 0;
 if(*AIndex < 0)return -3;     // Already finished
 if(!Array[*AIndex])return -2;    // No such arg   // (AOffs >= (sint)GetArgC())
 const achar* CurStr = Array[(*AIndex)++];  //  GetArgV()[AOffs++];
 uint ArgLen = 0;
 if(!DstBuf)
  {
   while(CurStr[ArgLen])ArgLen++;
   return sint(ArgLen+1);   // +Space for terminating 0
  }
 for(;CurStr[ArgLen] && (ArgLen < BufLen);ArgLen++)DstBuf[ArgLen] = CurStr[ArgLen];
 DstBuf[ArgLen] = 0;
 if(!Array[*AIndex])*AIndex = -1;    // (AOffs >= (sint)GetArgC())
 return sint(ArgLen);
}
//------------------------------------------------------------------------------------------------------------
// Returns FDesc if found and optional path
// https://github.com/BishopFox/asminject/blob/main/docs/examples-shared_library_injection.md
//
static sint FindDynLoader(achar* Buf=nullptr, usize MaxLen=0)
{
// NCTM::CPStr LdLinux1("/lib/ld-linux.so.3");   // ArmX32 or X86X64  // Usually present, at least as a link (Native, not X32 compatibility layer)
 //NCTM::CPStr LdLinux2("/lib/ld-linux.so.2");   // X86X32
 //NCTM::CPStr LdLinux3("/system/bin/linker");   // Android 
// NCTM::CPStr LdLinux4("/system/bin/linker64"); // Android 
         
                             // int *__fastcall dl_allocate_tls(_DWORD *)
 //sint df = -1;
 //df = NPTM::NAPI::openat(PX::AT_FDCWD,FileName,PX::O_RDONLY,0);
 return 0;//df;
}
//------------------------------------------------------------------------------------------------------------
};