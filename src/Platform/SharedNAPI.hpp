
// NOTE: Extra Div + Mul on Windows in clocksleep!
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::nsleep,  nsleep  )     // Nanoseconds
{
 PX::timespec* pts = GetParFromPk<0>(args...);
 PX::timespec* rts;
 PX::timespec  tts;
 uint32 Flags = GetParFromPk<1>(args...);
 bool  canint = Flags & PX::CLKFG_INTRABLE;
 rts = canint?pts:&tts;
 int res = 0;
 while((res=NAPI::clocksleep(pts, rts, Flags)) == PXERR(EINTR))    // Abs no need Ret, just put it again
  {
   if(canint)return 1;
   pts = rts;  // Resume with remaining time     // Is it enough to just reuse the pointers?
  }
 return res;  // 0 or negative (error)
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::usleep,  usleep  )     // Microseconds
{
 uint64 msecs = GetParFromPk<0>(args...);
 int    Flags = GetParFromPk<1>(args...);
 bool  canint = Flags & PX::CLKFG_INTRABLE;

 PX::timespec ts;
 ts.sec  = msecs / 1000000;    // 1000000 Microseconds in one Second
 ts.frac = (msecs % 1000000) * 1000;      // 1000 nanoseconds in one Microsecond

 int res = 0;
 while((res=NAPI::clocksleep(&ts, &ts, Flags)) == PXERR(EINTR)) 
  {
   if(canint)return (ts.sec * 1000000) + (ts.frac / 1000);    // Return remaining Microseconds        // 1000 nanoseconds in one Microsecond
  }
 return (uint64)res;  // 0 or negative (error)                                            
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::msleep,  msleep  )     // Milliseconds
{
 uint64 msecs = GetParFromPk<0>(args...);
 int    Flags = GetParFromPk<1>(args...);
 bool  canint = Flags & PX::CLKFG_INTRABLE;

 PX::timespec ts;
 ts.sec  = msecs / 1000;
 ts.frac = (msecs % 1000) * 1000000;  // (milisec*1000000) % 1000000000; ?

 int res = 0;
 while((res=NAPI::clocksleep(&ts, &ts, Flags)) == PXERR(EINTR)) 
  {
   if(canint)return (ts.sec * 1000) + (ts.frac / 1000000);    // Return remaining Milliseconds     // 1000000 nanoseconds in one millisecond
  }
 return (uint64)res;  // 0 or negative (error)
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::sleep,  sleep  )       // Seconds
{
 uint64 secs  = GetParFromPk<0>(args...);
 int    Flags = GetParFromPk<1>(args...);
 bool  canint = Flags & PX::CLKFG_INTRABLE;

 PX::timespec ts;
 ts.sec  = secs;
 ts.frac = 0;

 int res = 0;
 while((res=NAPI::clocksleep(&ts, &ts, Flags)) == PXERR(EINTR)) 
  {
   if(canint)return ts.sec;    // Return remaining Seconds   
  }
 return (uint64)res;  // 0 or negative (error)
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::spollGD,   spoll       )
{
 PX::fdsc_t fd   = GetParFromPk<0>(args...);
 uint32 events   = GetParFromPk<1>(args...);
 sint64 timeout  = GetParFromPk<2>(args...);
 int64* time_rem = GetParFromPk<3>(args...);

 PX::pollfd fds {.fd=(int)fd,.events=(short)events,.revents=0}; 
 int res = NAPI::poll(&fds, 1, timeout, time_rem);
 if(res > 0)return fds.revents;
 return res;
}
//------------------------------------------------------------------------------------------------------------
