/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFindBase.h"

#include "cmConfigure.h"
#include <deque>
#include <iostream>
#include <iterator>
#include <map>
#include <stddef.h>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmSearchPath.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

cmFindBase::cmFindBase()
{
  this->AlreadyInCache = false;
  this->AlreadyInCacheWithoutMetaInfo = false;
  this->NamesPerDir = false;
  this->NamesPerDirAllowed = false;
}

bool cmFindBase::ParseArguments(std::vector<std::string> const& argsIn)
{
  if (argsIn.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // copy argsIn into args so it can be modified,
  // in the process extract the DOC "documentation"
  size_t size = argsIn.size();
  std::vector<std::string> args;
  bool foundDoc = false;
  for (unsigned int j = 0; j < size; ++j) {
    if (foundDoc || argsIn[j] != "DOC") {
      if (argsIn[j] == "ENV") {
        if (j + 1 < size) {
          j++;
          cmSystemTools::GetPath(args, argsIn[j].c_str());
        }
      } else {
        args.push_back(argsIn[j]);
      }
    } else {
      if (j + 1 < size) {
        foundDoc = true;
        this->VariableDocumentation = argsIn[j + 1];
        j++;
        if (j >= size) {
          break;
        }
      }
    }
  }
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  this->VariableName = args[0];
  if (this->CheckForVariableInCache()) {
    this->AlreadyInCache = true;
    return true;
  }
  this->AlreadyInCache = false;

  this->SelectDefaultNoPackageRootPath();

  // Find the current root path mode.
  this->SelectDefaultRootPathMode();

  // Find the current bundle/framework search policy.
  this->SelectDefaultMacMode();

  bool newStyle = false;
  enum Doing
  {
    DoingNone,
    DoingNames,
    DoingPaths,
    DoingPathSuffixes,
    DoingHints
  };
  Doing doing = DoingNames; // assume it starts with a name
  for (unsigned int j = 1; j < args.size(); ++j) {
    if (args[j] == "NAMES") {
      doing = DoingNames;
      newStyle = true;
    } else if (args[j] == "PATHS") {
      doing = DoingPaths;
      newStyle = true;
    } else if (args[j] == "HINTS") {
      doing = DoingHints;
      newStyle = true;
    } else if (args[j] == "PATH_SUFFIXES") {
      doing = DoingPathSuffixes;
      newStyle = true;
    } else if (args[j] == "NAMES_PER_DIR") {
      doing = DoingNone;
      if (this->NamesPerDirAllowed) {
        this->NamesPerDir = true;
      } else {
        this->SetError("does not support NAMES_PER_DIR");
        return false;
      }
    } else if (args[j] == "NO_SYSTEM_PATH") {
      doing = DoingNone;
      this->NoDefaultPath = true;
    } else if (this->CheckCommonArgument(args[j])) {
      doing = DoingNone;
      // Some common arguments were accidentally supported by CMake
      // 2.4 and 2.6.0 in the short-hand form of the command, so we
      // must support it even though it is not documented.
    } else if (doing == DoingNames) {
      this->Names.push_back(args[j]);
    } else if (doing == DoingPaths) {
      this->UserGuessArgs.push_back(args[j]);
    } else if (doing == DoingHints) {
      this->UserHintsArgs.push_back(args[j]);
    } else if (doing == DoingPathSuffixes) {
      this->AddPathSuffix(args[j]);
    }
  }

  if (this->VariableDocumentation.empty()) {
    this->VariableDocumentation = "Where can ";
    if (this->Names.empty()) {
      this->VariableDocumentation += "the (unknown) library be found";
    } else if (this->Names.size() == 1) {
      this->VariableDocumentation +=
        "the " + this->Names[0] + " library be found";
    } else {
      this->VariableDocumentation += "one of the ";
      this->VariableDocumentation +=
        cmJoin(cmMakeRange(this->Names).retreat(1), ", ");
      this->VariableDocumentation +=
        " or " + this->Names[this->Names.size() - 1] + " libraries be found";
    }
  }

  // look for old style
  // FIND_*(VAR name path1 path2 ...)
  if (!newStyle) {
    // All the short-hand arguments have been recorded as names.
    std::vector<std::string> shortArgs = this->Names;
    this->Names.clear(); // clear out any values in Names
    this->Names.push_back(shortArgs[0]);
    this->UserGuessArgs.insert(this->UserGuessArgs.end(),
                               shortArgs.begin() + 1, shortArgs.end());
  }
  this->ExpandPaths();

  this->ComputeFinalPaths();

  return true;
}

void cmFindBase::ExpandPaths()
{
  if (!this->NoDefaultPath) {
    if (!this->NoPackageRootPath) {
      this->FillPackageRootPath();
    }
    if (!this->NoCMakePath) {
      this->FillCMakeVariablePath();
    }
    if (!this->NoCMakeEnvironmentPath) {
      this->FillCMakeEnvironmentPath();
    }
  }
  this->FillUserHintsPath();
  if (!this->NoDefaultPath) {
    if (!this->NoSystemEnvironmentPath) {
      this->FillSystemEnvironmentPath();
    }
    if (!this->NoCMakeSystemPath) {
      this->FillCMakeSystemVariablePath();
    }
  }
  this->FillUserGuessPath();
}

void cmFindBase::FillCMakeEnvironmentPath()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::CMakeEnvironment];

  // Add CMAKE_*_PATH environment variables
  std::string var = "CMAKE_";
  var += this->CMakePathName;
  var += "_PATH";
  paths.AddEnvPrefixPath("CMAKE_PREFIX_PATH");
  paths.AddEnvPath(var);

  if (this->CMakePathName == "PROGRAM") {
    paths.AddEnvPath("CMAKE_APPBUNDLE_PATH");
  } else {
    paths.AddEnvPath("CMAKE_FRAMEWORK_PATH");
  }
  paths.AddSuffixes(this->SearchPathSuffixes);
}

void cmFindBase::FillPackageRootPath()
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
    paths.AddCMakePrefixPath(varName);
    paths.AddEnvPrefixPath(varName);
  }

  paths.AddSuffixes(this->SearchPathSuffixes);
}

void cmFindBase::FillCMakeVariablePath()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::CMake];

  // Add CMake varibles of the same name as the previous environment
  // varibles CMAKE_*_PATH to be used most of the time with -D
  // command line options
  std::string var = "CMAKE_";
  var += this->CMakePathName;
  var += "_PATH";
  paths.AddCMakePrefixPath("CMAKE_PREFIX_PATH");
  paths.AddCMakePath(var);

  if (this->CMakePathName == "PROGRAM") {
    paths.AddCMakePath("CMAKE_APPBUNDLE_PATH");
  } else {
    paths.AddCMakePath("CMAKE_FRAMEWORK_PATH");
  }
  paths.AddSuffixes(this->SearchPathSuffixes);
}

void cmFindBase::FillSystemEnvironmentPath()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::SystemEnvironment];

  // Add LIB or INCLUDE
  if (!this->EnvironmentPath.empty()) {
    paths.AddEnvPath(this->EnvironmentPath);
#if defined(_WIN32) || defined(__CYGWIN__)
    paths.AddEnvPrefixPath("PATH", true);
#endif
  }
  // Add PATH
  paths.AddEnvPath("PATH");
  paths.AddSuffixes(this->SearchPathSuffixes);
}

void cmFindBase::FillCMakeSystemVariablePath()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::CMakeSystem];

  std::string var = "CMAKE_SYSTEM_";
  var += this->CMakePathName;
  var += "_PATH";
  paths.AddCMakePrefixPath("CMAKE_SYSTEM_PREFIX_PATH");
  paths.AddCMakePath(var);

  if (this->CMakePathName == "PROGRAM") {
    paths.AddCMakePath("CMAKE_SYSTEM_APPBUNDLE_PATH");
  } else {
    paths.AddCMakePath("CMAKE_SYSTEM_FRAMEWORK_PATH");
  }
  paths.AddSuffixes(this->SearchPathSuffixes);
}

void cmFindBase::FillUserHintsPath()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::Hints];

  for (std::vector<std::string>::const_iterator p =
         this->UserHintsArgs.begin();
       p != this->UserHintsArgs.end(); ++p) {
    paths.AddUserPath(*p);
  }
  paths.AddSuffixes(this->SearchPathSuffixes);
}

void cmFindBase::FillUserGuessPath()
{
  cmSearchPath& paths = this->LabeledPaths[PathLabel::Guess];

  for (std::vector<std::string>::const_iterator p =
         this->UserGuessArgs.begin();
       p != this->UserGuessArgs.end(); ++p) {
    paths.AddUserPath(*p);
  }
  paths.AddSuffixes(this->SearchPathSuffixes);
}

void cmFindBase::PrintFindStuff()
{
  std::cerr << "SearchFrameworkLast: " << this->SearchFrameworkLast << "\n";
  std::cerr << "SearchFrameworkOnly: " << this->SearchFrameworkOnly << "\n";
  std::cerr << "SearchFrameworkFirst: " << this->SearchFrameworkFirst << "\n";
  std::cerr << "SearchAppBundleLast: " << this->SearchAppBundleLast << "\n";
  std::cerr << "SearchAppBundleOnly: " << this->SearchAppBundleOnly << "\n";
  std::cerr << "SearchAppBundleFirst: " << this->SearchAppBundleFirst << "\n";
  std::cerr << "VariableName " << this->VariableName << "\n";
  std::cerr << "VariableDocumentation " << this->VariableDocumentation << "\n";
  std::cerr << "NoDefaultPath " << this->NoDefaultPath << "\n";
  std::cerr << "NoCMakeEnvironmentPath " << this->NoCMakeEnvironmentPath
            << "\n";
  std::cerr << "NoCMakePath " << this->NoCMakePath << "\n";
  std::cerr << "NoSystemEnvironmentPath " << this->NoSystemEnvironmentPath
            << "\n";
  std::cerr << "NoCMakeSystemPath " << this->NoCMakeSystemPath << "\n";
  std::cerr << "EnvironmentPath " << this->EnvironmentPath << "\n";
  std::cerr << "CMakePathName " << this->CMakePathName << "\n";
  std::cerr << "Names  " << cmJoin(this->Names, " ") << "\n";
  std::cerr << "\n";
  std::cerr << "SearchPathSuffixes  ";
  std::cerr << cmJoin(this->SearchPathSuffixes, "\n") << "\n";
  std::cerr << "SearchPaths\n";
  std::cerr << cmWrap("[", this->SearchPaths, "]", "\n") << "\n";
}

bool cmFindBase::CheckForVariableInCache()
{
  if (const char* cacheValue =
        this->Makefile->GetDefinition(this->VariableName)) {
    cmState* state = this->Makefile->GetState();
    const char* cacheEntry = state->GetCacheEntryValue(this->VariableName);
    bool found = !cmSystemTools::IsNOTFOUND(cacheValue);
    bool cached = cacheEntry != CM_NULLPTR;
    if (found) {
      // If the user specifies the entry on the command line without a
      // type we should add the type and docstring but keep the
      // original value.  Tell the subclass implementations to do
      // this.
      if (cached &&
          state->GetCacheEntryType(this->VariableName) ==
            cmStateEnums::UNINITIALIZED) {
        this->AlreadyInCacheWithoutMetaInfo = true;
      }
      return true;
    }
    if (cached) {
      const char* hs =
        state->GetCacheEntryProperty(this->VariableName, "HELPSTRING");
      this->VariableDocumentation = hs ? hs : "(none)";
    }
  }
  return false;
}
