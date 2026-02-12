
#pragma once

#include "../PlatDef.hpp"
//============================================================================================================
// All "members" are placed sequentially in memory but their order may change
// NOTE: Do not expect that the memory can be executable and writable at the same time! There is 'maxprot' values may be defined in EXE header which will limit that or it may be limited by the system
// It MUST be Namespace, not Struct to keep these in code segment (MSVC linker, even with Clang compiler) // FIXED: declared as _codesec SysApi
// NOTE: Unreferenced members will be optimized out!
// NOTE: Do not use these directly, not all of them are portable even between x32/x64 on the same cpu
// AT* functions are only useful with getdents? (Calculating relative path backward (../) is complicated for them)
//
struct SAPI  // POSIX API implementation  // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
DECL_SYSCALL(NSYSC::ESysCNum::exit,       PX::exit,       exit       )
DECL_SYSCALL(NSYSC::ESysCNum::exit_group, PX::exit_group, exit_group )
#if !defined(SYS_BSD) && !defined(SYS_MACOS)
#  if defined(CPU_X86) && defined(ARCH_X64)
DECL_SYSCALL(NSYSC::ESysCNum::clone,      PX::cloneB0,    clone      )  // Linux only
#  else
DECL_SYSCALL(NSYSC::ESysCNum::clone,      PX::cloneB1,    clone      )  // Linux only
#  endif
#endif
#if !defined(CPU_ARM) || !defined(ARCH_X64)
DECL_SYSCALL(NSYSC::ESysCNum::fork,       PX::fork,       fork       )
DECL_SYSCALL(NSYSC::ESysCNum::vfork,      PX::vfork,      vfork      )
#endif
DECL_SYSCALL(NSYSC::ESysCNum::kill,       PX::kill,       kill       )
DECL_SYSCALL(NSYSC::ESysCNum::tgkill,     PX::tgkill,     tgkill     )
DECL_SYSCALL(NSYSC::ESysCNum::execve,     PX::execve,     execve     )
DECL_SYSCALL(NSYSC::ESysCNum::ptrace,   NDBG::ptrace,     ptrace     )
DECL_SYSCALL(NSYSC::ESysCNum::process_vm_readv,     PX::process_vm_readv,     process_vm_readv     )
DECL_SYSCALL(NSYSC::ESysCNum::process_vm_writev,    PX::process_vm_writev,    process_vm_writev    )

DECL_SYSCALL(NSYSC::ESysCNum::wait4,      PX::wait4,      wait4      )
DECL_SYSCALL(NSYSC::ESysCNum::futex,      PX::futex,      futex      )
DECL_SYSCALL(NSYSC::ESysCNum::nanosleep,  PX::nanosleep,  nanosleep  )
DECL_SYSCALL(NSYSC::ESysCNum::clock_nanosleep,  PX::clock_nanosleep,  clock_nanosleep  )
DECL_SYSCALL(NSYSC::ESysCNum::clock_gettime,    PX::clock_gettime,    clock_gettime    )

DECL_SYSCALL(NSYSC::ESysCNum::gettimeofday, PX::gettimeofday, gettimeofday  )
DECL_SYSCALL(NSYSC::ESysCNum::settimeofday, PX::settimeofday, settimeofday  )

DECL_SYSCALL(NSYSC::ESysCNum::gettid,     PX::gettid,     gettid     )
DECL_SYSCALL(NSYSC::ESysCNum::getpid,     PX::getpid,     getpid     )
DECL_SYSCALL(NSYSC::ESysCNum::getppid,    PX::getppid,    getppid    )
//DECL_SYSCALL(NSYSC::ESysCNum::getpgrp,    PX::getpgrp,    getpgrp    )   // Not present on ARM64, reimplemented with getpgid(0)
DECL_SYSCALL(NSYSC::ESysCNum::getpgid,    PX::getpgid,    getpgid    )
DECL_SYSCALL(NSYSC::ESysCNum::setpgid,    PX::setpgid,    setpgid    )

#if !defined(ARCH_X32)
DECL_SYSCALL(NSYSC::ESysCNum::mmap,       PX::mmap,       mmap       )
#endif

DECL_SYSCALL(NSYSC::ESysCNum::munmap,     PX::munmap,     munmap     )
DECL_SYSCALL(NSYSC::ESysCNum::mremap,     PX::mremap,     mremap     )
DECL_SYSCALL(NSYSC::ESysCNum::madvise,    PX::madvise,    madvise    )
DECL_SYSCALL(NSYSC::ESysCNum::mprotect,   PX::mprotect,   mprotect   )
DECL_SYSCALL(NSYSC::ESysCNum::msync,      PX::msync,      msync      )
DECL_SYSCALL(NSYSC::ESysCNum::mlock,      PX::mlock,      mlock      )
DECL_SYSCALL(NSYSC::ESysCNum::munlock,    PX::munlock,    munlock    )

DECL_SYSCALL(NSYSC::ESysCNum::truncate,   PX::truncate,   truncate   )
DECL_SYSCALL(NSYSC::ESysCNum::ftruncate,  PX::ftruncate,  ftruncate  )
DECL_SYSCALL(NSYSC::ESysCNum::getdents,   PX::getdents64, getdents   )     // getdents64 on x32 and x64
DECL_SYSCALL(NSYSC::ESysCNum::fstat,      PX::fstat,      fstat      )     // Struct?
DECL_SYSCALL(NSYSC::ESysCNum::fcntl,      PX::fcntl,      fcntl      )     // Too specific to put in NAPI?
DECL_SYSCALL(NSYSC::ESysCNum::ioctl,      PX::ioctl,      ioctl      )
DECL_SYSCALL(NSYSC::ESysCNum::flock,      PX::flock,      flock      )
DECL_SYSCALL(NSYSC::ESysCNum::fsync,      PX::fsync,      fsync      )
DECL_SYSCALL(NSYSC::ESysCNum::fdatasync,  PX::fdatasync,  fdatasync  )
DECL_SYSCALL(NSYSC::ESysCNum::pipe2,      PX::pipe2,      pipe2      )
DECL_SYSCALL(NSYSC::ESysCNum::ppoll,      PX::ppoll,      ppoll      )     // Use only for Linux ARM X64!
DECL_SYSCALL(NSYSC::ESysCNum::dup3,       PX::dup3,       dup3       )
DECL_SYSCALL(NSYSC::ESysCNum::dup,        PX::dup,        dup        )     // Does not allow to pass any flags (O_CLOEXEC), can be replaced with fcntl

DECL_SYSCALL(NSYSC::ESysCNum::getcwd,     PX::getcwd,     getcwd     )
DECL_SYSCALL(NSYSC::ESysCNum::chdir,      PX::chdir,      chdir      )
DECL_SYSCALL(NSYSC::ESysCNum::fchdir,     PX::fchdir,     fchdir     )

#if !defined(CPU_ARM) || !defined(ARCH_X64)
//DECL_SYSCALL(NSYSC::ESysCNum::stat,       PX::stat,       stat       )     // Struct?
DECL_SYSCALL(NSYSC::ESysCNum::open,       PX::open,       open       )
DECL_SYSCALL(NSYSC::ESysCNum::mknod,      PX::mknod,      mknod      )     // Too specific to put in NAPI?
DECL_SYSCALL(NSYSC::ESysCNum::mkdir,      PX::mkdir,      mkdir      )
DECL_SYSCALL(NSYSC::ESysCNum::rmdir,      PX::rmdir,      rmdir      )     
DECL_SYSCALL(NSYSC::ESysCNum::link,       PX::link,       link       )
DECL_SYSCALL(NSYSC::ESysCNum::symlink,    PX::symlink,    symlink    )
DECL_SYSCALL(NSYSC::ESysCNum::unlink,     PX::unlink,     unlink     )
DECL_SYSCALL(NSYSC::ESysCNum::rename,     PX::rename,     rename     )
DECL_SYSCALL(NSYSC::ESysCNum::readlink,   PX::readlink,   readlink   )
DECL_SYSCALL(NSYSC::ESysCNum::access,     PX::access,     access     )
DECL_SYSCALL(NSYSC::ESysCNum::poll,       PX::poll,       poll       )
#endif
// Bunch of *at functions (Useful together with 'getdents')                 // TODO: rmdirat wrapper
DECL_SYSCALL(NSYSC::ESysCNum::openat,     PX::openat,     openat     )
DECL_SYSCALL(NSYSC::ESysCNum::mknodat,    PX::mknodat,    mknodat    )
DECL_SYSCALL(NSYSC::ESysCNum::mkdirat,    PX::mkdirat,    mkdirat    )
DECL_SYSCALL(NSYSC::ESysCNum::linkat,     PX::linkat,     linkat     )
DECL_SYSCALL(NSYSC::ESysCNum::symlinkat,  PX::symlinkat,  symlinkat  )
DECL_SYSCALL(NSYSC::ESysCNum::unlinkat,   PX::unlinkat,   unlinkat   )
DECL_SYSCALL(NSYSC::ESysCNum::renameat,   PX::renameat,   renameat   )
DECL_SYSCALL(NSYSC::ESysCNum::readlinkat, PX::readlinkat, readlinkat )
DECL_SYSCALL(NSYSC::ESysCNum::faccessat,  PX::faccessat,  faccessat  )
DECL_SYSCALL(NSYSC::ESysCNum::fstatat,    PX::fstatat,    fstatat    )     // fstatat64 on x32 and newfstatat on x64  (original FStat struct is unreliable anyway)

DECL_SYSCALL(NSYSC::ESysCNum::close,      PX::close,    close        )
DECL_SYSCALL(NSYSC::ESysCNum::read,       PX::read,     read         )
DECL_SYSCALL(NSYSC::ESysCNum::write,      PX::write,    write        )
DECL_SYSCALL(NSYSC::ESysCNum::readv,      PX::readv,    readv        )
DECL_SYSCALL(NSYSC::ESysCNum::writev,     PX::writev,   writev       )
DECL_SYSCALL(NSYSC::ESysCNum::lseek,      PX::lseek,    lseek        )     // Offsets are same size as the architecture

#if defined(ARCH_X32)
DECL_SYSCALL(NSYSC::ESysCNum::mmap2,      PX::mmap2,    mmap2        )
//DECL_SYSCALL(NSYSC::ESysCNum::stat64,     PX::stat64,   stat64       )
//DECL_SYSCALL(NSYSC::ESysCNum::fstat64,    PX::fstat64,  fstat64      )
DECL_SYSCALL(NSYSC::ESysCNum::llseek,     PX::llseek,   llseek       )     // Definition is different from lseek
#endif


} static constexpr inline SysApi alignas(16);   // Declared to know exact address(?), its size is ALWAYS 1
//============================================================================================================
#include "../UtilsFmtELF.hpp"
//============================================================================================================
// In fact, this is LINUX, not POSIX API emulation
//   FUNC_WRAPPER(PX::cloneB0,    clone   *** MAKES THE CODE 1.5k BIGGER ***   ) {return [&]<typename T=SAPI>() _finline {if constexpr(IsArchX64&&IsCpuX86)return T::clone(args...); else return T::clone(GetParFromPk<0>(args...),GetParFromPk<1>(args...),GetParFromPk<2>(args...),GetParFromPk<4>(args...),GetParFromPk<3>(args...));}();}  // Linux-specifix, need implementation for BSD
//
struct NAPI    // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
//#include "../../UtilsNAPI.hpp"

FUNC_WRAPPERFI(PX::exit,       exit       ) {return SAPI::exit(args...);}
FUNC_WRAPPERFI(PX::exit_group, exit_group ) {return SAPI::exit_group(args...);}
FUNC_WRAPPERFI(PX::cloneB0,    clone      ) { CALL_IFEXISTRPC(clone,clone,(IsArchX64&&IsCpuX86),(args...),(flags,newsp,parent_tid,tls,child_tid),(uint32 flags, vptr newsp, int32* parent_tid, int32* child_tid, vptr tls)) }
FUNC_WRAPPERFI(PX::fork,       fork       ) { CALL_IFEXISTRN(fork,clone,NAPI,(args...),(PX::SIGCHLD, nullptr, nullptr, nullptr, nullptr)) }
FUNC_WRAPPERFI(PX::vfork,      vfork      ) { CALL_IFEXISTRN(vfork,clone,NAPI,(args...),(PX::CLONE_VM | PX::CLONE_VFORK | PX::SIGCHLD, nullptr, nullptr, nullptr, nullptr)) }   // SIGCHLD makes the cloned process work like a "normal" unix child process
//FUNC_WRAPPERFI(PX::execve,     execve     ) {return SAPI::execve(args...);}     // Not portable for windows

FUNC_WRAPPERFI(PX::process_vm_readv,     process_vm_readv     ) {return SAPI::process_vm_readv(args...);}
FUNC_WRAPPERFI(PX::process_vm_writev,    process_vm_writev    ) {return SAPI::process_vm_writev(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::gettimeofday,  gettimeofday  )     // TODO: Prefer VDSO
{
 PX::timeval*  tv = GetParFromPk<0>(args...);
 PX::timezone* tz = GetParFromPk<1>(args...);
 int res = SAPI::gettimeofday(tv, tz);
 if(res < 0)return res;
 if(tz)        // No caching, always update. Use tz only if you expect the 'localtime' file to be changed
  {
   PX::timeval tvb = {};
   if(!tv)
    {
     int resi = SAPI::gettimeofday(&tvb, tz);
     if(resi < 0)return res;
     tv = &tvb;
    }
   tz->dsttime = 0;
   if(tz->utcoffs == -1)
    {
     int resi = UNIX::UpdateTZOffsUTC(tv->sec);
     if(resi < 0){tz->utcoffs = 0; return res;}
    }
   tz->utcoffs = GetTZOffsUTC();
  }
 return res;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::settimeofday,  settimeofday  ) {return SAPI::settimeofday(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::futexGD,    futex      ) {return SAPI::futex(args..., nullptr, 0);}
//FUNC_WRAPPERFI(PX::wait4,      wait       ) {return SAPI::wait4(args...);}     // Not portable, use spawn_cleanup
FUNC_WRAPPERFI(PX::gettid,     gettid     ) {return SAPI::gettid(args...);}
FUNC_WRAPPERFI(PX::getpid,     getpid     ) {return SAPI::getpid(args...);}     // VDSO?
FUNC_WRAPPERFI(PX::getppid,    getppid    ) {return SAPI::getppid(args...);}
FUNC_WRAPPERFI(PX::getpgrp,    getpgrp    ) {return SAPI::getpgid(0);}
FUNC_WRAPPERFI(PX::getpgid,    getpgid    ) {return SAPI::getpgid(args...);}
FUNC_WRAPPERFI(PX::setpgid,    setpgid    ) {return SAPI::setpgid(args...);}
//------------------------------------------------------------------------------------------------------------
// VDSO: gettimeofday; clock_gettime; clock_gettime64; getpid; getppid
//
FUNC_WRAPPERFI(PX::gettime,  gettime  ) 
{
 PX::timespec* ts  = GetParFromPk<0>(args...);
 uint32 clkid = GetParFromPk<1>(args...);
 return SAPI::clock_gettime(PX::EClockType((uint16)clkid), ts);
}

FUNC_WRAPPERFI(PX::clocksleep,  clocksleep  ) 
{
 PX::timespec* dur  = GetParFromPk<0>(args...);
 PX::timespec* rem  = GetParFromPk<1>(args...);
 uint32 type = GetParFromPk<2>(args...);
 if(type & PX::CLK_LOWMSK)return SAPI::clock_nanosleep(PX::EClockType((uint16)type), uint8(type >> 16), dur, rem);    // Flags is only 0 or 1     
 return SAPI::nanosleep(dur, rem);   // No clock specified    (Probably when 0 too, at least on Linux(CLOCK_REALTIME, no TIMER_ABSTIME))
}

FUNC_WRAPPERFI(PX::nanosleep,  nanosleep  ) {return SAPI::nanosleep(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::mmapGD,     mmap       ) { CALL_IFEXISTRPC(mmap,mmap2,(IsArchX64),(args...),(addr,length,prot,flags,fd,uint32(offset>>12)),(vptr addr, size_t length, uint prot, uint flags, int fd, uint64 offset)) }
FUNC_WRAPPERFI(PX::munmap,     munmap     ) {return SAPI::munmap(args...);}
FUNC_WRAPPERFI(PX::mremap,     mremap     ) {return SAPI::mremap(args...);}
FUNC_WRAPPERFI(PX::madvise,    madvise    ) {return SAPI::madvise(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::mprotectex,   mprotect   ) 
{
 vptr    addr  = GetParFromPk<0>(args...);
 usize   len   = GetParFromPk<1>(args...);
 uint32  prot  = GetParFromPk<2>(args...);
 uint32* pprot = GetParFromPk<3>(args...);
 if(pprot)
  {
   SMemRange Range;
   sint res = NPFS::FindMappedRangeByAddr(-1, (usize)addr, &Range); 
   if(!res)*pprot = Range.Mode;
     else *pprot = 0;   // Means PROT_NONE
  }
 return SAPI::mprotect(addr,len,prot);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::msync,      msync      ) {return SAPI::msync(args...);}
FUNC_WRAPPERNI(PX::mlock,      mlock      ) {return SAPI::mlock(args...);}
FUNC_WRAPPERNI(PX::munlock,    munlock    ) {return SAPI::munlock(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::close,      close      ) {return SAPI::close(args...);}
FUNC_WRAPPERFI(PX::read,       read       ) {return SAPI::read(args...);}
FUNC_WRAPPERFI(PX::write,      write      ) {return SAPI::write(args...);}
FUNC_WRAPPERFI(PX::readv,      readv      ) {return SAPI::readv(args...);}
FUNC_WRAPPERFI(PX::writev,     writev     ) {return SAPI::writev(args...);}
FUNC_WRAPPERFI(PX::ftruncate,  ftruncate  ) {return SAPI::ftruncate(args...);}
//------------------------------------------------------------------------------------------------------------
// How the file descriptor connects llseek to underlying devices?
// loff_t mtdchar_lseek(struct file *file, loff_t offset, int orig)  // This matches lseek, not llseek on x32
// Is llseek on x32 cannot actually replace lseek, at least for devices?  (We must always use SAPI::lseek for them?)
FUNC_WRAPPERFI(PX::lseekGD,    lseek      )
{
 if constexpr (IsArchX32)
  {
   sint64 rofs = 0;
   sint64 offs = GetParFromPk<1>(args...);
   int res = [](int fd, uint32 offset_high, uint32 offset_low, sint64* result, int whence) _finline {return Y::llseek((PX::fdsc_t)fd, offset_high, offset_low, result, (PX::ESeek)whence);} (GetParFromPk<0>(args...), (uint32)(offs >> 32), (uint32)offs, &rofs, GetParFromPk<2>(args...));  // Lambdas are delayed and can reference an nonexistent members while used in constexpr
   if(res < 0)return res;  // How to return the error code?  // > 0xFFFFF000
   return rofs;
  }
   else return SAPI::lseek(args...);   // Compatible
}
//------------------------------------------------------------------------------------------------------------
// Complicated
FUNC_WRAPPERFI(PX::mkfifo,     mkfifo     ) { CALL_IFEXISTR(mknod,mknodat,(GetParFromPk<0>(args...), PX::S_IFIFO|GetParFromPk<1>(args...), 0),(PX::AT_FDCWD, GetParFromPk<0>(args...), PX::S_IFIFO|GetParFromPk<1>(args...), 0)) }
FUNC_WRAPPERFI(PX::mkdir,      mkdir      ) { CALL_IFEXISTR(mkdir,mkdirat,(args...),(PX::AT_FDCWD, args...)) }
FUNC_WRAPPERFI(PX::rmdir,      rmdir      ) { CALL_IFEXISTR(rmdir,unlinkat,(args...),(PX::AT_FDCWD, args..., PX::AT_REMOVEDIR)) }
FUNC_WRAPPERFI(PX::link,       link       ) { CALL_IFEXISTRP(link,linkat,(args...),(PX::AT_FDCWD,oldpath,PX::AT_FDCWD,newpath,0),(achar* oldpath, achar* newpath)) }
FUNC_WRAPPERFI(PX::symlink,    symlink       ) { CALL_IFEXISTRP(symlink,symlinkat,(args...),(target,PX::AT_FDCWD,linkpath),(achar* target, achar* linkpath)) }
FUNC_WRAPPERFI(PX::unlink,     unlink     ) { CALL_IFEXISTR(unlink,unlinkat,(args...),(PX::AT_FDCWD, args..., 0)) }
FUNC_WRAPPERFI(PX::rename,     rename     ) { CALL_IFEXISTRP(rename,renameat,(args...),(PX::AT_FDCWD,oldpath,PX::AT_FDCWD,newpath),(achar* oldpath, achar* newpath)) }
FUNC_WRAPPERFI(PX::readlink,   readlink   ) { CALL_IFEXISTR(readlink,readlinkat,(args...),(PX::AT_FDCWD, args...)) }
FUNC_WRAPPERFI(PX::access,     access     ) { CALL_IFEXISTR(access,faccessat,(args...),(PX::AT_FDCWD, args..., 0)) }
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::fstatat,    fstatat    )
{
 int res = SAPI::fstatat(args...);
// DBGDBG("res %i:\r\n%#*.32D",res,256,GetParFromPk<2>(args...));
 if(res >= 0){vptr buf = GetParFromPk<2>(args...); PX::ConvertToNormalFstat((PX::SFStat*)buf, buf);}
 return res;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::stat,       stat       )
{
 int res = SAPI::fstatat(PX::AT_FDCWD, args..., 0);     //  CALL_RIFEXISTR(stat,fstatat,(args...),(PX::AT_FDCWD, args..., 0))
// DBGDBG("res %i:\r\n%#*.32D",res,256,GetParFromPk<1>(args...));
 if(res >= 0){vptr buf = GetParFromPk<1>(args...); PX::ConvertToNormalFstat((PX::SFStat*)buf, buf);}
 return res;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::fstat,      fstat      )
{
 int res = SAPI::fstat(args...);
// DBGDBG("res %i:\r\n%#*.32D",res,256,GetParFromPk<1>(args...));
 if(res >= 0){vptr buf = GetParFromPk<1>(args...); PX::ConvertToNormalFstat((PX::SFStat*)buf, buf);}
 return res;
}
//------------------------------------------------------------------------------------------------------------
//
// https://stackoverflow.com/questions/52329604/how-to-get-the-file-desciptor-of-a-symbolic-link
// https://man7.org/linux/man-pages/man7/symlink.7.html
// Supports '.' and '..' natively
//
FUNC_WRAPPERFI(PX::open,       open       ) { CALL_IFEXISTR(open,openat,(args...),(PX::AT_FDCWD, args...)) }
FUNC_WRAPPERFI(PX::openat,     openat       ) 
{ 
 PX::fdsc_t dfd = GetParFromPk<0>(args...);
 if(dfd == PX::AT_FDCWD)return NAPI::open(GetParFromPk<1>(args...),GetParFromPk<2>(args...),GetParFromPk<3>(args...));  // Try not to use openat if not required?
 // TODO: Emulate if 'openat' is not available 
 return SAPI::openat(args...);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::pipe2,      pipe       ) {return SAPI::pipe2(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::pollGD,     poll       )
{
 PX::pollfd* fds  = GetParFromPk<0>(args...);
 uint32  nfds     = GetParFromPk<1>(args...);
 sint64  timeout  = GetParFromPk<2>(args...);
 sint64* time_rem = GetParFromPk<3>(args...);

 if constexpr (!IsCpuARM || !IsArchX64)
  {
   if(!time_rem && ((timeout == -1) || ((timeout >= 0) && (timeout < (1ll << 31)))))   // Try to use old poll
    {
     for(;;)
      {
       int res;
       if constexpr(requires { Y::poll; })res = Y::poll(fds, nfds, (sint32)timeout); else res = Y::ppoll(fds, nfds, nullptr, nullptr, 0);  //  SAPI::poll(fds, nfds, (sint32)timeout);  // ppoll isn't actually used here     // Timeout is in milliseconds by default
       if((res != PXERR(EINTR)) && (res != PXERR(EAGAIN)))return res;
      }
    }
  }
 // Use ppoll. Only ppoll is available on Arm64
 PX::timespec ts{0,0};
 PX::timespec* pts = &ts;   // Set to NULL if infinity
 if(timeout)
  {
   if(timeout != -1)
    {
     if(timeout < -1){ts.sec  = -timeout / 1000000; ts.frac = (-timeout % 1000000) * 1000;}     // Microseconds
       else {ts.sec = timeout / 1000; ts.frac = (timeout % 1000) * 1000000;}              // Milliseconds 
    }
     else pts = nullptr;
  }
 for(;;)
  {
   int res = SAPI::ppoll(fds, nfds, pts, nullptr, 0);      // !!!!!!!! May be missing   // Last arg is sigsetsize, can it be because sigmask is nullptr
   if((res == PXERR(EINTR)) && time_rem)   // Break only if we expect to receive remaining time
    {
     if(timeout ==  0){*time_rem = 0; return res;}   // No wait
     if(timeout == -1){*time_rem = -1; return res;}  // Infinity
     if(timeout <  -1){*time_rem = (ts.sec * 1000000) + (ts.frac / 1000); return res;}    // Microseconds
       else {*time_rem = (ts.sec * 1000) + (ts.frac / 1000000); return res;}    // Milliseconds     
    }
     else if((res != PXERR(EINTR)) && (res != PXERR(EAGAIN)))return res;
  }
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::getdentsGD, getdents    )
{
 size_t len = GetParFromPk<2>(args...);
 if((ssize_t)len < 0)     // Retrieve real info about links (file/dir)  // FRMWK extension
  {
//   DBGDBG("Following links");
   PX::fdsc_t dfd = GetParFromPk<0>(args...);
   vptr buf = GetParFromPk<1>(args...);
   len = -(ssize_t)len;
   int res = SAPI::getdents(dfd,buf,len);
   if(res <= 0)return res;
   for(int pos=0;pos < res;)
    {
     PX::SDirEnt* ent = (PX::SDirEnt*)((uint8*)buf + pos);
     pos += ent->reclen;
     if(ent->type != PX::DT_LNK)continue;
//     DBGDBG("Link is: %s", &ent->name);
     PX::SFStat sti;
     int sres = NAPI::fstatat(dfd, ent->name, &sti, 0);
//     DBGDBG("fstatat for the link: %i", sres);
     if(sres < 0)continue;   // Or skip the entire loop?
//     DBGDBG("SDirEnt:\r\n%#*.32D",sizeof(sti),&sti);
     if((sti.mode & PX::S_IFREG) == PX::S_IFREG)ent->type = PX::DT_REG;
     else if((sti.mode & PX::S_IFDIR) == PX::S_IFDIR)ent->type = PX::DT_DIR;
    }
   return res;
  }
  else return SAPI::getdents(args...);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::flock,      flock       ) {return SAPI::flock(args...);}
FUNC_WRAPPERFI(PX::fsync,      fsync       ) {return SAPI::fsync(args...);}
FUNC_WRAPPERFI(PX::fdatasync,  fdatasync   ) {return SAPI::fdatasync(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::dup3,       dup        )
{
 if(GetParFromPk<1>(args...) < 0)  // if newfd is -1 then behave like dup but with flags
  {
   if(GetParFromPk<2>(args...) & PX::O_CLOEXEC)return SAPI::fcntl(GetParFromPk<0>(args...),PX::F_DUPFD_CLOEXEC,0);
     else return SAPI::dup(GetParFromPk<0>(args...));
  }
   else return SAPI::dup3(args...);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::getcwd,  getcwd   ) {return SAPI::getcwd(args...);}
FUNC_WRAPPERFI(PX::chdir,    chdir   ) {return SAPI::chdir(args...);}
FUNC_WRAPPERFI(PX::fchdir,  fchdir   ) {return SAPI::fchdir(args...);}
//------------------------------------------------------------------------------------------------------------
//FUNC_WRAPPER(PX::stat,       stat       ) {return SAPI::stat(args...);}
//FUNC_WRAPPER(PX::fstat,      fstat      ) {return SAPI::fstat(args...);}
//---------------------------------------------------------------------------
// First, it returns from 'clone' into a child process if succeeds.
// Parent process is suspended by call to 'clone' with CLONE_VFORK
// If child exits with 'exit' or address space is replaced with 'execve' parent precess gets resumed
// Without CLONE_VFORK it will return immediately to parent process from 'clone'
// NOTE: You should pass file descriptors to siofd opened with O_CLOEXEC because there will be no attempt to close them before exec
// https://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-and-user-space-f
// X86-X64: %rdi, %rsi, %rdx, %rcx, %r8 and %r9
// X86-X32: on stack
// Stack:
//   Temps
//   Locals
//   RetAddr (Just a local on ARM)
//
// https://developers.redhat.com/blog/2015/08/19/launching-helper-process-under-memory-and-latency-constraints-pthread_create-and-vfork
/* Block all signals in the parent before calling vfork. This is for the safety of the child which inherits signal dispositions and
handlers. The child, running in the parent's stack, may be delivered a signal. For example on Linux a killpg call delivering a signal to a
process group may deliver the signal to the vfork-ing child and you want to avoid this. The easy way to do this is via: sigemptyset,
sigaction, and then undo this when you return to the parent. To be completely correct the child should set all non-SIG_IGN signals to
SIG_DFL and then restore the original signal mask, thus allowing the vforking child to receive signals that were actually intended for it, but
without executing any handlers the parent had setup that could corrupt state. When using glibc and Linux these functions i.e. sigemtpyset,
sigaction, etc. are safe to use after vfork. */
// https://stackoverflow.com/questions/45430087/is-it-legal-to-pass-a-null-program-argument-vector-to-execve
// By convention, the first of these strings (i.e., argv[0]) should contain the filename associated with the file being executed.
// On many other UNIX systems, specifying argv as NULL will result in an error (EFAULT).
// If CLONE_PARENT is set, then the parent of the new child (as returned by getppid(2)) will be the same as that of the calling process.
// CLONE_PARENT_SETTID(parent_tid) // Can use it for CLONE_PIDFD (Kernel 5.2)
// https://lwn.net/Articles/794707/
// https://stackoverflow.com/questions/74555660/given-a-pid-fd-as-acquired-from-pidfd-open-how-does-one-get-the-underlying
// 
// https://github.com/giampaolo/psutil
// pidfd_getfd() can fail with "Too many open files" after 1024 calls
// FreeBSD does not allow opening arbitrary processes for file descriptors. The "process descriptors" in FreeBSD can only be created by pdfork(2)
// https://man.freebsd.org/cgi/man.cgi?query=pdfork&sektion=2&n=1
// FreeBSD 9: pdfork(), pdgetpid(), pdkill()  [year 2012]
// 
// https://tip.golang.org/src/os/
// 
// TODO: This function should be moved to SharedNixNAPI.hpp and be reused for BSD/MacOS too
// 
//
FUNC_WRAPPERNI(NTHD::process_spawn,       process_spawn       )         // TODO: !!! Create as a child, not sibling so that waitpid could work!!!
{
 SCVR uint32 BaseFlags  = PX::CLONE_VM | PX::CLONE_VFORK | PX::SIGCHLD;    // CLONE_PARENT  // if CLONE_PARENT is set, then the parent of the calling process, rather than the calling process itself, is signaled.
 SCVR uint32 LinuxFlags = PX::CLONE_PIDFD;      // Old kernels should ignore CLONE_PIDFD (What about old Android kernels?)
 uint32  CloneFlg = BaseFlags | LinuxFlags;      
 uint32  CloneMsk = 0;     // For 'dup3'  // The caller can force the close-on-exec flag to be set for the new file descriptor by specifying O_CLOEXEC
 volatile ssize* Configs   = (volatile ssize*)GetParFromPk<3>(args...);
 volatile achar* AltCurDir = nullptr;
 volatile bool  UseAppDir  = false;    // Overrides AltCurDir
 volatile bool   NoPathA0  = false;

 if(Configs)
  {
   volatile ssize* Conf = Configs; 
   for(size_t type=0;(type=*Conf);Conf+=(type >> 16)+1)  // Unsafe, NULL terminated sequence
    {
     switch((uint16)type)
      {
       case uint16(NTHD::CfgFlagsDef):
        CloneFlg |= Conf[1];
        break;
       case uint16(NTHD::CfgFlagsMsk):
        CloneMsk |= Conf[1];
        break;
       case uint16(NTHD::CfgPsCurDir):
        AltCurDir = (volatile achar*)Conf[1];
        break;
       case uint16(NTHD::CfgSetTgtCurDir):
        UseAppDir = true;
        break;
       case uint16(NTHD::CfgNoPathInArg0):
        NoPathA0  = true;
        break;
      }
    }
  }
    //  volatile PX::PPCHAR argx = GetParFromPk<1>(args...);
   //  LOGMSG("AB %p: %p, %p", argx, argx[0], argx[1]);  // <<<<<<<<<<<<<<<<<<<<
 CloneFlg &= ~CloneMsk;
// TODO: Use 'access' to check if the file exist and is executable?
 volatile int PidFd   = -1;  // If supported by the kernel
 volatile int ExecRes = 0;   // If in parent process, we see this to be nonzero when execve has failed    // __asm__ __volatile__("" :: "m" (ExecRes));
 volatile PX::pid_t pid = NAPI::clone(CloneFlg, nullptr, (int*)&PidFd, (int*)&PidFd, nullptr);  // vfork     // Same stack, no copy-on-write      
 if(pid)   // Not in child (Resumed after execve or exit) (Error Child create error if negative)
  {
   volatile int tmp = ExecRes;  // Some extra to prevent optimization
   if(tmp)return tmp;  // We need the result of execv if it failed  // if exec has failed then pid have no meaning because the child should have been exited by now
   return NTHD::SHDesc{.PrHd=(size_t)pid,.TrHd=size_t(PidFd & NTHD::SHDesc::IdMsk)}.Id;  // Success or vfork has failed  // Process and its FD are the single ID  
  }
// Only a child gets here
 {
//#if defined(CPU_X86) && defined(ARCH_X64)  // On ARM32 the stack is corrupted too! // Only X86-X64 suffers from overwriting return address from clone by execve or exit so we need to move stack pointer to have more space for the child to overwrite
  volatile PX::PCCHAR pathname = GetParFromPk<0>(args...); //  By convention, the first of these strings (i.e., argv[0]) should contain the filename associated with the file being executed.
  volatile size_t MinRLen = size_t((pid >> 24)+64);
  volatile size_t NameLen = 0;
  if(pathname)
   {
    if(UseAppDir)NameLen = NSTR::StrLen(pathname);
      else if(AltCurDir && !IsFilePathSep(*pathname))NameLen = NSTR::StrLen(pathname) + PATH_MAX;    // + PATH_MAX for getcwd to add to current EXE path (May be not enough!)
   }
  MinRLen += NameLen;
  volatile size_t* padd = (volatile size_t*)StkAlloc(MinRLen);   // Some trick with volatile var to avoid optimizations // alloca must be called at the block scope
  *padd = 0;     // Some extra to prevent optimization
  if(NameLen)    //  chdir breaks pathname for execve  
   {
    PX::PCHAR ptr = (PX::PCHAR)padd;
    if(!UseAppDir && AltCurDir)
     {
      int len = SAPI::getcwd(ptr, MinRLen - (NameLen - PATH_MAX));
      if(len > 0)
       {
        while(*pathname == '.')pathname++;
        PX::PCHAR dptr = &ptr[len-1];   // Points to term 0
        if((len > 1) && IsFilePathSep(dptr[-1]))
         {
          if(IsFilePathSep(*pathname))pathname++;
         }
          else if(!IsFilePathSep(*pathname))*(dptr++) = PATHSEPNIX;  // Instead of term 0
      
        NSTR::StrCopy(dptr, pathname);    // Leave any './' at beginning
        pathname = ptr;
//      LOGMSG("FullPath: %s",pathname);
       }
     }
      else
       {
        NSTR::StrCopy(ptr, pathname); 
        TrimFilePath(ptr);
        AltCurDir = ptr;
        pathname  = GetFileName(pathname);
//        LOGMSG("pathname: '%s', AltCurDir: '%s'",pathname, AltCurDir);
       }
   }
//#endif
//  volatile size_t* ConfigsDup = (size_t*)GetParFromPk<3>(args...);  
  if(Configs)
   {
    for(size_t type=0;(type=*Configs);Configs+=(type >> 16)+1)     // All non O_CLOEXEC descriptors are already inherited
     {
//      DBGMSG(">>>: Val=%08X",type);
      if(type != NTHD::CfgPsFDShare)continue;
      sint OldFD = Configs[1];
      sint NewFD = Configs[2];
      if(OldFD == NewFD)continue;    // Supposed to be inherited
      SAPI::close(NewFD);  // Close any existing NewFD first
      int res = SAPI::dup3(OldFD, NewFD, 0);     // Not PX::O_CLOEXEC !
      SAPI::close(OldFD);  // Close OldFD because it's value is not expected here 
//      DBGMSG(">>>: NewFD=%i, OldFD=%i, Res=%i",NewFD,OldFD,res);
     }
    if(AltCurDir)SAPI::chdir((achar*)AltCurDir);     // Relative path in 'pathname' will become incorrect! 
   }

  // TODO: Inherit all EVARS and update them from the passed array (As default on Windows)
  volatile PX::PPCHAR argv = GetParFromPk<1>(args...);
  volatile PX::PPCHAR envp = GetParFromPk<2>(args...);
  PX::PCCHAR DefArgs[2];
  if(!envp)envp = GetEnvP(); // Should not be NULL
  if(!argv)      // By convention     // Pass {NULL} if you want it empty
   {
    argv = DefArgs;
    DefArgs[0] = pathname;
    DefArgs[1] = nullptr;
   }
    else if(!NoPathA0)     // Shift all slots
     {
      PX::PCCHAR lval = *argv;
      for(uint ctr=1;lval;ctr++)
       {
        PX::PCCHAR* ptr = &argv[ctr];
        PX::PCCHAR  val = *ptr;
        *ptr = lval;
        lval = val;
       }
      *argv = pathname;  // Set Arg0 to the EXE path
   //   LOGMSG("AA %p: %p, %p", argv, argv[0], argv[1]);
     }
     
  ExecRes = SAPI::execve(pathname, argv, envp);  // Should not return on success    // Is it possible to just drop last argument?
//#if defined(CPU_X86) && defined(ARCH_X64)   // On ARM32 the stack is corrupted too!
  NAPI::exit(ExecRes + (int)*padd);    // Exit the child thread only  // Resume parent on exit  // NOTE: Child enters here and overwrites parent`s return address from 'clone' and skips return value assignment from 'clone'
//#else
//  NAPI::exit(ExecRes);   // Or ESRCH ?
//#endif
 }      
 return pid;   // Should never be reached
}
//------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/18476138/is-there-a-version-of-the-wait-system-call-that-sets-a-timeout
// https://gaultier.github.io/blog/way_too_many_ways_to_wait_for_a_child_process_with_a_timeout.html
// https://www.corsix.org/content/what-is-a-pidfd
// 
// https://man7.org/linux/man-pages/man2/pidfd_open.2.html
//  int syscall(SYS_pidfd_open, pid_t pid, unsigned int flags);
// A PID file descriptor returned by pidfd_open() (or by clone(2) with the CLONE_PID flag) can be used for the following purposes: A PID file descriptor can be monitored using poll(2), select(2), and epoll(7).
//  CLONE_PIDFD: kernel 5.2; If that flag is present, the kernel will return a pidfd (referring to the newly created child) to the parent by way of the ptid argument
// pidfd_open: kernel 5.3 poll / select / epoll; 5.4 waitid with P_PIDFD mode (P_PIDFD value is 3)
// /proc/self/fdinfo/$pidfd
// 
// signalfd may grab SIGCHLD signals that other threads may wait for 
// 
// When a process exits, it returns an integer value to the operating system. On most unix variants, this value is taken modulo 256: everything but the low-order bits is ignored. 
// The status of a child process is returned to its parent through a 16-bit integer in which
//   bits 0-6 (the 7 low-order bits) are the signal number that was used to kill the process, or 0 if the process exited normally;
//   bit 7 is set if the process was killed by a signal and dumped core;
//   bits 8-15 are the process's exit code if the process exited normally, or 0 if the process was killed by a signal.
//
FUNC_WRAPPERNI(NTHD::process_wait,       process_wait       )      // PX::pid_t pid, uint64 wait_ns, sint* status
{
 NTHD::SHDesc pid {.Id = GetParFromPk<0>(args...)};
 uint32 wait_ms = GetParFromPk<1>(args...);
 sint*  estatus = GetParFromPk<2>(args...);

 int32 tfd = pid.TrHd;
 if(tfd != NTHD::SHDesc::IdMsk)   // Have FD for this process
  {
   int res = NAPI::spoll(tfd, PX::POLLHUP|PX::POLLIN, wait_ms, nullptr);
   DBGMSG("PollRes: %i",res);
   if(!res)return 0;      // Time out
   SAPI::close(tfd);      // Put it aftter wait4?
  }                   
 sint32 exitval = 0;
 int wflg = ((wait_ms == (uint32)-1)?0:PX::WNOHANG);
 DBGMSG("WillLoop: %i",(bool)wflg);
 for(;;)              // Will just loop if FD is not supported
  {
   int res = SAPI::wait4(pid.PrHd, &exitval, wflg, nullptr);
   if(res < 0)return res;  // Not PID // On success it should return the PID but better not to depend on that.
   if(res > 0)break;  // The proceess is terminated (The PID will be recycled)
   if(!wait_ms)return 0;   // Time out
   NAPI::sleep(1,0);    // WNOHANG, wait 1 sec and repeat
   if(wflg)wait_ms -= (wait_ms > 1000)?1000:wait_ms;
  }
 // Any clean up here?
 if(estatus)*estatus = sint(PX::WEXITSTATUS(exitval));
 return 1; // Exited for sure?    // Use WIFEXITED to return additional info?
}
//------------------------------------------------------------------------------------------------------------
// sysdeps/unix/sysv/linux/createthread.c)        pthread_create
// If CLONE_THREAD is set, the child is placed in the same thread group as the calling process.
// CLONE_SYSVSEM is set, then the child and the calling process share a single list of System V semaphore adjustment (semadj) values
// ARCH_CLONE (&start_thread, STACK_VARIABLES_ARGS, clone_flags, pd, &pd->tid, tp, &pd->tid
// Who will free the stack if the thread suddenly terminated?
// No CREATE_SUSPENDED on Linux
// LIBC-X32: unsigned int pthread_self(){return __readgsdword(8u);}   // Cannot use same approach with GS to avoid conflicts with PTHREADS (LibC may be loaded and initialized from app`s main thread(i.e. by loading some other library dynamically))
// Calculation of TLS space of different libs by the Loader is a MESS!
// TODO: Use ThreadID as some index in a special memory area to find address of a thread`s context frame (SFWCTX)
// No way to disable FramePointer for this function?
// " In fact, CLONE_THREAD implies cloning parent process ID (PPID), just like CLONE_PARENT, and this way children threads are not
//    actually children of the thread that issued clone(), but of its parent. And that's why my wait() calls failed with ECHILD - there were no children. "
// Either remove CLONE_PARENT and make the new thread a child of the one that creates it or use ThreadID to wait for a futex.
// A new thread created with CLONE_THREAD has the same parent process as the caller of clone() (i.e., like CLONE_PARENT)
// NOTE: it is the parent process, which is signaled when the child terminates
// Do not forget about 'wait' and 'zombies'
// 
// 
// A new thread created with CLONE_THREAD has the same parent process as the process that made the clone call (i.e., like CLONE_PARENT), so that calls to getppid(2) 
// return the same value for all of the threads in a thread group.  When a CLONE_THREAD thread terminates, the thread that created it is not sent a SIGCHLD (or other termination) signal;
// nor can the status of such a thread be obtained using wait(2). (The thread is said to be detached.)
//
// After all of the threads in a thread group terminate the parent process of the thread group is sent a SIGCHLD (or other termination) signal.
// 
// CLONE_CHILD_CLEARTID(child_tid) alerts the futex when threads exit
// CLONE_PARENT_SETTID(parent_tid) is used to store the new threqd's id avoiding race conditions for that slot (probably) 
// 
// https://nullprogram.com/blog/2023/03/23/
//
// Stack:
// Rest of the stack
// SThCtx
// User Data
// TLS Block
//
FUNC_WRAPPERNI(NTHD::thread_spawn,       thread_spawn       )
{
 auto    ThProc   = GetParFromPk<0>(args...);
 vptr    ThData   = GetParFromPk<1>(args...);
 if(!ThProc)return PXERR(ENOEXEC);     // Nothing to execute
 size_t  DatSize  = GetParFromPk<2>(args...);
 size_t* Configs  = (size_t*)GetParFromPk<3>(args...);
 size_t  StkSize  = NCFG::DefStkSize;   // NOTE: As StkSize is aligned to a page size, there will be at least one page wasted for ThreadContext struct (Assume it always available for some thread local data?)
 size_t  TlsSize  = NCFG::DefTlsSize;   // Slots is at least of pointer size
 uint32  CloneFlg = (PX::CLONE_VM | PX::CLONE_FS | PX::CLONE_FILES | PX::CLONE_SYSVSEM | PX::CLONE_SIGHAND | PX::CLONE_PARENT_SETTID | PX::CLONE_CHILD_CLEARTID | PX::CLONE_THREAD);   // | PX::CLONE_PARENT CLONE_PTRACE  CLONE_SETTLS |  CLONE_PARENT_SETTID // CLONE_CHILD_CLEARTID (? Thread pools? Other systems?)    // CLONE_PARENT is probably not needed with CLONE_THREAD
 uint32  CloneMsk = 0;   // Excluded user-specified flags         // Pass SIGCHLD to inform parent of the thread termination? (NOTE: The parent is noth the caller of 'clone', its the parent of the caller)

 if(Configs)
  {
   for(size_t type=0;(type=*Configs);Configs+=(type >> 16)+1)  // Unsafe, NULL terminated sequence
    {
     switch((uint16)type)
      {
       case uint16(NTHD::CfgThStkSize):
        StkSize   = Configs[1];
        break;
       case uint16(NTHD::CfgThTlsSize):
        TlsSize   = Configs[1];
        break;
       case uint16(NTHD::CfgFlagsDef):
        CloneFlg |= Configs[1];
        break;
       case uint16(NTHD::CfgFlagsMsk):
        CloneMsk |= Configs[1];
        break;
      }
    }
  }

 CloneFlg &= ~CloneMsk;
 size_t* StkFrame = nullptr;
 NTHD::SThCtx* ThrFrame = InitThreadRec((vptr)ThProc, ThData, StkSize, TlsSize, DatSize, &StkFrame);
 if(uint err=MMERR(ThrFrame);err)return -err;
 if constexpr (IsCpuX86)
  {
   if constexpr (IsArchX32)    // NOTE: must match with syscall stub which uses 'popad' on exit: EDI, ESI, EBP, EBX, EDX, ECX, EAX    // ESP is not loaded(ignored) from stack and just incremented
    {                 // Stack: clone_args, clone_ret_addr, pushad_8regs
     StkFrame[-1] = (size_t)&ThProcCallStub;      // Just return there, no args needed
     for(uint idx=2;idx <= 9;idx++)StkFrame[-idx] = (size_t)ThrFrame;     // All popped registers(including EBP) will point to the thread desc (And stack will be considered above it)
     StkFrame    -= 9;      // Number of registers to 'popd'  // 8 regs and ret addr
    }
   else
    {
     StkFrame[-1] = (size_t)&ThProcCallStub;      // Just return there, no args needed
     StkFrame    -= 1;    // Ret addr
    }
  }
 else {} // ???
 DBGMSG("StkFrame=%p, ThrFrame=%p",StkFrame,ThrFrame);
 DBGMSG("Info %p: Rec0=%p, Rec1=%p, Rec2=%p, Rec3=%p, Rec4=%p",NPTM::GetThDesc()->ThreadInfo,(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[0],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[1],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[2],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[3],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[4]);

// register NTHD::SThCtx* ThFrm __asm__("5") = ThrFrame;                  // Allocate a register by index  // NOTE: no free registers on X32-X64
// __asm__ __volatile__("" : "=r"(ThFrm) : "r"(ThFrm));    // This will preserve the register
// Because the call is not inlined we have to have a return address on the new stack (X86)
 PX::pid_t pid = NAPI::clone(CloneFlg, StkFrame, (PX::PINT32)&ThrFrame->ThreadID, (PX::PINT32)&ThrFrame->ThreadID, nullptr);  // vfork     // Same stack, no copy-on-write   // pid saved to [ebp-XXXh] on X86 with O0
 if(!pid)    // In the provided stack, all required values MUST be in registers already!   // ARM32(LR), ARM64(X30), X86-X64(Stack), X86-X32(Stack)(watch out for cdecl stack release)
  {
   ThProcCallStub();   // Only on ARM it will get here  // Will try to get GETSTKFRAME inside   // X86-X32: GOT will be reread into EBX from [ebp-XXXh] (O0)
  }
 return NTHD::SHDesc{.PrHd=0,.TrHd=(size_t)pid}.Id;  // Not in a new thread - return TID or an error code  // Cannot store the futex(&ThreadID) here (it is a pointer)   // May be at least store the index (in thread array)?
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_sleep,      thread_sleep     )     // Sleep self  (Until timeout or a signal)      // sys_pause ?
{
 uint64 wait_us = GetParFromPk<0>(args...);
 NTHD::SThCtx* tinf = GetThreadSelf();
 if(!tinf)return PXERR(EBADF);
 PX::timespec ts;
 PX::PTiSp tsp = nullptr;
 if(wait_us != (uint64)-1)
  {
   ts.sec  = wait_us / 1000000;    // 1000000 Microseconds in one Second
   ts.frac = (wait_us % 1000000) * 1000;      // 1000 nanoseconds in one Microsecond
   tsp = &ts;
  }
 sint32 res = NAPI::futex((uint32*)&tinf->ThreadID, PX::FUTEX_WAIT|PX::FUTEX_PRIVATE_FLAG, tinf->ThreadID, tsp);  // FUTEX_WAIT or FUTEX_WAIT_BITSET operation can be interrupted by a signal
 if(res == -PX::ENOSYS)res = NAPI::futex((uint32*)&tinf->ThreadID, PX::FUTEX_WAIT, tinf->ThreadID, tsp);
 return res;
}
//------------------------------------------------------------------------------------------------------------
// Doing this from another thread so we must find its context by its ID
//
FUNC_WRAPPERNI(NTHD::thread_wait,       thread_wait      )
{
 NTHD::SHDesc tid {.Id = GetParFromPk<0>(args...)};
 uint32 wait_ms = GetParFromPk<1>(args...);
 NTHD::SThCtx* tinf = GetThreadByID(tid.TrHd);
 if(!tinf)return PXERR(EBADF);   // i.e. the thread is already finished

 PX::timespec ts;
 PX::PTiSp tsp = nullptr;
 if(wait_ms != (uint64)-1)
  {
   ts.sec  = wait_ms / 1000;
   ts.frac = (wait_ms % 1000) * 1000000;   // 1000000 nanoseconds in one millisecond
   tsp = &ts;
  }
 DBGMSG("Waiting for: %u",tinf->ThreadID);
// Returns 0 if the caller was woken up. callers should always conservatively assume that a return value of 0 can mean a spurious wake-up (Why?)
 sint32 res;
 for(;;)
  {
   res = NAPI::futex((uint32*)&tinf->ThreadID, PX::FUTEX_WAIT, tinf->ThreadID, tsp);   // Will not work with FUTEX_PRIVATE_FLAG - infinite waiting (Why?)  // FUTEX_CLOCK_REALTIME ???
   if(res != PXERR(EINTR))break;     // Is timeout updated to remaining time?
  }
 return res;
}
//------------------------------------------------------------------------------------------------------------
// Only a parent process can do wait/waitpid and that is not the one that spawned the thread with CLONE_THREAD which is acting like a sibling (all threads are siblings here, it simplifes a lot of things)
//
FUNC_WRAPPERNI(NTHD::thread_status,   thread_status      )     // Get the thread return code (Works on finished threads with unavailable SThCtx)
{
 NTHD::SHDesc tidr {.Id = GetParFromPk<0>(args...)};
 sint tid = tidr.TrHd;
 NTHD::SThCtx* ThCtx = nullptr;
 NTHD::STDesc* ThDsc = NPTM::GetThDesc();
 if(tid != ThDsc->MainTh.ThreadID)
  {
   if(!ThDsc->ThreadInfo)return PXERR(ENOMEM); // No more threads
   NTHD::SThCtx** ptr = ThDsc->ThreadInfo->FindOldThreadByTID(tid);
   if(!ptr)return PXERR(ENOENT);
   ThCtx = NTHD::ReadRecPtr(ptr);
  }
   else ThCtx = &ThDsc->MainTh;
 if(!ThCtx)return PXERR(EBADF);
 DBGMSG("Status: %08X",ThCtx->ExitCode);
 return ThCtx->ExitCode;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_exit,       thread_exit      )
{
 sint status = GetParFromPk<0>(args...);   // If this var is on stack, the stack may become deallocated (probably - Even marked records should be checked for zero TID)
 NTHD::SThCtx* tinf = GetThreadSelf();
 if(tinf && tinf->SelfPtr)
  {
   tinf->LastThrdID = tinf->ThreadID;
   tinf->ExitCode   = status;
   NTHD::ReleaseRec((NTHD::SThCtx**)tinf->SelfPtr);
 }
 return NAPI::exit(status);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(NDBG::ptrace,  ptrace   ) {return SAPI::ptrace(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(NDBG::cache_flush,  cache_flush   )  
{
#if defined(CPU_ARM) && !defined(ARCH_X64)       // ARM_cacheflush
 register const vptr   r0 asm("r0") = GetParFromPk<0>(args...);  //begin;
 register const vptr   r1 asm("r1") = GetParFromPk<1>(args...);  //end;
 register const uint32 r2 asm("r2") = 0;
 register const uint32 r7 asm("r7") = 0xf0002;
 asm volatile ("svc 0x0" :: "r" (r0), "r" (r1), "r" (r2), "r" (r7));
#endif
}
//------------------------------------------------------------------------------------------------------------
#include "../../SharedNAPI.hpp"

};
//============================================================================================================

#include "../../NixUtils.hpp"
#include "Startup.hpp"

//============================================================================================================
static sint Initialize(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr, bool InitConLog=false)    // _finline ?
{
 if(IsInitialized())return 1;
 if(!NLOG::CurrLog)NLOG::CurrLog = &NLOG::GLog;  // Will be set with correct address, relative to the Base
 if(InitConLog)   // On this stage file logging is not possible yet (needs InitStartupInfo)
  {
   NPTM::NLOG::GLog.LogModes   = NPTM::NLOG::lmCons;
   NPTM::NLOG::GLog.ConsHandle = NPTM::GetStdErr();
  }
 // NOTE: Init syscalls before InitStartupInfo if required
 InitStartupInfo(StkFrame, ArgA, ArgB, ArgC);
 IFDBG{DbgLogStartupInfo();}
 InitMainThreadRec(StkFrame);
 return 0;
}
//============================================================================================================
