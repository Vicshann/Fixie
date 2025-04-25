
#if defined(_OBFUSCATE)
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
    ".section " #sect ",\"wx\" \n"                  /* Change section */\
  /*  ".balign 16\n"                            Word alignment */\
 /*   ".global " #sym "\n"                     Export the object address */\
    #sym ":\n"                              /* Define the object label */\
    ".incbin \"" file "\"\n"                /* Import the file */\
   /* ".global _sizeof_" #sym "\n"             Export the object size */\
   /*".set _sizeof_" #sym ", . - " #sym "\n"   Define the object size (. is relative to the section)*/\
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
__attribute__ ((section (".exebeg"))) volatile const char ___MODULEBEG alignas(1)[0] = {};
__attribute__ ((section (".rodend"))) volatile const char ___RODATAEND alignas(1)[0] = {};
__attribute__ ((section (".plbeg"))) volatile const char ___PAYLOADBEG alignas(1)[0] = {};   // NOTE: Such pointers are 'tainted' and optimizer will go crazy if they are used directly
IMPORT_BIN(".payload", "FILES/payload.bin", EDataBlk);                                       // Can control section attributes too
// Unreachable by ADR with big payloads
__attribute__ ((section (".plend"))) volatile const char ___PAYLOADEND alignas(1)[0] = {};   // ((size_t)&__LNK_BINEND - _sizeof_EDataBlk) gives actual base address of the data block
__attribute__ ((section (".exeend"))) volatile const char ___MODULEEND alignas(1)[0] = {};   // Byte-aligned to put it right after any data  // Must be referenced or will be optimized away
//============================================================================================================
// uint8* exebeg = (uint8*)UnbindPtr(&___MODULEBEG);
// uint8* exeend = (uint8*)UnbindPtr(&___RODATAEND);

// uint8* pldbeg = (uint8*)UnbindPtr(&___PAYLOADBEG);   // Should be 16 byte aligned!
// uint8* pldend = (uint8*)UnbindPtr(&___PAYLOADEND);