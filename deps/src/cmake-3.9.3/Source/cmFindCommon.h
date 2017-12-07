/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFindCommon_h
#define cmFindCommon_h

#include "cmConfigure.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "cmCommand.h"
#include "cmPathLabel.h"
#include "cmSearchPath.h"

/** \class cmFindCommon
 * \brief Base class for FIND_XXX implementations.
 *
 * cmFindCommon is a parent class for cmFindBase,
 * cmFindProgramCommand, cmFindPathCommand, cmFindLibraryCommand,
 * cmFindFileCommand, and cmFindPackageCommand.
 */
class cmFindCommon : public cmCommand
{
public:
  cmFindCommon();
  ~cmFindCommon() CM_OVERRIDE;

protected:
  friend class cmSearchPath;

  /** Used to define groups of path labels */
  class PathGroup : public cmPathLabel
  {
  protected:
    PathGroup();

  public:
    PathGroup(const std::string& label)
      : cmPathLabel(label)
    {
    }
    static PathGroup All;
  };

  /* Individual path types */
  class PathLabel : public cmPathLabel
  {
  protected:
    PathLabel();

  public:
    PathLabel(const std::string& label)
      : cmPathLabel(label)
    {
    }
    static PathLabel PackageRoot;
    static PathLabel CMake;
    static PathLabel CMakeEnvironment;
    static PathLabel Hints;
    static PathLabel SystemEnvironment;
    static PathLabel CMakeSystem;
    static PathLabel Guess;
  };

  enum RootPathMode
  {
    RootPathModeNever,
    RootPathModeOnly,
    RootPathModeBoth
  };

  /** Construct the various path groups and labels */
  void InitializeSearchPathGroups();

  /** Place a set of search paths under the search roots.  */
  void RerootPaths(std::vector<std::string>& paths);

  /** Get ignored paths from CMAKE_[SYSTEM_]IGNORE_path variables.  */
  void GetIgnoredPaths(std::vector<std::string>& ignore);
  void GetIgnoredPaths(std::set<std::string>& ignore);

  /** Compute final search path list (reroot + trailing slash).  */
  void ComputeFinalPaths();

  /** Decide whether to enable the PACKAGE_ROOT search entries.  */
  void SelectDefaultNoPackageRootPath();

  /** Compute the current default root path mode.  */
  void SelectDefaultRootPathMode();

  /** Compute the current default bundle/framework search policy.  */
  void SelectDefaultMacMode();

  // Path arguments prior to path manipulation routines
  std::vector<std::string> UserHintsArgs;
  std::vector<std::string> UserGuessArgs;

  std::string CMakePathName;
  RootPathMode FindRootPathMode;

  bool CheckCommonArgument(std::string const& arg);
  void AddPathSuffix(std::string const& arg);

  bool NoDefaultPath;
  bool NoPackageRootPath;
  bool NoCMakePath;
  bool NoCMakeEnvironmentPath;
  bool NoSystemEnvironmentPath;
  bool NoCMakeSystemPath;

  std::vector<std::string> SearchPathSuffixes;

  std::map<PathGroup, std::vector<PathLabel> > PathGroupLabelMap;
  std::vector<PathGroup> PathGroupOrder;
  std::map<std::string, PathLabel> PathLabelStringMap;
  std::map<PathLabel, cmSearchPath> LabeledPaths;

  std::vector<std::string> SearchPaths;
  std::set<std::string> SearchPathsEmitted;

  bool SearchFrameworkFirst;
  bool SearchFrameworkOnly;
  bool SearchFrameworkLast;

  bool SearchAppBundleFirst;
  bool SearchAppBundleOnly;
  bool SearchAppBundleLast;
};

#endif
