
struct Test_BitOps
{
//------------------------------------------------------------------------------------
static int DoTests(void)
{
 LOGMSG("Begin: BitOps");

 CTRT_TEST(clz(uint8(0)), == 8)
 CTRT_TEST(clz(uint8(1)), == 7) 
 CTRT_TEST(clz(uint8(-1)), == 0) 
 CTRT_TEST(clz(uint8(1u<<7)), == 0) 
 
 CTRT_TEST(clz(uint16(0)), == 16) 
 CTRT_TEST(clz(uint16(1)), == 15) 
 CTRT_TEST(clz(uint16(-1)), == 0) 
 CTRT_TEST(clz(uint16(1u<<15)), == 0)
  
 CTRT_TEST(clz(uint32(0)), == 32) 
 CTRT_TEST(clz(uint32(1)), == 31) 
 CTRT_TEST(clz(uint32(-1)), == 0)
 CTRT_TEST(clz(uint32(1ul<<31)), == 0) 
 
 CTRT_TEST(clz(uint64(0)), == 64)
 CTRT_TEST(clz(uint64(1)), == 63) 
 CTRT_TEST(clz(uint64(-1)), == 0) 
 CTRT_TEST(clz(uint64(1ull<<63)), == 0) 
 // --- CTZ
 CTRT_TEST(ctz(uint8(0)), == 8)
 CTRT_TEST(ctz(uint8(1)), == 0) 
 CTRT_TEST(ctz(uint8(-1)), == 0) 
 CTRT_TEST(ctz(uint8(1u<<7)), == 7) 
            
 CTRT_TEST(ctz(uint16(0)), == 16) 
 CTRT_TEST(ctz(uint16(1)), == 0) 
 CTRT_TEST(ctz(uint16(-1)), == 0) 
 CTRT_TEST(ctz(uint16(1u<<15)), == 15)
            
 CTRT_TEST(ctz(uint32(0)), == 32) 
 CTRT_TEST(ctz(uint32(1)), == 0) 
 CTRT_TEST(ctz(uint32(-1)), == 0)
 CTRT_TEST(ctz(uint32(1ul<<31)), == 31) 
            
 CTRT_TEST(ctz(uint64(0)), == 64)
 CTRT_TEST(ctz(uint64(1)), == 0) 
 CTRT_TEST(ctz(uint64(-1)), == 0) 
 CTRT_TEST(ctz(uint64(1ull<<63)), == 63) 

#ifdef HAS_INT128  
 CTRT_TEST(clz(uint128(0)), == 128) 
 CTRT_TEST(clz(uint128(1)), == 127) 
 CTRT_TEST(clz(uint128(-1)), == 0) 
 CTRT_TEST(clz(uint128(uint128(1)<<127)), == 0) 

 CTRT_TEST(ctz(uint128(0)), == 128) 
 CTRT_TEST(ctz(uint128(1)), == 0) 
 CTRT_TEST(ctz(uint128(-1)), == 0) 
 CTRT_TEST(ctz(uint128(uint128(1)<<127)), == 127) 
#endif

 // ===== 8-bit tests (no-op) =====
 CTRT_TEST(SwapBytes<uint8>(0x12), == 0x12)
 CTRT_TEST(SwapBytes<sint8>(0x7F), == 0x7F)
 CTRT_TEST(SwapBytes<uint8>(0xFF), == 0xFF)
 
 // ===== 16-bit tests =====
 CTRT_TEST(SwapBytes<uint16>(0x1234), == 0x3412)
 CTRT_TEST(SwapBytes<uint16>(0xABCD), == 0xCDAB)
 CTRT_TEST(SwapBytes<uint16>(0xFF00), == 0x00FF)
 CTRT_TEST(SwapBytes<uint16>(0x00FF), == 0xFF00)
 CTRT_TEST(SwapBytes<sint16>(0x1234), == 0x3412)
 
 // Edge cases
 CTRT_TEST(SwapBytes<uint16>(0x0000), == 0x0000)
 CTRT_TEST(SwapBytes<uint16>(0xFFFF), == 0xFFFF)
 CTRT_TEST(SwapBytes<uint16>(0x0001), == 0x0100)
 CTRT_TEST(SwapBytes<uint16>(0x8000), == 0x0080)
 
 // ===== 32-bit tests =====
 CTRT_TEST(SwapBytes<uint32>(0x12345678), == 0x78563412)
 CTRT_TEST(SwapBytes<uint32>(0xAABBCCDD), == 0xDDCCBBAA)
 CTRT_TEST(SwapBytes<uint32>(0xFF000000), == 0x000000FF)
 CTRT_TEST(SwapBytes<uint32>(0x000000FF), == 0xFF000000)
 CTRT_TEST(SwapBytes<sint32>(0x12345678), == 0x78563412)
 
 // Edge cases
 CTRT_TEST(SwapBytes<uint32>(0x00000000), == 0x00000000)
 CTRT_TEST(SwapBytes<uint32>(0xFFFFFFFF), == 0xFFFFFFFF)
 CTRT_TEST(SwapBytes<uint32>(0x00000001), == 0x01000000)
 CTRT_TEST(SwapBytes<uint32>(0x80000000), == 0x00000080)
 
 // ===== 64-bit tests =====
 CTRT_TEST(SwapBytes<uint64>(0x123456789ABCDEF0ULL), == 0xF0DEBC9A78563412ULL)
 CTRT_TEST(SwapBytes<uint64>(0x0102030405060708ULL), == 0x0807060504030201ULL)
 CTRT_TEST(SwapBytes<uint64>(0xFF00000000000000ULL), == 0x00000000000000FFULL)
 CTRT_TEST(SwapBytes<uint64>(0x00000000000000FFULL), == 0xFF00000000000000ULL)
 CTRT_TEST(SwapBytes<sint64>(0x123456789ABCDEF0LL),  == 0xF0DEBC9A78563412ULL)
 
 // Edge cases
 CTRT_TEST(SwapBytes<uint64>(0x0000000000000000ULL), == 0x0000000000000000ULL)
 CTRT_TEST(SwapBytes<uint64>(0xFFFFFFFFFFFFFFFFULL), == 0xFFFFFFFFFFFFFFFFULL)
 CTRT_TEST(SwapBytes<uint64>(0x0000000000000001ULL), == 0x0100000000000000ULL)
 CTRT_TEST(SwapBytes<uint64>(0x8000000000000000ULL), == 0x0000000000000080ULL)
 
 // ===== Double-swap tests (should return original) =====
 CTRT_TEST(SwapBytes(SwapBytes<uint16>(0x1234)), == 0x1234)
 CTRT_TEST(SwapBytes(SwapBytes<uint32>(0x12345678)), == 0x12345678)
 CTRT_TEST(SwapBytes(SwapBytes<uint64>(0x123456789ABCDEF0ULL)), == 0x123456789ABCDEF0ULL)
 
 // ===== Pattern tests =====
 // Alternating bytes
 CTRT_TEST(SwapBytes<uint32>(0xAA55AA55), == 0x55AA55AA)
 CTRT_TEST(SwapBytes<uint64>(0xAA55AA55AA55AA55ULL), == 0x55AA55AA55AA55AAULL)
 
 // Sequential bytes
 CTRT_TEST(SwapBytes<uint32>(0x01020304), == 0x04030201)
 CTRT_TEST(SwapBytes<uint64>(0x0102030405060708ULL), == 0x0807060504030201ULL)
    
#ifdef HAS_INT128
 // ===== 128-bit tests (if supported) =====
 CTRT_TEST(SwapBytes<uint128>(((uint128)0x0102030405060708ULL << 64) | 0x090A0B0C0D0E0F10ULL), == (((uint128)0x100F0E0D0C0B0A09ULL << 64) | 0x0807060504030201ULL))
#endif
//------------------------------------------------
// Test 8-bit
 CTRT_TEST(SwapBits((uint8)0b00000000), == 0b00000000)
 CTRT_TEST(SwapBits((uint8)0b11111111), == 0b11111111)
 CTRT_TEST(SwapBits((uint8)0b10000000), == 0b00000001)
 CTRT_TEST(SwapBits((uint8)0b00000001), == 0b10000000)
 CTRT_TEST(SwapBits((uint8)0b10101010), == 0b01010101)
 CTRT_TEST(SwapBits((uint8)0b11001100), == 0b00110011)
 CTRT_TEST(SwapBits((uint8)0b11110000), == 0b00001111)
 CTRT_TEST(SwapBits((uint8)0xAB), == 0xD5) // 10101011 -> 11010101
 
 // Test 16-bit
 CTRT_TEST(SwapBits((uint16)0x0000), == 0x0000)
 CTRT_TEST(SwapBits((uint16)0xFFFF), == 0xFFFF)
 CTRT_TEST(SwapBits((uint16)0x8000), == 0x0001)
 CTRT_TEST(SwapBits((uint16)0x0001), == 0x8000)
 CTRT_TEST(SwapBits((uint16)0xAAAA), == 0x5555)
 CTRT_TEST(SwapBits((uint16)0x1234), == 0x2C48) // 0001001000110100 -> 0010110001001000
 CTRT_TEST(SwapBits((uint16)0xF0F0), == 0x0F0F)
 
 // Test 32-bit
 CTRT_TEST(SwapBits((uint32)0x00000000), == 0x00000000)
 CTRT_TEST(SwapBits((uint32)0xFFFFFFFF), == 0xFFFFFFFF)
 CTRT_TEST(SwapBits((uint32)0x80000000), == 0x00000001)
 CTRT_TEST(SwapBits((uint32)0x00000001), == 0x80000000)
 CTRT_TEST(SwapBits((uint32)0xAAAAAAAA), == 0x55555555)
 CTRT_TEST(SwapBits((uint32)0x12345678), == 0x1E6A2C48)
 CTRT_TEST(SwapBits((uint32)0xF0F0F0F0), == 0x0F0F0F0F)
 CTRT_TEST(SwapBits((uint32)0xDEADBEEF), == 0xF77DB57B)
 
 // Test 64-bit
 CTRT_TEST(SwapBits((uint64)0x0000000000000000), == 0x0000000000000000)
 CTRT_TEST(SwapBits((uint64)0xFFFFFFFFFFFFFFFF), == 0xFFFFFFFFFFFFFFFF)
 CTRT_TEST(SwapBits((uint64)0x8000000000000000), == 0x0000000000000001)
 CTRT_TEST(SwapBits((uint64)0x0000000000000001), == 0x8000000000000000)
 CTRT_TEST(SwapBits((uint64)0xAAAAAAAAAAAAAAAA), == 0x5555555555555555)
 CTRT_TEST(SwapBits((uint64)0x123456789ABCDEF0), == 0x0F7B3D591E6A2C48)
 CTRT_TEST(SwapBits((uint64)0xF0F0F0F0F0F0F0F0), == 0x0F0F0F0F0F0F0F0F)
 
 // Test signed types work correctly
 CTRT_TEST(SwapBits((int8)-1), == uint8(-1))
 CTRT_TEST(SwapBits((int16)-1), == uint16(-1))
 CTRT_TEST(SwapBits((int32)-1), == uint32(-1))
 CTRT_TEST(SwapBits((int64)-1), == uint64(-1))
        
 // Test that SwapBits is its own inverse (double application returns original)
 CTRT_TEST(SwapBits(SwapBits((uint8)0xAB)), == 0xAB)
 CTRT_TEST(SwapBits(SwapBits((uint16)0x1234)), == 0x1234)
 CTRT_TEST(SwapBits(SwapBits((uint32)0x12345678)), == 0x12345678)
 CTRT_TEST(SwapBits(SwapBits((uint64)0x123456789ABCDEF0)), == 0x123456789ABCDEF0)
 
 // Test edge cases with specific bit patterns
 CTRT_TEST(SwapBits((uint32)0x01020304), == 0x20C04080)
 CTRT_TEST(SwapBits((uint32)0xF0E1D2C3), == 0xC34B870F)
        
#ifdef HAS_INT128
 // Test 128-bit if available
 CTRT_TEST(SwapBits((uint128)0), == 0)
 CTRT_TEST(SwapBits((uint128)-1), == (uint128)-1)
 CT_TEST((SwapBits((uint128)1) == ((uint128)1 << 127)), == true)
 CT_TEST((SwapBits((uint128)1 << 127) == (uint128)1), == true)
#endif
//------------------------------------------------        
 // ===== 8-bit tests =====
 // Basic rotations
 CTRT_TEST(RotL(uint8(0b10110011), 1), == 0b01100111, "8-bit RotL by 1")
 CTRT_TEST(RotL(uint8(0b10110011), 4), == 0b00111011, "8-bit RotL by 4")
 CTRT_TEST(RotR(uint8(0b10110011), 1), == 0b11011001, "8-bit RotR by 1")
 CTRT_TEST(RotR(uint8(0b10110011), 4), == 0b00111011, "8-bit RotR by 4")
 
 // Zero shift (identity)
 CTRT_TEST(RotL(uint8(0xAB), 0), == 0xAB, "8-bit RotL by 0")
 CTRT_TEST(RotR(uint8(0xAB), 0), == 0xAB, "8-bit RotR by 0")
 
 // Full rotation (should be identity)
 CTRT_TEST(RotL(uint8(0xAB), 8), == 0xAB, "8-bit RotL by 8 (full)")
 CTRT_TEST(RotR(uint8(0xAB), 8), == 0xAB, "8-bit RotR by 8 (full)")
 
 // Overflow shift (should wrap)
 CTRT_TEST(RotL(uint8(0xAB), 9), == RotL(uint8(0xAB), 1), "8-bit RotL overflow")
 CTRT_TEST(RotR(uint8(0xAB), 9), == RotR(uint8(0xAB), 1), "8-bit RotR overflow")
 
 // Edge cases
 CTRT_TEST(RotL(uint8(0xFF), 3), == 0xFF, "8-bit RotL all 1s")
 CTRT_TEST(RotL(uint8(0x00), 3), == 0x00, "8-bit RotL all 0s")
 CTRT_TEST(RotL(uint8(0x01), 7), == 0x80, "8-bit RotL single bit")
 CTRT_TEST(RotR(uint8(0x80), 7), == 0x01, "8-bit RotR single bit")
 
 // ===== 16-bit tests =====
 CTRT_TEST(RotL(uint16(0x1234), 4), == 0x2341, "16-bit RotL by 4")
 CTRT_TEST(RotL(uint16(0x1234), 8), == 0x3412, "16-bit RotL by 8")
 CTRT_TEST(RotR(uint16(0x1234), 4), == 0x4123, "16-bit RotR by 4")
 CTRT_TEST(RotR(uint16(0x1234), 8), == 0x3412, "16-bit RotR by 8")
 
 // Full rotation
 CTRT_TEST(RotL(uint16(0xABCD), 16), == 0xABCD, "16-bit RotL by 16 (full)")
 CTRT_TEST(RotR(uint16(0xABCD), 16), == 0xABCD, "16-bit RotR by 16 (full)")
 
 // Overflow shift
 CTRT_TEST(RotL(uint16(0xABCD), 20), == RotL(uint16(0xABCD), 4), "16-bit RotL overflow")
 
 // ===== 32-bit tests =====
 CTRT_TEST(RotL(uint32(0x12345678), 4), == 0x23456781, "32-bit RotL by 4")
 CTRT_TEST(RotL(uint32(0x12345678), 8), == 0x34567812, "32-bit RotL by 8")
 CTRT_TEST(RotL(uint32(0x12345678), 16), == 0x56781234, "32-bit RotL by 16")
 CTRT_TEST(RotR(uint32(0x12345678), 4), == 0x81234567, "32-bit RotR by 4")
 CTRT_TEST(RotR(uint32(0x12345678), 8), == 0x78123456, "32-bit RotR by 8")
 CTRT_TEST(RotR(uint32(0x12345678), 16), == 0x56781234, "32-bit RotR by 16")
 
 // Zero and full rotation
 CTRT_TEST(RotL(uint32(0xDEADBEEF), 0), == 0xDEADBEEF, "32-bit RotL by 0")
 CTRT_TEST(RotL(uint32(0xDEADBEEF), 32), == 0xDEADBEEF, "32-bit RotL by 32 (full)")
 CTRT_TEST(RotR(uint32(0xDEADBEEF), 32), == 0xDEADBEEF, "32-bit RotR by 32 (full)")
 
 // Large shift values
 CTRT_TEST(RotL(uint32(0x12345678), 36), == RotL(uint32(0x12345678), 4), "32-bit RotL large shift")
 CTRT_TEST(RotR(uint32(0x12345678), 40), == RotR(uint32(0x12345678), 8), "32-bit RotR large shift")
 
 // ===== 64-bit tests =====
 CTRT_TEST(RotL(uint64(0x123456789ABCDEF0), 4), == 0x23456789ABCDEF01, "64-bit RotL by 4")
 CTRT_TEST(RotL(uint64(0x123456789ABCDEF0), 32), == 0x9ABCDEF012345678, "64-bit RotL by 32")
 CTRT_TEST(RotR(uint64(0x123456789ABCDEF0), 4), == 0x0123456789ABCDEF, "64-bit RotR by 4")
 CTRT_TEST(RotR(uint64(0x123456789ABCDEF0), 32), == 0x9ABCDEF012345678, "64-bit RotR by 32")
 
 // Full rotation
 CTRT_TEST(RotL(uint64(0xFEDCBA9876543210), 64), == 0xFEDCBA9876543210, "64-bit RotL by 64 (full)")
 CTRT_TEST(RotR(uint64(0xFEDCBA9876543210), 64), == 0xFEDCBA9876543210, "64-bit RotR by 64 (full)")
 
 // Overflow
 CTRT_TEST(RotL(uint64(0x123456789ABCDEF0), 68), == RotL(uint64(0x123456789ABCDEF0), 4), "64-bit RotL overflow")
 
 // ===== Symmetry tests (RotL then RotR should give original) =====
 CTRT_TEST(RotR(RotL(uint8(0xA5), 3), 3), == 0xA5, "8-bit symmetry")
 CTRT_TEST(RotR(RotL(uint16(0xA5A5), 7), 7), == 0xA5A5, "16-bit symmetry")
 CTRT_TEST(RotR(RotL(uint32(0xA5A5A5A5), 13), 13), == 0xA5A5A5A5, "32-bit symmetry")
 CTRT_TEST(RotR(RotL(uint64(0xA5A5A5A5A5A5A5A5), 37), 37), == 0xA5A5A5A5A5A5A5A5, "64-bit symmetry")
 
 // ===== RotL(n) == RotR(bits-n) tests =====
 CTRT_TEST(RotL(uint8(0x12), 3), == RotR(uint8(0x12), 5), "8-bit RotL(3) == RotR(5)")
 CTRT_TEST(RotL(uint16(0x1234), 6), == RotR(uint16(0x1234), 10), "16-bit RotL(6) == RotR(10)")
 CTRT_TEST(RotL(uint32(0x12345678), 12), == RotR(uint32(0x12345678), 20), "32-bit RotL(12) == RotR(20)")
 CTRT_TEST(RotL(uint64(0x123456789ABCDEF0), 40), == RotR(uint64(0x123456789ABCDEF0), 24), "64-bit RotL(40) == RotR(24)")
 
 // ===== Signed input tests (should convert to unsigned) =====
 CTRT_TEST(RotL(int8(-1), 1), == 0xFF, "Signed 8-bit input")
 CTRT_TEST(RotL(int16(-1), 1), == 0xFFFF, "Signed 16-bit input")
 CTRT_TEST(RotL(int32(-1), 1), == 0xFFFFFFFF, "Signed 32-bit input")
 CTRT_TEST(RotL(int64(-1), 1), == 0xFFFFFFFFFFFFFFFF, "Signed 64-bit input")
 
 // Signed value rotation
 CTRT_TEST(RotL(int8(0x81), 1), == 0x03, "Signed 8-bit rotation")
 CTRT_TEST(RotR(int8(0x81), 1), == 0xC0, "Signed 8-bit rotation reverse")
 
 // ===== Pattern preservation tests =====
 CTRT_TEST(RotL(uint32(0xAAAAAAAA), 1), == 0x55555555, "32-bit alternating pattern RotL")
 CTRT_TEST(RotR(uint32(0xAAAAAAAA), 1), == 0x55555555, "32-bit alternating pattern RotR")
 CTRT_TEST(RotL(uint32(0x0F0F0F0F), 4), == 0xF0F0F0F0, "32-bit nibble pattern")
//------------------------------------------------

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 return 0;
}
//------------------------------------------------------------------------------------
};
