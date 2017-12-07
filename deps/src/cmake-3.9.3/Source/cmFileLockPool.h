/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFileLockPool_h
#define cmFileLockPool_h

#include "cmConfigure.h"

#include <string>
#include <vector>

class cmFileLock;
class cmFileLockResult;

class cmFileLockPool
{
  CM_DISABLE_COPY(cmFileLockPool)

public:
  cmFileLockPool();
  ~cmFileLockPool();

  //@{
  /**
    * @brief Function scope control.
    */
  void PushFunctionScope();
  void PopFunctionScope();
  //@}

  //@{
  /**
    * @brief File scope control.
    */
  void PushFileScope();
  void PopFileScope();
  //@}

  //@{
  /**
    * @brief Lock the file in given scope.
    * @param timeoutSec Lock timeout. If -1 try until success or fatal error.
    */
  cmFileLockResult LockFunctionScope(const std::string& filename,
                                     unsigned long timeoutSec);
  cmFileLockResult LockFileScope(const std::string& filename,
                                 unsigned long timeoutSec);
  cmFileLockResult LockProcessScope(const std::string& filename,
                                    unsigned long timeoutSec);
  //@}

  /**
    * @brief Unlock the file explicitly.
    */
  cmFileLockResult Release(const std::string& filename);

private:
  bool IsAlreadyLocked(const std::string& filename) const;

  class ScopePool
  {
    CM_DISABLE_COPY(ScopePool)

  public:
    ScopePool();
    ~ScopePool();

    cmFileLockResult Lock(const std::string& filename,
                          unsigned long timeoutSec);
    cmFileLockResult Release(const std::string& filename);
    bool IsAlreadyLocked(const std::string& filename) const;

  private:
    typedef std::vector<cmFileLock*> List;
    typedef List::iterator It;
    typedef List::const_iterator CIt;

    List Locks;
  };

  typedef std::vector<ScopePool*> List;

  typedef List::iterator It;
  typedef List::const_iterator CIt;

  List FunctionScopes;
  List FileScopes;
  ScopePool ProcessScope;
};

#endif // cmFileLockPool_h
