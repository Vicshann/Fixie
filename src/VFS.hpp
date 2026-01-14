
// Mount FS providers in a single virtual file system
// The providers may be a real FS directories, package files or something specific (like a browser's persistent storage)
// EntTy: FS_LNK: Link is on FS provider. VFS_MOUNT: This directory is mounted by VFS manager 
struct NVFS
{
  // SetRootDir    // Read and Write paths separately    // Write path have priority over Read path
  // RegisterDir   // Same as RegisterPkg
  // RegisterPkg   // RO: Writing will fail; RW: Writing will write into a copy relative to writing root
};
