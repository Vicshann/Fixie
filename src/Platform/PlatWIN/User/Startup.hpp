
#pragma once

//------------------------------------------------------------------------------------------------------------
private:
//------------------------------------------------------------------------------------------------------------
class SModStrm    // Reads the module from somewhere    // Uses direct access on X64
{
public:
SCVR size_t BufMax = 1024;   // Can't read more than this
private:
 uint64 DllBase;
 size_t DllSize;  
 size_t Offset;   // Current offset Relative to the DllBase
 size_t BufOffs;  // Offset of the buffer base Relative to the DllBase
 uint8  Buffer[BufMax];

//------------------------------------------------------

public:
//------------------------------------------------------
int Init(NT::UNICODE_STRING* Name, bool Wow=false)      // Can't use CRC, the FS is case insensitive by default
{
#ifdef ARCH_X32
 if(Wow)this->DllBase = WOW64E::GetModuleHandle64(Name->Buffer, &DllSize);
   else this->DllBase = (uint64)NTX::LdrGetModuleBase(Name->Buffer, &DllSize);
#else
 this->DllBase = (uint64)NTX::LdrGetModuleBase(Name->Buffer, &DllSize); 
#endif
 this->Offset  = 0;
 this->BufOffs = (size_t)-1;
 return 0;
}
//------------------------------------------------------
uint8  ReadUint8(size_t At=size_t(-1))     // Negative is not applied     // Every fynction uses 'At' which is not cheap but this is actually more frequently used
{
 uint8 val;
 ReadBytes(&val, sizeof(val), At);
 return val;
}
//------------------------------------------------------
uint16 ReadUint16(size_t At=size_t(-1))
{
 uint16 val;
 ReadBytes(&val, sizeof(val), At);
 return val;
}
//------------------------------------------------------
uint32 ReadUint32(size_t At=size_t(-1))
{
 uint32 val;
 ReadBytes(&val, sizeof(val), At);
 return val;
}
//------------------------------------------------------
uint64 ReadUint64(size_t At=size_t(-1))
{
 uint64 val;
 ReadBytes(&val, sizeof(val), At);
 return val;
}
//------------------------------------------------------
size_t ReadBytes(vptr Addr, size_t len, size_t At=size_t(-1))     // A circular buffer may be better?
{
 if(At == size_t(-1))At = this->Offset;  // Continue at current offset
 size_t end = At + len;
 if(end > this->DllSize){end = this->DllSize; len = end - At;}
 if((At < this->BufOffs)||(end >= (this->BufOffs + BufMax)))   // Need reread
  {
   this->BufOffs = AlignBkwdP2(At, 16);   // sizeof(uint64) ?
   size_t Size = ((this->BufOffs + BufMax) > this->DllSize)?(this->DllSize - this->BufOffs):BufMax;
#ifdef ARCH_X32
   if(this->DllBase > 0xFFFFFFFFull)WOW64E::getMem64(&this->Buffer, this->DllBase + this->BufOffs, Size);     // Beyond 4GB (WOW64)
     else memcpy(&this->Buffer, (uint8*)this->DllBase + this->BufOffs, Size);   
#else
   memcpy(&this->Buffer, (uint8*)this->DllBase + this->BufOffs, Size);
#endif
  }
 size_t offs = At - this->BufOffs;
 memcpy(Addr, &this->Buffer[offs], len);
 this->Offset = At + len;
 return len;   
}
//------------------------------------------------------
};

//------------------------------------------------------------------------------------------------------------
// Compares sequences of stubs and finds most frequent match
[[clang::optnone]] _ninline static auto CalcSyscallStubInfo(SModStrm* ModStrm, uint32* HashArr, uint32* AddrArr) __attribute__ ((optnone))    // NOTE: optnone is ignored!!!  // NOTE: This function is randomly messed up even with -O1
{
 struct SRes {uint SCNOffs; uint StubSize; uint64 StubFB;};
 SCVR   uint MaxStubLenA = 16;   // Max dist to syscall number
 SCVR   sint MaxStubLenB = 64;   // Max syscall stub size

 uint32 WinHdrRVA = ModStrm->ReadUint32(OffsetOf(NPE::SDosHdr, OffsetHeaderPE));
 uint32 ExpDirRVA = ModStrm->ReadUint32(((IsArchX64 || NTX::IsWow64())?(OffsetOf(NPE64::SWinHdr, OptionalHeader.DataDirectories.ExportTable)):(OffsetOf(NPE32::SWinHdr, OptionalHeader.DataDirectories.ExportTable))) + WinHdrRVA);
 uint32 ExpDirLen = ModStrm->ReadUint32();
 if(!ExpDirRVA || !ExpDirLen)return SRes{0,0,0};		 // No export directory!
 NPE::SExpDir Export;
 ModStrm->ReadBytes(&Export, sizeof(Export), ExpDirRVA);

 uint8   PrvFEByte = 0; //uint32  PrevEntry = 0;
 uint32  LastEntry = 0;
 uint16  MCount[MaxStubLenA]   = {};    // Most of same dsizes are hopefully belong to syscall stubs
 uint16  SCount[MaxStubLenB]   = {};
 uint16  BCount[MaxStubLenB*2] = {};
 uint64  BVals[MaxStubLenB*2]  = {};    // Keep first 8 bytes for each of stub sizes and swith to other half of the array on each mismatch

 sint LastDist = 0;      
 sint BCntOffs = -1;     
 for(volatile uint ctr=0, tot=Export.NamePointersNumber;ctr < tot;ctr++)  // Find diff sizes and hash names  // Without 'volatile' the 'ctr' is comepletely broken with -O2 !!!
  {
   uint32 nrva = ModStrm->ReadUint32(Export.NamePointersRVA + (ctr * sizeof(uint32))); // NamePointers[ctr];
   if(!nrva)continue;  // No name, ordinal only (Not needed right now)
   uint32 crc = ~NCRYPT::InitialCrc32;
   for(uint8 val=ModStrm->ReadUint8(nrva);val;val=ModStrm->ReadUint8())crc = NCRYPT::ByteCrc32(val,crc,NCRYPT::DefRevPolyCrc32);
   uint32 NameHash = ~crc;  //NCRYPT::CRC32((achar*)&((uint8*)ModBase)[nrva]);      // TODO: Use fast Table/Hardware optimized version
   uint16 Ordinal  = ModStrm->ReadUint16(Export.OrdinalTableRVA + (ctr * sizeof(uint16)));   // OrdinalTable[ctr]
   uint32 CurEntry = ModStrm->ReadUint32(Export.AddressTableRVA + (Ordinal * sizeof(uint32)));   //  &((uint8*)ModBase)[AddressTable[OrdinalTable[ctr]]];
   for(uint idx=0;HashArr[idx];idx++){if(!AddrArr[idx] && (HashArr[idx] == NameHash))AddrArr[idx] = CurEntry;}   // Found an address for API name
   if(LastEntry)
    {
     if(CurEntry == LastEntry)continue;   // Same export
     sint CurDist = CurEntry - LastEntry;
     uint mlen = 0;
     uint8 CurEntBuf[MaxStubLenA];
     uint8 LastEntBuf[MaxStubLenA];
     ModStrm->ReadBytes(&CurEntBuf,  MaxStubLenA, CurEntry);
     ModStrm->ReadBytes(&LastEntBuf, MaxStubLenA, LastEntry);
     for(uint idx=0;(idx <= MaxStubLenA)&&(LastEntBuf[idx]==CurEntBuf[idx]);idx++)mlen++;
     if(mlen && (mlen < MaxStubLenA))MCount[mlen]++;
     if((CurDist == LastDist)&&(CurDist < MaxStubLenB))
      {
       SCount[CurDist]++;
       if(BCntOffs < 0){BCntOffs = 0; BVals[BCntOffs + CurDist] = *(uint64*)&CurEntBuf;}
       if(*CurEntBuf != PrvFEByte)  //ModStrm->ReadUint8(PrevEntry))   // ??? Can cache *PrevEntry?                    //*CurEntry != *PrevEntry)   // mlen is 0    // Min is 1 byte for first syscall instruction
        {
         sint OldBCntOffs = BCntOffs;        
         BCntOffs = BCntOffs ? 0 : MaxStubLenB;
         if(BCount[BCntOffs + CurDist] > BCount[OldBCntOffs + CurDist])  // Preserve in opposite slot if here counter is bigger than there   // Needed?
          {
           BCount[OldBCntOffs + CurDist] = BCount[BCntOffs + CurDist];
           BVals[OldBCntOffs + CurDist]  = BVals[BCntOffs + CurDist];
          }
         BCount[BCntOffs + CurDist] = 1;   // Start over
         BVals[BCntOffs + CurDist]  = *(uint64*)&CurEntBuf;
        }
         else BCount[BCntOffs + CurDist]++;
       PrvFEByte = *CurEntBuf;  // PrevEntry = CurEntry;
      }
     LastDist = CurDist;
    }
     else PrvFEByte = ModStrm->ReadUint8(CurEntry);  //PrevEntry = CurEntry;
   LastEntry = CurEntry;
  }
 uint16 MaxLen = 0;
 uint BestOffs = 0;
 uint BestSize = 0;
 uint BestBIdx = 0;
 for(uint idx=0;idx < MaxStubLenA;idx++){if(MCount[idx] > MaxLen){MaxLen=MCount[idx];BestOffs=idx;}}  // Find most frequently encountered match size (From 1?)
 MaxLen = 0;
 for(uint idx=0;idx < MaxStubLenB;idx++){if(SCount[idx] > MaxLen){MaxLen=SCount[idx];BestSize=idx;}}  // Find most frequently encountered proc size
 MaxLen = 0;
 for(uint idx=1;idx < (MaxStubLenB*2);idx++){if(BCount[idx] > MaxLen){MaxLen=BCount[idx];BestBIdx=idx;}}  // Find most frequent (likely not hooked) first bytes of syscall stub
 return SRes{BestOffs, BestSize, BVals[BestBIdx]};
}
//------------------------------------------------------------------------------------------------------------
// Try to find a valid syscall before or after and extrapolate to required syscall number
// Returns an offset to add to syscall at the returned stub
// NOTE: There is no gurantee that syscall numbers be in order
static sint FindNotHookedStub(SModStrm* ModStrm, uint32* CurStub, uint64 StubFB, uint SCOffs, uint StubLen, uint MaxDist)       // !!! SModStrm* ModStrm
{
 for(uint ctr=1;ctr <= MaxDist;ctr++)    // Scan forward
  {
   uint32 rva = *CurStub + (ctr * StubLen);
   uint64 fbv = ModStrm->ReadUint64(rva);
   bool Hooked = memcmp(&fbv, &StubFB, SCOffs);    // Max 8 bytes. Optimize?
   if(!Hooked){*CurStub = rva; return -(sint)ctr;}
  }
 for(uint ctr=1;ctr <= MaxDist;ctr++)    // Scan backward
  {
   uint32 rva = *CurStub - (ctr * StubLen);
   uint64 fbv = ModStrm->ReadUint64(rva);
   bool Hooked = memcmp(&fbv, &StubFB, SCOffs);
   if(!Hooked){*CurStub = rva; return (sint)ctr;}
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static sint ExtractSyscallNumbers(SModStrm* ModStrm, uint64 StubFB, uint SCOffs, uint StubLen, uint Total, uint32* AddrArr, uint32* SyscArr)    // !!! SModStrm* ModStrm
{
 sint ResCtr = 0;
 for(uint ctr=0;ctr < Total;ctr++)
  {
   uint32 rva = AddrArr[ctr]; 
   if(!rva)continue; 
   uint64 fb = ModStrm->ReadUint64(rva);        
   bool Hooked = memcmp(&fb, &StubFB, SCOffs);   // memcmp(AddrArr[ctr], &StubFB, SCOffs);    // Compare until a syscall number (should be same)
   if(Hooked)
    {
     sint soffs = FindNotHookedStub(ModStrm, &rva, StubFB, SCOffs, StubLen, 5);
     if(!soffs)continue;    // Failed to resolve
     SyscArr[ctr] = uint32((sint32)ModStrm->ReadUint32(rva+SCOffs) + soffs);     // uint32(*(int32*)&rva[SCOffs] + soffs);
    }
     else SyscArr[ctr] = ModStrm->ReadUint32(rva+SCOffs);   //*(uint32*)&AddrArr[ctr][SCOffs];
   ResCtr++;
  }
 return ResCtr;
}
//------------------------------------------------------------------------------------------------------------
static uint FindSyscallStubsBlock(uint8** SBlkPtrBeg, uint8** SBlkPtrEnd)  
{
 uint8* PtrBeg = (uint8*)&SAPI::NtProtectVirtualMemory;    // NOTE: may be relocated  (Or not?)     // NOTE: Expects relocs to be resolved
 uint8* PtrEnd = PtrBeg;
 uint8 tmpl[NSYSC::MinStubSize];
 memcpy(tmpl, PtrBeg, sizeof(tmpl));
 *(uint32*)&tmpl[NSYSC::SYSCALLOFFS] = 0;
#if defined(SYS_WINDOWS) && defined(ARCH_X32)
   *(uint8*)&tmpl[NSYSC::ARGSIZEOFFS] = 0;      // Ret args         
#endif

 for(;;PtrBeg -= NSYSC::MaxStubSize)
  {
   uint8 curr[NSYSC::MaxStubSize];
   memcpy(curr, &PtrBeg[-NSYSC::MaxStubSize], sizeof(curr));
   *(uint32*)&curr[NSYSC::SYSCALLOFFS] = 0;
#if defined(SYS_WINDOWS) && defined(ARCH_X32)
   *(uint8*)&curr[NSYSC::ARGSIZEOFFS] = 0;      // Ret args         
#endif
   if(memcmp(curr, tmpl, NSYSC::MinStubSize))break;
  }
 for(;;)
  {
   uint8 curr[NSYSC::MinStubSize];
   PtrEnd += NSYSC::MaxStubSize;
   memcpy(curr, PtrEnd, sizeof(curr));
   *(uint32*)&curr[NSYSC::SYSCALLOFFS] = 0;
#if defined(SYS_WINDOWS) && defined(ARCH_X32)
   *(uint8*)&curr[NSYSC::ARGSIZEOFFS] = 0;      // Ret args         
#endif
   if(memcmp(curr, tmpl, NSYSC::MinStubSize))break;
  }
 *SBlkPtrBeg = PtrBeg;
 *SBlkPtrEnd = PtrEnd;
 return uint(PtrEnd - PtrBeg);
}
//------------------------------------------------------------------------------------------------------------
// We cannot inline those stubs with a constant numbers
// It would be possible to inline a templated stubs which take syscall numbers from a table (
// Is having a table of syscalls in the module body a bad idea?
// Anyway, stubs are much easier to debug an they can be easily patched in a binary if arguments compatibility will be broken in future versions of OS
// None of this is required on Linux because syscall numbers are already known
//
// Variable length arrays:
//    Visual studio build supported only with Clang
// All path convertions on Windows require such memory allocations(UTF8 to WCHAR)
// -fms-extensions
// -mno-stack-arg-probe   // VLA always causes chkstk
// MSVC: /clang:-mno-stack-arg-probe    // -mstack-probe-size=100000
// LINK: /STACK:0x100000,0x100000
//
static sint InitSyscalls(void)
{
 SModStrm NtDll;
 NT::UNICODE_STRING* FullDllName; 
 NT::UNICODE_STRING* BaseDllName;
 uint8* SBlkPtrBeg  = nullptr;
 uint8* SBlkPtrEnd  = nullptr; 
 uint   StubsLen    = FindSyscallStubsBlock(&SBlkPtrBeg, &SBlkPtrEnd);    // Calculate syscalls stubs size
 uint8* BaseOfNtdll = (uint8*)NTX::GetBaseOfNtdll(&FullDllName, &BaseDllName);   // Take drive as 'C:\'   // SharedUserData->NtSystemRoot ???   &fwsinf.SysDrive, 3
 uint   TotalRecs   = StubsLen / NSYSC::MaxStubSize;

 if(!BaseOfNtdll)return -1;
 NSTR::StrCopy((wchar*)&fwsinf.SysDrive, FullDllName->Buffer, 3);
 bool Wow64 = IsArchX32 && NTX::IsWow64();
 NtDll.Init(BaseDllName, Wow64);     // Need real NTDLL if running under WOW64
 
 uint32 SyscArr[TotalRecs];       // VLA is better than alloca. why extra pointer?
 uint32 HashArr[TotalRecs]; 
 uint32 ProcArr[TotalRecs];       // Offsets of procs relative to DllBase      // uint8* to uint32
 memset(SyscArr,0,TotalRecs*sizeof(*SyscArr));
 memset(HashArr,0,TotalRecs*sizeof(*HashArr));
 memset(ProcArr,0,TotalRecs*sizeof(*ProcArr));

 for(uint ctr=0;ctr < TotalRecs;ctr++){HashArr[ctr] = *(uint32*)&SBlkPtrBeg[(ctr * NSYSC::MaxStubSize) + NSYSC::SYSCALLOFFS];}   // Gather all hashes (Not all of them may be for NTDLL)
 auto sinf = CalcSyscallStubInfo(&NtDll, HashArr, ProcArr);   // Outputs 'ProcArr'         // TODO: Resolve win32u.dll syscalls (If they can work without user32.dll or gdi32.dll)
// After all DLLs resolved
 ExtractSyscallNumbers(&NtDll, sinf.StubFB,sinf.SCNOffs, sinf.StubSize, TotalRecs, ProcArr, SyscArr);    // Outputs 'SyscArr'

 vptr   Addr    = SBlkPtrBeg;
 size_t Size    = StubsLen;
 uint32 OldProt = 0;   // Always 32 bit?
 uint32 resa    = 0;

#ifdef ARCH_X32
 if(!Wow64)
  {
   auto pNtProtectVirtualMemory = SAPI::NtProtectVirtualMemory.GetPtr<uint32 (_fcall*)(uint32, uint32, size_t, vptr*, size_t*, uint32, uint32*)>();   // First API is expected to be NtProtectVirtualMemory
   *(uint8**)&pNtProtectVirtualMemory += sizeof(size_t);   // !!! Hardcoded offset
   resa = pNtProtectVirtualMemory(SyscArr[0], 0, NT::NtCurrentProcess, &Addr, &Size, NT::PAGE_EXECUTE_READWRITE, &OldProt);   // NtProtectVirtualMemory is expected to be first in SysApi list     // PAGE_EXECUTE_READWRITE may be forbidden, try PAGE_READWRITE but avoid making caller function nonexecutable
  }
   else resa = WOW64::NtProtectVirtualMemory(SyscArr[0], 0, NT::NtCurrentProcess, &Addr, &Size, NT::PAGE_EXECUTE_READWRITE, &OldProt);
#else
 auto pNtProtectVirtualMemory = SAPI::NtProtectVirtualMemory.GetPtr<uint32 (_scall*)(size_t, vptr*, size_t*, uint32, uint32*, uint32)>();   // First API is expected to be NtProtectVirtualMemory
 resa = pNtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, NT::PAGE_EXECUTE_READWRITE, &OldProt, SyscArr[0]);   // NtProtectVirtualMemory is expected to be first in SysApi list     // PAGE_EXECUTE_READWRITE may be forbidden, try PAGE_READWRITE but avoid making caller function nonexecutable
#endif

 if constexpr (IsCpuX86)
  {
   if constexpr (IsArchX32)    //!IsArchX64 && NTX::IsWow64())   
    {
     if(Wow64)
      {
       for(uint ctr=0;ctr < TotalRecs;ctr++)     // TODO: Pointer
        {
         uint8* RecPtr = &SBlkPtrBeg[ctr * NSYSC::MaxStubSize];
         uint32 WowPtr = *(uint32*)&RecPtr[NSYSC::MinStubSize];    // The pointer is expected to be right after the stub bytes    // May be relocated!
         memset(RecPtr, 0xCC, NSYSC::MaxStubSize);
         *RecPtr   = 0xB9;       // mov ecx, scnum   // (fastcall)   // Restore our stub from its static state ( call by a passed syscall number )       
         *(uint32*)&RecPtr[1] = SyscArr[ctr];
         RecPtr[5] = 0xE9;       // jump
         *(sint32*)&RecPtr[6] = -(((size_t)&RecPtr[5] + 5) - WowPtr);                                        // NOTE: Expects relocs to be resolved
        }
      }
       else       // Native X32
        {
         for(uint ctr=0;ctr < TotalRecs;ctr++)
          {
           uint8* RecPtr = &SBlkPtrBeg[ctr * NSYSC::MaxStubSize];
           *RecPtr   = 0xB8;   // mov eax, scnum     // Restore our stub from its static state ( call by a passed syscall number )
           RecPtr[6] = 0xC0;   // mov eax, eax       // Hardcoded!     
           *(uint32*)&RecPtr[1] = SyscArr[ctr];      
           *(uint32*)&RecPtr[NSYSC::MinStubSize] = 0xCCCCCCCC;  // Wipe out WOW64 function pointer (Not used)
          }
        }
    }
     else    // Native X64
      {
       for(uint ctr=0;ctr < TotalRecs;ctr++)
        {
         uint8* RecPtr = &SBlkPtrBeg[ctr * NSYSC::MaxStubSize];
         *RecPtr = 0xB8;   // mov rax/eax        // Restore our stub from its static state ( call by a passed syscall number )
         *(uint32*)&RecPtr[1] = SyscArr[ctr];
         *(uint32*)&RecPtr[NSYSC::SYSCALLOFFS] = 0xCCCCCCCC;  // Wipe out Hash of an API name
        }
      }
  }
 else  // Windows ARM is X64 only, (NoWOW64 like emulation?)    // TODO: Windows ARM (AARCH64)
  {

  }
 resa = SAPI::NtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, OldProt, &OldProt);  // SAPI is should be working by now            //pNtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, OldProt, &OldProt, SyscArr[0]);
 return (sint)StubsLen;
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
enum ESFlags {
        sfInitialized = 0x0001,         // The framework is initialized
        sfDynamicLib  = 0x0002,         // The framework is loaded as a DLL
        sfLoadedByLdr = 0x0004,         // The framework module is loaded by a OS loader
        sfConOwned    = 0x0008,         // The console is created by the framework

        sfLoadedGUI   = 0x0010,         // GUI initialized (A hidden window for some console events may be needed)
        sfServiceApp  = 0x0020,         // The app is a service
        sfNativeApp   = 0x0040,         // The app is a native app (boot process)
        sfDriverApp   = 0x0080,         // The app is a driver

        sfConHandled  = 0x0100,         // A console event handler is installed
        sfWndHandled  = 0x0200,         // A hidden window message handler is installed
        sfExpHandled  = 0x0400,         // An exception handle is installed

        sfTerminating = 0x80000000      // The application is terminating
};

struct SSINF       // Cannot put it on some thread`s stack. Must be persistent to be usable by exported functions
{
 vptr   ModBase;
 size_t ModSize;
 vptr   TheModBase;
 size_t TheModSize;
 vptr   MainModBase;
 size_t MainModSize;
 achar  SysDrive[8];
 uint64 SigMask;       // Mask of signals to handle
 sint32 MemPageSize;
 sint32 MemGranSize;
 sint32 UTCOffs;       // In seconds
 uint32 Flags;
 vptr   pNtDll;
 vptr   hKeyedEvent;   // Futex support only  
 vptr   SigHndlArg;
 NSIG::SigHandlerT SigHandler;   // User-supplied signal handler (One for all signals)
 NTHD::STDesc thd;

 PX::fdsc_t DevNull;
 PX::fdsc_t DevRand;

} static inline fwsinf = {};
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return 1;}   // On Windows should be always 1?
static _finline const wchar* GetArgV(void){return NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CommandLine.Buffer;}     // Single string, space separated, args in quotes
static _finline const wchar* GetEnvP(void){return NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->Environment;}            // Block of null-terminated strings, last is 0
static _finline void UpdateTZOffsUTC(void){fwsinf.UTCOffs = sint32(-NTX::GetTimeZoneBias() / NDT::SECS_TO_FT_MULT);}   // Number of 100-ns intervals in a second
static _finline NTHD::STDesc* GetThDesc(void){return &fwsinf.thd;}
//------------------------------------------------------------------------------------------------------------

public:

static _finline uint32 GetPageSize(void)  {return fwsinf.MemPageSize;}
static _finline uint32 GetGranSize(void)  {return fwsinf.MemGranSize;}
//------------------------------------------------------------------------------------------------------------
// NOTE: Te handles will be 0 if PE subsystem is not 'Windows Console'. Must init those somehow
static _finline PX::fdsc_t GetStdIn(void)  {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardInput; }
static _finline PX::fdsc_t GetStdOut(void) {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardOutput;}  
static _finline PX::fdsc_t GetStdErr(void) {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardError; }

static _finline PX::fdsc_t GetStdNull(void) {return fwsinf.DevNull;}
static _finline PX::fdsc_t GetStdRand(void) {return fwsinf.DevRand;}
//------------------------------------------------------------------------------------------------------------
static _finline sint32 GetTZOffsUTC(void){return fwsinf.UTCOffs;}   // In seconds
static _finline bool   IsInitialized(void) {return fwsinf.Flags & sfInitialized;}
static _finline bool   IsLoadedByLdr(void) {return fwsinf.Flags & sfLoadedByLdr;}  // If loaded by loader then should return to the loader   // OnWindows, Any DLL that loaded by loader
static _finline bool   IsDynamicLib(void) {return fwsinf.Flags & sfDynamicLib;}
//------------------------------------------------------------------------------------------------------------
static _finline vptr   GetMainBase(void) {return fwsinf.MainModBase;}
static _finline size_t GetMainSize(void) {return fwsinf.MainModSize;}
static _finline vptr   GetModuleBase(void){return fwsinf.ModBase;}
static _finline size_t GetModuleSize(void){return fwsinf.ModSize;}
//------------------------------------------------------------------------------------------------------------
static sint GetMainPath(achar* DstBuf, size_t BufSize=uint(-1)) 
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static _finline sint GetModulePath(achar* DstBuf, size_t BufSize=size_t(-1))
{
 sint aoffs = 0;
 return (size_t)GetCLArg(aoffs, DstBuf, BufSize);       // TODO TODO TODO !!!

// Search in loader list first

// Search by memory mapping

 return 0;
}
//------------------------------------------------------------------------------------------------------------
static vptr LoadLibrary(const achar* Path, bool Init=true)
{
 static vptr paddr = nullptr;
 if(!paddr)
  {
   paddr = NPE::GetProcAddrSafe(fwsinf.pNtDll, "LdrLoadDll"); 
   if(!paddr)return nullptr;
  }   
 return NTX::LdrLoadLibrary(Path, paddr);
}
//------------------------------------------------------------------------------------------------------------
static sint InitStartupInfo(vptr StkFrame=nullptr, vptr ArgA=nullptr, vptr ArgB=nullptr, vptr ArgC=nullptr)
{
 DBGDBG("StkFrame=%p, ArgA=%p, ArgB=%p, ArgC=%p",StkFrame,ArgA,ArgB,ArgC);
 vptr AddrInTheMod = (vptr)&InitStartupInfo;
 fwsinf.ModBase = NTX::LdrGetModuleByAddr(AddrInTheMod, &fwsinf.ModSize);
 if(!fwsinf.ModBase)   // Not present in the loader list (Loaded by other means)
  {
   // TODO: Find by VirtuaQuery
  }
   else fwsinf.Flags |= sfLoadedByLdr;      // Normal processes`s entry point is called by some loader stub  // TODO: Support native process creation then there will be no such stub
 if((ArgA != NT::NtCurrentPeb())&&(((size_t)ArgA & ~(MEMGRANSIZE-1)) == ((size_t)fwsinf.ModBase & ~(MEMGRANSIZE-1))))fwsinf.Flags |= sfDynamicLib;   // System passes PEB as first argument to EXE`s entry point then it is safe to exit from entry point without calling 'exit'
 fwsinf.pNtDll = NTX::GetBaseOfNtdll();

 fwsinf.hKeyedEvent = nullptr;
 if(!SAPI::NtWaitForAlertByThreadId.IsValid())SAPI::NtCreateKeyedEvent((NT::HANDLE*)&fwsinf.hKeyedEvent, -1, nullptr, 0); 

 UpdateTZOffsUTC();
 fwsinf.MemPageSize = MEMPAGESIZE;
 fwsinf.MemGranSize = MEMGRANSIZE;
 AtomicOr(&fwsinf.Flags, sfInitialized);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-seterrormode
// Windows 7:
// System default is to display all error dialog boxes.
// SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX
// GetErrorMode: NtQueryInformationProcess, 4
// SetErrorMode: NtSetInformationProcess, 4
//
static sint SetErrorHandlers(void)
{

 return 0;
}
//------------------------------------------------------------------------------------------------------------
static void DbgLogStartupInfo(void)
{
 uint  alen = 0;
 sint  aoff = 0;
 LOGDBG("CArguments: ");
 for(uint idx=0;aoff >= 0;idx++)
  {
   const syschar* val = NPTM::SkipCLArg(aoff, &alen);
   LOGDBG("  Arg %u: %.*ls",idx,alen,val);     // NOTE: Terminal should support UTF8
  }
 LOGDBG("EVariables: ");
 aoff = 0;
 for(uint idx=0;aoff >= 0;idx++)
  {
   const syschar* val = NPTM::GetEnvVar(aoff, &alen);
   LOGDBG("  Var %u: %ls",idx,val);     // NOTE: Terminal should support UTF8
  }
 DBGDBG("Done!");
}
//------------------------------------------------------------------------------------------------------------
