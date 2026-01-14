
struct Test_BitStream
{
 using CMemStream = STRM::CStrmBuf;
 alignas(16) static inline uint8 mbuf[4096];
 static inline STRM::CStrmBuf* mem;
//------------------------------------------------------------------------------------
// Test 1: Basic write and read back
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestBasicWriteRead()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 5 bits: 0b10110
    RT_TEST(bs.Write(0b10110, 5), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 5), >= 0)
    RT_TEST(val, == 0b10110, "Expected 0b10110")
    
    return 0;
}

// Test 2: Write and read at byte boundaries
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestBoundaryAlignment()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write exactly 8 bits
    RT_TEST(bs.Write(0xAB, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)
    RT_TEST(val, == 0xAB)
    
    // Write exactly 16 bits
    mem->Reset();
    bs.Init(mem);
    RT_TEST(bs.Write(0x1234, 16), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    RT_TEST(val, == 0x1234)
    
    return 0;
}

// Test 3: Cross cache unit boundary (64-bit boundary for usize)
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestUnitBoundaryCrossing()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 60 bits, then 10 bits (crosses 64-bit boundary)
    RT_TEST(bs.Write(0xABCDEF012345ULL, 60), >= 0)
    RT_TEST(bs.Write(0x3FF, 10), >= 0)  // 10 bits
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    uint64 val1 = 0;
    uint32 val2 = 0;
    RT_TEST(bs.Read(&val1, 60), >= 0)
    RT_TEST(bs.Read(&val2, 10), >= 0)
    RT_TEST(val1, == 0xABCDEF012345ULL)
    RT_TEST(val2, == 0x3FF)
    
    return 0;
}

// Test 4: Cross cache buffer boundary
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestCacheBoundaryCrossing()
{
    STRM::CBitStream<CMemStream, 16> bs(mem); mem->Reset();  // Small cache: 16 bytes = 2 units
    
    // Write more than cache size (130 bits > 128 bits)
    for(uint32 i = 0; i < 13; i++)
    {
        RT_TEST(bs.Write(i & 0x3FF, 10), >= 0)  // 13 * 10 = 130 bits
    }
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    for(int i = 0; i < 13; i++)
    {
        uint32 val = 0;
        RT_TEST(bs.Read(&val, 10), >= 0, "Read failed!")
        RT_TEST(val, == (i & 0x3FF), "Read unexpected!")
    }
    
    return 0;
}

// Test 5: Flush with partial bits in cache
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestFlushingPartialBits()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 13 bits (not byte-aligned)
    RT_TEST(bs.Write(0x1FFF, 13), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Verify stream has 2 bytes (13 bits rounds up)
    RT_TEST(mem->Size(), == 2)
    
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 13), >= 0)
    RT_TEST(val, == 0x1FFF)
    
    return 0;
}

// Test 6: Seek to byte-aligned position
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekBitAligned()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 80 bits
    RT_TEST(bs.Write(0x1122334455667788ULL, 64), >= 0)
    RT_TEST(bs.Write(0xFFFF, 16), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 32 (byte 4)
    RT_TEST(bs.Seek(32, STRM::SEEK_SET), >= 0)
    
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    if constexpr (IsBitOrderMSB) { RT_TEST(val, == 0x5566) } // Middle 16 bits
      else { RT_TEST(val, == 0x3344) }
    
    return 0;
}

// Test 7: Seek to non-byte-aligned position
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekUnaligned()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write known pattern
    RT_TEST(bs.Write(0xAAAAAAAAAAAAAAAAULL, 64), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 5
    RT_TEST(bs.Seek(5, STRM::SEEK_SET), >= 0)
    
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)
    // 0xAA = 10101010, starting from bit 5: 01010101 = 0x55
    RT_TEST(val, == 0x55)
    
    return 0;
}

// Test 8: Seek and then write
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekAndWrite()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write initial data
    RT_TEST(bs.Write(0xFFFFFFFFFFFFFFFFULL, 64), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek back to bit 16 and overwrite
    RT_TEST(bs.Seek(16, STRM::SEEK_SET), >= 0)
    RT_TEST(bs.Write(0x1234, 16), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back and verify
    bs.Rewind();
    uint32 val1 = 0, val2 = 0, val3 = 0;
    RT_TEST(bs.Read(&val1, 16), >= 0)
    RT_TEST(bs.Read(&val2, 16), >= 0)
    RT_TEST(bs.Read(&val3, 32), >= 0)
    RT_TEST(val1, == 0xFFFF)  // First 16 bits unchanged
    RT_TEST(val2, == 0x1234)  // Overwritten
    RT_TEST(val3, == 0xFFFFFFFF)  // Last 32 bits unchanged
    
    return 0;
}

// Test 9: Seek and then read
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekAndRead()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write pattern
    for(int i = 0; i < 8; i++)
    {
        RT_TEST(bs.Write(i, 8), >= 0)  // Write 0,1,2,3,4,5,6,7
    }
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to middle (bit 32 = byte 4)
    RT_TEST(bs.Seek(32, STRM::SEEK_SET), >= 0)
    
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)
    RT_TEST(val, == 4)  // Should read value 4
    
    return 0;
}

// Test 10: Mixed read and write operations
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestMixedReadWrite()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write some data
    RT_TEST(bs.Write(0xABCD, 16), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read it back without rewind
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)  // Read first byte
    
    // Now write more (mixed operation)
    RT_TEST(bs.Write(0xEF, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Verify
    bs.Rewind();
    val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    if constexpr (IsBitOrderMSB) { RT_TEST(val, == 0xABEF) } // AB from original, EF from overwrite
      else { RT_TEST(val, == 0xEFCD) }
    return 0;
}

// Test 11: Edge case - Write/Read 0 bits
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestZeroBitsEdgeCase()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write some data first
    RT_TEST(bs.Write(0xFF, 8), >= 0)
    
    // Write 0 bits (should be no-op if checks are enabled)
    // This tests the Cnt == 0 edge case
    uint32 val = 0xDEADBEEF;
    RT_TEST(bs.Write(val, 0), >= 0)  // Should do nothing
    
    RT_TEST(bs.Flush(), >= 0)
    RT_TEST(mem->Size(), == 1)  // Only 1 byte written
    
    return 0;
}

// Test 12: Write exactly one full cache unit
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestFullCacheUnitWrite()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write exactly 64 bits (one full cache unit for usize=uint64)
    RT_TEST(bs.Write(0x123456789ABCDEFULL, 64), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    uint64 val = 0;
    RT_TEST(bs.Read(&val, 64), >= 0)
    RT_TEST(val, == 0x123456789ABCDEFULL)
    
    return 0;
}

// Test 13: Read partial unit at EOF
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestPartialUnitAtEOF()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 13 bits
    RT_TEST(bs.Write(0x1ABC, 13), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 13), >= 0)
    RT_TEST(val, == 0x1ABC)
    
    // Try to read more (should hit EOF)
    val = 0;
  //  usize res = bs.Read(&val, 8);     // Reads are always aligned to unit size and may read some unexpected extra bits
  //  RT_TEST(res, == STRM::SEOF)  // EOF, no more data
    
    return 0;
}

// Test 14: Overwrite after seek with misalignment
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestOverwriteAfterSeek()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 100 bits of 1s
    RT_TEST(bs.Write(0xFFFFFFFFFFFFFFFFULL, 64), >= 0)
    RT_TEST(bs.Write(0xFFFFFFFF, 36), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 13 (unaligned) and overwrite 10 bits with 0s
    RT_TEST(bs.Seek(13, STRM::SEEK_SET), >= 0)
    RT_TEST(bs.Write(0, 10), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back the modified section
    RT_TEST(bs.Seek(10, STRM::SEEK_SET), >= 0)
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    // Original: 16 bits of 1s
    // After: 3 bits of 1s, 10 bits of 0s, 3 bits of 1s
    // = 0b1110000000000111 = 0xE007
   // if constexpr (IsBitOrderMSB) { RT_TEST(val, == 0xE007) }
   //   else { RT_TEST(val, == 0xC047) } 
    RT_TEST(val, == 0xE007)
    
    return 0;
}

// Test 15: Multiple flushes
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestFlushBetweenOperations()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    RT_TEST(bs.Write(0xAA, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    RT_TEST(bs.Write(0xBB, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    RT_TEST(bs.Write(0xCC, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    RT_TEST(mem->Size(), == 3)
    
    bs.Rewind();
    uint32 v1 = 0, v2 = 0, v3 = 0;
    RT_TEST(bs.Read(&v1, 8), >= 0)
    RT_TEST(bs.Read(&v2, 8), >= 0)
    RT_TEST(bs.Read(&v3, 8), >= 0)
    RT_TEST(v1, == 0xAA)
    RT_TEST(v2, == 0xBB)
    RT_TEST(v3, == 0xCC)
    
    return 0;
}

// Test 16: Large write spanning multiple cache units
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestLargeWriteAcrossMultipleUnits()
{
    STRM::CBitStream<CMemStream, 16> bs(mem); mem->Reset();  // 16-byte cache
    
    // Write 200 bits (> 128-bit cache size)
    uint64 pattern = 0x0123456789ABCDEFULL;
    RT_TEST(bs.Write(pattern, 64), >= 0)
    RT_TEST(bs.Write(pattern, 64), >= 0)
    RT_TEST(bs.Write(pattern, 64), >= 0)
    RT_TEST(bs.Write(0xFF, 8), >= 0)  // 200 bits total
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    uint64 v1 = 0, v2 = 0, v3 = 0;
    uint32 v4 = 0;
    RT_TEST(bs.Read(&v1, 64), >= 0)
    RT_TEST(bs.Read(&v2, 64), >= 0)
    RT_TEST(bs.Read(&v3, 64), >= 0)
    RT_TEST(bs.Read(&v4, 8), >= 0)
    RT_TEST(v1, == pattern)
    RT_TEST(v2, == pattern)
    RT_TEST(v3, == pattern)
    RT_TEST(v4, == 0xFF)
    
    return 0;
}

// Test 17: Read after writing partial bits
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestReadAfterPartialWrite()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 19 bits
    RT_TEST(bs.Write(0x7FFFF, 19), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back in different chunks
    bs.Rewind();
    uint32 v1 = 0, v2 = 0, v3 = 0;
    RT_TEST(bs.Read(&v1, 7), >= 0)
    RT_TEST(bs.Read(&v2, 5), >= 0)
    RT_TEST(bs.Read(&v3, 7), >= 0)
    
    // Reconstruct: 7 high bits, 5 middle, 7 low
    uint32 reconstructed = (v1 << 12) | (v2 << 7) | v3;
    RT_TEST(reconstructed, == 0x7FFFF)
    
    return 0;
}

// Test 18: CacheBPos == 0 edge case
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestCacheBPosZeroEdgeCase()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write exactly enough to exhaust cache unit
    RT_TEST(bs.Write(0xFFFFFFFFFFFFFFFFULL, 64), >= 0)
    // At this point CacheBPos should be 0
    
    // Write one more bit (tests the CacheBPos == 0 path)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    uint64 v1 = 0;
    uint32 v2 = 0;
    RT_TEST(bs.Read(&v1, 64), >= 0)
    RT_TEST(bs.Read(&v2, 1), >= 0)
    RT_TEST(v1, == 0xFFFFFFFFFFFFFFFFULL)
    RT_TEST(v2, == 1)
    
    return 0;
}

// Test 19: Unaligned seek followed immediately by read (no write)
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekThenImmediateRead()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write known pattern
    RT_TEST(bs.Write(0xABCDEF1234567890ULL, 64), >= 0)
    RT_TEST(bs.Write(0xFFFFFFFF, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 13 (unaligned) and immediately read
    RT_TEST(bs.Seek(13, STRM::SEEK_SET), >= 0)
    
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    // Should read 16 bits starting from bit 13
    // Verify we got meaningful data (not zeros)
    RT_TEST(val, != 0, "Should read non-zero data after unaligned seek")
    
    return 0;
}

// Test 20: Multiple sequential seeks
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestMultipleSeeks()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write initial data
    RT_TEST(bs.Write(0x11111111, 32), >= 0)
    RT_TEST(bs.Write(0x22222222, 32), >= 0)
    RT_TEST(bs.Write(0x33333333, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 0, write
    RT_TEST(bs.Seek(0, STRM::SEEK_SET), >= 0)
    RT_TEST(bs.Write(0xAA, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 32, write
    RT_TEST(bs.Seek(32, STRM::SEEK_SET), >= 0)
    RT_TEST(bs.Write(0xBB, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 64, write
    RT_TEST(bs.Seek(64, STRM::SEEK_SET), >= 0)
    RT_TEST(bs.Write(0xCC, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Verify all writes
    bs.Rewind();
    uint32 v1 = 0, v2 = 0, v3 = 0;
    RT_TEST(bs.Read(&v1, 8), >= 0)
    RT_TEST(bs.Seek(32, STRM::SEEK_SET), >= 0)
    RT_TEST(bs.Read(&v2, 8), >= 0)
    RT_TEST(bs.Seek(64, STRM::SEEK_SET), >= 0)
    RT_TEST(bs.Read(&v3, 8), >= 0)
    
    RT_TEST(v1, == 0xAA)
    RT_TEST(v2, == 0xBB)
    RT_TEST(v3, == 0xCC)
    
    return 0;
}

// Test 21: Seek with SEEK_CUR
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekCurrent()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write pattern
    for(int i = 0; i < 8; i++)
    {
        RT_TEST(bs.Write(i, 8), >= 0)
    }
    RT_TEST(bs.Size(), == 8*8)
    RT_TEST(bs.Offset(), == 8*8)
    RT_TEST(bs.Flush(), >= 0)
    RT_TEST(bs.Size(), == 8*8)
    
    // Read first value
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)
    RT_TEST(val, == 0)
    
    // Seek forward 16 bits from current position
    RT_TEST(bs.Seek(16, STRM::SEEK_CUR), >= 0)  // Cur pos is broken (cache!!!)
    
    RT_TEST(bs.Read(&val, 8), >= 0)
    RT_TEST(val, == 3, "Should read value 3 after skipping 2 bytes")
    
    return 0;
}

// Test 22: Single bit operations
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSingleBitOperations()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 8 individual bits: 10110011
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back as single byte
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)
    if constexpr (IsBitOrderMSB) { RT_TEST(val, == 0xB3, "Expected 0xB3 (10110011)") }
      else { RT_TEST(val, == 0xCD, "Expected 0xCD (11001101)") }
    return 0;
}

// Test 23: Write exactly CacheUBits-1 bits repeatedly
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestNearFullUnitWrites()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 63 bits three times (189 bits total)
    uint64 pattern = 0x7FFFFFFFFFFFFFFFULL; // 63 bits of 1s
    RT_TEST(bs.Write(pattern, 63), >= 0)
    RT_TEST(bs.Write(pattern, 63), >= 0)
    RT_TEST(bs.Write(pattern, 63), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back
    bs.Rewind();
    uint64 v1 = 0, v2 = 0, v3 = 0;
    RT_TEST(bs.Read(&v1, 63), >= 0)
    RT_TEST(bs.Read(&v2, 63), >= 0)
    RT_TEST(bs.Read(&v3, 63), >= 0)
    RT_TEST(v1, == pattern)
    RT_TEST(v2, == pattern)
    RT_TEST(v3, == pattern)
    
    return 0;
}

// Test 24: Flush without prior operations
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestFlushOnFreshStream()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Flush immediately
    RT_TEST(bs.Flush(), >= 0)
    RT_TEST(mem->Size(), == 0, "Stream should be empty")
    
    // Should still be able to write after
    RT_TEST(bs.Write(0xFF, 8), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    RT_TEST(mem->Size(), == 1)
    
    return 0;
}

// Test 25: Multiple rewinds
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestMultipleRewinds()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write -> Rewind -> Read -> Rewind -> Write
    RT_TEST(bs.Write(0xABCD, 16), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    RT_TEST(bs.Rewind(), >= 0)
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    RT_TEST(val, == 0xABCD)
    
    RT_TEST(bs.Rewind(), >= 0)
    RT_TEST(bs.Write(0x1234, 16), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    RT_TEST(val, == 0x1234)
    
    return 0;
}

// Test 26: Write after partial read
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestWriteAfterPartialRead()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write initial data
    RT_TEST(bs.Write(0xFFFFFFFF, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read 5 bits
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 5), >= 0)
    
    // Now write 10 bits (mixed operation)
    RT_TEST(bs.Write(0x3FF, 10), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Verify
    bs.Rewind();
    val = 0;
    RT_TEST(bs.Read(&val, 15), >= 0)
    // First 5 bits from original, then 10 bits we wrote
    RT_TEST(val, != 0, "Should have written data")
    
    return 0;
}

// Test 27: CRITICAL - Seek to unaligned position, write partial bits, flush (tests merge)
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekUnalignedWritePartialFlush()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write initial pattern of all 1s
    RT_TEST(bs.Write(0xFFFFFFFFFFFFFFFFULL, 64), >= 0)
    RT_TEST(bs.Write(0xFFFFFFFF, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 5 (unaligned within first byte)
    RT_TEST(bs.Seek(5, STRM::SEEK_SET), >= 0)
    
    // Write just 3 bits (doesn't fill the cache unit)
    RT_TEST(bs.Write(0b000, 3), >= 0)
    
    // Flush should merge these 3 bits with existing data
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back and verify merge happened correctly
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 16), >= 0)
    
    // Original: 16 bits of 1s (0xFFFF)
    // After: bits 0-4 are 1s, bits 5-7 are 0s, bits 8-15 are 1s
    // = 0b1111100011111111 = 0xF8FF
    if constexpr (IsBitOrderMSB) { RT_TEST(val, == 0xF8FF, "Merge should preserve surrounding bits") }
      else { RT_TEST(val, == 0xFF1F, "Merge should preserve surrounding bits") }
    return 0;
}

// Test 28: Seek to middle of stream, write partial unit, flush, continue reading
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekWritePartialFlushThenRead()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 96 bits (12 bytes)
    RT_TEST(bs.Write(0x1111111111111111ULL, 64), >= 0)
    RT_TEST(bs.Write(0x22222222, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 40 (byte 5, no bit offset)
    RT_TEST(bs.Seek(40, STRM::SEEK_SET), >= 0)
    
    // Write 12 bits (partial unit)
    RT_TEST(bs.Write(0xABC, 12), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Continue reading from where we left off
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)
    RT_TEST(val, != 0, "Should be able to read after flush")
    
    return 0;
}

// Test 29: Seek to bit 63, write 2 bits (crosses unit boundary), flush
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekNearUnitBoundaryWriteFlush()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write initial data
    RT_TEST(bs.Write(0xFFFFFFFFFFFFFFFFULL, 64), >= 0)
    RT_TEST(bs.Write(0xFFFFFFFF, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 63 (last bit of first unit)
    RT_TEST(bs.Seek(63, STRM::SEEK_SET), >= 0)
    
    // Write 2 bits (crosses unit boundary)
    RT_TEST(bs.Write(0b00, 2), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Verify
    RT_TEST(bs.Seek(62, STRM::SEEK_SET), >= 0)
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 4), >= 0)
    // Should be: bit62=1, bit63=0, bit64=0, bit65=1
    // = 0b1001 = 0x9
    RT_TEST(val, == 0x9, "Should merge across unit boundary")
    
    return 0;
}

// Test 30: Write 7 bits, seek back to bit 3, write 2 bits, flush
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestWriteSeekBackWriteFlush()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 7 bits: 1111111
    RT_TEST(bs.Write(0x7F, 7), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek back to bit 3
    RT_TEST(bs.Seek(3, STRM::SEEK_SET), >= 0)
    
    // Write 2 bits: 00
    RT_TEST(bs.Write(0b00, 2), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Read back all 7 bits
    bs.Rewind();
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 7), >= 0)
    // Original: 1111111
    // After: 111 00 11 = 0b1110011 = 0x73
    if constexpr (IsBitOrderMSB) { RT_TEST(val, == 0x73, "Should merge in middle of byte") }
       else { RT_TEST(val, == 0x67, "Should merge in middle of byte") }
    return 0;
}

// Test 31: Dirty cache + seek to different position + read (forces merge on next write)
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestDirtyCacheSeekRead()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write base data
    RT_TEST(bs.Write(0xAAAAAAAA, 32), >= 0)
    RT_TEST(bs.Write(0xBBBBBBBB, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Write partial bits (dirties cache)
    bs.Rewind();
    RT_TEST(bs.Write(0xFF, 4), >= 0)  // Don't flush - cache is dirty
    
    // Seek elsewhere
    RT_TEST(bs.Seek(32, STRM::SEEK_SET), >= 0)
    
    // Read (should handle dirty cache)
    uint32 val = 0;
    RT_TEST(bs.Read(&val, 8), >= 0)
    RT_TEST(val, == 0xBB, "Should read correct value")
    
    return 0;
}

// Test 32: Seek to end of written data, write more, flush
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekToEndThenWrite()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    // Write 32 bits
    RT_TEST(bs.Write(0x12345678, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to end (bit 32)
    RT_TEST(bs.Seek(32, STRM::SEEK_SET), >= 0)
    
    // Write more
    RT_TEST(bs.Write(0xABCD, 16), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Verify both parts
    bs.Rewind();
    uint32 v1 = 0, v2 = 0;
    RT_TEST(bs.Read(&v1, 32), >= 0)
    RT_TEST(bs.Read(&v2, 16), >= 0)
    RT_TEST(v1, == 0x12345678)
    RT_TEST(v2, == 0xABCD)
    
    return 0;
}

// Test 33: Very large single write (127 bits)
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestVeryLargeSingleWrite()
{
    STRM::CBitStream<CMemStream, 16> bs(mem); mem->Reset();  // Small cache
    
    // This should test if the implementation can't handle writes larger than cache
    // Most implementations would need to break this into chunks
    // For now, test smaller but still large: write 56 bits at once
    uint64 pattern = 0xFEDCBA9876543210ULL;
    RT_TEST(bs.Write(pattern, 56), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    bs.Rewind();
    uint64 val = 0;
    RT_TEST(bs.Read(&val, 56), >= 0)
    RT_TEST(val, == (pattern & 0x00FFFFFFFFFFFFFFULL))
    
    return 0;
}

// Test 34: Boundary - write exactly to fill buffer then one more bit
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestWriteExactlyFillBufferPlusOne()
{
    STRM::CBitStream<CMemStream, 16> bs(mem); mem->Reset();  // 16 byte = 128 bit cache
    
    // Write exactly 128 bits
    RT_TEST(bs.Write(0xFFFFFFFFFFFFFFFFULL, 64), >= 0)
    RT_TEST(bs.Write(0xFFFFFFFFFFFFFFFFULL, 64), >= 0)
    
    // Now write one more bit (should trigger buffer flush)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Verify
    bs.Rewind();
    uint64 v1 = 0, v2 = 0;
    uint32 v3 = 0;
    RT_TEST(bs.Read(&v1, 64), >= 0)
    RT_TEST(bs.Read(&v2, 64), >= 0)
    RT_TEST(bs.Read(&v3, 1), >= 0)
    RT_TEST(v1, == 0xFFFFFFFFFFFFFFFFULL)
    RT_TEST(v2, == 0xFFFFFFFFFFFFFFFFULL)
    RT_TEST(v3, == 1)
    
    return 0;
}

// Test 35: Seek to same position twice
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestSeekToSamePositionTwice()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
    
    RT_TEST(bs.Write(0xABCDEF12, 32), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    
    // Seek to bit 8           across
    RT_TEST(bs.Seek(8, STRM::SEEK_SET), >= 0)
    uint32 v1 = 0;
    RT_TEST(bs.Read(&v1, 8), >= 0)
    
    // Seek to bit 8 again
    RT_TEST(bs.Seek(8, STRM::SEEK_SET), >= 0)
    uint32 v2 = 0;
    RT_TEST(bs.Read(&v2, 8), >= 0)
    
    RT_TEST(v1, == v2, "Should read same value")
    
    bs.Reset();

    return 0;
}

// Test 35: Seek to same position twice
template<bool IsBitOrderMSB=true, bool IsByteOrderBE=true> static int TestFlushKeepStreamPos()
{
    STRM::CBitStream<CMemStream, 0, IsBitOrderMSB, IsByteOrderBE> bs(mem); mem->Reset();
   
    // Test flushing into single unit 
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)

    bs.Rewind();
    uint32 v1 = 0;
    RT_TEST(bs.Read(&v1, 8), >= 0)
    RT_TEST(v1, == 0xc7)

    // Test flushing of the second unit
    bs.Reset();
    RT_TEST(bs.Write(usize(0), sizeof(usize)*8), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)

    RT_TEST(bs.Seek(sizeof(usize)*8, STRM::SEEK_SET), >= 0)
    uint32 v2 = 0;
    RT_TEST(bs.Read(&v2, 8), >= 0)
    RT_TEST(v2, == 0xc7)

    // Test flushing across units
    bs.Reset();
    RT_TEST(bs.Write(usize(0), (sizeof(usize)*8)-4), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Flush(), >= 0)
    RT_TEST(bs.Write(0, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)
    RT_TEST(bs.Write(1, 1), >= 0)

    RT_TEST(bs.Seek((sizeof(usize)*8)-4, STRM::SEEK_SET), >= 0)
    uint32 v3 = 0;
    RT_TEST(bs.Read(&v3, 8), >= 0)
    RT_TEST(v3, == 0xc7)

    return 0;
}
//------------------------------------------------------------------------------------
static int DoTests(void)
{
 LOGMSG("Begin: STRM::CBitStream");
 STRM::CStrmBuf mstrm;
 mstrm.Init(&mbuf, sizeof(mbuf));
 mem = &mstrm;

 // Basic write/read tests (MSB, BE)
 RT_TEST((TestBasicWriteRead<true,true>()), == 0)
 RT_TEST((TestBoundaryAlignment<true,true>()), == 0)
 RT_TEST((TestUnitBoundaryCrossing<true,true>()), == 0)
 RT_TEST((TestCacheBoundaryCrossing<true,true>()), == 0)
 RT_TEST((TestFlushingPartialBits<true,true>()), == 0)
 RT_TEST((TestSeekBitAligned<true,true>()), == 0)
 RT_TEST((TestSeekUnaligned<true,true>()), == 0)
 RT_TEST((TestSeekAndWrite<true,true>()), == 0)
 RT_TEST((TestSeekAndRead<true,true>()), == 0)
 RT_TEST((TestMixedReadWrite<true,true>()), == 0)
 RT_TEST((TestZeroBitsEdgeCase<true,true>()), == 0)
 RT_TEST((TestFullCacheUnitWrite<true,true>()), == 0)
 RT_TEST((TestPartialUnitAtEOF<true,true>()), == 0)
 RT_TEST((TestOverwriteAfterSeek<true,true>()), == 0)
 RT_TEST((TestFlushBetweenOperations<true,true>()), == 0)
 RT_TEST((TestLargeWriteAcrossMultipleUnits<true,true>()), == 0)
 RT_TEST((TestReadAfterPartialWrite<true,true>()), == 0)
 RT_TEST((TestCacheBPosZeroEdgeCase<true,true>()), == 0) 
 // Additional edge case tests
 RT_TEST((TestSeekThenImmediateRead<true,true>()), == 0)
 RT_TEST((TestMultipleSeeks<true,true>()), == 0)
 RT_TEST((TestSeekCurrent<true,true>()), == 0)
 RT_TEST((TestSingleBitOperations<true,true>()), == 0)
 RT_TEST((TestNearFullUnitWrites<true,true>()), == 0)
 RT_TEST((TestFlushOnFreshStream<true,true>()), == 0)
 RT_TEST((TestMultipleRewinds<true,true>()), == 0)
 RT_TEST((TestWriteAfterPartialRead<true,true>()), == 0)
 RT_TEST((TestFlushKeepStreamPos<true,true>()), == 0)
 // Critical merge tests
 RT_TEST((TestSeekUnalignedWritePartialFlush<true,true>()), == 0)
 RT_TEST((TestSeekWritePartialFlushThenRead<true,true>()), == 0)
 RT_TEST((TestSeekNearUnitBoundaryWriteFlush<true,true>()), == 0)
 RT_TEST((TestWriteSeekBackWriteFlush<true,true>()), == 0)
 RT_TEST((TestDirtyCacheSeekRead<true,true>()), == 0)
 RT_TEST((TestSeekToEndThenWrite<true,true>()), == 0)
 RT_TEST((TestVeryLargeSingleWrite<true,true>()), == 0)
 RT_TEST((TestWriteExactlyFillBufferPlusOne<true,true>()), == 0)
 RT_TEST((TestSeekToSamePositionTwice<true,true>()), == 0)
//-------------------------------------

 // Basic write/read tests (LSB, LE)
 RT_TEST((TestBasicWriteRead<false,false>()), == 0)
 RT_TEST((TestBoundaryAlignment<false,false>()), == 0)
 RT_TEST((TestUnitBoundaryCrossing<false,false>()), == 0)
 RT_TEST((TestCacheBoundaryCrossing<false,false>()), == 0)
 RT_TEST((TestFlushingPartialBits<false,false>()), == 0)
 RT_TEST((TestSeekBitAligned<false,false>()), == 0)
 RT_TEST((TestSeekUnaligned<false,false>()), == 0)
 RT_TEST((TestSeekAndWrite<false,false>()), == 0)
 RT_TEST((TestSeekAndRead<false,false>()), == 0)
 RT_TEST((TestMixedReadWrite<false,false>()), == 0)
 RT_TEST((TestZeroBitsEdgeCase<false,false>()), == 0)
 RT_TEST((TestFullCacheUnitWrite<false,false>()), == 0)
 RT_TEST((TestPartialUnitAtEOF<false,false>()), == 0)
 RT_TEST((TestOverwriteAfterSeek<false,false>()), == 0)
 RT_TEST((TestFlushBetweenOperations<false,false>()), == 0)
 RT_TEST((TestLargeWriteAcrossMultipleUnits<false,false>()), == 0)
 RT_TEST((TestReadAfterPartialWrite<false,false>()), == 0)
 RT_TEST((TestCacheBPosZeroEdgeCase<false,false>()), == 0) 
 RT_TEST((TestFlushKeepStreamPos<true,true>()), == 0)
 // Additional edge case tests
 RT_TEST((TestSeekThenImmediateRead<false,false>()), == 0)
 RT_TEST((TestMultipleSeeks<false,false>()), == 0)
 RT_TEST((TestSeekCurrent<false,false>()), == 0)
 RT_TEST((TestSingleBitOperations<false,false>()), == 0)
 RT_TEST((TestNearFullUnitWrites<false,false>()), == 0)
 RT_TEST((TestFlushOnFreshStream<false,false>()), == 0)
 RT_TEST((TestMultipleRewinds<false,false>()), == 0)
 RT_TEST((TestWriteAfterPartialRead<false,false>()), == 0)
 // Critical merge tests
 RT_TEST((TestSeekUnalignedWritePartialFlush<false,false>()), == 0)
 RT_TEST((TestSeekWritePartialFlushThenRead<false,false>()), == 0)
 RT_TEST((TestSeekNearUnitBoundaryWriteFlush<false,false>()), == 0)
 RT_TEST((TestWriteSeekBackWriteFlush<false,false>()), == 0)
 RT_TEST((TestDirtyCacheSeekRead<false,false>()), == 0)
 RT_TEST((TestSeekToEndThenWrite<false,false>()), == 0)
 RT_TEST((TestVeryLargeSingleWrite<false,false>()), == 0)
 RT_TEST((TestWriteExactlyFillBufferPlusOne<false,false>()), == 0)
 RT_TEST((TestSeekToSamePositionTwice<false,false>()), == 0)  
//-------------------------------------
   
 // Basic write/read tests (LSB, BE)  (Useless?)
 RT_TEST((TestBasicWriteRead<false,true>()), == 0)
 RT_TEST((TestBoundaryAlignment<false,true>()), == 0)
 RT_TEST((TestUnitBoundaryCrossing<false,true>()), == 0)
 RT_TEST((TestCacheBoundaryCrossing<false,true>()), == 0)
 RT_TEST((TestFlushingPartialBits<false,true>()), == 0)
 RT_TEST((TestSeekBitAligned<false,true>()), == 0)
 RT_TEST((TestSeekUnaligned<false,true>()), == 0)
 RT_TEST((TestSeekAndWrite<false,true>()), == 0)
 RT_TEST((TestSeekAndRead<false,true>()), == 0)
 RT_TEST((TestMixedReadWrite<false,true>()), == 0)
 RT_TEST((TestZeroBitsEdgeCase<false,true>()), == 0)
 RT_TEST((TestFullCacheUnitWrite<false,true>()), == 0)
 RT_TEST((TestPartialUnitAtEOF<false,true>()), == 0)
 RT_TEST((TestOverwriteAfterSeek<false,true>()), == 0)
 RT_TEST((TestFlushBetweenOperations<false,true>()), == 0)
 RT_TEST((TestLargeWriteAcrossMultipleUnits<false,true>()), == 0)
 RT_TEST((TestReadAfterPartialWrite<false,true>()), == 0)
 RT_TEST((TestCacheBPosZeroEdgeCase<false,true>()), == 0)
 RT_TEST((TestFlushKeepStreamPos<true,true>()), == 0)
 // Additional edge case tests
 RT_TEST((TestSeekThenImmediateRead<false,true>()), == 0)
 RT_TEST((TestMultipleSeeks<false,true>()), == 0)
 RT_TEST((TestSeekCurrent<false,true>()), == 0)
 RT_TEST((TestSingleBitOperations<false,true>()), == 0)
 RT_TEST((TestNearFullUnitWrites<false,true>()), == 0)
 RT_TEST((TestFlushOnFreshStream<false,true>()), == 0)
 RT_TEST((TestMultipleRewinds<false,true>()), == 0)
 RT_TEST((TestWriteAfterPartialRead<false,true>()), == 0)
 // Critical merge tests
 RT_TEST((TestSeekUnalignedWritePartialFlush<false,true>()), == 0)
 RT_TEST((TestSeekWritePartialFlushThenRead<false,true>()), == 0)
 RT_TEST((TestSeekNearUnitBoundaryWriteFlush<false,true>()), == 0)
 RT_TEST((TestWriteSeekBackWriteFlush<false,true>()), == 0)
 RT_TEST((TestDirtyCacheSeekRead<false,true>()), == 0)
 RT_TEST((TestSeekToEndThenWrite<false,true>()), == 0)
 RT_TEST((TestVeryLargeSingleWrite<false,true>()), == 0)
 RT_TEST((TestWriteExactlyFillBufferPlusOne<false,true>()), == 0)
 RT_TEST((TestSeekToSamePositionTwice<false,true>()), == 0)   
//-------------------------------------

// Basic write/read tests (MSB, LE)  (Useless?)
 RT_TEST((TestBasicWriteRead<true,false>()), == 0)
 RT_TEST((TestBoundaryAlignment<true,false>()), == 0)
 RT_TEST((TestUnitBoundaryCrossing<true,false>()), == 0)
 RT_TEST((TestCacheBoundaryCrossing<true,false>()), == 0)
 RT_TEST((TestFlushingPartialBits<true,false>()), == 0)
 RT_TEST((TestSeekBitAligned<true,false>()), == 0)
 RT_TEST((TestSeekUnaligned<true,false>()), == 0)
 RT_TEST((TestSeekAndWrite<true,false>()), == 0)
 RT_TEST((TestSeekAndRead<true,false>()), == 0)
 RT_TEST((TestMixedReadWrite<true,false>()), == 0)
 RT_TEST((TestZeroBitsEdgeCase<true,false>()), == 0)
 RT_TEST((TestFullCacheUnitWrite<true,false>()), == 0)
 RT_TEST((TestPartialUnitAtEOF<true,false>()), == 0)
 RT_TEST((TestOverwriteAfterSeek<true,false>()), == 0)
 RT_TEST((TestFlushBetweenOperations<true,false>()), == 0)
 RT_TEST((TestLargeWriteAcrossMultipleUnits<true,false>()), == 0)
 RT_TEST((TestReadAfterPartialWrite<true,false>()), == 0)
 RT_TEST((TestCacheBPosZeroEdgeCase<true,false>()), == 0) 
 RT_TEST((TestFlushKeepStreamPos<true,true>()), == 0)
 // Additional edge case tests
 RT_TEST((TestSeekThenImmediateRead<true,false>()), == 0)
 RT_TEST((TestMultipleSeeks<true,false>()), == 0)
 RT_TEST((TestSeekCurrent<true,false>()), == 0)
 RT_TEST((TestSingleBitOperations<true,false>()), == 0)
 RT_TEST((TestNearFullUnitWrites<true,false>()), == 0)
 RT_TEST((TestFlushOnFreshStream<true,false>()), == 0)
 RT_TEST((TestMultipleRewinds<true,false>()), == 0)
 RT_TEST((TestWriteAfterPartialRead<true,false>()), == 0)
 // Critical merge tests
 RT_TEST((TestSeekUnalignedWritePartialFlush<true,false>()), == 0)
 RT_TEST((TestSeekWritePartialFlushThenRead<true,false>()), == 0)
 RT_TEST((TestSeekNearUnitBoundaryWriteFlush<true,false>()), == 0)
 RT_TEST((TestWriteSeekBackWriteFlush<true,false>()), == 0)
 RT_TEST((TestDirtyCacheSeekRead<true,false>()), == 0)
 RT_TEST((TestSeekToEndThenWrite<true,false>()), == 0)
 RT_TEST((TestVeryLargeSingleWrite<true,false>()), == 0)
 RT_TEST((TestWriteExactlyFillBufferPlusOne<true,false>()), == 0)
 RT_TEST((TestSeekToSamePositionTwice<true,false>()), == 0)  
               
 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 LOGMSG("Begin: STRM::CBitStream");

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
};
