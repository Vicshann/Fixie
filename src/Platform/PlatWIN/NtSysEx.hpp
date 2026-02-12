
#pragma once

//============================================================================================================
template<typename PHT=PTRCURRENT> struct NTSYX: public NTSYS<PHT>    // NUFmtPE
{
// See KUSER_SHARED_DATA in ntddk.h
static _finline uint8* GetUserSharedData(void)
{
 return reinterpret_cast<uint8*>(0x7FFE0000);
}
//---------------------------------------------------------------------------
static _finline bool IsWinXPOrOlder(void)
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return (bool)*(uint32*)pKiUserSharedData;    // Deprecated since 5.2(XP x64) and equals 0
}
//---------------------------------------------------------------------------
static _finline uint64 GetTicks(void)
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return (*(uint16*)pKiUserSharedData)?((uint64)*(uint32*)pKiUserSharedData):(*(uint64*)&pKiUserSharedData[0x320]);
}
//----------------------------------------------------------------------------
static _finline uint64 GetTicksCount(void)
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return (GetTicks() * *(uint32*)&pKiUserSharedData[4]) >> 24;  // 'TickCountLow * TickCountMultiplier' or 'TickCount * TickCountMultiplier'
}
//----------------------------------------------------------------------------
// The interrupt time reported by QueryInterruptTime is based on the latest tick of the system clock timer. 
// The system clock timer is the hardware timer that periodically generates interrupts for the system clock. 
// The uniform period between system clock timer interrupts is referred to as a system clock tick, and is typically in the range of 0.5 milliseconds to 15.625 milliseconds
// 
// Expiration times are measured relative to the system clock, and the accuracy with which the operating system can detect when a timer expires is limited by the granularity of the system clock.
// Affected by NtSetTimerResolution
//
static _finline uint64 GetInterruptTime(void)   // FILETIME
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return *(uint64*)&pKiUserSharedData[0x0008];
}
//----------------------------------------------------------------------------
static _finline uint64 GetSystemTime(void)  // FILETIME
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return *(uint64*)&pKiUserSharedData[0x0014];
}
//----------------------------------------------------------------------------
static _finline uint64 GetTimeZoneBias(void)  // FILETIME
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return *(uint64*)&pKiUserSharedData[0x0020];
}
//----------------------------------------------------------------------------
static _finline uint64 GetLocalTime(void)  // FILETIME
{
 return GetSystemTime() - GetTimeZoneBias();
}
//----------------------------------------------------------------------------
// _ReadBarrier       // Forces memory reads to complete
// _WriteBarrier      // Forces memory writes to complete
// _ReadWriteBarrier  // Block the optimization of reads and writes to global memory
//
/*static inline volatile UINT64 GetAbstractTimeStamp(UINT64 volatile* PrevVal)
{
 volatile UINT64 cval = __rdtsc();
 volatile UINT64 pval = *PrevVal;   // Interlocked.Read
 if(cval <= pval)return pval;       // Sync to increment
 _InterlockedCompareExchange64((INT64*)PrevVal,cval, pval);  // Assign atomically if it is not changed yet  // This is the only one 64bit operand LOCK instruction available on x32 (cmpxchg8b)
 return *PrevVal;   // Return a latest value
} */
//----------------------------------------------------------------------------
static NT::NTSTATUS NtSleep(uint32 dwMiliseconds, bool Alertable=false)
{
 struct
  {
   uint32 Low;
   uint32 Hi;
  } MLI = {{uint32((uint32)-10000 * dwMiliseconds)},{uint32(0xFFFFFFFFu)}};    // Relative time used
 return SAPI::NtDelayExecution(Alertable, (NT::PLARGE_INTEGER)&MLI);
}
//----------------------------------------------------------------------------
// Will fallback to NtQueryPerformanceCounter
// Freq is in Hz (Ticks/Secs) 
//
static NT::NTSTATUS QueryPerformanceCounter(NT::PLARGE_INTEGER PerformanceCounter, NT::PLARGE_INTEGER PerformanceFrequency)
{
 return 0;   // TODO
}
//----------------------------------------------------------------------------
static inline uint64 GetNativeWowTebAddrWin10(void)     // !!! Probably unstable
{
 NT::PTEB teb = NT::NtCurrentTeb();
 if(!teb->WowTebOffset)return 0;  // Not WOW or below Win10  // From 10.0 and higher
 uint8* TebX64 = (uint8*)teb;
 if(long(teb->WowTebOffset) < 0)
  {
   TebX64 = &TebX64[(long)teb->WowTebOffset];  // In WOW processes WowTebOffset is negative in x32 TEB and positive in x64 TEB
   if(*(uint64*)&TebX64[0x30] != (uint64)TebX64)return 0;   // x64 Self
  }
   else if((size_t)(*(uint32*)&TebX64[0x18]) != (size_t)TebX64)return 0;   // x32 Self
 return uint64(TebX64);
}
//---------------------------------------------------------------------------
// Returns 'false' if running under native x32 or native x64
static inline bool IsWow64(void)
{
 if constexpr(!IsArchX64)return NT::NtCurrentTeb()->WOW32Reserved;   // Is it reliable?  Is it always non NULL under Wow64?   // Contains pointer to 'jmp far 33:Addr'
 return false;
}
//---------------------------------------------------------------------------
// http://waleedassar.blogspot.com/2012/12/skipthreadattach.html
//
/*static int SetSkipThreadReport(HANDLE ThLocalThread)
{
 THREAD_BASIC_INFORMATION tinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationThread(ThLocalThread,ThreadBasicInformation,&tinf,sizeof(THREAD_BASIC_INFORMATION),&RetLen);
 if(res)return -1;
 return SetSkipThreadReport(tinf.TebBaseAddress);
}
//---------------------------------------------------------------------------
static int SetSkipThreadReport(PTEB ThTeb)  // See RtlIsCurrentThreadAttachExempt
{
 PBYTE TebAddr = (PBYTE)ThTeb;
 for(int idx=0;idx < 2;idx++)
  {
   bool IsTgtTebX32 = (*(PBYTE*)&TebAddr[0x18] == TebAddr);  // NT_TIB
   bool IsTgtTebX64 = (*(PBYTE*)&TebAddr[0x30] == TebAddr);  // NT_TIB
   if(!IsTgtTebX32 && !IsTgtTebX64)return -2;

   long WowTebOffset;
   USHORT* pSameTebFlags;
   if(IsTgtTebX32)
    {
     WowTebOffset  = *(long*)&TebAddr[0x0FDC];
     pSameTebFlags = (USHORT*)&TebAddr[0x0FCA];
    }
     else
      {
       WowTebOffset  = *(long*)&TebAddr[0x180C];
       pSameTebFlags = (USHORT*)&TebAddr[0x17EE];
      }
   *pSameTebFlags |= 0x0008;   // SkipThreadAttach
   *pSameTebFlags &= ~0x0020;  // RanProcessInit
   DBGMSG("TEB=%p, pSameTebFlags=%p, IsTgtTebX32=%u, IsTgtTebX64=%u, WowTebOffset=%i", TebAddr, pSameTebFlags, IsTgtTebX32, IsTgtTebX64, WowTebOffset);
   if(WowTebOffset == 0)break;
   TebAddr = &TebAddr[WowTebOffset];  // Next WOW TEB
  }
 return 0;
} */
//------------------------------------------------------------------------------------
static bool LdrDisableThreadDllCalls(vptr DllBase)
{
 NT::PEB* CurPeb = NT::NtCurrentPeb();
 NT::PEB_LDR_DATA* ldr = CurPeb->Ldr;
 for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(DllBase == me->DllBase)
    {
     me->u.Flags |= NT::LDRP_DONT_CALL_FOR_THREADS;   // .DontCallForThreads
     return true;
    }
  }
 return false;
}
//------------------------------------------------------------------------------------
// Returns base of NTDLL and optionally its path (can be used to get the system drive letter)
//
static vptr GetBaseOfNtdll(NT::UNICODE_STRING** FullDllName=nullptr, NT::UNICODE_STRING** BaseDllName=nullptr)
{
 NT::PEB* CurPeb = NT::NtCurrentPeb();
 uint8* AddrInNtDll = (uint8*)CurPeb->FastPebLock;    // Stable?
 NT::PEB_LDR_DATA* ldr = CurPeb->Ldr; //  CurTeb->ProcessEnvironmentBlock->Ldr;
 for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   uint8* DllBeg = (uint8*)me->DllBase;
   uint8* DllEnd = DllBeg + me->SizeOfImage;
   if((AddrInNtDll >= DllBeg)&&(AddrInNtDll < DllEnd))
    {
     if(FullDllName)*FullDllName = &me->FullDllName;
     if(BaseDllName)*BaseDllName = &me->BaseDllName;
     return me->DllBase;
    }
  }
 return nullptr;
}
//---------------------------------------------------------------------------
static vptr LdrGetExeBase(size_t* Size=nullptr, uint32* BaseIdx=nullptr)
{
 vptr BaseAddr = NT::NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;
 if(Size)
  {
   NT::PPEB_LDR_DATA ldr = NT::NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
   for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
    {
     if(BaseAddr == me->DllBase){*Size = me->SizeOfImage; break;}
    }
  }
 return BaseAddr;
}
//---------------------------------------------------------------------------
template<typename T> static vptr LdrGetModuleBase(T ModName, size_t* Size=nullptr, uint32* BaseIdx=nullptr)  // NOTE: No loader locking used!
{
 NT::PPEB_LDR_DATA ldr = NT::NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
// DBGMSG("PEB_LDR_DATA: %p, %s",ldr,ModName);     // TODO: Use only in FULL info mode
 long StartIdx = (BaseIdx)?(*BaseIdx + 1):0;
 long CurrIdx  = 0;
 for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;CurrIdx++,me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(!me->BaseDllName.Length || !me->BaseDllName.Buffer)continue;
   if(CurrIdx < StartIdx)continue;
//   DBGMSG("Base=%p, Name='%ls'",me->DllBase,me->BaseDllName.Buffer);    // Zero terminated?     // Spam
   bool Match = true;
   uint ctr = 0;
   for(uint tot=me->BaseDllName.Length/sizeof(wchar);ctr < tot;ctr++)
    {
     if(me->BaseDllName.Buffer[ctr] != (wchar)ModName[ctr]){Match=false; break;}     // Any actual wide chars?
    }
   if(Match && !ModName[ctr])
    {
     if(BaseIdx)*BaseIdx = CurrIdx;
     if(Size)*Size = me->SizeOfImage;
     return me->DllBase;
    }
  }
// DBGMSG("Not found for: %s",ModName);   // TODO: Use only in FULL info mode
 return nullptr;
}
//---------------------------------------------------------------------------
static vptr LdrGetModuleBase(uint32 NameHash, size_t* Size=nullptr, uint32* BaseIdx=nullptr)  // NOTE: No loader locking used!  // NOTE: NameHash of a low case string (ANSI)
{
 NT::PPEB_LDR_DATA ldr = NT::NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
// DBGMSG("PEB_LDR_DATA: %p, %s",ldr,ModName);     // TODO: Use only in FULL info mode
 long StartIdx = (BaseIdx)?(*BaseIdx + 1):0;
 long CurrIdx  = 0;
 for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;CurrIdx++,me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(!me->BaseDllName.Length || !me->BaseDllName.Buffer)continue;
   if(CurrIdx < StartIdx)continue;
//   DBGMSG("Base=%p, Name='%ls'",me->DllBase,me->BaseDllName.Buffer);    // Zero terminated?     // Spam
   uint32 crc = ~NCRYPT::InitialCrc32;                   
   for(uint32 i=0,val=0;(val=uint32(me->BaseDllName.Buffer[i]));i++)crc = NCRYPT::ByteCrc32(NSTR::CharToLoASCII((achar)val),crc,NCRYPT::DefRevPolyCrc32);
   if(NameHash == ~crc)
    {
     if(BaseIdx)*BaseIdx = CurrIdx;
     if(Size)*Size = me->SizeOfImage;
     return me->DllBase;
    }
  }
// DBGMSG("Not found for: %s",ModName);   // TODO: Use only in FULL info mode
 return nullptr;
}
//---------------------------------------------------------------------------
static vptr LdrGetModuleByAddr(vptr ModAddr, size_t* ModSize=nullptr)
{
 NT::PPEB_LDR_DATA ldr = NT::NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;    // TODO: Loader lock
 for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(((uint8*)ModAddr < (uint8*)me->DllBase) || ((uint8*)ModAddr >= ((uint8*)me->DllBase + me->SizeOfImage)))continue;
   if(ModSize)*ModSize = me->SizeOfImage;
   return me->DllBase;
  }
 return nullptr;
}
//---------------------------------------------------------------------------
static vptr LdrLoadLibrary(const achar* LibName, vptr pLdrLoadDll)
{
 wchar NamBuf[PATH_MAX];
 NT::UNICODE_STRING DllName; // we will use this to hold the information for the LdrLoadDll call
 vptr ModBase = nullptr;
 int ctr = 0;
 for(;LibName[ctr] && (ctr < (sizeof(NamBuf)/2));ctr++)NamBuf[ctr] = LibName[ctr];
 NamBuf[ctr] = 0;
 DllName.Buffer = (wchar*)&NamBuf; // the dll path must be the .Buffer -> you can always just do = L"path" instead of passing a param for it
 DllName.Length = (ctr * sizeof(wchar)); // calc the length
 DllName.MaximumLength = (DllName.Length + sizeof(wchar)); // max length calc
 if(((decltype(NT::LdrLoadDll)*)pLdrLoadDll)(nullptr, nullptr, &DllName, (vptr*)&ModBase))return nullptr;   // Can load EXE but won't call its entry point
 return ModBase;
}
//---------------------------------------------------------------------------
// "\\GLOBAL??\\A:"    // LOCAL: "\\??"  (Network drives are mapped there)
//
static uint32 GetObjectNS(wchar* buf, bool Global)
{
 return NSTR::StrCopy(buf, Global?_PS(L"\\GLOBAL??\\"):_PS(L"\\??\\"));
}
//---------------------------------------------------------------------------
static sint32 IsGlobalObjectNS(const wchar* buf)
{
 if(buf[0] != '\\')return -1;
 if(buf[1] == '?')
  {         
   if(buf[2] != '?')return -2;
   if(buf[3] != '\\')return -3;
   return 0;    // LOCAL
  }
 if(buf[1] == 'G')
  {
   if(buf[7] != '?')return -4;
   if(buf[8] != '?')return -5;
   if(buf[9] != '\\')return -6;
   return 1;    // GLOBAL
  }
 return -9;
}
//---------------------------------------------------------------------------
// https://github.com/MartinDrab/VrtuleTree/blob/master/vtdrv/utils-devices.c
// /Device/HardDisk... to C:
// DstLen in bytes
//
static NT::NTSTATUS FindDosDevForPath(wchar* DosPart, const wchar* NtPath, uint32 DstLen, uint32* OldPLen=nullptr, uint32* NewPLen=nullptr, bool Global=true)   
{
 NT::HANDLE hODir = 0;
 NT::UNICODE_STRING path;
 NT::OBJECT_ATTRIBUTES oattr;
 wchar PathLnk[64];
 uint32 onslen = GetObjectNS(PathLnk, Global);
 path.Set(PathLnk, onslen-1);  
 oattr.Length = sizeof(NT::OBJECT_ATTRIBUTES);
 oattr.RootDirectory = 0;
 oattr.ObjectName = &path;
 oattr.Attributes = 0;   //NT::OBJ_INHERIT;            // OBJ_CASE_INSENSITIVE;  
 oattr.SecurityDescriptor = nullptr;           
 oattr.SecurityQualityOfService = nullptr;
 NT::NTSTATUS res = SAPI::NtOpenDirectoryObject(&hODir, NT::DIRECTORY_QUERY | NT::DIRECTORY_TRAVERSE, &oattr);
 if(res)return res;
 sint32 MupPxLen  = NSTR::MatchCI(NtPath, _PS(L"\\Device\\Mup\\"));   // \Device\LanmanRedirector\;Z:0000000000063507\192.168.32.50\workfolder
 sint32 mpoffs = 0;     // offset of \workfolder
 bool IsMupNtPath = (MupPxLen == 12);
 if(IsMupNtPath)
  {
   NtPath += 11;
   mpoffs  = NSTR::CharOffsetCS(NtPath, PATHSEPWIN, 1);      // mpoffs is OldPLen
   if(mpoffs <= 0)IsMupNtPath = false;
  }
 
 const size_t BufLen = 4096;   // Only 10 recs fits in 2K!
 uint32 PrvIdx = 0;
 uint32 RecIdx = 0;
 uint32 RetLen = 0; 
 alignas(16) uint8  RecBuf[BufLen];
 usize PathByteLen = NSTR::StrLen(NtPath) << 1;
 NT::UNICODE_STRING* ObjPath = nullptr;
 oattr.Attributes = NT::OBJ_CASE_INSENSITIVE;
 PathLnk[onslen+1] = ':';
 path.Set(PathLnk, onslen+2);   // Full path  

 NT::UNICODE_STRING lnkpath;
 wchar pathbuf[PATH_MAX];
 lnkpath.Buffer = pathbuf;
 lnkpath.Length = 0;
 lnkpath.MaximumLength = sizeof(pathbuf);
 for(;;)
  {
   PrvIdx = RecIdx;
   NT::NTSTATUS stat = SAPI::NtQueryDirectoryObject(hODir, &RecBuf, sizeof(RecBuf), false, false, &RecIdx, &RetLen);   // STATUS_MORE_ENTRIES
   if(!NT::IsSuccess(stat)){SAPI::NtClose(hODir); return stat;}
   auto Recs = (NT::OBJECT_DIRECTORY_INFORMATION*)RecBuf;
   for(uint32 idx=0,tot=RecIdx-PrvIdx;idx < tot;idx++)       // TypeName: SymbolicLink  // Only SymLinks in \\GLOBAL??
    {
     NT::OBJECT_DIRECTORY_INFORMATION* rec = &Recs[idx];
     if(!rec->Name.Buffer || (rec->Name.Length < 4))continue;   // C:
     if(rec->Name.Buffer[1] != ':')continue;
     path.Buffer[onslen] = rec->Name.Buffer[0];   // Drive letter
     NT::HANDLE hSObj = 0;
     res = SAPI::NtOpenSymbolicLinkObject(&hSObj, NT::SYMBOLIC_LINK_QUERY, &oattr);
     if(!NT::IsSuccess(res))continue;
     lnkpath.Length = 0; 
     res = SAPI::NtQuerySymbolicLinkObject(hSObj, &lnkpath, &RetLen);
     SAPI::NtClose(hSObj);
     if(!NT::IsSuccess(res))continue;

     ssize clen;
     bool Match;
     if(IsMupNtPath && NSTR::MatchCI(lnkpath.Buffer, _PS(L"\\Device\\LanmanRedirector\\")) == 25)
      {
       wchar* lptr  = lnkpath.Buffer + 25;
       sint32 moffs = NSTR::StrOffset(lptr, NtPath, 0, usize(-1), mpoffs);    // Offset of IP part ( \\192.168.32.50\\ )
       if(moffs < 0)continue;
       uint32 fblen = (moffs + 25) << 1;
       uint32 brem  = (lnkpath.Length - fblen);
       if(brem >= PathByteLen)continue;
       lptr += moffs;
       uint32 crem  = brem >> 1;
       ssize mlen = NSTR::MatchCI(NtPath, lptr, crem, crem); 
       Match = (mlen == crem); 
       clen  = (MupPxLen + mlen) - 1;  //(MupPxLen + mpoffs) - 1;
      }
       else
        {
         if(lnkpath.Length >= PathByteLen)continue;
               clen = lnkpath.Length >> 1; 
         ssize mlen = NSTR::MatchCI(NtPath, lnkpath.Buffer, clen, clen); 
         Match = (mlen == clen); 
        }
     if(Match)
      {
       uint32 len = NSTR::StrCopy(DosPart, rec->Name.Buffer, DstLen >> 1);
       SAPI::NtClose(hODir);
       if(NewPLen)*NewPLen = len;    // New prefix size
       if(OldPLen)*OldPLen = clen;   // Old prefix size
       return NT::STATUS_SUCCESS;
      }
    }
   if(ObjPath || (NT::STATUS_MORE_ENTRIES != stat))break;
  }
 SAPI::NtClose(hODir);
 return NT::STATUS_NOT_FOUND;
}
//---------------------------------------------------------------------------
// C: to /Device/HardDisk... 
// \Device\LanmanRedirector\;Z:0000000000063507\192.168.32.50\workfolder
//
static NT::NTSTATUS FindPathForDosDev(wchar* NtPart, const wchar* DosPath, uint32 DstLen, uint32* OldPLen=nullptr, uint32* NewPLen=nullptr, bool Global=true)   
{
 sint32 plen = NSTR::CharOffsetCS(DosPath, PATHSEPWIN); 
 if(plen <= 0)return NT::STATUS_OBJECT_PATH_INVALID;
 uint32 RetLen = 0;
 NT::HANDLE hSObj = 0;
 NT::UNICODE_STRING path;
 NT::OBJECT_ATTRIBUTES oattr;
 wchar PathLnk[PATH_MAX];
 uint32 onslen = GetObjectNS(PathLnk, Global);
 if((onslen+plen+1) > (sizeof(PathLnk)/2))return NT::STATUS_NAME_TOO_LONG;
 uint32 oplen = NSTR::StrCopy(&PathLnk[onslen], DosPath, plen+1);
 path.Set(PathLnk, onslen+oplen); 
 oattr.Length = sizeof(NT::OBJECT_ATTRIBUTES);
 oattr.RootDirectory = 0;
 oattr.ObjectName = &path;
 oattr.Attributes = 0;   //NT::OBJ_INHERIT;            // OBJ_CASE_INSENSITIVE;  
 oattr.SecurityDescriptor = nullptr;           
 oattr.SecurityQualityOfService = nullptr;
 NT::NTSTATUS res = SAPI::NtOpenSymbolicLinkObject(&hSObj, NT::SYMBOLIC_LINK_QUERY, &oattr);
 if(res)return res;

 NT::UNICODE_STRING lnkpath;
 wchar pathbuf[PATH_MAX];
 lnkpath.Buffer = pathbuf;
 lnkpath.Length = 0;
 lnkpath.MaximumLength = sizeof(pathbuf);
 res = SAPI::NtQuerySymbolicLinkObject(hSObj, &lnkpath, &RetLen);
 SAPI::NtClose(hSObj);
 if(!NT::IsSuccess(res))return res;
 uint32 dlen = NSTR::StrCopy(NtPart, lnkpath.Buffer, DstLen >> 1);
 if(NewPLen)*NewPLen = dlen;
 if(OldPLen)*OldPLen = plen;
 return NT::STATUS_SUCCESS;
}
//---------------------------------------------------------------------------
static NT::NTSTATUS GetDosDevForPath(wchar* DosPart, const wchar* NtPath, uint32 DstLen, uint32* OldPLen=nullptr, uint32* NewPLen=nullptr)    // Nt path to DOS path
{
 NT::NTSTATUS stat = FindDosDevForPath(DosPart, NtPath, DstLen, OldPLen, NewPLen, false);    // Try local first - less entries to process
 if(!stat)return stat;
 return FindDosDevForPath(DosPart, NtPath, DstLen, OldPLen, NewPLen, true);   
}
//---------------------------------------------------------------------------
static NT::NTSTATUS GetPathForDosDev(wchar* NtPart, const wchar* DosPath, uint32 DstLen, uint32* OldPLen=nullptr, uint32* NewPLen=nullptr)    // Nt path to DOS path
{
 NT::NTSTATUS stat = FindPathForDosDev(NtPart, DosPath, DstLen, OldPLen, NewPLen, true);    // Try global first - more likely
 if(!stat)return stat;
 return FindPathForDosDev(NtPart, DosPath, DstLen, OldPLen, NewPLen, false);   
}
//---------------------------------------------------------------------------
// NOTE: It is unspecified if this path is in GLOBAL or LOCAL namespace (Have to try such paths twice)
//
static NT::UNICODE_STRING* GetCurrentDir(void)       // 'C:\xxxxx\yyyyy\'
{
 auto pars = NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters; 
 if(!pars)return nullptr;   // The process params may be missing (Created by an owner process)
 return &pars->CurrentDirectory.DosPath; 
}
//---------------------------------------------------------------------------
// Sets the path and handle as is
static NT::NTSTATUS SetCurrentDirNT(NT::HANDLE hDir, const wchar* sDir, usize PathLen=0)  // Sets the CD directly   
{
 static wchar* OwnPtr = nullptr;     // NOTE: Other libs will allocate this on Heap (And will try to free it from heap)
 auto pars = NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters; 
 if(!pars)return NT::STATUS_INVALID_ADDRESS;  // No ProcessParameters!     // NT::STATUS_NO_SUCH_FILE
 if(!PathLen)PathLen = NSTR::StrLen(sDir);

 usize ByteLen = (PathLen<<1) + 2;
 NT::UNICODE_STRING* CurrDir = &pars->CurrentDirectory.DosPath;
 if(CurrDir->MaximumLength < ByteLen)   // Alloc/Realloc the block
  {
   vptr  RegionBase = nullptr;   
   usize RegionSize = ByteLen;      // + null size
   NT::NTSTATUS res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_COMMIT, NT::PAGE_READWRITE); 
   if(res){SAPI::NtClose(hDir); return res;}    // ENOMEM
   if(OwnPtr == CurrDir->Buffer)    // Otherwise leak the initial, heap allocated buffer memory (May be allocated by SetCurrentDirectory)
    {
     vptr  RBase = OwnPtr;
     usize RSize = 0;      // Free the entire region by MEM_RELEASE
     SAPI::NtFreeVirtualMemory(NT::NtCurrentProcess, &RBase, &RSize, NT::MEM_RELEASE); 
    }
   usize len = NSTR::StrCopy((wchar*)RegionBase, sDir);        // NOTE: No path validation    // TODO: ENOENT if The file does not exist
   CurrDir->Buffer = OwnPtr = (wchar*)RegionBase;    
   CurrDir->Length = len << 1;    // not including the terminating NULL character
   CurrDir->MaximumLength = RegionSize;
  }
   else CurrDir->Length = NSTR::StrCopy(CurrDir->Buffer, sDir) << 1;    // not including the terminating NULL character

 SAPI::NtClose(pars->CurrentDirectory.Handle);    // Close the prev handle
 pars->CurrentDirectory.Handle = hDir;
 return NT::STATUS_SUCCESS;
} 
//---------------------------------------------------------------------------
static NT::NTSTATUS SetCurrentDir(const achar* sDir, bool CaseSens=false) 
{
 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::IO_STATUS_BLOCK iosb = {};
 NT::UNICODE_STRING FilePathUS;
 NT::HANDLE hDir = 0;

 uint32 CreateOptions  = NT::FILE_DIRECTORY_FILE | NT::FILE_SYNCHRONOUS_IO_NONALERT;
 uint32 CreateDisposition = NT::FILE_OPEN;
 uint32 ShareAccess    = NT::FILE_SHARE_READ | NT::FILE_SHARE_WRITE;
 uint32 FileAttributes = NT::FILE_ATTRIBUTE_NORMAL;
 uint32 DesiredAccess  = uint32(NT::FILE_TRAVERSE) | NT::SYNCHRONIZE;
 uint32 ObjAttributes  = NT::OBJ_INHERIT | NT::OBJ_PATH_PARSE_DOTS;
 if(!CaseSens)oattr.Attributes |= NT::OBJ_CASE_INSENSITIVE;

 uint plen;
 NTX::EPathType ptype = ptUnknown;
 uint PathLen = CalcFilePathBufSize(sDir, plen, ptype);
 wchar FullPath[PathLen+2];    // NOTE: VLA
 InitFileObjectAttributes(sDir, plen, ptype, ObjAttributes, FullPath, &FilePathUS, &oattr);   // Normalizes the path  // Try GLOBAL first (reserves path space for LOCAL)
 NT::NTSTATUS res = SAPI::NtCreateFile(&hDir, DesiredAccess, &oattr, &iosb, nullptr, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, nullptr, 0);   
 int gls = IsGlobalObjectNS(FullPath);
 if((res == NT::STATUS_OBJECT_PATH_NOT_FOUND) && (gls > 0))   // Try to open in local object namespace
  {
   FilePathUS.Length -= (6*2);
   FilePathUS.Buffer += 6;            // GLOBAL to LOCAL
   *FilePathUS.Buffer = PATHSEPWIN;
   res = SAPI::NtCreateFile(&hDir, DesiredAccess, &oattr, &iosb, nullptr, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, nullptr, 0);    // This will open network shares (mapped drives)
   gls = 0;
  }
 if(!NT::IsSuccess(res))return res;
 if(gls > 0){FilePathUS.Buffer += 10; FilePathUS.Length -= (10*2);}
  else if(!gls){FilePathUS.Buffer += 4; FilePathUS.Length -= (4*2);}             
 return SetCurrentDirNT(hDir, FilePathUS.Buffer, FilePathUS.Length >> 1);
} 
//---------------------------------------------------------------------------
// NtQueryObject: For network shares returns: \Device\Mup\192.168.32.50\workfolder
//  In LOCAL namespace, mapped drive Z: is a link to \Device\LanmanRedirector\;Z:0000000000063507\192.168.32.50\workfolder
//  \Device\LanmanRedirector is a link to \Device\Mup\;LanmanRedirector
//
static NT::NTSTATUS SetCurrentDir(NT::HANDLE hDir) 
{
 wchar* FullPath = nullptr;
 uint32 PathLen  = 0;   
 struct SFPath: NT::UNICODE_STRING
  {
   wchar RestOfBuf[PATH_MAX];
  } PathBuf;
 PathBuf.Length = PathBuf.MaximumLength = 0;
 NT::NTSTATUS res = SAPI::NtQueryObject(hDir, NT::ObjectNameInformation, &PathBuf, sizeof(PathBuf), &PathLen);  
 if(res)
  {
   if(!PathLen)return res;
   usize size = AlignFrwdP2(PathLen,16);
   FullPath = (wchar*)StkAlloc(size);
   res = SAPI::NtQueryObject(hDir, NT::ObjectNameInformation, FullPath, size, &PathLen);
   if(!NT::IsSuccess(res))return res;
  }
   else FullPath = (wchar*)&PathBuf;

 NT::UNICODE_STRING* str = (NT::UNICODE_STRING*)FullPath;
 uint32 OldPLen = 0;
 uint32 NewPLen = 0;
 wchar DriveLnk[64];
 res = GetDosDevForPath(DriveLnk, str->Buffer, sizeof(DriveLnk), &OldPLen, &NewPLen);     
 if(!NT::IsSuccess(res))return res;
 if(NewPLen > OldPLen)return NT::STATUS_UNSUCCESSFUL;

 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::IO_STATUS_BLOCK iosb = {};
 NT::UNICODE_STRING FilePathUS;
 oattr.Length = sizeof(NT::OBJECT_ATTRIBUTES);
 oattr.RootDirectory = 0;
 oattr.ObjectName = str;
 oattr.Attributes = NT::OBJ_INHERIT;            // OBJ_CASE_INSENSITIVE;   //| OBJ_KERNEL_HANDLE;
 oattr.SecurityDescriptor = nullptr;            // TODO: Arg3: mode_t mode
 oattr.SecurityQualityOfService = nullptr;
 res = SAPI::NtCreateFile(&hDir, uint32(NT::FILE_TRAVERSE) | NT::SYNCHRONIZE, &oattr, &iosb, nullptr, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ | NT::FILE_SHARE_WRITE, NT::FILE_OPEN, NT::FILE_DIRECTORY_FILE | NT::FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);   
 if(!NT::IsSuccess(res))return res;

 uint32 plen = OldPLen - NewPLen;
 str->Buffer += plen;
 str->Length -= plen << 1;
 memcpy(str->Buffer, &DriveLnk, NewPLen << 1);  
 return SetCurrentDirNT(hDir, str->Buffer, str->Length >> 1);             // Case sensitive?
} 
//---------------------------------------------------------------------------
enum EPathType {ptUnknown=0,ptAbsolute=1,ptSysRootRel=2,ptCurrDirRel=3,ptWinUsrLevel=0x00010000,ptWinUsrAtRel=0x00010001, ptFlgMask=0x000FFFFF};  // May be ORed with OBJ_PATH_GLOBAL_NS and OBJ_PATH_PARSE_DOTS

template<typename T> static EPathType GetPathTypeNt(T Src)
{
 if((Src[0] == ':') || (Src[1] == ':'))return ptAbsolute;   // 'C:\'    // First ':' is a hack to open object SymLinks directly
 if((Src[0] == '/') || (Src[0] == '\\'))return ptSysRootRel;  // \MyHomeFolder      // Drop that flag if we are at \\Device\\
 //if((Src[0] == '.') && ((Src[1] == '/') || (Src[1] == '\\')))return ptCurrDirRel;    // .\MyDrkDir
 return ptCurrDirRel;  // ptUnknown;    // CurrDir must be added and any '.' and '..' will be handled later if needed for UNIX compatibility
}
//------------------------------------------------------------------------------------
static _minline size_t CalcFilePathBufSize(const achar* Path, uint& plen, EPathType& ptype)  // Returns required buffer size ib wchar
{
 bool Extra = (((uint32)ptype & ptFlgMask) < ptWinUsrLevel); 
 plen = NUTF::Len8To16(Path);  //  NSTR::StrLen(Path);      // TODO: Check that it is fast enough
 if(!(uint16)ptype)ptype = EPathType((uint32)NTX::GetPathTypeNt(Path) | (uint32)ptype);  // If ptUnknown
 uint ExtraLen = Extra?(4+10):0;  // +10 for size of "\\GLOBAL??\\"
 NT::UNICODE_STRING* CurrDir = GetCurrentDir();    
 if((uint16)ptype == NTX::ptSysRootRel)ExtraLen += NSTR::StrLen((wchar*)&fwsinf.SysDrive);
 else if(((uint16)ptype == NTX::ptCurrDirRel) && CurrDir)ExtraLen += CurrDir->Length / sizeof(wchar);
 return plen + ExtraLen;    // (plen*4)
}
//------------------------------------------------------------------------------------
static _minline size_t InitFilePathBuf(const achar* Path, uint plen, EPathType ptype, wchar* buf_path, const wchar* root_path=nullptr)
{
 size_t DstLen, POffs;
 if(root_path)
  {
   DstLen = NSTR::StrCopy(buf_path, root_path);
   if(!IsFilePathSep(buf_path[DstLen-1]))buf_path[DstLen++] = '\\';
   POffs = 0;
  }
   else 
    {
     DstLen = (((uint32)ptype & ptFlgMask) < ptWinUsrLevel)?(NSTR::StrCopy(buf_path, (ptype & +NT::OBJ_PATH_GLOBAL_NS)?_PS(L"\\GLOBAL??\\"):_PS(L"\\??\\"))):0;     // Windows XP?
     POffs  = DstLen;
    }
 if(*Path == ':'){Path++;plen--;}     // To open symlinks directly
 NT::UNICODE_STRING* CurrDir = GetCurrentDir(); 
 if((uint16)ptype == NTX::ptSysRootRel)DstLen += NSTR::StrCopy(&buf_path[DstLen], (wchar*)&fwsinf.SysDrive);        // Add system drive path
 else if(((uint16)ptype == NTX::ptCurrDirRel) && CurrDir)DstLen += NSTR::StrCopy(&buf_path[DstLen], (wchar*)CurrDir->Buffer);    // Add path to current directory
 DstLen += NUTF::Utf8To16(&buf_path[DstLen], Path, plen);
 if(buf_path[POffs+1] == ':')POffs += 2;  // Preserve 'DRV:'
 buf_path[DstLen] = 0;
 DstLen  = NormalizePathNt(&buf_path[POffs], &buf_path[POffs], !(ptype & +NT::OBJ_PATH_PARSE_DOTS)) + POffs;  // TODO: Check that we cannot step back from GLOBAL?? root     // TODO: Support of '.' and '..' must be optional
 buf_path[DstLen] = 0;
// DBGMSG("Path: %ls",buf_path);
 return DstLen;    // Actual size, Not including NULL
}
//------------------------------------------------------------------------------------
// Volume symlinks are \GLOBAL??\C:
// https://stackoverflow.com/questions/14192887/status-invalid-parameter-from-ntcreatefile
//
static _minline void InitFileObjectAttributes(const achar* Path, uint plen, EPathType ptype, uint32 ObjAttributes, wchar* buf_path, NT::UNICODE_STRING* buf_ustr, NT::OBJECT_ATTRIBUTES* oattr, NT::HANDLE RootDir=0, const wchar* RootPath=nullptr)
{
 ptype = EPathType(ptype | (ObjAttributes & NT::OBJ_EXTRA_MASK));
 size_t DstLen = InitFilePathBuf(Path, plen, ptype, buf_path, RootPath);
 if(buf_ustr)
  {
   if(IsFilePathSep(*buf_path))RootDir = 0;     // Absolute paths start with a slash and incompatible with RootDir (Report STATUS_INVALID_PARAMETER) - let it be STATUS_OBJECT_PATH_NOT_FOUND
   buf_ustr->Set(buf_path, DstLen);
   if(oattr)
    {
     oattr->Length = sizeof(NT::OBJECT_ATTRIBUTES);
     oattr->RootDirectory = RootDir;
     oattr->ObjectName = buf_ustr;
     oattr->Attributes = ObjAttributes & NT::OBJ_VALID_ATTRIBUTES;            // OBJ_CASE_INSENSITIVE;   //| OBJ_KERNEL_HANDLE;
     oattr->SecurityDescriptor = nullptr;          // TODO: Arg3: mode_t mode
     oattr->SecurityQualityOfService = nullptr;
    }
  }
}
//------------------------------------------------------------------------------------
// Used to replace a directory handle with its path
// A variable size array's space is freed at the end of the scope of the name of the array. The space allocated with `alloca' remains until the end of the function.
// NOTE: No returns (except on the end) in case we will have to turn this into a macro
// NT::NTSTATUS stat = SAPI::NtQueryInformationFile(RootDir, &iost, &RootPathBuf, sizeof(RootPathBuf), NT::FileNameInformation);
// https://stackoverflow.com/questions/24751387/can-i-comment-multi-line-macros
//
#define AllocaObjectPath(hObj, Path)   \
{                                                   \
 uint32 DataLen = AlignFrwdP2(sizeof(NT::UNICODE_STRING)+PATH_MAX,16);   \
 NT::UNICODE_STRING* ObjPathBuf = (NT::UNICODE_STRING*)StkAlloc(DataLen);   /* NOTE: This allocation may be wasted */ \
  /*ObjPathBuf->Length = ObjPathBuf->MaximumLength = 0;   // Just in case */ \
 NT::NTSTATUS stat = SAPI::NtQueryObject(hObj, NT::ObjectNameInformation, ObjPathBuf, DataLen, &DataLen);  /* Format: \Device\HarddiskVolume2\    // STATUS_OBJECT_PATH_INVALID or STATUS_BUFFER_OVERFLOW and required len in RetLen */  \
 if(DataLen && stat)      /* Need more memory (Always STATUS_BUFFER_OVERFLOW ?). DataLen was updated */ \
  {                    \
   DataLen    = AlignFrwdP2(DataLen,16); \
   ObjPathBuf = (NT::UNICODE_STRING*)StkAlloc(DataLen); \
  /*ObjPathBuf->Length = ObjPathBuf->MaximumLength = 0;   // Just in case */ \
   stat = SAPI::NtQueryObject(hObj, NT::ObjectNameInformation, ObjPathBuf, DataLen, &DataLen); \
  }  \
 if(DataLen && !stat)Path = ObjPathBuf; \
}
//------------------------------------------------------------------------------------
// NOTE: RootDir is nullified for absolute paths
//
#define AllocaObjectAttrs(sPath, hRootDir, uAttrs, pObjAttr, ForceRel)  \
{    \
 uint plen;      \
 NTX::EPathType ptype = (hRootDir||ForceRel)?ptWinUsrAtRel:ptUnknown;    \
 uint PathLen = CalcFilePathBufSize(sPath, plen, ptype);  \
 wchar* RootPath = nullptr;  \
 if((uAttrs & NT::OBJ_PATH_PARSE_DOTS) && hRootDir && !IsFilePathSep(*sPath) && IsStepBackOutPath(sPath))      /* Need to expand root path to process steps back because there is no actual '..' links as on Linux */ \
  {            \
   NT::UNICODE_STRING* ObjPath = nullptr;   \
   AllocaObjectPath(hRootDir, ObjPath);     \
   if(ObjPath)                              \
    {                                       \
     hRootDir = 0;                          \
     PathLen += ObjPath->Length >> 1;       \
     RootPath = ObjPath->Buffer;            \
    }                                       \
  }                                         \
 usize  Offs   = AlignFrwdP2(sizeof(NT::UNICODE_STRING)+32,16);  /* Should fit some info structs which use wchar[1] instead of UNICODE_STRING */  \
 usize  BufLen = AlignFrwdP2(Offs+(PathLen << 1)+4,16);  \
 uint8* Buffer = (uint8*)StkAlloc(BufLen);    \
 InitFileObjectAttributes(sPath, plen, ptype, uAttrs, (wchar*)&Buffer[Offs], (NT::UNICODE_STRING*)Buffer, pObjAttr, hRootDir, RootPath); \
}
//------------------------------------------------------------------------------------
static void _finline ONSGlobalToLocalPtr(auto& Ptr, auto& Len)
{
 Len -= (6*2);
 Ptr += 6;            // GLOBAL to LOCAL
 *Ptr = PATHSEPWIN;
}
//------------------------------------------------------------------------------------
// On NT systems '.' and '..' can be a file/dir name and NTX::OpenFileObject should support that 
//  but NtQueryDirectoryFile returns '.' and '..' as first entries, except for a drive's root
//
static NT::NTSTATUS OpenFileObject(NT::PHANDLE FileHandle, const achar* Path, NT::ACCESS_MASK DesiredAccess, NT::ULONG ObjAttributes, NT::ULONG FileAttributes, NT::ULONG ShareAccess, NT::ULONG CreateDisposition, NT::ULONG CreateOptions, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::HANDLE RootDir=0)
{
 NT::OBJECT_ATTRIBUTES oattr;  // = {};
 ObjAttributes |= NT::OBJ_PATH_GLOBAL_NS;  // Try the global first
 AllocaObjectAttrs(Path, RootDir, ObjAttributes, &oattr, false)
 NT::NTSTATUS res = SAPI::NtCreateFile(FileHandle, DesiredAccess, &oattr, IoStatusBlock, nullptr, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, nullptr, 0);   
 if((res == NT::STATUS_OBJECT_PATH_NOT_FOUND) && (IsGlobalObjectNS(oattr.ObjectName->Buffer) > 0))   // Try to open in local object namespace
  {
   ONSGlobalToLocalPtr(oattr.ObjectName->Buffer, oattr.ObjectName->Length);
   res = SAPI::NtCreateFile(FileHandle, DesiredAccess, &oattr, IoStatusBlock, nullptr, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, nullptr, 0);    // This will open network shares (mapped drives)
  }
 return res;
}
//------------------------------------------------------------------------------------
/*
 https://stackoverflow.com/questions/46473507/delete-folder-for-which-every-api-call-fails-with-error-access-denied

To delete a file is enough 2 things - we have FILE_DELETE_CHILD on parent folder. and file is not read only.
In this case call ZwDeleteFile (but not DeleteFile or RemoveDirectory - both this api will fail if file have empty DACL) is enough.
In case file have read-only attribute - ZwDeleteFile fail with code STATUS_CANNOT_DELETE. in this case we need first remove read only.
For this we need to open the file with FILE_WRITE_ATTRIBUTES access. we can do this if we have SE_RESTORE_PRIVILEGE and set FILE_OPEN_FOR_BACKUP_INTENT option in call to ZwOpenFile.

A file in the file system is basically a link to an inode.
A hard link, then, just creates another file with a link to the same underlying inode.
When you delete a file, it removes one link to the underlying inode. The inode is only deleted (or deletable/over-writable) when all links to the inode have been deleted.

Once a hard link has been made the link is to the inode. Deleting, renaming, or moving the original file will not affect the hard link as it links to the underlying inode.
Any changes to the data on the inode is reflected in all files that refer to that inode.

https://superuser.com/questions/343074/directory-junction-vs-directory-symbolic-link

Default behaviour:
 NtDeleteFile("C:/TST/TestDirJunction");       // Reparse point          // Deleted target directory itself, not the link!   // "mklink  /J"       // Marked by Explorer
 NtDeleteFile("C:/TST/TestDirSoftLink");       // Reparse point          // Deleted target directory itself, not the link!   // "mklink  /D"       // Marked by Explorer
 NtDeleteFile("C:/TST/TestFileSoftLink");      // Reparse point          // Deleted target file itself, not the link!        // "mklink "  // Marked by Explorer but size is not correctly displayed (0)
 NtDeleteFile("C:/TST/TestFileHardLink.ini");  // A Link to same INODE   // Deleted the link, not the target file!    // "mklink  /H" (Works as on Linux)   // Not marked by Explorer

NOTE: Linux does not allows hardlinking directories

IoFileObjectType           // NtDeleteFile (CreateOptions = FILE_DELETE_ON_CLOSE, DeleteOnly)  // IoFileObjectType: InvalidAttributes is OBJ_PERMANENT | OBJ_EXCLUSIVE | OBJ_OPENLINK
ObpDirectoryObjectType
ObpSymbolicLinkObjectType

NtOpenSymbolicLinkObject
 NtDeleteFile is much simpler and faster but it is not used by DeleteFileW (Because it just follows a file obect`s name by any link?)
 NOTE: OBJ_DONT_REPARSE disables following symlinks, including those on Object Directory (Like 'C:')
       !!! Resolving C: to actual file path is not cheap!

if you have only one handle which was open with FILE_DELETE_ON_CLOSE (let's call it H1), once it is closed the stream will be deleted. 
If you have at least one more handle (H2), then once H1 is closed you can use H2 to query and reset the delete disposition which will then prevent the stream from being deleted.

https://devblogs.microsoft.com/oldnewthing/20160108-00/?p=92821

 https://stackoverflow.com/questions/3593768/is-there-a-way-to-remove-file-flag-delete-on-close

https://github.com/dokan-dev/dokany/issues/883

https://stackoverflow.com/questions/3764072/c-win32-how-to-wait-for-a-pending-delete-to-complete

https://stackoverflow.com/questions/52687370/could-dropbox-interfere-with-deletefile-rename   // FILE_DISPOSITION_POSIX_SEMANTICS  (From windows 10 rs1)

 FILE_FLAG_DELETE_ON_CLOSE requires FILE_SHARE_DELETE permission  (CreateFile(FILE_SHARE_DELETE) will fail if someone holds a handle without FILE_SHARE_DELETE)

 MOVEFILE_DELAY_UNTIL_REBOOT

 NOTE: NtDeleteFile will return STATU_SSUCCESS after attempt to remove a nonempty directory (The directory stays)

 NOTE: NtOpenFile may modify last access time

 NOTE: STATUS_NOT_A_REPARSE_POINT is never reported when opening any file/directory with FILE_OPEN_REPARSE_POINT
        Looks like STATUS_NOT_A_REPARSE_POINT is for NtFsControlFile(FSCTL_GET_REPARSE_POINT) only

 NtDeleteFile:      // ObOpenObjectByName(DELETE)
    CreateOptions = FILE_DELETE_ON_CLOSE;
    ShareAccess = (USHORT) FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    Disposition = FILE_OPEN;
    DeleteOnly = TRUE;               (KERNEL, special - forces deref)
    openPacket.TraversedMountPoint = FALSE; (KERNEL)

 Is ObOpenObjectByName faster with traversing reparse points or with direct paths?????????????????????

 status = NtFsControlFile (h, NULL, NULL, NULL, &io, FSCTL_GET_REPARSE_POINT, NULL, 0, (LPVOID) rp, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

 Conclusion: NtDeleteFile is useless here

 if the application is using FILE_DISPOSITION_INFO with the DeleteFile member set to TRUE, the file would need DELETE access
  FILE_DELETE_ON_CLOSE requires DELETE permission too.

  // STATUS_OBJECT_NAME_NOT_FOUND   or   STATUS_OBJECT_PATH_NOT_FOUND

using FILE_DISPOSITION_INFORMATION rather than using the FILE_DELETE_ON_CLOSE flag. it is better as it handles the directory-not-empty case for free and it's complementary to the use of FILE_DISPOSITION_INFORMATION_EX on newer platforms.

// STATUS_SHARING_VIOLATION  --  if there are open handles without SHARE_DELETE   ( OpenFileObject )  // Or STATUS_LOCK_NOT_GRANTED on network shares
// STATUS_DELETE_PENDING  -- Consider it as success
// STATUS_CANNOT_DELETE  -- The file is ReadOnly (or there is an existing mapped view to the file)

https://learn.microsoft.com/en-us/windows/win32/fileio/hard-links-and-junctions 
if you clear the read-only attribute flag on a particular hard link so you can delete that hard link, and there are multiple hard links to the file, the other hard links display that the read-only attribute is still set, which isn't true. 
To change the file back to the read-only state, you must set the read-only flag on the file from one of its remaining hard links.
*/


// Working on Windows XP
// Working with network shares
// Prefers POSIX delete behaviour   (Microsoft STL does that too)
// AsDir: <0:File/Dir; 0:File; >0:Dir
// CaseSens: When need POSIX compatibility
// KeepDots: Need in special cases when need to delete a file/dir named as '.' or '..'
//
static NT::NTSTATUS DeleteFileObject(const achar* Path, int AsDir=-1, NT::HANDLE RootDir=0, bool CaseSens=false, bool KeepDots=false)    
{
 NT::IO_STATUS_BLOCK iosb = {}; 
 NT::HANDLE hFileObj = 0; 
 NT::NTSTATUS res = 0; 
                        
// NOTE: Without FILE_OPEN_REPARSE_POINT the target File/Dir will be removed. There will be no STATUS_NOT_A_REPARSE_POINT error if the object is not a reparse point.
 uint32 CreateOptions  = NT::FILE_OPEN_REPARSE_POINT|NT::FILE_SYNCHRONOUS_IO_NONALERT;     // Opens any file object (files/directories)     // NT::FILE_DELETE_ON_CLOSE| - Makes OpenFileObject fail if the file is ReadOnly
 uint32 ShareAccess    = NT::FILE_SHARE_VALID_FLAGS;   // NT::FILE_SHARE_READ | NT::FILE_SHARE_WRITE | NT::FILE_SHARE_DELETE
 uint32 DesiredAccess  = +NT::DELETE | NT::FILE_READ_ATTRIBUTES | NT::FILE_WRITE_ATTRIBUTES | NT::SYNCHRONIZE;        
 uint32 ObjAttributes  = 0;  // NT::OBJ_INHERIT;
 if(!KeepDots)ObjAttributes |= NT::OBJ_PATH_PARSE_DOTS;
 if(!CaseSens)ObjAttributes |= NT::OBJ_CASE_INSENSITIVE; 
 if(AsDir >= 0)                                              // Most anoying feature of IDEs? - "copy entire line"
  {
   if(AsDir > 0)CreateOptions |= NT::FILE_DIRECTORY_FILE;   // Reports STATUS_NOT_A_DIRECTORY on files
    else CreateOptions |= NT::FILE_NON_DIRECTORY_FILE;      // Reports STATUS_FILE_IS_A_DIRECTORY on directories
  }

 for(;;){       // Use loop break instead of GOTO                   
 res = OpenFileObject(&hFileObj, Path, DesiredAccess, ObjAttributes, NT::FILE_ATTRIBUTE_NORMAL, ShareAccess, NT::FILE_OPEN, CreateOptions, &iosb, RootDir);
 if(res == NT::STATUS_DELETE_PENDING)return NT::STATUS_PENDING;  // Already deleting  // Return a positive to pass NT_SUCCESS() 
 if(res == NT::STATUS_SHARING_VIOLATION)return res;     // Failed to open  // Nothing can be done about that?
 if(!NT::IsSuccess(res))return res;  
           
 NT::FILE_DISPOSITION_INFORMATION fdi = {NT::FILE_DISPOSITION_DELETE | NT::FILE_DISPOSITION_POSIX_SEMANTICS | NT::FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE};  // this can't get around the shared-access mode, and most applications do not open files with delete sharing.
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, &fdi, sizeof(fdi), NT::FileDispositionInformationEx);
 if(res == NT::STATUS_DIRECTORY_NOT_EMPTY)break;
 if(NT::IsSuccess(res))break;     
   
// Probably unsupported (WinVer is too low?) - trying old approach
 fdi.DeleteFile = true;  // puts the file into a "delete pending" state. It will be deleted once all existing handles are closed. No new handles will be possible to open
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, &fdi, sizeof(fdi), NT::FileDispositionInformation);
 if(NT::IsSuccess(res) || (res != NT::STATUS_CANNOT_DELETE))break;      // If Success or not ReadOnly

 NT::FILE_BASIC_INFORMATION fBasicInfo;
 res = SAPI::NtQueryInformationFile(hFileObj, &iosb, &fBasicInfo, sizeof(fBasicInfo), NT::FileBasicInformation);    // STATUS_ACCESS_DENIED if no access to attrs
 if(!NT::IsSuccess(res))break;   
 if(!(fBasicInfo.FileAttributes & NT::FILE_ATTRIBUTE_READONLY))break;    // Probably mapped somewhere
 //fBasicInfo.FileAttributes = NT::FILE_ATTRIBUTE_NORMAL;    // FILE_ATTRIBUTE_NORMAL flag cannot be set or returned in combination with any other attributes.
 fBasicInfo.FileAttributes &= ~NT::FILE_ATTRIBUTE_READONLY;
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, &fBasicInfo, sizeof(fBasicInfo), NT::FileBasicInformation);
 if(!NT::IsSuccess(res))break; 
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, &fdi, sizeof(fdi), NT::FileDispositionInformation);   // Try again
  
 //if(res == NT::STATUS_CANNOT_DELETE){}   // The file is ReadOnly or mapped     // Nothing can be done here?
 break;}   // End the loop

 SAPI::NtClose(hFileObj);
 return res;
}
//------------------------------------------------------------------------------------
// Check cross-device before touching anything.  Otherwise we might end up with an unlinked target dir even if the actual rename didn't work. (?)
// 	 - DELETE is required to rename a file.  
//	 - At least one cifs FS (Tru64) needs FILE_READ_ATTRIBUTE, otherwise the FileRenameInformation call fails with STATUS_ACCESS_DENIED.  However, on NFS we get a STATUS_ACCESS_DENIED if FILE_READ_ATTRIBUTE is used and the file we try to rename is a symlink.  Urgh.
//	 - Samba (only some versions?) doesn't like the FILE_SHARE_DELETE mode if the file has the R/O attribute set and returns STATUS_ACCESS_DENIED in that case. 
// 
// Some virus scanners check newly generated files and while doing that disallow DELETE access. That's really bad because it breaks applications which copy files by creating a 
//   temporary filename and then rename the temp filename to the target filename.
// Renaming a dir to another, existing dir fails always, even if ReplaceIfExists is set to TRUE and the existing dir is empty.
// You can't copy a file if the destination exists and has the R/O attribute set.
// Do not forget: we cannot move files across mount points
// 
// NOTE: Target RootDir is not needed for inplace renaming. Also not needed if target path is fully qualified. If used, the target file name must be a simple file name.  
// NOTE: A file or directory can only be renamed within a volume. In other words, a rename operation cannot cause a file or directory to be moved to a different volume. 
// TODO: Check that it does not rename a file to a dir   (AsDir should be useless)
// ????: Will replace action replace a symlink or its target?
// 
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/87f86c9b-6c2a-4803-84b7-131a74a434fa
//
static NT::NTSTATUS RenameFileObject(const achar* SrcFile, const achar* DstFile, NT::HANDLE SrcRootDir=0, NT::HANDLE DstRootDir=0, bool Replace=false, bool CaseSens=false, bool KeepDots=false) 
{
 NT::OBJECT_ATTRIBUTES oattr;  // = {  };
 NT::IO_STATUS_BLOCK iosb = {}; 
 NT::HANDLE hFileObj = 0; 
 NT::NTSTATUS res = 0; 
 usize DstLen = NSTR::StrLen(DstFile);
                        
// NOTE: Without FILE_OPEN_REPARSE_POINT the target File/Dir will be renamed. There will be no STATUS_NOT_A_REPARSE_POINT error if the object is not a reparse point.
 uint32 CreateOptions  = NT::FILE_OPEN_REPARSE_POINT|NT::FILE_SYNCHRONOUS_IO_NONALERT;     // Opens any file object (files/directories)   
 uint32 ShareAccess    = NT::FILE_SHARE_VALID_FLAGS;   // NT::FILE_SHARE_READ | NT::FILE_SHARE_WRITE | NT::FILE_SHARE_DELETE
 uint32 DesiredAccess  = +NT::DELETE | NT::FILE_READ_ATTRIBUTES | NT::FILE_WRITE_ATTRIBUTES | NT::SYNCHRONIZE;    // DELETE access is required for renaming    
 uint32 ObjAttributes  = 0;  // NT::OBJ_INHERIT;
 if(!KeepDots)ObjAttributes |= NT::OBJ_PATH_PARSE_DOTS;
 if(!CaseSens)ObjAttributes |= NT::OBJ_CASE_INSENSITIVE; 
                  
 res = OpenFileObject(&hFileObj, SrcFile, DesiredAccess, ObjAttributes, NT::FILE_ATTRIBUTE_NORMAL, ShareAccess, NT::FILE_OPEN, CreateOptions, &iosb, SrcRootDir);
 if(!NT::IsSuccess(res))return res;
 bool  ForceRel = !IsSepOnPath(DstFile);
 AllocaObjectAttrs(DstFile, DstRootDir, ObjAttributes|NT::OBJ_PATH_GLOBAL_NS, &oattr, ForceRel)
 usize  PathLen = oattr.ObjectName->Length;   // In bytes
 wchar* PathPtr = oattr.ObjectName->Buffer;

 for(;;){       // Use loop break instead of GOTO                      // Should not report STATUS_OBJECT_NAME_NOT_FOUND because the DstFile does not contain s path
 usize InfoSize = PathLen + sizeof(NT::FILE_RENAME_INFORMATION);  //  AlignFrwdP2(DstBLen+sizeof(NT::FILE_RENAME_INFORMATION),16);       
 NT::FILE_RENAME_INFORMATION* RenameInfo = (NT::FILE_RENAME_INFORMATION*)((uint8*)PathPtr - (sizeof(NT::FILE_RENAME_INFORMATION)-4));     //    StkAlloc(InfoSize);
 RenameInfo->Flags          = NT::FILE_RENAME_POSIX_SEMANTICS | NT::FILE_RENAME_IGNORE_READONLY_ATTRIBUTE;
 RenameInfo->RootDirectory  = DstRootDir;    // Relative paths like 'xxx/222/v.7z' are accepted (Tested Win10)
 RenameInfo->FileNameLength = PathLen;
 if(Replace)RenameInfo->Flags |= NT::FILE_RENAME_REPLACE_IF_EXISTS;
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, RenameInfo, InfoSize, NT::FileRenameInformationEx);
 if(NT::IsSuccess(res))break;          // TODO: Break only on a specific error code when the FileRenameInformationEx is not supported. // NOTE: FS driver decides how to respond if it does not support FileRenameInformationEx
 if((res = NT::STATUS_OBJECT_PATH_NOT_FOUND) && (IsGlobalObjectNS(RenameInfo->FileName) > 0))
  {
   ONSGlobalToLocalPtr(PathPtr, PathLen);
   continue;
  }      
 if(res == NT::STATUS_ACCESS_DENIED)break;
 if(res == NT::STATUS_DELETE_PENDING)break;  
 if(res == NT::STATUS_NOT_SAME_DEVICE)break;
 if(res == NT::STATUS_OBJECT_NAME_INVALID)break;
 if(res == NT::STATUS_OBJECT_NAME_COLLISION)break;   // Replace is false
 if(res == NT::STATUS_NOT_A_DIRECTORY)break;         // Tried to replace rename a directory to a file
 if(res == NT::STATUS_FILE_IS_A_DIRECTORY)break;     // Tried to replace rename a file to a directory  
// if(res == NT::STATUS_INVALID_PARAMETER)break;       // On a network mapped drive (SMB does not support FileRenameInformationEx?)  (Or if DstRootDir is not 0 and DstFile starts with a slash)               
 RenameInfo->ReplaceIfExists &= NT::FILE_RENAME_REPLACE_IF_EXISTS;    // Replace ? true : false;
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, RenameInfo, InfoSize, NT::FileRenameInformation);       

 break;}   // End the loop
 SAPI::NtClose(hFileObj);
 return res;
}
//------------------------------------------------------------------------------------
// Attempts to read from a file ReparsePoint will result in STATUS_END_OF_FILE
// Attempts to read from a directory ReparsePoint will result in STATUS_INVALID_DEVICE_REQUEST
// Attempts to read from a file HardLink will read its target's contents
// 
// https://stackoverflow.com/questions/10260676/programmatically-finding-the-target-of-a-windows-hard-link
// 
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/46021e52-29b1-475c-b6d3-fe5497d23277
// 
// https://gist.githubusercontent.com/juntalis/4a90ee024732b88539a65d92d0b6a296/raw/1dd9c4aa0374ebf7e1edcad6c2574cc9f72bb97d/findlinks.c
// https://helgeklein.com/blog/hard-links-soft-symbolic-links-and-junctions-in-ntfs-what-are-they-for/
// https://googleprojectzero.blogspot.com/2015/12/between-rock-and-hard-link.html
// 
// https://github.com/googleprojectzero/symboliclink-testing-tools/blob/main/CommonUtils/Hardlink.cpp
// 
// There is no difference between the 'original' file and the file created with a hardlink. They are simply directory entries to the same file. And givena file with multiple links, 
//  there's no way to find all the links without searching the file system and comparing the file id of each file.
// 
// 'readlinkat' reads symlinks created by 'symlinkat'. 'linkat' creates hard links
// 
// Conclusion: No reading hard links!
// 
//
static NT::NTSTATUS ReadFileObjectSLink(achar* DstBuf, usize* DstLen, const achar* FilePath, NT::HANDLE RootDir=0, bool CaseSens=false, bool KeepDots=false)
{
 uint8 BigBuf[4096];
 NT::OBJECT_ATTRIBUTES oattr;  // = {};
 NT::IO_STATUS_BLOCK iosb = {}; 
 NT::HANDLE hFileObj = 0; 
 NT::NTSTATUS res = 0;
 vptr BufPtr = BigBuf;
 bool AllocatedBuf = false; 
                        
 uint32 CreateOptions  = NT::FILE_OPEN_REPARSE_POINT|NT::FILE_SYNCHRONOUS_IO_NONALERT;     // Opens any file object (files/directories)   
 uint32 ShareAccess    = NT::FILE_SHARE_VALID_FLAGS;   // NT::FILE_SHARE_READ | NT::FILE_SHARE_WRITE | NT::FILE_SHARE_DELETE
 uint32 DesiredAccess  = +NT::FILE_READ_DATA | NT::FILE_READ_ATTRIBUTES | NT::SYNCHRONIZE;    // DELETE access is required for renaming    
 uint32 ObjAttributes  = 0;  // NT::OBJ_INHERIT;
 if(!KeepDots)ObjAttributes |= NT::OBJ_PATH_PARSE_DOTS;
 if(!CaseSens)ObjAttributes |= NT::OBJ_CASE_INSENSITIVE; 
                  
 res = OpenFileObject(&hFileObj, FilePath, DesiredAccess, ObjAttributes, NT::FILE_ATTRIBUTE_NORMAL, ShareAccess, NT::FILE_OPEN, CreateOptions, &iosb, RootDir);
 if(!NT::IsSuccess(res))return res;

 for(;;){
 res = SAPI::NtFsControlFile(hFileObj, 0, nullptr, nullptr, &iosb, NT::FSCTL_GET_REPARSE_POINT, nullptr, 0, &BigBuf, sizeof(BigBuf));  // Returns STATUS_BUFFER_OVERFLOW if buffer is too small (Does not report required size)
 if((res == NT::STATUS_BUFFER_OVERFLOW) && !AllocatedBuf)
  {
   usize Size = 0x10000;   // Network shares have problems with buffers larger than 64K
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &BufPtr, 0, &Size, NT::MEM_COMMIT, NT::PAGE_READWRITE); 
   if(!NT::IsSuccess(res))return res;
   AllocatedBuf = true;
   continue;
  }
 if(!NT::IsSuccess(res))break;
 NT::REPARSE_DATA_BUFFER* rpd = (NT::REPARSE_DATA_BUFFER*)BufPtr;
 wchar* Buf;
 usize  Len;   // In chars
 bool   Rel = false;   // The Target is relative to hFileObj   // Probably directories only
 if(rpd->ReparseTag == NT::IO_REPARSE_TAG_SYMLINK)     // Flags?  // SYMLINK_FILE, SYMLINK_DIRECTORY, SYMLINK_FLAG_RELATIVE
  {
   Buf = &rpd->SymbolicLinkReparseBuffer.PathBuffer[rpd->SymbolicLinkReparseBuffer.SubstituteNameOffset >> 1];
   Len = rpd->SymbolicLinkReparseBuffer.SubstituteNameLength >> 1;
   Rel = (rpd->SymbolicLinkReparseBuffer.Flags & NT::SYMLINK_FLAG_RELATIVE);
  }
 else if(rpd->ReparseTag == NT::IO_REPARSE_TAG_MOUNT_POINT)   // NOTE: May be different rules of the path normalization   // \??\C:\TEST\DIR\TESTDIR
  {
   Buf = &rpd->MountPointReparseBuffer.PathBuffer[rpd->SymbolicLinkReparseBuffer.SubstituteNameOffset >> 1];
   Len = rpd->MountPointReparseBuffer.SubstituteNameLength >> 1;
  }
 else {res = NT::STATUS_INVALID_INFO_CLASS; break;}  

 size_t nlen = NUTF::Utf16To8((achar*)BufPtr, Buf, Len);   // Will be smaller     // FileNameLength is in bytes   // TODO: Path normalization?   // WARNING: Reusing the same buffer
// if(Rel) ::: READ hFileObj PATH :::     // TODO:
 *DstLen = NSTR::StrCopy(DstBuf, (achar*)BufPtr, Min(nlen+1, *DstLen));  // \??\C:\TEST\DIR\TESTDIR\TESTFILE.txt       // TODO: Normalize
 break;
 }
 SAPI::NtClose(hFileObj);
 if(AllocatedBuf){ usize RSize=0; SAPI::NtFreeVirtualMemory(NT::NtCurrentProcess, &BufPtr, &RSize, NT::MEM_RELEASE); }
 return res;
} 
//------------------------------------------------------------------------------------
// LnkFollow: AT_SYMLINK_FOLLOW - to cause oldpath to be dereferenced if it is a symbolic link
// DosDevices is SymLink to \??\
// https://learn.microsoft.com/en-us/windows/win32/termserv/kernel-object-namespaces
// There is a global namespace used primarily by services in client/server applications. In addition, each session has a separate namespace for these objects.
//
static NT::NTSTATUS CreateFileObjectSLink(const achar* LnkFile, const achar* TgtFile, NT::HANDLE LnkRootDir=0, NT::HANDLE TgtRootDir=0, bool Relative=false, bool LnkFollow=false, bool CaseSens=false, bool KeepDots=false)
{
 NT::OBJECT_ATTRIBUTES oattr;  // = {};
 NT::IO_STATUS_BLOCK iosb = {}; 
 NT::HANDLE hFileObj = 0; 
 NT::NTSTATUS res = 0;
 usize TPathLen = NSTR::StrLen(LnkFile);
 usize LPathLen = NSTR::StrLen(LnkFile);

 uint32 CreateOptions  = NT::FILE_SYNCHRONOUS_IO_NONALERT;     // Opens any file object (files/directories)   
 uint32 ShareAccess    = NT::FILE_SHARE_VALID_FLAGS;   // NT::FILE_SHARE_READ | NT::FILE_SHARE_WRITE | NT::FILE_SHARE_DELETE
 uint32 DesiredAccess  = +NT::FILE_WRITE_ATTRIBUTES | NT::FILE_READ_ATTRIBUTES | NT::SYNCHRONIZE;    // DELETE access is required for renaming    
 uint32 ObjAttributes  = 0;  // NT::OBJ_INHERIT;
 if(!KeepDots)ObjAttributes  |= NT::OBJ_PATH_PARSE_DOTS;
 if(!CaseSens)ObjAttributes  |= NT::OBJ_CASE_INSENSITIVE; 
 if(!LnkFollow)CreateOptions |= NT::FILE_OPEN_REPARSE_POINT;
                  
 res = OpenFileObject(&hFileObj, TgtFile, DesiredAccess, ObjAttributes, NT::FILE_ATTRIBUTE_NORMAL, ShareAccess, NT::FILE_OPEN, CreateOptions, &iosb, TgtRootDir);
 if(!NT::IsSuccess(res))return res;

 usize FullLen = PATH_MAX*2;
 // TODO: Convert the hFileObj path to DOS path TgtFile   // How to preserve it without reading by a syscall?
 // TODO: Add "\\??\\%s" to absolute paths
   /* https://github.com/neosmart/ln-win/blob/master/ln-win/JunctionPoint.cpp
        size_t size = sizeof(REPARSE_DATA_BUFFER) - sizeof(TCHAR) + nativeTarget.GetLength() * sizeof(TCHAR);
        auto_ptr<REPARSE_DATA_BUFFER> reparseBuffer((REPARSE_DATA_BUFFER*) new unsigned char[size]);

        //Fill the reparse buffer
        reparseBuffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        reparseBuffer->Reserved = NULL;

        reparseBuffer->MountPointReparseBuffer.SubstituteNameOffset = 0;
        reparseBuffer->MountPointReparseBuffer.SubstituteNameLength = nativeTarget.GetLength() * (int) sizeof(TCHAR);

        //No substitute name, point it outside the bounds of the string
        reparseBuffer->MountPointReparseBuffer.PrintNameOffset = reparseBuffer->MountPointReparseBuffer.SubstituteNameLength + (int) sizeof(TCHAR);
        reparseBuffer->MountPointReparseBuffer.PrintNameLength = 0;

        //Copy the actual string
        //_tcscpy(reparseBuffer->MountPointReparseBuffer.PathBuffer, nativeTarget);
        memcpy(reparseBuffer->MountPointReparseBuffer.PathBuffer, (LPCTSTR) nativeTarget, reparseBuffer->MountPointReparseBuffer.SubstituteNameLength);

        //Set ReparseDataLength to the size of the MountPointReparseBuffer
        //Kind in mind that with the padding for the union (given that SymbolicLinkReparseBuffer is larger),
        //this is NOT equal to sizeof(MountPointReparseBuffer)
        reparseBuffer->ReparseDataLength = sizeof(REPARSE_DATA_BUFFER) - REPARSE_DATA_BUFFER_HEADER_SIZE - sizeof(TCHAR) + reparseBuffer->MountPointReparseBuffer.SubstituteNameLength;

        //Create the junction directory
        CreateDirectory(junction, NULL);

        //Set the reparse point
        SafeHandle hDir;
        hDir.Handle = CreateFile(junction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);

        DeviceIoControl(hDir.Handle, FSCTL_SET_REPARSE_POINT, reparseBuffer.get(), (unsigned int) size, NULL, 0, &bytesReturned, NULL))
       */
 SAPI::NtClose(hFileObj);
 return res;
} 
//------------------------------------------------------------------------------------
// NOTE: Untested
//
static NT::NTSTATUS CreateFileObjectHLink(const achar* LnkFile, const achar* TgtFile, NT::HANDLE LnkRootDir=0, NT::HANDLE TgtRootDir=0, bool Replace=false, bool LnkFollow=false, bool CaseSens=false, bool KeepDots=false)
{
 NT::OBJECT_ATTRIBUTES oattr;  // = {};
 NT::IO_STATUS_BLOCK iosb = {}; 
 NT::HANDLE hFileObj = 0; 
 NT::NTSTATUS res = 0;
                        
 uint32 CreateOptions  = NT::FILE_SYNCHRONOUS_IO_NONALERT;     // Opens any file object (files/directories)   
 uint32 ShareAccess    = NT::FILE_SHARE_VALID_FLAGS;   // NT::FILE_SHARE_READ | NT::FILE_SHARE_WRITE | NT::FILE_SHARE_DELETE
 uint32 DesiredAccess  = +NT::FILE_WRITE_ATTRIBUTES | NT::SYNCHRONIZE;    // DELETE access is required for renaming    
 uint32 ObjAttributes  = 0;  // NT::OBJ_INHERIT;
 if(!KeepDots)ObjAttributes |= NT::OBJ_PATH_PARSE_DOTS;
 if(!CaseSens)ObjAttributes |= NT::OBJ_CASE_INSENSITIVE; 
 if(!LnkFollow)CreateOptions |= NT::FILE_OPEN_REPARSE_POINT;
                  
 res = OpenFileObject(&hFileObj, TgtFile, DesiredAccess, ObjAttributes, NT::FILE_ATTRIBUTE_NORMAL, ShareAccess, NT::FILE_OPEN, CreateOptions, &iosb, TgtRootDir);
 if(!NT::IsSuccess(res))return res;

 bool  ForceRel = !IsSepOnPath(LnkFile);
 AllocaObjectAttrs(LnkFile, LnkRootDir, ObjAttributes|NT::OBJ_PATH_GLOBAL_NS, &oattr, ForceRel)
 usize  PathLen = oattr.ObjectName->Length;   // In bytes
 wchar* PathPtr = oattr.ObjectName->Buffer;

 for(;;){       // Use loop break instead of GOTO                      // Should not report STATUS_OBJECT_NAME_NOT_FOUND because the DstFile does not contain s path
 usize InfoSize = PathLen + sizeof(NT::FILE_LINK_INFORMATION);  //  AlignFrwdP2(DstBLen+sizeof(NT::FILE_LINK_INFORMATION),16);       
 NT::FILE_LINK_INFORMATION* HLinkInfo = (NT::FILE_LINK_INFORMATION*)((uint8*)PathPtr - (sizeof(NT::FILE_LINK_INFORMATION)-4));     //    StkAlloc(InfoSize);
 HLinkInfo->Flags          = NT::FILE_LINK_POSIX_SEMANTICS | NT::FILE_LINK_IGNORE_READONLY_ATTRIBUTE;
 HLinkInfo->RootDirectory  = LnkRootDir;    // Relative paths like 'xxx/222/v.7z' are accepted (Tested Win10)
 HLinkInfo->FileNameLength = PathLen;
 if(Replace)HLinkInfo->Flags |= NT::FILE_LINK_REPLACE_IF_EXISTS;
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, HLinkInfo, InfoSize, NT::FileLinkInformationEx);
 if(NT::IsSuccess(res))break;          // TODO: Break only on a specific error code when the FilLinkInformationEx is not supported. // NOTE: FS driver decides how to respond if it does not support FileRenameInformationEx
 if((res = NT::STATUS_OBJECT_PATH_NOT_FOUND) && (IsGlobalObjectNS(HLinkInfo->FileName) > 0))
  {
   ONSGlobalToLocalPtr(PathPtr, PathLen);
   continue;
  }      
 if(res != NT::STATUS_INVALID_PARAMETER)break;       // On a network mapped drive (SMB does not support FileLinkInformationEx?)  (Or if DstRootDir is not 0 and DstFile starts with a slash)               
 HLinkInfo->ReplaceIfExists &= NT::FILE_LINK_REPLACE_IF_EXISTS;    // Replace ? true : false;
 res = SAPI::NtSetInformationFile(hFileObj, &iosb, HLinkInfo, InfoSize, NT::FileLinkInformation);       

 break;}   // End the loop

 SAPI::NtClose(hFileObj);
 return res;
} 
//------------------------------------------------------------------------------------
// NOTE: Objects are not files but files accessed through them
/*static NT::NTSTATUS DeleteSymbolicLinkObject(const achar* Path)     // Delete a symlink in objects directory
{
if (NtOpenSymbolicLinkObject( &handle, DELETE, &objectAttributes) == STATUS_SUCCESS)
{
    NtMakeTemporaryObject( handle);
    NtClose( handle);
} 
 return 0;
}  */
//------------------------------------------------------------------------------------
/*static NTSTATUS CreateNtObjDirectory(PWSTR ObjDirName, PHANDLE phDirObj)   // Create objects directory with NULL security
{
 wchar_t Path[512] = {'\\'};
 UNICODE_STRING ObjectNameUS;
 SECURITY_DESCRIPTOR  sd = {SECURITY_DESCRIPTOR_REVISION, 0, 4};    // NULL security descriptor: InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION); SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
 OBJECT_ATTRIBUTES oattr = { sizeof(OBJECT_ATTRIBUTES), 0, &ObjectNameUS, OBJ_CASE_INSENSITIVE|OBJ_OPENIF, &sd };
 UINT Length = 1;
 for(;*ObjDirName;ObjDirName++)Path[Length++] = *ObjDirName;
 ObjectNameUS.Buffer = Path;
 ObjectNameUS.Length = Length * sizeof(wchar_t);
 ObjectNameUS.MaximumLength = ObjectNameUS.Length + sizeof(wchar_t);
 return NtCreateDirectoryObject(phDirObj, DIRECTORY_ALL_ACCESS, &oattr);
}
//----------------------------------------------------------------------------
static NTSTATUS FileCreateSync(PWSTR FileName, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PHANDLE FileHandle)
{
 IO_STATUS_BLOCK iosb = {};
 OBJECT_ATTRIBUTES oattr = {};
 UNICODE_STRING FilePathUS;
 UINT Length = 11;
 wchar_t Path[512] = {'\\','?','?','\\','G','l','o','b','a','l','\\'};
 for(int idx=0;*FileName;FileName++,Length++)Path[Length] = *FileName;
 FilePathUS.Buffer = Path;
 FilePathUS.Length = Length * sizeof(wchar_t);
 FilePathUS.MaximumLength = FilePathUS.Length + sizeof(wchar_t);
 oattr.Length = sizeof(OBJECT_ATTRIBUTES);
 oattr.RootDirectory = NULL;
 oattr.ObjectName = &FilePathUS;
 oattr.Attributes =  OBJ_CASE_INSENSITIVE;   //| OBJ_KERNEL_HANDLE;
 oattr.SecurityDescriptor = NULL;
 oattr.SecurityQualityOfService = NULL;
 return NtCreateFile(FileHandle, DesiredAccess|SYNCHRONIZE, &oattr, &iosb, NULL, FileAttributes, ShareAccess, CreateDisposition, CreateOptions|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
}
//------------------------------------------------------------------------------------
static void NtEndSystem(void)  // Requires debug privileges
{
 DWORD Value = TRUE;
 NtCurrentTeb()->ProcessEnvironmentBlock->NtGlobalFlag |= 0x100000;    // ???
 NtSetInformationProcess(NtCurrentProcess,ProcessBreakOnTermination,&Value,sizeof(Value));
 NtTerminateProcess(NtCurrentProcess, 0xBAADC0DE);
}
//------------------------------------------------------------------------------------
static BOOL NTAPI DeviceIsRunning(IN PWSTR DeviceName)
{
 UNICODE_STRING str;
 OBJECT_ATTRIBUTES attr;
 HANDLE hLink;
 BOOL result = FALSE;

 RtlInitUnicodeString(&str, DeviceName);   // TODO: Replace with macro
 InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, 0);
 NTSTATUS Status = NtOpenSymbolicLinkObject(&hLink, SYMBOLIC_LINK_QUERY, &attr);
 if(NT_SUCCESS(Status)){result = TRUE;NtClose(hLink);}
 return result;
}
//------------------------------------------------------------------------------------
static NTSTATUS NativeDeleteFile(PWSTR FileName)     // NtDeleteFile
{
 HANDLE hFile;
 OBJECT_ATTRIBUTES attr;
 IO_STATUS_BLOCK iost;
 FILE_DISPOSITION_INFORMATION fDispositionInfo;
 FILE_BASIC_INFORMATION fBasicInfo;
 UNICODE_STRING TargetFileName;

 RtlInitUnicodeString(&TargetFileName, FileName);
 InitializeObjectAttributes(&attr, &TargetFileName, OBJ_CASE_INSENSITIVE, 0, 0);
 NTSTATUS Status = NtOpenFile(&hFile, DELETE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, &attr, &iost, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);
 if(NT_SUCCESS(Status))
  {
   Status = NtQueryInformationFile(hFile, &iost, &fBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
   if(NT_SUCCESS(Status))
    {
     fBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
     NtSetInformationFile(hFile, &iost, &fBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
    }
   fDispositionInfo.DeleteFile = TRUE;  // puts the file into a "delete pending" state. It will be deleted once all existing handles are closed. No new handles will be possible to open
   Status = NtSetInformationFile(hFile, &iost, &fDispositionInfo, sizeof(FILE_DISPOSITION_INFORMATION), FileDispositionInformation);
   NtClose(hFile);
  }
 return Status;
}
//------------------------------------------------------------------------------------
static NTSTATUS OpenDevice(PWSTR DevName, HANDLE* hDev)
{
 UNICODE_STRING DevPathUS;
 OBJECT_ATTRIBUTES attr;
 IO_STATUS_BLOCK iost;
 UINT Length = 8;
 wchar_t DevPath[MAX_PATH] = {'\\','D','e','v','i','c','e','\\'};
 for(int idx=0;*DevName;DevName++,Length++)DevPath[Length] = *DevName;
 DevPathUS.Buffer = DevPath;
 DevPathUS.Length = Length * sizeof(wchar_t);
 DevPathUS.MaximumLength = DevPathUS.Length + sizeof(wchar_t);   //RtlInitUnicodeString(&str, L"\\Device\\xxx");

 attr.Length = sizeof(OBJECT_ATTRIBUTES);
 attr.RootDirectory = 0;
 attr.ObjectName = &DevPathUS;
 attr.Attributes = 0;
 attr.SecurityDescriptor = 0;
 attr.SecurityQualityOfService = 0;
 return NtCreateFile(hDev, GENERIC_READ | GENERIC_WRITE, &attr, &iost, 0, 0, 0, FILE_OPEN, 0, 0, 0);
}
//------------------------------------------------------------------------------------
static HANDLE OpenBeep(void)
{
 HANDLE hBeep = NULL;
 OpenDevice(L"Beep", &hBeep);
 return hBeep;
}
//------------------------------------------------------------------------------------
static NTSTATUS DoBeep(HANDLE hBeep, DWORD Freq, DWORD Duration)
{
 struct {
  ULONG uFrequency;
  ULONG uDuration;
 } param;
 IO_STATUS_BLOCK iost;
 param.uFrequency = Freq;    // short
 param.uDuration  = Duration;
 return NtDeviceIoControlFile(hBeep, 0, 0, 0, &iost, 0x00010000, &param, sizeof(param), 0, 0);
} */
//------------------------------------------------------------------------------------
typedef void (_scall *PNT_THREAD_PROC)(vptr Data, size_t Size);   // Param may be on stack in x32 or not  // Exit with NtTerminateThread  // stdcall on X86-X32

// http://rsdn.org/forum/winapi/164784.hot
// CreateRemoteThread requires PROCESS_VM_WRITE but that is too much to ask for a stealth thread injection
static NT::NTSTATUS NativeCreateThread(PNT_THREAD_PROC ThreadRoutine, NT::PVOID ParData, NT::SIZE_T ParSize, NT::HANDLE ProcessHandle, uint32 CrtSusp, NT::PPVOID StackBase, NT::PSIZE_T StackSize, NT::PHANDLE ThreadHandle, NT::PULONG ThreadID)
{
 static constexpr int PAGE_SIZE = MEMPAGESIZE;
 NT::NTSTATUS   Status = 0;
 NT::USER_STACK UserStack;
 NT::CONTEXT    Context;
 NT::CLIENT_ID  ClientId;
 NT::PVOID      LocStackBase = nullptr;
 NT::SIZE_T     LocStackSize = 0;

 DBGMSG("ProcessHandle=%p, ThreadRoutine=%p, ParData=%p, ParSize=%p",ProcessHandle,ThreadRoutine,ParData,ParSize);
 UserStack.FixedStackBase  = nullptr;
 UserStack.FixedStackLimit = nullptr;
 if(!StackBase)StackBase = &LocStackBase;
 if(!StackSize)StackSize = &LocStackSize;

 if(!*StackBase)
  {
   if(*StackSize)*StackSize = AlignFrwdP2(*StackSize + PAGE_SIZE, MEMGRANSIZE);    // Must allocate by 64k to avoid address space wasting
    else *StackSize = PAGE_SIZE * 256;   // 1Mb as default stack size (Including a guard page)
   Status = SAPI::NtAllocateVirtualMemory(ProcessHandle, StackBase, 0, StackSize, NT::MEM_RESERVE, NT::PAGE_READWRITE);   // Reserve the memory first
   if(!NT::IsSuccess(Status)){DBGMSG("AVM Failed 1"); return Status;}

   // NOTE: Growing of a reserved stack space is not used but a guard page is still placed because kernel may check for it
   UserStack.ExpandableStackBase   = &((uint8*)*StackBase)[*StackSize];   // Where the stack memory block ends (entire reserved region) // ESP is usually points here initially
   UserStack.ExpandableStackLimit  = &((uint8*)*StackBase)[PAGE_SIZE];    // Points to beginning of committed memory range
   UserStack.ExpandableStackBottom = *StackBase;    // Beginning of the memory block, no growing above it

   NT::SIZE_T StackCommit = *StackSize;         // Use 2 guard pages
   NT::PVOID  CommitBase  = *StackBase;
   Status = SAPI::NtAllocateVirtualMemory(ProcessHandle, &CommitBase, 0, &StackCommit, NT::MEM_COMMIT, NT::PAGE_READWRITE);    // Stack commit, includes one guard page
   if(!NT::IsSuccess(Status)){DBGMSG("AVM Failed 2"); return Status;}
  }
   else    // Assume that there may be some data on the stack already
    {
     UserStack.ExpandableStackBase   = &((uint8*)*StackBase)[AlignFrwdP2(*StackSize, MEMGRANSIZE)];  // Arbitrary - StackSize may specify offset to some data already on the stack
     UserStack.ExpandableStackLimit  = &((uint8*)*StackBase)[PAGE_SIZE];
     UserStack.ExpandableStackBottom = *StackBase;
    }
 {
  NT::ULONG  OldProtect;
  NT::SIZE_T GuardSize = PAGE_SIZE;
  NT::PVOID  GuardBase = *StackBase;
  Status = SAPI::NtProtectVirtualMemory(ProcessHandle, &GuardBase, &GuardSize, NT::PAGE_READWRITE | NT::PAGE_GUARD, &OldProtect);   // create a GUARD page
  if(!NT::IsSuccess(Status)){DBGMSG("PVM Failed"); return Status;}
 }

// Avoiding RtlInitializeContext because it uses NtWriteVirtualMemory for a Parameter on x32 (A remote process may be opened without rights to do that)
 Context.EFlags       = 0x3000;    // ??? 0x200 ?
 Context.ContextFlags = NT::CONTEXT_CONTROL|NT::CONTEXT_INTEGER;
#ifdef ARCH_X64     // Cannot use 'if constexr' it requires the names to exist unless used from a template type
 Context.Rsp  = (NT::SIZE_T)&((uint8*)*StackBase)[*StackSize];
 Context.Rip  = (NT::SIZE_T)ThreadRoutine;
 Context.Rsp -= sizeof(NT::SIZE_T) * 5;  // For a reserved block of 4 Arguments (RCX, RDX, R8, R9) and a fake ret addr
 Context.Rcx  = Context.Rax = (NT::SIZE_T)ParData;    // Match with fastcall
 Context.Rdx  = Context.Rbx = (NT::SIZE_T)ParSize;
#else        // On WOW64 these are passed directly to new x64 thread context: EIP(RIP), ESP(R8), EBX(RDX), EAX(RCX)  // Later EAX and EBX will get to x32 entry point even without Wow64pCpuInitializeStartupContext  // On native x32/64 all registers are passed to thread`s entry point
 Context.Esp  = (NT::SIZE_T)&((uint8*)*StackBase)[*StackSize];
 Context.Eip  = (NT::SIZE_T)ThreadRoutine;   // Native only
 Context.Esp -= sizeof(NT::SIZE_T) * 5;  // For a Return addr(fake, just to keep frame right) and a possible Params for not fastcall ThreadProc if you prepared it in your stack
 Context.Ecx  = Context.Eax = (NT::SIZE_T)ParData;    // Match with fastcall if Wow64pCpuInitializeStartupContext is used
 Context.Edx  = Context.Ebx = (NT::SIZE_T)ParSize;

 Context.ContextFlags |= NT::CONTEXT_SEGMENTS;    // NOTE: Only Windows x32 requires segment registers to be set
 Context.SegGs = 0x00;
 Context.SegFs = 0x38;
 Context.SegEs = 0x20;
 Context.SegDs = 0x20;
 Context.SegSs = 0x20;
 Context.SegCs = 0x18;

 if(ProcessHandle == NT::NtCurrentProcess)
  {
   ((NT::PVOID*)Context.Esp)[1] = ParData;    // Win7 WOW64 pops ret addr from stack making this argument unavailable // Solution: Pass same value in ParData and ParSize
   ((NT::PVOID*)Context.Esp)[2] = (NT::PVOID)ParSize;
  }
   else  // Creating in a remote process
    {
     NT::PVOID Arr[] = {ParData,(NT::PVOID)ParSize};
     if(NT::NTSTATUS stat = SAPI::NtWriteVirtualMemory(ProcessHandle, &((NT::PVOID*)Context.Esp)[1], &Arr, sizeof(Arr), nullptr)){DBGMSG("Write stack args failed with: %08X",stat);}   // It is OK to fail if no PROCESS_VM_WRITE
    }
#endif

 NT::ULONG ThAcc = +NT::SYNCHRONIZE|NT::THREAD_GET_CONTEXT|NT::THREAD_SET_CONTEXT|NT::THREAD_QUERY_INFORMATION|NT::THREAD_SET_INFORMATION|NT::THREAD_SUSPEND_RESUME|NT::THREAD_TERMINATE;
// NOTE: To avoid some WOW64 bugs on Windows10 we must always call a native version of this function
 Status = SAPI::NtCreateThread(ThreadHandle, ThAcc, nullptr, ProcessHandle, &ClientId, &Context, &UserStack, (uint)CrtSusp); // Always returns ACCESS_DENIED for any CFG enabled process? // The thread starts at specified IP and with specified SP and no return address // End it with NtTerminateThread  // CrtSusp must be exactly 1 to suspend
#ifdef ARCH_X32
// Always use native syscalls // if((Status == STATUS_ACCESS_DENIED) && (FixWinTenWow64NtCreateThread() > 0))Status = NtCreateThread(ThreadHandle, ThAcc, nullptr, ProcessHandle, &ClientId, &Context, (PINITIAL_TEB)&UserStack, CrtSusp);     // Try to fix Wow64NtCreateThread bug in latest versions of Windows 10
#endif
 if(!NT::IsSuccess(Status)){DBGMSG("CreateThread Failed"); return Status;}
 if(ThreadID)*ThreadID = NT::ULONG(ClientId.UniqueThread);
 return Status;
}
//------------------------------------------------------------------------------------
// Starting in Windows 8.1, GetVersion() and GetVersionEx() are subject to application manifestation
// See https://stackoverflow.com/questions/32115255/c-how-to-detect-windows-10
//
/*static inline DWORD _fastcall GetRealVersionInfo(PDWORD dwMajor=NULL, PDWORD dwMinor=NULL, PDWORD dwBuild=NULL, PDWORD dwPlatf=NULL)
{
 PPEB peb = NtCurrentPeb();
 if(dwMajor)*dwMajor = peb->OSMajorVersion;
 if(dwMinor)*dwMinor = peb->OSMinorVersion;
 if(dwBuild)*dwBuild = peb->OSBuildNumber;
 if(dwPlatf)*dwPlatf = peb->OSPlatformId;
 DWORD Composed = (peb->OSPlatformId << 16)|(peb->OSMinorVersion << 8)|peb->OSMajorVersion;
 return Composed;
}
//---------------------------------------------------------------------------
static int GetMappedFilePath(HANDLE hProcess, PVOID BaseAddr, PWSTR PathBuf, UINT BufByteSize)   // Returns as '\Device\HarddiskVolume'
{
 SIZE_T RetLen = 0;
 if(!NtQueryVirtualMemory(hProcess,BaseAddr,MemoryMappedFilenameInformation,PathBuf,BufByteSize,&RetLen) && RetLen)
  {
   PWSTR PathStr = ((UNICODE_STRING*)PathBuf)->Buffer;
   UINT  PathLen = ((UNICODE_STRING*)PathBuf)->Length;
   memmove(PathBuf, PathStr, PathLen);
   *((WCHAR*)&((PBYTE)PathBuf)[PathLen]) = 0;
   return PathLen / sizeof(WCHAR);
  }
 *PathBuf = 0;
 return 0;
};
//------------------------------------------------------------------------------------
static inline ULONG GetProcessID(HANDLE hProcess)
{
 if(!hProcess || (hProcess == NtCurrentProcess))return NtCurrentProcessId();
 PROCESS_BASIC_INFORMATION pinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationProcess(hProcess,ProcessBasicInformation,&pinf,sizeof(PROCESS_BASIC_INFORMATION),&RetLen);
 if(res){DBGMSG("Failed to get process ID: %08X", res); return 0;}   // 0 id belongs to the system
 return (ULONG)pinf.UniqueProcessId;
}
//------------------------------------------------------------------------------------
static inline ULONG GetThreadID(HANDLE hThread, ULONG* ProcessID=NULL, PTEB* pTeb=NULL)
{
 if(!hThread || (hThread == NtCurrentThread))
  {
   PTEB teb = NtCurrentTeb();
   if(ProcessID)*ProcessID = (ULONG)teb->ClientId.UniqueProcess;
   if(pTeb)*pTeb = teb;
//   DBGMSG("hThread=%p, CurrentTEB=%p, ProcID=%u, ThID=%u", hThread, teb, (ULONG)teb->ClientId.UniqueProcess, (ULONG)teb->ClientId.UniqueThread);
   return (ULONG)teb->ClientId.UniqueThread;
  }
 THREAD_BASIC_INFORMATION tinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationThread(hThread,ThreadBasicInformation,&tinf,sizeof(THREAD_BASIC_INFORMATION),&RetLen);
 if(res){DBGMSG("Failed to get thread ID: %08X", res); return 0;}   // 0 id belongs to the system
 if(ProcessID)*ProcessID = (ULONG)tinf.ClientId.UniqueProcess;
 if(pTeb)*pTeb = (PTEB)tinf.TebBaseAddress;
// DBGMSG("hThread=%p, TEB=%p, ProcID=%u, ThID=%u", hThread, tinf.TebBaseAddress, (ULONG)tinf.ClientId.UniqueProcess, (ULONG)tinf.ClientId.UniqueThread);
 return (ULONG)tinf.ClientId.UniqueThread;
}
//------------------------------------------------------------------------------------
static inline ULONG GetCurrProcessThreadID(HANDLE hThread)
{
 ULONG PrID = 0;
 ULONG ThID = GetThreadID(hThread, &PrID);
 if(!ThID || (PrID != NtCurrentProcessId()))return 0;
 return ThID;
}
//------------------------------------------------------------------------------------
static inline PTEB GetCurrProcessTEB(HANDLE hThread)
{
 PTEB  teb  = NULL;
 ULONG PrID = 0;
 ULONG ThID = GetThreadID(hThread, &PrID, &teb);
 if(!ThID || (PrID != NtCurrentProcessId()))return 0;
 return teb;
}
//------------------------------------------------------------------------------------
static inline bool IsCurrentProcess(HANDLE hProcess)
{
 return GetProcessID(hProcess) == NtCurrentProcessId();
}
//------------------------------------------------------------------------------------
static inline bool IsCurrentThread(HANDLE hThread)
{
 return GetThreadID(hThread) == NtCurrentThreadId();
}
//------------------------------------------------------------------------------------
static inline bool IsCurrentProcessThread(HANDLE hThread)
{
 return GetCurrProcessThreadID(hThread);
}  */
//------------------------------------------------------------------------------------
//
// RtlUserThreadStart (Callback)    // CREATE_SUSPENDED thread`s IP
//   x32  x64
//   EAX  RCX = ThreadProc
//   EBX  RDX = ThreadParam
//
// Native thread:   (Suspend is TRUE)
//   x32  x64
//   EAX  RCX = ThreadProc
//   EBX  RDX = ThreadParam
//
static bool ChangeNewSuspThProcAddr(NT::HANDLE hThread, NT::PVOID NewThProc, NT::PVOID* Param, bool Native=false)
{
 NT::CONTEXT ctx;
 ctx.ContextFlags = NT::CONTEXT_INTEGER;   // CONTEXT_CONTROL - no check if IP is at RtlUserThreadStart
 if(Native)ctx.ContextFlags |= NT::CONTEXT_CONTROL;
// if(NT::NTSTATUS stat = NtGetContextThread(hThread, &ctx)){DBGMSG("Failed to get CONTEXT: %08X",stat); return false;}
#ifdef ARCH_X64
 if(NewThProc)
  {
   if(Native)ctx.Rip = (uint64)NewThProc;
     else ctx.Rcx = (uint64)NewThProc;
  }
 if(Param)
  {
   uint64 Prv;
   if(Native){Prv = ctx.Rcx; ctx.Rcx = (uint64)*Param;}
    else {Prv = ctx.Rdx; ctx.Rdx = (uint64)*Param;}
   *Param  = (vptr)Prv;
  }
#else
 if(NewThProc)
  {
   if(Native)ctx.Eip = (uint32)NewThProc;    // Doesn`t work for x32!!!!!!!!!!!!!!!!!!!
     else ctx.Eax = (uint32)NewThProc;
  }
 if(Param)
  {
   uint32 Prv;
   if(Native){Prv = ctx.Ecx; ctx.Ecx = (uint32)*Param;}
     else {Prv = ctx.Ebx; ctx.Ebx = (uint32)*Param;}
   *Param  = (vptr)Prv;
  }
#endif
// if(NT::NTSTATUS stat = NtSetContextThread(hThread, &ctx)){DBGMSG("Failed to set CONTEXT: %08X",stat); return false;}
 DBGMSG("NewThProc=%p, Param=%p", NewThProc,Param?*Param:nullptr);
 return true;
}
//------------------------------------------------------------------------------------
/*static jmp_buf jenv;  // setjmp env for the jump back into the fork() function

static int child_entry(void)  // entry point for our child thread process - just longjmp into fork
{
 longjmp(jenv, 1);
 return 0;
}
//------------------------------------------------------------------------------------
static void set_inherit_all(void)
{
 ULONG  n = 0x1000;
 PULONG p = (PULONG) calloc(n, sizeof(ULONG));
 while(NtQuerySystemInformation(SystemHandleInformation, p, n * sizeof(ULONG), 0) == STATUS_INFO_LENGTH_MISMATCH)  // some guesswork to allocate a structure that will fit it all
  {
   free(p);
   n *= 2;
   p  = (PULONG) calloc(n, sizeof(ULONG));
  }
 PSYSTEM_HANDLE_INFORMATION h = (PSYSTEM_HANDLE_INFORMATION)(p + 1);  // p points to an ULONG with the count, the entries follow (hence p[0] is the size and p[1] is where the first entry starts
 ULONG pid = GetCurrentProcessId();
 ULONG i = 0, count = *p;
 while(i < count)
  {
   if(h[i].ProcessId == pid)SetHandleInformation((HANDLE)(ULONG) h[i].Handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
   i++;
  }
 free(p);
}
//------------------------------------------------------------------------------------
// https://github.com/Mattiwatti/BSOD10/
// On Windows 10 >= 10586 and < 16299,
// Windows 10 after 1511 (10.0.10586, TH2) up to 1703 (10.0.15063, RS2)
// One interesting thing to note is that NtCreateUserProcess requires an initial thread, unlike NtCreateProcess[Ex].
//   Because terminating the last thread of a process will always terminate the process, this makes NtCreateProcess[Ex] the only way to create a Windows process with zero threads.
// The crash occurs in PspInitializeFullProcessImageName, which obtains the process name from the process attribute list (the first argument to the function).
// NOTE: The process may inherit any system lib hooks (The memory is CopyOnWrite of a parent process) // NOTE: Address space will be reduced by number of parent`s region allocations
// NtCreaterProcess[Ex] is very similair to Linux 'clone' syscall with (CLONE_VM) buth there is no actual execution thread is created
// NtCreateUserProcess is a system call. It exposes process forking by setting the PS_ATTRIBUTE_PARENT_PROCESS within the PPS_ATTRIBUTE_LIST AttributeList parameter
// https://www.deepinstinct.com/blog/dirty-vanity-a-new-approach-to-code-injection-edr-bypass
// Process Snapshotting is invoked with Kernel32!PssCaptureSnapshot and if we go down the call chain we will see Kernel32!PssCaptureSnapshot calls ntdll!PssNtCaptureSnapshot calls ntdll!NtCreateProcessEx
//   ZwCreateProcessEx(vpProcessHandle, 0x2000000, 0, aParentProcess, aFlags, 0, 0, 0, 0);
// NtCreateProcess[Ex] are two legacy process creation syscalls that offer another route to access the forking mechanism.
//  However, as opposed to the newer NtCreateUserProcess, one can fork a remote process with them by setting the HANDLE ParentProcess parameter with the target process handle.
// MmInitializeProcessAddressSpace with a flag specifying that the address should be a copy-on-write copy of the target process instead of an initial process address space.
// https://groups.google.com/forum/#!topic/comp.os.ms-windows.programmer.nt.kernel-mode/hoN_RYtnp58
// The most important parameter here is SectionHandle. If this parameter is NULL, the kernel will fork the current process.
//  Otherwise, this parameter must be a handle of the SEC_IMAGE section object created on the EXE file before calling ZwCreateProcess().
//

static int NtFork(bool InheritAll=false)   // RtlCloneUserProcess ?
{
 if(setjmp(jenv) != 0)return 0;    // return as a child
 if(InheritAll)set_inherit_all();  //  make sure all handles are inheritable
 HANDLE hProcess = 0, hThread = 0;
 OBJECT_ATTRIBUTES oa = { sizeof(oa) };
 NtCreateProcess(&hProcess, PROCESS_ALL_ACCESS, &oa, NtCurrentProcess, TRUE, 0, 0, 0);  // create forked process     // Starting from Win8.1 won't allow creation of new threads (NULL section)
 CONTEXT context = {CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS | CONTEXT_FLOATING_POINT};

 NtGetContextThread(NtCurrentThread, &context);     // set the Eip for the child process to our child function
// context.Eip = (ULONG)child_entry;

 MEMORY_BASIC_INFORMATION mbi;
// NtQueryVirtualMemory(NtCurrentProcess, (PVOID)context.Esp, MemoryBasicInformation, &mbi, sizeof mbi, 0);

 USER_STACK stack = {0, 0, (PCHAR)mbi.BaseAddress + mbi.RegionSize, mbi.BaseAddress, mbi.AllocationBase};
 CLIENT_ID cid;
// NtCreateThread(&hThread, THREAD_ALL_ACCESS, &oa, hProcess, &cid, &context, &stack, TRUE);  // create thread using the modified context and stack   // Do not use THREAD_ALL_ACCESS it has been changed

 THREAD_BASIC_INFORMATION tbi;
 NtQueryInformationThread(NtCurrentThread, ThreadBasicInformation, &tbi, sizeof tbi, 0);
 PNT_TIB tib = (PNT_TIB)tbi.TebBaseAddress;
 NtQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof tbi, 0);
 NtWriteVirtualMemory(hProcess, tbi.TebBaseAddress, &tib->ExceptionList, sizeof tib->ExceptionList, 0);  // copy exception table

 NtResumeThread(hThread, 0);   // start (resume really) the child

 NtClose(hThread);
 NtClose(hProcess);
 return (int)cid.UniqueProcess;  //  exit with child's pid
}
//------------------------------------------------------------------------------------
static UINT64 GetObjAddrByHandle(HANDLE hObj, DWORD OwnerProcId)
{
 SIZE_T InfoBufSize = 1048576;
 NTSTATUS stat = 0;
 ULONG RetLen  = 0;
 UINT64 Addr   = 0;
 PVOID InfoArr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InfoBufSize);
 if(IsWow64())
  {
   while(stat = NWOW64E::QuerySystemInformation(SystemExtendedHandleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((NWOW64E::SYSTEM_HANDLE_INFORMATION_EX_64*)InfoArr)->NumberOfHandles;idx++)
    {
     NWOW64E::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX<DWORD64>* HndlInfo = &((NWOW64E::SYSTEM_HANDLE_INFORMATION_EX_64*)InfoArr)->Handles[idx];
     if(HndlInfo->UniqueProcessId != (UINT64)OwnerProcId)continue;     // Wrong process
     if(HndlInfo->HandleValue != (UINT64)hObj)continue;     // Wrong Handle
     Addr = (UINT64)HndlInfo->Object;
     break;
    }
  }
 else
  {
   while(stat = NtQuerySystemInformation(SystemExtendedHandleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((PSYSTEM_HANDLE_INFORMATION_EX)InfoArr)->NumberOfHandles;idx++)
    {
     PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HndlInfo = &((PSYSTEM_HANDLE_INFORMATION_EX)InfoArr)->Handles[idx];
     if(HndlInfo->UniqueProcessId != OwnerProcId)continue;     // Wrong process
     if(HndlInfo->HandleValue != (ULONG_PTR)hObj)continue;     // Wrong Handle
     Addr = (ULONG_PTR)HndlInfo->Object;   // Why casting to UINT64 makes this pointer sign extended on x32???  // Is 'void*' actually a signed type?
     break;
    }
   }
 if(InfoArr)HeapFree(GetProcessHeap(),0,InfoArr);
 return Addr;
}
//------------------------------------------------------------------------------------
static UINT64 GeKernelModuleBase(LPSTR ModuleName, ULONG* ImageSize)
{
 SIZE_T InfoBufSize = 1048576;
 NTSTATUS stat = 0;
 ULONG RetLen  = 0;
 UINT64 Addr   = 0;
 PVOID InfoArr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InfoBufSize);
 if(IsWow64())
  {
   while(stat = NWOW64E::QuerySystemInformation(SystemModuleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((NWOW64E::RTL_PROCESS_MODULES_64*)InfoArr)->NumberOfModules;idx++)
    {
     NWOW64E::RTL_PROCESS_MODULE_INFORMATION<DWORD64>* HndlInfo = &((NWOW64E::RTL_PROCESS_MODULES_64*)InfoArr)->Modules[idx];
    // if(HndlInfo->UniqueProcessId != (UINT64)OwnerProcId)continue;     // Wrong process
    // if(HndlInfo->HandleValue != (UINT64)hObj)continue;     // Wrong Handle
    // Addr = (UINT64)HndlInfo->Object;
     break;
    }
  }
 else
  {
   while(stat = NtQuerySystemInformation(SystemModuleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((PRTL_PROCESS_MODULES)InfoArr)->NumberOfModules;idx++)
    {
     PRTL_PROCESS_MODULE_INFORMATION HndlInfo = &((PRTL_PROCESS_MODULES)InfoArr)->Modules[idx];
    // if(HndlInfo->UniqueProcessId != OwnerProcId)continue;     // Wrong process
    // if(HndlInfo->HandleValue != (ULONG_PTR)hObj)continue;     // Wrong Handle
    // Addr = (ULONG_PTR)HndlInfo->Object;   // Why casting to UINT64 makes this pointer sign extended on x32???  // Is 'void*' actually a signed type?
     break;
    }
   }
 if(InfoArr)HeapFree(GetProcessHeap(),0,InfoArr);
 return Addr;
}
//------------------------------------------------------------------------------------
static SIZE_T IsMemAvailable(PVOID Addr, bool* IsImage=nullptr)      // Helps to skip a Reserved mapping rgions
{
 SIZE_T RetLen = 0;
 MEMORY_BASIC_INFORMATION minf;
 NTSTATUS Status = NtQueryVirtualMemory(NtCurrentProcess,Addr,MemoryBasicInformation,&minf,sizeof(MEMORY_BASIC_INFORMATION),&RetLen);
 if(Status || !(minf.State & MEM_COMMIT))return 0;
 if(IsImage)*IsImage = (minf.Type & MEM_IMAGE);  // (minf.Type & MEM_MAPPED)
 return minf.RegionSize;
}
//------------------------------------------------------------------------------------
Key Differences from Linux
Immutability: On Windows, once a process is assigned to a Job Object, it generally cannot be moved to a different one (unless they are nested, which is supported on Windows 8+).
Automatic Cleanup: If you use NtSetInformationJobObject to set the KillOnJobClose flag, closing the job handle will terminate all processes in that group—useful for cleaning up child processes.
Console Groups: If you specifically need setpgid for signal handling (like Ctrl+C), Windows uses "Console Process Groups." This is managed during process creation via the CREATE_NEW_PROCESS_GROUP flag in CreateProcess, but it is much harder to manipulate via ntdll after the process has started. 

void NativeSetJobGroup(HANDLE hProcess) {
    auto hNtdll = GetModuleHandleA("ntdll.dll");
    auto NtCreateJobObject = (_NtCreateJobObject)GetProcAddress(hNtdll, "NtCreateJobObject");
    auto NtAssignProcessToJobObject = (_NtAssignProcessToJobObject)GetProcAddress(hNtdll, "NtAssignProcessToJobObject");

    if (NtCreateJobObject && NtAssignProcessToJobObject) {
        HANDLE hJob = NULL;
        OBJECT_ATTRIBUTES objAttr;
        InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);

        // 1. Create the "Process Group" (Job Object)
        NTSTATUS status = NtCreateJobObject(&hJob, JOB_OBJECT_ALL_ACCESS, &objAttr);
        
        if (status == 0) { // STATUS_SUCCESS
            // 2. Assign the process to the group (setpgid equivalent)
            NtAssignProcessToJobObject(hJob, hProcess);
        }
    }
}
*/
// A PID can be recycled the instant the last thread of a process terminates. However, a Process Object in the kernel is not destroyed until the last Handle to it is closed.
int GetProcessState(NT::HANDLE hProcess)   // Returns 1 if the process is in a state of termination
{
 NT::PROCESS_BASIC_INFORMATION pbi;
 NT::NTSTATUS status = SAPI::NtQueryInformationProcess(hProcess, NT::ProcessBasicInformation, &pbi, sizeof(pbi), nullptr);
 if(!NT::IsSuccess(status)) return -1;        // The process is dead or we have no access to it, either way we cannot get its state
 return (pbi.ExitStatus == NT::STATUS_PENDING);   // If status is success, check if ExitStatus is still pending (Beware of Zombies which is forever pending their termination on some broken I/O operation)
}
//------------------------------------------------------------------------------------
// NOTE: LdrInitializeThunk will crash without this
// CreateProcessParameters(hProcess, pbi.PebBaseAddress, name);     // Do not forget that the we can be a X32 process and the target process is X64 or vice versa
// NOTE: Do this before first thread creation
// NOTE: A lot of WOW64 hassle for Windows XP X64
//
static NT::NTSTATUS InitProcessEnvironment(NT::HANDLE hProcess, const NT::UNICODE_STRING* ImgPath, const achar** Args, const achar** EVars)  // TODO TODO TODO
{
 NT::PROCESS_BASIC_INFORMATION pbi;   // For PEB addr

 NT::PEB* ThisPEB = NT::NtCurrentPeb();
 NT::RTL_USER_PROCESS_PARAMETERS* ThisProcParams = ThisPEB->ProcessParameters;
 NT::NTSTATUS res = SAPI::NtQueryInformationProcess(hProcess, NT::ProcessBasicInformation, &pbi, sizeof pbi, nullptr);
 if(res)return res;

 if(ThisProcParams)    // [RTL_USER_PROCESS_PARAMETERS,Strings],Environment
  {
   size_t RmtBlkSize = ThisProcParams->MaximumLength;
   RmtBlkSize += ThisProcParams->EnvironmentSize;

 //  CurrentDirectories   // Useless // Always empty?

  /*  NT::PPROCESS_PARAMETERS pp;

        p->MaximumLength = ByteCount;
        p->Length = ByteCount;
        p->Flags = RTL_USER_PROC_PARAMS_NORMALIZED;
        p->DebugFlags = 0;
        p->Environment = Environment;
        p->CurrentDirectory.Handle = CurDirHandle;

    NT::RtlCreateProcessParameters(&pp, ImageFile, 0, 0, 0, 0, 0, 0, 0, 0);

    pp->Environment = CopyEnvironment(hProcess);

    ULONG n = pp->Size;
    PVOID p = 0;
    NT::ZwAllocateVirtualMemory(hProcess, &p, 0, &n, MEM_COMMIT, PAGE_READWRITE);

    NT::ZwWriteVirtualMemory(hProcess, p, pp, pp->Size, 0);

    NT::ZwWriteVirtualMemory(hProcess, PCHAR(Peb) + 0x10, &p, sizeof p, 0);

    NT::RtlDestroyProcessParameters(pp);
       */
  }
 return 0;
}
//------------------------------------------------------------------------------------
// ntdll!LdrInitializeThunk
// https://vrodxda.hatenablog.com/entry/2019/09/18/085454
// https://github.com/mic101/windows/blob/master/WRK-v1.2/base/ntos/rtl/rtlexec.c
// https://learn.microsoft.com/en-us/windows/win32/sbscs/activation-contexts
//
// Init by system(NtCreateProcess) PEB at LdrInitializeThunk:
//    BeingDebugged, BitField(?), Mutant, ImageBaseAddress, ApiSetMap, AnsiCodePageData, OemCodePageData, UnicodeCaseTableData, NumberOfProcessors, CriticalSectionTimeout, HeapSegmentReserve, HeapSegmentCommit, HeapDeCommitTotalFreeThreshold, HeapDeCommitFreeBlockThreshold
//    OSMajorVersion, OSMinorVersion, OSBuildNumber, OSCSDVersion, OSPlatformId, ImageSubsystem, ImageSubsystemMajorVersion, ImageSubsystemMinorVersion, ActiveProcessAffinityMask, SessionId
// CreateProcess -> NtCreateUserProcess adds:
//    ProcessParameters, pShimData(64k), ActivationContextData(64k), SystemDefaultActivationContextData(64k)
//  NOTE: Call this before main thread creation
//  NOTE: Use ONLY if NtCreateUserProcess syscall is not available (Windows XP)
//
static NT::NTSTATUS NativeCreateProcess(const achar* ImgPath, const achar** Args, const achar** EVars, uint32 Flags, NT::HANDLE* phProc, NT::HANDLE* phThrd)
{
 NT::HANDLE hProcess, hThread, hSection, hFile;
 NT::NTSTATUS res;
 NT::IO_STATUS_BLOCK iosb = {};
 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::UNICODE_STRING FilePathUS;
 NT::SECTION_IMAGE_INFORMATION sii;   // Stack info of the PE image

 uint plen;
 NTX::EPathType ptype = ptUnknown;
 uint PathLen = CalcFilePathBufSize(ImgPath, plen, ptype);

 wchar FullPath[PathLen];
 InitFileObjectAttributes(ImgPath, plen, ptype, 0, FullPath, &FilePathUS, &oattr);

 res = SAPI::NtCreateFile(&hFile, +NT::SYNCHRONIZE | NT::FILE_EXECUTE, &oattr, &iosb, nullptr, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ, NT::FILE_OPEN, NT::FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);
 if(res)return res;

 oattr.ObjectName = nullptr;  // Actually, it is possible to create a named process object here
 res = SAPI::NtCreateSection(&hSection, NT::SECTION_ALL_ACCESS, &oattr, nullptr, NT::PAGE_EXECUTE, NT::SEC_IMAGE, hFile);
 SAPI::NtClose(hFile);   // Not inherited even if not closed (Not inheritable)
 if(res)return res;

// NOTE: Execution actually starts at ntdll.dll::LdrInitializeThunk  // WOW64?
 res = SAPI::NtCreateProcess(&hProcess, NT::PROCESS_ALL_ACCESS, &oattr, NT::NtCurrentProcess, true, hSection, 0, 0);   // Are there any use for 'fork' ability? // Only Ntdll.dll is mapped(Not a GUI process?), console handles are inherited
 if(res){SAPI::NtClose(hSection); return res;}

 res = SAPI::NtQuerySection(hSection, NT::SectionImageInformation, &sii, sizeof sii, nullptr);
 SAPI::NtClose(hSection);
 if(res){SAPI::NtClose(hProcess); return res;}

 res = InitProcessEnvironment(hProcess, &FilePathUS, Args, EVars);
 if(res){SAPI::NtClose(hProcess); return res;}

 NT::ULONG  ThreadID;
 NT::PVOID  StackBase = nullptr;   // Alloc at any addr
 NT::SIZE_T StackSize = sii.MaximumStackSize;
 res = NTX::NativeCreateThread((PNT_THREAD_PROC)(sii.TransferAddress), nullptr, 0, hProcess, true, &StackBase, &StackSize, &hThread, &ThreadID);  // Suspended?  // Parameters for the thread?
 if(res){SAPI::NtClose(hProcess); return res;}

   // if(Flags & 2)InformCsrss(hProcess, hThread, ULONG(cid.UniqueProcess), ULONG(cid.UniqueThread));    // Only if suspended

 if(!(Flags & 1))SAPI::NtResumeThread(hThread, nullptr);     // If suspended
 if(!phProc)SAPI::NtClose(hProcess);
  else *phProc = hProcess;
 if(!phThrd)SAPI::NtClose(hThread);
  else *phThrd = hThread;
 return 0;
}
//------------------------------------------------------------------------------------
static NT::NTSTATUS _scall NativeCreateUserProcess(NT::PHANDLE ProcessHandle, NT::PHANDLE ThreadHandle, NT::ACCESS_MASK ProcessDesiredAccess, NT::ACCESS_MASK ThreadDesiredAccess, NT::POBJECT_ATTRIBUTES ProcessObjectAttributes, NT::POBJECT_ATTRIBUTES ThreadObjectAttributes, NT::ULONG ProcessFlags, NT::ULONG ThreadFlags, NT::PRTL_USER_PROCESS_PARAMETERS ProcessParameters, NT::PPS_CREATE_INFO CreateInfo, NT::PPS_ATTRIBUTE_LIST AttributeList)
{

 // InformCsrss(hProcess, hThread, ULONG(cid.UniqueProcess), ULONG(cid.UniqueThread));    // Only if suspended?
 return 0;
}
//------------------------------------------------------------------------------------
static NT::NTSTATUS OpenProcess(NT::PHANDLE ProcessHandle, NT::ACCESS_MASK DesiredAccess, uint32 ProcID)
{
 NT::CLIENT_ID pid = {ProcID, 0};
 NT::OBJECT_ATTRIBUTES oattr = { .Length=sizeof(NT::OBJECT_ATTRIBUTES) };
 return SAPI::NtOpenProcess(ProcessHandle, DesiredAccess, &oattr, &pid);
}
//------------------------------------------------------------------------------------
static NT::NTSTATUS OpenThread(NT::PHANDLE ThreadHandle, NT::ACCESS_MASK DesiredAccess, uint32 ThreadID)
{
 NT::CLIENT_ID tid = {0, ThreadID};
 NT::OBJECT_ATTRIBUTES oattr = { .Length=sizeof(NT::OBJECT_ATTRIBUTES) };
 return SAPI::NtOpenThread(ThreadHandle, DesiredAccess, &oattr, &tid);
}
//------------------------------------------------------------------------------------
static NT::NTSTATUS GetMappedFilePath(NT::HANDLE ProcH, usize Addr, achar* Buf, usize* Len=nullptr)
{
 uint8  buf[2048];
 uint8* PathBuf = buf;
 usize  RetLen  = sizeof(buf);
 NT::NTSTATUS res = SAPI::NtQueryVirtualMemory(ProcH, (vptr)Addr, NT::MemoryMappedFilenameInformation, PathBuf, RetLen, &RetLen);   // sizeof buf
 if((res == NT::STATUS_BUFFER_OVERFLOW) || (res == NT::STATUS_BUFFER_TOO_SMALL))    // Allocate on the stack as much as needed then     // STATUS_BUFFER_TOO_SMALL is just in case
  {
   usize blen = AlignFrwdP2(RetLen, 16);
   PathBuf = (uint8*)StkAlloc(blen);   //uint8 FullPath[blen];    // NOTE: VLA
   res = SAPI::NtQueryVirtualMemory(ProcH, (vptr)Addr, NT::MemoryMappedFilenameInformation, PathBuf, blen, &RetLen);  
   if(!NT::IsSuccess(res))return res;
  }
   else if(!NT::IsSuccess(res))return res;

 NT::UNICODE_STRING* str = (NT::UNICODE_STRING*)PathBuf;
 usize SrcLen = str->Length >> 1;     // In chars
// if(Len && ((SrcLen+1) > *Len)){*Len = NUTF::Len16To8(str->Buffer, SrcLen); return NT::STATUS_BUFFER_TOO_SMALL;}  // No data copied  // Number of WIDE chars is greater than number of DST bytes
 usize DstLen = Len ? (*Len - 1) : usize(-1); 
 uint  DstIdx = 0; 
 uint  SrcIdx = 0;                            
 usize DstRes = NUTF::Utf16To8(Buf, str->Buffer, DstLen, SrcLen, DstIdx, SrcIdx);  // NOTE: No POSIX slash conversion (getdents implementation doesn't do that too)
 Buf[DstIdx]  = 0;
 if(Len)*Len  = ++DstIdx;  // Include a term zero? 
 return (SrcIdx != SrcLen) ? NT::STATUS_BUFFER_TOO_SMALL : NT::STATUS_SUCCESS;    // NT::STATUS_BUFFER_OVERFLOW    // *Len = DstIdx + NUTF::Len16To8(str->Buffer, SrcLen, SrcIdx);
}
//------------------------------------------------------------------------------------
static sint FindMappedRangeByAddr(NT::HANDLE ProcH, usize Addr, SMemRange* Range)
{                                        
 Range->FMOffs = Range->INode = Range->DevH = Range->DevL = 0;
 NT::MEMORY_BASIC_INFORMATION mbi;
 NT::NTSTATUS res = SAPI::NtQueryVirtualMemory(ProcH, (vptr)Addr, NT::MemoryBasicInformation, &mbi, sizeof mbi, nullptr);  
 if(!NT::IsSuccess(res))return -9;
 if(!(mbi.State & NT::MEM_COMMIT))return -1;  // Not present
 Range->RangeBeg = (usize)mbi.BaseAddress;
 Range->RangeEnd = Range->RangeBeg + mbi.RegionSize; 
 Range->Mode     = MemProtNTtoPX(mbi.Protect);
 if(!(mbi.Type & NT::MEM_PRIVATE))Range->Mode |= mmShared;
 if((mbi.Type & (NT::MEM_IMAGE|NT::MEM_MAPPED)) && Range->FPathLen && Range->FPath)
  {
   usize Len = Range->FPathLen;
   res = GetMappedFilePath(ProcH, (usize)mbi.BaseAddress, Range->FPath, &Len);
   if(NT::IsSuccess(res) || (res == NT::STATUS_BUFFER_OVERFLOW))Range->FPathLen = Len-1;
     else Range->FPathLen = 0;
   return Range->FPathLen + sizeof(SMemRange) + (bool)Range->FPathLen; 
  }
 return sizeof(SMemRange);
}
//------------------------------------------------------------------------------------
static sint FindMappedRangeByAddr(sint32 ProcId, usize Addr, SMemRange* Range)
{
 NT::HANDLE PrHndl = NT::NtCurrentProcess;
 if(ProcId > 0)
  {                                                         
   NT::NTSTATUS nres = OpenProcess(&PrHndl, NT::PROCESS_QUERY_INFORMATION, (uint32)ProcId);
   if(!NT::IsSuccess(nres))return -1;
  }
 sint res = FindMappedRangeByAddr(PrHndl, Addr, Range);
 if(ProcId > 0)SAPI::NtClose(PrHndl);
 return res;
}
//------------------------------------------------------------------------------------
static sint FindMappedRangesByPath(NT::HANDLE ProcH, usize Addr, const achar* ModPath, SMemMap* MappedRanges, usize BufSize)
{
 return 0;
}
//------------------------------------------------------------------------------------
static sint FindMappedRangesByPath(sint32 ProcId, usize Addr, const achar* ModPath, SMemMap* MappedRanges, usize BufSize)
{
 NT::HANDLE PrHndl = NT::NtCurrentProcess;
 if(ProcId > 0)
  {                                                         
   NT::NTSTATUS nres = OpenProcess(&PrHndl, NT::PROCESS_QUERY_INFORMATION, (uint32)ProcId);
   if(!NT::IsSuccess(nres))return -1;
  }
 sint res = FindMappedRangesByPath(PrHndl, Addr, ModPath, MappedRanges, BufSize);
 if(ProcId > 0)SAPI::NtClose(PrHndl);
 return res;
}
//------------------------------------------------------------------------------------
static sint ReadMappedRanges(NT::HANDLE ProcH, usize AddrFrom, usize AddrTo, SMemMap* MappedRanges, usize BufSize)  // Windows: QueryVirtualMemory; Linux: ProcFS; BSD:?; MacOS:?
{
 size_t stroffs = BufSize;
 vptr CurBaseAddr = nullptr;
 sint res = -1000;
 sint Total = 0;
 SMemRange* Range = MappedRanges->Ranges;
 MappedRanges->NextAddr  = 0;
 MappedRanges->RangesCnt = 0;  // To avoid the path copy skipping
 BufSize -= sizeof(SMemMap);
 for(;;)
  {
   NT::MEMORY_BASIC_INFORMATION mbi;
   NT::NTSTATUS nres = SAPI::NtQueryVirtualMemory(ProcH, (vptr)AddrFrom, NT::MemoryBasicInformation, &mbi, sizeof mbi, nullptr);
   if(!NT::IsSuccess(nres))return -9;
   if((usize)mbi.BaseAddress >= AddrTo)break;
   if(!(mbi.State & NT::MEM_COMMIT))continue;  // Not present
   Range->RangeBeg = (usize)mbi.BaseAddress;
   Range->RangeEnd = Range->RangeBeg + mbi.RegionSize; 
   Range->Mode     = MemProtNTtoPX(mbi.Protect);
   if(mbi.Type & (NT::MEM_IMAGE|NT::MEM_MAPPED))  // The range belongs to a mapped file
    {
     if(mbi.AllocationBase != CurBaseAddr)  // Need to add a new path
      {
       // TODO: Request the path
       usize Len = MappedRanges->TmpBufLen;
       nres = GetMappedFilePath(ProcH, (usize)mbi.BaseAddress, (achar*)MappedRanges->TmpBufPtr, &Len);
       if(NT::IsSuccess(nres))       // Check if Len < TmpBufLen ?
        {
         uint len = Range->FPathLen + 1;
         if((len + sizeof(SMemRange)) > BufSize){MappedRanges->NextAddr = Range->RangeBeg; break;}  // No space left
         CurBaseAddr = mbi.AllocationBase;
         achar* srcstr = Range->FPath;   // In the line buffer
         Range->FPath  = (achar*)MappedRanges + (stroffs - len);      // Put the path at the end of the buffer
         NSTR::StrCopy(Range->FPath, srcstr, len);
         stroffs -= len;
         BufSize -= len;
         Total   += len;
        }
      }
       else   // Same path
        {
         Range->FPath    = Range[-1].FPath;
         Range->FPathLen = Range[-1].FPathLen;
        }
    }
   if(sizeof(SMemRange) > BufSize){MappedRanges->NextAddr = Range->RangeBeg; break;}  // No space left
   MappedRanges->RangesCnt++;
   BufSize -= sizeof(SMemRange);
   Total   += sizeof(SMemRange);
   Range++;
  }
 if(res >= 0)return (MappedRanges->RangesCnt)?(Total+sizeof(SMemMap)):(Total);    // For statistics: there will be gap between records and strings
 return res;   // Not found
}
//------------------------------------------------------------------------------------
static sint ReadMappedRanges(sint32 ProcId, usize AddrFrom, usize AddrTo, SMemMap* MappedRanges, usize BufSize)
{
 NT::HANDLE PrHndl = NT::NtCurrentProcess;
 if(ProcId > 0)
  {                                                         
   NT::NTSTATUS res = OpenProcess(&PrHndl, NT::PROCESS_QUERY_INFORMATION, (uint32)ProcId);
   if(!NT::IsSuccess(res))return -1;
  }
 sint res = ReadMappedRanges(PrHndl, AddrFrom, AddrTo, MappedRanges, BufSize);
 if(ProcId > 0)SAPI::NtClose(PrHndl);
 return res;
}
//------------------------------------------------------------------------------------
// On some hardware architectures (e.g., i386), PROT_WRITE implies PROT_READ.
// It is architecture dependent whether PROT_READ implies PROT_EXEC or not.
// Portable programs should always set PROT_EXEC if they intend to execute code in the new mapping.
//
// Works for memory allocation/protection and file mapping
//
static uint32 MemProtPXtoNT(uint32 prot)
{
 uint32 PageProt = 0;
 if(prot & PX::PROT_EXEC)     // Use EXEC group of flags
  {
   if(prot & PX::PROT_WRITE)
    {
     if(prot & PX::PROT_READ)PageProt |= NT::PAGE_EXECUTE_READWRITE;
       else PageProt |= NT::PAGE_EXECUTE_WRITECOPY;        // Windows Server 2003 and Windows XP:  This value is not supported.
    }
   else
    {
     if(prot & PX::PROT_READ)PageProt |= NT::PAGE_EXECUTE_READ;
       else PageProt |= NT::PAGE_EXECUTE;
    }
  }
 else   // Use ordinary R/W group of flags
  {
   if(prot & PX::PROT_WRITE)
    {
     if(prot & PX::PROT_READ)PageProt |= NT::PAGE_READWRITE;
       else PageProt |= NT::PAGE_WRITECOPY;
    }
   else
    {
     if(prot & PX::PROT_READ)PageProt |= NT::PAGE_READONLY;
       else PageProt |= NT::PAGE_NOACCESS;           // PAGE_GUARD ?  // PROT_NONE 
    }
  }
 return PageProt;
}
/*
   if(prot & PX::PROT_EXEC)
    {
     if(prot & PX::PROT_WRITE)PageProt = NT::PAGE_EXECUTE_READWRITE;
       else if(prot & PX::PROT_READ)PageProt = NT::PAGE_EXECUTE_READ;
         else PageProt = NT::PAGE_EXECUTE;
    }
     else
      {
       if(prot & PX::PROT_WRITE)PageProt = NT::PAGE_READWRITE;
         else PageProt = NT::PAGE_READONLY;
      }
*/
//------------------------------------------------------------------------------------
static uint32 MemProtNTtoPX(uint32 prot)
{
 uint32 PageProt = PX::PROT_NONE;
 if(!(prot & NT::PAGE_NOACCESS))
  {
   if(prot & NT::PAGE_EXECUTE_READWRITE)PageProt = PX::PROT_EXEC|PX::PROT_READ|PX::PROT_WRITE;
   else if(prot & NT::PAGE_EXECUTE_READ)PageProt = PX::PROT_EXEC|PX::PROT_READ;
   else if(prot & NT::PAGE_EXECUTE)PageProt = PX::PROT_EXEC;
   else if(prot & NT::PAGE_WRITECOPY)PageProt = PX::PROT_WRITE;    // |PX::PROT_READ;     // ???
   else if(prot & NT::PAGE_READWRITE)PageProt = PX::PROT_WRITE|PX::PROT_READ;
   else if(prot & NT::PAGE_READONLY)PageProt = PX::PROT_READ;                
  }
 return PageProt;
}
//------------------------------------------------------------------------------------
// From MingW
static uint NTStatusToLinuxErr(NT::NTSTATUS err, uint Default=PX::EPERM)
{
 if(!err)return PX::NOERROR;
 switch((uint)err)
  {
  case NT::STATUS_ABANDONED:
//  case NT::STATUS_ABANDONED_WAIT_0:
    return PX::ECHILD;

  case NT::STATUS_TIMEOUT:
    return PX::ETIMEDOUT;

  case NT::STATUS_ALERTED:    // NtWaitForAlertByThreadId returns it after NtAlertThreadByThreadId  // FUTEX_WAIT returns 0 if the caller was woken up
  case NT::STATUS_USER_APC:   // More fitting EINTR
    return PX::EINTR;         // since Linux 2.6.22 does not returned for a spurious wakeup

  case NT::STATUS_DEVICE_BUSY:
  case NT::STATUS_SHARING_VIOLATION:
  case NT::STATUS_FILE_LOCK_CONFLICT:
  case NT::STATUS_LOCK_NOT_GRANTED:
  case NT::STATUS_INSTANCE_NOT_AVAILABLE:
  case NT::STATUS_PIPE_NOT_AVAILABLE:
  case NT::STATUS_PIPE_BUSY:
  case NT::STATUS_PIPE_CONNECTED:
    return PX::EBUSY;

  case NT::STATUS_PENDING:                // ???
  case NT::STATUS_ALREADY_DISCONNECTED:
  case NT::STATUS_PAGEFILE_QUOTA:
  case NT::STATUS_WORKING_SET_QUOTA:
  case NT::STATUS_FILES_OPEN:
  case NT::STATUS_CONNECTION_IN_USE:
  case NT::STATUS_COMMITMENT_LIMIT:
    return PX::EAGAIN;

  case NT::STATUS_MORE_ENTRIES:
  case NT::STATUS_BUFFER_OVERFLOW:
  case NT::STATUS_MORE_PROCESSING_REQUIRED:
    return PX::EMSGSIZE;

  case NT::STATUS_WORKING_SET_LIMIT_RANGE:
  case NT::STATUS_INVALID_EA_NAME:
  case NT::STATUS_EA_LIST_INCONSISTENT:
  case NT::STATUS_INVALID_EA_FLAG:
  case NT::STATUS_INVALID_INFO_CLASS:
  case NT::STATUS_INVALID_CID:
  case NT::STATUS_INVALID_PARAMETER:
  case NT::STATUS_NONEXISTENT_SECTOR:
  case NT::STATUS_CONFLICTING_ADDRESSES:
  case NT::STATUS_NOT_MAPPED_VIEW:
  case NT::STATUS_UNABLE_TO_FREE_VM:
  case NT::STATUS_UNABLE_TO_DELETE_SECTION:
  case NT::STATUS_UNABLE_TO_DECOMMIT_VM:
  case NT::STATUS_NOT_COMMITTED:
  case NT::STATUS_INVALID_PARAMETER_MIX:
  case NT::STATUS_INVALID_PAGE_PROTECTION:
  case NT::STATUS_PORT_ALREADY_SET:
  case NT::STATUS_SECTION_NOT_IMAGE:
  case NT::STATUS_BAD_WORKING_SET_LIMIT:
  case NT::STATUS_INCOMPATIBLE_FILE_MAP:
  case NT::STATUS_SECTION_PROTECTION:
  case NT::STATUS_EA_TOO_LARGE:
  case NT::STATUS_NONE_MAPPED:
  case NT::STATUS_NO_TOKEN:
  case NT::STATUS_NOT_MAPPED_DATA:
  case NT::STATUS_FREE_VM_NOT_AT_BASE:
  case NT::STATUS_MEMORY_NOT_ALLOCATED:
  case NT::STATUS_BAD_MASTER_BOOT_RECORD:
  case NT::STATUS_INVALID_PIPE_STATE:
  case NT::STATUS_INVALID_READ_MODE:
  case NT::STATUS_INVALID_PARAMETER_1:
  case NT::STATUS_INVALID_PARAMETER_2:
  case NT::STATUS_INVALID_PARAMETER_3:
  case NT::STATUS_INVALID_PARAMETER_4:
  case NT::STATUS_INVALID_PARAMETER_5:
  case NT::STATUS_INVALID_PARAMETER_6:
  case NT::STATUS_INVALID_PARAMETER_7:
  case NT::STATUS_INVALID_PARAMETER_8:
  case NT::STATUS_INVALID_PARAMETER_9:
  case NT::STATUS_INVALID_PARAMETER_10:
  case NT::STATUS_INVALID_PARAMETER_11:
  case NT::STATUS_INVALID_PARAMETER_12:
  case NT::STATUS_DEVICE_CONFIGURATION_ERROR:
  case NT::STATUS_FAIL_CHECK:
    return PX::EINVAL;

  case NT::STATUS_DATATYPE_MISALIGNMENT:
  case NT::STATUS_ACCESS_VIOLATION:
  case NT::STATUS_DATATYPE_MISALIGNMENT_ERROR:
    return PX::EFAULT;

/*  case NT::STATUS_NO_MORE_FILES:
  case NT::STATUS_NO_MORE_EAS:
  case NT::STATUS_NO_MORE_ENTRIES:
  case NT::STATUS_GUIDS_EXHAUSTED:
  case NT::STATUS_AGENTS_EXHAUSTED:
    return PX::ENMFILE;  */

/*  case NT::STATUS_DEVICE_POWERED_OFF:
  case NT::STATUS_DEVICE_OFF_LINE:
  case NT::STATUS_NO_MEDIA_IN_DEVICE:
  case NT::STATUS_DEVICE_POWER_FAILURE:
  case NT::STATUS_DEVICE_NOT_READY:
  case NT::STATUS_NO_MEDIA:
  case NT::STATUS_VOLUME_DISMOUNTED:
  case NT::STATUS_POWER_STATE_INVALID:
    return PX::ENOMEDIUM;   */

  case NT::STATUS_FILEMARK_DETECTED:
  case NT::STATUS_BUS_RESET:
  case NT::STATUS_BEGINNING_OF_MEDIA:
  case NT::STATUS_SETMARK_DETECTED:
  case NT::STATUS_NO_DATA_DETECTED:
  case NT::STATUS_DISK_CORRUPT_ERROR:
  case NT::STATUS_DATA_OVERRUN:
  case NT::STATUS_DATA_LATE_ERROR:
  case NT::STATUS_DATA_ERROR:
  case NT::STATUS_CRC_ERROR:
  case NT::STATUS_QUOTA_EXCEEDED:
  case NT::STATUS_SUSPEND_COUNT_EXCEEDED:
  case NT::STATUS_DEVICE_DATA_ERROR:
  case NT::STATUS_UNEXPECTED_NETWORK_ERROR:
  case NT::STATUS_UNEXPECTED_IO_ERROR:
  case NT::STATUS_LINK_FAILED:
  case NT::STATUS_LINK_TIMEOUT:
  case NT::STATUS_INVALID_CONNECTION:
  case NT::STATUS_INVALID_ADDRESS:
  case NT::STATUS_FT_MISSING_MEMBER:
  case NT::STATUS_FT_ORPHANING:
  case NT::STATUS_INVALID_BLOCK_LENGTH:
  case NT::STATUS_EOM_OVERFLOW:
  case NT::STATUS_DRIVER_INTERNAL_ERROR:
  case NT::STATUS_IO_DEVICE_ERROR:
  case NT::STATUS_DEVICE_PROTOCOL_ERROR:
  case NT::STATUS_USER_SESSION_DELETED:
  case NT::STATUS_TRANSACTION_ABORTED:
  case NT::STATUS_TRANSACTION_TIMED_OUT:
  case NT::STATUS_TRANSACTION_NO_RELEASE:
  case NT::STATUS_TRANSACTION_NO_MATCH:
  case NT::STATUS_TRANSACTION_RESPONDED:
  case NT::STATUS_TRANSACTION_INVALID_ID:
  case NT::STATUS_TRANSACTION_INVALID_TYPE:
  case NT::STATUS_DEVICE_REQUIRES_CLEANING:
  case NT::STATUS_DEVICE_DOOR_OPEN:
    return PX::EIO;

  case NT::STATUS_END_OF_MEDIA:
  case NT::STATUS_DISK_FULL:
    return PX::ENOSPC;

  case NT::STATUS_NOT_IMPLEMENTED:
  case NT::STATUS_INVALID_DEVICE_REQUEST:
  case NT::STATUS_INVALID_SYSTEM_SERVICE:
  case NT::STATUS_ILLEGAL_FUNCTION:
  case NT::STATUS_VOLUME_NOT_UPGRADED:
  case NT::DBG_NO_STATE_CHANGE:
    return PX::EBADRQC;

  case NT::STATUS_INVALID_HANDLE:
  case NT::STATUS_OBJECT_TYPE_MISMATCH:
  case NT::STATUS_PORT_DISCONNECTED:
  case NT::STATUS_INVALID_PORT_HANDLE:
  case NT::STATUS_FILE_CLOSED:
  case NT::STATUS_HANDLE_NOT_CLOSABLE:
  case NT::RPC_NT_INVALID_BINDING:
  case NT::RPC_NT_SS_IN_NULL_CONTEXT:
  case NT::RPC_NT_SS_CONTEXT_MISMATCH:
    return PX::EBADF;

  case NT::STATUS_BAD_INITIAL_PC:
  case NT::STATUS_INVALID_FILE_FOR_SECTION:
  case NT::STATUS_INVALID_IMAGE_FORMAT:
  case NT::STATUS_INVALID_IMAGE_NE_FORMAT:
  case NT::STATUS_INVALID_IMAGE_LE_FORMAT:
  case NT::STATUS_INVALID_IMAGE_NOT_MZ:
  case NT::STATUS_INVALID_IMAGE_PROTECT:
  case NT::STATUS_INVALID_IMAGE_WIN_16:
  case NT::STATUS_IMAGE_CHECKSUM_MISMATCH:
  case NT::STATUS_IMAGE_MP_UP_MISMATCH:
  case NT::STATUS_INVALID_IMAGE_WIN_32:
  case NT::STATUS_INVALID_IMAGE_WIN_64:
    return PX::ENOEXEC;

  case NT::STATUS_NO_SUCH_FILE:
  case NT::STATUS_OBJECT_NAME_INVALID:
  case NT::STATUS_OBJECT_NAME_NOT_FOUND:
  case NT::STATUS_OBJECT_PATH_INVALID:
  case NT::STATUS_OBJECT_PATH_NOT_FOUND:
  case NT::STATUS_OBJECT_PATH_SYNTAX_BAD:
  case NT::STATUS_DFS_EXIT_PATH_FOUND:
  case NT::STATUS_BAD_NETWORK_PATH:
  case NT::STATUS_DEVICE_DOES_NOT_EXIST:
  case NT::STATUS_NETWORK_NAME_DELETED:
  case NT::STATUS_BAD_NETWORK_NAME:
  case NT::STATUS_REDIRECTOR_NOT_STARTED:
  case NT::STATUS_DLL_NOT_FOUND:
  case NT::STATUS_LOCAL_DISCONNECT:
  case NT::STATUS_REMOTE_DISCONNECT:
  case NT::STATUS_ADDRESS_CLOSED:
  case NT::STATUS_CONNECTION_DISCONNECTED:
  case NT::STATUS_CONNECTION_RESET:
  case NT::STATUS_DIRECTORY_IS_A_REPARSE_POINT:
  case NT::STATUS_OBJECTID_NOT_FOUND:
  case NT::DBG_APP_NOT_IDLE:
    return PX::ENOENT;

  case NT::STATUS_END_OF_FILE:
  case NT::STATUS_FILE_FORCED_CLOSED:
    return PX::ENODATA;

  case NT::STATUS_NO_MEMORY:
  case NT::STATUS_SECTION_TOO_BIG:
  case NT::STATUS_SECTION_NOT_EXTENDED:
  case NT::STATUS_TOO_MANY_PAGING_FILES:
    return PX::ENOMEM;

  case NT::STATUS_FILE_IS_A_DIRECTORY:
    return PX::EISDIR;                    

  case NT::STATUS_INVALID_LOCK_SEQUENCE:
  case NT::STATUS_INVALID_VIEW_SIZE:
  case NT::STATUS_ALREADY_COMMITTED:
  case NT::STATUS_ACCESS_DENIED:
  case NT::STATUS_PORT_CONNECTION_REFUSED:
  case NT::STATUS_THREAD_IS_TERMINATING:
  case NT::STATUS_DELETE_PENDING:
  case NT::STATUS_FILE_RENAMED:
  case NT::STATUS_PROCESS_IS_TERMINATING:
  case NT::STATUS_CANNOT_DELETE:
  case NT::STATUS_FILE_DELETED:
  case NT::STATUS_ENCRYPTION_FAILED:
  case NT::STATUS_DECRYPTION_FAILED:
  case NT::STATUS_NO_RECOVERY_POLICY:
  case NT::STATUS_NO_EFS:
  case NT::STATUS_WRONG_EFS:
  case NT::STATUS_NO_USER_KEYS:
    return PX::EACCES;

  case NT::STATUS_OBJECT_NAME_COLLISION:
  case NT::STATUS_NONEXISTENT_EA_ENTRY:
  case NT::STATUS_NO_EAS_ON_FILE:
  case NT::STATUS_EA_CORRUPT_ERROR:
  case NT::STATUS_FILE_CORRUPT_ERROR:
    return PX::EEXIST;

  case NT::STATUS_MUTANT_NOT_OWNED:
  case NT::STATUS_PRIVILEGE_NOT_HELD:
  case NT::STATUS_RESOURCE_NOT_OWNED:
  case NT::STATUS_CANNOT_MAKE:
    return PX::EPERM;

//  case NT::STATUS_EAS_NOT_SUPPORTED:
//    return PX::ENOTSUP;

  case NT::STATUS_CTL_FILE_NOT_SUPPORTED:
  case NT::STATUS_NOT_SUPPORTED:
  case NT::STATUS_INVALID_NETWORK_RESPONSE:
  case NT::STATUS_NOT_SERVER_SESSION:
  case NT::STATUS_NOT_CLIENT_SESSION:
  case NT::STATUS_WMI_NOT_SUPPORTED:
    return PX::ENOSYS;

  case NT::STATUS_PROCEDURE_NOT_FOUND:
  case NT::STATUS_ENTRYPOINT_NOT_FOUND:
  case NT::STATUS_DRIVER_ENTRYPOINT_NOT_FOUND:
    return PX::ESRCH;

  case NT::STATUS_FILE_INVALID:
  case NT::STATUS_MAPPED_FILE_SIZE_ZERO:
    return PX::ENXIO;

  case NT::STATUS_INSUFFICIENT_RESOURCES:
    return PX::EFBIG;

  case NT::STATUS_MEDIA_WRITE_PROTECTED:
  case NT::STATUS_TOO_LATE:
    return PX::EROFS;

  case NT::STATUS_PIPE_DISCONNECTED:
  case NT::STATUS_PIPE_LISTENING:
    return PX::ECOMM;

  case NT::STATUS_PIPE_CLOSING:
  case NT::STATUS_PIPE_EMPTY:
  case NT::STATUS_PIPE_BROKEN:
    return PX::EPIPE;

  case NT::STATUS_REMOTE_NOT_LISTENING:
  case NT::STATUS_REMOTE_RESOURCES:
    return PX::ENONET;

  case NT::STATUS_DUPLICATE_NAME:
  case NT::STATUS_ADDRESS_ALREADY_EXISTS:
    return PX::ENOTUNIQ;

  case NT::STATUS_NOT_SAME_DEVICE:
    return PX::EXDEV;

  case NT::STATUS_DIRECTORY_NOT_EMPTY:
    return PX::ENOTEMPTY;

  case NT::STATUS_NOT_A_DIRECTORY:
    return PX::ENOTDIR;

  case NT::STATUS_NAME_TOO_LONG:
    return PX::ENAMETOOLONG;

  case NT::STATUS_TOO_MANY_OPENED_FILES:
    return PX::EMFILE;

  case NT::STATUS_POSSIBLE_DEADLOCK:
    return PX::EDEADLOCK;

  case NT::STATUS_CONNECTION_REFUSED:
    return PX::ECONNREFUSED;

  case NT::STATUS_TOO_MANY_LINKS:
    return PX::EMLINK;

   default: break;   // Why there is a warning about this? Same nonsense as explicit fallthrough?
  }
 if(!(err & 0xC0000000))return PX::NOERROR;    // Unknown Status, not an error
 return Default;
}
//------------------------------------------------------------------------------------


};

using NTX = NTSYX<uint>;
//============================================================================================================
