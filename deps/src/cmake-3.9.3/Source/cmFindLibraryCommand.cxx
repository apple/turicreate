/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFindLibraryCommand.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <set>
#include <stdio.h>
#include <string.h>

#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

cmFindLibraryCommand::cmFindLibraryCommand()
{
  this->EnvironmentPath = "LIB";
  this->NamesPerDirAllowed = true;
}

// cmFindLibraryCommand
bool cmFindLibraryCommand::InitialPass(std::vector<std::string> const& argsIn,
                                       cmExecutionStatus&)
{
  this->VariableDocumentation = "Path to a library.";
  this->CMakePathName = "LIBRARY";
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

  // add custom lib<qual> paths instead of using fixed lib32, lib64 or
  // libx32
  if (const char* customLib = this->Makefile->GetDefinition(
        "CMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX")) {
    this->AddArchitecturePaths(customLib);
  }
  // add special 32 bit paths if this is a 32 bit compile.
  else if (this->Makefile->PlatformIs32Bit() &&
           this->Makefile->GetState()->GetGlobalPropertyAsBool(
             "FIND_LIBRARY_USE_LIB32_PATHS")) {
    this->AddArchitecturePaths("32");
  }
  // add special 64 bit paths if this is a 64 bit compile.
  else if (this->Makefile->PlatformIs64Bit() &&
           this->Makefile->GetState()->GetGlobalPropertyAsBool(
             "FIND_LIBRARY_USE_LIB64_PATHS")) {
    this->AddArchitecturePaths("64");
  }
  // add special 32 bit paths if this is an x32 compile.
  else if (this->Makefile->PlatformIsx32() &&
           this->Makefile->GetState()->GetGlobalPropertyAsBool(
             "FIND_LIBRARY_USE_LIBX32_PATHS")) {
    this->AddArchitecturePaths("x32");
  }

  std::string library = this->FindLibrary();
  if (library != "") {
    // Save the value in the cache
    this->Makefile->AddCacheDefinition(this->VariableName, library.c_str(),
                                       this->VariableDocumentation.c_str(),
                                       cmStateEnums::FILEPATH);
    return true;
  }
  std::string notfound = this->VariableName + "-NOTFOUND";
  this->Makefile->AddCacheDefinition(this->VariableName, notfound.c_str(),
                                     this->VariableDocumentation.c_str(),
                                     cmStateEnums::FILEPATH);
  return true;
}

void cmFindLibraryCommand::AddArchitecturePaths(const char* suffix)
{
  std::vector<std::string> original;
  original.swap(this->SearchPaths);
  for (std::vector<std::string>::const_iterator i = original.begin();
       i != original.end(); ++i) {
    this->AddArchitecturePath(*i, 0, suffix);
  }
}

static bool cmLibDirsLinked(std::string const& l, std::string const& r)
{
  // Compare the real paths of the two directories.
  // Since our caller only changed the trailing component of each
  // directory, the real paths can be the same only if at least one of
  // the trailing components is a symlink.  Use this as an optimization
  // to avoid excessive realpath calls.
  return (cmSystemTools::FileIsSymlink(l) ||
          cmSystemTools::FileIsSymlink(r)) &&
    cmSystemTools::GetRealPath(l) == cmSystemTools::GetRealPath(r);
}

void cmFindLibraryCommand::AddArchitecturePath(
  std::string const& dir, std::string::size_type start_pos, const char* suffix,
  bool fresh)
{
  std::string::size_type pos = dir.find("lib/", start_pos);

  if (pos != std::string::npos) {
    // Check for "lib".
    std::string lib = dir.substr(0, pos + 3);
    bool use_lib = cmSystemTools::FileIsDirectory(lib);

    // Check for "lib<suffix>" and use it first.
    std::string libX = lib + suffix;
    bool use_libX = cmSystemTools::FileIsDirectory(libX);

    // Avoid copies of the same directory due to symlinks.
    if (use_libX && use_lib && cmLibDirsLinked(libX, lib)) {
      use_libX = false;
    }

    if (use_libX) {
      libX += dir.substr(pos + 3);
      std::string::size_type libX_pos = pos + 3 + strlen(suffix) + 1;
      this->AddArchitecturePath(libX, libX_pos, suffix);
    }

    if (use_lib) {
      this->AddArchitecturePath(dir, pos + 3 + 1, suffix, false);
    }
  }

  if (fresh) {
    // Check for the original unchanged path.
    bool use_dir = cmSystemTools::FileIsDirectory(dir);

    // Check for <dir><suffix>/ and use it first.
    std::string dirX = dir + suffix;
    bool use_dirX = cmSystemTools::FileIsDirectory(dirX);

    // Avoid copies of the same directory due to symlinks.
    if (use_dirX && use_dir && cmLibDirsLinked(dirX, dir)) {
      use_dirX = false;
    }

    if (use_dirX) {
      dirX += "/";
      this->SearchPaths.push_back(dirX);
    }

    if (use_dir) {
      this->SearchPaths.push_back(dir);
    }
  }
}

std::string cmFindLibraryCommand::FindLibrary()
{
  std::string library;
  if (this->SearchFrameworkFirst || this->SearchFrameworkOnly) {
    library = this->FindFrameworkLibrary();
  }
  if (library.empty() && !this->SearchFrameworkOnly) {
    library = this->FindNormalLibrary();
  }
  if (library.empty() && this->SearchFrameworkLast) {
    library = this->FindFrameworkLibrary();
  }
  return library;
}

struct cmFindLibraryHelper
{
  cmFindLibraryHelper(cmMakefile* mf);

  // Context information.
  cmMakefile* Makefile;
  cmGlobalGenerator* GG;

  // List of valid prefixes and suffixes.
  std::vector<std::string> Prefixes;
  std::vector<std::string> Suffixes;
  std::string PrefixRegexStr;
  std::string SuffixRegexStr;

  // Keep track of the best library file found so far.
  typedef std::vector<std::string>::size_type size_type;
  std::string BestPath;

  // Support for OpenBSD shared library naming: lib<name>.so.<major>.<minor>
  bool OpenBSD;

  // Current names under consideration.
  struct Name
  {
    bool TryRaw;
    std::string Raw;
    cmsys::RegularExpression Regex;
    Name()
      : TryRaw(false)
    {
    }
  };
  std::vector<Name> Names;

  // Current full path under consideration.
  std::string TestPath;

  void RegexFromLiteral(std::string& out, std::string const& in);
  void RegexFromList(std::string& out, std::vector<std::string> const& in);
  size_type GetPrefixIndex(std::string const& prefix)
  {
    return std::find(this->Prefixes.begin(), this->Prefixes.end(), prefix) -
      this->Prefixes.begin();
  }
  size_type GetSuffixIndex(std::string const& suffix)
  {
    return std::find(this->Suffixes.begin(), this->Suffixes.end(), suffix) -
      this->Suffixes.begin();
  }
  bool HasValidSuffix(std::string const& name);
  void AddName(std::string const& name);
  void SetName(std::string const& name);
  bool CheckDirectory(std::string const& path);
  bool CheckDirectoryForName(std::string const& path, Name& name);
};

cmFindLibraryHelper::cmFindLibraryHelper(cmMakefile* mf)
  : Makefile(mf)
{
  this->GG = this->Makefile->GetGlobalGenerator();

  // Collect the list of library name prefixes/suffixes to try.
  const char* prefixes_list =
    this->Makefile->GetRequiredDefinition("CMAKE_FIND_LIBRARY_PREFIXES");
  const char* suffixes_list =
    this->Makefile->GetRequiredDefinition("CMAKE_FIND_LIBRARY_SUFFIXES");
  cmSystemTools::ExpandListArgument(prefixes_list, this->Prefixes, true);
  cmSystemTools::ExpandListArgument(suffixes_list, this->Suffixes, true);
  this->RegexFromList(this->PrefixRegexStr, this->Prefixes);
  this->RegexFromList(this->SuffixRegexStr, this->Suffixes);

  // Check whether to use OpenBSD-style library version comparisons.
  this->OpenBSD = this->Makefile->GetState()->GetGlobalPropertyAsBool(
    "FIND_LIBRARY_USE_OPENBSD_VERSIONING");
}

void cmFindLibraryHelper::RegexFromLiteral(std::string& out,
                                           std::string const& in)
{
  for (std::string::const_iterator ci = in.begin(); ci != in.end(); ++ci) {
    char ch = *ci;
    if (ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '\\' ||
        ch == '.' || ch == '*' || ch == '+' || ch == '?' || ch == '-' ||
        ch == '^' || ch == '$') {
      out += "\\";
    }
#if defined(_WIN32) || defined(__APPLE__)
    out += tolower(ch);
#else
    out += ch;
#endif
  }
}

void cmFindLibraryHelper::RegexFromList(std::string& out,
                                        std::vector<std::string> const& in)
{
  // Surround the list in parens so the '|' does not apply to anything
  // else and the result can be checked after matching.
  out += "(";
  const char* sep = "";
  for (std::vector<std::string>::const_iterator si = in.begin();
       si != in.end(); ++si) {
    // Separate from previous item.
    out += sep;
    sep = "|";

    // Append this item.
    this->RegexFromLiteral(out, *si);
  }
  out += ")";
}

bool cmFindLibraryHelper::HasValidSuffix(std::string const& name)
{
  for (std::vector<std::string>::const_iterator si = this->Suffixes.begin();
       si != this->Suffixes.end(); ++si) {
    std::string suffix = *si;
    if (name.length() <= suffix.length()) {
      continue;
    }
    // Check if the given name ends in a valid library suffix.
    if (name.substr(name.size() - suffix.length()) == suffix) {
      return true;
    }
    // Check if a valid library suffix is somewhere in the name,
    // this may happen e.g. for versioned shared libraries: libfoo.so.2
    suffix += ".";
    if (name.find(suffix) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void cmFindLibraryHelper::AddName(std::string const& name)
{
  Name entry;

  // Consider checking the raw name too.
  entry.TryRaw = this->HasValidSuffix(name);
  entry.Raw = name;

  // Build a regular expression to match library names.
  std::string regex = "^";
  regex += this->PrefixRegexStr;
  this->RegexFromLiteral(regex, name);
  regex += this->SuffixRegexStr;
  if (this->OpenBSD) {
    regex += "(\\.[0-9]+\\.[0-9]+)?";
  }
  regex += "$";
  entry.Regex.compile(regex.c_str());
  this->Names.push_back(entry);
}

void cmFindLibraryHelper::SetName(std::string const& name)
{
  this->Names.clear();
  this->AddName(name);
}

bool cmFindLibraryHelper::CheckDirectory(std::string const& path)
{
  for (std::vector<Name>::iterator i = this->Names.begin();
       i != this->Names.end(); ++i) {
    if (this->CheckDirectoryForName(path, *i)) {
      return true;
    }
  }
  return false;
}

bool cmFindLibraryHelper::CheckDirectoryForName(std::string const& path,
                                                Name& name)
{
  // If the original library name provided by the user matches one of
  // the suffixes, try it first.  This allows users to search
  // specifically for a static library on some platforms (on MS tools
  // one cannot tell just from the library name whether it is a static
  // library or an import library).
  if (name.TryRaw) {
    this->TestPath = path;
    this->TestPath += name.Raw;
    if (cmSystemTools::FileExists(this->TestPath.c_str(), true)) {
      this->BestPath = cmSystemTools::CollapseFullPath(this->TestPath);
      cmSystemTools::ConvertToUnixSlashes(this->BestPath);
      return true;
    }
  }

  // No library file has yet been found.
  size_type bestPrefix = this->Prefixes.size();
  size_type bestSuffix = this->Suffixes.size();
  unsigned int bestMajor = 0;
  unsigned int bestMinor = 0;

  // Search for a file matching the library name regex.
  std::string dir = path;
  cmSystemTools::ConvertToUnixSlashes(dir);
  std::set<std::string> const& files = this->GG->GetDirectoryContent(dir);
  for (std::set<std::string>::const_iterator fi = files.begin();
       fi != files.end(); ++fi) {
    std::string const& origName = *fi;
#if defined(_WIN32) || defined(__APPLE__)
    std::string testName = cmSystemTools::LowerCase(origName);
#else
    std::string const& testName = origName;
#endif
    if (name.Regex.find(testName)) {
      this->TestPath = path;
      this->TestPath += origName;
      if (!cmSystemTools::FileIsDirectory(this->TestPath)) {
        // This is a matching file.  Check if it is better than the
        // best name found so far.  Earlier prefixes are preferred,
        // followed by earlier suffixes.  For OpenBSD, shared library
        // version extensions are compared.
        size_type prefix = this->GetPrefixIndex(name.Regex.match(1));
        size_type suffix = this->GetSuffixIndex(name.Regex.match(2));
        unsigned int major = 0;
        unsigned int minor = 0;
        if (this->OpenBSD) {
          sscanf(name.Regex.match(3).c_str(), ".%u.%u", &major, &minor);
        }
        if (this->BestPath.empty() || prefix < bestPrefix ||
            (prefix == bestPrefix && suffix < bestSuffix) ||
            (prefix == bestPrefix && suffix == bestSuffix &&
             (major > bestMajor ||
              (major == bestMajor && minor > bestMinor)))) {
          this->BestPath = this->TestPath;
          bestPrefix = prefix;
          bestSuffix = suffix;
          bestMajor = major;
          bestMinor = minor;
        }
      }
    }
  }

  // Use the best candidate found in this directory, if any.
  return !this->BestPath.empty();
}

std::string cmFindLibraryCommand::FindNormalLibrary()
{
  if (this->NamesPerDir) {
    return this->FindNormalLibraryNamesPerDir();
  }
  return this->FindNormalLibraryDirsPerName();
}

std::string cmFindLibraryCommand::FindNormalLibraryNamesPerDir()
{
  // Search for all names in each directory.
  cmFindLibraryHelper helper(this->Makefile);
  for (std::vector<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    helper.AddName(*ni);
  }
  // Search every directory.
  for (std::vector<std::string>::const_iterator p = this->SearchPaths.begin();
       p != this->SearchPaths.end(); ++p) {
    if (helper.CheckDirectory(*p)) {
      return helper.BestPath;
    }
  }
  // Couldn't find the library.
  return "";
}

std::string cmFindLibraryCommand::FindNormalLibraryDirsPerName()
{
  // Search the entire path for each name.
  cmFindLibraryHelper helper(this->Makefile);
  for (std::vector<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    // Switch to searching for this name.
    helper.SetName(*ni);

    // Search every directory.
    for (std::vector<std::string>::const_iterator p =
           this->SearchPaths.begin();
         p != this->SearchPaths.end(); ++p) {
      if (helper.CheckDirectory(*p)) {
        return helper.BestPath;
      }
    }
  }
  // Couldn't find the library.
  return "";
}

std::string cmFindLibraryCommand::FindFrameworkLibrary()
{
  if (this->NamesPerDir) {
    return this->FindFrameworkLibraryNamesPerDir();
  }
  return this->FindFrameworkLibraryDirsPerName();
}

std::string cmFindLibraryCommand::FindFrameworkLibraryNamesPerDir()
{
  std::string fwPath;
  // Search for all names in each search path.
  for (std::vector<std::string>::const_iterator di = this->SearchPaths.begin();
       di != this->SearchPaths.end(); ++di) {
    for (std::vector<std::string>::const_iterator ni = this->Names.begin();
         ni != this->Names.end(); ++ni) {
      fwPath = *di;
      fwPath += *ni;
      fwPath += ".framework";
      if (cmSystemTools::FileIsDirectory(fwPath)) {
        return cmSystemTools::CollapseFullPath(fwPath);
      }
    }
  }

  // No framework found.
  return "";
}

std::string cmFindLibraryCommand::FindFrameworkLibraryDirsPerName()
{
  std::string fwPath;
  // Search for each name in all search paths.
  for (std::vector<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    for (std::vector<std::string>::const_iterator di =
           this->SearchPaths.begin();
         di != this->SearchPaths.end(); ++di) {
      fwPath = *di;
      fwPath += *ni;
      fwPath += ".framework";
      if (cmSystemTools::FileIsDirectory(fwPath)) {
        return cmSystemTools::CollapseFullPath(fwPath);
      }
    }
  }

  // No framework found.
  return "";
}
