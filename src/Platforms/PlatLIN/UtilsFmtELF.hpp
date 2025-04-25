
#pragma once

// NOTE: User mode only
//============================================================================================================
template<typename PHT=PTRCURRENT> struct NUFELF: public NFMTELF<PHT>    // NUFmtELF
{
// https://github.com/kushaldas/elfutils/blob/master/libdwfl/elf-from-memory.c
// NOTE: Target ELF in memory may be of different arch
// NOTE: Assumed thar PHeaders in memory accessible at the same offset as in the file
// NOTE: Sections will most likely be at the end of file and they are not define memory mapping anyway
//
static uint GetModuleSizeInMem(vptr Base)
{
 if(!ELF::IsValidHeaderELF(Base))return 0;
 size_t MaxOffs = 0;   // Will include BSS
 size_t MinOffs = -1;
 if(((ELF::Elf_Hdr*)Base)->arch == ELF::ELFCLASS64)
  {
   ELF::Elf64_Hdr* hdr = (ELF::Elf64_Hdr*)Base;
   ELF::Elf64_Phdr* phdrs = (ELF::Elf64_Phdr*)((uint8*)Base + hdr->ReadVal(hdr->phoff));
   for(uint16 idx=0,tot=hdr->ReadVal(hdr->phnum);idx < tot;idx++)  // Or add phentsize???
    {
//     DBGDBG("Seg: %p",&phdrs[idx]);
     uint32 type  = hdr->ReadVal(phdrs[idx].type);
     if(type != ELF::PT_LOAD)continue;
     size_t boffs = hdr->ReadVal(phdrs[idx].vaddr);           // Alignment?
     size_t eoffs = boffs + hdr->ReadVal(phdrs[idx].memsz);   // Alignment?
     if(boffs < MinOffs)MinOffs = boffs;
     if(eoffs > MaxOffs)MaxOffs = eoffs;
    }
  }
  else
   {
    ELF::Elf32_Hdr* hdr = (ELF::Elf32_Hdr*)Base;
    ELF::Elf32_Phdr* phdrs = (ELF::Elf32_Phdr*)((uint8*)Base + hdr->ReadVal(hdr->phoff));
    for(uint16 idx=0,tot=hdr->ReadVal(hdr->phnum);idx < tot;idx++)  // Or add phentsize???
     {
//      DBGDBG("Seg: %p",&phdrs[idx]);
      uint32 type  = hdr->ReadVal(phdrs[idx].type);
      if(type != ELF::PT_LOAD)continue;
      size_t boffs = hdr->ReadVal(phdrs[idx].vaddr);           // Alignment?
      size_t eoffs = boffs + hdr->ReadVal(phdrs[idx].memsz);   // Alignment?
      if(boffs < MinOffs)MinOffs = boffs;
      if(eoffs > MaxOffs)MaxOffs = eoffs;
     }
   }
 MaxOffs = AlignFrwdP2(MaxOffs, MEMPAGESIZE);
 MinOffs = AlignBkwdP2(MinOffs, MEMPAGESIZE);
 if(MinOffs){MaxOffs -= MinOffs; MinOffs = 0;}   // Usually default base is 0 but not have to be
 return AlignFrwdP2(MaxOffs - MinOffs, MEMPAGESIZE);
}
//------------------------------------------------------------------------------------------------------------
// Why Linux loads VDSO ELF between Code and Data/BSS of the executable?    (.dynamic + 0x10000 bytes of a hole)
// Looks like segment alignment is 64K by default(ARM) in case of 64K memory pages (VDSO happily maps in this hole bacause it always take only one page anyway)
// NOTE: There are not mapped pages in alignment holes!
// ElfHdr-.rodata-|?|-.text-|?|-.data-.bss
// Why first fragment of the main EXE is in a hole between .rodata and .text ?
// NOTE: Very inefficient way to determine own base address!
// ARM: 'Bus Error' inside a mapped module on an read-only pages which belong to alignment holes (msync fails to detect that! 'write' also have no problem accessing that memory; process_vm_readv works) // Range,Mode,INode is defined
//
static vptr FindModuleByAddr(vptr Addr, size_t* ModSize, bool SafeAddr=true)   // Unsafe!  Pass only an address in code - less likely to have a hole of not mapped pages
{
 static constexpr size_t MaxPages4K = 16;
 size_t PageLen = GetPageSize();
 size_t ptr = AlignBkwdP2((size_t)Addr, PageLen);
 size_t LastElfSize = 0;
 size_t LastElfAddr = 0;
 size_t MaxElfDist  = MaxPages4K * PageLen;  // In case of 64K pages
 size_t NoPageCnt   = 0;
// DBGDBG("Starting from %p",(vptr)ptr);
 for(size_t fpg=ptr;;ptr -= PageLen)   // TODO: Check if the address is valid somehow. For now - just crash
  {
   if(!SafeAddr)    // Initial address mabe safe (By coming from PHDR addr in AUX or any addr inside the module)
    {
     if(!IsValidMemPtr((vptr)ptr, PageLen))
      {
       if(++NoPageCnt > MaxPages4K)break;  // Max 64K alignment holes (Should do more?)
       if(LastElfAddr && (LastElfAddr - ptr) >= MaxElfDist)break;
//       DBGDBG("Skip=%p",(vptr)ptr);
       continue;
      }
     NoPageCnt = 0;
//     DBGDBG("Trying=%p",(vptr)ptr);
      {   // On some systems those hole pages in a file mapping are read-inly but cause Bus Error when read
       uint8 buf[16];
       PX::SIOV vec_l;
       PX::SIOV vec_r;
       vec_r.base = (vptr)ptr;
       vec_l.base = &buf;
       vec_l.size = vec_r.size = sizeof(buf);
       sint res2 = NAPI::process_vm_readv(NAPI::getpid(), &vec_l,1, &vec_r,1, 0);  // Move to IsValidMemPtr? // Any faster way to test the readability?
       if(res2 == -PX::EFAULT)continue;   // Any other error probably means that process_vm_readv is not implemented right
      }
    }
     else SafeAddr = false;
   size_t size = GetModuleSizeInMem((vptr)ptr);
//   DBGDBG("Addr=%p, Size=%p",(vptr)ptr,(vptr)size);
//   DBGDBG("\r\n%#*.32D",256,(vptr)ptr);
   if(!size)continue;
   if((ptr+size) < (size_t)Addr)continue;  // Skip VDSO which could be mapped in a hole
   if(LastElfAddr && memcmp((vptr)ptr,(vptr)LastElfAddr,128))break;   // Already encountered some valid ELF header  // This ELF is different, take a previous one // Hope there will be no random ELFs in the segment gaps, just VDSO and a piece of the same which is mapped here
   LastElfAddr = ptr;
   LastElfSize = size;
   if(ptr == fpg)break;  // Match in the same page - most likekty the addr was in EHDR or PHDR
  }
 if(ModSize)*ModSize = LastElfSize;
 return (vptr)LastElfAddr;
}
//------------------------------------------------------------------------------------------------------------

};

// https://github.com/jhector/armhook-core/blob/master/ELF.cpp

using UELF = NUFELF<size_t>;
//============================================================================================================
/*
  SMemRange range;
   memset(&range,0,sizeof(range));
   sint res = NPFS::FindMappedRangeByAddr(-1, ptr, &range);
   DBGDBG("Range: Res=%i, Beg=%p, End=%p, Mode=%08X, INode=%u",res,(vptr)range.RangeBeg,(vptr)range.RangeEnd,range.Mode,range.INode);
*/
