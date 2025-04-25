
//============================================================================================================
class SHookImpl
{
 SCVR uint StubLen = 64;

enum EHTrTypeSize
{
 etlJmpRel8  = 0x0100 | 5,      // And the jmp [Addr] above it if enough space  (Not rehookable!)
 etlJmpRel32 = 0x0200 | 5,      // E9 XX XX XX XX   // Jmp REL32   (+/- 2GB)
 etlRopRet32 = 0x0300 | 6,      // 68 ?? ?? ?? ??   push XXXXXXXX  // C3   retn      // If the target addr is in x32 area                    // May be blocked by ShadowStack
 etlJmpIpU64 = 0x0400 | 6,      // FF 25 F2 FF FF FF   jmp [rip-14] (Addr is above it, in a padding area)   (If available)   // 6 + 8 above it    (Not rehookable!)
 etlJmpIpD64 = 0x0500 | 14,     // FF 25 00 00 00 00   jmp [rip] (Addr is below it)
 etlJmpRax64 = 0x0600 | 12,     // 48 B8 11 22 33 00 FF FF FF 0F   movabs rax,FFFFFFF00332211  // FF E0   jmp rax     // Longest   // Spills the RAX (Bad for Linux)
};

public:
enum EHookFlg {hfNone,hfFollowJmp=1,hfForceHook=2,hfAllowROP=4}; 

public:
// uint32  exit[4];     // Far stub, actually  // Stored here to check if it is hooked?
// uint32  backup[4];   // align 16     // uint8_t   backup[16];
// uint32  backup_len;  // == 4 or 16
// uint32  exit_type;   // Stores prev fill byte of the gap and its allocation type (TODO)
// vptr    exit_addr;   // Probably points to some hole nearby or into allocated page  (Must be in an Executable memory, nearby to target proc start addr)
 uint8* enter_addr;  // Points to a stub with instructions taken(and possibly recompiled) from a target proc start addr (Must be in an Executable memory)
 uint8* OrigPtr;
/// usize OrigLen;   // always 16
 alignas(16) uint8 OrigCode[16];  // Original code to restore when hook is removed    // Size of __m128i 
 alignas(16) uint8 Trampo[16]; // uint32  trampo[4];   // align 16 // length == backup_len    // Used on unhook to check that we still own this hook
// uint8 StolenCode[64];  // A modified stolen code (Executable buffer)

template<typename T> _finline static ssize AddrToRelAddr(T CmdAddr, uint32 CmdLen, T TgtAddr){return -((CmdAddr + CmdLen) - TgtAddr);}        // x86 only?
template<typename T> _finline static T     RelAddrToAddr(T CmdAddr, uint32 CmdLen, ssize TgtOffset){return ((CmdAddr + CmdLen) + TgtOffset);}  // x86 only?
//------------------------------------------------------------------------------------------------------------
// On stack the is addr of a variable that contains the target addr. Read the variable and store its value on stack, instead of its addr.
template<typename T=usize> static usize GenAddrDeref(uint8* DstBuf, usize Offs)
{
 if constexpr (sizeof(T) >= sizeof(uint64))  
  {
   *(uint64*)&DstBuf[Offs += 8] = 0x008B48F824446748;  //  xchg qword ptr [rsp-8], rax;    mov rax, qword ptr [rax]    // Dereferences the pointer
   *(uint32*)&DstBuf[Offs += 4] = 0x24448748;          //  xchg qword ptr [rsp-8], rax    // Partial
   *(uint8*)&DstBuf[Offs++] = 0xF8;
  }
 else
  {
   *(uint64*)&DstBuf[Offs += 8] = 0x4487008BFC244487;  //  xchg dword ptr [esp-4], eax;    mov eax, dword ptr [eax]   
   *(uint16*)&DstBuf[Offs += 2] = 0xFC24;              //  xchg dword ptr [esp-4], eax;   // Final part
  }
 return Offs;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Deref helps to avoid grabbing the REL referenced data that someone can change from outside
//
template<typename T=usize> static usize GenAddrStore(T Addr, uint8* DstBuf, usize Offs)
{
 if constexpr (sizeof(T) >= sizeof(uint64))    
  {
   *(uint32*)&DstBuf[Offs += 4] = 0xF82444C7;	// mov [RSP-8], DWORD
   *(uint32*)&DstBuf[Offs += 4] = uint32((usize)Addr);                       // ((PDWORD)&Addr)[0];
   *(uint32*)&DstBuf[Offs += 4] = 0xFC2444C7;	// mov [RSP-4], DWORD
   *(uint32*)&DstBuf[Offs += 4] = uint32((usize)Addr >> 32);                 // ((PDWORD)&Addr)[1];
  }
 else
  {
   *(uint32*)&DstBuf[Offs += 4] = 0xFC2444C7;	// mov [RSP-4], DWORD
   *(uint32*)&DstBuf[Offs += 4] = usize(Addr); 
  }
 return Offs;
}
//------------------------------------------------------------------------------------------------------------
// No ROP - (Shadow Stack may be used); CFG is usually disabled
template<bool Deref=false, typename T=usize> _finline static usize GenJumpTransfer(T Addr, uint8* DstBuf, usize Offs)     // Deref is needed for 'JMP [REL]'
{
 Offs = GenAddrStore<T>(Addr, DstBuf, Offs);
 if constexpr (Deref)Offs = GenAddrDeref(DstBuf, Offs);
 if constexpr (sizeof(T) >= sizeof(uint64))*(uint32*)&DstBuf[Offs += 4] = 0xF82464FF;    // jmp [RSP-8] 
   else *(uint32*)&DstBuf[Offs += 4] = 0xFC2464FF;    // jmp [RSP-4]    
 return Offs;
}
//------------------------------------------------------------------------------------------------------------
template<bool Deref=false, typename T=usize> _finline static usize GenCallTransfer(T Addr, uint8* DstBuf, usize Offs) 
{
 Offs = GenAddrStore<T>(Addr, DstBuf, Offs);
 if constexpr (Deref)Offs = GenAddrDeref(DstBuf, Offs);
 if constexpr (sizeof(T) >= sizeof(uint64))*(uint32*)&DstBuf[Offs += 4] = 0xF82454FF;    // call [RSP-8]  
   else *(uint32*)&DstBuf[Offs += 4] = 0xFC2454FF;     // call [RSP-4]   
 return Offs;
}
//------------------------------------------------------------------------------------------------------------
// mov REG, [REL]  // Converted addr for the REL is on stack    // Other parts of the register must be preserved!
/*template<typename T=usize> _finline static usize GenRegLoad(T Addr, uint8* DstBuf, usize Offs)    // Supports X32 registers
{
 Offs = GenAddrStore<T>(Addr, DstBuf, Offs);
 Offs = GenAddrDeref(DstBuf, Offs);
 // mov REG, [RSP-8]
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// mov [REL], REG    // What if this REG is RAX?
template<typename T=usize> _finline static usize GenRegStore(T Addr, uint8* DstBuf, usize Offs)   // Supports X32 registers
{
// mov [REG2], REG1; XCHG REG2, [RSP-8]
 return 0;
} */
//------------------------------------------------------------------------------------------------------------
static usize CountPadding(uint8* Addr, int Step=1, uint MaxDist=uint(-1))   // Step +/- 1
{
 uint8  Val = *Addr;
 if(!Val)return 0;  // 0 is usually belongs to some data
 uint8* Ptr = Addr;
 do {Ptr += Step; MaxDist--;} while((*Ptr == Val) && MaxDist);
 usize  Len = Ptr - Addr;
 if(Len < 4)
  {
   if((Val != 0x90)&&(Val != 0xCC))return 0;  // Too small to allow as unknown padding
  }
 return Len;
}
//------------------------------------------------------------------------------------------------------------
static sint BuildDisplacedStub(auto dhde, uint8* DstPtr, usize DstLen, usize MaxToTake, uint8** TargetAddr, usize* RebuiltLen, uint32 Flags)
{
 uint8* JmpDst = nullptr;   // Largest found JmpDest
 uint8* BaseAddr = *TargetAddr;
 sint   BytesTaken = 0;      // this->HookLen 
 uint32 CodeLen = 0;
 uint32 InstrIdx = 0;
 bool   HaveMoreIns = true;   // Until an unconditional jump or RET is encountered (Will use any padding after it too)
 for(uint8* SrcPtr=BaseAddr;(BytesTaken < MaxToTake) && HaveMoreIns;InstrIdx++,SrcPtr += dhde.len)   // MSVC compiler crash if 'BytesTaken += dhde.len' is here
  {
   BytesTaken += dhde.Disasm(SrcPtr);
   // TODO: Check errors in 'flags'
   if((*SrcPtr & 0xFE) == 0xC2)   // RET (C2 or C3)
    { 
     if(SrcPtr >= JmpDst){HaveMoreIns = false; BytesTaken += CountPadding(&SrcPtr[BytesTaken]);} // Complete the function if not in a branch.
     continue;
    }
   if(*SrcPtr == 0xE8)  // Direct relative CALL
    {     
     uint8* DstAddr = RelAddrToAddr(SrcPtr,dhde.len,*(uint32*)&SrcPtr[1]);    // dhde.imm.imm32;
     CodeLen = GenCallTransfer(DstAddr, DstPtr, CodeLen);
     continue;
    }
   if((*SrcPtr & 0xFD) == 0xE9)   // Direct relative JMP (EB or E9)  
    {
     uint8* DstAddr = (*SrcPtr == 0xE9)?RelAddrToAddr(SrcPtr,dhde.len,*(uint32*)&SrcPtr[1]):RelAddrToAddr(SrcPtr,dhde.len,*(uint8*)&SrcPtr[1]);
     if(!InstrIdx && (Flags & EHookFlg::hfFollowJmp))   // Follow only if the jump is a first instruction
      {
       JmpDst = nullptr;
       BaseAddr = SrcPtr = DstAddr;
       CodeLen = BytesTaken = dhde.len = 0;
      }
     else 
      {
       if((DstAddr >= BaseAddr) && (DstAddr < &BaseAddr[MaxToTake]))   // UNSAFE! // An unconditional jump means that a next instruction is likely a someone's jump target (Probably from outside of the block)
        {
         if(JmpDst < DstAddr)JmpDst = DstAddr; 
         // TODO: Remember it to update its offset after all rewrites are done
        }
       else if(SrcPtr >= JmpDst)   // Not in a branch - stop disassembling!  // May be safe to use padding bytes between functions
        {
         HaveMoreIns = false; 
         BytesTaken += CountPadding(&SrcPtr[BytesTaken]);
        } 
       else  // Rewrite the jump for a direct address
        {
         CodeLen = GenJumpTransfer(DstAddr, DstPtr, CodeLen);
        }
      }
     continue;
    }
   if(((dhde.opcode & 0xF0) == 0x70) || ((dhde.opcode & 0xFC) == 0xE0) || ((dhde.opcode2 & 0xF0) == 0x80))   // Direct relative Jcc
    {     
     uint8* DstAddr = ((dhde.opcode2 & 0xF0) == 0x80)?RelAddrToAddr(SrcPtr,dhde.len,(uint32)dhde.imm.imm32):RelAddrToAddr(SrcPtr,dhde.len,(uint8)dhde.imm.imm8);  // imm8:  Jcc   LOOPNZ/LOOPZ/LOOP/JECXZ    
     if((DstAddr >= BaseAddr) && (DstAddr < &BaseAddr[MaxToTake]))  // The conditional jump is inside the displaced code
      {
       if(JmpDst < DstAddr)JmpDst = DstAddr; 
       // TODO: Remember it to update its offset after all rewrites are done
      }
     else if ((dhde.opcode & 0xFC) == 0xE0)   // LOOPNZ/LOOPZ/LOOP/JCXZ/JECXZ to the outside are not supported.
      {      
       BytesTaken -= dhde.len;    
       break;
      }
     else
      {
       uint8 cond = ((dhde.opcode != 0x0F ? dhde.opcode : dhde.opcode2) & 0x0F);
       uint8* CurAddr = &DstPtr[CodeLen];
       DstPtr[CodeLen++] = 0x71 ^ cond;
       DstPtr[CodeLen++] = 0;   // REL is unknown yet
      // LOGMSG("jmp at %p to %p, disp32 = %08X, imm32 = %08X",SrcPtr,Addr,dhde.disp.disp32,dhde.imm.imm32);
       CodeLen = GenJumpTransfer(DstAddr, DstPtr, CodeLen);
       CurAddr[1] = AddrToRelAddr(CurAddr,2,&DstPtr[CodeLen]);
      }
     continue;
    }
   if(IsArchX64 && ((dhde.modrm & 0xC7) == 0x05))  // Instructions using RIP relative addressing. (ModR/M = 00???101B) // Modify the RIP relative address.
    {
     uint8* PAddr = RelAddrToAddr(SrcPtr,dhde.len,(uint32)dhde.disp.disp32);
     // Relative address is stored at (instruction length - immediate value length - 4).
     // pRelAddr = (PUINT32)(instBuf + dhde.len - ((dhde.flags & 0x3C) >> 2) - 4);    // Why?  // Should not be any imm with disp32
     if((dhde.opcode == 0xFF) && ((dhde.modrm_reg == 4)||(dhde.modrm_reg == 2)))   // jmp [REL] or call [REL]     // NOTE: If the addr in the range then it is not updated to rewritten instruction offsets
      {
       if(dhde.modrm_reg == 4)    // jmp [REL]  // Should not continue after this
        {
         bool Hooked = (PAddr == &SrcPtr[-8]); 
         if(!InstrIdx && !Hooked && (Flags & EHookFlg::hfFollowJmp))   // Follow only if the jump is a first instruction
          {
           JmpDst = nullptr;
           BaseAddr = SrcPtr = *(uint8**)PAddr;
           CodeLen = BytesTaken = dhde.len = 0;
          }
           else
            {
             if(Hooked)CodeLen = GenJumpTransfer<false>(*(uint8**)PAddr, DstPtr, CodeLen);
              else CodeLen = GenJumpTransfer<true>(PAddr, DstPtr, CodeLen);
             HaveMoreIns = false; 
             BytesTaken += CountPadding(&SrcPtr[BytesTaken]);
            }
        }
         else CodeLen = GenCallTransfer<true>(PAddr, DstPtr, CodeLen);
      }
     if((dhde.opcode == 0xFF) && (dhde.modrm_reg == 6))    // push [REL] 
      {
        // TODO
      }
     if((dhde.opcode == 0x8F) && (dhde.modrm_reg == 0))    // pop [REL]
      {
        // TODO
      }
     else       // Now we need: mov [ESP-8], REG movabs REG, ADDR; rewrite the instruction to use [REG] instead of [REL]   // NOTE: Breaks if the instruction modifies RSP
      {                                                                         
       uint8 RegMod = (dhde.modrm_reg == 0);   // eax is our reg
       DstPtr[CodeLen++] = 0x50 | RegMod;  // Push RAX
       if constexpr(IsArchX64)DstPtr[CodeLen++] = 0x48;                    // constexpr?
       *(uint32*)&DstPtr[CodeLen += 4] = uint32((usize)PAddr);
       if constexpr(IsArchX64)*(uint32*)&DstPtr[CodeLen += 4] = uint32((usize)PAddr >> 32);    // High part
       __movsb(&DstPtr[CodeLen], SrcPtr, dhde.len - 4);      // Copy the original instruction but without disp32
       CodeLen += dhde.len - 4;
       uint8 val = DstPtr[CodeLen-1];
       DstPtr[CodeLen-1] = (val & 0xF8) | RegMod;  // Make it [RAX] instead of [REL32]
       DstPtr[CodeLen++] = 0x58 | RegMod;  // Pop RAX
      }
     continue;
    }

   if((CodeLen + dhde.len) >= DstLen)return -1;   
   memcpy(&DstPtr[CodeLen],SrcPtr,dhde.len);   // Copy current instruction as is
   CodeLen += dhde.len;
  }
 *RebuiltLen = CodeLen;
 *TargetAddr = BaseAddr;
 return BytesTaken;
}
//------------------------------------------------------------------------------------------------------------
bool IsActiveAt(vptr TargetAddr)
{
 int r = memcmp(TargetAddr, this->Trampo, sizeof(this->Trampo));    // TODO: Try-catch or validate the addr somehow
 return 0 == r;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Rel32 may happen to be out of +/-2GB range on X32 if 3GB address space is enabled (Fix it)
// NOTE: Address of FarJump must be in range of +/- 2GB of TargetAddr. Each entry is 16 bytes
// TODO: Made it able to hook already hooked functions
//
int InstallHook(vptr TargetAddr, vptr HookFunc, vptr FarJump=nullptr, uint32 Flags=(IsArchX64)?hfNone:EHookFlg::hfFollowJmp)   // Can be reused with same TargetAddr after 'Remove'  // Do not refer to 'T' from here or this function may be duplicated
{
 if(!TargetAddr || (this->IsActiveAt(TargetAddr) && !(Flags & EHookFlg::hfForceHook))){/*DBGMSG("Failed: %p",TargetAddr);*/ return -1;}       // Logging here will make the message duplicated by templating and size will bloat!
// DBGMSG("Hooking: %p",TargetAddr);

TSW<IsArchX64,NHDE::HDE64,NHDE::HDE32>::T dhde;
int HookTypeLen = IsArchX64?etlJmpIpD64:etlJmpRel32;
if(!FarJump)
 {
  uint64 JAddr = ((uint64)TargetAddr + 5);   // 5 is size of 'jmp rel32'
  if(((uint64)HookFunc <= (JAddr + 0x7FFFFFFF)) && ((uint64)HookFunc >= (JAddr - 0x80000000)))HookTypeLen = etlJmpRel32;     // In range of a simple Rel32 jump  (+/- 2Gb)
  else if(((uint64)HookFunc <= 0xFFFFFFFF) && (Flags & hfAllowROP))HookTypeLen = etlRopRet32;    // In x32 area
    else 
     {
      uint padd = CountPadding((uint8*)TargetAddr - 1, -1, 8);
      if(padd == 8)HookTypeLen = etlJmpIpU64; 
     }
 }	
  else HookTypeLen = etlJmpRel32;    // Allocated in +/-2GB range
		
 uint8* TgtAddr = (uint8*)TargetAddr;   
 usize  CodeLen = 0;

 if(*TgtAddr == 0xFC)TgtAddr++;   // CLD  // Just in case
 sint BytesTaken = BuildDisplacedStub(dhde, this->enter_addr, StubLen, (uint8)HookTypeLen, &TgtAddr, &CodeLen, Flags);
 if(BytesTaken <= 0)return -2;
 if(BytesTaken < sint((uint8)HookTypeLen))return -3;
 
 if constexpr (IsArchX64)
  {
   this->enter_addr[CodeLen]   = 0xFF;
   this->enter_addr[CodeLen+1] = 0x25;
   this->enter_addr[CodeLen+2] = this->enter_addr[CodeLen+3] = this->enter_addr[CodeLen+4] = this->enter_addr[CodeLen+5] = 0;  // RIP+0
   *((uint8**)&this->enter_addr[CodeLen+6]) = TgtAddr + BytesTaken;   // Aligned by HDE to whole command
   CodeLen += 14;
  }
   else
    {
     this->enter_addr[CodeLen]   = 0x68;   // push NNNNNNNN
     *((uint8**)&this->enter_addr[CodeLen+1]) = TgtAddr + BytesTaken;   // Aligned by HDE to whole command
     this->enter_addr[CodeLen+5] = 0x83;
     *((uint16*)&this->enter_addr[CodeLen+6]) = 0x04C4;      // Add ESP, 4
     *((uint32*)&this->enter_addr[CodeLen+8]) = 0xFC2464FF;  // jmp [ESP-4]  // No ROP  (ShadowStack should be OK)
     CodeLen += 12;
    }

 uint8* BaseAddress = TgtAddr;    
 usize  RegionSize  = BytesTaken;
 alignas(16) uint8 Patch[16];   // __m128i
 if(HookTypeLen == etlJmpIpU64)    // Extra addr above
  {
   BaseAddress -= 8;
   RegionSize  += 8;
  }
 // TODO: else etlJmpRel8
 this->OrigPtr = BaseAddress;     // To restore the orig code
 *(v128*)&Patch = *(v128*)BaseAddress;
 *(v128*)&this->OrigCode = *(v128*)&Patch;    // _mm_storeu_si128 ???

 uint POffs = TgtAddr - BaseAddress;
 if(HookTypeLen == etlJmpRax64)
  {
   Patch[POffs+0]  = 0x48;				       // movabs rax,FFFFFFF00332211   // EAX is unused in x64 calling convention  // TODO: Usesome SSE instructions to copy this by one operation
   Patch[POffs+1]  = 0xB8;
   *(vptr*)&Patch[POffs+2] = *(vptr*)&HookFunc;        // NOTE: This was modified to use a single assignment somewhere already!!!!!
   Patch[POffs+10] = 0xFF;                       // jmp EAX      // EAX is not preserved!  // Replace with QHalves method if EAX must be preserved  // No relative addresses here for easy overhooking
   Patch[POffs+11] = 0xE0;
  }
 else if(HookTypeLen == etlRopRet32)
  {
   Patch[POffs+0]  = 0x68;   // push
   *(uint32*)&Patch[POffs+1] = uint32((usize)HookFunc);   // Truncating
   Patch[POffs+5]  = 0xC3;   // ret
  }
 else if(HookTypeLen == etlJmpIpU64)
  {
   *(uint32*)&Patch[POffs+0] = 0xFFF225FF;    // jmp [rip-14]
   *(uint16*)&Patch[POffs+4] = 0xFFFF;
   *(vptr*)&Patch[POffs-sizeof(vptr)] = HookFunc;   // Have padding there?
  }
 else if(HookTypeLen == etlJmpRel32)    // In range or to FarJump stub (build it gere)
  {
   Patch[POffs+0] = 0xE9;
   if(FarJump)
    {
     *((uint32*)&Patch[POffs+1]) = AddrToRelAddr(&Patch[POffs],5,(uint8*)FarJump);
     uint8* Ptr = (uint8*)FarJump;     // Must be writable
     if(IsArchX64)
      {
       Ptr[0] = 0xFF;
       Ptr[1] = 0x25;
       Ptr[2] = Ptr[3] = Ptr[4] = Ptr[5] = 0;  // RIP+0
       *((vptr*)&Ptr[6]) = HookFunc; 
      }
       else
        {
         Ptr[0]   = 0x68;   // push NNNNNNNN
         *((vptr*)&Ptr[1]) = HookFunc;   // Aligned by HDE to whole command
         Ptr[5] = 0x83;
         *((uint16*)&Ptr[6]) = 0x04C4;      // Add ESP, 4
         *((uint32*)&Ptr[8]) = 0xFC2464FF;  // jmp [ESP-4]  // No ROP  (ShadowStack should be OK)
        }
    }
     else *((uint32*)&Patch[POffs+1]) = AddrToRelAddr(&Patch[POffs],5,(uint8*)HookFunc);
  }
    
 *(v128*)&this->Trampo = *(v128*)&Patch; //  _mm_storeu_si128((v128*)&this->Trampo, *(v128*)&Patch);  // Preserve our hook     // Should use SSE implicitly, when enabled
 //uint32 OldProtect = 0;	       // Not Windows!!!!!
 if(NPTM::MemProtect(BaseAddress, RegionSize, PX::PROT_READ | PX::PROT_WRITE | PX::PROT_EXEC))return -4;     // TODO: Find out if it is already writable (Then we can restore as RWX only if required)
 *(v128*)this->OrigPtr = *(v128*)&Patch; //  _mm_storeu_si128((v128*)this->OrigPtr, *(v128*)&Patch);  // SSE2, single operation, should be safe       // TODO: memcpy with aligned SSE2 16 byte copy
 // TODO: Find a way to restore old protection(Linux)
 return 0;
}  
//------------------------------------------------------------------------------------------------------------
int Remove(vptr TargetAddr)
{
 if(!this->IsActiveAt(TargetAddr))return -1;
 if(NPTM::MemProtect(TargetAddr, sizeof(this->OrigCode), PX::PROT_READ | PX::PROT_WRITE | PX::PROT_EXEC))return -2;     // TODO: Find out if it is already writable (Then we can restore as RWX only if required)
 *(v128*)TargetAddr = *(v128*)&this->OrigCode;  // _mm_storeu_si128((v128*)TargetAddr, *(v128*)&this->OrigCode);
 // TODO: Find a way to restore old protection(Linux)
 return 0;
}
//------------------------------------------------------------------------------------------------------------

};
//============================================================================================================
