#pragma once

// Helpers for thunking 32-bit NT structures to 64-bit variants and back.

//------------------------------------------------------------------------------------------------------------
static _finline void Expand_UNICODE_STRING(NT64::UNICODE_STRING* dst, NT::PUNICODE_STRING src)     // TODO: Return DST pointers
{
 if (!src) { dst->Length = 0; dst->MaximumLength = 0; dst->Buffer = nullptr; return; }
 dst->Length = src->Length;
 dst->MaximumLength = src->MaximumLength;
 dst->Buffer = (uint64)src->Buffer;
}
//------------------------------------------------------------------------------------------------------------
static _finline void Collapse_UNICODE_STRING(NT::PUNICODE_STRING dst, NT64::UNICODE_STRING* src)
{
 if (!dst) return;
 dst->Length = (NT::USHORT)src->Length;
 dst->MaximumLength = (NT::USHORT)src->MaximumLength;
 dst->Buffer = (NT::PWSTR)(usize)src->Buffer; // trust caller to ensure validity
}
//------------------------------------------------------------------------------------------------------------
static _finline void ExpandOBJECT_ATTRIBUTES(NT64::OBJECT_ATTRIBUTES* dst, NT::POBJECT_ATTRIBUTES src)
{
 if(!src) 
  {
   dst->Length = sizeof(dst);
   dst->RootDirectory = 0;
   dst->Attributes = 0;
   dst->ObjectName->Length = 0;
   dst->ObjectName->MaximumLength = 0;
   dst->ObjectName->Buffer = nullptr;
   dst->SecurityDescriptor = nullptr;
   dst->SecurityQualityOfService = nullptr;   
   return;
  }
 dst->Length = sizeof(dst);
 dst->RootDirectory = WOW64::HndlExp(src->RootDirectory);
 dst->Attributes = src->Attributes;
 if(src->ObjectName)Expand_UNICODE_STRING(dst->ObjectName, src->ObjectName);
 dst->SecurityDescriptor = (uint64)src->SecurityDescriptor;
 dst->SecurityQualityOfService = (uint64)src->SecurityQualityOfService;
}
//------------------------------------------------------------------------------------------------------------
static _finline void CollapseOBJECT_ATTRIBUTES(NT::POBJECT_ATTRIBUTES dst, NT64::OBJECT_ATTRIBUTES* src)
{
 if (!dst) return;
 dst->Length = sizeof(*dst);
 dst->RootDirectory = (NT::HANDLE)(uint32)src->RootDirectory;
 dst->Attributes = src->Attributes;
 // ObjectName pointer stays the same in the 32-bit caller (we don't allocate)
 if (dst->ObjectName)
 Collapse_UNICODE_STRING(dst->ObjectName, src->ObjectName);
 dst->SecurityDescriptor = src->SecurityDescriptor;
 dst->SecurityQualityOfService = vptr((size_t)src->SecurityQualityOfService);
}
//------------------------------------------------------------------------------------------------------------
static _finline void ExpandIO_STATUS_BLOCK(NT64::IO_STATUS_BLOCK &dst, NT::PIO_STATUS_BLOCK src)
{
 if (!src) { dst.Status = 0; dst.Information = 0; return; }
 dst.Status = (uint64)src->Status;
 dst.Information = (uint64)src->Information;
}
//------------------------------------------------------------------------------------------------------------
static _finline void CollapseIO_STATUS_BLOCK(NT::PIO_STATUS_BLOCK dst, NT64::IO_STATUS_BLOCK &src)
{
 if(!dst) return;
 dst->Status = (NT::NTSTATUS)src.Status;
 dst->Information = (size_t)src.Information;
}
//------------------------------------------------------------------------------------------------------------
static _finline void ExpandCLIENT_ID(NT64::CLIENT_ID &dst, NT::PCLIENT_ID src)
{
 if(!src) { dst.UniqueProcess = 0; dst.UniqueThread = 0; return; }
 dst.UniqueProcess = (uint64)(uint32)src->UniqueProcess;
 dst.UniqueThread = (uint64)(uint32)src->UniqueThread;
}
//------------------------------------------------------------------------------------------------------------
static _finline void CollapseCLIENT_ID(NT::PCLIENT_ID dst, NT64::CLIENT_ID &src)
{
 if(!dst) return;
 dst->UniqueProcess = (NT::HANDLE)(uint32)src.UniqueProcess;
 dst->UniqueThread = (NT::HANDLE)(uint32)src.UniqueThread;
}
//------------------------------------------------------------------------------------------------------------
static _finline void ExpandLARGE_INTEGER(NT64::LARGE_INTEGER* dst, NT::PLARGE_INTEGER src)
{
 //dst.QuadPart = src ? src->QuadPart : 0;
}
//------------------------------------------------------------------------------------------------------------
static _finline void CollapseLARGE_INTEGER(NT::PLARGE_INTEGER dst, NT64::LARGE_INTEGER &src)
{
 if (!dst) return;
 //dst->QuadPart = src.QuadPart;
}
//------------------------------------------------------------------------------------------------------------
// Minimal CONTEXT translator: copy core integer registers. Expand this if you need debug/fpu/xmm state.
static _finline void ExpandCONTEXT(NT64::CONTEXT &dst, NT::PCONTEXT src)
{
 if (!src) {memset(&dst, 0, sizeof(dst)); return; }
 // Map common registers (x86 CONTEXT layout differs; caller should set flags appropriately)
//dst.Rax = (uint64)(uint32)src->Eax;
//dst.Rbx = (uint64)(uint32)src->Ebx;
//dst.Rcx = (uint64)(uint32)src->Ecx;
//dst.Rdx = (uint64)(uint32)src->Edx;
//dst.Rsi = (uint64)(uint32)src->Esi;
//dst.Rdi = (uint64)(uint32)src->Edi;
//dst.Rbp = (uint64)(uint32)src->Ebp;
//dst.Rsp = (uint64)(uint32)src->Esp;
//dst.Rip = (uint64)(uint32)src->Eip;
//dst.EFlags = (uint64)src->EFlags;
}
//------------------------------------------------------------------------------------------------------------
static _finline void CollapseCONTEXT(NT::PCONTEXT dst, NT64::CONTEXT &src)
{
 if (!dst) return;
//dst->Eax = (uint32)src.Rax;
//dst->Ebx = (uint32)src.Rbx;
//dst->Ecx = (uint32)src.Rcx;
//dst->Edx = (uint32)src.Rdx;
//dst->Esi = (uint32)src.Rsi;
//dst->Edi = (uint32)src.Rdi;
//dst->Ebp = (uint32)src.Rbp;
//dst->Esp = (uint32)src.Rsp;
//dst->Eip = (uint32)src.Rip;
//dst->EFlags = (uint32)src.EFlags;
}
//------------------------------------------------------------------------------------------------------------

// Small utilities if you prefer them to in-place casts
//static _finline uint64 Ptr32To64(void* p) { return (uint64)(uintptr)p; }
//static _finline void Ptr64ToPtr32(void** dst, uint64 val) { if (dst) *dst = (void*)(uintptr)val; }

