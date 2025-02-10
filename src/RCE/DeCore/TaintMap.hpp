

//===========================================================================
class CTaintTree
{
 enum EFlags {fNone, 
              fRead  = 0x01,   // The range had Read access (Propagates to all parents)      // Should propagate to children too
              fWrite = 0x02,   // The range had Write access (Propagates to all parents)     // User defined flags should be separate?
 //             fGroup = 0x10,   // The region is for grouping only, actual accesses is expected to its children  // Its tag most likely will refer to allocated memory, if some emulation is done
};

struct alignas(size_t) SRangeNode  // Size: 48 bytes! (p2 size is optimal)  
{
 uint32 PrevNode;
 uint32 NextNode; 
 uint32 ChildNode;     // First child node that exists (closest offset, sorted)
 uint32 ParentNode;

 uint64 BitOffs;       // Virtual address space  // x64 address ranges are 48,49,52,56 bits wide. High bit is extended to unused bits   // x86-64: bit 47 is the sign bit that extends to higher bits (0-user;1-kernel)
 uint64 BitSize;       // For max 512mb blocks   // Should be enough but some extra bits could be taken from BitOffs and Flags fields if needed  // Registers are too mapped to the same memory range somewhere

// Offs 32  
 uint32 NodeTag;       // Propagates to relevant touched nodes  // To reference some metadata (Like a context to the memory or a last accessor)     // Last reader, last writer, Memory pointer, if allocated
 uint32 SharedTag;     // Behaviours: Stays only on this node, taken from parent when adding new
    
 uint16 NodeID;        // Some location specific annotation
 uint16 SharedID;      // Propagated to new Child/Parent regions   // (Register, stack, memory, exe_lib, ...) 

 uint16 NodeFlags;     // Private, not propagated  (Like a flag that this region have allocated memory)
 uint16 SharedFlags;   // Internal, propagated to Child regions
};

static_assert(sizeof(SRangeNode) == 48);
//---------------------------------------------------------------------------
 NALC::SBlkAlloc<65536,NALC::afGrowLin,SRangeNode> Array;       // TODO: Test that the block size and alloc strategy is optimal for this container            1024*64
 uint32 FirstNode;       // First node at the top level (No parents)
//---------------------------------------------------------------------------
// May not find the associated Node but still return closest Prev and Next ranges
// If returns NULL but PrevNode and NextNode then the new node may be added between them
// NOTE: There may be gaps between siblings, untouched at their level
//
SRangeNode* FindRangeByOffs(uint64 BitOffs, uint32 FromNode=0, uint32* RetNodeID=nullptr, SRangeNode** PrevNode=nullptr, SRangeNode** NextNode=nullptr)
{ 
 SRangeNode* PNode = nullptr; 
 SRangeNode* NNode = nullptr; 
 SRangeNode* Node  = nullptr; 
 size_t   NodeIdx  = 0; 

 if(this->FirstNode)return nullptr;   // No nodes!
 if(!FromNode)FromNode = this->FirstNode;
 auto it = this->Array.ElmFrom(FromNode-1);
 while(it)  // Goes deeper into children if a parent range matches      // The nodes are sorted by ByteOffs:BitOffs
  {   
   uint64 EndBOffs = it->BitOffs + it->BitSize;
   if(EndBOffs > BitOffs)
    {
     if(BitOffs < it->BitOffs){NNode = &*it; break;}  // No associated node!  // This node may to become a Next node for this range
     NodeIdx = it;   // As an index
     Node    = &*it; // As a pointer to SRangeNode
     if(!it->ChildNode)break;
     it = (it->ChildNode - 1);    // Try children to narrow the match
     PNode = NNode = nullptr;     // Reset the chain
    }
     else 
      {
       PNode = &*it;
       it = (it->NextNode - 1);
      }
  }
 if(Node && RetNodeID)*RetNodeID = (NodeIdx + 1);
 if(PrevNode)
  {
   SRangeNode* MatchNode = Node?Node:NNode;
   if(!PNode && MatchNode && MatchNode->PrevNode) 
    {
     it = (MatchNode->PrevNode - 1);
     PNode = &*it;
    }
   *PrevNode = PNode; 
  }
 if(NextNode)
  {
   if(!NNode && Node && Node->NextNode) 
    {
     it = (Node->NextNode - 1);
     NNode = &*it;
    }
   *NextNode = NNode;
  }
 return Node;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
public:
//---------------------------------------------------------------------------
CTaintTree(void)
{
 this->FirstNode = 0;
}
//---------------------------------------------------------------------------
~CTaintTree()
{

}
//---------------------------------------------------------------------------
// Searches for exact range match if BitSize is not 0
// Returns NULL if the range is not found
//
SRangeNode* FindRange(uint64 ByteOffs, uint32 BitSize=0, uint8 BitOffs=0)    // Internal find should return last parent and closest sibling which will help to avoit iterating them again
{
 return nullptr;
}
//---------------------------------------------------------------------------
// Register a memory region for grouping (Can do it after tracing too)
// You can partition some range on the stack to represent a structure composed of bitfields
// Does nothing if the range matches exactly
// NOTE: Only regions with same parent become siblings!
//
SRangeNode* AddRange(uint64 ByteOffs, uint32 BitSize, uint8 BitOffs=0)      
{
 return nullptr;
}
//---------------------------------------------------------------------------
// May create a new range if touching it first time (RegionID will be taken from its parent, if any - must check for 0)
// It is your responsibility to update the record, associated with the tag, to refer to the last accessor.
// If several nodes at same level is touched at once and have same parent, they become siblings
// When touching covers several nodes, a new node is created and becomes their parent. The child nodes will become siblings (Watch for gaps)
// Flags: fRead, fWrite 
//
SRangeNode* TouchRange(uint64 ByteOffs, uint32 BitSize, uint8 Flags, uint8 BitOffs=0)   // Additionally pass a index to a parent node in which we should start lookup
{
 return nullptr;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
};
//===========================================================================