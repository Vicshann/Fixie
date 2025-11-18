
#pragma once

//---------------------------------------------------------------------------
// NOTE: Not using uint8 overflow to avoid any possible undefined behaviour
// OpenSSL key sizes:
//    rc4                128 bit RC4
//    rc4-64             64  bit RC4
//    rc4-40             40  bit RC4
//
static constexpr void EncDecMsgRC4(uint8* MsgSrc, uint8* MsgDst, uint32 MsgLen, uint8* Key, uint32 KeyLen)  //__attribute__ ((optnone))  // RC4   // Can be constexpr?
{
 SCVR uint32 MAXIDX = 256;   // Max RC4 key length
 uint8 IndexArr[MAXIDX];    

 for(uint32 ctr=0;ctr < MAXIDX;ctr++)IndexArr[ctr] = uint8(ctr);  // Init RC4 SBOX
 for(uint32 KBIdx=0,BIdx=0,IndexA=0;IndexA < MAXIDX;KBIdx++,IndexA++)  // Reorganize array
  {
   if(KBIdx >= KeyLen)KBIdx = 0;      // Loop the key
   uint32 valb = IndexArr[IndexA];
   BIdx = uint8(valb + BIdx + Key[KBIdx]);  // % KeyLen 
   IndexArr[IndexA] = IndexArr[BIdx];       // SWAP
   IndexArr[BIdx]   = uint8(valb);
  }
 for(uint32 CIdxA=0,CIdxB=0,IndexA=0;IndexA < MsgLen;IndexA++)
  {
   CIdxA = uint8(++CIdxA);  
   uint32 valb = IndexArr[CIdxA];
   CIdxB = uint8(CIdxB + valb);  
   uint32 vala = IndexArr[CIdxB];
   IndexArr[CIdxA] = uint8(vala);
   IndexArr[CIdxB] = uint8(valb);
   MsgDst[IndexA]  = MsgSrc[IndexA] ^ IndexArr[uint8(vala + valb)];  
  }
}
//---------------------------------------------------------------------------



