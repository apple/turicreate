/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFindCommon.h"

#include <algorithm>
#include <string.h>
#include <utility>

#include "cmMakefile.h"
#include "cmSystemTools.h"

cmFindCommon::PathGroup cmFindCommon::PathGroup::All("ALL");
cmFindCommon::PathLabel cmFindCommon::PathLabel::PackageRoot(
  "PackageName_ROOT");
cmFindCommon::PathLabel cmFindCommon::PathLabel::CMake("CMAKE");
cmFindCommon::PathLabel cmFindCommon::PathLabel::CMakeEnvironment(
  "CMAKE_ENVIRONMENT");
cmFindCommon::PathLabel cmFindCommon::PathLabel::Hints("HINTS");
cmFindCommon::PathLabel cmFindCommon::PathLabel::SystemEnvironment(
  "SYSTM_ENVIRONMENT");
cmFindCommon::PathLabel cmFindCommon::PathLabel::CMakeSystem("CMAKE_SYSTEM");
cmFindCommon::PathLabel cmFindCommon::PathLabel::Guess("GUESS");

cmFindCommon::cmFindCommon()
{
  this->FindRootPathMode = RootPathModeBoth;
  this->NoDefaultPath = false;
  this->NoPackageRootPath = false;
  this->NoCMakePath = false;
  this->NoCMakeEnvironmentPath = false;
  this->NoSystemEnvironmentPath = false;
  this->NoCMakeSystemPath = false;

// OS X Bundle and Framework search policy.  The default is to
// search frameworks first on apple.
#if defined(__APPLE__)
  this->SearchFrameworkFirst = true;
  this->SearchAppBundleFirst = true;
#else
  this->SearchFrameworkFirst = false;
  this->SearchAppBundleFirst = false;
#endif
  this->SearchFrameworkOnly = false;
  this->SearchFrameworkLast = false;
  this->SearchAppBundleOnly = false;
  this->SearchAppBundleLast = false;

  this->InitializeSearchPathGroups();
}

cmFindCommon::~cmFindCommon()
{
}

void cmFindCommon::InitializeSearchPathGroups()
{
  std::vector<PathLabel>* labels;

  // Define the varoius different groups of path types

  // All search paths
  labels = &this->PathGroupLabelMap[PathGroup::All];
  labels->push_back(PathLabel::PackageRoot);
  labels->push_back(PathLabel::CMake);
  labels->push_back(PathLabel::CMakeEnvironment);
  labels->push_back(PathLabel::Hints);
  labels->push_back(PathLabel::SystemEnvironment);
  labels->push_back(PathLabel::CMakeSystem);
  labels->push_back(PathLabel::Guess);

  // Define the search group order
  this->PathGroupOrder.push_back(PathGroup::All);

  // Create the idividual labeld search paths
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::PackageRoot, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::CMake, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::CMakeEnvironment, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::Hints, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::SystemEnvironment, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::CMakeSystem, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::Guess, cmSearchPath(this)));
}

void cmFindCommon::SelectDefaultNoPackageRootPath()
{
  if (!this->Makefile->IsOn("__UNDOCUMENTED_CMAKE_FIND_PACKAGE_ROOT")) {
    this->NoPackageRootPath = true;
  }
}

void cmFindCommon::SelectDefaultRootPathMode()
{
  // Check the policy variable for this find command type.
  std::string findRootPathVar = "CMAKE_FIND_ROOT_PATH_MODE_";
  findRootPathVar += this->CMakePathName;
  std::string rootPathMode =
    this->Makefile->GetSafeDefinition(findRootPathVar);
  if (rootPathMode == "NEVER") {
    this->FindRootPathMode = RootPathModeNever;
  } else if (rootPathMode == "ONLY") {
    this->FindRootPathMode = RootPathModeOnly;
  } else if (rootPathMode == "BOTH") {
    this->FindRootPathMode = RootPathModeBoth;
  }
}

void cmFindCommon::SelectDefaultMacMode()
{
  std::string ff = this->Makefile->GetSafeDefinition("CMAKE_FIND_FRAMEWORK");
  if (ff == "NEVER") {
    this->SearchFrameworkLast = false;
    this->SearchFrameworkFirst = false;
    this->SearchFrameworkOnly = false;
  } else if (ff == "ONLY") {
    this->SearchFrameworkLast = false;
    this->SearchFrameworkFirst = false;
    this->SearchFrameworkOnly = true;
  } else if (ff == "FIRST") {
    this->SearchFrameworkLast = false;
    this->SearchFrameworkFirst = true;
    this->SearchFrameworkOnly = false;
  } else if (ff == "LAST") {
    this->SearchFrameworkLast = true;
    this->SearchFrameworkFirst = false;
    this->SearchFrameworkOnly = false;
  }

  std::string fab = this->Makefile->GetSafeDefinition("CMAKE_FIND_APPBUNDLE");
  if (fab == "NEVER") {
    this->SearchAppBundleLast = false;
    this->SearchAppBundleFirst = false;
    this->SearchAppBundleOnly = false;
  } else if (fab == "ONLY") {
    this->SearchAppBundleLast = false;
    this->SearchAppBundleFirst = false;
    this->SearchAppBundleOnly = true;
  } else if (fab == "FIRST") {
    this->SearchAppBundleLast = false;
    this->SearchAppBundleFirst = true;
    this->SearchAppBundleOnly = false;
  } else if (fab == "LAST") {
    this->SearchAppBundleLast = true;
    this->SearchAppBundleFirst = false;
    this->SearchAppBundleOnly = false;
  }
}

void cmFindCommon::RerootPaths(std::vector<std::string>& paths)
{
#if 0
  for(std::vector<std::string>::const_iterator i = paths.begin();
      i != paths.end(); ++i)
    {
    fprintf(stderr, "[%s]\n", i->c_str());
    }
#endif
  // Short-circuit if there is nothing to do.
  if (this->FindRootPathMode == RootPathModeNever) {
    return;
  }

  const char* sysroot = this->Makefile->GetDefinition("CMAKE_SYSROOT");
  const char* sysrootCompile =
    this->Makefile->GetDefinition("CMAKE_SYSROOT_COMPILE");
  const char* sysrootLink =
    this->Makefile->GetDefinition("CMAKE_SYSROOT_LINK");
  const char* rootPath = this->Makefile->GetDefinition("CMAKE_FIND_ROOT_PATH");
  const bool noSysroot = !sysroot || !*sysroot;
  const bool noCompileSysroot = !sysrootCompile || !*sysrootCompile;
  const bool noLinkSysroot = !sysrootLink || !*sysrootLink;
  const bool noRootPath = !rootPath || !*rootPath;
  if (noSysroot && noCompileSysroot && noLinkSysroot && noRootPath) {
    return;
  }

  // Construct the list of path roots with no trailing slashes.
  std::vector<std::string> roots;
  if (rootPath) {
    cmSystemTools::ExpandListArgument(rootPath, roots);
  }
  if (sysrootCompile) {
    roots.push_back(sysrootCompile);
  }
  if (sysrootLink) {
    roots.push_back(sysrootLink);
  }
  if (sysroot) {
    roots.push_back(sysroot);
  }
  for (std::vector<std::string>::iterator ri = roots.begin();
       ri != roots.end(); ++ri) {
    cmSystemTools::ConvertToUnixSlashes(*ri);
  }

  const char* stagePrefix =
    this->Makefile->GetDefinition("CMAKE_STAGING_PREFIX");

  // Copy the original set of unrooted paths.
  std::vector<std::string> unrootedPaths = paths;
  paths.clear();

  for (std::vector<std::string>::const_iterator ri = roots.begin();
       ri != roots.end(); ++ri) {
    for (std::vector<std::string>::const_iterator ui = unrootedPaths.begin();
         ui != unrootedPaths.end(); ++ui) {
      // Place the unrooted path under the current root if it is not
      // already inside.  Skip the unrooted path if it is relative to
      // a user home directory or is empty.
      std::string rootedDir;
      if (cmSystemTools::IsSubDirectory(*ui, *ri) ||
          (stagePrefix && cmSystemTools::IsSubDirectory(*ui, stagePrefix))) {
        rootedDir = *ui;
      } else if (!ui->empty() && (*ui)[0] != '~') {
        // Start with the new root.
        rootedDir = *ri;
        rootedDir += "/";

        // Append the original path with its old root removed.
        rootedDir += cmSystemTools::SplitPathRootComponent(*ui);
      }

      // Store the new path.
      paths.push_back(rootedDir);
    }
  }

  // If searching both rooted and unrooted paths add the original
  // paths again.
  if (this->FindRootPathMode == RootPathModeBoth) {
    paths.insert(paths.end(), unrootedPaths.begin(), unrootedPaths.end());
  }
}

void cmFindCommon::GetIgnoredPaths(std::vector<std::string>& ignore)
{
  // null-terminated list of paths.
  static const char* paths[] = { "CMAKE_SYSTEM_IGNORE_PATH",
                                 "CMAKE_IGNORE_PATH", CM_NULLPTR };

  // Construct the list of path roots with no trailing slashes.
  for (const char** pathName = paths; *pathName; ++pathName) {
    // Get the list of paths to ignore from the variable.
    const char* ignorePath = this->Makefile->GetDefinition(*pathName);
    if ((ignorePath == CM_NULLPTR) || (strlen(ignorePath) == 0)) {
      continue;
    }

    cmSystemTools::ExpandListArgument(ignorePath, ignore);
  }

  for (std::vector<std::string>::iterator i = ignore.begin();
       i != ignore.end(); ++i) {
    cmSystemTools::ConvertToUnixSlashes(*i);
  }
}

void cmFindCommon::GetIgnoredPaths(std::set<std::string>& ignore)
{
  std::vector<std::string> ignoreVec;
  GetIgnoredPaths(ignoreVec);
  ignore.insert(ignoreVec.begin(), ignoreVec.end());
}

bool cmFindCommon::CheckCommonArgument(std::string const& arg)
{
  if (arg == "NO_DEFAULT_PATH") {
    this->NoDefaultPath = true;
  } else if (arg == "NO_PACKAGE_ROOT_PATH") {
    this->NoPackageRootPath = true;
  } else if (arg == "NO_CMAKE_PATH") {
    this->NoCMakePath = true;
  } else if (arg == "NO_CMAKE_ENVIRONMENT_PATH") {
    this->NoCMakeEnvironmentPath = true;
  } else if (arg == "NO_SYSTEM_ENVIRONMENT_PATH") {
    this->NoSystemEnvironmentPath = true;
  } else if (arg == "NO_CMAKE_SYSTEM_PATH") {
    this->NoCMakeSystemPath = true;
  } else if (arg == "NO_CMAKE_FIND_ROOT_PATH") {
    this->FindRootPathMode = RootPathModeNever;
  } else if (arg == "ONLY_CMAKE_FIND_ROOT_PATH") {
    this->FindRootPathMode = RootPathModeOnly;
  } else if (arg == "CMAKE_FIND_ROOT_PATH_BOTH") {
    this->FindRootPathMode = RootPathModeBoth;
  } else {
    // The argument is not one of the above.
    return false;
  }

  // The argument is one of the above.
  return true;
}

void cmFindCommon::AddPathSuffix(std::string const& arg)
{
  std::string suffix = arg;

  // Strip leading and trailing slashes.
  if (suffix.empty()) {
    return;
  }
  if (suffix[0] == '/') {
    suffix = suffix.substr(1);
  }
  if (suffix.empty()) {
    return;
  }
  if (suffix[suffix.size() - 1] == '/') {
    suffix = suffix.substr(0, suffix.size() - 1);
  }
  if (suffix.empty()) {
    return;
  }

  // Store the suffix.
  this->SearchPathSuffixes.push_back(suffix);
}

void AddTrailingSlash(std::string& s)
{
  if (!s.empty() && *s.rbegin() != '/') {
    s += '/';
  }
}
void cmFindCommon::ComputeFinalPaths()
{
  // Filter out ignored paths from the prefix list
  std::set<std::string> ignored;
  this->GetIgnoredPaths(ignored);

  // Combine the seperate path types, filtering out ignores
  this->SearchPaths.clear();
  std::vector<PathLabel>& allLabels = this->PathGroupLabelMap[PathGroup::All];
  for (std::vector<PathLabel>::const_iterator l = allLabels.begin();
       l != allLabels.end(); ++l) {
    this->LabeledPaths[*l].ExtractWithout(ignored, this->SearchPaths);
  }

  // Expand list of paths inside all search roots.
  this->RerootPaths(this->SearchPaths);

  // Add a trailing slash to all paths to aid the search process.
  std::for_each(this->SearchPaths.begin(), this->SearchPaths.end(),
                &AddTrailingSlash);
}
