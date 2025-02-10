
#pragma once

// Put here everything that useful only for Windows system. 
// Like work with mapped PE files and functions that is required to do the platform initialization
// Replace with https://github.com/can1357/linux-pe ?
// 
//============================================================================================================
template<typename PHT, bool RawPE=false> struct NUFPE: public NFMTPE<PHT,RawPE>    // NUFmtPE
{
// https://stackoverflow.com/questions/12561544/two-phase-name-lookup-for-c-templates-why
// https://learn.microsoft.com/en-us/cpp/build/reference/zc-twophase?view=msvc-170
// MSVC: /Zc:twoPhase-         /permissive-
// CLANG: -fdelayed-template-parsing   -fno-delayed-template-parsing
 using BPE = NFMTPE<PHT,RawPE>;       // Dumb C++ pretends to be even dumbier(Two phase template name lookup - Why a language standard dictates a way a compiler should be implemented?)!
 using typename BPE::SDosHdr;
 using typename BPE::SWinHdr;
 using typename BPE::SImpDir;
 using typename BPE::SExpDir;
 using typename BPE::SDataDir;
 using typename BPE::SFileHdr;
 using typename BPE::SRelocDesc;
 using typename BPE::SSecHdr;
 using typename BPE::SImportThunk;  
 using typename BPE::SImportByName;
 using typename BPE::SRichRec;
 using typename BPE::EImgRelBased;

 using BPE::GetWinHdr;
 using BPE::GetProcAddr; 
 using BPE::BaseOfModule; 
 using BPE::GetOffsInSec; 
 using BPE::RvaToFileOffset; 
 using BPE::IsValidHeaderPE;
 using BPE::CalcModuleSizeRaw;
//------------------------------------------------------------------------------------------------------------
static bool FixRelocations(vptr ModuleBase, vptr TargetBase, bool Raw=false)   // Not completely correct?
{
 auto WinHdr = GetWinHdr(ModuleBase);
 SDataDir* ReloctDir = &WinHdr->OptionalHeader.DataDirectories.FixUpTable;
 if(!ReloctDir->DirectoryRVA || !ReloctDir->DirectorySize)return false;
 size_t ImageBase = BaseOfModule(ModuleBase);
 size_t LoadDelta = (size_t)TargetBase - ImageBase;
 uint8*  RelocPtr = &((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, ReloctDir->DirectoryRVA)];
 for(uint32 RelOffs=0;RelOffs < ReloctDir->DirectorySize;)
  {
   SRelocDesc* CurRelBlk = (SRelocDesc*)&RelocPtr[RelOffs];
   uint8* BasePtr = &((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, CurRelBlk->BaseRVA)];
   for(uint32 RIdx=0,RTotal=CurRelBlk->Count();RIdx < RTotal;RIdx++)
    {   
     uint8 Type = CurRelBlk->Records[RIdx].Type;   // NOTE: 'switch()' makes the code Base dependant (Remember Reflective Injection)
     if(Type == EImgRelBased::REL_HIGHLOW)     // x32
      {
       uint32* Value = (uint32*)&BasePtr[CurRelBlk->Records[RIdx].Offset];     
       if(Raw)*Value = uint32(size_t((uint8*)TargetBase + RvaToFileOffset(ModuleBase, (*Value - ImageBase))));    // Direct x32 address      // Resolve to a Raw address
         else *Value += LoadDelta;           
      }              
     else if(Type == EImgRelBased::REL_DIR64)  // x64
      {
       uint64* Value = (uint64*)&BasePtr[CurRelBlk->Records[RIdx].Offset];     
       if(Raw)*Value = uint64(size_t((uint8*)TargetBase + RvaToFileOffset(ModuleBase, (*Value - ImageBase))));    // Direct x32 address      // Resolve to a Raw address
         else *Value += LoadDelta;           
      }
//     else if(Type != REL_ABSOLUTE){DBGMSG("Unsupported reloc type: %u", Type);}    // 11:IMAGE_REL_BASED_HIGH3ADJ(3xWORD) and 4:IMAGE_REL_BASED_HIGHADJ(2xWORD)  // Can`t log if self relocating
    }
   RelOffs += CurRelBlk->BlkSize;
  }
 return true;
}
//------------------------------------------------------------------------------------------------------------
// Calculate aligned offset of reloc of size T which was encountered at byte offset 'Offset'
static int GetRelocOffset(uint8* DefDataPtr, uint8* AltDataPtr, uint32 Offset, PHT DefBase, PHT BaseDelta)
{
 int StepBk  = 2;   // Module loading granularity is 0sx10000 so low 2 bytes is never changed by relocs
 int TotBk   = sizeof(PHT)-1;  // 3 or 7
 DefDataPtr -= StepBk;
 AltDataPtr -= StepBk;
 for(;StepBk <= TotBk;StepBk++,Offset--)
  {
   PHT DefVal = *(PHT*)&DefDataPtr[Offset];
   PHT AltVal = *(PHT*)&AltDataPtr[Offset] - BaseDelta;
   if(AltVal == DefVal)return StepBk;   // The difference is a relocated value, return its offset
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// This function rebuilds all relocs
// If we dumped an uncorrupted and unresolved IAT, entries there are RVA to ORD:Name names table yet and affected by relocs (???)
// DefModData is updated target
// NOTE: Expect differences in TlsIndex which is referred by TlsDirectory and security_cookie which is referred by LoadConfigDirectory
//
static int RebuildRelocs(uint8* DefModData, size_t DefModSize, uint8* AltModData, size_t AltModSize, size_t AltModBase, int SkipSecFirst=0, int SkipSecLast=0, int RelocSecIdx=-1, bool IgnoreMismatch=false, bool LogRecs=false)
{
 DBGMSG("Entering...");
 SDosHdr* DosHdr = (SDosHdr*)DefModData;
 SWinHdr* WinHdr = (SWinHdr*)&DefModData[DosHdr->OffsetHeaderPE];
 if((SkipSecFirst+SkipSecLast) >= WinHdr->FileHeader.SectionsNumber)return -1;
 if(RelocSecIdx < 0)RelocSecIdx = WinHdr->FileHeader.SectionsNumber-(SkipSecLast+1);
 uint32   HdrLen   = DosHdr->OffsetHeaderPE + WinHdr->FileHeader.HeaderSizeNT + sizeof(SFileHdr) + sizeof(uint32);
 SSecHdr* SecArr   = (SSecHdr*)&((uint8*)DefModData)[HdrLen];
 SSecHdr* RelocSec = &SecArr[RelocSecIdx];   
 size_t ImageBase  = BaseOfModule(DefModData);
 size_t LoadDelta  = (size_t)AltModBase - ImageBase;

 int LastRASec  = WinHdr->FileHeader.SectionsNumber - RelocSecIdx - 1; // Last section, affected by relocs   

 uint8* RelocSecBeg = &DefModData[RelocSec->VirtualOffset];  // File offset or RVA (?)
 //uint8* RelocSecEnd = RelocSecBeg + RelocSec->PhysicalSize;

 uint32 RelRVATo    = SecArr[LastRASec].VirtualOffset + SecArr[LastRASec].PhysicalSize;
 uint32 RelRVAFrom  = SecArr[SkipSecFirst].VirtualOffset;
 uint32 LstBlkRVA   = 0;
 uint32 RelRecIdx   = 0;

 SRelocDesc* RDesc = nullptr; 
 int TotalRelocSize = 0;
 for(uint32 Offs=RelRVAFrom;Offs < RelRVATo;Offs++)    // Compare all bytes
  {
   if(DefModData[Offs] == AltModData[Offs])continue;    // No difference
   int StepBk = GetRelocOffset(DefModData, AltModData, Offs, ImageBase, LoadDelta);
   if(StepBk == 0)
    {
     LOGMSG("No reloc match for diff at %08X",Offs);     // A difference encountered between dumps which is not caused by relocation!
     if(IgnoreMismatch)continue;
     return -2;
    }    
   Offs -= StepBk;  // Align to reloc
   uint32 CurBlkRVA = Offs & 0xFFFFF000;   // 4k per reloc block
   if(CurBlkRVA != LstBlkRVA)   // Prepare a new reloc block
    {
     RelRecIdx = 0;
     LstBlkRVA = CurBlkRVA;
     if(RDesc)   // We already had a reloc block
      {
       if(RDesc->BlkSize & 3)       // Add a NULL record to keep all records aligned to uint32
        {
         *(uint16*)&RelocSecBeg[RDesc->BlkSize] = 0;   // Add an unused NULL entry
         RDesc->BlkSize += sizeof(uint16);   // Blocks must be uint32 aligned      
         if(LogRecs){LOGMSG("   NULL");}
        }
       RelocSecBeg += RDesc->BlkSize;
       TotalRelocSize += RDesc->BlkSize;
      }
     RDesc = (SRelocDesc*)RelocSecBeg;
     RDesc->BaseRVA = CurBlkRVA;
     RDesc->BlkSize = sizeof(SRelocDesc);
     if(LogRecs){LOGMSG("New block at %08X: BaseRVA=%08X",RelocSecBeg-DefModData,CurBlkRVA);}
    }
   uint32 Offset = Offs - LstBlkRVA;       // RVA to reloc block offset
   uint32 RType  = (sizeof(PHT) == sizeof(uint32)) ? EImgRelBased::REL_HIGHLOW : EImgRelBased::REL_DIR64;  
   if(LogRecs){LOGMSG("   Type=%u, Offs=%04X, RVA=%08X",RType, Offset, RDesc->BaseRVA + Offset);}
   RDesc->Records[RelRecIdx].Type   = RType;
   RDesc->Records[RelRecIdx].Offset = Offset;
   RDesc->BlkSize += sizeof(uint16);
   RelRecIdx++;
   Offs += sizeof(PHT)-1;   //  3;  // sizeof(uint32)-1
  }
 if(RDesc)
  {
   if(RDesc->BlkSize & 3)
    {
     *(uint16*)&RelocSecBeg[RDesc->BlkSize] = 0;   // Add an unused NULL entry
     RDesc->BlkSize += sizeof(uint16);   // Blocks must be uint32 aligned  
     if(LogRecs){LOGMSG("   NULL");}  
    }
   TotalRelocSize += RDesc->BlkSize;
  }
 LOGMSG("TotalRelocSize: %08X",TotalRelocSize);
 return TotalRelocSize;
}
//------------------------------------------------------------------------------------------------------------
static int LogRelocs(vptr ModBase, size_t ModSize, int SkipSecFirst=0, int SkipSecLast=0, bool Raw=true)       // Raw?????
{
 int RelocSecIdx = -1;   // ??????????????????????????????????????????????????????????
 uint8* DllCopyBase = (uint8*)ModBase;
 SDosHdr* DosHdr = (SDosHdr*)DllCopyBase;
 SWinHdr* WinHdr = (SWinHdr*)&DllCopyBase[DosHdr->OffsetHeaderPE];
 if(RelocSecIdx < 0)RelocSecIdx = WinHdr->FileHeader.SectionsNumber-(SkipSecLast+1);
// size_t ModuleBase = BaseOfModule(DllCopyBase);
 uint32   HdrLen   = DosHdr->OffsetHeaderPE + WinHdr->FileHeader.HeaderSizeNT + sizeof(SFileHdr) + sizeof(uint32);
 SSecHdr* SecArr   = (SSecHdr*)&((uint8*)DllCopyBase)[HdrLen];
 SSecHdr* RelocSec = &SecArr[WinHdr->FileHeader.SectionsNumber-(SkipSecLast+1)];   // Skipping 5 protector`s sections at the end

 int FirstRASec = SkipSecFirst; // First section, affected by relocs     // static const?
 int LastRASec  = WinHdr->FileHeader.SectionsNumber - RelocSecIdx - 1; // Last section, affected by relocs   

 uint32 RelRVATo    = SecArr[LastRASec].VirtualOffset + SecArr[LastRASec].PhysicalSize;
 uint32 RelRVAFrom  = SecArr[FirstRASec].VirtualOffset;
 
 uint8* RelocSecBeg = &DllCopyBase[(Raw)?(RelocSec->PhysicalOffset):(RelocSec->VirtualOffset)];  // File offset or RVA
 uint8* RelocSecEnd = RelocSecBeg + RelocSec->PhysicalSize;

 size_t RelBase = 0;
 for(uint8* BytePtr=RelocSecBeg;BytePtr < RelocSecEnd;)
  {
   SRelocDesc* RDesc = (SRelocDesc*)BytePtr;
   if(!RDesc->BaseRVA && !RDesc->BlkSize){LOGMSG("No more blocks at %08X",(BytePtr-DllCopyBase)); break;}
   LOGMSG("RelBlk at %08X: BaseRVA=%08X, BlkSize=%08X, Count=%u",(BytePtr-DllCopyBase), RDesc->BaseRVA, RDesc->BlkSize, RDesc->Count());
   RelBase += RDesc->BaseRVA;
   for(uint ctr=0,tot=RDesc->Count();ctr < tot;ctr++)
    {
     uint32 Type = RDesc->Records[ctr].Type;
     uint32 Offs = RDesc->Records[ctr].Offset;
     if(!Type && !Offs){OUTMSG("   NULL"); continue;}
     if(Type == EImgRelBased::REL_HIGHLOW)     // x32
      { 
       OUTMSG("   Type=%u, Offs=%04X, RVA=%08X",Type, Offs, RDesc->BaseRVA + Offs);   
      }              
     else if(Type == EImgRelBased::REL_DIR64)  // x64
      { 
       OUTMSG("   Type=%u, Offs=%04X, RVA=%08X",Type, Offs, RDesc->BaseRVA + Offs);          
      }
     else {OUTMSG("   Type=%u, Offs=%04X",Type, Offs); }
     
    }
   BytePtr += RDesc->BlkSize;
  }
 LOGMSG("PE sectons range RVA: %08X - %08X",RelRVAFrom,RelRVATo);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static int LoadRichInfo(uint8* ModuleBase, SRichRec* Recs, uint32* RecsNum, uint32* Offset=nullptr, bool* IsChkValid=nullptr)
{
 if(!IsValidHeaderPE(ModuleBase))return -1;
 SDosHdr* DosHdr  = (SDosHdr*)ModuleBase;
 uint32 EndOffs = DosHdr->OffsetHeaderPE;      // Rich is before PE
 if(EndOffs < sizeof(SDosHdr))return -1;
 if(EndOffs > 1024)EndOffs = 1024;  
 uint32* EndPtr = (uint32*)&((uint8*)ModuleBase)[EndOffs];
 uint32  XorKey = 0;
 uint32  Sign   = 'hciR';
 uint32* RBeg   = nullptr;
 uint32* REnd   = nullptr;
 do
  {
   EndPtr--;
   if(*EndPtr == Sign)
    {
     if(REnd){RBeg=EndPtr; break;}
     XorKey = EndPtr[1];
     REnd   = EndPtr;
     Sign   = 'SnaD' ^ XorKey;
    }
  }
   while(EndPtr > (uint32*)ModuleBase);
 if(!RBeg || !REnd)return -3;
 sint  Counter = ((REnd - RBeg) / 2) - 2;
 uint32 ROffs = (uint8*)RBeg - (uint8*)ModuleBase;
 if(Counter < 0)return -4;
 if(RecsNum)*RecsNum = Counter;
 if(Offset)*Offset = ROffs;
 RBeg += 4;
 uint32* DstPtr  = (uint32*)Recs;
 uint32 RichSize = ((((XorKey >> 5) % 3) + Counter) * 8) + 0x20;   // Counter not includes first 4 uint32s
 for(long ctr=0;ctr < Counter;ctr++)
 {
  *(DstPtr++) = *(RBeg++) ^ XorKey;
  *(DstPtr++) = *(RBeg++) ^ XorKey;
 }
 if(IsChkValid)
  {
   uint32 OrgXorKey = ROffs;
   for(uint Idx=0;Idx < ROffs;Idx++)OrgXorKey += ((Idx < 0x3C)||(Idx > 0x3F))?(RotL((uint32)ModuleBase[Idx], Idx)):(0);    // Skipping OffsetHeaderPE (Slower but without a buffer)
   for(sint Idx=0;Idx < Counter;Idx++)
    {
     uint32* Rec = (uint32*)&Recs[Idx];
     OrgXorKey += RotL(Rec[0], (uint8)Rec[1]);
    }
   *IsChkValid = (OrgXorKey == XorKey);
  }
 return RichSize;
}
//------------------------------------------------------------------------------------------------------------
static int SaveRichInfo(uint8* ModuleBase, SRichRec* Recs, uint32 RecsNum, uint32 Offset)
{
 if(!IsValidHeaderPE(ModuleBase))return -1;
 SDosHdr* DosHdr = (SDosHdr*)ModuleBase;
 SWinHdr* WinHdr = (SWinHdr*)&(ModuleBase[DosHdr->OffsetHeaderPE]);

 uint32 OldOffsPE = DosHdr->OffsetHeaderPE;
 uint32 XorKey = Offset;
 DosHdr->OffsetHeaderPE = 0;    // Must be set to 0 before calculation
 for(uint32 Idx=0;Idx < Offset;Idx++)XorKey += RotL((uint32)ModuleBase[Idx], Idx);
 for(uint32 Idx=0;Idx < RecsNum;Idx++)
  {
   uint32* Rec = (uint32*)&Recs[Idx];
   XorKey += RotL(Rec[0], (uint8)Rec[1]);
  }
 uint32 RichSize = ((((XorKey >> 5) % 3) + RecsNum) * 8) + 0x20;
 uint32* OPtr    = (uint32*)&ModuleBase[Offset];

 uint32 NewOffsPE = Offset + RichSize;
 uint32 SizePE    = WinHdr->OptionalHeader.SizeOfHeaders - OldOffsPE;
 if(NewOffsPE < OldOffsPE)
  {
   memmove(&ModuleBase[NewOffsPE], &ModuleBase[OldOffsPE], SizePE);
   memset(&ModuleBase[NewOffsPE+SizePE],0,OldOffsPE-NewOffsPE);
  }
   else if(NewOffsPE > OldOffsPE)
    {
     SizePE -= (NewOffsPE - OldOffsPE);
     memmove(&ModuleBase[NewOffsPE], &ModuleBase[OldOffsPE], SizePE);
    }

 DosHdr->OffsetHeaderPE = NewOffsPE;
 memset(OPtr, 0, RichSize);  
 *(OPtr++) = XorKey ^ 'SnaD';
 *(OPtr++) = XorKey;
 *(OPtr++) = XorKey;
 *(OPtr++) = XorKey;
 for(uint32 Idx=0,Tot=RecsNum*2;Idx < Tot;Idx++)*(OPtr++) = ((uint32*)Recs)[Idx] ^ XorKey; 
 *(OPtr++) = 'hciR';
 *(OPtr++) = XorKey;
 return RichSize;
}
//------------------------------------------------------------------------------------------------------------
static vptr GetProcAddress(vptr ModuleBase, const achar* ApiName)    // With forwarders support      
{
 achar* Forwarder = nullptr;
 vptr Addr = GetProcAddr(ModuleBase, ApiName, &Forwarder);
 if(Addr && Forwarder)
  {
   int DotPos = 0;
   char StrBuf[256];
   for(int ctr=0;Forwarder[ctr];ctr++)
     if(Forwarder[ctr]=='.')DotPos = ctr;  
   memcpy(&StrBuf,Forwarder,DotPos);
   StrBuf[DotPos] = 0;
   ModuleBase = NTX::LdrGetModuleBase(StrBuf);    // GetModuleHandleA(StrBuf);          // TODO: Use PEB  // How to decode 'api-ms-win-core-com-l1-1-0.dll' ?
   if(!ModuleBase)return nullptr;                 // Should it be able to load DLLs?
   return GetProcAddr(ModuleBase, &Forwarder[DotPos+1]);
  }
 return Addr;
}
//------------------------------------------------------------------------------------------------------------
static int ParseForwarderStr(const achar* InStr, achar* OutDllName, achar* OutProcName)    //  "NTDLL.RtlDeleteCriticalSection",  "NTDLL.#491"
{         
 int namctr = 0;
 for(;InStr[namctr] && (InStr[namctr] != '.');namctr++)OutDllName[namctr] = InStr[namctr];
 OutDllName[namctr++] = '.';
 OutDllName[namctr+0] = 'd';
 OutDllName[namctr+1] = 'l';
 OutDllName[namctr+2] = 'l';
 OutDllName[namctr+3] = 0;
 if(InStr[namctr] == '#')
  {
   *OutProcName = 0;
   return NCNV::DecStrToNum<int>(&InStr[++namctr]);
  }
   else 
    {
     int ctr=0;
     for(;InStr[namctr];ctr++,namctr++)OutProcName[ctr] = InStr[namctr];
     OutProcName[ctr] = 0;
    }
 return -1;
}
//------------------------------------------------------------------------------------------------------------
static int ResolveImportsForMod(achar* ImpModName, uint8* ModuleBase, uint8* ExpModBase, vptr pNtProtVMem=nullptr, vptr pLdrLoadDll=nullptr)   // Duplicate???????????????????
{
 SDosHdr* DosHdr = (SDosHdr*)ModuleBase;
 SWinHdr* WinHdr = (SWinHdr*)&ModuleBase[DosHdr->OffsetHeaderPE];
 SDataDir* ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 if(!ImportDir->DirectoryRVA)return 0;
 SImpDir* Import = (SImpDir*)&ModuleBase[ImportDir->DirectoryRVA];

 PHT OMask = ((PHT)1 << ((sizeof(PHT)*8)-1));
 vptr   BaseAddress = nullptr;
 size_t RegionSize  = 0;
 NT::ULONG  OldProtect = 0;
 for(uint32 tctr=0;Import[tctr].AddressTabRVA;tctr++)    
  {
   achar* ModName = (achar*)&ModuleBase[Import[tctr].ModuleNameRVA];
   if(!NSTR::IsStrEqualCI(ModName,ImpModName))continue;     // May by more than one entry for a same module
   DBGMSG("Updating import for '%s'",ImpModName);
   SImportThunk* Table = (SImportThunk*)&ModuleBase[Import[tctr].LookUpTabRVA];
   SImportThunk* LtRVA = (SImportThunk*)&ModuleBase[Import[tctr].AddressTabRVA];
   for(uint32 actr=0;Table[actr].Value;actr++)
    {
     bool OnlyOrd = (Table[actr].Value & OMask);
     vptr PAddr;
     if(OnlyOrd)   // Have only API ordinal
      {	                                                         
       PHT Ord = Table[actr].Value & ~OMask;
       DBGMSG("Ordinal: %u",Ord);
       PAddr = GetProcAddr(ExpModBase, (achar*)Ord);
      }
       else    // Have an import API name
        {
         SImportByName* INam = (SImportByName*)&ModuleBase[Table[actr].Value]; 
         DBGMSG("Name: %s",(achar*)&INam->Name);
         achar* Forwarder = nullptr;
         PAddr = GetProcAddr(ExpModBase, (achar*)&INam->Name, &Forwarder);   
         DBGMSG("New Address: %p",PAddr);
         if(Forwarder)
          {
           uint8 OutDllName[PATH_MAX]; 
           uint8 OutProcName[PATH_MAX];
           achar* PNamePtr;
           uint32 Ord = ParseForwarderStr(Forwarder, (achar*)&OutDllName, (achar*)&OutProcName);  
           if(!OutProcName[0])PNamePtr = (achar*)size_t(Ord);
            else PNamePtr = (achar*)&OutProcName;
           uint8* ImpModBaseF = pLdrLoadDll?( (uint8*)NTX::LdrLoadLibrary((achar*)&OutDllName,pLdrLoadDll) ):( (uint8*)NTX::LdrGetModuleBase((achar*)&OutDllName) );          //   GetModuleHandleA((achar*)&OutDllName);         // <<<<<<<<<<<<<<<<<<<<<<<<<<
           if(!ImpModBaseF)return -1;
           PAddr = GetProcAddr(ImpModBaseF, PNamePtr, &Forwarder);    // No more forwarding?
          }
//         if(IsBadWritePtr(&LtRVA[actr].Value,sizeof(vptr))){DBGMSG("Import table is not writable at %p !",&LtRVA[actr].Value); return -2;}      // Make optional?   // Avoid  IsBadReadPtr?
        }  
     if(pNtProtVMem)
      {
       if(OldProtect)   // if(!OldProt && !VirtualProtect(LtRVA,0x1000,PAGE_EXECUTE_READWRITE,&OldProtect)){DBGMSG("VP failed: %u", GetLastError()); return -2;}   // TODO: IAT may span multiple pages
        {
         uint8* Addr = (uint8*)&LtRVA[actr]; 
         if((Addr < (uint8*)BaseAddress) || (&Addr[sizeof(SImportThunk)] >= ((uint8*)BaseAddress+RegionSize)))  // Outside of unprotected block
          {
           ((decltype(NT::NtProtectVirtualMemory)*)pNtProtVMem)(NT::NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);  // Restore last page protection
           OldProtect = 0;    // Reset
          }
        }
       if(!OldProtect)   
        {
         BaseAddress = &LtRVA[actr];
         RegionSize  = sizeof(SImportThunk);  
         if(NT::NTSTATUS stat = ((decltype(NT::NtProtectVirtualMemory)*)pNtProtVMem)(NT::NtCurrentProcess, &BaseAddress, &RegionSize, NT::PAGE_EXECUTE_READWRITE, &OldProtect)){DBGMSG("VP failed: %08X", stat); return -2;}    
        }
      }
     LtRVA[actr].Value = (size_t)PAddr;  
     if(!LtRVA[actr].Value)return -3;  // Leaving OldProt unrestored is OK?
    }
  }
 if(pNtProtVMem && OldProtect)((decltype(NT::NtProtectVirtualMemory)*)pNtProtVMem)(NT::NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);  //    VirtualProtect(LtRVA,0x1000,OldProt,&OldProt);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
enum EFixMod {fmNone,fmEncKeyMsk=0xFF, fmFixSec=0x0100,fmFixImp=0x0200,fmFixRel=0x0400,   fmCryHdr=0x1000,fmCryImp=0x2000,fmCryExp=0x4000,fmCryRes=0x8000,  fmEncMode=0x00010000,fmOwnLDib=0x00040000,fmSelfMov=0x00080000};

static int ResolveImports(uint8* ModuleBase, vptr pLdrLoadDll, uint32 Flags=0)
{
 uint8 EncKey = Flags & fmEncKeyMsk;
 SDosHdr* DosHdr = (SDosHdr*)ModuleBase;
 SWinHdr* WinHdr = (SWinHdr*)&((uint8*)ModuleBase)[DosHdr->OffsetHeaderPE];
// uint32   HdrLen = DosHdr->OffsetHeaderPE + WinHdr->FileHeader.HeaderSizeNT + sizeof(SFileHdr) + sizeof(uint32);
 SDataDir* ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 if(!ImportDir->DirectoryRVA)return 1;     // Not Present
 SImpDir* Import = (SImpDir*)&ModuleBase[ImportDir->DirectoryRVA];
 PHT OMask = ((PHT)1 << ((sizeof(PHT)*8)-1));
 achar* PrvMNamePtr = nullptr;
 achar* PrvImpModBaseF = nullptr;
 uint8* ImpModBase = nullptr;
 for(uint32 tctr=0;Import[tctr].AddressTabRVA;tctr++)
  {
   uint8  MName[256];  // Probably not enough for impoorting templates
   achar* MNamePtr = (achar*)&ModuleBase[Import[tctr].ModuleNameRVA];
   achar* CurMNamePtr = MNamePtr;
   if(EncKey){NSTR::DecryptSimple(MNamePtr,(achar*)&MName, EncKey); MNamePtr=(achar*)&MName;}   
   if(Flags & fmOwnLDib)ImpModBase = nullptr;  // TODO: Own Load library (Hidden)
    else 
     {
      if(CurMNamePtr != PrvMNamePtr)
       {
        if(pLdrLoadDll)ImpModBase = (uint8*)NTX::LdrLoadLibrary(MNamePtr, pLdrLoadDll);
          else ImpModBase = (uint8*)NTX::LdrGetModuleBase(MNamePtr);
        PrvMNamePtr = CurMNamePtr;
       }
     }
   if(!ImpModBase)return -1;                // No logging in case of self import resolving
   SImportThunk* Table = (SImportThunk*)&ModuleBase[Import[tctr].LookUpTabRVA];
   SImportThunk* LtRVA = (SImportThunk*)&ModuleBase[Import[tctr].AddressTabRVA];
   for(uint32 actr=0;Table[actr].Value;actr++)
    {
     bool OnlyOrd = (Table[actr].Value & OMask);
     if(OnlyOrd)   // Have only API ordinal
      {	                                                         
       PHT Ord = Table[actr].Value & ~OMask;
       LtRVA[actr].Value = (size_t)GetProcAddr(ImpModBase, (achar*)Ord);
      }
       else    // Have an import API name
        {
  	     SImportByName* INam = (SImportByName*)&ModuleBase[Table[actr].Value];
         uint8  PName[256];
         achar* PNamePtr  = (achar*)&INam->Name;
         if(EncKey){NSTR::DecryptSimple(PNamePtr,(achar*)&PName, EncKey); PNamePtr=(achar*)&PName;}     
         achar* Forwarder = nullptr;
         vptr PAddr = GetProcAddr(ImpModBase, PNamePtr, &Forwarder);          
         if(Forwarder)
          {
           uint8 OutDllName[PATH_MAX]; 
           uint8 OutProcName[PATH_MAX];
           uint32 Ord = ParseForwarderStr(Forwarder, (achar*)&OutDllName, (achar*)&OutProcName);  
           if(!OutProcName[0])PNamePtr = (achar*)size_t(Ord);
             else PNamePtr = (achar*)&OutProcName;
           uint8* ImpModBaseF = nullptr;
           achar* CurImpModBaseF = Forwarder;
           if(Flags & fmOwnLDib)ImpModBaseF = nullptr;  // TODO: Own Load library (Hidden)
            else 
             {
              if(CurImpModBaseF != PrvImpModBaseF)
               {
                if(pLdrLoadDll)ImpModBaseF = (uint8*)NTX::LdrLoadLibrary((achar*)&OutDllName, pLdrLoadDll);
                  else ImpModBaseF = (uint8*)NTX::LdrGetModuleBase((achar*)&OutDllName);
                PrvImpModBaseF = CurImpModBaseF;
               }
             }
           if(!ImpModBaseF)return -2;
           PAddr = GetProcAddr(ImpModBaseF, PNamePtr, &Forwarder);    // No more forwarding?
          }
         LtRVA[actr].Value = (size_t)PAddr;        
	    }
     if(!LtRVA[actr].Value)return -3;
    } 
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
_ninline static uint32 MoveSections(uint32 SecArrOffs, uint32 TotalSecs, uint8* ModuleBase, uint8* Headers=nullptr, uint32 HdrSize=0)  // Must not be any function calls in body of this function (watch for memset optimizations for cycles!)
{
 if(!Headers || !HdrSize)Headers = ModuleBase;
 SSecHdr* SecArr = (SSecHdr*)&Headers[SecArrOffs];
 uint8** pRetAddr = (uint8**)_AddressOfReturnAddress();      // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 uint32 RetFix = 0;
 volatile int MemSetKill = 0;
 for(int idx = TotalSecs-1;idx >= 0;idx--)  // Compatible with any normal linker that keeps sections in order  // All section moved forward
  {
   if(HdrSize && ((uint8*)&SecArr[idx] >= &Headers[HdrSize]))SecArr = (SSecHdr*)&ModuleBase[SecArrOffs];   // Continue at original header
   SSecHdr* CurSec = &SecArr[idx];
   uint8* Dst   = &ModuleBase[CurSec->VirtualOffset]; 
   uint8* Src   = &ModuleBase[CurSec->PhysicalOffset];
   size_t Size = CurSec->PhysicalSize;
   size_t ALen = Size/sizeof(size_t);
   size_t BLen = Size%sizeof(size_t);
   if(!RetFix && (*pRetAddr >= Src) && (*pRetAddr < &Src[Size]))RetFix = (Dst - Src);  // If RetAddr in current section being moved
   for(size_t ctr=Size-1;BLen;ctr--,BLen--,MemSetKill++)((char*)Dst)[ctr] = ((char*)Src)[ctr];  
   for(size_t ctr=ALen-1;ALen;ctr--,ALen--,MemSetKill++)((size_t*)Dst)[ctr] = ((size_t*)Src)[ctr];                // Copy of memcpy to avoid an inlining problems
   if(CurSec->VirtualSize > CurSec->PhysicalSize)                  // Fill ZERO space
    {
     Dst  = &ModuleBase[CurSec->VirtualOffset+CurSec->PhysicalSize]; 
     Size = CurSec->VirtualSize - CurSec->PhysicalSize;
     ALen = Size/sizeof(size_t);
     BLen = Size%sizeof(size_t);
     for(size_t ctr=0;ctr < ALen;ctr++,MemSetKill++)((size_t*)Dst)[ctr] = 0; 
     for(size_t ctr=(ALen*sizeof(size_t));ctr < Size;ctr++,MemSetKill++)((char*)Dst)[ctr] = 0;  
    }
  }
 if(RetFix)*pRetAddr += RetFix;
 return RetFix;
}
//------------------------------------------------------------------------------------------------------------
static int CryptSensitiveParts(uint8* ModuleBase, uint32 Flags, bool Raw)
{
 uint8 EncKey = Flags & fmEncKeyMsk;
 if((Flags & (fmCryHdr|fmEncMode)) == fmCryHdr)   // Decrypt header first
  {
   uint32 Offs    = 0;
   uint32 EndOffs = sizeof(SDosHdr);
   for(;Offs < EndOffs;Offs++)ModuleBase[Offs] = NSTR::DecryptByteWithIdx(ModuleBase[Offs], EncKey, Offs);
   SDosHdr* DosHdr = (SDosHdr*)ModuleBase;
   EndOffs = DosHdr->OffsetHeaderPE + sizeof(SWinHdr);
   for(;Offs < EndOffs;Offs++)ModuleBase[Offs] = NSTR::DecryptByteWithIdx(ModuleBase[Offs], EncKey, Offs);
   SWinHdr* WinHdr = (SWinHdr*)&ModuleBase[DosHdr->OffsetHeaderPE];
   EndOffs = WinHdr->OptionalHeader.SizeOfHeaders;
   for(;Offs < EndOffs;Offs++)ModuleBase[Offs] = NSTR::DecryptByteWithIdx(ModuleBase[Offs], EncKey, Offs);    
  }
 SDosHdr* DosHdr = (SDosHdr*)ModuleBase;
 SWinHdr* WinHdr = (SWinHdr*)&((uint8*)ModuleBase)[DosHdr->OffsetHeaderPE];
 //uint32   HdrLen = DosHdr->OffsetHeaderPE + WinHdr->FileHeader.HeaderSizeNT + sizeof(SFileHdr) + sizeof(uint32);
 SDataDir* ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 SDataDir* ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 SDataDir* ResourDir = &WinHdr->OptionalHeader.DataDirectories.ResourceTable;

 if((Flags & fmCryImp) && ImportDir->DirectoryRVA)
  {
   SImpDir* Import = (SImpDir*)&ModuleBase[(Raw)?(RvaToFileOffset(ModuleBase,ImportDir->DirectoryRVA)):(ImportDir->DirectoryRVA)];
   PHT OMask = ((PHT)1 << ((sizeof(PHT)*8)-1));
   for(uint32 tctr=0;Import[tctr].AddressTabRVA;tctr++)
    {
     achar* MNamePtr = (achar*)&ModuleBase[(Raw)?(RvaToFileOffset(ModuleBase,Import[tctr].ModuleNameRVA)):(Import[tctr].ModuleNameRVA)];        
     if(Flags & fmEncMode)NSTR::EncryptSimple(MNamePtr, MNamePtr, EncKey);      
       else NSTR::DecryptSimple(MNamePtr, MNamePtr, EncKey);   
     SImportThunk* Table = (SImportThunk*)&((uint8*)ModuleBase)[(Raw)?(RvaToFileOffset(ModuleBase,Import[tctr].LookUpTabRVA)):(Import[tctr].LookUpTabRVA)];
     for(uint32 actr=0;Table[actr].Value;actr++)
      {
       if(Table[actr].Value & OMask)continue; // No name, only ordinal
  	   SImportByName* INam = (SImportByName*)&ModuleBase[(Raw)?(RvaToFileOffset(ModuleBase,Table[actr].Value)):(Table[actr].Value)];        
       if(Flags & fmEncMode)NSTR::EncryptSimple((achar*)&INam->Name, (achar*)&INam->Name, EncKey);          
        else NSTR::DecryptSimple((achar*)&INam->Name, (achar*)&INam->Name, EncKey);  
      } 
    }
  }
 if((Flags & fmCryExp) && ExportDir->DirectoryRVA)
  {
   SExpDir* Export = (SExpDir*)&ModuleBase[GetOffsInSec(ModuleBase, ExportDir->DirectoryRVA)];
   achar*  ModName = (achar*)&ModuleBase[GetOffsInSec(ModuleBase, Export->NameRVA)]; 
   if(Flags & fmEncMode)NSTR::EncryptSimple(ModName, ModName, EncKey);            
     else NSTR::DecryptSimple(ModName, ModName, EncKey);            
   for(uint32 ctr=0;ctr < Export->NamePointersNumber;ctr++)
    {   
     uint32 Offs = GetOffsInSec(ModuleBase, Export->NamePointersRVA);     
     uint32 rva  = (((uint32*)&ModuleBase[Offs])[ctr]);                
     achar* CurProcName = (achar*)&ModuleBase[GetOffsInSec(ModuleBase, rva)];         
     if(Flags & fmEncMode)NSTR::EncryptSimple(CurProcName, CurProcName, EncKey);     
       else NSTR::DecryptSimple(CurProcName, CurProcName, EncKey);     
    }
  }
 if((Flags & fmCryRes) && ResourDir->DirectoryRVA)
  {
   uint8* ResBase = &ModuleBase[GetOffsInSec(ModuleBase,ResourDir->DirectoryRVA)];
   if(Flags & fmEncMode){for(uint32 ctr=0;ctr < ResourDir->DirectorySize;ctr++)ResBase[ctr] = NSTR::EncryptByteWithIdx(ResBase[ctr], EncKey, ctr);}
     else {for(uint32 ctr=0;ctr < ResourDir->DirectorySize;ctr++)ResBase[ctr] = NSTR::DecryptByteWithIdx(ResBase[ctr], EncKey, ctr);}      
  }
 if((Flags & (fmCryHdr|fmEncMode)) == (fmCryHdr|fmEncMode))
  {
   uint32 HdrLen = WinHdr->OptionalHeader.SizeOfHeaders;
   for(uint32 ctr=0;ctr < HdrLen;ctr++)ModuleBase[ctr] = NSTR::EncryptByteWithIdx(ModuleBase[ctr], EncKey, ctr);
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
_ninline static int FixUpModuleInplace(uint8*& ModuleBase, vptr pNtDll, uint32& Flags, uint32* pRetFix=nullptr)   // Buffer at ModuleBase must be large enough to contain all sections
{
 struct SHolder { _ninline static uint8* RetAddrProc(void) {return (uint8*)GETRETADDR();} };    // Helps to avoid problems with inlining of functions, called from a thread`s EP (Or we may get address in ntdll.dll instead of our module)

 if(!ModuleBase)    // Will move Self if Base is not specified
  {
   size_t ThisModBase = (size_t)SHolder::RetAddrProc();  
   if(Flags & fmCryHdr)   // Search for encrypted PE header
    {
     for(ThisModBase &= MEMGRANSIZE-1;;ThisModBase -= MEMGRANSIZE)   // May crash if header is incorrect
      {
       uint8  Header[0x400];
       uint8  EncKey = Flags & fmEncKeyMsk;
       uint32 Offs    = 0;
       uint32 EndOffs = sizeof(SDosHdr);
       memcpy(&Header,(uint8*)ThisModBase,sizeof(Header));
       for(;Offs < EndOffs;Offs++)Header[Offs] = NSTR::DecryptByteWithIdx(Header[Offs], EncKey, Offs);
       SDosHdr* XDosHdr = (SDosHdr*)&Header;
       if((XDosHdr->FlagMZ != BPE::SIGN_MZ) || (XDosHdr->OffsetHeaderPE < sizeof(SDosHdr)) || (XDosHdr->OffsetHeaderPE >= 0x400))continue;
       EndOffs = XDosHdr->OffsetHeaderPE + sizeof(SWinHdr);
       for(;Offs < EndOffs;Offs++)Header[Offs] = NSTR::DecryptByteWithIdx(Header[Offs], EncKey, Offs);
       SWinHdr* XWinHdr = (SWinHdr*)&Header[XDosHdr->OffsetHeaderPE];
       if((XWinHdr->FlagPE != BPE::SIGN_PE))continue;
       break;
      }
    }
     else
      {
       for(ThisModBase &= MEMGRANSIZE-1;;ThisModBase -= MEMGRANSIZE)   // May crash if header is incorrect
        {
         SDosHdr* XDosHdr = (SDosHdr*)ThisModBase;
         if((XDosHdr->FlagMZ != BPE::SIGN_MZ) || (XDosHdr->OffsetHeaderPE < sizeof(SDosHdr)) || (XDosHdr->OffsetHeaderPE >= 0x400))continue;
         SWinHdr* XWinHdr = (SWinHdr*)&((uint8*)ThisModBase)[XDosHdr->OffsetHeaderPE];
         if((XWinHdr->FlagPE != BPE::SIGN_PE))continue;
         break;
        }
      }
   Flags |= fmSelfMov;
   ModuleBase = (uint8*)ThisModBase;
  }
 SDosHdr* DosHdr = (SDosHdr*)ModuleBase;
 if((Flags & fmEncKeyMsk) && (Flags & (fmCryHdr|fmCryImp|fmCryExp|fmCryRes)))
  {
   CryptSensitiveParts(ModuleBase, Flags, (Flags & fmFixSec));   // fmFixSec == Raw module?
   if(ModuleBase[1] != 'Z')return -1;        // Invalid
   if(*ModuleBase != 'M')*ModuleBase = 'M';  // May be invalidated on purpose 
  }
 SWinHdr* WinHdr = (SWinHdr*)&((uint8*)ModuleBase)[DosHdr->OffsetHeaderPE];
 uint32 RetFix = 0;
 if(Flags & fmFixSec)         // A Raw module assumed as input
  {
   uint32 SecArrOffs = DosHdr->OffsetHeaderPE + WinHdr->FileHeader.HeaderSizeNT + sizeof(SFileHdr) + sizeof(uint32);
   uint32 SecsNum    = WinHdr->FileHeader.SectionsNumber;
   if(!(Flags & fmSelfMov))   // Check if this code is inside of this PE image to move
    {
     uint8* ThisProcAddr = SHolder::RetAddrProc();  
     size_t RawSize = CalcModuleSizeRaw(ModuleBase);
     if((ThisProcAddr >= ModuleBase)&&(ThisProcAddr < &ModuleBase[RawSize]))Flags |= fmSelfMov;    // Inside
    }
   if(Flags & fmSelfMov)
    {
     uint8* MProcAddr = (uint8*)&MoveSections;    // On x64 it will be a correct address of 'MoveSections' because of usage of LEA instruction for RIP addressing
#ifndef _AMD64_ 
     if((Flags & fmFixRel) && WinHdr->OptionalHeader.DataDirectories.FixUpTable.DirectoryRVA)MProcAddr = &ModuleBase[RvaToFileOffset(ModuleBase,(MProcAddr - (uint8*)BaseOfModule(ModuleBase)))];  // Fails if relocs already fixed  // Fix section in a Raw module image    // Assume that &MoveSections originally will be as for default image base
      else MProcAddr = &ModuleBase[RvaToFileOffset(ModuleBase, (MProcAddr - (uint8*)ModuleBase))];
#endif  
     uint8 Header[0x400];  // Must be enough to save a full PE header 
     memcpy(&Header,ModuleBase,sizeof(Header));   // Save an original PE header
     memcpy(ModuleBase, MProcAddr, sizeof(Header));
     RetFix = ((decltype(&MoveSections))ModuleBase)(SecArrOffs, SecsNum, ModuleBase, (uint8*)&Header, sizeof(Header));  // (UINT (_stdcall *)(UINT,UINT,uint8*,uint8*,UINT))
     memcpy(ModuleBase,&Header,sizeof(Header));   // Put header back
     if(pRetFix)*pRetFix = RetFix;
     if(RetFix)*((uint8**)_AddressOfReturnAddress()) += RetFix;    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }
     else MoveSections(SecArrOffs, SecsNum, ModuleBase);   // Not this module, just move its sections   
  }
 if((Flags & fmFixRel) && WinHdr->OptionalHeader.DataDirectories.FixUpTable.DirectoryRVA)FixRelocations(ModuleBase, ModuleBase, false);   // The module is not raw after MoveSections
 if((Flags & fmFixImp) && WinHdr->OptionalHeader.DataDirectories.ImportTable.DirectoryRVA)  
  {
   uint32 NLdrLoadDll[] = {~0x4C72644Cu, ~0x4464616Fu, ~0x00006C6Cu};  // LdrLoadDll   // TODO: Encrypt by a build time dependent constant ot a hash  
   NLdrLoadDll[0] = ~NLdrLoadDll[0];
   NLdrLoadDll[1] = ~NLdrLoadDll[1];
   NLdrLoadDll[2] = ~NLdrLoadDll[2];
   vptr Proc = GetProcAddr((uint8*)pNtDll, (achar*)&NLdrLoadDll);
   if(ResolveImports(ModuleBase, Proc, Flags) < 0)return -2;
  }
 return 0;    //((WinHdr->OptionalHeader.EntryPointRVA)?(&ModuleBase[WinHdr->OptionalHeader.EntryPointRVA]):(nullptr));
}
//------------------------------------------------------------------------------------------------------------
/*static int FixUpModuleInplace(uint8*& ModuleBase, vptr pNtDll, uint32 Flags=0, uint32* pRetFix=nullptr)
{
 if(IsValidModuleX64(ModuleBase))return TFixUpModuleInplace<PETYPE64>(ModuleBase, pNtDll, Flags, pRetFix);
 return FixUpModuleInplace<PETYPE32>(ModuleBase, pNtDll, Flags, pRetFix);
}
//------------------------------------------------------------------------------------------------------------
static int CryptSensitiveParts(uint8* ModuleBase, uint32 Flags, bool Raw)
{
 if(IsValidModuleX64(ModuleBase))return TCryptSensitiveParts<PETYPE64>(ModuleBase, Flags, Raw);
 return CryptSensitiveParts<PETYPE32>(ModuleBase, Flags, Raw);      
} */
//------------------------------------------------------------------------------------------------------------
  
};

// Mapped PE
using UFPE    = NUFPE<size_t>;
using UFPE32  = NUFPE<uint32>;
using UFPE64  = NUFPE<uint64>;

// Raw PE
using UFPER   = NUFPE<size_t,true>;
using UFPE32R = NUFPE<uint32,true>;
using UFPE64R = NUFPE<uint64,true>;
//============================================================================================================
