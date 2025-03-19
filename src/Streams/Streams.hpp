
//============================================================================================================
struct STRM
{
enum ESeek   // Same as in PX
{
 SEEK_SET  = 0,       // Seek relative to begining of file
 SEEK_CUR  = 1,       // Seek relative to current file position
 SEEK_END  = 2,       // Seek relative to end of file
};

// NOTE: Those are EXACT values, not just any negative value (Last 4096 bytes of SIZE_T ir reserved for it (POSIX error))
SCVR usize SNONE = (usize)-3;      // Not implemented
SCVR usize SFAIL = (usize)-2;      // The operation has failed
SCVR usize SEOF  = (usize)-1;      // EOF can be generated with Ctrl-Z on Windows and Ctrl-D on many other OSes   // NOTE: Conflicts with POSIX::EPERM
// TODO: Check perfomane - dynamic vs static polymorphism


class CStrmBase
{
public:
// Stream functions return -1 if failed, -2 if not applicable   // Must be here to be accessible from 'auto' args of allocators (out of seq)
 static _finline bool IsEOF(usize v){return !~v;}
 static _finline bool IsFail(usize v){return v >= ~usize(0xFFF);}

 //virtual ~CStrmBase(){};         // Useless!  Don't even try to delete that you don't own!
 virtual usize Size(void){return SNONE;}
 virtual usize Offset(void){return SNONE;}                       // Returns offset from beginning or -1/-2
 virtual usize Seek(usize offs, ESeek wh){return SNONE;}      // Returns new offset from beginning or -1/-2
 virtual usize Read(vptr DstBuf, usize Size, bool vec=false){return SNONE;}     // If vec is true then BUF points to PX::iovec and SIZE is number of iovec structures
 virtual usize Write(vptr SrcBuf, usize Size, bool vec=false){return SNONE;}
 virtual usize Rewind(void){return SNONE;}
 virtual usize Reset(void){return SNONE;}       // Reset internal state, discard data, will not delete any files
 virtual usize Discard(void){return SNONE;}     // Discard all data and reset the stream, delete temporary file - job interrupted
};     // TODO: Read8,read16,read32,read64 ???  May be optimized for buffer streams
//============================================================================================================
class CStrmFile: public CStrmBase
{
 NPTM::PX::fdsc_t fd;

public:
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
 if(this->fd < 0)return -1;
 NPTM::NAPI::close(this->fd);
 return 0;
}
//-----------------------------------------------------
int Flush(void){return NPTM::NAPI::fsync(this->fd);}
//-----------------------------------------------------
virtual usize Size(void) override
{
 NPTM::PX::SFStat sti;
 usize res = NPTM::NAPI::fstat(this->fd, &sti);
 if(res)return SFAIL;
 return sti.size;
}
//-----------------------------------------------------
virtual usize Offset(void) override {return NPTM::NAPI::lseek(this->fd,0,NPTM::PX::SEEK_CUR);}
//-----------------------------------------------------
virtual usize Seek(usize offs, ESeek wh) override {return NPTM::NAPI::lseek(this->fd,offs,(NPTM::PX::ESeek)wh);}
//-----------------------------------------------------
virtual usize Read(vptr DstBuf, usize Size, bool vec=false) override
{
 if(vec)return NPTM::NAPI::readv(this->fd,(NPTM::PX::PIOVec)DstBuf, Size);
   else return NPTM::NAPI::read(this->fd, DstBuf, Size);
}
//-----------------------------------------------------
virtual usize Write(vptr SrcBuf, usize Size, bool vec=false) override
{
 if(vec)return NPTM::NAPI::writev(this->fd,(NPTM::PX::PIOVec)SrcBuf, Size);
   else return NPTM::NAPI::write(this->fd, SrcBuf, Size);
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
class CStrmBuf: public CStrmBase
{
 uint8* Beg;
 uint8* End;
 uint8* Ptr;
public:
 void Init(vptr Data, usize size){this->Ptr = this->Beg = (uint8*)Data; this->End = this->Beg + size;}

 virtual usize Size(void){return this->End - this->Beg;}
 virtual usize Offset(void){return this->Ptr - this->Beg;}                       // Returns offset from beginning or -1/-2
 virtual usize Seek(usize offs, ESeek wh){return SNONE;}    // TODO!!!  // Returns new offset from beginning or -1/-2
//-----------------------------------------------------
 virtual usize Read(vptr DstBuf, usize Size, bool vec=false)   // If vec is true then BUF points to PX::iovec and SIZE is number of iovec structures
 {
  //usize left = this->End - this->Ptr;
  //if(Size > left)Size = 
  switch(Size)
   {
    case 1:
     if(this->Ptr >= this->End)return false;
     *(uint8*)DstBuf = *(this->Ptr++);
     break;
    case 2:
     break;
    case 4:
     break;
    case 8:
     break;
   }
  return true;
 }
//-----------------------------------------------------
 virtual usize Write(vptr SrcBuf, usize Size, bool vec=false){return SNONE;}  // TODO!!!
 virtual usize Rewind(void){return this->Reset();}
 virtual usize Reset(void){this->Ptr = this->Beg; return 0;}       // Reset internal state, discard data, will not delete any files
 virtual usize Discard(void){return SNONE;}     // Discard all data and reset the stream, delete temporary file - job interrupted
}; 
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
