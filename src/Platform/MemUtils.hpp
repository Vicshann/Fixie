
//------------------------------------------------------------------------------------------------------------
enum EMemMode {mmNone=0, mmRead=0x01, mmWrite=0x02, mmExec=0x04, mmShared=0x08};   // Mapped to EMapProt

struct SMemRange
{
 usize  RangeBeg;
 usize  RangeEnd;
 usize  FMOffs;
 usize  INode;     // FileID
 uint32 Mode;      // EMemMode // rwxp
 uint32 DevH;
 uint32 DevL;
 uint32 FPathLen;  // Not including term 0
 achar* FPath;     // ProcfsParseMMapLine sets it pointing inside the Line that has been parsed

 SMemRange(void){FPathLen=0;}
};

struct SMemMap
{
 usize     NextAddr;      // 0 if  no more
 usize     RangesCnt;
 ssize     TmpBufOffs;    // Must be set to 0 initially
 usize     TmpBufLen;
 uint8*    TmpBufPtr;     // For reading from the system
 SMemRange Ranges[0];

 SMemMap(void){NextAddr=RangesCnt=TmpBufOffs=0;}
};
//------------------------------------------------------------------------------------------------------------
// NOTE: Range->FPath and Range->FPathLen must be set to actual buffer address and size or NULL if not needed
// TODO: MacOS
// TODO: WASM stub
//
static sint FindMappedRangeByAddr(sint ProcId, usize Addr, SMemRange* Range)
{
#if defined(SYS_ISOLATED)
 return -1;    // Always
#elif defined(SYS_WINDOWS)
 return NTX::FindMappedRangeByAddr((sint32)ProcId, Addr, Range);
#elif defined(SYS_MACOS) 
 return -1;
#elif defined(SYS_BSD)
 return -1;
#elif defined(SYS_UNIX)
 return NPFS::FindMappedRangeByAddr(ProcId, Addr, Range);
#else
 return -1;
#endif
}
//------------------------------------------------------------------------------------------------------------
static sint FindMappedRangesByPath(sint ProcId, usize Addr, const achar* ModPath, SMemMap* MappedRanges, usize BufSize)
{
#if defined(SYS_ISOLATED)
 return -1;    // Always
#elif defined(SYS_WINDOWS)
 return NTX::FindMappedRangesByPath((sint32)ProcId, Addr, ModPath, MappedRanges, BufSize);
#elif defined(SYS_MACOS) 
 return -1;
#elif defined(SYS_BSD)
 return -1;
#elif defined(SYS_UNIX)
 return NPFS::FindMappedRangesByPath(ProcId, Addr, ModPath, MappedRanges, BufSize);
#else
 return -1;
#endif
}
//------------------------------------------------------------------------------------------------------------
static sint ReadMappedRanges(sint ProcId, usize AddrFrom, usize AddrTo, SMemMap* MappedRanges, usize BufSize)  // Windows: QueryVirtualMemory; Linux: ProcFS; BSD:?; MacOS:?
{
#if defined(SYS_ISOLATED)
 return -1;    // Always
#elif defined(SYS_WINDOWS)
 return NTX::ReadMappedRanges((sint32)ProcId, AddrFrom, AddrTo, MappedRanges, BufSize);
#elif defined(SYS_MACOS) 
 return -1;
#elif defined(SYS_BSD)
 return -1;
#elif defined(SYS_UNIX)
 return NPFS::ReadMappedRanges(ProcId, AddrFrom, AddrTo, MappedRanges, BufSize);
#else
 return -1;
#endif
}
//------------------------------------------------------------------------------------------------------------
static bool IsValidMemPtr(vptr ptr, usize len)
{
 usize PageLen = GetPageSize();
 len += (usize)ptr & (PageLen - 1);
 ptr  = (vptr)AlignBkwdP2((usize)ptr, PageLen);
 len  = AlignFrwdP2(len, PageLen);
 return !NAPI::msync(ptr, len, PX::MS_ASYNC);   // Returns ENOMEM if the memory (or part of it) was not mapped
}
//------------------------------------------------------------------------------------------------------------
static sint MemProtect(vptr Addr, usize Size, uint32 Prot, uint32* Prev=nullptr)
{
 usize pageSize = 4096;  //sysconf(_SC_PAGESIZE);    MEMPAGESIZE  ???   GetPageSize() ???  // Or always 4K?
 usize end = (usize)Addr + Size;
 usize pageStart = (usize)Addr & -pageSize;
 int res = NPTM::NAPI::mprotect((void *)pageStart, end - pageStart, Prot, Prev);     // PROT_READ | PROT_WRITE | PROT_EXEC
// LOGMSG("Addr=%08X, Size=%08X, Res=%i", Addr, Size, res);
 return res;
}
//------------------------------------------------------------------------------------------------------------
// Addr is IN/OUT
static sint MemQuery(vptr* Addr, usize* Size, uint32* Prot)
{
 SMemRange Range;
 sint res = FindMappedRangeByAddr(-1, (usize)*Addr, &Range);
 if(res < 0)return res;
 if(Size)*Size = Range.RangeEnd - Range.RangeBeg;
 if(Prot)*Prot = Range.Mode;
 *Addr = (vptr)Range.RangeBeg;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
