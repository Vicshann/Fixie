
#pragma once
template<typename T, int ATerm=-1> class CArr    // TODO: Allocator based?
{
 using Self = CArr<T,ATerm>;
 static constexpr const sint DescLen  = sizeof(uint)*2;
 static constexpr const sint SizeIdx  = -1;
 static constexpr const sint ASizeIdx = -2;
 static constexpr const sint BeginIdx = (DescLen/sizeof(uint));   // 2 on x64, 4 on x32
 T* AData;

//----------------------------------------------------------
// Ptr is Base + 16
static bool DeAllocate(vptr Ptr)      // TODO: Use allocator
{
 if(!Ptr)return true;
 uint* DPtr = (uint*)Ptr;
 if(NPTM::NAPI::munmap(&DPtr[-BeginIdx], DPtr[ASizeIdx]) < 0){ DBGERR("Failed to deallocate!"); return false; }
 return true;
}
//----------------------------------------------------------
static vptr Allocate(uint Len)
{
 uint  flen = AlignFrwdP2(Len + DescLen, NPTM::MEMPAGESIZE);
 void* fdat = (void*)NPTM::NAPI::mmap(nullptr, flen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);    // NOTE: No attempts to grow inplace!!!
 if(NPTM::GetMMapErrFromPtr(fdat)){ DBGERR("Error: Failed to allocate!"); return nullptr; }
 uint* Ptr  = &((uint*)fdat)[BeginIdx];
 Ptr[SizeIdx]  = Len;
 Ptr[ASizeIdx] = flen;
 return Ptr;
}
//----------------------------------------------------------
static vptr ReAllocate(uint Len, vptr OldPtr)  // TODO: Use allocator   // TODO: use mremap    
{
 if(!OldPtr)return Allocate(Len);
 uint flen = AlignFrwdP2(Len + DescLen, NPTM::MEMPAGESIZE);
 if(flen <= ((uint*)OldPtr)[ASizeIdx])   // Fits in last allocation
  {
   ((uint*)OldPtr)[SizeIdx] = Len;
   return OldPtr;
  }
 vptr NewPtr = Allocate(Len);
 if(NewPtr)memcpy(NewPtr, OldPtr, ((uint*)OldPtr)[SizeIdx]);  // Copy old data
 DeAllocate(OldPtr);    // NOTE: May fail and leak memory
 return NewPtr;
}
//----------------------------------------------------------

public:
 CArr(void){this->AData = nullptr;}
 CArr(uint Cnt){this->AData = nullptr; this->Resize(Cnt);}
 ~CArr(){this->SetLength(0);}
 operator  T*() const {return this->AData;}     // operator   const T*() {return this->AData;}
 T* Data(void) const {return this->AData;}
 T* c_data(void) const {return this->AData;}    // For name compatibility in a templates
 T* Ptr(uint at)const {return &this->AData[at];} // NOTE: No checks
 T* Get(uint at)const {return &this->AData[at];}
 usize Count(void) const {return (this->Size() / sizeof(T));}
 usize Size(void) const {return ((this->AData)?(((uint*)this->AData)[SizeIdx]):(0));}
 usize AllocSize(void) const {return ((this->AData)?(((uint*)this->AData)[ASizeIdx] - DescLen):(0));}
 usize Length(void) const {return this->Size();}

//----------------------------------------------------------
sint Clear(void){return this->SetLength(0);}
void Nullify(void){this->AData = nullptr;}
//----------------------------------------------------------
sint TakeFrom(Self* arr)
{
 if(sint res=this->SetLength(0);res < 0)return res;
 this->AData = arr->AData;
 arr->AData = nullptr;
 return 0;
}
//----------------------------------------------------------
sint MoveTo(Self* arr)
{
 if(sint res=arr->SetLength(0);res < 0)return res;
 arr->AData = this->AData;
 this->AData = nullptr;
 return 0;
}
//----------------------------------------------------------
T* Assign(const void* Items, uint Cnt=1)     // Cnt is in Items
{
 uint NewLen = Cnt * sizeof(T);
 if(sint res=this->SetLength(NewLen);res < 0)return nullptr;
 if(Items)memcpy(this->AData, Items, NewLen);
 return this->AData;
}
//----------------------------------------------------------
T* Append(const void* Items, uint Cnt=1)     // Cnt is in Items
{
 uint OldSize = this->Size();
 uint NewLen  = Cnt * sizeof(T);
 if(sint res=this->SetLength(OldSize+NewLen);res < 0)return nullptr;
 if(Items)memcpy(&((uint8*)this->AData)[OldSize], Items, NewLen);
 //DBGTRC("DST=%p, SRC=%p, LEN=%08X",&((uint8*)this->AData)[OldSize], Items, NewLen);
 return (T*)&((uint8*)this->AData)[OldSize];
}
//----------------------------------------------------------
sint Insert(const void* Items, uint Cnt, uint At)
{
 uint OldSize = this->Size();
 uint ExtLen  = Cnt * sizeof(T);
 uint AtOffs  = At * sizeof(T);
 if(sint res=this->SetLength(OldSize+ExtLen);res < 0)return res;
 memmove(&((uint8*)this->AData)[AtOffs+ExtLen], &((uint8*)this->AData)[AtOffs], OldSize - AtOffs);
 if(Items)memcpy(&((uint8*)this->AData)[AtOffs], Items, ExtLen);
 return 0;
}
//----------------------------------------------------------
sint Remove(uint Cnt, uint At)
{
 return 0;  // Really needed realloc!
}
//----------------------------------------------------------
 //CArr<T>& operator += (const char* str){this->Append((void*)str, lstrlenA(str)); return *this;}
//----------------------
 //CArr<T>& operator += (const wchar_t* str){this->Append((void*)str, lstrlenW(str)); return *this;}
//----------------------
T&  operator [] (ssize index){return this->AData[index];}
//----------------------
void  operator = (const Self &arr){this->Assign(arr.Data(), arr.Count());}   // Must be defined or '=' op will steal the memory

inline Self& operator += (const Self& arr){this->Append(arr.AData, arr.Size()); return *this;}
template<typename A> inline Self& operator += (const A* src)
{
 static_assert(sizeof(A) == sizeof(T));  // At least size must match!
 uint len;
 if constexpr (ATerm >= 0)    // I.E. if this array is for a string then it expects a null-terminated source
  {
   for(len=0;src[len] != (A)ATerm;)len++;
  }
   else len = 1;   // Can't know the size so just take an one element
 this->Append((vptr)&src, len);
 return *this;
}
//----------------------------------------------------------
template<typename A, uint N> inline Self& operator += (const A(&src)[N])
{
 static_assert(sizeof(A) == sizeof(T));  // At least size must match!
 uint len; // = N;  //(N && (sizeof(*src) == 1) && !src[N-1])?(N-1):(N);
 if constexpr (ATerm >= 0)
  {
   for(len=0;(src[len] != (A)ATerm)&&(len < N);)len++;
  }
   else len = N;
 this->Append((vptr)&src, len);
 return *this;
}
//----------------------------------------------------------
inline ssize Resize(uint Cnt){return this->SetLength(Cnt*sizeof(T));}  // In Elements     // TODO: Grow mode
inline ssize SetSize(uint Len) {return this->SetLength(Len);}  // In bytes!
ssize SetLength(uint Len)    // In bytes!     // Preserves old data!   // TODO: Prealloc!
{
 uint* Ptr = (uint*)this->AData;
 if(Len && Ptr)Ptr = (uint*)ReAllocate(Len, Ptr);    // Resize (by reallocation)
	else if(!Ptr && Len)Ptr = (uint*)Allocate(Len);  //
	  else if(!Len && Ptr)   // Free the memory
    {
     DeAllocate(Ptr);
     this->AData = nullptr;
     return 0;  // Free
    }
      else return 0;  // Do nothing
 if(!Ptr)return -PX::ENOMEM;   // Allocation failed
 this->AData = (T*)Ptr;
 return ssize(Len);
}
//----------------------------------------------------------
sint FromFile(const achar* FileName, sint RootDir=PX::AT_FDCWD)
{
 sint df = NPTM::NAPI::openat(RootDir,FileName,PX::O_RDONLY,0);
 if(df < 0){ DBGERR("Failed to open the file %i: '%s'!",df,FileName); return (sint)df; }
 sint flen = NPTM::NAPI::lseek(df, 0, PX::SEEK_END);    // TODO: Use fstat
 if(flen < 0){NPTM::NAPI::close(df); return flen;}
 NPTM::NAPI::lseek(df, 0, PX::SEEK_SET);     // Set the file position back
 if(sint res=this->SetLength(flen);res < 0){NPTM::NAPI::close(df); return res;}
 sint rlen = NPTM::NAPI::read(df, this->AData, this->Size());
 NPTM::NAPI::close(df);
 if(rlen < 0)return rlen;
 if(rlen != flen){ DBGERR("Data size mismatch!"); return PX::EFBIG; }
 return sint(rlen / sizeof(T));
}
//----------------------------------------------------------
sint IntoFile(const achar* FileName, uint Length=0, uint Offset=0, sint RootDir=PX::AT_FDCWD)       // TODO: Bool Append    // From - To
{
 uint DataSize = this->Size();
 if(Offset >= DataSize)return 0;  // Beyond the data
 if(!Length)Length = DataSize;
 if((Offset+Length) > DataSize)Length = (DataSize - Offset);
 if(!Length)return 0;
 sint df = NPTM::NAPI::openat(RootDir,FileName,PX::O_CREAT|PX::O_WRONLY|PX::O_TRUNC, 0666);   // O_TRUNC  O_EXCL   // 0666
 if(df < 0){ DBGERR("Failed to create the output data file(%i): '%s'!",df,FileName); return (sint)df; }
 sint wlen = NPTM::NAPI::write(df, &((uint8*)this->AData)[Offset], Length);
 NPTM::NAPI::close(df);
 if(wlen < 0)return wlen;
 if((size_t)wlen != this->Size()){ DBGERR("Data size mismatch!"); return PX::ENOSPC; }
 return (wlen / sizeof(T));
}
//----------------------------------------------------------

};

using CAChArr = CArr<achar,0>;
using CWChArr = CArr<wchar,0>;
//---------------------------------------------------------------------------
