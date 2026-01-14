
struct Test_TestMemArenas
{
//------------------------------------------------------------------------------------
static int DoTests(void)
{
 LOGMSG("Begin: TestMemArenas");
/*
{
   NALC::CGenAlloc<4096,4096,NALC::afObjAlloc|NALC::afSinglePtr> alc;
     bool t1 = alc.IsElmExist(56);
   volatile vptr xx = alc.GetBlock(5);
     bool t2 = alc.IsElmExist(67);
      xx = alc.GetBlock(8);
        alc.DeleteBlk(5);
        xx = alc.GetBlock(43);
    vptr yy = alc.FindBlock(nullptr);
    auto dd = alc.GetBlkData(8, 4);
    alc.SetBlkTag(8, 9); 
    uint rr = alc.GetBlkTag(8);
    uint TotalElm1 = 0;
    uint TotalElm2 = 0;
    uint TotalElm3 = 0;
    for (auto it = alc.begin(), end = alc.end(); it != end; ++it) 
     {
      *it = 6;
      TotalElm1++;
     }
    for (auto it = alc.begin(), end = alc.end(); it ; ++it) 
     {
      *it = 6;
      TotalElm2++;
     }
    for (auto& el : alc) 
     {
      el = 6;
      TotalElm3++;
     }
   //int v = AlignToP2Dn(5);
   //v++;
   return 11;
  }

*/
 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
static int DoBenchmarks(void) 
{
 LOGMSG("Begin: TestMemArenas");

 LOGMSG("Done");
 return 0;
}
//------------------------------------------------------------------------------------
};
