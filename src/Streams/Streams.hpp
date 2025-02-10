
//============================================================================================================
struct NSTM
{
enum ESeek   // Same as in PX
{
 SEEK_SET  = 0,       // Seek relative to begining of file
 SEEK_CUR  = 1,       // Seek relative to current file position
 SEEK_END  = 2,       // Seek relative to end of file
};

SCVR size_t SFAIL = (size_t)-2;
SCVR size_t SNONE = (size_t)-1;
// TODO: Check perfomane - dynamic vs static polymorphism

// Stream functions return -1 if failed, -2 if not applicable

class CStrmBase
{
public:
 static bool IsFail(size_t v){return v >= ~size_t(0xFFF);}

 //virtual ~CStrmBase(){};         // Useless!  Don't even try to delete that you don't own!
 virtual size_t Size(void){return SNONE;}
 virtual size_t Offs(void){return SNONE;}                       // Returns offset from beginning or -1/-2
 virtual size_t Seek(size_t offs, ESeek wh){return SNONE;}      // Returns new offset from beginning or -1/-2
 virtual size_t Read(vptr DstBuf, size_t Size, bool vec=false){return SNONE;}     // If vec is true then BUF points to PX::iovec and SIZE is number of iovec structures
 virtual size_t Write(vptr SrcBuf, size_t Size, bool vec=false){return SNONE;}
 virtual size_t Rewind(void){return SNONE;}
 virtual size_t Reset(void){return SNONE;}       // Reset internal state, discard data, will not delete any files
 virtual size_t Discard(void){return SNONE;}     // Discard all data and reset the stream, delete temporary file - job interrupted
};
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
ssize_t Open(achar* pathname, uint32 flags=NPTM::PX::O_RDWR|NPTM::PX::O_CREAT, uint32 mode=0)   // Flags: 0,1,2 = O_RDONLY,O_WRONLY,O_RDWR
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
virtual size_t Size(void) override
{
 NPTM::PX::SFStat sti;
 size_t res = NPTM::NAPI::fstat(this->fd, &sti);
 if(res)return SFAIL;
 return sti.size;
}
//-----------------------------------------------------
virtual size_t Offs(void) override {return NPTM::NAPI::lseek(this->fd,0,NPTM::PX::SEEK_CUR);}
//-----------------------------------------------------
virtual size_t Seek(size_t offs, ESeek wh) override {return NPTM::NAPI::lseek(this->fd,offs,(NPTM::PX::ESeek)wh);}
//-----------------------------------------------------
virtual size_t Read(vptr DstBuf, size_t Size, bool vec=false) override
{
 if(vec)return NPTM::NAPI::readv(this->fd,(NPTM::PX::PIOVec)DstBuf, Size);
   else return NPTM::NAPI::read(this->fd, DstBuf, Size);
}
//-----------------------------------------------------
virtual size_t Write(vptr SrcBuf, size_t Size, bool vec=false) override
{
 if(vec)return NPTM::NAPI::writev(this->fd,(NPTM::PX::PIOVec)SrcBuf, Size);
   else return NPTM::NAPI::write(this->fd, SrcBuf, Size);
}
//-----------------------------------------------------
virtual size_t Rewind(void) override {return NPTM::NAPI::lseek(this->fd,0,NPTM::PX::SEEK_SET);}
//-----------------------------------------------------
virtual size_t Reset(void) override        // ????????????? Need the file name to reopen it???
{
 this->Close();
 return 0;
}
//-----------------------------------------------------
virtual size_t Discard(void) override         // ????????????? Need the file name to unlink it???
{
 NPTM::NAPI::ftruncate(this->fd, 0);      // No delete on close?  // unlink requires a file path
 this->Close();
 return 0;
}
};
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
