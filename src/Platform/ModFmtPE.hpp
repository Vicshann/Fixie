
#pragma once

// https://github.com/libyal/libexe/blob/main/documentation/Executable%20(EXE)%20file%20format.asciidoc
//============================================================================================================
template<typename PHT, bool RawPE=false> struct NFMTPE
{ 
static constexpr uint32 SIGN_MZ = 0x5A4D;
static constexpr uint32 SIGN_PE = 0x4550;
//------------------------------------------------------------------------------------------------------------
enum ESecFlags: uint32   // No easy binary ops with enum class - no enum class!
{
 TYPE_DSECT              =  0x00000001,  // Reserved.
 TYPE_NOLOAD             =  0x00000002,  // Reserved.
 TYPE_GROUP              =  0x00000004,  // Reserved.
 TYPE_NO_PAD             =  0x00000008,  // Reserved.
 TYPE_COPY               =  0x00000010,  // Reserved.
 CNT_CODE                =  0x00000020,  // Section contains code.
 CNT_INITIALIZED_DATA    =  0x00000040,  // Section contains initialized data.
 CNT_UNINITIALIZED_DATA  =  0x00000080,  // Section contains uninitialized data.
 LNK_OTHER               =  0x00000100,  // Reserved.
 LNK_INFO                =  0x00000200,  // Section contains comments or some other type of information.
 TYPE_OVER               =  0x00000400,  // Reserved.
 LNK_REMOVE              =  0x00000800,  // Section contents will not become part of image.
 LNK_COMDAT              =  0x00001000,  // Section contents comdat.
 UNKNOW                  =  0x00002000,  // Reserved.
 NO_DEFER_SPEC_EXC       =  0x00004000,  // Reset speculative exceptions handling bits in the TLB entries for this section.
 MEM_FARDATA             =  0x00008000,  // Section content can be accessed relative to GP
 MEM_SYSHEAP             =  0x00010000,  // Obsolete
 MEM_PURGEABLE           =  0x00020000,  //
 MEM_LOCKED              =  0x00040000,  //
 MEM_PRELOAD             =  0x00080000,  //
 ALIGN_1BYTES            =  0x00100000,  //
 ALIGN_2BYTES            =  0x00200000,  //
 ALIGN_4BYTES            =  0x00300000,  //
 ALIGN_8BYTES            =  0x00400000,  //
 ALIGN_16BYTES           =  0x00500000,  // Default alignment if no others are specified.
 ALIGN_32BYTES           =  0x00600000,  //
 ALIGN_64BYTES           =  0x00700000,  //
 ALIGN_128BYTES          =  0x00800000,  //
 ALIGN_256BYTES          =  0x00900000,  //
 ALIGN_512BYTES          =  0x00A00000,  //
 ALIGN_1024BYTES         =  0x00B00000,  //
 ALIGN_2048BYTES         =  0x00C00000,  //
 ALIGN_4096BYTES         =  0x00D00000,  //
 ALIGN_8192BYTES         =  0x00E00000,  //
 ALIGN_MASK              =  0x00F00000,  // UNUSED - Helps reading align value
 LNK_NRELOC_OVFL         =  0x01000000,  // Section contains extended relocations.
 MEM_DISCARDABLE         =  0x02000000,  // Section can be discarded.
 MEM_NOT_CACHED          =  0x04000000,  // Section is not cachable.
 MEM_NOT_PAGED           =  0x08000000,  // Section is not pageable.
 MEM_SHARED              =  0x10000000,  // Section is shareable.
 MEM_EXECUTE             =  0x20000000,  // Section is executable.
 MEM_READ                =  0x40000000,  // Section is readable.
 MEM_WRITE               =  0x80000000,  // Section is writeable.
};

enum EImgRelBased: uint32
{
 REL_ABSOLUTE       = 0,
 REL_HIGH           = 1,
 REL_LOW            = 2,
 REL_HIGHLOW        = 3,
 REL_HIGHADJ        = 4,
 REL_MIPS_JMPADDR   = 5,
 REL_MIPS_JMPADDR16 = 9,
 REL_IA64_IMM64     = 9,
 REL_DIR64          = 10,
};

enum EImgFlags: uint32
{
 IMAGE_FILE_RELOCS_STRIPPED         = 0x0001,  // Does not contain base relocations
 IMAGE_FILE_EXECUTABLE_IMAGE        = 0x0002,  // Is an executable (image file)
 IMAGE_FILE_LINE_NUMS_STRIPPED      = 0x0004,  // Line numbers have been removed
 IMAGE_FILE_LOCAL_SYMS_STRIPPED     = 0x0008,  // Symbol table entries for local symbols have been removed
 IMAGE_FILE_AGGRESSIVE_WS_TRIM      = 0x0010,  // Aggressively trim working set
 IMAGE_FILE_LARGE_ADDRESS_AWARE     = 0x0020,  // Application can handle > 2 GiB addresses
 IMAGE_FILE_16BIT_MACHINE           = 0x0040,  // Unknown (reserved for future use)
 IMAGE_FILE_BYTES_REVERSED_LO       = 0x0080,  // Little-endian
 IMAGE_FILE_32BIT_MACHINE           = 0x0100,  // 32-bit architecture
 IMAGE_FILE_DEBUG_STRIPPED          = 0x0200,  // Debugging information removed from file
 IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400,  // If the file is on removable media, copy and run from swap file
 IMAGE_FILE_SYSTEM                  = 0x1000,  // Is a system file, not a user program
 IMAGE_FILE_DLL                     = 0x2000,  // Is a dynamic-link library (DLL)
 IMAGE_FILE_UP_SYSTEM_ONLY          = 0x4000,  // File should be run only on a UP machine
 IMAGE_FILE_BYTES_REVERSED_HI       = 0x8000,  // Big-endian
};

enum EImgDllChars: uint32
{
 IMAGE_DLL_CHARACTERISTICS_RESERVED1             = 0x0001,  // Reserved.
 IMAGE_DLL_CHARACTERISTICS_RESERVED2             = 0x0002,  // Reserved.
 IMAGE_DLL_CHARACTERISTICS_RESERVED3             = 0x0004,  // Reserved.
 IMAGE_DLL_CHARACTERISTICS_RESERVED4             = 0x0008,  // Reserved.
 IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA       = 0x0020,  // ASLR with 64 bit address space.
 IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE          = 0x0040,  // The DLL can be relocated at load time.
 IMAGE_DLL_CHARACTERISTICS_FORCE_INTEGRITY       = 0x0080,  // Code integrity checks are forced. If you set this flag and a section contains only uninitialized data, set the PointerToRawData member of IMAGE_SECTION_HEADER for that section to zero; otherwise, the image will fail to load because the digital signature cannot be verified.
 IMAGE_DLL_CHARACTERISTICS_NX_COMPAT             = 0x0100,  // The image is compatible with data execution prevention (DEP).
 IMAGE_DLL_CHARACTERISTICS_NO_ISOLATION          = 0x0200,  // The image is isolation aware, but should not be isolated.
 IMAGE_DLL_CHARACTERISTICS_NO_SEH                = 0x0400,  // The image does not use structured exception handling (SEH). No handlers can be called in this image.
 IMAGE_DLL_CHARACTERISTICS_NO_BIND               = 0x0800,  // Do not bind the image.
 IMAGE_DLL_CHARACTERISTICS_APPCONTAINER          = 0x1000,  // Image should execute in an AppContainer.
 IMAGE_DLL_CHARACTERISTICS_WDM_DRIVER            = 0x2000,  // A WDM driver.
 IMAGE_DLL_CHARACTERISTICS_GUARD_CF              = 0x4000,  // Image supports Control Flow Guard.
 IMAGE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE = 0x8000,  // The image is terminal server aware.
};

enum EImgDirEntry: uint32 
{
 IMAGE_DIRECTORY_ENTRY_EXPORT         = 0,   // Export directory
 IMAGE_DIRECTORY_ENTRY_IMPORT         = 1,   // Import directory
 IMAGE_DIRECTORY_ENTRY_RESOURCE       = 2,   // Resource directory
 IMAGE_DIRECTORY_ENTRY_EXCEPTION      = 3,   // Exception directory
 IMAGE_DIRECTORY_ENTRY_SECURITY       = 4,   // Security directory
 IMAGE_DIRECTORY_ENTRY_BASERELOC      = 5,   // Base relocation table
 IMAGE_DIRECTORY_ENTRY_DEBUG          = 6,   // Debug directory
 IMAGE_DIRECTORY_ENTRY_ARCHITECTURE   = 7,   // Architecture-specific data
 IMAGE_DIRECTORY_ENTRY_GLOBALPTR      = 8,   // The relative virtual address of global pointer
 IMAGE_DIRECTORY_ENTRY_TLS            = 9,   // Thread local storage directory
 IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    = 10,  // Load configuration directory
 IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   = 11,  // Bound import directory
 IMAGE_DIRECTORY_ENTRY_IAT            = 12,  // Import address table
 IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   = 13,  // Delay import table
 IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR = 14,  // COM descriptor table
};

enum EImgSubSys: uint32
{
 IMAGE_SUBSYSTEM_UNKNOWN                  = 0,   // Unknown subsystem.
 IMAGE_SUBSYSTEM_NATIVE                   = 1,   // No subsystem required (device drivers and native system processes).
 IMAGE_SUBSYSTEM_WINDOWS_GUI              = 2,   // Windows graphical user interface (GUI) subsystem.
 IMAGE_SUBSYSTEM_WINDOWS_CUI              = 3,   // Windows character-mode user interface (CUI) subsystem.
 IMAGE_SUBSYSTEM_OS2_CUI                  = 5,   // OS/2 CUI subsystem.
 IMAGE_SUBSYSTEM_POSIX_CUI                = 7,   // POSIX CUI subsystem.
 IMAGE_SUBSYSTEM_WINDOWS_CE_GUI           = 9,   // Windows CE system.
 IMAGE_SUBSYSTEM_EFI_APPLICATION          = 10,  // Extensible Firmware Interface (EFI) application.
 IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  = 11,  // EFI driver with boot services.
 IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER       = 12,  // EFI driver with run-time services.
 IMAGE_SUBSYSTEM_EFI_ROM                  = 13,  // EFI ROM image.
 IMAGE_SUBSYSTEM_XBOX                     = 14,  // Xbox system.
 IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION = 16,  // Boot application.
};

//------------------------------------------------------------------------------------------------------------
#pragma pack( push, 1 )     // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Check alignment! <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

struct SDosHdr   //DOS_HEADER
{
 uint16  FlagMZ;               // Magic number                            0x00
 uint16  LastPageSize;         // Bytes on last page of file              0x02
 uint16  PageCount;            // Pages in file (Page = 512 bytes)        0x04
 uint16  RelocCount;           // Elements count in Relocations table     0x06
 uint16  HeaderSize;           // Size of header in paragraphs            0x08
 uint16  MinMemory;            // Min. extra paragraphs needed            0x0A
 uint16  MaxMemory;            // Max. extra paragraphs needed            0x0C
 uint16  ValueSS;              // Initial SS value                        0x0E
 uint16  ValueSP;              // Initial SP value                        0x10
 uint16  CheckSum;             // Checksum of the file                    0x12
 uint16  ValueIP;              // Initial IP value                        0x14
 uint16  ValueCS;              // Initial CS value                        0x16
 uint16  RelocTableOffset;     // Address of relocation table             0x18
 uint16  OverlayNumber;        // Overlay number (0 for main module)      0x1A
 uint32  Compl20h;             // Double paragraph align                  0x1C
 uint32  Reserved1;            // Reserved words                          0x20
 uint16  OemID;                // OEM identifier                          0x24
 uint16  OemInfo;              // OEM information                         0x26
 uint8   Reserved2[20];        // Reserved bytes                          0x28
 uint32  OffsetHeaderPE;       // File address of new exe header          0x3C
};
//------------------------------------------------------------------------------------------------------------
struct SDataDir     // DATA_DIRECTORY
{
 uint32  DirectoryRVA;     // RVA of the location of the directory.       0x00
 uint32  DirectorySize;    // Size of the directory.                      0x04
};
//------------------------------------------------------------------------------------------------------------
struct SDataDirTbl   // DATA_DIRECTORIES_TABLE
{
 SDataDir  ExportTable;      // Export Directory             0x78
 SDataDir  ImportTable;      // Import Directory             0x80
 SDataDir  ResourceTable;    // Resource Directory           0x88
 SDataDir  ExceptionTable;   // Exception Directory          0x90
 SDataDir  SecurityTable;    // Security Directory           0x98
 SDataDir  FixUpTable;       // Base Relocation Table        0xA0
 SDataDir  DebugTable;       // Debug Directory              0xA8
 SDataDir  ImageDescription; // Description String           0xB0
 SDataDir  MachineSpecific;  // Machine Value (MIPS GP)      0xB8
 SDataDir  TlsDirectory;     // TLS Directory                0xC0
 SDataDir  LoadConfigDir;    // Load Configuration Directory 0xC8
 SDataDir  BoundImportDir;   // Bound(Delayed) Import Dir    0xD0
 SDataDir  ImportAddrTable;  // Import Address Table         0xD8
 SDataDir  Reserved1;        // Reserved (Must be NULL)      0xE0
 SDataDir  Reserved2;        // Reserved (Must be NULL)      0xE8
 SDataDir  Reserved3;        // Reserved (Must be NULL)      0xF0
};
//------------------------------------------------------------------------------------------------------------
struct SFileHdr    // FILE_HEADER
{
 uint16  TypeCPU;         // Machine((Alpha/Motorola/.../0x014C = I386)   0x04
 uint16  SectionsNumber;  // Number of sections in the file               0x06
 uint32  TimeDateStamp;   // Number of seconds since Dec 31,1969,4:00PM   0x08
 uint32  TablePtrCOFF;    // Used in OBJ files and PE with debug info     0x0C
 uint32  TableSizeCOFF;   // The number of symbols in COFF table          0x10
 uint16  HeaderSizeNT;    // Size of the OptionalHeader structure         0x14
 uint16  Flags;           // 0000-Program; 0001-NoReloc; 0002-Can Exec;   0x16
};                        // 0200-Address fixed; 2000-This DLL
//------------------------------------------------------------------------------------------------------------
struct SOptHdr    // OPTIONAL_HEADER
{
STASRT(SameType<PHT, PTRCURRENT>::V || SameType<PHT, PTRTYPE32>::V || SameType<PHT, PTRTYPE64>::V, "Unsupported architecture type!");

 uint16  Magic;           // 0107-ROM projection;010B-Normal projection   0x18
 uint8   MajLinkerVer;    // Linker version number                        0x1A
 uint8   MinLinkerVer;    // Linker version number                        0x1B
 uint32  CodeSize;        // Sum of sizes all code sections(ordinary one) 0x1C
 uint32  InitDataSize;    // Size of the initialized data                 0x20
 uint32  UnInitDataSize;  // Size of the uninitialized data section (BSS) 0x24
 uint32  EntryPointRVA;   // Address of 1st instruction to be executed    0x28
 uint32  BaseOfCode;      // Address (RVA) of beginning of code section   0x2C
 typename TSW<SameType<PHT, PTRTYPE32>::V, uint32, ETYPE>::T  BaseOfData;   // Address (RVA) of beginning of data section   0x30
 typename TSW<SameType<PHT, PTRTYPE32>::V, uint32, uint64>::T ImageBase;    // The *preferred* load address of the file     0x34
/* union
  {
   struct
    {
     uint32 BaseOfData;  // Address (RVA) of beginning of data section   0x30
     ULONG ImageBase;    // The *preferred* load address of the file     0x34
    };
   ULONGLONG ImageBase64;
  };  */
 uint32  SectionAlign;      // Alignment of sections when loaded into mem   0x38
 uint32  FileAlign;         // Align. of sections in file(mul of 512 bytes) 0x3C
 uint16  MajOperSysVer;     // Version number of required OS                0x40
 uint16  MinOperSysVer;     // Version number of required OS                0x42
 uint16  MajImageVer;       // Version number of image                      0x44
 uint16  MinImageVer;       // Version number of image                      0x46
 uint16  MajSubSysVer;      // Version number of subsystem                  0x48
 uint16  MinSubSysVer;      // Version number of subsystem                  0x4A
 uint32  Win32Version;      // Dunno! But I guess for future use.           0x4C
 uint32  SizeOfImage;       // Total size of the PE image in memory         0x50
 uint32  SizeOfHeaders;     // Size of all headers & section table          0x54
 uint32  FileCheckSum;      // Image file checksum                          0x58
 uint16  SubSystem;         // 1-NotNeeded;2-WinGUI;3-WinCON;5-OS2;7-Posix  0x5C
 uint16  FlagsDLL;          // Used to indicate if a DLL image includes EPs 0x5E
 PHT     StackReserveSize;  // Size of stack to reserve                     0x60
 PHT     StackCommitSize;   // Size of stack to commit                      0x64 / 0x68
 PHT     HeapReserveSize;   // Size of local heap space to reserve          0x68 / 0x70
 PHT     HeapCommitSize;    // Size of local heap space to commit           0x6C / 0x78
 uint32  LoaderFlags;       // Choose Break/Debug/RunNormally(def) on load  0x70 / 0x80
 uint32  NumOfSizesAndRVA;  // Length of next DataDirectory array(alw10h)   0x74 / 0x84
 SDataDirTbl DataDirectories; //                             0x78 / 0x88
};
//------------------------------------------------------------------------------------------------------------
struct SWinHdr   // WIN_HEADER                // Must be uint64 aligned
{
 uint32    FlagPE;          // PE File Signature             0x00
 SFileHdr  FileHeader;      // File header                   0x04
 SOptHdr   OptionalHeader;  // Optional file header          0x18
};
//------------------------------------------------------------------------------------------------------------
struct SExpDir  // EXPORT_DIR
{
 uint32  Characteristics;     // Reserved MUST BE NULL                  0x00
 uint32  TimeDateStamp;       // Date of Creation                       0x04
 uint32  Version;             // Export Version - Not Used              0x08
 uint32  NameRVA;             // RVA of Module Name                     0x0C
 uint32  OrdinalBase;         // Base Number of Functions               0x10
 uint32  FunctionsNumber;     // Number of all exported functions       0x14
 uint32  NamePointersNumber;  // Number of functions names              0x18
 uint32  AddressTableRVA;     // RVA of Functions Address Table         0x1C
 uint32  NamePointersRVA;     // RVA of Functions Name Pointers Table   0x20
 uint32  OrdinalTableRVA;     // RVA of Functions Ordinals Table        0x24
};
//------------------------------------------------------------------------------------------------------------
struct SImpDir   // IMPORT_DESC
{
 uint32  LookUpTabRVA;     // 0x00
 uint32  TimeDateStamp;    // 0x04
 uint32  ForwarderChain;   // 0x08
 uint32  ModuleNameRVA;    // 0x0C
 uint32  AddressTabRVA;    // 0x10
};
//------------------------------------------------------------------------------------------------------------
struct SSecHdr   // SECTION_HEADER
{
 char    SectionName[8];        // 00
 uint32  VirtualSize;           // 08
 uint32  VirtualOffset;         // 0C   // SectionRva
 uint32  PhysicalSize;          // 10
 uint32  PhysicalOffset;        // 14
 uint32  PtrToRelocations;      // 18
 uint32  PtrToLineNumbers;      // 1C
 uint16  NumOfRelocations;      // 20
 uint16  NumOfLineNumbers;      // 22
 uint32  Characteristics;       // 24
};
//------------------------------------------------------------------------------------------------------------
struct SDbgHdr   // DEBUG_DIR
{
 uint32  Characteristics;
 uint32  TimeDateStamp;
 uint16  MajorVersion;
 uint16  MinorVersion;
 uint32  Type;
 uint32  SizeOfData;
 uint32  AddressOfRawData;
 uint32  PointerToRawData;
};
//------------------------------------------------------------------------------------------------------------
struct SRelocDesc   // RELOCATION_DESC  // Max for 4k page // There may be more than one such block on Relocaton Directory
{
 uint32 BaseRVA;
 uint32 BlkSize;
 struct
  {
   uint16 Offset : 12;
   uint16 Type   : 4;
  }Records[0];

 uint Count(void){return (this->BlkSize - sizeof(SRelocDesc)) / sizeof(uint16);}
};
//------------------------------------------------------------------------------------------------------------
struct SRsrcDirEntry       //   _IMAGE_RESOURCE_DIRECTORY_ENTRY
{
 union
  {
   struct
    {
     uint32 NameOffset   : 31;
     uint32 NameIsString : 1;
    };
   uint32 Name;
   uint16 Id;
  };
 union
  {
   uint32 OffsetToData;
   struct
    {
     uint32 OffsetToDirectory : 31;
     uint32 DataIsDirectory   : 1;
    };
  };
};
//------------------------------------------------------------------------------------------------------------
struct SRsrcDir    //RESOURCE_DIR
{
 uint32  Characteristics;
 uint32  TimeDateStamp;
 uint16  MajorVersion;
 uint16  MinorVersion;
 uint16  NumberOfNamedEntries;
 uint16  NumberOfIdEntries;
 SRsrcDirEntry DirectoryEntries[];    // Difined in WINNT.H    // IMAGE_RESOURCE_DIRECTORY_ENTRY
};
//------------------------------------------------------------------------------------------------------------
struct SRsrcDataEntry    // IMAGE_RESOURCE_DATA_ENTRY
{
 uint32  OffsetToData;
 uint32  Size;
 uint32  CodePage;
 uint32  Reserved;
};
//------------------------------------------------------------------------------------------------------------
struct SRsrcDirString     // IMAGE_RESOURCE_DIRECTORY_STRING
{
 uint16  Length;
 achar   NameString[1];
};
//------------------------------------------------------------------------------------------------------------
struct SImportByName
{
 uint16  Hint;
 uint8   Name[1];
};
struct SImportThunk
{
 PHT Value;	   // ForwarderString; Function; Ordinal; AddressOfData;
};
//------------------------------------------------------------------------------------------------------------
struct SRichRec
{
 uint16  Ver;    // MinVer
 uint16  PId;    // Product Identifier
 uint32  Cntr;   // Counter of what?
};
#pragma pack( pop )
//============================================================================================================
//
//------------------------------------------------------------------------------------------------------------
 /*size_t    ModLen   = 0;
 size_t    HdrLen   = 0;
 uint8*    HdrPtr   = nullptr;
 SDosHdr*  DosHdr   = nullptr;
 SWinHdr*  WinHdr   = nullptr;
 SOptHdr*  OptHdr   = nullptr;
 SFileHdr* FileHdr  = nullptr;
 SSecHdr*  FirstSec = nullptr;

int Assign(vptr ModuleBase, size_t ModuleSize=0)     // Useless?
{
 this->ModLen   = ModuleSize;
 this->HdrPtr   = (uint8*)ModuleBase;
 this->DosHdr   = (SDosHdr*)this->HdrPtr;
 if((this->DosHdr->FlagMZ != SIGN_MZ))return -1;
 if(this->ModLen && ((this->DosHdr->OffsetHeaderPE+sizeof(SWinHdr)) > this->ModLen))return -2;
 this->WinHdr   = (SWinHdr*)&this->HdrPtr[this->DosHdr->OffsetHeaderPE];
 if((this->WinHdr->FlagPE != SIGN_PE))return -3;
 this->OptHdr   = &this->OptionalHeader;
 this->FileHdr  = &this->FileHeader;

 this->HdrLen   = this->DosHdr->OffsetHeaderPE + this->WinHdr->FileHeader.HeaderSizeNT + sizeof(SFileHdr) + sizeof(uint32);
 this->FirstSec = (SSecHdr*)&this->HdrPtr[HdrLen];
 return 0;
}  */
//============================================================================================================
//
//------------------------------------------------------------------------------------------------------------
_finline static uint32 GetOffsInSec(vptr ModuleBase, uint rva)
{
 if constexpr(RawPE)return RvaToFileOffset(ModuleBase, rva);
   else return rva;  // Any unfixed compiler bugs? (undetected exit branch by the optimizer)
}
//------------------------------------------------------------------------------------------------------------
_finline static SWinHdr* GetWinHdr(vptr ModuleBase)
{
 return (SWinHdr*)&(((uint8*)ModuleBase)[((SDosHdr*)ModuleBase)->OffsetHeaderPE]);
}
//-----------------------------------------------------------------------------------------------------------
_finline static PHT& BaseOfModule(vptr ModuleBase)
{
 auto WinHdr = GetWinHdr(ModuleBase);
 return WinHdr->OptionalHeader.ImageBase;
}             
//------------------------------------------------------------------------------------------------------------
_finline static uint32& SizeOfModule(vptr ModuleBase)      // Virtual
{
 auto WinHdr = GetWinHdr(ModuleBase);  
 return WinHdr->OptionalHeader.SizeOfImage;
}
//------------------------------------------------------------------------------------------------------------
_finline static size_t GetModuleEntryOffset(vptr ModuleBase)
{
 auto WinHdr = GetWinHdr(ModuleBase); 
 if(!WinHdr->OptionalHeader.EntryPointRVA)return 0;
 return GetOffsInSec(ModuleBase, WinHdr->OptionalHeader.EntryPointRVA);
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Do not expect that all PE image pages will be allocated
_finline static bool IsValidHeaderPE(vptr ModuleBase)
{
 SDosHdr* DosHdr = (SDosHdr*)ModuleBase;
 if((DosHdr->FlagMZ != SIGN_MZ))return false;
 auto WinHdr = GetWinHdr(ModuleBase);  
 if((WinHdr->FlagPE != SIGN_PE))return false;
 return true;
}
//------------------------------------------------------------------------------------------------------------
static bool IsValidHeaderPE(vptr ModuleBase, uint Size)
{
 if(Size < sizeof(SDosHdr))return false;
 SDosHdr* DosHdr  = (SDosHdr*)ModuleBase;
 if((DosHdr->FlagMZ != SIGN_MZ))return false;
 if(Size < (DosHdr->OffsetHeaderPE + sizeof(SWinHdr)))return false;
 auto WinHdr = GetWinHdr(ModuleBase);  
 if((WinHdr->FlagPE != SIGN_PE))return false;
 return true;
}
//------------------------------------------------------------------------------------------------------------
_finline static bool IsModuleDLL(vptr ModuleBase)
{
 auto WinHdr = GetWinHdr(ModuleBase); 
 return (WinHdr->FileHeader.Flags & 0x2002) == 0x2002;   // 0x2000=IsDll; 0x0002=IsExecutable
}
//------------------------------------------------------------------------------------------------------------
_finline static bool IsModuleEXE(vptr ModuleBase)
{
 auto WinHdr = GetWinHdr(ModuleBase); 
 return !(WinHdr->FileHeader.Flags & 0x2000); // IsDll flag
}
//------------------------------------------------------------------------------------------------------------
_finline static bool IsModuleX64(vptr ModuleBase)
{
 auto WinHdr = GetWinHdr(ModuleBase);  
 return (WinHdr->OptionalHeader.Magic == 0x020B);
}
//------------------------------------------------------------------------------------------------------------
_finline static bool IsRvaInSection(SSecHdr* Sec, uint Rva)
{
 if(Sec->VirtualOffset > Rva)return false;
 if((Sec->VirtualOffset+Sec->VirtualSize) <= Rva)return false;
 return true;
}
//------------------------------------------------------------------------------------------------------------
_finline static size_t GetSecArrOffs(vptr ModuleBase)
{
 SDosHdr*  DosHdr = (SDosHdr*)ModuleBase;
 auto      WinHdr = GetWinHdr(ModuleBase);  
 return DosHdr->OffsetHeaderPE + WinHdr->FileHeader.HeaderSizeNT + sizeof(SFileHdr) + sizeof(uint32);
}
//------------------------------------------------------------------------------------------------------------
static uint GetSections(vptr ModuleBase, SSecHdr** FirstSec)
{
 auto WinHdr = GetWinHdr(ModuleBase);  
 uint HdrLen = GetSecArrOffs(ModuleBase);
 *FirstSec = (SSecHdr*)&((uint8*)ModuleBase)[HdrLen];
 return WinHdr->FileHeader.SectionsNumber;
}
//------------------------------------------------------------------------------------------------------------
static size_t RvaToFileOffset(vptr ModuleBase, uint Rva)
{
 SSecHdr* CurSec = nullptr;
 for(uint ctr = 0, total = GetSections(ModuleBase, &CurSec);ctr < total;ctr++,CurSec++)
  {
   if(!IsRvaInSection(CurSec, Rva))continue;
   Rva -= CurSec->VirtualOffset;
   if(Rva >= CurSec->PhysicalSize)return 0;  // Not present in the file as physical
   return (CurSec->PhysicalOffset + Rva);
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static size_t FileOffsetToRva(vptr ModuleBase, uint Offset)
{
 SSecHdr* CurSec = nullptr;
 for(uint ctr = 0, total = GetSections(ModuleBase, &CurSec);ctr < total;ctr++,CurSec++)
  {
   if((Offset >= CurSec->PhysicalOffset)&&(Offset < (CurSec->PhysicalOffset+CurSec->PhysicalSize)))
	{
	 return ((Offset-CurSec->PhysicalOffset)+CurSec->VirtualOffset);
	}
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static size_t SizeOfSections(vptr ModuleBase, uint MaxSecs=(uint)-1, bool RawSize=false)
{
 auto WinHdr = GetWinHdr(ModuleBase); 
 SSecHdr* CurSec = nullptr; 
 uint TotalSecs  = GetSections(ModuleBase, &CurSec);
 //uint BOffs = (uint)-1;
 uint EOffs = 0;
 if(TotalSecs < MaxSecs)TotalSecs = MaxSecs;
 for(uint ctr=0;ctr < TotalSecs;ctr++)
  {
   uint soffs = RawSize?(CurSec[ctr].PhysicalOffset):(CurSec[ctr].VirtualOffset);
   uint ssize = RawSize?(AlignFrwd(CurSec[ctr].PhysicalSize, WinHdr->OptionalHeader.FileAlign)):(AlignFrwd(CurSec[ctr].VirtualSize, WinHdr->OptionalHeader.SectionAlign));
   uint sendo = soffs + ssize;
 //  if(soffs < BOffs)BOffs = soffs;
   if(sendo > EOffs)EOffs = sendo; 
  }
 //if(!Offs)Offs = AlignFrwd(WinHdr->OptionalHeader.SizeOfHeaders, (RawSize?WinHdr->OptionalHeader.FileAlign:WinHdr->OptionalHeader.SectionAlign));
 return EOffs;  //Offs + Size;
}
//------------------------------------------------------------------------------------------------------------
static bool GetSectionForAddress(vptr ModuleBase, vptr Address, SSecHdr** ResSec)
{
 SSecHdr* CurSec = nullptr; 
 uint TotalSecs = GetSections(ModuleBase, &CurSec);
 for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
  {
   if(((uint8*)Address >= ((uint8*)ModuleBase + CurSec->VirtualOffset)) && ((uint8*)Address < ((uint8*)ModuleBase + CurSec->VirtualOffset + CurSec->VirtualSize)))
    {
     if(ResSec)*ResSec = CurSec;  // NULL for only a presense test
     return true;
    }
  }
 return false;
}
//------------------------------------------------------------------------------------------------------------
// SecName can be an integer section index
static bool GetModuleSection(vptr ModuleBase, achar* SecName, SSecHdr** ResSec)
{
 SSecHdr* CurSec = nullptr;  
 uint TotalSecs = GetSections(ModuleBase, &CurSec);
 for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
  {
   if(((uint)SecName == ctr) || NSTR::IsStrEqualCS((achar*)&CurSec->SectionName, SecName))
    {
     if(ResSec)*ResSec = CurSec;  // NULL for only a presense test
     return true;
    }
  }
 return false;
}
//------------------------------------------------------------------------------------------------------------
// Based on a last section size, unaligned
static size_t CalcModuleSize(vptr ModuleBase)
{
 SSecHdr* CurSec = nullptr;
 uint TotalSecs  = GetSections(ModuleBase, &CurSec);
 if(!TotalSecs)return 0;
 SSecHdr* LstSec = CurSec;
 for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
  {
   if(CurSec->VirtualOffset > LstSec->VirtualOffset)LstSec = CurSec;
  }
 return LstSec->VirtualOffset + LstSec->VirtualSize;
}
//------------------------------------------------------------------------------------------------------------
static size_t CalcModuleSizeRaw(vptr ModuleBase)
{
 SSecHdr* CurSec = nullptr;
 uint TotalSecs  = GetSections(ModuleBase, &CurSec);
 if(!TotalSecs)return 0;
 SSecHdr* LstSec = CurSec;
 for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
  {
   if(CurSec->PhysicalOffset > LstSec->PhysicalOffset)LstSec = CurSec;
  }
 return LstSec->PhysicalOffset + LstSec->PhysicalSize;
}
//------------------------------------------------------------------------------------------------------------
static uint32 CalcChecksumPE(vptr ModuleBase, size_t Size)   // TODO: Rewrite
{
 auto WinHdr = GetWinHdr(ModuleBase);  

 uint64 checksum = 0;   //  unsigned long long
 uint64 top = 0xFFFFFFFF + 1;  // top++;

 uint32 CSimOffs = (uint8*)&WinHdr->OptionalHeader.FileCheckSum - (uint8*)ModuleBase;
 uint8* DataPtr  = (uint8*)ModuleBase;
 for(size_t idx=0;idx < Size;idx += 4)
  {
   if(idx == CSimOffs)continue;   //Skip "CheckSum" DWORD
   checksum = (checksum & 0xFFFFFFFF) + *(uint32*)&DataPtr[idx] + (checksum >> 32);     // Calculate checksum
   if(checksum > top)checksum = (checksum & 0xFFFFFFFF) + (checksum >> 32);             // TODO: Without 64bit shift
  }
 //Finish the checksum
 checksum  = (checksum & 0xFFFF) + (checksum >> 16);
 checksum  = checksum + (checksum >> 16);
 checksum  = checksum & 0xFFFF;
 checksum += Size;
 return checksum;
}
//------------------------------------------------------------------------------------------------------------
static achar* GetExpModuleName(vptr ModuleBase)  // Any PE type(PE32/PE64) // Only if an Export section is present     // TODO: Optional Pointer validation mode
{
 SDataDir* ExportDir = (IsModuleX64(ModuleBase))?((SDataDir*)&NPE64::GetWinHdr(ModuleBase)->OptionalHeader.DataDirectories.ExportTable):((SDataDir*)&NPE32::GetWinHdr(ModuleBase)->OptionalHeader.DataDirectories.ExportTable);
 if(!ExportDir->DirectoryRVA)return nullptr;
 SExpDir* Export     = (SExpDir*)&((uint8*)ModuleBase)[ExportDir->DirectoryRVA];
 return &((achar*)ModuleBase)[GetOffsInSec(ModuleBase,Export->NameRVA)];
}
//------------------------------------------------------------------------------------------------------------
static void* ModuleAddressToBase(vptr Addr)   // NOTE: Will find a PE header or will crash trying:)    // WARNING: May crash! Not all module sections may be present in memory (discarded)
{
 uint8* Base = (uint8*)(((size_t)Addr) & (size_t)~0xFFFF);   // All modules are 64k aligned by loader
 while(!IsValidHeaderPE(Base))Base -= 0x10000;  // !MmIsAddressValid(Base) ? IsBadReadPtr(Base,sizeof(DOS_HEADER)) // Why not all pages of ntoskrnl.exe are available on x64?
 //DBGMSG("Found a module base: %p", Base);
 return Base;
}
//------------------------------------------------------------------------------------------------------------
static vptr GetResBodyRaw(vptr ModuleBase, vptr ResBase, SRsrcDir* Resour, uint32 RootID, uint32* ResIdx, size_t* Size, achar** Name, SRsrcDataEntry** DEntr=nullptr)
{
 for(uint32 ctr=0;ctr < Resour->NumberOfIdEntries;ctr++)
  {
   SRsrcDirEntry* entr = &Resour->DirectoryEntries[ctr];
   if((RootID != (uint32)-1)&&((uint8*)Resour == (uint8*)ResBase))
    {
     if(entr->Name & 0x80000000)continue; // Name instead of ID encountered in the Root
     if(RootID != (uint32)entr->Name)continue;
    }
   if(entr->OffsetToData & 0x80000000)
    {
     SRsrcDir* RDir = (SRsrcDir*)&((uint8*)ResBase)[entr->OffsetToData & ~0x80000000];
     vptr res = GetResBodyRaw(ModuleBase, ResBase, RDir, RootID, ResIdx, Size, Name, DEntr);
     if(res)return res;  // Found resource
    }
     else
      {
       if(ResIdx && *ResIdx){(*ResIdx)--; continue;}
       achar* LNam = nullptr;
       if(entr->Name & 0x80000000)    // Here it is a LangID only?
        {
         SRsrcDirString* str = (SRsrcDirString*)&((uint8*)ResBase)[entr->Name & ~0x80000000];
         LNam = (achar*)&str->NameString;  // -2(WORD) is size of the string
        }
         else LNam = (achar*)entr->Name;   // Type      // x64 ????????????????????????????
       if(*Name)
        {
         if(entr->Name & 0x80000000){if(!NSTR::IsStrEqualCS(LNam,*Name))continue;}       // lstrcmp(LNam,*Name) != 0
           else if(LNam != *Name)continue;
        }
         else *Name = LNam;
       SRsrcDataEntry* Dentry = (SRsrcDataEntry*)&((uint8*)ResBase)[entr->OffsetToData];
       *Size = Dentry->Size;
       if(DEntr)*DEntr = Dentry;
       return &((uint8*)ModuleBase)[RvaToFileOffset(ModuleBase,Dentry->OffsetToData)];   // Raw???????
      }
  }
 return nullptr; 
}
//------------------------------------------------------------------------------------------------------------
static vptr GetResourceRaw(vptr ModuleBase, uint32 RootID, uint32 ResIdx, size_t* Size, achar** Name, SRsrcDataEntry** DEntr=nullptr)
{
 auto WinHdr = GetWinHdr(ModuleBase);
 SDataDir* ResDir = &WinHdr->OptionalHeader.DataDirectories.ResourceTable;
 SRsrcDir* Resour = (SRsrcDir*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, ResDir->DirectoryRVA)];
 return GetResBodyRaw(ModuleBase, (uint8*)Resour, Resour, RootID, &ResIdx, Size, Name, DEntr);
}
//------------------------------------------------------------------------------------------------------------
static achar* FindImportTable(vptr ModuleBase, achar* LibName, SImportThunk** LookUpRec, SImportThunk** AddrRec)
{
 auto WinHdr = GetWinHdr(ModuleBase);
 SDataDir* ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 if(!ImportDir->DirectoryRVA)return nullptr;
 SImpDir*  Import    = (SImpDir*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, ImportDir->DirectoryRVA)];
 for(uint32 tctr=0;Import[tctr].AddressTabRVA;tctr++)
  {
   achar* ModName = (achar*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Import[tctr].ModuleNameRVA)];
   if(LibName && !NSTR::IsStrEqualCI(ModName,LibName))continue;     // Can search for a proc of any module  
   SImportThunk* Table = (SImportThunk*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Import[tctr].LookUpTabRVA)];
   SImportThunk* LtRVA = (SImportThunk*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Import[tctr].AddressTabRVA)];
   if(AddrRec)*AddrRec = &LtRVA[tctr];
   if(LookUpRec)*LookUpRec = &Table[tctr];
   return ModName;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
static bool FindImportRecord(vptr ModuleBase, achar* LibName, achar* ProcName, SImportThunk** LookUpRec, SImportThunk** AddrRec)   // Possible not fully correct  
{
 auto WinHdr = GetWinHdr(ModuleBase);
 SDataDir* ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 if(!ImportDir->DirectoryRVA)return false;
 SImpDir*  Import    = (SImpDir*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, ImportDir->DirectoryRVA)];

 PHT  OMask = ((PHT)1 << ((sizeof(PHT)*8)-1));
 bool ByOrd = ((PHT)ProcName <= 0xFFFF);
 for(uint32 tctr=0;Import[tctr].AddressTabRVA;tctr++)
  {
   achar* ModName = (achar*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Import[tctr].ModuleNameRVA)];
   if(LibName && !NSTR::IsStrEqualCI(ModName,LibName))continue;     // Can search for a proc of any module  
   SImportThunk* Table = (SImportThunk*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Import[tctr].LookUpTabRVA)];
   SImportThunk* LtRVA = (SImportThunk*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Import[tctr].AddressTabRVA)];
   if(ProcName == LibName)    // Secret :)   // If we need just any address inside that module
    {
     for(;!(*Table).Value || !(*LtRVA).Value;Table++,LtRVA++);
     if(AddrRec)*AddrRec = LtRVA;
     if(LookUpRec)*LookUpRec = Table;
	 return true;
    }
   for(uint32 actr=0;Table[actr].Value;actr++)
    {
	 bool OnlyOrd  = (Table[actr].Value & OMask);
	 uint16 OIndex = 0;
	 if(OnlyOrd)
	  {
	   if(!ByOrd)continue;
	   PHT Ord = Table[actr].Value & ~OMask;
	   if((PHT)ProcName != Ord)continue;
	   OIndex = Ord;          // LONGLONG ordinal?
	  }
	   else
	    {
		 SImportByName* INam = (SImportByName*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Table[actr].Value)];
		 if(ByOrd)
		  {
		   if(((PHT)ProcName != INam->Hint))continue;
		  }
		   else
		    {
			 if(!NSTR::IsStrEqualCS((achar*)&INam->Name, ProcName))continue;
			}
//		 OIndex = OIndex;  // ???
		}
     if(AddrRec)*AddrRec = &LtRVA[actr];
     if(LookUpRec)*LookUpRec = &Table[actr];
	 return true;
    } 
  }
 return false;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Kernel32.dll exports some functions with a different names(i.e. lstrlen and lstrlenA)
// Will not find a forwarded address
//
static const achar* GetProcInfo(vptr ModuleBase, vptr ProcAddrOrRva, uint16* OrdinalOut=nullptr, int MatchIdx=0)    // NOTE: No raw support (because lookup is by addr)
{
 auto WinHdr = GetWinHdr(ModuleBase);
 SDataDir* ExportDir  = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 SExpDir*  Export     = (SExpDir*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, ExportDir->DirectoryRVA)];
                   
 uint32* NamePointers = (uint32*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Export->NamePointersRVA)];
 uint32* AddressTable = (uint32*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Export->AddressTableRVA)];
 uint16* OrdinalTable = (uint16*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, Export->OrdinalTableRVA)];
 for(uint32 Ordinal=0; (Ordinal < Export->FunctionsNumber) && (Ordinal <= 0xFFFF);Ordinal++)
  {
   size_t rva  = AddressTable[Ordinal];
   size_t Addr = (size_t)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase,rva)];  
   if((Addr != (size_t)ProcAddrOrRva)&&(rva != (size_t)ProcAddrOrRva))continue;
   MatchIdx--;
   if(MatchIdx > 0)continue;
   for(uint32 ctr=0;ctr < Export->NamePointersNumber;ctr++)  // By name
    {      
     if(Ordinal != OrdinalTable[ctr])continue;
     uint32 nrva = NamePointers[ctr];   
     if(OrdinalOut)*OrdinalOut = Ordinal;
     return (achar*)&((uint8*)ModuleBase)[GetOffsInSec(ModuleBase, nrva)];
    }
   break;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------     
static vptr GetProcAddr(vptr ModuleBase, const achar* ProcName, achar** Forwarder=nullptr, vptr* ProcEntry=nullptr)  // No forwarding support
{
 uint8* DllBase  = (uint8*)((size_t)ModuleBase & ~(size_t)1);
 bool   UseNHash = (size_t)ModuleBase & 1;   // Secret flag  // TODO: Check - it is probably already used by handles for something secret too
 auto WinHdr = GetWinHdr(DllBase);
 SDataDir* ExportDir  = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 if(!ExportDir->DirectoryRVA || !ExportDir->DirectorySize)return nullptr;		 // No export directory!
 SExpDir*  Export     = (SExpDir*)&DllBase[GetOffsInSec(DllBase, ExportDir->DirectoryRVA)];
                   
 uint32* NamePointers = (uint32*)&DllBase[GetOffsInSec(DllBase, Export->NamePointersRVA)];
 uint32* AddressTable = (uint32*)&DllBase[GetOffsInSec(DllBase, Export->AddressTableRVA)];
 uint16* OrdinalTable = (uint16*)&DllBase[GetOffsInSec(DllBase, Export->OrdinalTableRVA)];
 size_t Ordinal = (size_t)ProcName;
 if(!UseNHash)
  {
   if(Ordinal <= 0xFFFF)  // By Ordnal
    {
     if(Ordinal < Export->OrdinalBase)return nullptr;
     Ordinal -= Export->OrdinalBase;
    }
     else
      {
       for(uint32 ctr=0;ctr < Export->NamePointersNumber;ctr++)  // By name
        {      
         uint32 nrva  = NamePointers[ctr];   
         if(!nrva || !NSTR::IsStrEqualCS((achar*)&DllBase[GetOffsInSec(DllBase, nrva)], ProcName))continue;    
         Ordinal = OrdinalTable[ctr];      // Name Ordinal 
         break;
        }
       if(Ordinal > 0xFFFF)return nullptr;
      }
   }
   else
   {
    uint32 NameCrc = uint32((size_t)ProcName);
    for(uint32 ctr=0;ctr < Export->NamePointersNumber;ctr++)  // By name
     {      
      uint32 nrva = NamePointers[ctr];   
      uint32 crc  = NCRYPT::CRC32((achar*)&DllBase[GetOffsInSec(DllBase, nrva)]);
      if(!nrva || (crc != NameCrc))continue;    
      Ordinal = OrdinalTable[ctr];      // Name Ordinal 
      break;
     }
    if(Ordinal > 0xFFFF)return nullptr;
   }
 uint8* Addr = &DllBase[GetOffsInSec(DllBase, AddressTable[Ordinal])];  
 if(ProcEntry)*ProcEntry = &AddressTable[Ordinal];
 if(Forwarder && (Addr >= (uint8*)Export) && (Addr < ((uint8*)Export+ExportDir->DirectorySize)))*Forwarder = (achar*)Addr;
 return Addr;     
}
//------------------------------------------------------------------------------------------------------------
static vptr GetProcAddrSafe(vptr ModuleBase, achar* ProcName, achar** Forwarder=nullptr, vptr* ProcEntry=nullptr)   // Any PE type
{
// DBGMSG("ApiName: %s",ApiName);
 if(!ModuleBase)return nullptr;
 if(!IsValidHeaderPE(ModuleBase))return nullptr;         // NOTE: MSVC mode Clang goes not validates arguments in function calls inside of an unreferenced template functions!  // Nonexistent function names with any number of arguments is acceplable!
 if(IsModuleX64(ModuleBase))return NFMTPE<uint64>::GetProcAddr(ModuleBase,ProcName,Forwarder,ProcEntry); 
 return NFMTPE<uint32>::GetProcAddr(ModuleBase,ProcName,Forwarder,ProcEntry); 
}
//------------------------------------------------------------------------------------------------------------
};

using NPE    = NFMTPE<size_t>;
using NPE32  = NFMTPE<uint32>;
using NPE64  = NFMTPE<uint64>;

using NPER   = NFMTPE<size_t,true>;
using NPE32R = NFMTPE<uint32,true>;
using NPE64R = NFMTPE<uint64,true>;
//============================================================================================================


