
#pragma once

//------------------------------------------------------------------------------------------------------------
static _finline int GetMMapErrFromPtr(void* ptr)   // POSIX or linux specific???
{
 uint err = (uint)ptr & (MEMPAGESIZE - 1);
 if(!err)return 0;
 return -err & (MEMPAGESIZE - 1);
}
//------------------------------------------------------------------------------------------------------------
// A Directory name must end with a path delimiter. Any file name is ignored
// All paths are UTF-8
static int MakeDirPath(const achar* Path, uint mode=PX::S_IRWXO|PX::S_IRWXG|PX::S_IRWXU) // Must end with '/', may contain a filename at the end
{
 if(!Path || !*Path)return -1001;
 ssize_t plen = (ssize_t)NSTR::StrLen(Path);
 if(!plen)return -1002;  // Empty path
 if(!IsFilePathSep(Path[plen-1]) && !NPTM::NAPI::access(Path, PX::F_OK))return 0;   // The path is fully accessible already   // 'my/path' or 'my/path/file.txt'

 achar FullPath[plen+4];
 plen = (ssize_t)NSTR::StrCopy(FullPath, Path, (uint)plen);
 plen = NPTM::TrimFilePath(FullPath);    // Remove a file name if have any
 if(!plen)return -1003;  // Empty path
 ssize_t PathLen = plen;   // Without a possible file name
 int DirCtr = 0;    
// Find an accessible path level
 for(;;)
  {
   if(plen <= 0)return -1004;
   if(FullPath[plen-1] == '.')   // Trim DirSpec backwards
    {
     if(plen == 1)return -1005;  // './' left and no success yet
     if(FullPath[plen-2] == '.')
      {
       if(plen == 2)return -1006;  // '../' left and no success yet
       if(IsFilePathSep(FullPath[plen-3])){plen-=3; FullPath[plen]=0; continue;}  // Skip '/../'
      }
     if(IsFilePathSep(FullPath[plen-2])){plen-=2; FullPath[plen]=0; continue;}  // Skip '/./'
    }
   if(IsFilePathSep(FullPath[plen-1]))
    {
     while(plen && IsFilePathSep(FullPath[plen-1]))FullPath[--plen]=0;   // Remove last slash from 'my/path/'
     if(!NPTM::NAPI::access(FullPath, PX::F_OK))break;    // This path level is already acessible
     DirCtr++;
    }
     else plen--;
  }

// Create missing directories
 for(;(DirCtr > 0) &&(plen < PathLen);plen++)
  {
   if(!FullPath[plen])    // Restore a path delimiter
    {
     FullPath[plen] = PATHSEPNIX;
     if(FullPath[plen+1] && !IsDirSpec(&FullPath[plen+1]))
      {
       int res = NPTM::NAPI::mkdir(FullPath, (PX::mode_t)mode);  // A single slash is restored, try to make a directory
       if(res < 0)return res;
       DirCtr--;
      }
     continue;
    }
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// https://insanecoding.blogspot.com/2007/11/implementing-realpath-in-c.html
// Turns '\\' into '/'; removes extra '/'; processes '../' and '../'
// Remove last '/' ?
// First './' or no '/' is converted to current directory
// TODO: ResolveLinks
// NOTE: DST must be at least of SRC size
// Remove final '/' as getcwd does?
//
static sint CanonicalizePath(achar* DstPath, const achar* SrcPath, size_t DstSize, bool ResolveLinks=false)
{
 size_t DstOffs = 0;
 if(!IsFilePathSep(SrcPath[0]))              //    // Have current dir
  {
   sint len = NAPI::getcwd(DstPath, DstSize);
   if(len < 0)return len; // (len - NSTR::StrLen(SrcPath));   // Return required size
   DstOffs += len-1;
   if((SrcPath[0] == '.') && IsFilePathSep(SrcPath[1]))SrcPath++;  // We have one separator here
    else DstPath[DstOffs++] = PATHSEPNIX;

   uint ReLen = (DstSize-DstOffs) - 1;   // +1 for 0 
   uint CoLen = NSTR::StrCopy(&DstPath[DstOffs], SrcPath, ReLen);     // Copy the path so that it can step back on the cur dir path
   if((CoLen == ReLen) && SrcPath[CoLen])return -(DstOffs + NSTR::StrLen(SrcPath));    // Need larger buffer
   SrcPath = DstPath;
  }     
 // When resolving after, we can step back to parent dir even if it is linked and then resolve the actual path if it is a link
 // When resolving before we may fail to find a file if it is a back step path on a parent linked dir (../)
 sint ResLen = NormalizePath(DstPath, SrcPath);   // Resolve links before or after or optionally both?   
 return ResLen;
}
//------------------------------------------------------------------------------------------------------------



