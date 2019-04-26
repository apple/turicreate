/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFindProgramCommand.h"

#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

#if defined(__APPLE__)
#  include <CoreFoundation/CoreFoundation.h>
#endif

struct cmFindProgramHelper
{
  cmFindProgramHelper()
  {
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
    // Consider platform-specific extensions.
    this->Extensions.push_back(".com");
    this->Extensions.push_back(".exe");
#endif
    // Consider original name with no extensions.
    this->Extensions.emplace_back();
  }

  // List of valid extensions.
  std::vector<std::string> Extensions;

  // Keep track of the best program file found so far.
  std::string BestPath;

  // Current names under consideration.
  std::vector<std::string> Names;

  // Current name with extension under consideration.
  std::string TestNameExt;

  // Current full path under consideration.
  std::string TestPath;

  void AddName(std::string const& name) { this->Names.push_back(name); }
  void SetName(std::string const& name)
  {
    this->Names.clear();
    this->AddName(name);
  }
  bool CheckCompoundNames()
  {
    for (std::string const& n : this->Names) {
      // Only perform search relative to current directory if the file name
      // contains a directory separator.
      if (n.find('/') != std::string::npos) {
        if (this->CheckDirectoryForName("", n)) {
          return true;
        }
      }
    }
    return false;
  }
  bool CheckDirectory(std::string const& path)
  {
    for (std::string const& n : this->Names) {
      if (this->CheckDirectoryForName(path, n)) {
        return true;
      }
    }
    return false;
  }
  bool CheckDirectoryForName(std::string const& path, std::string const& name)
  {
    for (std::string const& ext : this->Extensions) {
      if (!ext.empty() && cmSystemTools::StringEndsWith(name, ext.c_str())) {
        continue;
      }
      this->TestNameExt = name;
      this->TestNameExt += ext;
      this->TestPath =
        cmSystemTools::CollapseCombinedPath(path, this->TestNameExt);

      if (cmSystemTools::FileExists(this->TestPath, true)) {
        this->BestPath = this->TestPath;
        return true;
      }
    }
    return false;
  }
};

cmFindProgramCommand::cmFindProgramCommand()
{
  this->NamesPerDirAllowed = true;
}

// cmFindProgramCommand
bool cmFindProgramCommand::InitialPass(std::vector<std::string> const& argsIn,
                                       cmExecutionStatus&)
{
  this->VariableDocumentation = "Path to a program.";
  this->CMakePathName = "PROGRAM";
  // call cmFindBase::ParseArguments
  if (!this->ParseArguments(argsIn)) {
    return false;
  }
  if (this->AlreadyInCache) {
    // If the user specifies the entry on the command line without a
    // type we should add the type and docstring but keep the original
    // value.
    if (this->AlreadyInCacheWithoutMetaInfo) {
      this->Makefile->AddCacheDefinition(this->VariableName, "",
                                         this->VariableDocumentation.c_str(),
                                         cmStateEnums::FILEPATH);
    }
    return true;
  }

  std::string const result = FindProgram();
  if (!result.empty()) {
    // Save the value in the cache
    this->Makefile->AddCacheDefinition(this->VariableName, result.c_str(),
                                       this->VariableDocumentation.c_str(),
                                       cmStateEnums::FILEPATH);

    return true;
  }
  this->Makefile->AddCacheDefinition(
    this->VariableName, (this->VariableName + "-NOTFOUND").c_str(),
    this->VariableDocumentation.c_str(), cmStateEnums::FILEPATH);
  return true;
}

std::string cmFindProgramCommand::FindProgram()
{
  std::string program;

  if (this->SearchAppBundleFirst || this->SearchAppBundleOnly) {
    program = FindAppBundle();
  }
  if (program.empty() && !this->SearchAppBundleOnly) {
    program = this->FindNormalProgram();
  }

  if (program.empty() && this->SearchAppBundleLast) {
    program = this->FindAppBundle();
  }
  return program;
}

std::string cmFindProgramCommand::FindNormalProgram()
{
  if (this->NamesPerDir) {
    return this->FindNormalProgramNamesPerDir();
  }
  return this->FindNormalProgramDirsPerName();
}

std::string cmFindProgramCommand::FindNormalProgramNamesPerDir()
{
  // Search for all names in each directory.
  cmFindProgramHelper helper;
  for (std::string const& n : this->Names) {
    helper.AddName(n);
  }

  // Check for the names themselves if they contain a directory separator.
  if (helper.CheckCompoundNames()) {
    return helper.BestPath;
  }

  // Search every directory.
  for (std::string const& sp : this->SearchPaths) {
    if (helper.CheckDirectory(sp)) {
      return helper.BestPath;
    }
  }
  // Couldn't find the program.
  return "";
}

std::string cmFindProgramCommand::FindNormalProgramDirsPerName()
{
  // Search the entire path for each name.
  cmFindProgramHelper helper;
  for (std::string const& n : this->Names) {
    // Switch to searching for this name.
    helper.SetName(n);

    // Check for the names themselves if they contain a directory separator.
    if (helper.CheckCompoundNames()) {
      return helper.BestPath;
    }

    // Search every directory.
    for (std::string const& sp : this->SearchPaths) {
      if (helper.CheckDirectory(sp)) {
        return helper.BestPath;
      }
    }
  }
  // Couldn't find the program.
  return "";
}

std::string cmFindProgramCommand::FindAppBundle()
{
  for (std::string const& name : this->Names) {

    std::string appName = name + std::string(".app");
    std::string appPath =
      cmSystemTools::FindDirectory(appName, this->SearchPaths, true);

    if (!appPath.empty()) {
      std::string executable = GetBundleExecutable(appPath);
      if (!executable.empty()) {
        return cmSystemTools::CollapseFullPath(executable);
      }
    }
  }

  // Couldn't find app bundle
  return "";
}

std::string cmFindProgramCommand::GetBundleExecutable(
  std::string const& bundlePath)
{
  std::string executable;
  (void)bundlePath;
#if defined(__APPLE__)
  // Started with an example on developer.apple.com about finding bundles
  // and modified from that.

  // Get a CFString of the app bundle path
  // XXX - Is it safe to assume everything is in UTF8?
  CFStringRef bundlePathCFS = CFStringCreateWithCString(
    kCFAllocatorDefault, bundlePath.c_str(), kCFStringEncodingUTF8);

  // Make a CFURLRef from the CFString representation of the
  // bundle’s path.
  CFURLRef bundleURL = CFURLCreateWithFileSystemPath(
    kCFAllocatorDefault, bundlePathCFS, kCFURLPOSIXPathStyle, true);

  // Make a bundle instance using the URLRef.
  CFBundleRef appBundle = CFBundleCreate(kCFAllocatorDefault, bundleURL);

  // returned executableURL is relative to <appbundle>/Contents/MacOS/
  CFURLRef executableURL = CFBundleCopyExecutableURL(appBundle);

  if (executableURL != nullptr) {
    const int MAX_OSX_PATH_SIZE = 1024;
    char buffer[MAX_OSX_PATH_SIZE];

    // Convert the CFString to a C string
    CFStringGetCString(CFURLGetString(executableURL), buffer,
                       MAX_OSX_PATH_SIZE, kCFStringEncodingUTF8);

    // And finally to a c++ string
    executable = bundlePath + "/Contents/MacOS/" + std::string(buffer);
    // Only release CFURLRef if it's not null
    CFRelease(executableURL);
  }

  // Any CF objects returned from functions with "create" or
  // "copy" in their names must be released by us!
  CFRelease(bundlePathCFS);
  CFRelease(bundleURL);
  CFRelease(appBundle);
#endif

  return executable;
}
