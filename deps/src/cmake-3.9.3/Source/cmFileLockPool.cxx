/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFileLockPool.h"

#include <assert.h>

#include "cmAlgorithms.h"
#include "cmFileLock.h"
#include "cmFileLockResult.h"

cmFileLockPool::cmFileLockPool()
{
}

cmFileLockPool::~cmFileLockPool()
{
  cmDeleteAll(this->FunctionScopes);
  cmDeleteAll(this->FileScopes);
}

void cmFileLockPool::PushFunctionScope()
{
  this->FunctionScopes.push_back(new ScopePool());
}

void cmFileLockPool::PopFunctionScope()
{
  assert(!this->FunctionScopes.empty());
  delete this->FunctionScopes.back();
  this->FunctionScopes.pop_back();
}

void cmFileLockPool::PushFileScope()
{
  this->FileScopes.push_back(new ScopePool());
}

void cmFileLockPool::PopFileScope()
{
  assert(!this->FileScopes.empty());
  delete this->FileScopes.back();
  this->FileScopes.pop_back();
}

cmFileLockResult cmFileLockPool::LockFunctionScope(const std::string& filename,
                                                   unsigned long timeoutSec)
{
  if (this->IsAlreadyLocked(filename)) {
    return cmFileLockResult::MakeAlreadyLocked();
  }
  if (this->FunctionScopes.empty()) {
    return cmFileLockResult::MakeNoFunction();
  }
  return this->FunctionScopes.back()->Lock(filename, timeoutSec);
}

cmFileLockResult cmFileLockPool::LockFileScope(const std::string& filename,
                                               unsigned long timeoutSec)
{
  if (this->IsAlreadyLocked(filename)) {
    return cmFileLockResult::MakeAlreadyLocked();
  }
  assert(!this->FileScopes.empty());
  return this->FileScopes.back()->Lock(filename, timeoutSec);
}

cmFileLockResult cmFileLockPool::LockProcessScope(const std::string& filename,
                                                  unsigned long timeoutSec)
{
  if (this->IsAlreadyLocked(filename)) {
    return cmFileLockResult::MakeAlreadyLocked();
  }
  return this->ProcessScope.Lock(filename, timeoutSec);
}

cmFileLockResult cmFileLockPool::Release(const std::string& filename)
{
  for (It i = this->FunctionScopes.begin(); i != this->FunctionScopes.end();
       ++i) {
    const cmFileLockResult result = (*i)->Release(filename);
    if (!result.IsOk()) {
      return result;
    }
  }

  for (It i = this->FileScopes.begin(); i != this->FileScopes.end(); ++i) {
    const cmFileLockResult result = (*i)->Release(filename);
    if (!result.IsOk()) {
      return result;
    }
  }

  return this->ProcessScope.Release(filename);
}

bool cmFileLockPool::IsAlreadyLocked(const std::string& filename) const
{
  for (CIt i = this->FunctionScopes.begin(); i != this->FunctionScopes.end();
       ++i) {
    const bool result = (*i)->IsAlreadyLocked(filename);
    if (result) {
      return true;
    }
  }

  for (CIt i = this->FileScopes.begin(); i != this->FileScopes.end(); ++i) {
    const bool result = (*i)->IsAlreadyLocked(filename);
    if (result) {
      return true;
    }
  }

  return this->ProcessScope.IsAlreadyLocked(filename);
}

cmFileLockPool::ScopePool::ScopePool()
{
}

cmFileLockPool::ScopePool::~ScopePool()
{
  cmDeleteAll(this->Locks);
}

cmFileLockResult cmFileLockPool::ScopePool::Lock(const std::string& filename,
                                                 unsigned long timeoutSec)
{
  cmFileLock* lock = new cmFileLock();
  const cmFileLockResult result = lock->Lock(filename, timeoutSec);
  if (result.IsOk()) {
    this->Locks.push_back(lock);
    return cmFileLockResult::MakeOk();
  }
  delete lock;
  return result;
}

cmFileLockResult cmFileLockPool::ScopePool::Release(
  const std::string& filename)
{
  for (It i = this->Locks.begin(); i != this->Locks.end(); ++i) {
    if ((*i)->IsLocked(filename)) {
      return (*i)->Release();
    }
  }
  return cmFileLockResult::MakeOk();
}

bool cmFileLockPool::ScopePool::IsAlreadyLocked(
  const std::string& filename) const
{
  for (CIt i = this->Locks.begin(); i != this->Locks.end(); ++i) {
    if ((*i)->IsLocked(filename)) {
      return true;
    }
  }
  return false;
}
