
#pragma once

//#pragma warning(disable:4800)          // forcing value to bool 'true' or 'false' (performance warning)     // TODO: Reenable it afterwards
// TODO: If separate, include the framework if not included or if not defined

/*
 struct SPropIdx
 {

 }
 ObjBase
  {
   uint16 Flags;
   uint16 Type;
   uint32 PropIdx;    // Index of a first property, if any // When an object is created, it created with fixed
  }

 Prop
 {

 }

// https://stackoverflow.com/questions/6716946/why-do-x86-64-systems-have-only-a-48-bit-virtual-address-space



*/

struct SDeCoreDefCfg
{
 SCVR int AddrSize = 48;   // Max 56    // 1 byte is for SoftSeg and 1 byte is for MiIdx
};


template<typename TPConf=SDeCoreDefCfg> struct CDeCore
{
 using TCFG = TPConf;
 using TInsIdx = uint32;   // For instruction lists
 using TOpdIdx = uint32;   // For operand lists
 using TLstIdx = uint32;   // For misc lists

 SCVR int MaxAddrBits = 56;

//---------------------------------------------------------------------------
//#include "PropIndex.hpp"
#include "Objects.hpp"
//#include "TaintMap.hpp"
//#include "IrEntities.hpp"
//#include "Objects\\BaseLists.h"
//#include "Objects\\BaseArrays.h"
//#include "Objects\\EntityLists.h"
//#include "Objects\\InstrDefsList.h"
//#include "Objects\\InstructionItem.h"
//#include "Objects\\BranchVisList.h"
//#include "Objects\\EntryDefsList.h"
//#include "Objects\\DecompItem.h"
//#include "Objects\\EntryItem.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
public:
TInsIdx AddInstr(uint64 Addr, SOperand* OpdArr, uint8 OpdNum, uint8 Type, uint32 Flags, uint32 Hint=0, uint8 MIdx=0, uint8 Seg=0)
{

}
//---------------------------------------------------------------------------
TInsIdx InsertInstr(TInsIdx InsAfter, uint64 Addr, SOperand* OpdArr, uint8 OpdNum, uint8 Type, uint32 Flags, uint32 Hint=0, uint8 MIdx=0, uint8 Seg=0) 
{

}
//---------------------------------------------------------------------------
int DeleteInstr(TInsIdx Index)
{

}
//---------------------------------------------------------------------------
int AddInstrOperands(TInsIdx Index, SOperand* OpdArr, uint8 OpdNum)
{

}
//---------------------------------------------------------------------------
int RemoveInstrOperand(TInsIdx Index, TOpdIdx OpIdx)
{

}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------



};
