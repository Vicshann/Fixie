
struct Test_Streams
{
static int TestBufStream(void)
{
static uint8 testData[] = {
    0x11,0x22,0x33,0x44, 0x55,0x66,0x77,0x88,  // 8 bytes
    0x99,0xAA,0xBB,0xCC, 0xDD,0xEE,0xFF,0x00,  // +8 
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P'
};

 // Create stream with our test data
 alignas(16) uint8 buf[256];
 STRM::CStrmBuf strm(&testData, sizeof(testData), sizeof(testData));
  
 // Size/Offset basics
 RT_TEST(strm.Size(), == sizeof(testData))
 RT_TEST(strm.Offset(), == 0)
 
 // Seek SET - valid positions
 RT_TEST(strm.Seek(0, STRM::SEEK_SET), == 0)
 RT_TEST(strm.Seek(7, STRM::SEEK_SET), == 7)
 RT_TEST(strm.Offset(), == 7)
 
 // Seek CUR - forward/backward
 RT_TEST(strm.Seek(3, STRM::SEEK_CUR), == 10)
 RT_TEST(strm.Seek(-2, STRM::SEEK_CUR), == 8)
 
 // Seek END - backward only
 RT_TEST(strm.Seek(-8, STRM::SEEK_END), == (sizeof(testData)-8))
 RT_TEST(strm.Offset(), == (sizeof(testData)-8))
 
 // READ tests - power-of-2 fast paths + memcmp verification
 strm.Rewind();
 RT_TEST(strm.Read(buf, 1), == 1)  
 RT_TEST(memcmp(buf, testData+0, 1), == 0)
 RT_TEST(strm.Read(buf, 2), == 2)   
 RT_TEST(memcmp(buf, testData+1, 2), == 0)
 RT_TEST(strm.Read(buf, 4), == 4)  
 RT_TEST(memcmp(buf, testData+3, 4), == 0)
 RT_TEST(strm.Read(buf, 8), == 8)   
 RT_TEST(memcmp(buf, testData+7, 8), == 0)
 
 // Partial read at end
 RT_TEST((strm.Seek((sizeof(testData)-1), STRM::SEEK_SET), strm.Read(buf, 10)), == 1)  // Only 1 byte left
 
 // READ EOF
 RT_TEST((strm.Seek(sizeof(testData), STRM::SEEK_SET), strm.Read(buf, 1)), == STRM::SEOF)
 
 // WRITE tests - verify overwrites with memcmp
 uint8 writePat[] = {0xDE,0xAD,0xBE,0xEF};
 strm.Rewind();
 strm.Seek(4, STRM::SEEK_SET);
 RT_TEST(strm.Write(writePat, 4), == 4)    // NOTE: Modifies the test data
 RT_TEST(memcmp(testData+4, writePat, 4), == 0)  // Buffer modified!
 
 // Partial write
 RT_TEST((strm.Seek(sizeof(testData)-2, STRM::SEEK_SET), strm.Write(writePat, 4)), == 2)   // Only 2 bytes space
 
 // WRITE full
 RT_TEST((strm.Seek(sizeof(testData), STRM::SEEK_SET),strm.Write(writePat, 1)), == -PX::ENOSPC)
 
 // No-advance flag
 strm.Rewind();
 RT_TEST(strm.Read(buf, 4, STRM::rwfNoAdv), == 4)
 RT_TEST(strm.Offset(), == 0)  // No advance!
 
 // Reset/Rewind
 strm.Seek(sizeof(testData)-1, STRM::SEEK_SET);
 RT_TEST(strm.Reset(), == 0)
 RT_TEST(strm.Offset(), == 0)
 RT_TEST(strm.Rewind(), == 0)
 RT_TEST(strm.Seek(sizeof(testData), STRM::SEEK_CUR), == sizeof(testData))    // Last pos
 
 // Error cases
 RT_TEST(strm.Seek(-1, STRM::SEEK_SET), == -PX::ERANGE)
 RT_TEST(strm.Seek(200, STRM::SEEK_SET), == -PX::ERANGE)
 RT_TEST(strm.Seek(1, STRM::SEEK_CUR), == -PX::ERANGE) 

 // MEMCOPY path tests (sizes that don't hit fast paths: not 0,1,2,4,8,16)
 strm.Rewind();
 
 // Read 3 bytes (memcopy path)
 RT_TEST(strm.Read(buf, 3), == 3)
 RT_TEST(memcmp(buf, testData+0, 3), == 0)
 
 // Read 5 bytes (memcopy path)
 RT_TEST(strm.Read(buf, 5), == 5)
 RT_TEST(memcmp(buf, testData+3, 5), == 0)
 
 // Read 7 bytes (memcopy path)
 RT_TEST(strm.Read(buf, 7), == 7)
 RT_TEST(memcmp(buf, testData+8, 7), == 0)
 
 // Read 10 bytes (memcopy path)
 strm.Rewind();
 RT_TEST(strm.Read(buf, 10), == 10)
 RT_TEST(memcmp(buf, testData, 10), == 0)
 
 // Read 20 bytes (memcopy path, larger)
 strm.Rewind();
 RT_TEST(strm.Read(buf, 20), == 20)
 RT_TEST(memcmp(buf, testData, 20), == 0)
 
 // Read entire buffer via memcopy
 strm.Rewind();
 RT_TEST(strm.Read(buf, sizeof(testData)), == sizeof(testData))
 RT_TEST(memcmp(buf, testData, sizeof(testData)), == 0)
 
 // WRITE tests for memcopy path (NOTE: modifies testData)
 uint8 writePat3[] = {0xAA, 0xBB, 0xCC};
 uint8 writePat7[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
 uint8 writePat15[] = {0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
                       0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE};
 
 strm.Rewind();
 RT_TEST(strm.Write(writePat3, 3), == 3)
 RT_TEST(memcmp(testData, writePat3, 3), == 0)  // Modified
 
 strm.Seek(5, STRM::SEEK_SET);
 RT_TEST(strm.Write(writePat7, 7), == 7)
 RT_TEST(memcmp(testData+5, writePat7, 7), == 0)  // Modified
 
 // Write 15 bytes (memcopy path, not 16)
 strm.Rewind();
 RT_TEST(strm.Write(writePat15, 15), == 15)
 RT_TEST(memcmp(testData, writePat15, 15), == 0)  // Modified
 
 // Size 16 tests (128-bit fast path)
 alignas(16) uint8 buf16[16];
 alignas(16) uint8 pattern16[16] = {
     0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
     0x90,0xA0,0xB0,0xC0,0xD0,0xE0,0xF0,0x00
 };
 
 strm.Rewind();
 RT_TEST(strm.Read(buf16, 16), == 16)
 RT_TEST(memcmp(buf16, testData, 16), == 0)
 
 strm.Rewind();
 RT_TEST(strm.Write(pattern16, 16), == 16)
 RT_TEST(memcmp(testData, pattern16, 16), == 0)  // Modified
 
 // Read/Write 16 bytes with rwfNoAdv
 strm.Rewind();
 RT_TEST(strm.Read(buf16, 16, STRM::rwfNoAdv), == 16)
 RT_TEST(strm.Offset(), == 0)  // No advance
 
 strm.Rewind();
 RT_TEST(strm.Write(pattern16, 16, STRM::rwfNoAdv), == 16)
 RT_TEST(strm.Offset(), == 0)  // No advance
 RT_TEST(memcmp(testData, pattern16, 16), == 0)  // Modified
 
 // Zero-size operations
 RT_TEST(strm.Read(buf, 0), == 0)
 RT_TEST(strm.Offset(), == 0)  // No change
 RT_TEST(strm.Write(buf, 0), == 0)
 RT_TEST(strm.Offset(), == 0)  // No change
 
 // Boundary seeks
 RT_TEST(strm.Seek(sizeof(testData), STRM::SEEK_SET), == sizeof(testData))
 RT_TEST(strm.Offset(), == sizeof(testData))
 
 RT_TEST(strm.Seek(0, STRM::SEEK_END), == sizeof(testData))
 RT_TEST(strm.Offset(), == sizeof(testData))
 
 RT_TEST(strm.Seek(-sizeof(testData), STRM::SEEK_END), == 0)
 RT_TEST(strm.Offset(), == 0)
 
 // SEEK_CUR edge cases
 strm.Rewind();
 RT_TEST(strm.Seek(0, STRM::SEEK_CUR), == 0)  // No-op
 RT_TEST(strm.Seek(sizeof(testData), STRM::SEEK_CUR), == sizeof(testData))  // To end
 
 // SEEK_CUR beyond bounds
 RT_TEST(strm.Seek(1, STRM::SEEK_CUR), == -PX::ERANGE)  // Already at end
 strm.Rewind();
 RT_TEST(strm.Seek(-1, STRM::SEEK_CUR), == -PX::ERANGE)  // Before beginning
 
 // SEEK_END beyond bounds
 RT_TEST(strm.Seek(1, STRM::SEEK_END), == -PX::ERANGE)  // Positive offset
 RT_TEST(strm.Seek(-(sizeof(testData)+1), STRM::SEEK_END), == -PX::ERANGE)  // Too far back
 
 // Invalid seek type
 RT_TEST(strm.Seek(0, (STRM::ESeek)999), == -PX::EINVAL)
 
 // Read/Write with rwfNoAdv at various positions
 strm.Seek(10, STRM::SEEK_SET);
 RT_TEST(strm.Read(buf, 5, STRM::rwfNoAdv), == 5)
 RT_TEST(strm.Offset(), == 10)  // Unchanged
 RT_TEST(memcmp(buf, testData+10, 5), == 0)
 
 RT_TEST(strm.Write(writePat3, 3, STRM::rwfNoAdv), == 3)
 RT_TEST(strm.Offset(), == 10)  // Unchanged
 RT_TEST(memcmp(testData+10, writePat3, 3), == 0)  // Modified
 
 // Multiple sequential small reads (memcopy paths)
 strm.Rewind();
 RT_TEST(strm.Read(buf, 3), == 3)
 RT_TEST(strm.Read(buf, 5), == 5)
 RT_TEST(strm.Read(buf, 7), == 7)
 RT_TEST(strm.Offset(), == 15)
 
 // Large memcopy read near end with partial
 strm.Seek(-10, STRM::SEEK_END);
 RT_TEST(strm.Read(buf, 15), == 10)  // Only 10 bytes left
 RT_TEST(strm.Offset(), == sizeof(testData))


 //------------
  {
   uint  buf2[1024];
   achar path[260];
   STRM::CStrmBuf  dstrm;
   STRM::CStrmFile fstrm;
   STRM::CBitStream<STRM::CStrmBuf> bstrm(&dstrm);
   dstrm.Init(buf2, sizeof(buf2));
   
   bstrm.Write(1, 1);

   bstrm.Write(0b10110001101, 11);
   bstrm.Write(0, 2);
   bstrm.Write(0b111, 3);
   bstrm.Write(0, 4);
   bstrm.Write(0b11, 2);
   bstrm.Write(0, 32);
   bstrm.Write(0b111, 3);
   bstrm.Write(0, 3);
   bstrm.Write(0b111111, 6);

   bstrm.Flush();
   NSTR::StrCopy(path, TmpPath);
   NSTR::StrCnat(path, "/testbit.bin");
   ssize res = fstrm.Open(path);
   if(!fstrm.IsFail(res))
    {
     dstrm.ToStream(&fstrm, STRM::etsSizeUntilCurPos);
     fstrm.Close();
    }
  }



 return 0;
}
//------------------------------------------------------------------------------------
static int DoTests(void)
{
 LOGMSG("Begin: Buf stream");
 if(int res = TestBufStream();res < 0)return res;            

   
 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 LOGMSG("Begin: ?????");

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
};
