/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmStatePrivate_h
#define cmStatePrivate_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmDefinitions.h"
#include "cmLinkedTree.h"
#include "cmListFileCache.h"
#include "cmPolicies.h"
#include "cmPropertyMap.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"

namespace cmStateDetail {
struct BuildsystemDirectoryStateType;
struct PolicyStackEntry;
} // namespace cmStateDetail

static const std::string cmPropertySentinal = std::string();

struct cmStateDetail::SnapshotDataType
{
  cmStateDetail::PositionType ScopeParent;
  cmStateDetail::PositionType DirectoryParent;
  cmLinkedTree<cmStateDetail::PolicyStackEntry>::iterator Policies;
  cmLinkedTree<cmStateDetail::PolicyStackEntry>::iterator PolicyRoot;
  cmLinkedTree<cmStateDetail::PolicyStackEntry>::iterator PolicyScope;
  cmStateEnums::SnapshotType SnapshotType;
  bool Keep;
  cmLinkedTree<std::string>::iterator ExecutionListFile;
  cmLinkedTree<cmStateDetail::BuildsystemDirectoryStateType>::iterator
    BuildSystemDirectory;
  cmLinkedTree<cmDefinitions>::iterator Vars;
  cmLinkedTree<cmDefinitions>::iterator Root;
  cmLinkedTree<cmDefinitions>::iterator Parent;
  std::vector<std::string>::size_type IncludeDirectoryPosition;
  std::vector<std::string>::size_type CompileDefinitionsPosition;
  std::vector<std::string>::size_type CompileOptionsPosition;
};

struct cmStateDetail::PolicyStackEntry : public cmPolicies::PolicyMap
{
  typedef cmPolicies::PolicyMap derived;
  PolicyStackEntry(bool w = false)
    : derived()
    , Weak(w)
  {
  }
  PolicyStackEntry(derived const& d, bool w)
    : derived(d)
    , Weak(w)
  {
  }
  bool Weak;
};

struct cmStateDetail::BuildsystemDirectoryStateType
{
  cmStateDetail::PositionType DirectoryEnd;

  std::string Location;
  std::string OutputLocation;

  // The top-most directories for relative path conversion.  Both the
  // source and destination location of a relative path conversion
  // must be underneath one of these directories (both under source or
  // both under binary) in order for the relative path to be evaluated
  // safely by the build tools.
  std::string RelativePathTopSource;
  std::string RelativePathTopBinary;

  std::vector<std::string> IncludeDirectories;
  std::vector<cmListFileBacktrace> IncludeDirectoryBacktraces;

  std::vector<std::string> CompileDefinitions;
  std::vector<cmListFileBacktrace> CompileDefinitionsBacktraces;

  std::vector<std::string> CompileOptions;
  std::vector<cmListFileBacktrace> CompileOptionsBacktraces;

  std::vector<std::string> NormalTargetNames;

  std::string ProjectName;

  cmPropertyMap Properties;

  std::vector<cmStateSnapshot> Children;
};

#endif
