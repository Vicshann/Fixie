
struct Test_BitArray
{

// Test 1-bit array (typical bit array)
struct Test1BitArray
{
    static int DoTests()
    {
        CBitArr<1> arr;
        
        // Test initial state
        RT_TEST(arr.Count(), == 0, "Initial count should be 0")
        RT_TEST(arr.IsZero(), == true, "Empty array should be zero")
        
        // Test resizing
        RT_TEST(arr.Resize(64), >= sizeof(usize), "Should resize to one unit")
        RT_TEST(arr.Count(), == 64, "Count should be 64")
        RT_TEST(arr.IsZero(), == true, "New bits should be zero")
        
        // Test setting individual bits
        RT_TEST(arr.Set(0), == true, "Should set bit 0")
        RT_TEST(arr.Get(0), == 1, "Bit 0 should be 1")
        RT_TEST(arr.Get(1), == 0, "Bit 1 should be 0")
        RT_TEST(arr.IsZero(), == false, "Array should not be all zero")
        
        // Test setting last bit
        RT_TEST(arr.Set(63), == true, "Should set bit 63")
        RT_TEST(arr.Get(63), == 1, "Bit 63 should be 1")
        
        // Test clearing bits
        RT_TEST(arr.Clr(0), == true, "Should clear bit 0")
        RT_TEST(arr.Get(0), == 0, "Bit 0 should be 0")
        
        // Test setting all bits
        for(usize i = 0; i < 64; i++)
            arr.Set(i);
        RT_TEST(arr.IsOne(), == true, "All bits should be 1")
        
        // Test clearing all bits
        for(usize i = 0; i < 64; i++)
            arr.Clr(i);
        RT_TEST(arr.IsZero(), == true, "All bits should be 0")
        
        return 0;
    }
};

// Test edge cases with unit boundaries
struct TestUnitBoundaries
{
    static int DoTests()
    {
        CBitArr<1> arr;
        
        // Test at 64-bit boundary (assuming UT is uint64)
        RT_TEST(arr.Resize(128), >= sizeof(usize), "Should resize to 128")
        
        // Test bits around unit boundary
        RT_TEST(arr.Set(63), == true, "Should set last bit of first unit")
        RT_TEST(arr.Set(64), == true, "Should set first bit of second unit")
        RT_TEST(arr.Get(63), == 1, "Bit 63 should be 1")
        RT_TEST(arr.Get(64), == 1, "Bit 64 should be 1")
        RT_TEST(arr.Get(62), == 0, "Bit 62 should be 0")
        RT_TEST(arr.Get(65), == 0, "Bit 65 should be 0")
        
        // Test clearing at boundary
        RT_TEST(arr.Clr(63), == true, "Should clear bit 63")
        RT_TEST(arr.Get(63), == 0, "Bit 63 should be 0")
        RT_TEST(arr.Get(64), == 1, "Bit 64 should still be 1")
        
        return 0;
    }
};

// Test multi-bit items (2-bit, 4-bit, etc.)
struct TestMultiBitItems
{
    static int DoTests()
    {
        // Test 2-bit items
        {
            CBitArr<2> arr;
            RT_TEST(arr.Resize(32), >= sizeof(usize), "Should resize to 32 items")
            
            RT_TEST(arr.Put(0b00, 0), == true, "Should put 0")
            RT_TEST(arr.Put(0b01, 1), == true, "Should put 1")
            RT_TEST(arr.Put(0b10, 2), == true, "Should put 2")
            RT_TEST(arr.Put(0b11, 3), == true, "Should put 3")
            
            RT_TEST(arr.Get(0), == 0b00, "Item 0 should be 0")
            RT_TEST(arr.Get(1), == 0b01, "Item 1 should be 1")
            RT_TEST(arr.Get(2), == 0b10, "Item 2 should be 2")
            RT_TEST(arr.Get(3), == 0b11, "Item 3 should be 3")
        }
        
        // Test 4-bit items (nibbles)
        {
            CBitArr<4> arr;
            RT_TEST(arr.Resize(16), >= sizeof(usize), "Should resize to 16 items")
            
            RT_TEST(arr.Put(0xF, 0), == true, "Should put 0xF")
            RT_TEST(arr.Put(0xA, 1), == true, "Should put 0xA")
            RT_TEST(arr.Put(0x5, 2), == true, "Should put 0x5")
            RT_TEST(arr.Put(0x0, 3), == true, "Should put 0x0")
            
            RT_TEST(arr.Get(0), == 0xF, "Item 0 should be 0xF")
            RT_TEST(arr.Get(1), == 0xA, "Item 1 should be 0xA")
            RT_TEST(arr.Get(2), == 0x5, "Item 2 should be 0x5")
            RT_TEST(arr.Get(3), == 0x0, "Item 3 should be 0x0")
        }
        
        // Test 8-bit items (bytes)
        {
            CBitArr<8> arr;
            RT_TEST(arr.Resize(8), >= sizeof(usize), "Should resize to 8 items")
            
            RT_TEST(arr.Put(0xFF, 0), == true, "Should put 0xFF")
            RT_TEST(arr.Put(0xAA, 1), == true, "Should put 0xAA")
            RT_TEST(arr.Put(0x55, 2), == true, "Should put 0x55")
            RT_TEST(arr.Put(0x00, 3), == true, "Should put 0x00")
            
            RT_TEST(arr.Get(0), == 0xFF, "Item 0 should be 0xFF")
            RT_TEST(arr.Get(1), == 0xAA, "Item 1 should be 0xAA")
            RT_TEST(arr.Get(2), == 0x55, "Item 2 should be 0x55")
            RT_TEST(arr.Get(3), == 0x00, "Item 3 should be 0x00")
        }
        
        return 0;
    }
};

// Test grow/shrink operations
struct TestGrowShrink
{
    static int DoTests()
    {
        CBitArr<1> arr;
        
        // Test growing
        RT_TEST(arr.Resize(10), >= sizeof(usize), "Should resize to 10")
        for(usize i = 0; i < 10; i++)
            arr.Set(i);
        
        RT_TEST(arr.Resize(20), >= sizeof(usize), "Should grow to 20")
        RT_TEST(arr.Count(), == 20, "Count should be 20")
        
        // Original bits should remain
        for(usize i = 0; i < 10; i++) 
            RT_TEST(arr.Get(i), == 1, "Original bits should be preserved")
        
        // Test shrinking
        RT_TEST(arr.Resize(5), == 5, "Should shrink to 5")
        RT_TEST(arr.Count(), == 5, "Count should be 5")
        
        for(usize i = 0; i < 5; i++)
            RT_TEST(arr.Get(i), == 1, "Remaining bits should be preserved")
        
        // Test growing back
        RT_TEST(arr.Resize(15), >= sizeof(usize), "Should grow to 15")
        for(usize i = 0; i < 5; i++)
            RT_TEST(arr.Get(i), == 1, "Original bits should still be preserved")
        
        // Test Grow() method
        RT_TEST(arr.Grow(5), == 20, "Should grow by 5")
        RT_TEST(arr.Count(), == 20, "Count should be 20")
        
        RT_TEST(arr.Grow(-10), == 10, "Should shrink by 10")
        RT_TEST(arr.Count(), == 10, "Count should be 10")
        
        // Test shrinking to empty
        RT_TEST(arr.Grow(-100), == 0, "Should clear when shrinking too much")
        RT_TEST(arr.Count(), == 0, "Count should be 0")
        
        return 0;
    }
};

// Test Add() function
struct TestAdd
{
    static int DoTests()
    {
        CBitArr<4> arr;
        
        // Add items
        RT_TEST(arr.Add(0x1), == true, "Should add 0x1")
        RT_TEST(arr.Count(), == 1, "Count should be 1")
        RT_TEST(arr.Get(0), == 0x1, "Item 0 should be 0x1")
        
        RT_TEST(arr.Add(0x2), == true, "Should add 0x2")
        RT_TEST(arr.Count(), == 2, "Count should be 2")
        RT_TEST(arr.Get(1), == 0x2, "Item 1 should be 0x2")
        
        RT_TEST(arr.Add(0xF), == true, "Should add 0xF")
        RT_TEST(arr.Count(), == 3, "Count should be 3")
        RT_TEST(arr.Get(2), == 0xF, "Item 2 should be 0xF")
        
        // Add many items to test unit boundary crossing
        for(usize i = 0; i < 20; i++)
            RT_TEST(arr.Add(i & 0xF), == true, "Should add item")
        
        RT_TEST(arr.Count(), == 23, "Count should be 23")
        
        // Verify all items
        RT_TEST(arr.Get(0), == 0x1, "Item 0 should be 0x1")
        RT_TEST(arr.Get(1), == 0x2, "Item 1 should be 0x2")
        RT_TEST(arr.Get(2), == 0xF, "Item 2 should be 0xF")
        
        return 0;
    }
};

// Test Find() function
struct TestFind
{
    static int DoTests()
    {
        CBitArr<4> arr;
        
        // Add test data
        arr.Add(0x1);
        arr.Add(0x2);
        arr.Add(0x3);
        arr.Add(0x2);
        arr.Add(0x5);
        
        // Test finding existing values
      /*  RT_TEST(arr.Find(0x1), == 0, "Should find 0x1 at index 0")
        RT_TEST(arr.Find(0x2), == 1, "Should find first 0x2 at index 1")
        RT_TEST(arr.Find(0x3), == 2, "Should find 0x3 at index 2")
        RT_TEST(arr.Find(0x5), == 4, "Should find 0x5 at index 4")
        
        // Test finding from offset
        RT_TEST(arr.Find(0x2, 2), == 3, "Should find second 0x2 at index 3")
        
        // Test not found
        RT_TEST(arr.Find(0xF), == -1, "Should not find 0xF")
        RT_TEST(arr.Find(0x1, 1), == -1, "Should not find 0x1 after index 0")
             */
        return 0;
    }
};

// Test Clear() function
struct TestClear
{
    static int DoTests()
    {
        CBitArr<1> arr;
        
        arr.Resize(100);
        for(usize i = 0; i < 100; i++)
            arr.Set(i);
        
        RT_TEST(arr.Count(), == 100, "Count should be 100")
        RT_TEST(arr.IsOne(), == true, "All bits should be 1")
        
        arr.Clear();
        
        RT_TEST(arr.Count(), == 0, "Count should be 0 after clear")
        RT_TEST(arr.IsZero(), == true, "Should be zero after clear")
        
        return 0;
    }
};

// Test range Set() and Clr() functions
struct TestRangeOperations
{
    static int DoTests()
    {
        CBitArr<1> arr;
        arr.Resize(128);
        
        // Test setting a range in single unit
        arr.Set(10, 5);  // Set bits 10-14
        for(usize i = 10; i < 15; i++)
            RT_TEST(arr.Get(i), == 1, "Bits in range should be set")
        RT_TEST(arr.Get(9), == 0, "Bit before range should be clear")
        RT_TEST(arr.Get(15), == 0, "Bit after range should be clear")
        
        // Test clearing a range in single unit
        arr.Clr(11, 2);  // Clear bits 11-12
        RT_TEST(arr.Get(10), == 1, "Bit 10 should still be set")
        RT_TEST(arr.Get(11), == 0, "Bit 11 should be clear")
        RT_TEST(arr.Get(12), == 0, "Bit 12 should be clear")
        RT_TEST(arr.Get(13), == 1, "Bit 13 should still be set")
        
        // Test setting range across unit boundary
        arr.Clear();
        arr.Resize(128);
        arr.Set(60, 10);  // Set bits 60-69 (crosses 64-bit boundary)
        for(usize i = 60; i < 70; i++)
            RT_TEST(arr.Get(i), == 1, "Bits across boundary should be set")
        RT_TEST(arr.Get(59), == 0, "Bit before range should be clear")
        RT_TEST(arr.Get(70), == 0, "Bit after range should be clear")
        
        // Test clearing range across unit boundary
        arr.Clr(62, 6);  // Clear bits 62-67
        RT_TEST(arr.Get(60), == 1, "Bit 60 should still be set")
        RT_TEST(arr.Get(61), == 1, "Bit 61 should still be set")
        for(usize i = 62; i < 68; i++)
            RT_TEST(arr.Get(i), == 0, "Bits in range should be clear")
        RT_TEST(arr.Get(68), == 1, "Bit 68 should still be set")
        RT_TEST(arr.Get(69), == 1, "Bit 69 should still be set")
        
        // Test setting entire units
        arr.Clear();
        arr.Resize(128);
        arr.Set(0, 128);  // Set all bits
        RT_TEST(arr.IsOne(), == true, "All bits should be set")
        
        // Test clearing entire units
        arr.Clr(0, 128);  // Clear all bits
        RT_TEST(arr.IsZero(), == true, "All bits should be clear")
        
        // Test empty range
        arr.Set(10, 0);  // Should do nothing
        RT_TEST(arr.IsZero(), == true, "Empty range should not change array")
        
        return 0;
    }
};

// Test IsZero() and IsOne() edge cases
struct TestIsZeroOne
{
    static int DoTests()
    {
        CBitArr<1> arr;
        
        // Test empty array
        RT_TEST(arr.IsZero(), == true, "Empty array should be zero")
        RT_TEST(arr.IsOne(), == false, "Empty array should not be all ones")
        
        // Test single bit
        arr.Resize(1);
        RT_TEST(arr.IsZero(), == true, "Single bit 0 should be zero")
        arr.Set(0);
        RT_TEST(arr.IsOne(), == true, "Single bit 1 should be all ones")
        
        // Test partial unit
        arr.Clear();
        arr.Resize(10);
        RT_TEST(arr.IsZero(), == true, "Partial unit all 0 should be zero")
        for(usize i = 0; i < 10; i++)
            arr.Set(i);
        RT_TEST(arr.IsOne(), == true, "Partial unit all 1 should be all ones")
        
        // Test exactly one unit
        arr.Clear();
        arr.Resize(64);
        RT_TEST(arr.IsZero(), == true, "Full unit all 0 should be zero")
        for(usize i = 0; i < 64; i++)
            arr.Set(i);
        RT_TEST(arr.IsOne(), == true, "Full unit all 1 should be all ones")
        
        // Test multiple units
        arr.Clear();
        arr.Resize(130);
        RT_TEST(arr.IsZero(), == true, "Multiple units all 0 should be zero")
        for(usize i = 0; i < 130; i++)
            arr.Set(i);
        RT_TEST(arr.IsOne(), == true, "Multiple units all 1 should be all ones")
        
        // Test with one bit wrong
        arr.Clr(65);
        RT_TEST(arr.IsOne(), == false, "One bit clear should not be all ones")
        RT_TEST(arr.IsZero(), == false, "One bit clear should not be all zeros")
        
        return 0;
    }
};

// Test non-power-of-2 bit sizes
struct TestNonPow2BitSizes
{
    static int DoTests()
    {
        // Test 3-bit items
        {
            CBitArr<3> arr;
            RT_TEST(arr.Resize(21), == 21, "Should resize to 21 items")
            
            RT_TEST(arr.Put(0b000, 0), == true, "Should put 0")
            RT_TEST(arr.Put(0b111, 1), == true, "Should put 7")
            RT_TEST(arr.Put(0b101, 2), == true, "Should put 5")
            RT_TEST(arr.Put(0b010, 3), == true, "Should put 2")
            
            RT_TEST(arr.Get(0), == 0b000, "Item 0 should be 0")
            RT_TEST(arr.Get(1), == 0b111, "Item 1 should be 7")
            RT_TEST(arr.Get(2), == 0b101, "Item 2 should be 5")
            RT_TEST(arr.Get(3), == 0b010, "Item 3 should be 2")
        }
        
        // Test 5-bit items
        {
            CBitArr<5> arr;
            RT_TEST(arr.Resize(12), == 12, "Should resize to 12 items")
            
            RT_TEST(arr.Put(0b11111, 0), == true, "Should put 31")
            RT_TEST(arr.Put(0b10101, 1), == true, "Should put 21")
            RT_TEST(arr.Put(0b00000, 2), == true, "Should put 0")
            
            RT_TEST(arr.Get(0), == 0b11111, "Item 0 should be 31")
            RT_TEST(arr.Get(1), == 0b10101, "Item 1 should be 21")
            RT_TEST(arr.Get(2), == 0b00000, "Item 2 should be 0")
        }
        
        return 0;
    }
};

// Test with AutoGrow disabled
struct TestNoAutoGrow
{
    static int DoTests()
    {
        CBitArr<1, true, true, false> arr;  // AutoGrow = false
        
        // Initially empty
        RT_TEST(arr.Count(), == 0, "Initial count should be 0")
        
        // Should not auto-grow on Get
        RT_TEST(arr.Get(10), == 0, "Get beyond bounds should return 0")
        RT_TEST(arr.Count(), == 0, "Count should still be 0")
        
        // Should not auto-grow on Set
        RT_TEST(arr.Set(10), == false, "Set beyond bounds should fail")
        RT_TEST(arr.Count(), == 0, "Count should still be 0")
        
        // Manual resize should work
        RT_TEST(arr.Resize(20), == 20, "Manual resize should work")
        RT_TEST(arr.Count(), == 20, "Count should be 20")
        
        // Now operations within bounds should work
        RT_TEST(arr.Set(10), == true, "Set within bounds should work")
        RT_TEST(arr.Get(10), == 1, "Get within bounds should work")
        
        return 0;
    }
};

// Test stress cases
struct TestStress
{
    static int DoTests()
    {
        // Test many items
        CBitArr<1> arr;
        const usize NumItems = 10000;
        
        arr.Resize(NumItems);
        
        // Set every other bit
        for(usize i = 0; i < NumItems; i += 2)
            arr.Set(i);
        
        // Verify pattern
        for(usize i = 0; i < NumItems; i++)
            RT_TEST(arr.Get(i), == (i % 2 == 0 ? 1 : 0), "Pattern should be correct")
        
        // Clear all
        for(usize i = 0; i < NumItems; i++)
            arr.Clr(i);
        
        RT_TEST(arr.IsZero(), == true, "Should be all zeros")
        
        return 0;
    }
};
//------------------------------------------------------------------------------------
static int DoTests(void)
{
 LOGMSG("Begin: BitArray");

 DO_TEST(Test1BitArray)
 DO_TEST(TestUnitBoundaries)
 DO_TEST(TestMultiBitItems)
 DO_TEST(TestGrowShrink)
 DO_TEST(TestAdd)
 DO_TEST(TestFind)
 DO_TEST(TestClear)
 DO_TEST(TestRangeOperations)    // Broken
 DO_TEST(TestIsZeroOne)
 DO_TEST(TestNonPow2BitSizes)
 DO_TEST(TestNoAutoGrow)
 DO_TEST(TestStress)

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 LOGMSG("Begin: BitArray");

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
};
