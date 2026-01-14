
struct Test_RLE
{
 using CMemStream = STRM::CStrmBuf;
 alignas(16) static inline uint8 mbuf[4096];
 static inline STRM::CStrmBuf* mem;
//------------------------------------------------------------------------------------
static int TestVarInt(void)
{
 STRM::CBitStreamMSB<CMemStream> bs(mem); mem->Reset();
 NRLE::WriteBitVarInt<sint32>(&bs, 0x112233);
 NRLE::WriteBitVarInt<sint32>(&bs, -0x112233);

 sint32 Val1 = 0;
 sint32 Val2 = 0;
 bs.Rewind();
 int res1 = NRLE::ReadBitVarInt<sint32>(&bs, Val1);
 int res2 = NRLE::ReadBitVarInt<sint32>(&bs, Val2);

 RT_TEST(Val1, == 0x112233)
 RT_TEST(Val2, == -0x112233)

 return 0;
}
//------------------------------------------------------------------------------------
static int DoTests(void)
{
 LOGMSG("Begin: RLE");
 STRM::CStrmBuf mstrm;
 mstrm.Init(&mbuf, sizeof(mbuf));
 mem = &mstrm;

 RT_TEST((TestVarInt()), == 0)

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 LOGMSG("Begin: RLE");

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
};
