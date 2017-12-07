/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFileLock_h
#define cmFileLock_h

#include "cmConfigure.h"

#include <string>

#if defined(_WIN32)
#include <windows.h> // HANDLE
#endif

class cmFileLockResult;

/**
  * @brief Cross-platform file locking.
  * @details Under the hood this class use 'fcntl' for Unix-like platforms and
  * 'LockFileEx'/'UnlockFileEx' for Win32 platform. Locks are exclusive and
  * advisory.
  */
class cmFileLock
{
  CM_DISABLE_COPY(cmFileLock)

public:
  cmFileLock();
  ~cmFileLock();

  /**
    * @brief Lock the file.
    * @param timeoutSec Lock timeout. If -1 try until success or fatal error.
    */
  cmFileLockResult Lock(const std::string& filename, unsigned long timeoutSec);

  /**
    * @brief Unlock the file.
    */
  cmFileLockResult Release();

  /**
    * @brief Check file is locked by this class.
    * @details This function helps to find double locks (deadlocks) and to do
    * explicit unlocks.
    */
  bool IsLocked(const std::string& filename) const;

private:
  cmFileLockResult OpenFile();
  cmFileLockResult LockWithoutTimeout();
  cmFileLockResult LockWithTimeout(unsigned long timeoutSec);

#if defined(_WIN32)
  typedef HANDLE FileId;
  BOOL LockFile(DWORD flags);
#else
  typedef int FileId;
  int LockFile(int cmd, int type);
#endif

  FileId File;
  std::string Filename;
};

#endif // cmFileLock_h
