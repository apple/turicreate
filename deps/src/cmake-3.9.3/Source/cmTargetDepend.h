/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTargetDepend_h
#define cmTargetDepend_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <set>

class cmGeneratorTarget;

/** One edge in the global target dependency graph.
    It may be marked as a 'link' or 'util' edge or both.  */
class cmTargetDepend
{
  cmGeneratorTarget const* Target;

  // The set order depends only on the Target, so we use
  // mutable members to acheive a map with set syntax.
  mutable bool Link;
  mutable bool Util;

public:
  cmTargetDepend(cmGeneratorTarget const* t)
    : Target(t)
    , Link(false)
    , Util(false)
  {
  }
  operator cmGeneratorTarget const*() const { return this->Target; }
  cmGeneratorTarget const* operator->() const { return this->Target; }
  cmGeneratorTarget const& operator*() const { return *this->Target; }
  friend bool operator<(cmTargetDepend l, cmTargetDepend r)
  {
    return l.Target < r.Target;
  }
  void SetType(bool strong) const
  {
    if (strong) {
      this->Util = true;
    } else {
      this->Link = true;
    }
  }
  bool IsLink() const { return this->Link; }
  bool IsUtil() const { return this->Util; }
};

/** Unordered set of (direct) dependencies of a target. */
class cmTargetDependSet : public std::set<cmTargetDepend>
{
};

#endif
