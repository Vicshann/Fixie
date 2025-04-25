
// Members: void (Observer::*)(const T&...)

// NOTE: Those functions may do nothing but must be implemented to make apps compilable for all supported platforms
// NOTE: Return types must be of exact type
//============================================================================================================
//                                   Startup helper functions
//------------------------------------------------------------------------------------------------------------
API_VALIDATE(GetStdIn,                                PX::fdsc_t,             void)   // TODO: Remove (Use PX::EDFD instead)
API_VALIDATE(GetStdOut,                               PX::fdsc_t,             void)   // TODO: Remove (Use PX::EDFD instead)
API_VALIDATE(GetStdErr,                               PX::fdsc_t,             void)   // TODO: Remove (Use PX::EDFD instead)
           
// Date and time                                                                   
API_VALIDATE(GetTZOffsUTC,                                sint32,             void)
        
// Command line                                                                    
API_VALIDATE(CheckCLArg,                                    bool,             sint&, const achar*, uint32)
API_VALIDATE(GetCLArg,                                      sint,             sint&, achar*, uint)
API_VALIDATE(SkipCLArg,                           const syschar*,             sint&, uint*)
 
// Environment                                                                           
API_VALIDATE(GetEnvVar,                                     sint,             sint&, achar*, uint)
API_VALIDATE(GetEnvVar,                                     sint,             const achar*, achar*, uint)
API_VALIDATE(GetEnvVar,                           const syschar*,             sint&, uint*)
API_VALIDATE(GetEnvVar,                           const syschar*,             const achar*, uint*)                                                                              
API_VALIDATE(GetAuxInfo,                                    sint,             uint, vptr, size_t)

// Modules 
API_VALIDATE(IsLoadedByLdr,                                 bool,             void)
API_VALIDATE(IsDynamicLib,                                  bool,             void)                                                                            
API_VALIDATE(GetModuleBase,                                 vptr,             void)
API_VALIDATE(GetModuleSize,                               size_t,             void)
API_VALIDATE(GetModulePath,                                 sint,             achar*, size_t)
   
// Init                                                                                                                                                              
API_VALIDATE(InitStartupInfo,                               sint,             vptr, vptr, vptr, vptr)         // ???
API_VALIDATE(Initialize,                                    sint,             vptr, vptr, vptr, vptr, bool)

// Threads
API_VALIDATE(GetThreadSelf,                        NTHD::SThCtx*,             void)
API_VALIDATE(GetThreadByID,                        NTHD::SThCtx*,             uint)
API_VALIDATE(GetThreadByAddr,                      NTHD::SThCtx*,             vptr)
API_VALIDATE(GetThreadByHandle,                    NTHD::SThCtx*,             uint)
//API_VALIDATE(GetNextThread,               NTHD::SThCtx*,    ???)

//------------------------------------------------------------------------------------------------------------
//                                   Platform System Functions
//------------------------------------------------------------------------------------------------------------
// Memory
API_VALIDATE(NAPI::mmap,                               PX::PVOID,             vptr, size_t, uint, uint, PX::fdsc_t, uint64) 
API_VALIDATE(NAPI::munmap,                                   int,             vptr, size_t)
API_VALIDATE(NAPI::mremap,                             PX::PVOID,             vptr, size_t, size_t, int, vptr) 
API_VALIDATE(NAPI::madvise,                                  int,             vptr, size_t, PX::EMadv)                                                          
API_VALIDATE(NAPI::mprotect,                                 int,             vptr, size_t, uint32, uint32*) 
API_VALIDATE(NAPI::munlock,                                  int,             vptr, size_t) 
API_VALIDATE(NAPI::mlock,                                    int,             vptr, size_t)
API_VALIDATE(NAPI::msync,                                    int,             vptr, size_t, int) 
 
 /*  API_MUSTMATCH  API_MUSTMATCH  API_MUSTMATCH
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
API_VALIDATE(NAPI::munmap,  ) 
          */

//------------------------------------------------------------------------------------------------------------
//                                   Platform System Functions
//------------------------------------------------------------------------------------------------------------
//============================================================================================================
// How to validate NAPI with all those templates and argument forwarding? // TODO: Get rid of the templates and just copy declarations from PX
