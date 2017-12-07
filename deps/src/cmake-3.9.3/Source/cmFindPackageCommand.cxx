/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFindPackageCommand.h"

#include "cmSystemTools.h"
#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include "cmsys/RegularExpression.hxx"
#include "cmsys/String.h"
#include <algorithm>
#include <assert.h>
#include <deque>
#include <functional>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <utility>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmSearchPath.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmVersion.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

#if defined(__HAIKU__)
#include <FindDirectory.h>
#include <StorageDefs.h>
#endif

class cmExecutionStatus;
class cmFileList;

cmFindPackageCommand::PathLabel cmFindPackageCommand::PathLabel::UserRegistry(
  "PACKAGE_REGISTRY");
cmFindPackageCommand::PathLabel cmFindPackageCommand::PathLabel::Builds(
  "BUILDS");
cmFindPackageCommand::PathLabel
  cmFindPackageCommand::PathLabel::SystemRegistry("SYSTEM_PACKAGE_REGISTRY");

struct StrverscmpGreater
{
  bool operator()(const std::string& lhs, const std::string& rhs) const
  {
    return cmSystemTools::strverscmp(lhs, rhs) > 0;
  }
};

struct StrverscmpLesser
{
  bool operator()(const std::string& lhs, const std::string& rhs) const
  {
    return cmSystemTools::strverscmp(lhs, rhs) < 0;
  }
};

void cmFindPackageCommand::Sort(std::vector<std::string>::iterator begin,
                                std::vector<std::string>::iterator end,
                                SortOrderType order, SortDirectionType dir)
{
  if (order == Name_order) {
    if (dir == Dec) {
      std::sort(begin, end, std::greater<std::string>());
    } else {
      std::sort(begin, end);
    }
  } else if (order == Natural)
  // natural order uses letters and numbers (contiguous numbers digit are
  // compared such that e.g. 000  00 < 01 < 010 < 09 < 0 < 1 < 9 < 10
  {
    if (dir == Dec) {
      std::sort(begin, end, StrverscmpGreater());
    } else {
      std::sort(begin, end, StrverscmpLesser());
    }
  }
  // else do not sort
}

cmFindPackageCommand::cmFindPackageCommand()
{
  this->CMakePathName = "PACKAGE";
  this->Quiet = false;
  this->Required = false;
  this->NoUserRegistry = false;
  this->NoSystemRegistry = false;
  this->UseConfigFiles = true;
  this->UseFindModules = true;
  this->DebugMode = false;
  this->UseLib32Paths = false;
  this->UseLib64Paths = false;
  this->UseLibx32Paths = false;
  this->PolicyScope = true;
  this->VersionMajor = 0;
  this->VersionMinor = 0;
  this->VersionPatch = 0;
  this->VersionTweak = 0;
  this->VersionCount = 0;
  this->VersionExact = false;
  this->VersionFoundMajor = 0;
  this->VersionFoundMinor = 0;
  this->VersionFoundPatch = 0;
  this->VersionFoundTweak = 0;
  this->VersionFoundCount = 0;
  this->RequiredCMakeVersion = 0;
  this->SortOrder = None;
  this->SortDirection = Asc;
  this->AppendSearchPathGroups();
}

void cmFindPackageCommand::AppendSearchPathGroups()
{
  std::vector<cmFindCommon::PathLabel>* labels;

  // Update the All group with new paths
  labels = &this->PathGroupLabelMap[PathGroup::All];
  labels->insert(
    std::find(labels->begin(), labels->end(), PathLabel::CMakeSystem),
    PathLabel::UserRegistry);
  labels->insert(
    std::find(labels->begin(), labels->end(), PathLabel::CMakeSystem),
    PathLabel::Builds);
  labels->insert(std::find(labels->begin(), labels->end(), PathLabel::Guess),
                 PathLabel::SystemRegistry);

  // Create the new path objects
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::UserRegistry, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::Builds, cmSearchPath(this)));
  this->LabeledPaths.insert(
    std::make_pair(PathLabel::SystemRegistry, cmSearchPath(this)));
}

bool cmFindPackageCommand::InitialPass(std::vector<std::string> const& args,
                                       cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // Lookup required version of CMake.
  if (const char* rv =
        this->Makefile->GetDefinition("CMAKE_MINIMUM_REQUIRED_VERSION")) {
    unsigned int v[3] = { 0, 0, 0 };
    sscanf(rv, "%u.%u.%u", &v[0], &v[1], &v[2]);
    this->RequiredCMakeVersion = CMake_VERSION_ENCODE(v[0], v[1], v[2]);
  }

  // Check for debug mode.
  this->DebugMode = this->Makefile->IsOn("CMAKE_FIND_DEBUG_MODE");

  // Lookup target architecture, if any.
  if (const char* arch =
        this->Makefile->GetDefinition("CMAKE_LIBRARY_ARCHITECTURE")) {
    this->LibraryArchitecture = arch;
  }

  // Lookup whether lib32 paths should be used.
  if (this->Makefile->PlatformIs32Bit() &&
      this->Makefile->GetState()->GetGlobalPropertyAsBool(
        "FIND_LIBRARY_USE_LIB32_PATHS")) {
    this->UseLib32Paths = true;
  }

  // Lookup whether lib64 paths should be used.
  if (this->Makefile->PlatformIs64Bit() &&
      this->Makefile->GetState()->GetGlobalPropertyAsBool(
        "FIND_LIBRARY_USE_LIB64_PATHS")) {
    this->UseLib64Paths = true;
  }

  // Lookup whether libx32 paths should be used.
  if (this->Makefile->PlatformIsx32() &&
      this->Makefile->GetState()->GetGlobalPropertyAsBool(
        "FIND_LIBRARY_USE_LIBX32_PATHS")) {
    this->UseLibx32Paths = true;
  }

  // Check if User Package Registry should be disabled
  if (this->Makefile->IsOn("CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY")) {
    this->NoUserRegistry = true;
  }

  // Check if System Package Registry should be disabled
  if (this->Makefile->IsOn("CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY")) {
    this->NoSystemRegistry = true;
  }

  // Check if Sorting should be enabled
  if (const char* so =
        this->Makefile->GetDefinition("CMAKE_FIND_PACKAGE_SORT_ORDER")) {

    if (strcmp(so, "NAME") == 0) {
      this->SortOrder = Name_order;
    } else if (strcmp(so, "NATURAL") == 0) {
      this->SortOrder = Natural;
    } else {
      this->SortOrder = None;
    }
  }
  if (const char* sd =
        this->Makefile->GetDefinition("CMAKE_FIND_PACKAGE_SORT_DIRECTION")) {
    this->SortDirection = strcmp(sd, "ASC") == 0 ? Asc : Dec;
  }

  this->SelectDefaultNoPackageRootPath();

  // Find the current root path mode.
  this->SelectDefaultRootPathMode();

  // Find the current bundle/framework search policy.
  this->SelectDefaultMacMode();

  // Record options.
  this->Name = args[0];
  std::string components;
  const char* components_sep = "";
  std::set<std::string> requiredComponents;
  std::set<std::string> optionalComponents;

  // Always search directly in a generated path.
  this->SearchPathSuffixes.push_back("");

  // Parse the arguments.
  enum Doing
  {
    DoingNone,
    DoingComponents,
    DoingOptionalComponents,
    DoingNames,
    DoingPaths,
    DoingPathSuffixes,
    DoingConfigs,
    DoingHints
  };
  Doing doing = DoingNone;
  cmsys::RegularExpression version("^[0-9.]+$");
  bool haveVersion = false;
  std::set<unsigned int> configArgs;
  std::set<unsigned int> moduleArgs;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "QUIET") {
      this->Quiet = true;
      doing = DoingNone;
    } else if (args[i] == "EXACT") {
      this->VersionExact = true;
      doing = DoingNone;
    } else if (args[i] == "MODULE") {
      moduleArgs.insert(i);
      doing = DoingNone;
    } else if (args[i] == "CONFIG") {
      configArgs.insert(i);
      doing = DoingNone;
    } else if (args[i] == "NO_MODULE") {
      configArgs.insert(i);
      doing = DoingNone;
    } else if (args[i] == "REQUIRED") {
      this->Required = true;
      doing = DoingComponents;
    } else if (args[i] == "COMPONENTS") {
      doing = DoingComponents;
    } else if (args[i] == "OPTIONAL_COMPONENTS") {
      doing = DoingOptionalComponents;
    } else if (args[i] == "NAMES") {
      configArgs.insert(i);
      doing = DoingNames;
    } else if (args[i] == "PATHS") {
      configArgs.insert(i);
      doing = DoingPaths;
    } else if (args[i] == "HINTS") {
      configArgs.insert(i);
      doing = DoingHints;
    } else if (args[i] == "PATH_SUFFIXES") {
      configArgs.insert(i);
      doing = DoingPathSuffixes;
    } else if (args[i] == "CONFIGS") {
      configArgs.insert(i);
      doing = DoingConfigs;
    } else if (args[i] == "NO_POLICY_SCOPE") {
      this->PolicyScope = false;
      doing = DoingNone;
    } else if (args[i] == "NO_CMAKE_PACKAGE_REGISTRY") {
      this->NoUserRegistry = true;
      configArgs.insert(i);
      doing = DoingNone;
    } else if (args[i] == "NO_CMAKE_SYSTEM_PACKAGE_REGISTRY") {
      this->NoSystemRegistry = true;
      configArgs.insert(i);
      doing = DoingNone;
    } else if (args[i] == "NO_CMAKE_BUILDS_PATH") {
      // Ignore legacy option.
      configArgs.insert(i);
      doing = DoingNone;
    } else if (this->CheckCommonArgument(args[i])) {
      configArgs.insert(i);
      doing = DoingNone;
    } else if ((doing == DoingComponents) ||
               (doing == DoingOptionalComponents)) {
      // Set a variable telling the find script whether this component
      // is required.
      const char* isRequired = "1";
      if (doing == DoingOptionalComponents) {
        isRequired = "0";
        optionalComponents.insert(args[i]);
      } else {
        requiredComponents.insert(args[i]);
      }

      std::string req_var = this->Name + "_FIND_REQUIRED_" + args[i];
      this->AddFindDefinition(req_var, isRequired);

      // Append to the list of required components.
      components += components_sep;
      components += args[i];
      components_sep = ";";
    } else if (doing == DoingNames) {
      this->Names.push_back(args[i]);
    } else if (doing == DoingPaths) {
      this->UserGuessArgs.push_back(args[i]);
    } else if (doing == DoingHints) {
      this->UserHintsArgs.push_back(args[i]);
    } else if (doing == DoingPathSuffixes) {
      this->AddPathSuffix(args[i]);
    } else if (doing == DoingConfigs) {
      if (args[i].find_first_of(":/\\") != std::string::npos ||
          cmSystemTools::GetFilenameLastExtension(args[i]) != ".cmake") {
        std::ostringstream e;
        e << "given CONFIGS option followed by invalid file name \"" << args[i]
          << "\".  The names given must be file names without "
          << "a path and with a \".cmake\" extension.";
        this->SetError(e.str());
        return false;
      }
      this->Configs.push_back(args[i]);
    } else if (!haveVersion && version.find(args[i].c_str())) {
      haveVersion = true;
      this->Version = args[i];
    } else {
      std::ostringstream e;
      e << "called with invalid argument \"" << args[i] << "\"";
      this->SetError(e.str());
      return false;
    }
  }

  std::vector<std::string> doubledComponents;
  std::set_intersection(requiredComponents.begin(), requiredComponents.end(),
                        optionalComponents.begin(), optionalComponents.end(),
                        std::back_inserter(doubledComponents));
  if (!doubledComponents.empty()) {
    std::ostringstream e;
    e << "called with components that are both required and optional:\n";
    e << cmWrap("  ", doubledComponents, "", "\n") << "\n";
    this->SetError(e.str());
    return false;
  }

  // Maybe choose one mode exclusively.
  this->UseFindModules = configArgs.empty();
  this->UseConfigFiles = moduleArgs.empty();
  if (!this->UseFindModules && !this->UseConfigFiles) {
    std::ostringstream e;
    e << "given options exclusive to Module mode:\n";
    for (std::set<unsigned int>::const_iterator si = moduleArgs.begin();
         si != moduleArgs.end(); ++si) {
      e << "  " << args[*si] << "\n";
    }
    e << "and options exclusive to Config mode:\n";
    for (std::set<unsigned int>::const_iterator si = configArgs.begin();
         si != configArgs.end(); ++si) {
      e << "  " << args[*si] << "\n";
    }
    e << "The options are incompatible.";
    this->SetError(e.str());
    return false;
  }

  // Ignore EXACT with no version.
  if (this->Version.empty() && this->VersionExact) {
    this->VersionExact = false;
    this->Makefile->IssueMessage(
      cmake::AUTHOR_WARNING, "Ignoring EXACT since no version is requested.");
  }

  if (this->Version.empty() || components.empty()) {
    // Check whether we are recursing inside "Find<name>.cmake" within
    // another find_package(<name>) call.
    std::string mod = this->Name;
    mod += "_FIND_MODULE";
    if (this->Makefile->IsOn(mod)) {
      if (this->Version.empty()) {
        // Get version information from the outer call if necessary.
        // Requested version string.
        std::string ver = this->Name;
        ver += "_FIND_VERSION";
        this->Version = this->Makefile->GetSafeDefinition(ver);

        // Whether an exact version is required.
        std::string exact = this->Name;
        exact += "_FIND_VERSION_EXACT";
        this->VersionExact = this->Makefile->IsOn(exact);
      }
      if (components.empty()) {
        std::string components_var = this->Name + "_FIND_COMPONENTS";
        components = this->Makefile->GetSafeDefinition(components_var);
      }
    }
  }

  if (!this->Version.empty()) {
    // Try to parse the version number and store the results that were
    // successfully parsed.
    unsigned int parsed_major;
    unsigned int parsed_minor;
    unsigned int parsed_patch;
    unsigned int parsed_tweak;
    this->VersionCount =
      sscanf(this->Version.c_str(), "%u.%u.%u.%u", &parsed_major,
             &parsed_minor, &parsed_patch, &parsed_tweak);
    switch (this->VersionCount) {
      case 4:
        this->VersionTweak = parsed_tweak;
        CM_FALLTHROUGH;
      case 3:
        this->VersionPatch = parsed_patch;
        CM_FALLTHROUGH;
      case 2:
        this->VersionMinor = parsed_minor;
        CM_FALLTHROUGH;
      case 1:
        this->VersionMajor = parsed_major;
        CM_FALLTHROUGH;
      default:
        break;
    }
  }

  std::string disableFindPackageVar = "CMAKE_DISABLE_FIND_PACKAGE_";
  disableFindPackageVar += this->Name;
  if (this->Makefile->IsOn(disableFindPackageVar)) {
    if (this->Required) {
      std::ostringstream e;
      e << "for module " << this->Name << " called with REQUIRED, but "
        << disableFindPackageVar
        << " is enabled. A REQUIRED package cannot be disabled.";
      this->SetError(e.str());
      return false;
    }

    return true;
  }

  this->SetModuleVariables(components);

  // See if there is a Find<package>.cmake module.
  if (this->UseFindModules) {
    bool foundModule = false;
    if (!this->FindModule(foundModule)) {
      this->AppendSuccessInformation();
      return false;
    }
    if (foundModule) {
      this->AppendSuccessInformation();
      return true;
    }
  }

  if (this->UseFindModules && this->UseConfigFiles &&
      this->Makefile->IsOn("CMAKE_FIND_PACKAGE_WARN_NO_MODULE")) {
    std::ostringstream aw;
    if (this->RequiredCMakeVersion >= CMake_VERSION_ENCODE(2, 8, 8)) {
      aw << "find_package called without either MODULE or CONFIG option and "
            "no Find"
         << this->Name
         << ".cmake module is in CMAKE_MODULE_PATH.  "
            "Add MODULE to exclusively request Module mode and fail if "
            "Find"
         << this->Name
         << ".cmake is missing.  "
            "Add CONFIG to exclusively request Config mode and search for a "
            "package configuration file provided by "
         << this->Name << " (" << this->Name << "Config.cmake or "
         << cmSystemTools::LowerCase(this->Name) << "-config.cmake).  ";
    } else {
      aw
        << "find_package called without NO_MODULE option and no "
           "Find"
        << this->Name
        << ".cmake module is in CMAKE_MODULE_PATH.  "
           "Add NO_MODULE to exclusively request Config mode and search for a "
           "package configuration file provided by "
        << this->Name << " (" << this->Name << "Config.cmake or "
        << cmSystemTools::LowerCase(this->Name) << "-config.cmake).  "
                                                   "Otherwise make Find"
        << this->Name << ".cmake available in "
                         "CMAKE_MODULE_PATH.";
    }
    aw << "\n"
          "(Variable CMAKE_FIND_PACKAGE_WARN_NO_MODULE enabled this warning.)";
    this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, aw.str());
  }

  // No find module.  Assume the project has a CMake config file.  Use
  // a <package>_DIR cache variable to locate it.
  this->Variable = this->Name;
  this->Variable += "_DIR";

  // Add the default name.
  if (this->Names.empty()) {
    this->Names.push_back(this->Name);
  }

  // Add the default configs.
  if (this->Configs.empty()) {
    for (std::vector<std::string>::const_iterator ni = this->Names.begin();
         ni != this->Names.end(); ++ni) {
      std::string config = *ni;
      config += "Config.cmake";
      this->Configs.push_back(config);

      config = cmSystemTools::LowerCase(*ni);
      config += "-config.cmake";
      this->Configs.push_back(config);
    }
  }

  // get igonored paths from vars and reroot them.
  std::vector<std::string> ignored;
  this->GetIgnoredPaths(ignored);
  this->RerootPaths(ignored);

  // Construct a set of ignored paths
  this->IgnoredPaths.clear();
  this->IgnoredPaths.insert(ignored.begin(), ignored.end());

  // Find and load the package.
  bool result = this->HandlePackageMode();
  this->AppendSuccessInformation();
  return result;
}

void cmFindPackageCommand::SetModuleVariables(const std::string& components)
{
  this->AddFindDefinition("CMAKE_FIND_PACKAGE_NAME", this->Name.c_str());

  // Store the list of components.
  std::string components_var = this->Name + "_FIND_COMPONENTS";
  this->AddFindDefinition(components_var, components.c_str());

  if (this->Quiet) {
    // Tell the module that is about to be read that it should find
    // quietly.
    std::string quietly = this->Name;
    quietly += "_FIND_QUIETLY";
    this->AddFindDefinition(quietly, "1");
  }

  if (this->Required) {
    // Tell the module that is about to be read that it should report
    // a fatal error if the package is not found.
    std::string req = this->Name;
    req += "_FIND_REQUIRED";
    this->AddFindDefinition(req, "1");
  }

  if (!this->Version.empty()) {
    // Tell the module that is about to be read what version of the
    // package has been requested.
    std::string ver = this->Name;
    ver += "_FIND_VERSION";
    this->AddFindDefinition(ver, this->Version.c_str());
    char buf[64];
    sprintf(buf, "%u", this->VersionMajor);
    this->AddFindDefinition(ver + "_MAJOR", buf);
    sprintf(buf, "%u", this->VersionMinor);
    this->AddFindDefinition(ver + "_MINOR", buf);
    sprintf(buf, "%u", this->VersionPatch);
    this->AddFindDefinition(ver + "_PATCH", buf);
    sprintf(buf, "%u", this->VersionTweak);
    this->AddFindDefinition(ver + "_TWEAK", buf);
    sprintf(buf, "%u", this->VersionCount);
    this->AddFindDefinition(ver + "_COUNT", buf);

    // Tell the module whether an exact version has been requested.
    std::string exact = this->Name;
    exact += "_FIND_VERSION_EXACT";
    this->AddFindDefinition(exact, this->VersionExact ? "1" : "0");
  }

  // Push on to the pacakge stack
  this->Makefile->FindPackageModuleStack.push_back(this->Name);
}

void cmFindPackageCommand::AddFindDefinition(const std::string& var,
                                             const char* val)
{
  if (const char* old = this->Makefile->GetDefinition(var)) {
    this->OriginalDefs[var].exists = true;
    this->OriginalDefs[var].value = old;
  } else {
    this->OriginalDefs[var].exists = false;
  }
  this->Makefile->AddDefinition(var, val);
}

void cmFindPackageCommand::RestoreFindDefinitions()
{
  for (std::map<std::string, OriginalDef>::iterator i =
         this->OriginalDefs.begin();
       i != this->OriginalDefs.end(); ++i) {
    OriginalDef const& od = i->second;
    if (od.exists) {
      this->Makefile->AddDefinition(i->first, od.value.c_str());
    } else {
      this->Makefile->RemoveDefinition(i->first);
    }
  }
}

bool cmFindPackageCommand::FindModule(bool& found)
{
  std::string module = "Find";
  module += this->Name;
  module += ".cmake";
  std::string mfile = this->Makefile->GetModulesFile(module.c_str());
  if (!mfile.empty()) {
    // Load the module we found, and set "<name>_FIND_MODULE" to true
    // while inside it.
    found = true;
    std::string var = this->Name;
    var += "_FIND_MODULE";
    this->Makefile->AddDefinition(var, "1");
    bool result = this->ReadListFile(mfile.c_str(), DoPolicyScope);
    this->Makefile->RemoveDefinition(var);
    return result;
  }
  return true;
}

bool cmFindPackageCommand::HandlePackageMode()
{
  this->ConsideredConfigs.clear();

  // Support old capitalization behavior.
  std::string upperDir = cmSystemTools::UpperCase(this->Name);
  std::string upperFound = cmSystemTools::UpperCase(this->Name);
  upperDir += "_DIR";
  upperFound += "_FOUND";

  // Try to find the config file.
  const char* def = this->Makefile->GetDefinition(this->Variable);

  // Try to load the config file if the directory is known
  bool fileFound = false;
  if (this->UseConfigFiles) {
    if (!cmSystemTools::IsOff(def)) {
      // Get the directory from the variable value.
      std::string dir = def;
      cmSystemTools::ConvertToUnixSlashes(dir);

      // Treat relative paths with respect to the current source dir.
      if (!cmSystemTools::FileIsFullPath(dir.c_str())) {
        dir = "/" + dir;
        dir = this->Makefile->GetCurrentSourceDirectory() + dir;
      }
      // The file location was cached.  Look for the correct file.
      std::string file;
      if (this->FindConfigFile(dir, file)) {
        this->FileFound = file;
        fileFound = true;
      }
      def = this->Makefile->GetDefinition(this->Variable);
    }

    // Search for the config file if it is not already found.
    if (cmSystemTools::IsOff(def) || !fileFound) {
      fileFound = this->FindConfig();
    }

    // Sanity check.
    if (fileFound && this->FileFound.empty()) {
      this->Makefile->IssueMessage(
        cmake::INTERNAL_ERROR, "fileFound is true but FileFound is empty!");
      fileFound = false;
    }
  }

  std::string foundVar = this->Name;
  foundVar += "_FOUND";
  std::string notFoundMessageVar = this->Name;
  notFoundMessageVar += "_NOT_FOUND_MESSAGE";
  std::string notFoundMessage;

  // If the directory for the config file was found, try to read the file.
  bool result = true;
  bool found = false;
  bool configFileSetFOUNDFalse = false;

  if (fileFound) {
    if (this->Makefile->IsDefinitionSet(foundVar) &&
        !this->Makefile->IsOn(foundVar)) {
      // by removing Foo_FOUND here if it is FALSE, we don't really change
      // the situation for the Config file which is about to be included,
      // but we make it possible to detect later on whether the Config file
      // has set Foo_FOUND to FALSE itself:
      this->Makefile->RemoveDefinition(foundVar);
    }
    this->Makefile->RemoveDefinition(notFoundMessageVar);

    // Set the version variables before loading the config file.
    // It may override them.
    this->StoreVersionFound();

    // Parse the configuration file.
    if (this->ReadListFile(this->FileFound.c_str(), DoPolicyScope)) {
      // The package has been found.
      found = true;

      // Check whether the Config file has set Foo_FOUND to FALSE:
      if (this->Makefile->IsDefinitionSet(foundVar) &&
          !this->Makefile->IsOn(foundVar)) {
        // we get here if the Config file has set Foo_FOUND actively to FALSE
        found = false;
        configFileSetFOUNDFalse = true;
        notFoundMessage =
          this->Makefile->GetSafeDefinition(notFoundMessageVar);
      }
    } else {
      // The configuration file is invalid.
      result = false;
    }
  }

  // package not found
  if (result && !found) {
    // warn if package required or neither quiet nor in config mode
    if (this->Required ||
        !(this->Quiet || (this->UseConfigFiles && !this->UseFindModules &&
                          this->ConsideredConfigs.empty()))) {
      // The variable is not set.
      std::ostringstream e;
      std::ostringstream aw;
      if (configFileSetFOUNDFalse) {
        /* clang-format off */
        e << "Found package configuration file:\n"
          "  " << this->FileFound << "\n"
          "but it set " << foundVar << " to FALSE so package \"" <<
          this->Name << "\" is considered to be NOT FOUND.";
        /* clang-format on */
        if (!notFoundMessage.empty()) {
          e << " Reason given by package: \n" << notFoundMessage << "\n";
        }
      }
      // If there are files in ConsideredConfigs, it means that FooConfig.cmake
      // have been found, but they didn't have appropriate versions.
      else if (!this->ConsideredConfigs.empty()) {
        std::vector<ConfigFileInfo>::const_iterator duplicate_end =
          cmRemoveDuplicates(this->ConsideredConfigs);
        e << "Could not find a configuration file for package \"" << this->Name
          << "\" that "
          << (this->VersionExact ? "exactly matches" : "is compatible with")
          << " requested version \"" << this->Version << "\".\n"
          << "The following configuration files were considered but not "
             "accepted:\n";
        for (std::vector<ConfigFileInfo>::const_iterator i =
               this->ConsideredConfigs.begin();
             i != duplicate_end; ++i) {
          e << "  " << i->filename << ", version: " << i->version << "\n";
        }
      } else {
        std::string requestedVersionString;
        if (!this->Version.empty()) {
          requestedVersionString = " (requested version ";
          requestedVersionString += this->Version;
          requestedVersionString += ")";
        }

        if (this->UseConfigFiles) {
          if (this->UseFindModules) {
            e << "By not providing \"Find" << this->Name
              << ".cmake\" in "
                 "CMAKE_MODULE_PATH this project has asked CMake to find a "
                 "package configuration file provided by \""
              << this->Name << "\", "
                               "but CMake did not find one.\n";
          }

          if (this->Configs.size() == 1) {
            e << "Could not find a package configuration file named \""
              << this->Configs[0] << "\" provided by package \"" << this->Name
              << "\"" << requestedVersionString << ".\n";
          } else {
            e << "Could not find a package configuration file provided by \""
              << this->Name << "\"" << requestedVersionString
              << " with any of the following names:\n"
              << cmWrap("  ", this->Configs, "", "\n") << "\n";
          }

          e << "Add the installation prefix of \"" << this->Name
            << "\" to "
               "CMAKE_PREFIX_PATH or set \""
            << this->Variable
            << "\" to a "
               "directory containing one of the above files. "
               "If \""
            << this->Name << "\" provides a separate development "
                             "package or SDK, be sure it has been installed.";
        } else // if(!this->UseFindModules && !this->UseConfigFiles)
        {
          e << "No \"Find" << this->Name << ".cmake\" found in "
            << "CMAKE_MODULE_PATH.";

          aw
            << "Find" << this->Name
            << ".cmake must either be part of this "
               "project itself, in this case adjust CMAKE_MODULE_PATH so that "
               "it points to the correct location inside its source tree.\n"
               "Or it must be installed by a package which has already been "
               "found via find_package().  In this case make sure that "
               "package has indeed been found and adjust CMAKE_MODULE_PATH to "
               "contain the location where that package has installed "
               "Find"
            << this->Name
            << ".cmake.  This must be a location "
               "provided by that package.  This error in general means that "
               "the buildsystem of this project is relying on a Find-module "
               "without ensuring that it is actually available.\n";
        }
      }

      this->Makefile->IssueMessage(
        this->Required ? cmake::FATAL_ERROR : cmake::WARNING, e.str());
      if (this->Required) {
        cmSystemTools::SetFatalErrorOccured();
      }

      if (!aw.str().empty()) {
        this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, aw.str());
      }
    }
    // output result if in config mode but not in quiet mode
    else if (!this->Quiet) {
      std::ostringstream aw;
      aw << "Could NOT find " << this->Name << " (missing: " << this->Name
         << "_DIR)";
      this->Makefile->DisplayStatus(aw.str().c_str(), -1);
    }
  }

  // Set a variable marking whether the package was found.
  this->Makefile->AddDefinition(foundVar, found ? "1" : "0");

  // Set a variable naming the configuration file that was found.
  std::string fileVar = this->Name;
  fileVar += "_CONFIG";
  if (found) {
    this->Makefile->AddDefinition(fileVar, this->FileFound.c_str());
  } else {
    this->Makefile->RemoveDefinition(fileVar);
  }

  std::string consideredConfigsVar = this->Name;
  consideredConfigsVar += "_CONSIDERED_CONFIGS";
  std::string consideredVersionsVar = this->Name;
  consideredVersionsVar += "_CONSIDERED_VERSIONS";

  std::string consideredConfigFiles;
  std::string consideredVersions;

  const char* sep = "";
  for (std::vector<ConfigFileInfo>::const_iterator i =
         this->ConsideredConfigs.begin();
       i != this->ConsideredConfigs.end(); ++i) {
    consideredConfigFiles += sep;
    consideredVersions += sep;
    consideredConfigFiles += i->filename;
    consideredVersions += i->version;
    sep = ";";
  }

  this->Makefile->AddDefinition(consideredConfigsVar,
                                consideredConfigFiles.c_str());

  this->Makefile->AddDefinition(consideredVersionsVar,
                                consideredVersions.c_str());

  return result;
}

bool cmFindPackageCommand::FindConfig()
{
  // Compute the set of search prefixes.
  this->ComputePrefixes();

  // Look for the project's configuration file.
  bool found = false;

  // Search for frameworks.
  if (!found && (this->SearchFrameworkFirst || this->SearchFrameworkOnly)) {
    found = this->FindFrameworkConfig();
  }

  // Search for apps.
  if (!found && (this->SearchAppBundleFirst || this->SearchAppBundleOnly)) {
    found = this->FindAppBundleConfig();
  }

  // Search prefixes.
  if (!found && !(this->SearchFrameworkOnly || this->SearchAppBundleOnly)) {
    found = this->FindPrefixedConfig();
  }

  // Search for frameworks.
  if (!found && this->SearchFrameworkLast) {
    found = this->FindFrameworkConfig();
  }

  // Search for apps.
  if (!found && this->SearchAppBundleLast) {
    found = this->FindAppBundleConfig();
  }

  // Store the entry in the cache so it can be set by the user.
  std::string init;
  if (found) {
    init = cmSystemTools::GetFilenamePath(this->FileFound);
  } else {
    init = this->Variable + "-NOTFOUND";
  }
  std::string help =
    "The directory containing a CMake configuration file for ";
  help += this->Name;
  help += ".";
  // We force the value since we do not get here if it was already set.
  this->Makefile->AddCacheDefinition(this->Variable, init.c_str(),
                                     help.c_str(), cmStateEnums::PATH, true);
  return found;
}

bool cmFindPackageCommand::FindPrefixedConfig()
{
  std::vector<std::string> const& prefixes = this->SearchPaths;
  for (std::vector<std::string>::const_iterator pi = prefixes.begin();
       pi != prefixes.end(); ++pi) {
    if (this->SearchPrefix(*pi)) {
      return true;
    }
  }
  return false;
}

bool cmFindPackageCommand::FindFrameworkConfig()
{
  std::vector<std::string> const& prefixes = this->SearchPaths;
  for (std::vector<std::string>::const_iterator i = prefixes.begin();
       i != prefixes.end(); ++i) {
    if (this->SearchFrameworkPrefix(*i)) {
      return true;
    }
  }
  return false;
}

bool cmFindPackageCommand::FindAppBundleConfig()
{
  std::vector<std::string> const& prefixes = this->SearchPaths;
  for (std::vector<std::string>::const_iterator i = prefixes.begin();
       i != prefixes.end(); ++i) {
    if (this->SearchAppBundlePrefix(*i)) {
      return true;
    }
  }
  return false;
}

bool cmFindPackageCommand::ReadListFile(const char* f, PolicyScopeRule psr)
{
  const bool noPolicyScope = !this->PolicyScope || psr == NoPolicyScope;
  if (this->Makefile->ReadDependentFile(f, noPolicyScope)) {
    return true;
  }
  std::string e = "Error reading CMake code from \"";
  e += f;
  e += "\".";
  this->SetError(e);
  return false;
}

void cmFindPackageCommand::AppendToFoundProperty(bool found)
{
  std::vector<std::string> foundContents;
  const char* foundProp =
    this->Makefile->GetState()->GetGlobalProperty("PACKAGES_FOUND");
  if (foundProp && *foundProp) {
    std::string tmp = foundProp;

    cmSystemTools::ExpandListArgument(tmp, foundContents, false);
    std::vector<std::string>::iterator nameIt =
      std::find(foundContents.begin(), foundContents.end(), this->Name);
    if (nameIt != foundContents.end()) {
      foundContents.erase(nameIt);
    }
  }

  std::vector<std::string> notFoundContents;
  const char* notFoundProp =
    this->Makefile->GetState()->GetGlobalProperty("PACKAGES_NOT_FOUND");
  if (notFoundProp && *notFoundProp) {
    std::string tmp = notFoundProp;

    cmSystemTools::ExpandListArgument(tmp, notFoundContents, false);
    std::vector<std::string>::iterator nameIt =
      std::find(notFoundContents.begin(), notFoundContents.end(), this->Name);
    if (nameIt != notFoundContents.end()) {
      notFoundContents.erase(nameIt);
    }
  }

  if (found) {
    foundContents.push_back(this->Name);
  } else {
    notFoundContents.push_back(this->Name);
  }

  std::string tmp = cmJoin(foundContents, ";");
  this->Makefile->GetState()->SetGlobalProperty("PACKAGES_FOUND", tmp.c_str());

  tmp = cmJoin(notFoundContents, ";");
  this->Makefile->GetState()->SetGlobalProperty("PACKAGES_NOT_FOUND",
                                                tmp.c_str());
}

void cmFindPackageCommand::AppendSuccessInformation()
{
  {
    std::string transitivePropName = "_CMAKE_";
    transitivePropName += this->Name + "_TRANSITIVE_DEPENDENCY";
    this->Makefile->GetState()->SetGlobalProperty(transitivePropName, "False");
  }
  std::string found = this->Name;
  found += "_FOUND";
  std::string upperFound = cmSystemTools::UpperCase(found);

  const char* upperResult = this->Makefile->GetDefinition(upperFound);
  const char* result = this->Makefile->GetDefinition(found);
  bool packageFound =
    ((cmSystemTools::IsOn(result)) || (cmSystemTools::IsOn(upperResult)));

  this->AppendToFoundProperty(packageFound);

  // Record whether the find was quiet or not, so this can be used
  // e.g. in FeatureSummary.cmake
  std::string quietInfoPropName = "_CMAKE_";
  quietInfoPropName += this->Name;
  quietInfoPropName += "_QUIET";
  this->Makefile->GetState()->SetGlobalProperty(
    quietInfoPropName, this->Quiet ? "TRUE" : "FALSE");

  // set a global property to record the required version of this package
  std::string versionInfoPropName = "_CMAKE_";
  versionInfoPropName += this->Name;
  versionInfoPropName += "_REQUIRED_VERSION";
  std::string versionInfo;
  if (!this->Version.empty()) {
    versionInfo = this->VersionExact ? "==" : ">=";
    versionInfo += " ";
    versionInfo += this->Version;
  }
  this->Makefile->GetState()->SetGlobalProperty(versionInfoPropName,
                                                versionInfo.c_str());
  if (this->Required) {
    std::string requiredInfoPropName = "_CMAKE_";
    requiredInfoPropName += this->Name;
    requiredInfoPropName += "_TYPE";
    this->Makefile->GetState()->SetGlobalProperty(requiredInfoPropName,
                                                  "REQUIRED");
  }

  // Restore original state of "_FIND_" variables we set.
  this->RestoreFindDefinitions();

  // Pop the package stack
  this->Makefile->FindPackageModuleStack.pop_back();
}

void cmFindPackageCommand::ComputePrefixes()
{
  if (!this->NoDefaultPath) {
    if (!this->NoPackageRootPath) {
      this->FillPrefixesPackageRoot();
    }
    if (!this->NoCMakePath) {
      this->FillPrefixesCMakeVariable();
    }
    if (!this->NoCMakeEnvironmentPath) {
      this->FillPrefixesCMakeEnvironment();
    }
  }
  this->FillPrefixesUserHints();
  if (!this->NoDefaultPath) {
    if (!this->NoSystemEnvironmentPath) {
      this->FillPrefixesSystemEnvironment();
    }
    if (!this->NoUserRegistry) {
      this->FillPrefixesUserRegistry();
    }
    if (!this->NoCMakeSystemPath) {
      this->FillPrefixesCMakeSystemVariable();
    }
    if (!this->NoSystemRegistry) {
      this->FillPrefixesSystemRegistry();
    }
  }
  this->FillPrefixesUserGuess();

  this->ComputeFinalPaths();
}

void cmFindPackageCommand::FillPrefixesPackageRoot()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::PackageRoot];

  // Add package specific search prefixes
  // NOTE: This should be using const_reverse_iterator but HP aCC and
  //       Oracle sunCC both currently have standard library issues
  //       with the reverse iterator APIs.
  for (std::deque<std::string>::reverse_iterator pkg =
         this->Makefile->FindPackageModuleStack.rbegin();
       pkg != this->Makefile->FindPackageModuleStack.rend(); ++pkg) {
    std::string varName = *pkg + "_ROOT";
    paths.AddCMakePath(varName);
    paths.AddEnvPath(varName);
  }
}

void cmFindPackageCommand::FillPrefixesCMakeEnvironment()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::CMakeEnvironment];

  // Check the environment variable with the same name as the cache
  // entry.
  paths.AddEnvPath(this->Variable);

  // And now the general CMake environment variables
  paths.AddEnvPath("CMAKE_PREFIX_PATH");
  paths.AddEnvPath("CMAKE_FRAMEWORK_PATH");
  paths.AddEnvPath("CMAKE_APPBUNDLE_PATH");
}

void cmFindPackageCommand::FillPrefixesCMakeVariable()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::CMake];

  paths.AddCMakePath("CMAKE_PREFIX_PATH");
  paths.AddCMakePath("CMAKE_FRAMEWORK_PATH");
  paths.AddCMakePath("CMAKE_APPBUNDLE_PATH");
}

void cmFindPackageCommand::FillPrefixesSystemEnvironment()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::SystemEnvironment];

  // Use the system search path to generate prefixes.
  // Relative paths are interpreted with respect to the current
  // working directory.
  std::vector<std::string> tmp;
  cmSystemTools::GetPath(tmp);
  for (std::vector<std::string>::iterator i = tmp.begin(); i != tmp.end();
       ++i) {
    // If the path is a PREFIX/bin case then add its parent instead.
    if ((cmHasLiteralSuffix(*i, "/bin")) ||
        (cmHasLiteralSuffix(*i, "/sbin"))) {
      paths.AddPath(cmSystemTools::GetFilenamePath(*i));
    } else {
      paths.AddPath(*i);
    }
  }
}

void cmFindPackageCommand::FillPrefixesUserRegistry()
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  this->LoadPackageRegistryWinUser();
#elif defined(__HAIKU__)
  char dir[B_PATH_NAME_LENGTH];
  if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, false, dir, sizeof(dir)) ==
      B_OK) {
    std::string fname = dir;
    fname += "/cmake/packages/";
    fname += Name;
    this->LoadPackageRegistryDir(fname,
                                 this->LabeledPaths[PathLabel::UserRegistry]);
  }
#else
  std::string dir;
  if (cmSystemTools::GetEnv("HOME", dir)) {
    dir += "/.cmake/packages/";
    dir += this->Name;
    this->LoadPackageRegistryDir(dir,
                                 this->LabeledPaths[PathLabel::UserRegistry]);
  }
#endif
}

void cmFindPackageCommand::FillPrefixesSystemRegistry()
{
  if (this->NoSystemRegistry || this->NoDefaultPath) {
    return;
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  this->LoadPackageRegistryWinSystem();
#endif
}

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
// http://msdn.microsoft.com/en-us/library/aa384253%28v=vs.85%29.aspx
#if !defined(KEY_WOW64_32KEY)
#define KEY_WOW64_32KEY 0x0200
#endif
#if !defined(KEY_WOW64_64KEY)
#define KEY_WOW64_64KEY 0x0100
#endif
void cmFindPackageCommand::LoadPackageRegistryWinUser()
{
  // HKEY_CURRENT_USER\\Software shares 32-bit and 64-bit views.
  this->LoadPackageRegistryWin(true, 0,
                               this->LabeledPaths[PathLabel::UserRegistry]);
}

void cmFindPackageCommand::LoadPackageRegistryWinSystem()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::SystemRegistry];

  // HKEY_LOCAL_MACHINE\\SOFTWARE has separate 32-bit and 64-bit views.
  // Prefer the target platform view first.
  if (this->Makefile->PlatformIs64Bit()) {
    this->LoadPackageRegistryWin(false, KEY_WOW64_64KEY, paths);
    this->LoadPackageRegistryWin(false, KEY_WOW64_32KEY, paths);
  } else {
    this->LoadPackageRegistryWin(false, KEY_WOW64_32KEY, paths);
    this->LoadPackageRegistryWin(false, KEY_WOW64_64KEY, paths);
  }
}

void cmFindPackageCommand::LoadPackageRegistryWin(bool user, unsigned int view,
                                                  cmSearchPath& outPaths)
{
  std::wstring key = L"Software\\Kitware\\CMake\\Packages\\";
  key += cmsys::Encoding::ToWide(this->Name);
  std::set<std::wstring> bad;
  HKEY hKey;
  if (RegOpenKeyExW(user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key.c_str(),
                    0, KEY_QUERY_VALUE | view, &hKey) == ERROR_SUCCESS) {
    DWORD valueType = REG_NONE;
    wchar_t name[16383]; // RegEnumValue docs limit name to 32767 _bytes_
    std::vector<wchar_t> data(512);
    bool done = false;
    DWORD index = 0;
    while (!done) {
      DWORD nameSize = static_cast<DWORD>(sizeof(name));
      DWORD dataSize = static_cast<DWORD>(data.size() * sizeof(data[0]));
      switch (RegEnumValueW(hKey, index, name, &nameSize, 0, &valueType,
                            (BYTE*)&data[0], &dataSize)) {
        case ERROR_SUCCESS:
          ++index;
          if (valueType == REG_SZ) {
            data[dataSize] = 0;
            if (!this->CheckPackageRegistryEntry(
                  cmsys::Encoding::ToNarrow(&data[0]), outPaths)) {
              // The entry is invalid.
              bad.insert(name);
            }
          }
          break;
        case ERROR_MORE_DATA:
          data.resize((dataSize + sizeof(data[0]) - 1) / sizeof(data[0]));
          break;
        case ERROR_NO_MORE_ITEMS:
        default:
          done = true;
          break;
      }
    }
    RegCloseKey(hKey);
  }

  // Remove bad values if possible.
  if (user && !bad.empty() &&
      RegOpenKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, KEY_SET_VALUE | view,
                    &hKey) == ERROR_SUCCESS) {
    for (std::set<std::wstring>::const_iterator vi = bad.begin();
         vi != bad.end(); ++vi) {
      RegDeleteValueW(hKey, vi->c_str());
    }
    RegCloseKey(hKey);
  }
}
#else
class cmFindPackageCommandHoldFile
{
  const char* File;

public:
  cmFindPackageCommandHoldFile(const char* f)
    : File(f)
  {
  }
  ~cmFindPackageCommandHoldFile()
  {
    if (this->File) {
      cmSystemTools::RemoveFile(this->File);
    }
  }
  void Release() { this->File = CM_NULLPTR; }
};

void cmFindPackageCommand::LoadPackageRegistryDir(std::string const& dir,
                                                  cmSearchPath& outPaths)
{
  cmsys::Directory files;
  if (!files.Load(dir)) {
    return;
  }

  std::string fname;
  for (unsigned long i = 0; i < files.GetNumberOfFiles(); ++i) {
    fname = dir;
    fname += "/";
    fname += files.GetFile(i);

    if (!cmSystemTools::FileIsDirectory(fname)) {
      // Hold this file hostage until it behaves.
      cmFindPackageCommandHoldFile holdFile(fname.c_str());

      // Load the file.
      cmsys::ifstream fin(fname.c_str(), std::ios::in | std::ios::binary);
      std::string fentry;
      if (fin && cmSystemTools::GetLineFromStream(fin, fentry) &&
          this->CheckPackageRegistryEntry(fentry, outPaths)) {
        // The file references an existing package, so release it.
        holdFile.Release();
      }
    }
  }

  // TODO: Wipe out the directory if it is empty.
}
#endif

bool cmFindPackageCommand::CheckPackageRegistryEntry(const std::string& fname,
                                                     cmSearchPath& outPaths)
{
  // Parse the content of one package registry entry.
  if (cmSystemTools::FileIsFullPath(fname.c_str())) {
    // The first line in the stream is the full path to a file or
    // directory containing the package.
    if (cmSystemTools::FileExists(fname.c_str())) {
      // The path exists.  Look for the package here.
      if (!cmSystemTools::FileIsDirectory(fname)) {
        outPaths.AddPath(cmSystemTools::GetFilenamePath(fname));
      } else {
        outPaths.AddPath(fname);
      }
      return true;
    }
    // The path does not exist.  Assume the stream content is
    // associated with an old package that no longer exists, and
    // delete it to keep the package registry clean.
    return false;
  }
  // The first line in the stream is not the full path to a file or
  // directory.  Assume the stream content was created by a future
  // version of CMake that uses a different format, and leave it.
  return true;
}

void cmFindPackageCommand::FillPrefixesCMakeSystemVariable()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::CMakeSystem];

  paths.AddCMakePath("CMAKE_SYSTEM_PREFIX_PATH");
  paths.AddCMakePath("CMAKE_SYSTEM_FRAMEWORK_PATH");
  paths.AddCMakePath("CMAKE_SYSTEM_APPBUNDLE_PATH");
}

void cmFindPackageCommand::FillPrefixesUserGuess()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::Guess];

  for (std::vector<std::string>::const_iterator p =
         this->UserGuessArgs.begin();
       p != this->UserGuessArgs.end(); ++p) {
    paths.AddUserPath(*p);
  }
}

void cmFindPackageCommand::FillPrefixesUserHints()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::Hints];

  for (std::vector<std::string>::const_iterator p =
         this->UserHintsArgs.begin();
       p != this->UserHintsArgs.end(); ++p) {
    paths.AddUserPath(*p);
  }
}

bool cmFindPackageCommand::SearchDirectory(std::string const& dir)
{
  assert(!dir.empty() && dir[dir.size() - 1] == '/');

  // Check each path suffix on this directory.
  for (std::vector<std::string>::const_iterator si =
         this->SearchPathSuffixes.begin();
       si != this->SearchPathSuffixes.end(); ++si) {
    std::string d = dir;
    if (!si->empty()) {
      d += *si;
      d += "/";
    }
    if (this->CheckDirectory(d)) {
      return true;
    }
  }
  return false;
}

bool cmFindPackageCommand::CheckDirectory(std::string const& dir)
{
  assert(!dir.empty() && dir[dir.size() - 1] == '/');

  // Look for the file in this directory.
  std::string d = dir.substr(0, dir.size() - 1);
  if (this->FindConfigFile(d, this->FileFound)) {
    // Remove duplicate slashes.
    cmSystemTools::ConvertToUnixSlashes(this->FileFound);
    return true;
  }
  return false;
}

bool cmFindPackageCommand::FindConfigFile(std::string const& dir,
                                          std::string& file)
{
  if (this->IgnoredPaths.count(dir)) {
    return false;
  }

  for (std::vector<std::string>::const_iterator ci = this->Configs.begin();
       ci != this->Configs.end(); ++ci) {
    file = dir;
    file += "/";
    file += *ci;
    if (this->DebugMode) {
      fprintf(stderr, "Checking file [%s]\n", file.c_str());
    }
    if (cmSystemTools::FileExists(file.c_str(), true) &&
        this->CheckVersion(file)) {
      return true;
    }
  }
  return false;
}

bool cmFindPackageCommand::CheckVersion(std::string const& config_file)
{
  bool result = false; // by default, assume the version is not ok.
  bool haveResult = false;
  std::string version = "unknown";

  // Get the filename without the .cmake extension.
  std::string::size_type pos = config_file.rfind('.');
  std::string version_file_base = config_file.substr(0, pos);

  // Look for foo-config-version.cmake
  std::string version_file = version_file_base;
  version_file += "-version.cmake";
  if (!haveResult && cmSystemTools::FileExists(version_file.c_str(), true)) {
    result = this->CheckVersionFile(version_file, version);
    haveResult = true;
  }

  // Look for fooConfigVersion.cmake
  version_file = version_file_base;
  version_file += "Version.cmake";
  if (!haveResult && cmSystemTools::FileExists(version_file.c_str(), true)) {
    result = this->CheckVersionFile(version_file, version);
    haveResult = true;
  }

  // If no version was requested a versionless package is acceptable.
  if (!haveResult && this->Version.empty()) {
    result = true;
  }

  ConfigFileInfo configFileInfo;
  configFileInfo.filename = config_file;
  configFileInfo.version = version;
  this->ConsideredConfigs.push_back(configFileInfo);

  return result;
}

bool cmFindPackageCommand::CheckVersionFile(std::string const& version_file,
                                            std::string& result_version)
{
  // The version file will be loaded in an isolated scope.
  cmMakefile::ScopePushPop varScope(this->Makefile);
  cmMakefile::PolicyPushPop polScope(this->Makefile);
  static_cast<void>(varScope);
  static_cast<void>(polScope);

  // Clear the output variables.
  this->Makefile->RemoveDefinition("PACKAGE_VERSION");
  this->Makefile->RemoveDefinition("PACKAGE_VERSION_UNSUITABLE");
  this->Makefile->RemoveDefinition("PACKAGE_VERSION_COMPATIBLE");
  this->Makefile->RemoveDefinition("PACKAGE_VERSION_EXACT");

  // Set the input variables.
  this->Makefile->AddDefinition("PACKAGE_FIND_NAME", this->Name.c_str());
  this->Makefile->AddDefinition("PACKAGE_FIND_VERSION", this->Version.c_str());
  char buf[64];
  sprintf(buf, "%u", this->VersionMajor);
  this->Makefile->AddDefinition("PACKAGE_FIND_VERSION_MAJOR", buf);
  sprintf(buf, "%u", this->VersionMinor);
  this->Makefile->AddDefinition("PACKAGE_FIND_VERSION_MINOR", buf);
  sprintf(buf, "%u", this->VersionPatch);
  this->Makefile->AddDefinition("PACKAGE_FIND_VERSION_PATCH", buf);
  sprintf(buf, "%u", this->VersionTweak);
  this->Makefile->AddDefinition("PACKAGE_FIND_VERSION_TWEAK", buf);
  sprintf(buf, "%u", this->VersionCount);
  this->Makefile->AddDefinition("PACKAGE_FIND_VERSION_COUNT", buf);

  // Load the version check file.  Pass NoPolicyScope because we do
  // our own policy push/pop independent of CMP0011.
  bool suitable = false;
  if (this->ReadListFile(version_file.c_str(), NoPolicyScope)) {
    // Check the output variables.
    bool okay = this->Makefile->IsOn("PACKAGE_VERSION_EXACT");
    bool unsuitable = this->Makefile->IsOn("PACKAGE_VERSION_UNSUITABLE");
    if (!okay && !this->VersionExact) {
      okay = this->Makefile->IsOn("PACKAGE_VERSION_COMPATIBLE");
    }

    // The package is suitable if the version is okay and not
    // explicitly unsuitable.
    suitable = !unsuitable && (okay || this->Version.empty());
    if (suitable) {
      // Get the version found.
      this->VersionFound =
        this->Makefile->GetSafeDefinition("PACKAGE_VERSION");

      // Try to parse the version number and store the results that were
      // successfully parsed.
      unsigned int parsed_major;
      unsigned int parsed_minor;
      unsigned int parsed_patch;
      unsigned int parsed_tweak;
      this->VersionFoundCount =
        sscanf(this->VersionFound.c_str(), "%u.%u.%u.%u", &parsed_major,
               &parsed_minor, &parsed_patch, &parsed_tweak);
      switch (this->VersionFoundCount) {
        case 4:
          this->VersionFoundTweak = parsed_tweak;
          CM_FALLTHROUGH;
        case 3:
          this->VersionFoundPatch = parsed_patch;
          CM_FALLTHROUGH;
        case 2:
          this->VersionFoundMinor = parsed_minor;
          CM_FALLTHROUGH;
        case 1:
          this->VersionFoundMajor = parsed_major;
          CM_FALLTHROUGH;
        default:
          break;
      }
    }
  }

  result_version = this->Makefile->GetSafeDefinition("PACKAGE_VERSION");
  if (result_version.empty()) {
    result_version = "unknown";
  }

  // Succeed if the version is suitable.
  return suitable;
}

void cmFindPackageCommand::StoreVersionFound()
{
  // Store the whole version string.
  std::string ver = this->Name;
  ver += "_VERSION";
  if (this->VersionFound.empty()) {
    this->Makefile->RemoveDefinition(ver);
  } else {
    this->Makefile->AddDefinition(ver, this->VersionFound.c_str());
  }

  // Store the version components.
  char buf[64];
  sprintf(buf, "%u", this->VersionFoundMajor);
  this->Makefile->AddDefinition(ver + "_MAJOR", buf);
  sprintf(buf, "%u", this->VersionFoundMinor);
  this->Makefile->AddDefinition(ver + "_MINOR", buf);
  sprintf(buf, "%u", this->VersionFoundPatch);
  this->Makefile->AddDefinition(ver + "_PATCH", buf);
  sprintf(buf, "%u", this->VersionFoundTweak);
  this->Makefile->AddDefinition(ver + "_TWEAK", buf);
  sprintf(buf, "%u", this->VersionFoundCount);
  this->Makefile->AddDefinition(ver + "_COUNT", buf);
}

class cmFileListGeneratorBase
{
public:
  virtual ~cmFileListGeneratorBase() {}

protected:
  bool Consider(std::string const& fullPath, cmFileList& listing);

private:
  bool Search(cmFileList&);
  virtual bool Search(std::string const& parent, cmFileList&) = 0;
  virtual CM_AUTO_PTR<cmFileListGeneratorBase> Clone() const = 0;
  friend class cmFileList;
  cmFileListGeneratorBase* SetNext(cmFileListGeneratorBase const& next);
  CM_AUTO_PTR<cmFileListGeneratorBase> Next;
};

class cmFileList
{
public:
  cmFileList()
    : First()
    , Last(CM_NULLPTR)
  {
  }
  virtual ~cmFileList() {}
  cmFileList& operator/(cmFileListGeneratorBase const& rhs)
  {
    if (this->Last) {
      this->Last = this->Last->SetNext(rhs);
    } else {
      this->First = rhs.Clone();
      this->Last = this->First.get();
    }
    return *this;
  }
  bool Search()
  {
    if (this->First.get()) {
      return this->First->Search(*this);
    }
    return false;
  }

private:
  virtual bool Visit(std::string const& fullPath) = 0;
  friend class cmFileListGeneratorBase;
  CM_AUTO_PTR<cmFileListGeneratorBase> First;
  cmFileListGeneratorBase* Last;
};

class cmFindPackageFileList : public cmFileList
{
public:
  cmFindPackageFileList(cmFindPackageCommand* fpc, bool use_suffixes = true)
    : cmFileList()
    , FPC(fpc)
    , UseSuffixes(use_suffixes)
  {
  }

private:
  bool Visit(std::string const& fullPath) CM_OVERRIDE
  {
    if (this->UseSuffixes) {
      return this->FPC->SearchDirectory(fullPath);
    }
    return this->FPC->CheckDirectory(fullPath);
  }
  cmFindPackageCommand* FPC;
  bool UseSuffixes;
};

bool cmFileListGeneratorBase::Search(cmFileList& listing)
{
  return this->Search("", listing);
}

cmFileListGeneratorBase* cmFileListGeneratorBase::SetNext(
  cmFileListGeneratorBase const& next)
{
  this->Next = next.Clone();
  return this->Next.get();
}

bool cmFileListGeneratorBase::Consider(std::string const& fullPath,
                                       cmFileList& listing)
{
  if (this->Next.get()) {
    return this->Next->Search(fullPath + "/", listing);
  }
  return listing.Visit(fullPath + "/");
}

class cmFileListGeneratorFixed : public cmFileListGeneratorBase
{
public:
  cmFileListGeneratorFixed(std::string const& str)
    : cmFileListGeneratorBase()
    , String(str)
  {
  }
  cmFileListGeneratorFixed(cmFileListGeneratorFixed const& r)
    : cmFileListGeneratorBase()
    , String(r.String)
  {
  }

private:
  std::string String;
  bool Search(std::string const& parent, cmFileList& lister) CM_OVERRIDE
  {
    std::string fullPath = parent + this->String;
    return this->Consider(fullPath, lister);
  }
  CM_AUTO_PTR<cmFileListGeneratorBase> Clone() const CM_OVERRIDE
  {
    CM_AUTO_PTR<cmFileListGeneratorBase> g(
      new cmFileListGeneratorFixed(*this));
    return g;
  }
};

class cmFileListGeneratorEnumerate : public cmFileListGeneratorBase
{
public:
  cmFileListGeneratorEnumerate(std::vector<std::string> const& v)
    : cmFileListGeneratorBase()
    , Vector(v)
  {
  }
  cmFileListGeneratorEnumerate(cmFileListGeneratorEnumerate const& r)
    : cmFileListGeneratorBase()
    , Vector(r.Vector)
  {
  }

private:
  std::vector<std::string> const& Vector;
  bool Search(std::string const& parent, cmFileList& lister) CM_OVERRIDE
  {
    for (std::vector<std::string>::const_iterator i = this->Vector.begin();
         i != this->Vector.end(); ++i) {
      if (this->Consider(parent + *i, lister)) {
        return true;
      }
    }
    return false;
  }
  CM_AUTO_PTR<cmFileListGeneratorBase> Clone() const CM_OVERRIDE
  {
    CM_AUTO_PTR<cmFileListGeneratorBase> g(
      new cmFileListGeneratorEnumerate(*this));
    return g;
  }
};

class cmFileListGeneratorProject : public cmFileListGeneratorBase
{
public:
  cmFileListGeneratorProject(std::vector<std::string> const& names,
                             cmFindPackageCommand::SortOrderType so,
                             cmFindPackageCommand::SortDirectionType sd)
    : cmFileListGeneratorBase()
    , Names(names)
  {
    this->SetSort(so, sd);
  }
  cmFileListGeneratorProject(cmFileListGeneratorProject const& r)
    : cmFileListGeneratorBase()
    , Names(r.Names)
  {
    this->SetSort(r.SortOrder, r.SortDirection);
  }

  void SetSort(cmFindPackageCommand::SortOrderType o,
               cmFindPackageCommand::SortDirectionType d)
  {
    SortOrder = o;
    SortDirection = d;
  }

protected:
  // sort parameters
  cmFindPackageCommand::SortOrderType SortOrder;
  cmFindPackageCommand::SortDirectionType SortDirection;

private:
  std::vector<std::string> const& Names;
  bool Search(std::string const& parent, cmFileList& lister) CM_OVERRIDE
  {
    // Construct a list of matches.
    std::vector<std::string> matches;
    cmsys::Directory d;
    d.Load(parent);
    for (unsigned long i = 0; i < d.GetNumberOfFiles(); ++i) {
      const char* fname = d.GetFile(i);
      if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0) {
        continue;
      }
      for (std::vector<std::string>::const_iterator ni = this->Names.begin();
           ni != this->Names.end(); ++ni) {
        if (cmsysString_strncasecmp(fname, ni->c_str(), ni->length()) == 0) {
          matches.push_back(fname);
        }
      }
    }

    // before testing the matches check if there is a specific sorting order to
    // perform
    if (this->SortOrder != cmFindPackageCommand::None) {
      cmFindPackageCommand::Sort(matches.begin(), matches.end(), SortOrder,
                                 SortDirection);
    }

    for (std::vector<std::string>::const_iterator i = matches.begin();
         i != matches.end(); ++i) {
      if (this->Consider(parent + *i, lister)) {
        return true;
      }
    }
    return false;
  }
  CM_AUTO_PTR<cmFileListGeneratorBase> Clone() const CM_OVERRIDE
  {
    CM_AUTO_PTR<cmFileListGeneratorBase> g(
      new cmFileListGeneratorProject(*this));
    return g;
  }
};

class cmFileListGeneratorMacProject : public cmFileListGeneratorBase
{
public:
  cmFileListGeneratorMacProject(std::vector<std::string> const& names,
                                const char* ext)
    : cmFileListGeneratorBase()
    , Names(names)
    , Extension(ext)
  {
  }
  cmFileListGeneratorMacProject(cmFileListGeneratorMacProject const& r)
    : cmFileListGeneratorBase()
    , Names(r.Names)
    , Extension(r.Extension)
  {
  }

private:
  std::vector<std::string> const& Names;
  std::string Extension;
  bool Search(std::string const& parent, cmFileList& lister) CM_OVERRIDE
  {
    // Construct a list of matches.
    std::vector<std::string> matches;
    cmsys::Directory d;
    d.Load(parent);
    for (unsigned long i = 0; i < d.GetNumberOfFiles(); ++i) {
      const char* fname = d.GetFile(i);
      if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0) {
        continue;
      }
      for (std::vector<std::string>::const_iterator ni = this->Names.begin();
           ni != this->Names.end(); ++ni) {
        std::string name = *ni;
        name += this->Extension;
        if (cmsysString_strcasecmp(fname, name.c_str()) == 0) {
          matches.push_back(fname);
        }
      }
    }

    for (std::vector<std::string>::const_iterator i = matches.begin();
         i != matches.end(); ++i) {
      if (this->Consider(parent + *i, lister)) {
        return true;
      }
    }
    return false;
  }
  CM_AUTO_PTR<cmFileListGeneratorBase> Clone() const CM_OVERRIDE
  {
    CM_AUTO_PTR<cmFileListGeneratorBase> g(
      new cmFileListGeneratorMacProject(*this));
    return g;
  }
};

class cmFileListGeneratorCaseInsensitive : public cmFileListGeneratorBase
{
public:
  cmFileListGeneratorCaseInsensitive(std::string const& str)
    : cmFileListGeneratorBase()
    , String(str)
  {
  }
  cmFileListGeneratorCaseInsensitive(
    cmFileListGeneratorCaseInsensitive const& r)
    : cmFileListGeneratorBase()
    , String(r.String)
  {
  }

private:
  std::string String;
  bool Search(std::string const& parent, cmFileList& lister) CM_OVERRIDE
  {
    // Look for matching files.
    std::vector<std::string> matches;
    cmsys::Directory d;
    d.Load(parent);
    for (unsigned long i = 0; i < d.GetNumberOfFiles(); ++i) {
      const char* fname = d.GetFile(i);
      if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0) {
        continue;
      }
      if (cmsysString_strcasecmp(fname, this->String.c_str()) == 0) {
        if (this->Consider(parent + fname, lister)) {
          return true;
        }
      }
    }
    return false;
  }
  CM_AUTO_PTR<cmFileListGeneratorBase> Clone() const CM_OVERRIDE
  {
    CM_AUTO_PTR<cmFileListGeneratorBase> g(
      new cmFileListGeneratorCaseInsensitive(*this));
    return g;
  }
};

class cmFileListGeneratorGlob : public cmFileListGeneratorBase
{
public:
  cmFileListGeneratorGlob(std::string const& str)
    : cmFileListGeneratorBase()
    , Pattern(str)
  {
  }
  cmFileListGeneratorGlob(cmFileListGeneratorGlob const& r)
    : cmFileListGeneratorBase()
    , Pattern(r.Pattern)
  {
  }

private:
  std::string Pattern;
  bool Search(std::string const& parent, cmFileList& lister) CM_OVERRIDE
  {
    // Glob the set of matching files.
    std::string expr = parent;
    expr += this->Pattern;
    cmsys::Glob g;
    if (!g.FindFiles(expr)) {
      return false;
    }
    std::vector<std::string> const& files = g.GetFiles();

    // Look for directories among the matches.
    for (std::vector<std::string>::const_iterator fi = files.begin();
         fi != files.end(); ++fi) {
      if (cmSystemTools::FileIsDirectory(*fi)) {
        if (this->Consider(*fi, lister)) {
          return true;
        }
      }
    }
    return false;
  }
  CM_AUTO_PTR<cmFileListGeneratorBase> Clone() const CM_OVERRIDE
  {
    CM_AUTO_PTR<cmFileListGeneratorBase> g(new cmFileListGeneratorGlob(*this));
    return g;
  }
};

bool cmFindPackageCommand::SearchPrefix(std::string const& prefix_in)
{
  assert(!prefix_in.empty() && prefix_in[prefix_in.size() - 1] == '/');
  if (this->DebugMode) {
    fprintf(stderr, "Checking prefix [%s]\n", prefix_in.c_str());
  }

  // Skip this if the prefix does not exist.
  if (!cmSystemTools::FileIsDirectory(prefix_in)) {
    return false;
  }

  //  PREFIX/ (useful on windows or in build trees)
  if (this->SearchDirectory(prefix_in)) {
    return true;
  }

  // Strip the trailing slash because the path generator is about to
  // add one.
  std::string prefix = prefix_in.substr(0, prefix_in.size() - 1);

  //  PREFIX/(cmake|CMake)/ (useful on windows or in build trees)
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorCaseInsensitive("cmake");
    if (lister.Search()) {
      return true;
    }
  }

  //  PREFIX/(Foo|foo|FOO).*/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection);
    if (lister.Search()) {
      return true;
    }
  }

  //  PREFIX/(Foo|foo|FOO).*/(cmake|CMake)/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection) /
      cmFileListGeneratorCaseInsensitive("cmake");
    if (lister.Search()) {
      return true;
    }
  }

  // Construct list of common install locations (lib and share).
  std::vector<std::string> common;
  if (!this->LibraryArchitecture.empty()) {
    common.push_back("lib/" + this->LibraryArchitecture);
  }
  if (this->UseLib32Paths) {
    common.push_back("lib32");
  }
  if (this->UseLib64Paths) {
    common.push_back("lib64");
  }
  if (this->UseLibx32Paths) {
    common.push_back("libx32");
  }
  common.push_back("lib");
  common.push_back("share");

  //  PREFIX/(lib/ARCH|lib|share)/cmake/(Foo|foo|FOO).*/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorEnumerate(common) /
      cmFileListGeneratorFixed("cmake") /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection);
    if (lister.Search()) {
      return true;
    }
  }

  //  PREFIX/(lib/ARCH|lib|share)/(Foo|foo|FOO).*/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorEnumerate(common) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection);
    if (lister.Search()) {
      return true;
    }
  }

  //  PREFIX/(lib/ARCH|lib|share)/(Foo|foo|FOO).*/(cmake|CMake)/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorEnumerate(common) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection) /
      cmFileListGeneratorCaseInsensitive("cmake");
    if (lister.Search()) {
      return true;
    }
  }

  // PREFIX/(Foo|foo|FOO).*/(lib/ARCH|lib|share)/cmake/(Foo|foo|FOO).*/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection) /
      cmFileListGeneratorEnumerate(common) /
      cmFileListGeneratorFixed("cmake") /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection);
    if (lister.Search()) {
      return true;
    }
  }

  // PREFIX/(Foo|foo|FOO).*/(lib/ARCH|lib|share)/(Foo|foo|FOO).*/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection) /
      cmFileListGeneratorEnumerate(common) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection);
    if (lister.Search()) {
      return true;
    }
  }

  // PREFIX/(Foo|foo|FOO).*/(lib/ARCH|lib|share)/(Foo|foo|FOO).*/(cmake|CMake)/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection) /
      cmFileListGeneratorEnumerate(common) /
      cmFileListGeneratorProject(this->Names, this->SortOrder,
                                 this->SortDirection) /
      cmFileListGeneratorCaseInsensitive("cmake");
    if (lister.Search()) {
      return true;
    }
  }

  return false;
}

bool cmFindPackageCommand::SearchFrameworkPrefix(std::string const& prefix_in)
{
  assert(!prefix_in.empty() && prefix_in[prefix_in.size() - 1] == '/');
  if (this->DebugMode) {
    fprintf(stderr, "Checking framework prefix [%s]\n", prefix_in.c_str());
  }

  // Strip the trailing slash because the path generator is about to
  // add one.
  std::string prefix = prefix_in.substr(0, prefix_in.size() - 1);

  // <prefix>/Foo.framework/Resources/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorMacProject(this->Names, ".framework") /
      cmFileListGeneratorFixed("Resources");
    if (lister.Search()) {
      return true;
    }
  }
  // <prefix>/Foo.framework/Resources/CMake/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorMacProject(this->Names, ".framework") /
      cmFileListGeneratorFixed("Resources") /
      cmFileListGeneratorCaseInsensitive("cmake");
    if (lister.Search()) {
      return true;
    }
  }

  // <prefix>/Foo.framework/Versions/*/Resources/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorMacProject(this->Names, ".framework") /
      cmFileListGeneratorFixed("Versions") /
      cmFileListGeneratorGlob("*/Resources");
    if (lister.Search()) {
      return true;
    }
  }

  // <prefix>/Foo.framework/Versions/*/Resources/CMake/
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorMacProject(this->Names, ".framework") /
      cmFileListGeneratorFixed("Versions") /
      cmFileListGeneratorGlob("*/Resources") /
      cmFileListGeneratorCaseInsensitive("cmake");
    if (lister.Search()) {
      return true;
    }
  }

  return false;
}

bool cmFindPackageCommand::SearchAppBundlePrefix(std::string const& prefix_in)
{
  assert(!prefix_in.empty() && prefix_in[prefix_in.size() - 1] == '/');
  if (this->DebugMode) {
    fprintf(stderr, "Checking bundle prefix [%s]\n", prefix_in.c_str());
  }

  // Strip the trailing slash because the path generator is about to
  // add one.
  std::string prefix = prefix_in.substr(0, prefix_in.size() - 1);

  // <prefix>/Foo.app/Contents/Resources
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorMacProject(this->Names, ".app") /
      cmFileListGeneratorFixed("Contents/Resources");
    if (lister.Search()) {
      return true;
    }
  }

  // <prefix>/Foo.app/Contents/Resources/CMake
  {
    cmFindPackageFileList lister(this);
    lister / cmFileListGeneratorFixed(prefix) /
      cmFileListGeneratorMacProject(this->Names, ".app") /
      cmFileListGeneratorFixed("Contents/Resources") /
      cmFileListGeneratorCaseInsensitive("cmake");
    if (lister.Search()) {
      return true;
    }
  }

  return false;
}

// TODO: Debug cmsys::Glob double slash problem.
