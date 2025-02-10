#pragma once

// TODO: Move out of NPTM - nothing platform specific is here
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
static constexpr wchar PATHSEPNIX = 0x2F;    //  '/'
static constexpr wchar PATHSEPWIN = 0x5C;    //  '\'

template<typename T> constexpr _finline static bool IsFilePathSep(T val){return ((val == (T)PATHSEPNIX)||(val == (T)PATHSEPWIN));}

template<typename T> constexpr _finline static bool IsDirSpec(T Name){return (((Name[0] == '.')&&(!Name[1]||IsFilePathSep(Name[1])))||((Name[0] == '.')&&(Name[1] == '.')&&(!Name[2]||IsFilePathSep(Name[1]))));}

template<typename D, typename S> constexpr static size_t SetFilePath(D DstPath, const S FilePath, const S BasePath, size_t DstMaxLen=size_t(-1), size_t FPLen=size_t(-1), size_t BPLen=size_t(-1))  // TODO: Should return length
{
 if(!FilePath || !FilePath[0])return 0;
 if(FilePath[1] != ':')    // No drive-absolute path (Non Windows)
  {
   if(!IsFilePathSep(FilePath[0]))
    {
     NSTR::StrCopy(DstPath, BasePath, Min(DstMaxLen,BPLen)); 
     sint npos  = TrimFilePath(DstPath);            // Must end with a separator   
     sint dleft = DstMaxLen - npos;
     if(dleft > FPLen)dleft = FPLen;
     return NSTR::StrCnat(DstPath, FilePath, npos+dleft);
    }
   return NSTR::StrCopy(DstPath, FilePath, Min(DstMaxLen,FPLen));  // File system absolute path (Unix way)
  }
 return NSTR::StrCopy(DstPath, FilePath, Min(DstMaxLen,FPLen));    // Drive-absolute path (Windows way)
}
//---------------------------------------------------------------------------
template<typename T> constexpr _finline static sint TrimFilePath(T Path)
{
 sint SLast = -1;
 for(sint ctr=0;Path[ctr];ctr++)
  {
   if(IsFilePathSep(Path[ctr]))SLast = ctr;
  }
 SLast++;
 if(SLast > 0)Path[SLast] = 0;
 return SLast;
}
//---------------------------------------------------------------------------
template<typename T> constexpr _finline static T GetFileName(T FullPath, uint Length=(uint)-1)    // TODO: Just scan forward, no StrLen and backward scan  // Set constexpr 'IF' in case a T is a str obj an its size is known?
{
 sint LastDel = -1;
 for(sint ctr=0,val=FullPath[ctr];val && Length;ctr++,Length--,val=FullPath[ctr]){if(IsFilePathSep((wchar)val))LastDel=ctr;}
 return &FullPath[LastDel+1];
}
//---------------------------------------------------------------------------
template<typename T> constexpr _finline static T GetFileExt(T FullPath, uint Length=(uint)-1)
{
 sint LastDel = -1;
 uint ctr = 0;
 for(sint val=FullPath[ctr];val && (ctr < Length);ctr++,val=FullPath[ctr]){if(val=='.')LastDel=ctr;}
 if(LastDel < 0)LastDel = ctr-1;  // Point at the end if no ext
 return &FullPath[LastDel+1];     // Return points to ext after '.' or end of name (For an ext to be added without any checks)
}
//---------------------------------------------------------------------------
static bool IsValidAbsPath(const achar* Path)      // Use this to validate paths, returned by getcwd
{
 if(!Path || !*Path)return false;
 if constexpr(!IsSysWindows)return (*Path == PATHSEPNIX);    // Root only
   else return (Path[1] == ':') || IsFilePathSep(Path[0]);  // Root only if in 'C:\' format  // Cannot check more than first two chars   // TODO: GLOBAL; UNC
}
//---------------------------------------------------------------------------
// Replaces '/' with '\'
// Removes any '.\' (Including a leading one, so it should be resolved to current directory first, if needed)
// Removes excess '\' duplicates
// Excludes from the path directories that preceeded by '..\' up until root directory '\'
// So the output path can only become shorter
// Returns the string size, without 0
//
template<typename D, typename S, uint32 sep=PATHSEPNIX> static uint NormalizePath(D Dst, S Src)
{
 SCVR uint32 MaxDSegs = 256;      // Should be enough for now. If not - add back counting when overflowed
 SCVR uint32 BadSep = (sep == PATHSEPNIX)?PATHSEPWIN:PATHSEPNIX;
 sint SrcOffs = 0;
 uint DstOffs = 0;
 uint32 NumDSegs = 0;
 uint32 DstSegs[MaxDSegs];  // offsets of path segments, written in dst  (Is uint16 enough?)
 if(IsFilePathSep(Src[0]))DstSegs[NumDSegs++] = 1;   // Root pos  
 for(uint DotCtr=0,ChrCtr=0;;SrcOffs++)
  {
   uint32 val = Src[SrcOffs];
   if(!val)break;
   if(val == BadSep)val = sep;
   if(val == sep)
    {
     if(DotCtr)    // Have only dots between separators
      {
       uint NumDots = DotCtr;
       DotCtr = 0;
       if(NumDots < 3)   // Support only . and .. special dir links
        {
         if(NumDots > 1)  // Step back
          {
           if(NumDSegs > 1)DstOffs = DstSegs[--NumDSegs - 1];            // Sep1Pos1:SomeTest:SepPos2:..
             else DstOffs -= 2;   // just ignore last written '..'
           continue;
          }
           else {DstOffs--; continue;}  // drop './'    // ignore last written '.'
        }
      }
       else if(!ChrCtr && SrcOffs)continue;  // Two separators in sequence - no need to save the current separator
     DstSegs[NumDSegs++] = DstOffs + 1;  // Store the separator
    }
   else if(val == '.')DotCtr++;
          else ChrCtr++;     // Non dot chars
   Dst[DstOffs++] = (decltype(*Dst))val;
  }
 Dst[DstOffs] = 0;
 return DstOffs;
}

template<typename D, typename S> static _finline uint NormalizePathNt(D Dst, S Src){return NormalizePath<D,S,PATHSEPWIN>(Dst, Src);}   // No function aliases in C++!
//------------------------------------------------------------------------------------
_finline static uint SizeOfWStrAsUtf8(const wchar* str, uint size=uint(-1), uint term=uint(0))      // Can scan until one equal and one less tha char
{
 uint ResLen = 0;
 wchar terml = wchar(term >> 16);  // Usually 0
 wchar terme = wchar(term);
 for(uint SrcIdx = 0;(str[SrcIdx] ^ terme) && (str[SrcIdx] > terml) && (SrcIdx < size);)
  {
   uint32 Val;
   charb tmp[6];
   SrcIdx  = NUTF::ChrUtf16To32(&Val, str, 0, SrcIdx);  // Can read 2 WCHARs
   ResLen += NUTF::ChrUtf32To8(tmp, &Val, 0, 0);
  }
 return ResLen;
}
//---------------------------------------------------------------------------
_finline static uint WStrToUtf8(achar* dst, const wchar* str, auto&& dlen, auto&& slen, uint term=uint(0))   // Can scan until one equal and one less tha char
{
 uint SrcIdx = 0;
 uint DstIdx = 0;
 wchar terml = wchar(term >> 16);  // Usually 0
 wchar terme = wchar(term);
 while((str[SrcIdx] ^ terme) && (str[SrcIdx] > terml) && (DstIdx < (uint)dlen) && (SrcIdx < (uint)slen))
  {
   uint32 Val;
   SrcIdx  = NUTF::ChrUtf16To32(&Val, str, 0, SrcIdx);
   DstIdx  = NUTF::ChrUtf32To8(dst, &Val, DstIdx, 0);   // NOTE: May overflow +3 bytes!
  }
 slen = SrcIdx;
 dlen = DstIdx;
 return DstIdx;
}
//---------------------------------------------------------------------------
#ifdef CPU_X86
/*static int IsSupportedRdtscp(void)    // TODO
{
 uint32 a, b, c, d;
 return bool(__get_cpuid(0x80000001, &a, &b, &c, &d) && (d & (1<<27)));
} */
#endif
//---------------------------------------------------------------------------
