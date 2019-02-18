/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTarget_h
#define cmTarget_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cmAlgorithms.h"
#include "cmCustomCommand.h"
#include "cmListFileCache.h"
#include "cmPolicies.h"
#include "cmPropertyMap.h"
#include "cmStateTypes.h"
#include "cmTargetLinkLibraryType.h"

class cmGlobalGenerator;
class cmMakefile;
class cmMessenger;
class cmSourceFile;
class cmTargetInternals;

class cmTargetInternalPointer
{
public:
  cmTargetInternalPointer();
  cmTargetInternalPointer(cmTargetInternalPointer const& r);
  ~cmTargetInternalPointer();
  cmTargetInternalPointer& operator=(cmTargetInternalPointer const& r);
  cmTargetInternals* operator->() const { return this->Pointer; }
  cmTargetInternals* Get() const { return this->Pointer; }

private:
  cmTargetInternals* Pointer;
};

/** \class cmTarget
 * \brief Represent a library or executable target loaded from a makefile.
 *
 * cmTarget represents a target loaded from
 * a makefile.
 */
class cmTarget
{
public:
  enum Visibility
  {
    VisibilityNormal,
    VisibilityImported,
    VisibilityImportedGlobally
  };

  cmTarget(std::string const& name, cmStateEnums::TargetType type,
           Visibility vis, cmMakefile* mf);

  enum CustomCommandType
  {
    PRE_BUILD,
    PRE_LINK,
    POST_BUILD
  };

  /**
   * Return the type of target.
   */
  cmStateEnums::TargetType GetType() const { return this->TargetTypeValue; }

  cmGlobalGenerator* GetGlobalGenerator() const;

  ///! Set/Get the name of the target
  const std::string& GetName() const { return this->Name; }

  /** Get the cmMakefile that owns this target.  */
  cmMakefile* GetMakefile() const { return this->Makefile; }

#define DECLARE_TARGET_POLICY(POLICY)                                         \
  cmPolicies::PolicyStatus GetPolicyStatus##POLICY() const                    \
  {                                                                           \
    return this->PolicyMap.Get(cmPolicies::POLICY);                           \
  }

  CM_FOR_EACH_TARGET_POLICY(DECLARE_TARGET_POLICY)

#undef DECLARE_TARGET_POLICY

  /**
   * Get the list of the custom commands for this target
   */
  std::vector<cmCustomCommand> const& GetPreBuildCommands() const
  {
    return this->PreBuildCommands;
  }
  std::vector<cmCustomCommand> const& GetPreLinkCommands() const
  {
    return this->PreLinkCommands;
  }
  std::vector<cmCustomCommand> const& GetPostBuildCommands() const
  {
    return this->PostBuildCommands;
  }
  void AddPreBuildCommand(cmCustomCommand const& cmd)
  {
    this->PreBuildCommands.push_back(cmd);
  }
  void AddPreLinkCommand(cmCustomCommand const& cmd)
  {
    this->PreLinkCommands.push_back(cmd);
  }
  void AddPostBuildCommand(cmCustomCommand const& cmd)
  {
    this->PostBuildCommands.push_back(cmd);
  }

  /**
   * Add sources to the target.
   */
  void AddSources(std::vector<std::string> const& srcs);
  void AddTracedSources(std::vector<std::string> const& srcs);
  cmSourceFile* AddSourceCMP0049(const std::string& src);
  cmSourceFile* AddSource(const std::string& src);

  //* how we identify a library, by name and type
  typedef std::pair<std::string, cmTargetLinkLibraryType> LibraryID;

  typedef std::vector<LibraryID> LinkLibraryVectorType;
  const LinkLibraryVectorType& GetOriginalLinkLibraries() const
  {
    return this->OriginalLinkLibraries;
  }

  /**
   * Clear the dependency information recorded for this target, if any.
   */
  void ClearDependencyInformation(cmMakefile& mf);

  void AddLinkLibrary(cmMakefile& mf, const std::string& lib,
                      cmTargetLinkLibraryType llt);
  void AddLinkLibrary(cmMakefile& mf, std::string const& lib,
                      std::string const& libRef, cmTargetLinkLibraryType llt);

  enum TLLSignature
  {
    KeywordTLLSignature,
    PlainTLLSignature
  };
  bool PushTLLCommandTrace(TLLSignature signature,
                           cmListFileContext const& lfc);
  void GetTllSignatureTraces(std::ostream& s, TLLSignature sig) const;

  /**
   * Set the path where this target should be installed. This is relative to
   * INSTALL_PREFIX
   */
  std::string GetInstallPath() const { return this->InstallPath; }
  void SetInstallPath(const char* name) { this->InstallPath = name; }

  /**
   * Set the path where this target (if it has a runtime part) should be
   * installed. This is relative to INSTALL_PREFIX
   */
  std::string GetRuntimeInstallPath() const
  {
    return this->RuntimeInstallPath;
  }
  void SetRuntimeInstallPath(const char* name)
  {
    this->RuntimeInstallPath = name;
  }

  /**
   * Get/Set whether there is an install rule for this target.
   */
  bool GetHaveInstallRule() const { return this->HaveInstallRule; }
  void SetHaveInstallRule(bool h) { this->HaveInstallRule = h; }

  /**
   * Get/Set whether this target was auto-created by a generator.
   */
  bool GetIsGeneratorProvided() const { return this->IsGeneratorProvided; }
  void SetIsGeneratorProvided(bool igp) { this->IsGeneratorProvided = igp; }

  /** Add a utility on which this project depends. A utility is an executable
   * name as would be specified to the ADD_EXECUTABLE or UTILITY_SOURCE
   * commands. It is not a full path nor does it have an extension.
   */
  void AddUtility(const std::string& u, cmMakefile* makefile = nullptr);
  ///! Get the utilities used by this target
  std::set<std::string> const& GetUtilities() const { return this->Utilities; }
  cmListFileBacktrace const* GetUtilityBacktrace(const std::string& u) const;

  ///! Set/Get a property of this target file
  void SetProperty(const std::string& prop, const char* value);
  void AppendProperty(const std::string& prop, const char* value,
                      bool asString = false);
  ///! Might return a nullptr if the property is not set or invalid
  const char* GetProperty(const std::string& prop) const;
  ///! Always returns a valid pointer
  const char* GetSafeProperty(const std::string& prop) const;
  bool GetPropertyAsBool(const std::string& prop) const;
  void CheckProperty(const std::string& prop, cmMakefile* context) const;
  const char* GetComputedProperty(const std::string& prop,
                                  cmMessenger* messenger,
                                  cmListFileBacktrace const& context) const;

  bool IsImported() const { return this->IsImportedTarget; }
  bool IsImportedGloballyVisible() const
  {
    return this->ImportedGloballyVisible;
  }

  // Get the properties
  cmPropertyMap const& GetProperties() const { return this->Properties; }

  bool GetMappedConfig(std::string const& desired_config, const char** loc,
                       const char** imp, std::string& suffix) const;

  /** Return whether this target is an executable with symbol exports
      enabled.  */
  bool IsExecutableWithExports() const;

  /** Return whether this target is a shared library Framework on
      Apple.  */
  bool IsFrameworkOnApple() const;

  /** Return whether this target is an executable Bundle on Apple.  */
  bool IsAppBundleOnApple() const;

  /** Get a backtrace from the creation of the target.  */
  cmListFileBacktrace const& GetBacktrace() const;

  void InsertInclude(std::string const& entry, cmListFileBacktrace const& bt,
                     bool before = false);
  void InsertCompileOption(std::string const& entry,
                           cmListFileBacktrace const& bt, bool before = false);
  void InsertCompileDefinition(std::string const& entry,
                               cmListFileBacktrace const& bt);
  void InsertLinkOption(std::string const& entry,
                        cmListFileBacktrace const& bt, bool before = false);
  void InsertLinkDirectory(std::string const& entry,
                           cmListFileBacktrace const& bt, bool before = false);

  void AppendBuildInterfaceIncludes();

  std::string GetDebugGeneratorExpressions(const std::string& value,
                                           cmTargetLinkLibraryType llt) const;

  void AddSystemIncludeDirectories(const std::set<std::string>& incs);
  std::set<std::string> const& GetSystemIncludeDirectories() const
  {
    return this->SystemIncludeDirectories;
  }

  cmStringRange GetIncludeDirectoriesEntries() const;
  cmBacktraceRange GetIncludeDirectoriesBacktraces() const;

  cmStringRange GetCompileOptionsEntries() const;
  cmBacktraceRange GetCompileOptionsBacktraces() const;

  cmStringRange GetCompileFeaturesEntries() const;
  cmBacktraceRange GetCompileFeaturesBacktraces() const;

  cmStringRange GetCompileDefinitionsEntries() const;
  cmBacktraceRange GetCompileDefinitionsBacktraces() const;

  cmStringRange GetSourceEntries() const;
  cmBacktraceRange GetSourceBacktraces() const;

  cmStringRange GetLinkOptionsEntries() const;
  cmBacktraceRange GetLinkOptionsBacktraces() const;

  cmStringRange GetLinkDirectoriesEntries() const;
  cmBacktraceRange GetLinkDirectoriesBacktraces() const;

  cmStringRange GetLinkImplementationEntries() const;
  cmBacktraceRange GetLinkImplementationBacktraces() const;

  struct StrictTargetComparison
  {
    bool operator()(cmTarget const* t1, cmTarget const* t2) const;
  };

  std::string ImportedGetFullPath(const std::string& config,
                                  cmStateEnums::ArtifactType artifact) const;

private:
  const char* GetSuffixVariableInternal(
    cmStateEnums::ArtifactType artifact) const;
  const char* GetPrefixVariableInternal(
    cmStateEnums::ArtifactType artifact) const;

  // Use a makefile variable to set a default for the given property.
  // If the variable is not defined use the given default instead.
  void SetPropertyDefault(const std::string& property,
                          const char* default_value);

  bool CheckImportedLibName(std::string const& prop,
                            std::string const& value) const;

private:
  bool IsGeneratorProvided;
  cmPropertyMap Properties;
  std::set<std::string> SystemIncludeDirectories;
  std::set<std::string> Utilities;
  std::map<std::string, cmListFileBacktrace> UtilityBacktraces;
  cmPolicies::PolicyMap PolicyMap;
  std::string Name;
  std::string InstallPath;
  std::string RuntimeInstallPath;
  std::vector<cmCustomCommand> PreBuildCommands;
  std::vector<cmCustomCommand> PreLinkCommands;
  std::vector<cmCustomCommand> PostBuildCommands;
  std::vector<std::pair<TLLSignature, cmListFileContext>> TLLCommands;
  LinkLibraryVectorType OriginalLinkLibraries;
  cmMakefile* Makefile;
  cmTargetInternalPointer Internal;
  cmStateEnums::TargetType TargetTypeValue;
  bool HaveInstallRule;
  bool DLLPlatform;
  bool IsAndroid;
  bool IsImportedTarget;
  bool ImportedGloballyVisible;
  bool BuildInterfaceIncludesAppended;

  std::string ProcessSourceItemCMP0049(const std::string& s);

  /** Return whether or not the target has a DLL import library.  */
  bool HasImportLibrary() const;

  // Internal representation details.
  friend class cmTargetInternals;
  friend class cmGeneratorTarget;
  friend class cmTargetTraceDependencies;

  cmListFileBacktrace Backtrace;
};

typedef std::unordered_map<std::string, cmTarget> cmTargets;

class cmTargetSet : public std::set<std::string>
{
};
class cmTargetManifest : public std::map<std::string, cmTargetSet>
{
};

#endif
