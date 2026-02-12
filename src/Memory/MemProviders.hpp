

//============================================================================================================
struct alignas(usize) SMemProvBase      // Base/Default memory provider  // Min size is 4096 and min alignment is 4096  // TODO: The interface verification
{
 usize Granularity(void) const {return NPTM::MEMGRANSIZE;}    // On Windows Allocations  of less than a page will leave holes in address space   TODO.PERF: Test fragmentation 

 vptr Alloc(usize* len)  // For block allocations,   // May return a handle instead of actual memory
 {
  usize size = AlignFrwdP2(*len, this->Granularity());
  vptr BPtr = (vptr)NPTM::NAPI::mmap(nullptr, size, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   // Executable?  // Some platforms may refuse allocating of RWX memory
  if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
  *len = size;
  return BPtr;
 }

 bool Free(vptr ptr, usize len){return NPTM::NAPI::munmap(ptr, len) >= 0;}    // Size is optional
 
 vptr Lock(vptr ptr, usize len, usize offs=0){return ptr;}             // Size is optional      // Returns actual memory pointer   // NOTE: Do not expect to store any contexts or headers in memory that requires this
 
 bool UnLock(vptr ptr, usize len, usize offs=0){return true;}          // Size is optional
 
 vptr ReAlloc(vptr optr, usize olen, usize* nlen, bool maymove=true)   // May return a handle   // TODO: Implement mremap syscall  // NOTE: may fail if MayMove is false and ptr is not a handle
 {
  if(!optr)return this->Alloc(nlen);   // Should just allocate if ptr is NULL
  usize size = AlignFrwdP2(*nlen, this->Granularity());
  vptr BPtr = (vptr)NPTM::NAPI::mremap(optr, olen, size, !maymove?PX::MREMAP_FIXED:0, nullptr);
  if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
  *nlen = size;
  return nullptr; 
 } 
};
//============================================================================================================
