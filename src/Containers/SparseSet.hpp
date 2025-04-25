
//============================================================================================================
// Each bit position in bitmask belongs to an index
// When we add or remove an element, we have to relocate elements after it ( No multithreading:( )
//------------------------------------------------------------------------------------------------------------
ChunkDesc:{
  usize Mask   // Covers max 32/64 sparse chunks of units
  usize Offs;  // Index of a first element the mask refers to
}
// This list will grow as needed
// Each Insert/Delete Moves the data to appropriate size bucket (1-32/64 units), inserting the element and updating the BitMask and offset

template<> CSparseSet
{

};
//============================================================================================================