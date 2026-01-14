
#pragma once

// NOTE: Clang just refuses to accept __asm { } blocks even with -fasm-blocks
//============================================================================================================
class WOW64E
{
//------------------------------------------------------------------------------------------------------------
// NOTE: Cam't use cevent 'constexpr const' inside '__asm __emit ()'
 /*
#define _RAX   0
#define _RCX   1
#define _RDX   2
#define _RBX   3
#define _RSP   4
#define _RBP   5
#define _RSI   6
#define _RDI   7
#define _R8    8
#define _R9    9
#define _R10  10
#define _R11  11
#define _R12  12
#define _R13  13
#define _R14  14
#define _R15  15

#define X64_Push(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x50 | ((r) & 7))
#define X64_Pop(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x58 | ((r) & 7))
*/

#define EMIT(a) __asm __emit (a)

// ';' is required
#define X64_Start_with_CS(_cs) \
    ;{ \
    EMIT(0x6A) EMIT(_cs)                         /*  push   _cs             */ \
    EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)   /*  call   $+5             */ \
    EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(5)        /*  add    dword [esp], 5  */ \
    EMIT(0xCB)                                   /*  retf                   */ \
    };

#define X64_End_with_CS(_cs) \
    ;{ \
    EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)                                 /*  call   $+5                   */ \
    EMIT(0xC7) EMIT(0x44) EMIT(0x24) EMIT(4) EMIT(_cs) EMIT(0) EMIT(0) EMIT(0) /*  mov    dword [rsp + 4], _cs  */ \
    EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(0xD)                                    /*  add    dword [rsp], 0xD      */ \
    EMIT(0xCB)                                                                 /*  retf                         */ \
    };

#define X64_Start() X64_Start_with_CS(0x33)
#define X64_End() X64_End_with_CS(0x23)

// 0100 - fixed constant value
//    W - 1 when using 64-bit data
//    R - expands the Reg field to 4 bit
//    X - expands Index field to 4 bit
//    B - expands the R/M field or Base to 4 bit

// NOTE: The REX naming is random here
#define REX_B EMIT(0x49) __asm
#define REX_R EMIT(0x4C) __asm
#define REX_W EMIT(0x48) __asm
#define REP_W EMIT(0xF3) EMIT(0x48) __asm

union reg64    // use of just uint64 will generate wrong 'pop word ptr[]' and it will break stack
{
 uint64 v;
 uint32 dw[2];
};

SCVR uint64 MaxAddrX32 = 0x7FFFFFFF;
//------------------------------------------------------------------------------------------------------------

public:

//------------------------------------------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable : 4409)
static uint64 _ccall X64Call(uint64 func, uint32 argC, ...)    // TODO: Optimize
{
 va_list args;
 va_start(args, argC);
 reg64 _rcx = { argC ? argC--, va_arg(args, uint64) : 0 };   // va_arg reads at args and increments it by the value size
 reg64 _rdx = { argC ? argC--, va_arg(args, uint64) : 0 };
 reg64 _r8  = { argC ? argC--, va_arg(args, uint64) : 0 };
 reg64 _r9  = { argC ? argC--, va_arg(args, uint64) : 0 };
 reg64 _rax = { 0 };

 reg64 restArgs = { (uint32)args };  

 // conversion to QWORD for easier use in inline assembly
 reg64 _argC = { (uint64)argC };
 uint32 back_esp = 0;
 uint16 back_fs  = 0;

    // reset FS segment, to properly handle RFG
  __asm    mov    back_fs, fs
  __asm    mov    eax, 0x2B
  __asm    mov    fs, ax

    // keep original esp in back_esp variable
  __asm    mov    back_esp, esp

    // align esp to 0x10, without aligned stack some syscalls may return errors !
    // (actually, for syscalls it is sufficient to align to 8, but SSE opcodes require 0x10 alignment), it will be further adjusted according to the number of arguments above 4
  __asm    and    esp, 0xFFFFFFF0                    

    X64_Start()                        

    // below code is compiled as x86 inline asm, but it is executed as x64 code that's why it need sometimes REX_W() macro, right column contains detailed transcription how it will be interpreted by CPU

    // fill first four arguments
  REX_W    mov    ecx, _rcx.dw[0]                          // mov     rcx, qword ptr [_rcx]
  REX_W    mov    edx, _rdx.dw[0]                          // mov     rdx, qword ptr [_rdx]
 // __asm    push   _r8.v                                    // push    qword ptr [_r8]
 //   X64_Pop(_R8);                                          // pop     r8
 // __asm    push   _r9.v                                    // push    qword ptr [_r9]
 //   X64_Pop(_R9);                                          // pop     r9
  REX_R    mov    eax, _r8.dw[0]                         // mov     r8, qword ptr [_r8]
  REX_R    mov    ecx, _r9.dw[0]                         // mov     r9, qword ptr [_r9]

  REX_W    mov    eax, _argC.dw[0]                         // mov     rax, qword ptr [_argC]

    // final stack adjustment, according to the
    // number of arguments above 4
  __asm    test   al, 1                                    // test    al, 1
  __asm    jnz    _no_adjust                               // jnz     _no_adjust
  __asm    sub    esp, 8                                   // sub     rsp, 8
__asm  _no_adjust:

  __asm    push   edi                                      // push    rdi
  REX_W    mov    edi, restArgs.dw[0]                      // mov     rdi, qword ptr [restArgs]

    // put rest of arguments on the stack
  REX_W    test   eax, eax                                 // test    rax, rax
  __asm    jz     _ls_e                                    // je      _ls_e
  REX_W    lea    edi, dword ptr [edi + 8*eax - 8]         // lea     rdi, [rdi + rax*8 - 8]

__asm _ls:
  REX_W    test   eax, eax                                 // test    rax, rax
  __asm    jz     _ls_e                                    // je      _ls_e
  __asm    push   dword ptr [edi]                          // push    qword ptr [rdi]
  REX_W    sub    edi, 8                                   // sub     rdi, 8
  REX_W    sub    eax, 1                                   // sub     rax, 1
  __asm    jmp    _ls                                      // jmp     _ls
__asm  _ls_e:

    // create stack space for spilling registers
  REX_W    sub    esp, 0x20                                // sub     rsp, 20h

  __asm    call   func                                     // call    qword ptr [func]

    // cleanup stack
  REX_W    mov    ecx, _argC.dw[0]                         // mov     rcx, qword ptr [_argC]
  REX_W    lea    esp, dword ptr [esp + 8*ecx + 0x20]      // lea     rsp, [rsp + rcx*8 + 20h]

  __asm    pop    edi                                      // pop     rdi

    // set return value
  REX_W    mov    _rax.dw[0], eax                          // mov     qword ptr [_rax], rax    

    X64_End()                         

  __asm    mov    ax, ds
  __asm    mov    ss, ax
  __asm    mov    esp, back_esp

    // restore FS segment
  __asm    mov    ax, back_fs
  __asm    mov    fs, ax

 return _rax.v;
}
//------------------------------------------------------------------------------------------------------------
static uint64 _ccall X64SysCall(uint32 scIdx, uint32 argC, ...)    // TODO: Optimize
{
 va_list args;
 va_start(args, argC);
 reg64 _rcx = { argC ? argC--, va_arg(args, uint64) : 0 };   // va_arg reads at args and increments it by the value size
 reg64 _rdx = { argC ? argC--, va_arg(args, uint64) : 0 };
 reg64 _r8  = { argC ? argC--, va_arg(args, uint64) : 0 };
 reg64 _r9  = { argC ? argC--, va_arg(args, uint64) : 0 };
 reg64 _rax = { 0 };

 //reg64 _test0 = { 0 };  // rcx
 //reg64 _test1 = { 0 };  // rdx
 //reg64 _test2 = { 0 };  // r8
 //reg64 _test3 = { 0 };  // r9

 reg64 restArgs = { (uint32)args };  

 // conversion to QWORD for easier use in inline assembly
 reg64 _sysC = { (uint64)scIdx };
 reg64 _argC = { (uint64)argC };
 uint32 back_esp = 0;
 uint16 back_fs  = 0;

    // reset FS segment, to properly handle RFG
  __asm    mov    back_fs, fs
  __asm    mov    eax, 0x2B
  __asm    mov    fs, ax

    // keep original esp in back_esp variable
  __asm    mov    back_esp, esp

    // align esp to 0x10, without aligned stack some syscalls may return errors !
    // (actually, for syscalls it is sufficient to align to 8, but SSE opcodes require 0x10 alignment), it will be further adjusted according to the number of arguments above 4
  __asm    and    esp, 0xFFFFFFF0          

    X64_Start()                            

    // below code is compiled as x86 inline asm, but it is executed as x64 code that's why it need sometimes REX_W() macro, right column contains detailed transcription how it will be interpreted by CPU

    // fill first four arguments
  REX_W    mov    ecx, _rcx.dw[0]                          // mov     rcx, qword ptr [_rcx]
  REX_W    mov    edx, _rdx.dw[0]                          // mov     rdx, qword ptr [_rdx]
 // __asm    push   _r8.v                                    // push    qword ptr [_r8]
 //   X64_Pop(_R8);                                          // pop     r8
//  __asm    push   _r9.v                                    // push    qword ptr [_r9]
//    X64_Pop(_R9);                                          // pop     r9
  REX_R    mov    eax, _r8.dw[0]                         // mov     r8, qword ptr [_r8]
  REX_R    mov    ecx, _r9.dw[0]                         // mov     r9, qword ptr [_r9]

//REX_W    mov    _test0.dw[0], ecx
//REX_W    mov    _test1.dw[0], edx
//REX_R    mov    _test2.dw[0], eax                         // R8
//REX_R    mov    _test3.dw[0], ecx                         // R9

  REX_W    mov    eax, _argC.dw[0]                         // mov     rax, qword ptr [_argC]

 // final stack adjustment, according to the number of arguments above 4
  __asm    test   al, 1                                    // test    al, 1
  __asm    jnz    _no_adjust                               // jnz     _no_adjust
  __asm    sub    esp, 8                                   // sub     rsp, 8
__asm  _no_adjust:

  __asm    push   edi                                      // push    rdi
  REX_W    mov    edi, restArgs.dw[0]                      // mov     rdi, qword ptr [restArgs]

    // put rest of arguments on the stack
  REX_W    test   eax, eax                                 // test    rax, rax       // TODO: OPtimize this loop!
  __asm    jz     _ls_e                                    // je      _ls_e
  REX_W    lea    edi, dword ptr [edi + 8*eax - 8]         // lea     rdi, [rdi + rax*8 - 8]

__asm _ls:
  REX_W    test   eax, eax                                 // test    rax, rax
  __asm    jz     _ls_e                                    // je      _ls_e
  __asm    push   dword ptr [edi]                          // push    qword ptr [rdi]
  REX_W    sub    edi, 8                                   // sub     rdi, 8
  REX_W    sub    eax, 1                                   // sub     rax, 1    // dec rax is different on X64  // TODO: Just emit 48 FF C8
  __asm    jmp    _ls                                      // jmp     _ls
__asm  _ls_e:

    // create stack space for spilling registers
  REX_W    sub    esp, 0x20                                // sub     rsp, 20h

  REX_W    mov    eax, _sysC.dw[0]                         // mov rax, scIdx
  REX_B    mov    edx, ecx                                 // mov r10, rcx

//REX_W    mov    _test0.dw[0], ecx
//REX_W    mov    _test1.dw[0], edx
//REX_R    mov    _test2.dw[0], eax                         // R8
//REX_R    mov    _test3.dw[0], ecx                         // R9

  __asm    push eax                                         // Expects some return addr on stack!
  __asm    syscall                                         // call    qword ptr [func]

    // cleanup stack
  REX_W    mov    ecx, _argC.dw[0]                         // mov     rcx, qword ptr [_argC]
  REX_W    lea    esp, dword ptr [esp + 8*ecx + 0x20]      // lea     rsp, [rsp + rcx*8 + 20h]

  __asm    pop    edi                                      // pop     rdi

    // set return value
  REX_W    mov    _rax.dw[0], eax                          // mov     qword ptr [_rax], rax   

    X64_End()                                 

  __asm    mov    ax, ds
  __asm    mov    ss, ax
  __asm    mov    esp, back_esp

    // restore FS segment
  __asm    mov    ax, back_fs
  __asm    mov    fs, ax

 return _rax.v;
}
#pragma warning(pop)
//------------------------------------------------------------------------------------------------------------
static void _scall CopyMem64(uint64 dstMem, uint64 srcMem, uint32 len)
{
  X64_Start()    

  __asm    push   edi
  __asm    push   esi

  REX_W    mov    edi, dstMem
  REX_W    mov    esi, srcMem
  __asm    mov    eax, len

 // Do QWORDs
  __asm    mov    ecx, eax
  __asm    shr    ecx, 3
  REP_W    movsd                 // MOVSQ

// Do DWORDs
  __asm    and    eax, 7
  __asm    mov    ecx, eax
  __asm    shr    ecx, 2
  __asm    rep    movsd

// Do WORDs
  __asm    and    eax, 3
  __asm    mov    ecx, eax
  __asm    shr    ecx, 1
  __asm    rep    movsw          // REP helps to avoid any JMPs because it will skip the operation if ECX is 0

// Do BYTEs
  __asm    and    eax, 1
  __asm    mov    ecx, eax
  __asm    rep    movsb

  __asm    pop    esi
  __asm    pop    edi

  X64_End()          
}
//------------------------------------------------------------------------------------------------------------
static void _fcall getMem64(void* dstMem, uint64 srcMem, size_t len)
{
 if(!dstMem || !srcMem || !len)return;
 CopyMem64((uint64)dstMem, srcMem, (uint32)len);
}
//------------------------------------------------------------------------------------------------------------
static void _fcall setMem64(uint64 dstMem, void* srcMem, size_t len)
{
 if(!dstMem || !srcMem || !len)return;
 CopyMem64(dstMem, (uint64)srcMem, (uint32)len);
}
//------------------------------------------------------------------------------------------------------------
/*static bool _scall cmpMem64(void* dstMem, uint64 srcMem, size_t len)
{
    if (!dstMem || !srcMem || !len)return false;

    bool result = false;
    reg64 _src = { srcMem };
#ifndef _AMD64_
    __asm
    {
        X64_Start();

        ;// below code is compiled as x86 inline asm, but it is executed as x64 code
        ;// that's why it need sometimes REX_W() macro, right column contains detailed
        ;// transcription how it will be interpreted by CPU

        push   edi                  ;// push      rdi
        push   esi                  ;// push      rsi
                                    ;//
        mov    edi, dstMem          ;// mov       edi, dword ptr [dstMem]       ; high part of RDI is zeroed
  REX_W mov    esi, _src.dw[0]      ;// mov       rsi, qword ptr [_src]
        mov    ecx, len             ;// mov       ecx, dword ptr [len]           ; high part of RCX is zeroed
                                    ;//
        mov    eax, ecx             ;// mov       eax, ecx
        and    eax, 3               ;// and       eax, 3
        shr    ecx, 2               ;// shr       ecx, 2
                                    ;//
        repe   cmpsd                ;// repe cmps dword ptr [rsi], dword ptr [rdi]
        jnz     _ret_false          ;// jnz       _ret_false
                                    ;//
        test   eax, eax             ;// test      eax, eax
        je     _move_0              ;// je        _move_0
        cmp    eax, 1               ;// cmp       eax, 1
        je     _move_1              ;// je        _move_1
                                    ;//
        cmpsw                       ;// cmps      word ptr [rsi], word ptr [rdi]
        jnz     _ret_false          ;// jnz       _ret_false
        cmp    eax, 2               ;// cmp       eax, 2
        je     _move_0              ;// je        _move_0
                                    ;//
_move_1:                            ;//
        cmpsb                       ;// cmps      byte ptr [rsi], byte ptr [rdi]
        jnz     _ret_false          ;// jnz       _ret_false
                                    ;//
_move_0:                            ;//
        mov    result, 1            ;// mov       byte ptr [result], 1
                                    ;//
_ret_false:                         ;//
        pop    esi                  ;// pop      rsi
        pop    edi                  ;// pop      rdi

        X64_End();
    }
#endif
    return result;
} */
//------------------------------------------------------------------------------------------------------------
static uint64 _fcall getTEB64(void)                   // TODO: NTDLL::NtCurrentTeb (Optional)
{
 reg64 _reg {0};
 X64_Start()        
 REX_R  mov  _reg.dw[0], esp   // mov _reg.dw[0], r12  // R12 register should always contain pointer to TEB64 in WoW64 processes
 X64_End()          
 return _reg.v;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> static uint64 _fcall GetModuleHandle64(T lpModuleName, size_t* DllSize=nullptr)   // TODO: ntdll::LdrGetDllHandleEx
{
 NT64::TEB teb64;
 NT64::PEB peb64;
 NT64::PEB_LDR_DATA ldr;
 NT64::LDR_DATA_TABLE_ENTRY_LO head;
 wchar_t tempBuf[600];

 uint64 LastEntry = 0;
 uint64 pTeb64    = getTEB64();
 if(pTeb64 < MaxAddrX32)MOPR::MemCopy(&teb64, (vptr)pTeb64, sizeof(NT64::TEB));   // Always in upper memory?
   else getMem64(&teb64, pTeb64, sizeof(NT64::TEB));
 uint64 pPeb64 = teb64.ProcessEnvironmentBlock;
 if(pPeb64 < MaxAddrX32)MOPR::MemCopy(&peb64, (vptr)pPeb64, sizeof(NT64::PEB));
   else getMem64(&peb64, pPeb64, sizeof(NT64::PEB));
 uint64 pLdr64 = peb64.Ldr;
 if(pLdr64 < MaxAddrX32)MOPR::MemCopy(&ldr, (vptr)pLdr64, sizeof(NT64::PEB_LDR_DATA));
   else getMem64(&ldr, pLdr64, sizeof(NT64::PEB_LDR_DATA));

 head.InLoadOrderLinks.Flink = ldr.InLoadOrderModuleList.Flink;
 do
  {
   uint64 pHead64 = head.InLoadOrderLinks.Flink;
   if(pHead64 < MaxAddrX32)MOPR::MemCopy(&head, (vptr)pHead64, sizeof(NT64::LDR_DATA_TABLE_ENTRY));
     else getMem64(&head, pHead64, sizeof(NT64::LDR_DATA_TABLE_ENTRY));
   wchar_t* Name  = tempBuf;
   uint64 pName64 = head.BaseDllName.Buffer;
   if(pName64 < MaxAddrX32)Name = (wchar_t*)pName64;
     else getMem64(tempBuf, pName64, head.BaseDllName.MaximumLength);
   if(NSTR::IsStrEqASCII_IC(lpModuleName, Name))
    {
     if(DllSize)*DllSize = head.SizeOfImage;
     return head.DllBase;
    }
  }
   while(head.InLoadOrderLinks.Flink != LastEntry);     // head.InLoadOrderLinks.Flink
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static uint64 _fcall getNTDLL64(void)
{
 static uint64 ntdll64 = 0;
 if(ntdll64)return ntdll64;
 ntdll64 = GetModuleHandle64("ntdll.dll");
 return ntdll64;
}
//------------------------------------------------------------------------------------------------------------
static uint64 _fcall GetProcAddressSimpleX64(uint64 modBase, const achar* ProcName)
{
 if(!modBase || !ProcName || !*ProcName)return 0;
 NPE64::SDosHdr idh;
 NPE64::SWinHdr inh;
 NPE64::SExpDir ied;
 achar  PNameBuf[128];     // Enough for ntdll.dll
 uint32 NameTable[512];    // NOTE: Disable chkstk

 getMem64(&idh, modBase, sizeof(idh));
 getMem64(&inh, modBase + idh.OffsetHeaderPE, sizeof(inh));
 NPE64::SDataDir& idd = inh.OptionalHeader.DataDirectories.ExportTable;
 if(!idd.DirectoryRVA)return 0;
 getMem64(&ied, modBase + idd.DirectoryRVA, sizeof(ied));
 //DBGMSG("ModBase=%016llX, AddressOfNames=%08X",modBase, ied.NamePointersRVA);        // No logging allowed: May be not initialized yet

 int PNameLen = 0;
 while(ProcName[PNameLen])PNameLen++;
 uint32 MaxNamesInBuf = sizeof(NameTable) / sizeof(uint32);
 for(uint32 i=0,ctr=MaxNamesInBuf; i < ied.FunctionsNumber; i++, ctr++)   // lazy search, there is no need to use binsearch for just one function
  {
   if(ctr >= MaxNamesInBuf)
    {
     uint32 RecsLeft = ied.NamePointersNumber - i;
     if(RecsLeft > MaxNamesInBuf)RecsLeft = MaxNamesInBuf;
     getMem64(&NameTable, modBase + ied.NamePointersRVA + (i * sizeof(uint32)), sizeof(uint32)*RecsLeft);
     ctr = 0;
    }
//   DBGMSG("NameTable %p: %08X",&NameTable, NameTable[ctr]);
   getMem64(PNameBuf, modBase + NameTable[ctr], PNameLen + 1);  // Including 0
   int NLen = 0;
   while((PNameBuf[NLen] == ProcName[NLen]) && PNameBuf[NLen])NLen++;
   if(NLen != PNameLen)continue;        // Not match
   uint16 OrdTblVal;
   uint32 RvaTblVal;
   getMem64(&OrdTblVal, modBase + ied.OrdinalTableRVA + (sizeof(uint16) * i), sizeof(uint16));
   getMem64(&RvaTblVal, modBase + ied.AddressTableRVA + (sizeof(uint32) * OrdTblVal), sizeof(uint32));
//   DBGMSG("Found: %016llX",(modBase + RvaTblVal));
   return modBase + RvaTblVal;
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static uint64 _fcall GetProcAddress64(uint64 hModule, const achar* funcName)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddressSimpleX64(getNTDLL64(), "LdrGetProcedureAddress"); if(!xproc)return 0; }
 NT64::ANSI_STRING fName;
 fName.Buffer = (uint64)funcName;
 fName.Length = (uint16)NSTR::StrLen(funcName);
 fName.MaximumLength = fName.Length + 1;
 uint64 funcRet = 0;
 X64Call(xproc, 4, (uint64)hModule, (uint64)&fName, (uint64)0, (uint64)&funcRet);
 return funcRet;
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall QuerySystemInformation(NT::SYSTEM_INFORMATION_CLASS SystemInformationClass, vptr SystemInformation, uint32 SystemInformationLength, uint32* ReturnLength)    // Is ReturnLength actually uint32 on x54?
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtQuerySystemInformation"); if(!xproc)return -1; }
 return X64Call(xproc, 4, (uint64)SystemInformationClass, (uint64)SystemInformation, (uint64)SystemInformationLength, (uint64)ReturnLength);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall QueryInformationThread(NT64::HANDLE ThreadHandle, NT::THREADINFOCLASS ThreadInformationClass, vptr ThreadInformation, uint32 ThreadInformationLength, uint32* ReturnLength)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtQueryInformationThread"); if(!xproc)return -1; }
 return X64Call(xproc, 5, (uint64)ThreadHandle, (uint64)ThreadInformationClass, (uint64)ThreadInformation, (uint64)ThreadInformationLength, (uint64)ReturnLength);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall SetInformationThread(NT64::HANDLE ThreadHandle, NT::THREADINFOCLASS ThreadInformationClass, vptr ThreadInformation, uint32 ThreadInformationLength)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtSetInformationThread"); if(!xproc)return -1; }
 return X64Call(xproc, 4, (uint64)ThreadHandle, (uint64)ThreadInformationClass, (uint64)ThreadInformation, (uint64)ThreadInformationLength);
}
//------------------------------------------------------------------------------------------------------------
// Can`t read anything if the range includes a ME_RESERVE segment. (.NET modules have this)
static NT::NTSTATUS _fcall ReadVirtualMemory(NT64::HANDLE ProcessHandle, uint64 BaseAddress, vptr Buffer, uint32 BufferSize, uint64* NumberOfBytesRead)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtReadVirtualMemory"); if(!xproc)return -1; }
 return X64Call(xproc, 5, (uint64)ProcessHandle, (uint64)BaseAddress, (uint64)Buffer, (uint64)BufferSize, (uint64)NumberOfBytesRead);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall WriteVirtualMemory(NT64::HANDLE ProcessHandle, uint64 BaseAddress, vptr Buffer, uint32 BufferSize, uint64* NumberOfBytesWritten)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtWriteVirtualMemory"); if(!xproc)return -1; }
 return X64Call(xproc, 5, (uint64)ProcessHandle, (uint64)BaseAddress, (uint64)Buffer, (uint64)BufferSize, (uint64)NumberOfBytesWritten);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall GetContextThread(NT64::HANDLE ThreadHandle, NT::CONTEXT64* ThreadContext)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtGetContextThread"); if(!xproc)return -1; }
 return X64Call(xproc, 2, (uint64)ThreadHandle, (uint64)ThreadContext);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall SetContextThread(NT64::HANDLE ThreadHandle, NT::CONTEXT64* ThreadContext)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtSetContextThread"); if(!xproc)return -1; }
 return X64Call(xproc, 2, (uint64)ThreadHandle, (uint64)ThreadContext);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall UnmapViewOfSection(NT64::HANDLE ProcessHandle, uint64 BaseAddress)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtUnmapViewOfSection"); if(!xproc)return -1; }
 return X64Call(xproc, 2, (uint64)ProcessHandle, (uint64)BaseAddress);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall MapViewOfSection(NT64::HANDLE SectionHandle, NT::HANDLE ProcessHandle, uint64* BaseAddress, uint64 ZeroBits, uint64 CommitSize, NT::LARGE_INTEGER* SectionOffset, uint64* ViewSize, NT::SECTION_INHERIT InheritDisposition, uint32 AllocationType, uint32 Win32Protect)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtMapViewOfSection"); if(!xproc)return -1; }
 return X64Call(xproc, 10, (uint64)SectionHandle, (uint64)ProcessHandle, (uint64)BaseAddress, (uint64)ZeroBits, (uint64)CommitSize, (uint64)SectionOffset, (uint64)ViewSize, (uint64)InheritDisposition, (uint64)AllocationType, (uint64)Win32Protect);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall ProtectVirtualMemory(NT64::HANDLE ProcessHandle, uint64* BaseAddress, uint64* RegionSize, uint32 NewProtect, uint32* OldProtect)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtProtectVirtualMemory"); if(!xproc)return -1; }
 return X64Call(xproc, 5, (uint64)ProcessHandle, (uint64)BaseAddress, (uint64)RegionSize, (uint64)NewProtect, (uint64)OldProtect);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall QueryVirtualMemory(NT64::HANDLE ProcessHandle, uint64 BaseAddress, NT::MEMORY_INFORMATION_CLASS MemoryInformationClass, vptr MemoryInformation, uint32 MemoryInformationLength, uint64* ReturnLength)
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "NtQueryVirtualMemory"); if(!xproc)return -1; }
 return X64Call(xproc, 6, (uint64)ProcessHandle, (uint64)BaseAddress, (uint64)MemoryInformationClass, (uint64)MemoryInformation, (uint64)MemoryInformationLength, (uint64)ReturnLength);
}
//------------------------------------------------------------------------------------------------------------
static NT::NTSTATUS _fcall memcpy(uint64 Dst, uint64 Src, uint64 Size)       // Useless?
{
 static uint64 xproc = 0;
 if(!xproc){ xproc = GetProcAddress64(getNTDLL64(), "memcpy"); if(!xproc)return -1; }
 return X64Call(xproc, 3, (uint64)Dst, (uint64)Src, (uint64)Size);
}
//------------------------------------------------------------------------------------------------------------

};
