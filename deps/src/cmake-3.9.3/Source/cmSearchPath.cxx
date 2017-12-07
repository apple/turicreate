/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSearchPath.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "cmAlgorithms.h"
#include "cmFindCommon.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

cmSearchPath::cmSearchPath(cmFindCommon* findCmd)
  : FC(findCmd)
{
}

cmSearchPath::~cmSearchPath()
{
}

void cmSearchPath::ExtractWithout(const std::set<std::string>& ignore,
                                  std::vector<std::string>& outPaths,
                                  bool clear) const
{
  if (clear) {
    outPaths.clear();
  }
  for (std::vector<std::string>::const_iterator p = this->Paths.begin();
       p != this->Paths.end(); ++p) {
    if (ignore.count(*p) == 0) {
      outPaths.push_back(*p);
    }
  }
}

void cmSearchPath::AddPath(const std::string& path)
{
  this->AddPathInternal(path);
}

void cmSearchPath::AddUserPath(const std::string& path)
{
  assert(this->FC != CM_NULLPTR);

  std::vector<std::string> outPaths;

  // We should view the registry as the target application would view
  // it.
  cmSystemTools::KeyWOW64 view = cmSystemTools::KeyWOW64_32;
  cmSystemTools::KeyWOW64 other_view = cmSystemTools::KeyWOW64_64;
  if (this->FC->Makefile->PlatformIs64Bit()) {
    view = cmSystemTools::KeyWOW64_64;
    other_view = cmSystemTools::KeyWOW64_32;
  }

  // Expand using the view of the target application.
  std::string expanded = path;
  cmSystemTools::ExpandRegistryValues(expanded, view);
  cmSystemTools::GlobDirs(expanded, outPaths);

  // Executables can be either 32-bit or 64-bit, so expand using the
  // alternative view.
  if (expanded != path && this->FC->CMakePathName == "PROGRAM") {
    expanded = path;
    cmSystemTools::ExpandRegistryValues(expanded, other_view);
    cmSystemTools::GlobDirs(expanded, outPaths);
  }

  // Process them all from the current directory
  for (std::vector<std::string>::const_iterator p = outPaths.begin();
       p != outPaths.end(); ++p) {
    this->AddPathInternal(*p, this->FC->Makefile->GetCurrentSourceDirectory());
  }
}

void cmSearchPath::AddCMakePath(const std::string& variable)
{
  assert(this->FC != CM_NULLPTR);

  // Get a path from a CMake variable.
  if (const char* value = this->FC->Makefile->GetDefinition(variable)) {
    std::vector<std::string> expanded;
    cmSystemTools::ExpandListArgument(value, expanded);

    for (std::vector<std::string>::const_iterator p = expanded.begin();
         p != expanded.end(); ++p) {
      this->AddPathInternal(*p,
                            this->FC->Makefile->GetCurrentSourceDirectory());
    }
  }
}

void cmSearchPath::AddEnvPath(const std::string& variable)
{
  std::vector<std::string> expanded;
  cmSystemTools::GetPath(expanded, variable.c_str());
  for (std::vector<std::string>::const_iterator p = expanded.begin();
       p != expanded.end(); ++p) {
    this->AddPathInternal(*p);
  }
}

void cmSearchPath::AddCMakePrefixPath(const std::string& variable)
{
  assert(this->FC != CM_NULLPTR);

  // Get a path from a CMake variable.
  if (const char* value = this->FC->Makefile->GetDefinition(variable)) {
    std::vector<std::string> expanded;
    cmSystemTools::ExpandListArgument(value, expanded);

    this->AddPrefixPaths(expanded,
                         this->FC->Makefile->GetCurrentSourceDirectory());
  }
}

static std::string cmSearchPathStripBin(std::string const& s)
{
  // If the path is a PREFIX/bin case then add its parent instead.
  if ((cmHasLiteralSuffix(s, "/bin")) || (cmHasLiteralSuffix(s, "/sbin"))) {
    return cmSystemTools::GetFilenamePath(s);
  }
  return s;
}

void cmSearchPath::AddEnvPrefixPath(const std::string& variable, bool stripBin)
{
  std::vector<std::string> expanded;
  cmSystemTools::GetPath(expanded, variable.c_str());
  if (stripBin) {
    std::transform(expanded.begin(), expanded.end(), expanded.begin(),
                   cmSearchPathStripBin);
  }
  this->AddPrefixPaths(expanded);
}

void cmSearchPath::AddSuffixes(const std::vector<std::string>& suffixes)
{
  std::vector<std::string> inPaths;
  inPaths.swap(this->Paths);
  this->Paths.reserve(inPaths.size() * (suffixes.size() + 1));

  for (std::vector<std::string>::iterator ip = inPaths.begin();
       ip != inPaths.end(); ++ip) {
    cmSystemTools::ConvertToUnixSlashes(*ip);

    // if *i is only / then do not add a //
    // this will get incorrectly considered a network
    // path on windows and cause huge delays.
    std::string p = *ip;
    if (!p.empty() && *p.rbegin() != '/') {
      p += "/";
    }

    // Combine with all the suffixes
    for (std::vector<std::string>::const_iterator s = suffixes.begin();
         s != suffixes.end(); ++s) {
      this->Paths.push_back(p + *s);
    }

    // And now the original w/o any suffix
    this->Paths.push_back(*ip);
  }
}

void cmSearchPath::AddPrefixPaths(const std::vector<std::string>& paths,
                                  const char* base)
{
  assert(this->FC != CM_NULLPTR);

  // default for programs
  std::string subdir = "bin";

  if (this->FC->CMakePathName == "INCLUDE") {
    subdir = "include";
  } else if (this->FC->CMakePathName == "LIBRARY") {
    subdir = "lib";
  } else if (this->FC->CMakePathName == "FRAMEWORK") {
    subdir = ""; // ? what to do for frameworks ?
  }

  for (std::vector<std::string>::const_iterator p = paths.begin();
       p != paths.end(); ++p) {
    std::string dir = *p;
    if (!subdir.empty() && !dir.empty() && *dir.rbegin() != '/') {
      dir += "/";
    }
    if (subdir == "include" || subdir == "lib") {
      const char* arch =
        this->FC->Makefile->GetDefinition("CMAKE_LIBRARY_ARCHITECTURE");
      if (arch && *arch) {
        this->AddPathInternal(dir + subdir + "/" + arch, base);
      }
    }
    std::string add = dir + subdir;
    if (add != "/") {
      this->AddPathInternal(add, base);
    }
    if (subdir == "bin") {
      this->AddPathInternal(dir + "sbin", base);
    }
    if (!subdir.empty() && *p != "/") {
      this->AddPathInternal(*p, base);
    }
  }
}

void cmSearchPath::AddPathInternal(const std::string& path, const char* base)
{
  assert(this->FC != CM_NULLPTR);

  std::string collapsed = cmSystemTools::CollapseFullPath(path, base);

  if (collapsed.empty()) {
    return;
  }

  // Insert the path if has not already been emitted.
  if (this->FC->SearchPathsEmitted.insert(collapsed).second) {
    this->Paths.push_back(collapsed);
  }
}
