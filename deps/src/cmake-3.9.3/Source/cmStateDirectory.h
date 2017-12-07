/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmStateDirectory_h
#define cmStateDirectory_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmAlgorithms.h"
#include "cmLinkedTree.h"
#include "cmListFileCache.h"
#include "cmStatePrivate.h"
#include "cmStateSnapshot.h"

class cmStateDirectory
{
  cmStateDirectory(
    cmLinkedTree<cmStateDetail::BuildsystemDirectoryStateType>::iterator iter,
    cmStateSnapshot const& snapshot);

public:
  const char* GetCurrentSource() const;
  void SetCurrentSource(std::string const& dir);
  const char* GetCurrentBinary() const;
  void SetCurrentBinary(std::string const& dir);

  const char* GetRelativePathTopSource() const;
  const char* GetRelativePathTopBinary() const;
  void SetRelativePathTopSource(const char* dir);
  void SetRelativePathTopBinary(const char* dir);

  cmStringRange GetIncludeDirectoriesEntries() const;
  cmBacktraceRange GetIncludeDirectoriesEntryBacktraces() const;
  void AppendIncludeDirectoriesEntry(std::string const& vec,
                                     cmListFileBacktrace const& lfbt);
  void PrependIncludeDirectoriesEntry(std::string const& vec,
                                      cmListFileBacktrace const& lfbt);
  void SetIncludeDirectories(std::string const& vec,
                             cmListFileBacktrace const& lfbt);
  void ClearIncludeDirectories();

  cmStringRange GetCompileDefinitionsEntries() const;
  cmBacktraceRange GetCompileDefinitionsEntryBacktraces() const;
  void AppendCompileDefinitionsEntry(std::string const& vec,
                                     cmListFileBacktrace const& lfbt);
  void SetCompileDefinitions(std::string const& vec,
                             cmListFileBacktrace const& lfbt);
  void ClearCompileDefinitions();

  cmStringRange GetCompileOptionsEntries() const;
  cmBacktraceRange GetCompileOptionsEntryBacktraces() const;
  void AppendCompileOptionsEntry(std::string const& vec,
                                 cmListFileBacktrace const& lfbt);
  void SetCompileOptions(std::string const& vec,
                         cmListFileBacktrace const& lfbt);
  void ClearCompileOptions();

  void SetProperty(const std::string& prop, const char* value,
                   cmListFileBacktrace const& lfbt);
  void AppendProperty(const std::string& prop, const char* value,
                      bool asString, cmListFileBacktrace const& lfbt);
  const char* GetProperty(const std::string& prop) const;
  const char* GetProperty(const std::string& prop, bool chain) const;
  bool GetPropertyAsBool(const std::string& prop) const;
  std::vector<std::string> GetPropertyKeys() const;

  void AddNormalTargetName(std::string const& name);

private:
  void ComputeRelativePathTopSource();
  void ComputeRelativePathTopBinary();

private:
  cmLinkedTree<cmStateDetail::BuildsystemDirectoryStateType>::iterator
    DirectoryState;
  cmStateSnapshot Snapshot_;
  friend class cmStateSnapshot;
};

#endif
