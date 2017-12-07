/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmState_h
#define cmState_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <set>
#include <string>
#include <vector>

#include "cmDefinitions.h"
#include "cmLinkedTree.h"
#include "cmPolicies.h"
#include "cmProperty.h"
#include "cmPropertyDefinitionMap.h"
#include "cmPropertyMap.h"
#include "cmStatePrivate.h"
#include "cmStateTypes.h"

class cmCacheManager;
class cmCommand;
class cmPropertyDefinition;
class cmStateSnapshot;

class cmState
{
  friend class cmStateSnapshot;

public:
  cmState();
  ~cmState();

  static const char* GetTargetTypeName(cmStateEnums::TargetType targetType);

  cmStateSnapshot CreateBaseSnapshot();
  cmStateSnapshot CreateBuildsystemDirectorySnapshot(
    cmStateSnapshot const& originSnapshot);
  cmStateSnapshot CreateFunctionCallSnapshot(
    cmStateSnapshot const& originSnapshot, std::string const& fileName);
  cmStateSnapshot CreateMacroCallSnapshot(
    cmStateSnapshot const& originSnapshot, std::string const& fileName);
  cmStateSnapshot CreateIncludeFileSnapshot(
    cmStateSnapshot const& originSnapshot, std::string const& fileName);
  cmStateSnapshot CreateVariableScopeSnapshot(
    cmStateSnapshot const& originSnapshot);
  cmStateSnapshot CreateInlineListFileSnapshot(
    cmStateSnapshot const& originSnapshot, std::string const& fileName);
  cmStateSnapshot CreatePolicyScopeSnapshot(
    cmStateSnapshot const& originSnapshot);
  cmStateSnapshot Pop(cmStateSnapshot const& originSnapshot);

  static cmStateEnums::CacheEntryType StringToCacheEntryType(const char*);
  static const char* CacheEntryTypeToString(cmStateEnums::CacheEntryType);
  static bool IsCacheEntryType(std::string const& key);

  bool LoadCache(const std::string& path, bool internal,
                 std::set<std::string>& excludes,
                 std::set<std::string>& includes);

  bool SaveCache(const std::string& path);

  bool DeleteCache(const std::string& path);

  std::vector<std::string> GetCacheEntryKeys() const;
  const char* GetCacheEntryValue(std::string const& key) const;
  const char* GetInitializedCacheValue(std::string const& key) const;
  cmStateEnums::CacheEntryType GetCacheEntryType(std::string const& key) const;
  void SetCacheEntryValue(std::string const& key, std::string const& value);
  void SetCacheValue(std::string const& key, std::string const& value);

  void RemoveCacheEntry(std::string const& key);

  void SetCacheEntryProperty(std::string const& key,
                             std::string const& propertyName,
                             std::string const& value);
  void SetCacheEntryBoolProperty(std::string const& key,
                                 std::string const& propertyName, bool value);
  std::vector<std::string> GetCacheEntryPropertyList(std::string const& key);
  const char* GetCacheEntryProperty(std::string const& key,
                                    std::string const& propertyName);
  bool GetCacheEntryPropertyAsBool(std::string const& key,
                                   std::string const& propertyName);
  void AppendCacheEntryProperty(std::string const& key,
                                const std::string& property,
                                const std::string& value,
                                bool asString = false);
  void RemoveCacheEntryProperty(std::string const& key,
                                std::string const& propertyName);

  ///! Break up a line like VAR:type="value" into var, type and value
  static bool ParseCacheEntry(const std::string& entry, std::string& var,
                              std::string& value,
                              cmStateEnums::CacheEntryType& type);

  cmStateSnapshot Reset();
  // Define a property
  void DefineProperty(const std::string& name, cmProperty::ScopeType scope,
                      const char* ShortDescription,
                      const char* FullDescription, bool chain = false);

  // get property definition
  cmPropertyDefinition const* GetPropertyDefinition(
    const std::string& name, cmProperty::ScopeType scope) const;

  // Is a property defined?
  bool IsPropertyDefined(const std::string& name,
                         cmProperty::ScopeType scope) const;
  bool IsPropertyChained(const std::string& name,
                         cmProperty::ScopeType scope) const;

  void SetLanguageEnabled(std::string const& l);
  bool GetLanguageEnabled(std::string const& l) const;
  std::vector<std::string> GetEnabledLanguages() const;
  void SetEnabledLanguages(std::vector<std::string> const& langs);
  void ClearEnabledLanguages();

  bool GetIsInTryCompile() const;
  void SetIsInTryCompile(bool b);

  bool GetIsGeneratorMultiConfig() const;
  void SetIsGeneratorMultiConfig(bool b);

  cmCommand* GetCommand(std::string const& name) const;
  void AddBuiltinCommand(std::string const& name, cmCommand* command);
  void AddDisallowedCommand(std::string const& name, cmCommand* command,
                            cmPolicies::PolicyID policy, const char* message);
  void AddUnexpectedCommand(std::string const& name, const char* error);
  void AddScriptedCommand(std::string const& name, cmCommand* command);
  void RemoveUserDefinedCommands();
  std::vector<std::string> GetCommandNames() const;

  void SetGlobalProperty(const std::string& prop, const char* value);
  void AppendGlobalProperty(const std::string& prop, const char* value,
                            bool asString = false);
  const char* GetGlobalProperty(const std::string& prop);
  bool GetGlobalPropertyAsBool(const std::string& prop);

  const char* GetSourceDirectory() const;
  void SetSourceDirectory(std::string const& sourceDirectory);
  const char* GetBinaryDirectory() const;
  void SetBinaryDirectory(std::string const& binaryDirectory);

  void SetWindowsShell(bool windowsShell);
  bool UseWindowsShell() const;
  void SetWindowsVSIDE(bool windowsVSIDE);
  bool UseWindowsVSIDE() const;
  void SetWatcomWMake(bool watcomWMake);
  bool UseWatcomWMake() const;
  void SetMinGWMake(bool minGWMake);
  bool UseMinGWMake() const;
  void SetNMake(bool nMake);
  bool UseNMake() const;
  void SetMSYSShell(bool mSYSShell);
  bool UseMSYSShell() const;

  unsigned int GetCacheMajorVersion() const;
  unsigned int GetCacheMinorVersion() const;

private:
  friend class cmake;
  void AddCacheEntry(const std::string& key, const char* value,
                     const char* helpString,
                     cmStateEnums::CacheEntryType type);

  std::map<cmProperty::ScopeType, cmPropertyDefinitionMap> PropertyDefinitions;
  std::vector<std::string> EnabledLanguages;
  std::map<std::string, cmCommand*> BuiltinCommands;
  std::map<std::string, cmCommand*> ScriptedCommands;
  cmPropertyMap GlobalProperties;
  cmCacheManager* CacheManager;

  cmLinkedTree<cmStateDetail::BuildsystemDirectoryStateType>
    BuildsystemDirectory;

  cmLinkedTree<std::string> ExecutionListFiles;

  cmLinkedTree<cmStateDetail::PolicyStackEntry> PolicyStack;
  cmLinkedTree<cmStateDetail::SnapshotDataType> SnapshotData;
  cmLinkedTree<cmDefinitions> VarTree;

  std::string SourceDirectory;
  std::string BinaryDirectory;
  bool IsInTryCompile;
  bool IsGeneratorMultiConfig;
  bool WindowsShell;
  bool WindowsVSIDE;
  bool WatcomWMake;
  bool MinGWMake;
  bool NMake;
  bool MSYSShell;
};

#endif
