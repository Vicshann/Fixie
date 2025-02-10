

template<size_t PageLen, uint32 Flg, typename Ty=uint32, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemPrvBase, size_t MaxAlign=DefMaxAlign, size_t LBlkMin=0> class CEntiySys
{
// Components not considered to be removable and their ID does not have a SeqID
struct SCmptID
{
 uint32 Shared : 1;  // This component is shared with other entities
 uint32 Type   : 7;  // Should be high bits (TODO: Assert in debug builds)   // Size/Type class   // 128 different component types (Vec3 is the Type for a entity`s Position component)
 uint32 Index  : 24; // uint32: max 16777215       // TODO: Base type should be extendable to uint64
};

struct SEntiyID
{
 uint32 SeqID : 8;  // Should be high bits (TODO: Assert in debug builds)
 uint32 Index : 24; // uint32: max 16777215
};

struct SEntityBase     // Each entity is placed in own size list when deleted (Mask is used as index of a next free entity) 
{
 uint32  Mask;     // Max 32 components for this type of entity(See type maps for which components available for which type). Masking is used to determine if a component is present // Popcnt is used to determine its index in the entity`s body 
 uint16  Flags;    // Specific to the Type (Some flags that require most frequent access)
 uint8   SeqID;    // To detect if the entity has been deleted (Same value is stored in high 8 bits of a Entity ID)
 uint8   Type;
};

struct SEntityDyn: SEntityBase     // Better if size of SEntityBase is 12 (64-bit mask) [16 bytes per Entity record and max 64 components per entity]
{
 uint32 CmptIdx;  // Index of the component list (first component)
};

// Store entire entity in a separate list or only its dynamic lists of components? At least we can get flags without additional indirection if only component list is relocatable
struct SEntityStt: SEntityBase   // TODO: static assert its size     // Size of SEntityBase is not important     // Better locality. Slower entity lookup if its size is not pow2
{
 SCmptID CmptID[0];  // No add/remove, created with the entity (Max 32 component IDs)
};

using SSEntity = TSF<1,SEntityDyn,SEntityStt>::T;


// NOTE: Need another level of indirection if we want to be able to Add/Remove components dynamically
// TODO: Components should be shareable until modified
//
// CreateEntity(uint8 Type, uint16 Flags, uint32 CmptMask)     // For dynamic entities mask will help to reserve memory initially
// CreateEntity<T>(uint16 Flags, uint32 CmptMask)      // The type is derived from T which is unique sequential ID of a type (Scripts have MaxTypeID constants exported to them and must register their types to map them)



};
