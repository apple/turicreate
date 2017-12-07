/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFindPackageCommand_h
#define cmFindPackageCommand_h

#include "cmConfigure.h"

#include "cm_kwiml.h"
#include <map>
#include <set>
#include <string>
#include <vector>

#include "cmFindCommon.h"

class cmCommand;
class cmExecutionStatus;
class cmSearchPath;

/** \class cmFindPackageCommand
 * \brief Load settings from an external project.
 *
 * cmFindPackageCommand
 */
class cmFindPackageCommand : public cmFindCommon
{
public:
  /*! A sorting order strategy to be applied to recovered package folders (see
   * FIND_PACKAGE_SORT_ORDER)*/
  enum /*class*/ SortOrderType
  {
    None,
    Name_order,
    Natural
  };
  /*! A sorting direction to be applied to recovered package folders (see
   * FIND_PACKAGE_SORT_DIRECTION)*/
  enum /*class*/ SortDirectionType
  {
    Asc,
    Dec
  };

  /*! sorts a given list of string based on the input sort parameters */
  static void Sort(std::vector<std::string>::iterator begin,
                   std::vector<std::string>::iterator end, SortOrderType order,
                   SortDirectionType dir);

  cmFindPackageCommand();

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmFindPackageCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  class PathLabel : public cmFindCommon::PathLabel
  {
  protected:
    PathLabel();

  public:
    PathLabel(const std::string& label)
      : cmFindCommon::PathLabel(label)
    {
    }
    static PathLabel UserRegistry;
    static PathLabel Builds;
    static PathLabel SystemRegistry;
  };

  // Add additional search path labels and groups not present in the
  // parent class
  void AppendSearchPathGroups();

  void AppendSuccessInformation();
  void AppendToFoundProperty(bool found);
  void SetModuleVariables(const std::string& components);
  bool FindModule(bool& found);
  void AddFindDefinition(const std::string& var, const char* val);
  void RestoreFindDefinitions();
  bool HandlePackageMode();
  bool FindConfig();
  bool FindPrefixedConfig();
  bool FindFrameworkConfig();
  bool FindAppBundleConfig();
  enum PolicyScopeRule
  {
    NoPolicyScope,
    DoPolicyScope
  };
  bool ReadListFile(const char* f, PolicyScopeRule psr);
  void StoreVersionFound();

  void ComputePrefixes();
  void FillPrefixesPackageRoot();
  void FillPrefixesCMakeEnvironment();
  void FillPrefixesCMakeVariable();
  void FillPrefixesSystemEnvironment();
  void FillPrefixesUserRegistry();
  void FillPrefixesSystemRegistry();
  void FillPrefixesCMakeSystemVariable();
  void FillPrefixesUserGuess();
  void FillPrefixesUserHints();
  void LoadPackageRegistryDir(std::string const& dir, cmSearchPath& outPaths);
  void LoadPackageRegistryWinUser();
  void LoadPackageRegistryWinSystem();
  void LoadPackageRegistryWin(bool user, unsigned int view,
                              cmSearchPath& outPaths);
  bool CheckPackageRegistryEntry(const std::string& fname,
                                 cmSearchPath& outPaths);
  bool SearchDirectory(std::string const& dir);
  bool CheckDirectory(std::string const& dir);
  bool FindConfigFile(std::string const& dir, std::string& file);
  bool CheckVersion(std::string const& config_file);
  bool CheckVersionFile(std::string const& version_file,
                        std::string& result_version);
  bool SearchPrefix(std::string const& prefix);
  bool SearchFrameworkPrefix(std::string const& prefix_in);
  bool SearchAppBundlePrefix(std::string const& prefix_in);

  friend class cmFindPackageFileList;

  struct OriginalDef
  {
    bool exists;
    std::string value;
  };
  std::map<std::string, OriginalDef> OriginalDefs;

  std::string Name;
  std::string Variable;
  std::string Version;
  unsigned int VersionMajor;
  unsigned int VersionMinor;
  unsigned int VersionPatch;
  unsigned int VersionTweak;
  unsigned int VersionCount;
  bool VersionExact;
  std::string FileFound;
  std::string VersionFound;
  unsigned int VersionFoundMajor;
  unsigned int VersionFoundMinor;
  unsigned int VersionFoundPatch;
  unsigned int VersionFoundTweak;
  unsigned int VersionFoundCount;
  KWIML_INT_uint64_t RequiredCMakeVersion;
  bool Quiet;
  bool Required;
  bool UseConfigFiles;
  bool UseFindModules;
  bool NoUserRegistry;
  bool NoSystemRegistry;
  bool DebugMode;
  bool UseLib32Paths;
  bool UseLib64Paths;
  bool UseLibx32Paths;
  bool PolicyScope;
  std::string LibraryArchitecture;
  std::vector<std::string> Names;
  std::vector<std::string> Configs;
  std::set<std::string> IgnoredPaths;

  /*! the selected sortOrder (None by default)*/
  SortOrderType SortOrder;
  /*! the selected sortDirection (Asc by default)*/
  SortDirectionType SortDirection;

  struct ConfigFileInfo
  {
    std::string filename;
    std::string version;

    bool operator<(ConfigFileInfo const& rhs) const
    {
      return this->filename < rhs.filename;
    }

    bool operator==(ConfigFileInfo const& rhs) const
    {
      return this->filename == rhs.filename;
    }

    bool operator!=(ConfigFileInfo const& rhs) const
    {
      return !(*this == rhs);
    }
  };
  std::vector<ConfigFileInfo> ConsideredConfigs;
};

#endif
