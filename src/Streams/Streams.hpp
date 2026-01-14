
//============================================================================================================
struct STRM
{
enum ESeek   // Same as in PX
{
 SEEK_SET  = 0,       // Seek relative to begining
 SEEK_CUR  = 1,       // Seek relative to current position
 SEEK_END  = 2,       // Seek relative to end 
};

enum ERdWrFlg
{
 rwfNone   = 0,
 rwfNoAdv  = 0x00000001,  // Do not advance the offset
 rwfVector = 0x00000002   // Use array of PX::iovec
};

// NOTE: Those are EXACT values, not just any negative value (Last 4096 bytes of SIZE_T ir reserved for it (POSIX error))
// In Linux, the read() syscall itself does not "return EOF" as a special value or character. Instead, the end-of-file (EOF) condition is indicated by a return value of 0 from the read() system call. 
SCVR usize SNONE = (usize)-PX::ENOSYS;      // Not implemented             
SCVR usize SFAIL = (usize)-PX::EIO;         // The operation has failed
SCVR usize SEOF  = (usize)PX::EOF;          // 'read' syscall returns 0 // EOF can be generated with Ctrl-Z on Windows and Ctrl-D on many other OSes   // NOTE: PX::EOF conflicts with PX::EPERM (and -1 is confusing?)
// TODO: Check perfomane - dynamic vs static polymorphism

enum EToStrm: uint64      // For 'ToStream'
{
 etsFromCurrPos     = uint64(-1),
 etsSizeUntilEnd    = uint64(-1),  // Just MaxSize
 etsSizeUntilCurPos = 0
};

// Stream functions return -1 if failed, -2 if not applicable   // Must be here to be accessible from 'auto' args of allocators (out of seq)
// EOF is 0
// static _finline bool IsEOF(auto v){return !v);}   // End-Of-File (Specifically for 'Read')
// static _finline bool IsFail(auto v){return --v >= ~((MakeUnsignedT<decltype(v)>)(0xFFF));}  // Failed to complete the request (Including EOF)
// static _finline bool IsError(auto v){return v >= ~((MakeUnsignedT<decltype(v)>)(0xFFF));}
// EOF is -1
 static _finline bool IsEOF(auto v){return !~((MakeUnsignedT<decltype(v)>)v);}
 static _finline bool IsFail(auto v){return v >= ~((MakeUnsignedT<decltype(v)>)(0xFFF));}   // Includes IsEOF   // Low level (High level strems may return some cached data and EOF)
 static _finline bool IsError(auto v){return v >= ~((MakeUnsignedT<decltype(v)>)(0xFFE));}  // Does not include EOF (-1)
 static _finline bool GetError(auto v){return -v & 0xFFF;}

class CStrmBase
{
protected: ~CStrmBase() = default;
public:
  void operator delete(void*) = delete;
// Stream functions return -1 if failed, -2 if not applicable   // Must be here to be accessible from 'auto' args of allocators (out of seq)
// EOF is 0
// static _finline bool IsEOF(auto v){return !v);}   // End-Of-File (Specifically for 'Read')
// static _finline bool IsFail(auto v){return --v >= ~((MakeUnsignedT<decltype(v)>)(0xFFF));}  // Failed to complete the request (Including EOF)
// static _finline bool IsError(auto v){return v >= ~((MakeUnsignedT<decltype(v)>)(0xFFF));}
// EOF is -1
 static _finline bool IsEOF(auto v){return STRM::IsEOF(v);}
 static _finline bool IsFail(auto v){return STRM::IsFail(v);}   // Includes IsEOF   // Low level (High level strems may return some cached data and EOF)
 static _finline bool IsError(auto v){return STRM::IsError(v);}  // Does not include EOF (-1)
 static _finline bool GetError(auto v){return STRM::GetError(v);}

 //virtual ~CStrmBase() {}               // Bad design! - Don't even try to delete that you don't own!
 virtual uint64 Size(void){return SNONE;}
 virtual uint64 Offset(void){return SNONE;}                     // Returns offset from beginning or -1/-2
 virtual uint64 Seek(sint64 Offs, ESeek wh){return SNONE;}      // Returns new offset from beginning or -1/-2
 virtual uint64 Read(vptr DstBuf, usize Size, uint32 Flags=0){return SNONE;}     // If vec is true then BUF points to PX::iovec and SIZE is number of iovec structures
 virtual uint64 Write(vptr SrcBuf, usize Size, uint32 Flags=0){return SNONE;}
 virtual usize  Flush(void){return SNONE;}  
 virtual usize  Rewind(void){return SNONE;}      // Some streams may support rewinding but not seeking
 virtual usize  Reset(void){return SNONE;}       // Reset internal state, discard data, will not delete any files
 virtual usize  Discard(void){return SNONE;}     // Discard all data and reset the stream, delete temporary file - job interrupted
//------------------
// size is 0  - Read up to current position
// size is -1 - Read everything
// from is -1 - Do not change the position
 virtual uint64 ToStream(CStrmBase* strm, uint64 size=uint64(-1), uint64 from=0)   // Generic implementation (Slower for memory streams)
  {
   uint8  buffer[4096];  // Too big?
   uint64 res;
   uint64 cpos = this->Offset();
   if(!size)   // etsSizeUntilCurPos: Read 'from' up to 'CurrOffs' (Must seek)
    {
     if(from >= cpos)return 0;  // Nothing to read // Including (from == -1)
     size = cpos - from;
    }
   if(from != etsFromCurrPos)  // Can skip seeking if set it to -1
    {
     res = this->Seek(from, SEEK_SET);
     if(IsFail(res))return res;
    }
   uint64 Total = 0;
   while(size)
    {
     res = this->Read(&buffer, sizeof(buffer));
     if(IsFail(res))return res;
     if(res > size)res = size;
     size -= res;
     res = strm->Write(&buffer, res);
     if(IsFail(res))return res;
     Total += res;
    }
   return Total;   // Total bytes written
  }
};     // TODO: Read8,read16,read32,read64 ???  May be optimized for buffer streams
//============================================================================================================
class CStrmFile final: public CStrmBase
{
 NPTM::PX::fdsc_t fd;

public:

//virtual ~CStrmFile() {}
~CStrmFile(void)
{
 this->Close();
}
//-----------------------------------------------------
CStrmFile(void)
{
 this->fd = -1;
}
//-----------------------------------------------------
ssize Open(achar* pathname, uint32 flags=NPTM::PX::O_RDWR|NPTM::PX::O_CREAT, uint32 mode=0)   // Flags: 0,1,2 = O_RDONLY,O_WRONLY,O_RDWR
{
 this->Close();
 this->fd = NPTM::NAPI::open(pathname,flags,mode);
 return this->fd;   // Error if '< 0'
}
//-----------------------------------------------------
int Close(void)
{
 if(this->fd < 0)return -1;      // What error?
 NPTM::NAPI::close(this->fd);
 this->fd = -1;
 return 0;
}
//-----------------------------------------------------
virtual usize Flush(void) override {return (usize)NPTM::NAPI::fsync(this->fd);}
//-----------------------------------------------------
virtual uint64 Size(void) override
{
 NPTM::PX::SFStat sti;
 usize res = NPTM::NAPI::fstat(this->fd, &sti);
 if(res)return SFAIL;
 return sti.size;    // sint64
}
//-----------------------------------------------------
virtual uint64 Offset(void) override {return NPTM::NAPI::lseek(this->fd,0,NPTM::PX::SEEK_CUR);}
//-----------------------------------------------------
virtual uint64 Seek(sint64 Offs, ESeek wh) override {return NPTM::NAPI::lseek(this->fd,Offs,(NPTM::PX::ESeek)wh);}
//-----------------------------------------------------
virtual uint64 Read(vptr DstBuf, usize Size, uint32 Flags=0) override   // 'read' syscall indicates the end-of-file (EOF) condition by returning a value of 0
{
 uint64 res;
 if(Flags)res = NPTM::NAPI::readv(this->fd,(NPTM::PX::PIOVec)DstBuf, Size);
   else res = NPTM::NAPI::read(this->fd, DstBuf, Size);
 if(!res && Size)return PX::EOF;    // Streams should return non-null EOF in negative range of sint64
 return res;
}
//-----------------------------------------------------
virtual uint64 Write(vptr SrcBuf, usize Size, uint32 Flags=0) override
{
 uint64 res;
 if(Flags)res = NPTM::NAPI::writev(this->fd,(NPTM::PX::PIOVec)SrcBuf, Size);
   else res = NPTM::NAPI::write(this->fd, SrcBuf, Size);
 if(!res && Size)return PX::EOF;   // Whatever that means
 return res;
}
//-----------------------------------------------------
virtual usize Rewind(void) override {return NPTM::NAPI::lseek(this->fd,0,NPTM::PX::SEEK_SET);}
//-----------------------------------------------------
virtual usize Reset(void) override        // ????????????? Need the file name to reopen it???
{
 this->Close();
 return 0;
}
//-----------------------------------------------------
virtual usize Discard(void) override         // ????????????? Need the file name to unlink it???
{
 NPTM::NAPI::ftruncate(this->fd, 0);      // No delete on close?  // unlink requires a file path
 this->Close();
 return 0;
}
};
//------------------------------------------------------------------------------------------------------------
//============================================================================================================
// Static buffer stream
class CStrmBuf final: public CStrmBase    // Making final to keep its destructor safe and non-virtual. No point inheriting this?
{
 uint8* Beg;
 uint8* End;
 uint8* Ptr;
 uint8* Len;

 void _finline UpdateLen(void){if(this->Ptr > this->Len)this->Len = this->Ptr;}
public:
 CStrmBuf(void){this->Init(nullptr, 0, 0);}
 CStrmBuf(vptr Data, usize Capacity, usize Length=0){this->Init(Data, Capacity, Length);}
 void Init(vptr Data, usize Capacity, usize Length=0){this->Ptr = this->Beg = (uint8*)Data; this->End = this->Beg + Capacity; this->Len = this->Beg + Length;}
 //virtual ~CStrmBuf() {}

 virtual uint64 Size(void) override {return this->Len - this->Beg;}
 virtual uint64 Offset(void) override {return this->Ptr - this->Beg;}                       // Returns offset from beginning or -1/-2
 virtual uint64 Seek(sint64 Offs, ESeek wh) override 
 {
  uint8* NewPtr; // Upon successful completion, lseek() returns the resulting offset location as measured in bytes from the beginning of the file.
  switch(wh)     // Is it OK to set the new position right at the END?
   {
    default: return -PX::EINVAL;        // -Wcovered-switch-default  // Do not disable - someone may cast
    case SEEK_SET:    // Seek relative to beginning of file
      if(Offs < 0)return -PX::ERANGE;   // EINVAL
      NewPtr = &this->Beg[Offs];
      if(NewPtr > this->End)return -PX::ERANGE;   // EINVAL
      break;
    case SEEK_CUR:    // Seek relative to current file position
      NewPtr = &this->Ptr[Offs];
      if((NewPtr < this->Beg)||(NewPtr > this->End))return -PX::ERANGE;   // EINVAL
      break;
    case SEEK_END:    // Seek relative to end of file
      if(Offs > 0)return -PX::ERANGE;   // EINVAL
      NewPtr = &this->Len[Offs];        // Actual end may be less than Capacity
      if(NewPtr < this->Beg)return -PX::ERANGE;   // EINVAL
      break;
   }  
  this->Ptr = NewPtr;
  this->UpdateLen();
  return (this->Ptr - this->Beg);
 }   
//-----------------------------------------------------
 virtual uint64 Read(vptr DstBuf, usize Size, uint32 Flags=0) override  // TODO: Vec  // If vec is true then BUF points to PX::iovec and SIZE is number of iovec structures
 {
  usize bleft = (this->Len - this->Ptr);   // Should not read beyond actual data range (Full capacity is available for writing)
  if(bleft <= Size)
   {
    if(this->Ptr >= this->Len)return SEOF;     // Nothing to read
    Size = bleft;
   }
  switch(Size)     // MemCopy is still too slow     // TODO: More switches? Do switches in MemCopy? Should care about misalignment?
   {
    case 0:
     return 0;
    case 1:  
     *(uint8*)DstBuf = *(uint8*)this->Ptr;
     break;
    case 2:
     *(uint16*)DstBuf = *(uint16*)this->Ptr;
     break;
    case 4:
     *(uint32*)DstBuf = *(uint32*)this->Ptr;
     break;
    case 8:
     *(uint64*)DstBuf = *(uint64*)this->Ptr;
     break;
    case 16:                // NOTE: May be Unaligned!
     if constexpr (HaveScalarI128)*(uint128*)DstBuf = *(uint128*)this->Ptr;
       else *(u128*)DstBuf = *(u128*)this->Ptr;
     break;
    default: MOPR::MemCopy(DstBuf, this->Ptr, Size);
   }
  if(!(Flags & rwfNoAdv))this->Ptr += Size;
  return Size;
 }
//-----------------------------------------------------
 virtual uint64 Write(vptr SrcBuf, usize Size, uint32 Flags=0) override   // TODO: Vec
 {
  usize bleft = (this->End - this->Ptr);
  if(bleft <= Size)      // No growth for this stream type here
   {
    if(this->Ptr >= this->End)return -PX::ENOSPC;  //SEOF;   // End of tape :)
    Size = bleft;
   }
  switch(Size)     // MemCopy is still too slow     // TODO: More switches? Do switches in MemCopy? Should care about misalignment?
   {
    case 0:
     return 0;
    case 1:  
     *(uint8*)this->Ptr = *(uint8*)SrcBuf;
     break;
    case 2:
     *(uint16*)this->Ptr = *(uint16*)SrcBuf;
     break;
    case 4:
     *(uint32*)this->Ptr = *(uint32*)SrcBuf;
     break;
    case 8:
     *(uint64*)this->Ptr = *(uint64*)SrcBuf;
     break;
    case 16:                // NOTE: May be misaligned!
     if constexpr (HaveScalarI128)*(uint128*)this->Ptr = *(uint128*)SrcBuf;
       else *(u128*)this->Ptr = *(u128*)SrcBuf;
     break;
    default: MOPR::MemCopy(this->Ptr, SrcBuf, Size);
   }
  if(!(Flags & rwfNoAdv))this->Ptr += Size;
  this->UpdateLen();
  return Size;
 } 
//-----------------------------------------------------
 virtual usize Rewind(void) override {this->Ptr = this->Beg; return 0;}
// Resets size and rewinds
 virtual usize Reset(void) override {this->Ptr = this->Len = this->Beg; return 0;}       // Reset internal state, discard data, will not delete any files
}; 
//============================================================================================================
#include "BitStream.hpp"
};   // STRM
//============================================================================================================
