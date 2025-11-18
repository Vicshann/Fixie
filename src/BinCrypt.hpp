
//============================================================================================================
#if defined(_OBFUSCATE)  // TODO: constexpr flag to switch the types? Then it can't be used by a compile script
#pragma message(">>> Obfuscation enabled!")
using ci8  = xbni8;
using cu8  = xbnu8;
using ci16 = xbni16;
using cu16 = xbnu16;
using ci32 = xbni32;
using cu32 = xbnu32;
using ci64 = xbni64;
using cu64 = xbnu64;
using cptr = xbnptr;     // Same as size_t
#else
#pragma message(">>> No obfuscation!")
using ci8  = int8;
using cu8  = uint8;
using ci16 = int16;
using cu16 = uint16;
using ci32 = int32;
using cu32 = uint32;
using ci64 = int64;
using cu64 = uint64;
using cptr = size_t;
#endif
   

// Let it be as it is while waiting for C++26 '#embed'
// NOTE: 'sym' is relative to the section and its value is always 0
// Creates RWX for some reason. How to remove W flag?
// And now it doesn`t. Why?
//
#define IMPORT_BIN(sect, file, sym) asm (\
    ".section " #sect ",\"wx\" \n"          /* Change section */\
  /*  ".balign 16\n"                           Word alignment */\
  /*  ".global " #sym "\n"                     Export the object address */\
    #sym ":\n"                              /* Define the object label */\
    ".incbin \"" file "\"\n"                /* Import the file */\
  /*  ".global _sizeof_" #sym "\n"             Export the object size */\
  /*  ".set _sizeof_" #sym ", . - " #sym "\n"  Define the object size (. is relative to the section)*/\
  /*  ".balign 16\n"                           Word alignment */\
    ".section \".text\"\n")                 /* Restore section */

/* Import a part of binary file */
#define IMPORT_BIN_PART(sect, file, ofs, siz, sym) asm (\
    ".section " #sect "\n"\
    ".balign 4\n"\
    ".global " #sym "\n"\
    #sym ":\n"\
    ".incbin \"" file "\"," #ofs "," #siz "\n"\
    ".global _sizeof_" #sym "\n"\
    ".set _sizeof_" #sym ", . - " #sym "\n"\
    ".balign 4\n"\
    ".section \".text\"\n")

//============================================================================================================
// http://elm-chan.org/junk/32bit/binclude.html
// NOTE: Such pointers are 'tainted' and optimizer will go crazy if they are used directly - use with UnbindPtr.
// NOTE: modbeg, modend, rodend is only valid with the linker script (Placement order is important) 
// modbeg - module begin (The app's code, entry point)
// modend - module end (a payload may follow)
// rodend - Read-only section end (code/ro_data)
// 
__attribute__ ((section (".modbeg"))) static volatile const char ___MODULEBEG  alignas(1)[0] = {};
__attribute__ ((section (".modend"))) static volatile const char ___MODULEEND  alignas(1)[0] = {};   // Byte-aligned to put it right after any data  // Must be referenced or will be optimized away
__attribute__ ((section (".rodend"))) static volatile const char ___RODATAEND  alignas(1)[0] = {};

#define PAYLOAD_BEG __attribute__ ((section (".plbeg")))  volatile const char NFWK::___PAYLOADBEG alignas(1)[0] = {};    

// EXAMPLE: IMPORT_BIN(".payload", "FILES/payload.bin", EDataBlk);                                       // Can control section attributes too

// NOTE ARM: Unreachable by ADR with big payloads
#define PAYLOAD_END __attribute__ ((section (".plend")))  volatile const char NFWK::___PAYLOADEND alignas(1)[0] = {};  // ((size_t)&__LNK_BINEND - _sizeof_EDataBlk) gives actual base address of the data block 

extern volatile const char ___PAYLOADBEG alignas(1)[0];
extern volatile const char ___PAYLOADEND alignas(1)[0];
//============================================================================================================
// The section must be RW(X)
// CODEHASH|DATEHASH|EXTRAHASH
// NOTE: Framework must be initialized for logging to work
//
static auto _finline DecryptPayload(uint8* exptr=nullptr, uint exlen=0)     // NOTE: Decrypts entire '.payload' inplace  // TODO: Split, non inplace variant
{
 struct result {uint8* ptr; uint len;};
 uint8 FinHash[NCRYPT::CSHA1::HashSize];
 uint8 ExeHash[NCRYPT::CSHA1::HashSize*3];

 uint8* exebeg = (uint8*)UnbindPtr(&___MODULEBEG);
 uint8* exeend = (uint8*)UnbindPtr(&___RODATAEND);

 uint8* pldbeg = (uint8*)UnbindPtr(&___PAYLOADBEG);   // Should be 16 byte aligned!
// uint8* pldend = (uint8*)UnbindPtr(&___PAYLOADEND);  // Why not used?
 cptr   pldlen = 0;

//   00 2E 74 65 78 74 00 2E  64 61 74 61 00 2E 62 73    .text .data .bs
//   73 00 2E 62 69 6E 00 2E  73 68 73 74 72 74 61 62   s .bin .shstrtab
//   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
//   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
 for(;;pldlen += 16)    // ????????????????   // TODO: Must work on Windows!!!
  {
   bool m1 = !pldbeg[pldlen];
   bool m2 = !*(uint64*)&pldbeg[pldlen+32];
   bool m3 = !*(uint64*)&pldbeg[pldlen+32+16];
   if((m1&m2&m3))break;
  }

 cptr hdlen = NCRYPT::CSHA1::HashSize*2;

 DBGMSG("exebeg Size=%08X, data: %*D", (exeend-exebeg), 32, (vptr)exebeg);
 DBGMSG("pldbeg Size=%08X, data: %*D", pldlen, 32, (vptr)pldbeg);

 constexpr const auto DateHash = NCRYPT::ctSHA1(__DATE__);  // Example: 'Apr 29 2025'  // UTC? // Must be in sync with the build script
 NCRYPT::Digest_SHA1(ExeHash, (const uint8*)exebeg, (exeend-exebeg));
 DBGMSG("DateHash : %*D", 20, &DateHash);
 DBGMSG("ExeHash  : %*D", 20, &ExeHash);
 memcpy(&ExeHash[NCRYPT::CSHA1::HashSize], &DateHash, NCRYPT::CSHA1::HashSize);
 if(exptr && exlen){hdlen += NCRYPT::CSHA1::HashSize; NCRYPT::Digest_SHA1(&ExeHash[NCRYPT::CSHA1::HashSize*2], exptr, exlen); }
 NCRYPT::Digest_SHA1(FinHash, (const uint8*)&ExeHash, hdlen);
 DBGMSG("FullHash : %*D", hdlen, &ExeHash);
 DBGMSG("FinHash  : %*D", 20, &FinHash);
 DBGMSG("Encrypted: %*D", 32, (vptr)pldbeg);
 NCRYPT::EncDecMsgRC4(pldbeg, pldbeg, pldlen, &FinHash[2], 16);  // Have to use 128 bit key for compatibility with OpenSSL command line
 DBGMSG("Decrypted: %*D", 32, (vptr)pldbeg);
 NPTM::NAPI::cache_flush(pldbeg, &pldbeg[pldlen]);
 return result {pldbeg, pldlen};
}
//============================================================================================================
