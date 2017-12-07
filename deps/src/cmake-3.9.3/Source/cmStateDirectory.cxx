/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmStateDirectory.h"

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <map>
#include <utility>

#include "cmProperty.h"
#include "cmPropertyMap.h"
#include "cmState.h"
#include "cmStatePrivate.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

static std::string const kBINARY_DIR = "BINARY_DIR";
static std::string const kBUILDSYSTEM_TARGETS = "BUILDSYSTEM_TARGETS";
static std::string const kSOURCE_DIR = "SOURCE_DIR";
static std::string const kSUBDIRECTORIES = "SUBDIRECTORIES";

void cmStateDirectory::ComputeRelativePathTopSource()
{
  // Relative path conversion inside the source tree is not used to
  // construct relative paths passed to build tools so it is safe to use
  // even when the source is a network path.

  cmStateSnapshot snapshot = this->Snapshot_;
  std::vector<cmStateSnapshot> snapshots;
  snapshots.push_back(snapshot);
  while (true) {
    snapshot = snapshot.GetBuildsystemDirectoryParent();
    if (snapshot.IsValid()) {
      snapshots.push_back(snapshot);
    } else {
      break;
    }
  }

  std::string result = snapshots.front().GetDirectory().GetCurrentSource();

  for (std::vector<cmStateSnapshot>::const_iterator it = snapshots.begin() + 1;
       it != snapshots.end(); ++it) {
    std::string currentSource = it->GetDirectory().GetCurrentSource();
    if (cmSystemTools::IsSubDirectory(result, currentSource)) {
      result = currentSource;
    }
  }
  this->DirectoryState->RelativePathTopSource = result;
}

void cmStateDirectory::ComputeRelativePathTopBinary()
{
  cmStateSnapshot snapshot = this->Snapshot_;
  std::vector<cmStateSnapshot> snapshots;
  snapshots.push_back(snapshot);
  while (true) {
    snapshot = snapshot.GetBuildsystemDirectoryParent();
    if (snapshot.IsValid()) {
      snapshots.push_back(snapshot);
    } else {
      break;
    }
  }

  std::string result = snapshots.front().GetDirectory().GetCurrentBinary();

  for (std::vector<cmStateSnapshot>::const_iterator it = snapshots.begin() + 1;
       it != snapshots.end(); ++it) {
    std::string currentBinary = it->GetDirectory().GetCurrentBinary();
    if (cmSystemTools::IsSubDirectory(result, currentBinary)) {
      result = currentBinary;
    }
  }

  // The current working directory on Windows cannot be a network
  // path.  Therefore relative paths cannot work when the binary tree
  // is a network path.
  if (result.size() < 2 || result.substr(0, 2) != "//") {
    this->DirectoryState->RelativePathTopBinary = result;
  } else {
    this->DirectoryState->RelativePathTopBinary = "";
  }
}

const char* cmStateDirectory::GetCurrentSource() const
{
  return this->DirectoryState->Location.c_str();
}

void cmStateDirectory::SetCurrentSource(std::string const& dir)
{
  std::string& loc = this->DirectoryState->Location;
  loc = dir;
  cmSystemTools::ConvertToUnixSlashes(loc);
  loc = cmSystemTools::CollapseFullPath(loc);

  this->ComputeRelativePathTopSource();

  this->Snapshot_.SetDefinition("CMAKE_CURRENT_SOURCE_DIR", loc);
}

const char* cmStateDirectory::GetCurrentBinary() const
{
  return this->DirectoryState->OutputLocation.c_str();
}

void cmStateDirectory::SetCurrentBinary(std::string const& dir)
{
  std::string& loc = this->DirectoryState->OutputLocation;
  loc = dir;
  cmSystemTools::ConvertToUnixSlashes(loc);
  loc = cmSystemTools::CollapseFullPath(loc);

  this->ComputeRelativePathTopBinary();

  this->Snapshot_.SetDefinition("CMAKE_CURRENT_BINARY_DIR", loc);
}

const char* cmStateDirectory::GetRelativePathTopSource() const
{
  return this->DirectoryState->RelativePathTopSource.c_str();
}

const char* cmStateDirectory::GetRelativePathTopBinary() const
{
  return this->DirectoryState->RelativePathTopBinary.c_str();
}

void cmStateDirectory::SetRelativePathTopSource(const char* dir)
{
  this->DirectoryState->RelativePathTopSource = dir;
}

void cmStateDirectory::SetRelativePathTopBinary(const char* dir)
{
  this->DirectoryState->RelativePathTopBinary = dir;
}

cmStateDirectory::cmStateDirectory(
  cmLinkedTree<cmStateDetail::BuildsystemDirectoryStateType>::iterator iter,
  const cmStateSnapshot& snapshot)
  : DirectoryState(iter)
  , Snapshot_(snapshot)
{
}

template <typename T, typename U>
cmStringRange GetPropertyContent(T const& content, U contentEndPosition)
{
  std::vector<std::string>::const_iterator end =
    content.begin() + contentEndPosition;

  std::vector<std::string>::const_reverse_iterator rbegin =
    cmMakeReverseIterator(end);
  rbegin = std::find(rbegin, content.rend(), cmPropertySentinal);

  return cmMakeRange(rbegin.base(), end);
}

template <typename T, typename U, typename V>
cmBacktraceRange GetPropertyBacktraces(T const& content, U const& backtraces,
                                       V contentEndPosition)
{
  std::vector<std::string>::const_iterator entryEnd =
    content.begin() + contentEndPosition;

  std::vector<std::string>::const_reverse_iterator rbegin =
    cmMakeReverseIterator(entryEnd);
  rbegin = std::find(rbegin, content.rend(), cmPropertySentinal);

  std::vector<cmListFileBacktrace>::const_iterator it =
    backtraces.begin() + std::distance(content.begin(), rbegin.base());

  std::vector<cmListFileBacktrace>::const_iterator end = backtraces.end();
  return cmMakeRange(it, end);
}

template <typename T, typename U, typename V>
void AppendEntry(T& content, U& backtraces, V& endContentPosition,
                 const std::string& value, const cmListFileBacktrace& lfbt)
{
  if (value.empty()) {
    return;
  }

  assert(endContentPosition == content.size());

  content.push_back(value);
  backtraces.push_back(lfbt);

  endContentPosition = content.size();
}

template <typename T, typename U, typename V>
void SetContent(T& content, U& backtraces, V& endContentPosition,
                const std::string& vec, const cmListFileBacktrace& lfbt)
{
  assert(endContentPosition == content.size());

  content.resize(content.size() + 2);
  backtraces.resize(backtraces.size() + 2);

  content.back() = vec;
  backtraces.back() = lfbt;

  endContentPosition = content.size();
}

template <typename T, typename U, typename V>
void ClearContent(T& content, U& backtraces, V& endContentPosition)
{
  assert(endContentPosition == content.size());

  content.resize(content.size() + 1);
  backtraces.resize(backtraces.size() + 1);

  endContentPosition = content.size();
}

cmStringRange cmStateDirectory::GetIncludeDirectoriesEntries() const
{
  return GetPropertyContent(
    this->DirectoryState->IncludeDirectories,
    this->Snapshot_.Position->IncludeDirectoryPosition);
}

cmBacktraceRange cmStateDirectory::GetIncludeDirectoriesEntryBacktraces() const
{
  return GetPropertyBacktraces(
    this->DirectoryState->IncludeDirectories,
    this->DirectoryState->IncludeDirectoryBacktraces,
    this->Snapshot_.Position->IncludeDirectoryPosition);
}

void cmStateDirectory::AppendIncludeDirectoriesEntry(
  const std::string& vec, const cmListFileBacktrace& lfbt)
{
  AppendEntry(this->DirectoryState->IncludeDirectories,
              this->DirectoryState->IncludeDirectoryBacktraces,
              this->Snapshot_.Position->IncludeDirectoryPosition, vec, lfbt);
}

void cmStateDirectory::PrependIncludeDirectoriesEntry(
  const std::string& vec, const cmListFileBacktrace& lfbt)
{
  std::vector<std::string>::iterator entryEnd =
    this->DirectoryState->IncludeDirectories.begin() +
    this->Snapshot_.Position->IncludeDirectoryPosition;

  std::vector<std::string>::reverse_iterator rend =
    this->DirectoryState->IncludeDirectories.rend();
  std::vector<std::string>::reverse_iterator rbegin =
    cmMakeReverseIterator(entryEnd);
  rbegin = std::find(rbegin, rend, cmPropertySentinal);

  std::vector<std::string>::iterator entryIt = rbegin.base();
  std::vector<std::string>::iterator entryBegin =
    this->DirectoryState->IncludeDirectories.begin();

  std::vector<cmListFileBacktrace>::iterator btIt =
    this->DirectoryState->IncludeDirectoryBacktraces.begin() +
    std::distance(entryBegin, entryIt);

  this->DirectoryState->IncludeDirectories.insert(entryIt, vec);
  this->DirectoryState->IncludeDirectoryBacktraces.insert(btIt, lfbt);

  this->Snapshot_.Position->IncludeDirectoryPosition =
    this->DirectoryState->IncludeDirectories.size();
}

void cmStateDirectory::SetIncludeDirectories(const std::string& vec,
                                             const cmListFileBacktrace& lfbt)
{
  SetContent(this->DirectoryState->IncludeDirectories,
             this->DirectoryState->IncludeDirectoryBacktraces,
             this->Snapshot_.Position->IncludeDirectoryPosition, vec, lfbt);
}

void cmStateDirectory::ClearIncludeDirectories()
{
  ClearContent(this->DirectoryState->IncludeDirectories,
               this->DirectoryState->IncludeDirectoryBacktraces,
               this->Snapshot_.Position->IncludeDirectoryPosition);
}

cmStringRange cmStateDirectory::GetCompileDefinitionsEntries() const
{
  return GetPropertyContent(
    this->DirectoryState->CompileDefinitions,
    this->Snapshot_.Position->CompileDefinitionsPosition);
}

cmBacktraceRange cmStateDirectory::GetCompileDefinitionsEntryBacktraces() const
{
  return GetPropertyBacktraces(
    this->DirectoryState->CompileDefinitions,
    this->DirectoryState->CompileDefinitionsBacktraces,
    this->Snapshot_.Position->CompileDefinitionsPosition);
}

void cmStateDirectory::AppendCompileDefinitionsEntry(
  const std::string& vec, const cmListFileBacktrace& lfbt)
{
  AppendEntry(this->DirectoryState->CompileDefinitions,
              this->DirectoryState->CompileDefinitionsBacktraces,
              this->Snapshot_.Position->CompileDefinitionsPosition, vec, lfbt);
}

void cmStateDirectory::SetCompileDefinitions(const std::string& vec,
                                             const cmListFileBacktrace& lfbt)
{
  SetContent(this->DirectoryState->CompileDefinitions,
             this->DirectoryState->CompileDefinitionsBacktraces,
             this->Snapshot_.Position->CompileDefinitionsPosition, vec, lfbt);
}

void cmStateDirectory::ClearCompileDefinitions()
{
  ClearContent(this->DirectoryState->CompileDefinitions,
               this->DirectoryState->CompileDefinitionsBacktraces,
               this->Snapshot_.Position->CompileDefinitionsPosition);
}

cmStringRange cmStateDirectory::GetCompileOptionsEntries() const
{
  return GetPropertyContent(this->DirectoryState->CompileOptions,
                            this->Snapshot_.Position->CompileOptionsPosition);
}

cmBacktraceRange cmStateDirectory::GetCompileOptionsEntryBacktraces() const
{
  return GetPropertyBacktraces(
    this->DirectoryState->CompileOptions,
    this->DirectoryState->CompileOptionsBacktraces,
    this->Snapshot_.Position->CompileOptionsPosition);
}

void cmStateDirectory::AppendCompileOptionsEntry(
  const std::string& vec, const cmListFileBacktrace& lfbt)
{
  AppendEntry(this->DirectoryState->CompileOptions,
              this->DirectoryState->CompileOptionsBacktraces,
              this->Snapshot_.Position->CompileOptionsPosition, vec, lfbt);
}

void cmStateDirectory::SetCompileOptions(const std::string& vec,
                                         const cmListFileBacktrace& lfbt)
{
  SetContent(this->DirectoryState->CompileOptions,
             this->DirectoryState->CompileOptionsBacktraces,
             this->Snapshot_.Position->CompileOptionsPosition, vec, lfbt);
}

void cmStateDirectory::ClearCompileOptions()
{
  ClearContent(this->DirectoryState->CompileOptions,
               this->DirectoryState->CompileOptionsBacktraces,
               this->Snapshot_.Position->CompileOptionsPosition);
}

void cmStateDirectory::SetProperty(const std::string& prop, const char* value,
                                   cmListFileBacktrace const& lfbt)
{
  if (prop == "INCLUDE_DIRECTORIES") {
    if (!value) {
      this->ClearIncludeDirectories();
      return;
    }
    this->SetIncludeDirectories(value, lfbt);
    return;
  }
  if (prop == "COMPILE_OPTIONS") {
    if (!value) {
      this->ClearCompileOptions();
      return;
    }
    this->SetCompileOptions(value, lfbt);
    return;
  }
  if (prop == "COMPILE_DEFINITIONS") {
    if (!value) {
      this->ClearCompileDefinitions();
      return;
    }
    this->SetCompileDefinitions(value, lfbt);
    return;
  }

  this->DirectoryState->Properties.SetProperty(prop, value);
}

void cmStateDirectory::AppendProperty(const std::string& prop,
                                      const char* value, bool asString,
                                      cmListFileBacktrace const& lfbt)
{
  if (prop == "INCLUDE_DIRECTORIES") {
    this->AppendIncludeDirectoriesEntry(value, lfbt);
    return;
  }
  if (prop == "COMPILE_OPTIONS") {
    this->AppendCompileOptionsEntry(value, lfbt);
    return;
  }
  if (prop == "COMPILE_DEFINITIONS") {
    this->AppendCompileDefinitionsEntry(value, lfbt);
    return;
  }

  this->DirectoryState->Properties.AppendProperty(prop, value, asString);
}

const char* cmStateDirectory::GetProperty(const std::string& prop) const
{
  const bool chain =
    this->Snapshot_.State->IsPropertyChained(prop, cmProperty::DIRECTORY);
  return this->GetProperty(prop, chain);
}

const char* cmStateDirectory::GetProperty(const std::string& prop,
                                          bool chain) const
{
  static std::string output;
  output = "";
  if (prop == "PARENT_DIRECTORY") {
    cmStateSnapshot parent = this->Snapshot_.GetBuildsystemDirectoryParent();
    if (parent.IsValid()) {
      return parent.GetDirectory().GetCurrentSource();
    }
    return "";
  }
  if (prop == kBINARY_DIR) {
    output = this->GetCurrentBinary();
    return output.c_str();
  }
  if (prop == kSOURCE_DIR) {
    output = this->GetCurrentSource();
    return output.c_str();
  }
  if (prop == kSUBDIRECTORIES) {
    std::vector<std::string> child_dirs;
    std::vector<cmStateSnapshot> const& children =
      this->DirectoryState->Children;
    for (std::vector<cmStateSnapshot>::const_iterator ci = children.begin();
         ci != children.end(); ++ci) {
      child_dirs.push_back(ci->GetDirectory().GetCurrentSource());
    }
    output = cmJoin(child_dirs, ";");
    return output.c_str();
  }
  if (prop == kBUILDSYSTEM_TARGETS) {
    output = cmJoin(this->DirectoryState->NormalTargetNames, ";");
    return output.c_str();
  }

  if (prop == "LISTFILE_STACK") {
    std::vector<std::string> listFiles;
    cmStateSnapshot snp = this->Snapshot_;
    while (snp.IsValid()) {
      listFiles.push_back(snp.GetExecutionListFile());
      snp = snp.GetCallStackParent();
    }
    std::reverse(listFiles.begin(), listFiles.end());
    output = cmJoin(listFiles, ";");
    return output.c_str();
  }
  if (prop == "CACHE_VARIABLES") {
    output = cmJoin(this->Snapshot_.State->GetCacheEntryKeys(), ";");
    return output.c_str();
  }
  if (prop == "VARIABLES") {
    std::vector<std::string> res = this->Snapshot_.ClosureKeys();
    std::vector<std::string> cacheKeys =
      this->Snapshot_.State->GetCacheEntryKeys();
    res.insert(res.end(), cacheKeys.begin(), cacheKeys.end());
    std::sort(res.begin(), res.end());
    output = cmJoin(res, ";");
    return output.c_str();
  }
  if (prop == "INCLUDE_DIRECTORIES") {
    output = cmJoin(this->GetIncludeDirectoriesEntries(), ";");
    return output.c_str();
  }
  if (prop == "COMPILE_OPTIONS") {
    output = cmJoin(this->GetCompileOptionsEntries(), ";");
    return output.c_str();
  }
  if (prop == "COMPILE_DEFINITIONS") {
    output = cmJoin(this->GetCompileDefinitionsEntries(), ";");
    return output.c_str();
  }

  const char* retVal = this->DirectoryState->Properties.GetPropertyValue(prop);
  if (!retVal && chain) {
    cmStateSnapshot parentSnapshot =
      this->Snapshot_.GetBuildsystemDirectoryParent();
    if (parentSnapshot.IsValid()) {
      return parentSnapshot.GetDirectory().GetProperty(prop, chain);
    }
    return this->Snapshot_.State->GetGlobalProperty(prop);
  }

  return retVal;
}

bool cmStateDirectory::GetPropertyAsBool(const std::string& prop) const
{
  return cmSystemTools::IsOn(this->GetProperty(prop));
}

std::vector<std::string> cmStateDirectory::GetPropertyKeys() const
{
  std::vector<std::string> keys;
  keys.reserve(this->DirectoryState->Properties.size());
  for (cmPropertyMap::const_iterator it =
         this->DirectoryState->Properties.begin();
       it != this->DirectoryState->Properties.end(); ++it) {
    keys.push_back(it->first);
  }
  return keys;
}

void cmStateDirectory::AddNormalTargetName(std::string const& name)
{
  this->DirectoryState->NormalTargetNames.push_back(name);
}
