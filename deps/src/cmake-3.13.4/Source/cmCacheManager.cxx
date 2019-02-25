/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCacheManager.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>

#include "cmGeneratedFileStream.h"
#include "cmMessenger.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmVersion.h"
#include "cmake.h"

cmCacheManager::cmCacheManager()
{
  this->CacheMajorVersion = 0;
  this->CacheMinorVersion = 0;
}

void cmCacheManager::CleanCMakeFiles(const std::string& path)
{
  std::string glob = path;
  glob += cmake::GetCMakeFilesDirectory();
  glob += "/*.cmake";
  cmsys::Glob globIt;
  globIt.FindFiles(glob);
  std::vector<std::string> files = globIt.GetFiles();
  std::for_each(files.begin(), files.end(), cmSystemTools::RemoveFile);
}

bool cmCacheManager::LoadCache(const std::string& path, bool internal,
                               std::set<std::string>& excludes,
                               std::set<std::string>& includes)
{
  std::string cacheFile = path;
  cacheFile += "/CMakeCache.txt";
  // clear the old cache, if we are reading in internal values
  if (internal) {
    this->Cache.clear();
  }
  if (!cmSystemTools::FileExists(cacheFile)) {
    this->CleanCMakeFiles(path);
    return false;
  }

  cmsys::ifstream fin(cacheFile.c_str());
  if (!fin) {
    return false;
  }
  const char* realbuffer;
  std::string buffer;
  std::string entryKey;
  unsigned int lineno = 0;
  while (fin) {
    // Format is key:type=value
    std::string helpString;
    CacheEntry e;
    cmSystemTools::GetLineFromStream(fin, buffer);
    lineno++;
    realbuffer = buffer.c_str();
    while (*realbuffer != '0' &&
           (*realbuffer == ' ' || *realbuffer == '\t' || *realbuffer == '\r' ||
            *realbuffer == '\n')) {
      if (*realbuffer == '\n') {
        lineno++;
      }
      realbuffer++;
    }
    // skip blank lines and comment lines
    if (realbuffer[0] == '#' || realbuffer[0] == 0) {
      continue;
    }
    while (realbuffer[0] == '/' && realbuffer[1] == '/') {
      if ((realbuffer[2] == '\\') && (realbuffer[3] == 'n')) {
        helpString += "\n";
        helpString += &realbuffer[4];
      } else {
        helpString += &realbuffer[2];
      }
      cmSystemTools::GetLineFromStream(fin, buffer);
      lineno++;
      realbuffer = buffer.c_str();
      if (!fin) {
        continue;
      }
    }
    e.SetProperty("HELPSTRING", helpString.c_str());
    if (cmState::ParseCacheEntry(realbuffer, entryKey, e.Value, e.Type)) {
      if (excludes.find(entryKey) == excludes.end()) {
        // Load internal values if internal is set.
        // If the entry is not internal to the cache being loaded
        // or if it is in the list of internal entries to be
        // imported, load it.
        if (internal || (e.Type != cmStateEnums::INTERNAL) ||
            (includes.find(entryKey) != includes.end())) {
          // If we are loading the cache from another project,
          // make all loaded entries internal so that it is
          // not visible in the gui
          if (!internal) {
            e.Type = cmStateEnums::INTERNAL;
            helpString = "DO NOT EDIT, ";
            helpString += entryKey;
            helpString += " loaded from external file.  "
                          "To change this value edit this file: ";
            helpString += path;
            helpString += "/CMakeCache.txt";
            e.SetProperty("HELPSTRING", helpString.c_str());
          }
          if (!this->ReadPropertyEntry(entryKey, e)) {
            e.Initialized = true;
            this->Cache[entryKey] = e;
          }
        }
      }
    } else {
      std::ostringstream error;
      error << "Parse error in cache file " << cacheFile;
      error << " on line " << lineno << ". Offending entry: " << realbuffer;
      cmSystemTools::Error(error.str().c_str());
    }
  }
  this->CacheMajorVersion = 0;
  this->CacheMinorVersion = 0;
  if (const std::string* cmajor =
        this->GetInitializedCacheValue("CMAKE_CACHE_MAJOR_VERSION")) {
    unsigned int v = 0;
    if (sscanf(cmajor->c_str(), "%u", &v) == 1) {
      this->CacheMajorVersion = v;
    }
    if (const std::string* cminor =
          this->GetInitializedCacheValue("CMAKE_CACHE_MINOR_VERSION")) {
      if (sscanf(cminor->c_str(), "%u", &v) == 1) {
        this->CacheMinorVersion = v;
      }
    }
  } else {
    // CMake version not found in the list file.
    // Set as version 0.0
    this->AddCacheEntry("CMAKE_CACHE_MINOR_VERSION", "0",
                        "Minor version of cmake used to create the "
                        "current loaded cache",
                        cmStateEnums::INTERNAL);
    this->AddCacheEntry("CMAKE_CACHE_MAJOR_VERSION", "0",
                        "Major version of cmake used to create the "
                        "current loaded cache",
                        cmStateEnums::INTERNAL);
  }
  // check to make sure the cache directory has not
  // been moved
  const std::string* oldDir =
    this->GetInitializedCacheValue("CMAKE_CACHEFILE_DIR");
  if (internal && oldDir) {
    std::string currentcwd = path;
    std::string oldcwd = *oldDir;
    cmSystemTools::ConvertToUnixSlashes(currentcwd);
    currentcwd += "/CMakeCache.txt";
    oldcwd += "/CMakeCache.txt";
    if (!cmSystemTools::SameFile(oldcwd, currentcwd)) {
      const std::string* dir =
        this->GetInitializedCacheValue("CMAKE_CACHEFILE_DIR");
      std::ostringstream message;
      message << "The current CMakeCache.txt directory " << currentcwd
              << " is different than the directory " << (dir ? *dir : "")
              << " where CMakeCache.txt was created. This may result "
                 "in binaries being created in the wrong place. If you "
                 "are not sure, reedit the CMakeCache.txt";
      cmSystemTools::Error(message.str().c_str());
    }
  }
  return true;
}

const char* cmCacheManager::PersistentProperties[] = { "ADVANCED", "MODIFIED",
                                                       "STRINGS", nullptr };

bool cmCacheManager::ReadPropertyEntry(std::string const& entryKey,
                                       CacheEntry& e)
{
  // All property entries are internal.
  if (e.Type != cmStateEnums::INTERNAL) {
    return false;
  }

  const char* end = entryKey.c_str() + entryKey.size();
  for (const char** p = this->PersistentProperties; *p; ++p) {
    std::string::size_type plen = strlen(*p) + 1;
    if (entryKey.size() > plen && *(end - plen) == '-' &&
        strcmp(end - plen + 1, *p) == 0) {
      std::string key = entryKey.substr(0, entryKey.size() - plen);
      cmCacheManager::CacheIterator it = this->GetCacheIterator(key.c_str());
      if (it.IsAtEnd()) {
        // Create an entry and store the property.
        CacheEntry& ne = this->Cache[key];
        ne.Type = cmStateEnums::UNINITIALIZED;
        ne.SetProperty(*p, e.Value.c_str());
      } else {
        // Store this property on its entry.
        it.SetProperty(*p, e.Value.c_str());
      }
      return true;
    }
  }
  return false;
}

void cmCacheManager::WritePropertyEntries(std::ostream& os, CacheIterator i,
                                          cmMessenger* messenger)
{
  for (const char** p = this->PersistentProperties; *p; ++p) {
    if (const char* value = i.GetProperty(*p)) {
      std::string helpstring = *p;
      helpstring += " property for variable: ";
      helpstring += i.GetName();
      cmCacheManager::OutputHelpString(os, helpstring);

      std::string key = i.GetName();
      key += "-";
      key += *p;
      this->OutputKey(os, key);
      os << ":INTERNAL=";
      this->OutputValue(os, value);
      os << "\n";
      cmCacheManager::OutputNewlineTruncationWarning(os, key, value,
                                                     messenger);
    }
  }
}

bool cmCacheManager::SaveCache(const std::string& path, cmMessenger* messenger)
{
  std::string cacheFile = path;
  cacheFile += "/CMakeCache.txt";
  cmGeneratedFileStream fout(cacheFile);
  fout.SetCopyIfDifferent(true);
  if (!fout) {
    cmSystemTools::Error("Unable to open cache file for save. ",
                         cacheFile.c_str());
    cmSystemTools::ReportLastSystemError("");
    return false;
  }
  // before writing the cache, update the version numbers
  // to the
  this->AddCacheEntry("CMAKE_CACHE_MAJOR_VERSION",
                      std::to_string(cmVersion::GetMajorVersion()).c_str(),
                      "Major version of cmake used to create the "
                      "current loaded cache",
                      cmStateEnums::INTERNAL);
  this->AddCacheEntry("CMAKE_CACHE_MINOR_VERSION",
                      std::to_string(cmVersion::GetMinorVersion()).c_str(),
                      "Minor version of cmake used to create the "
                      "current loaded cache",
                      cmStateEnums::INTERNAL);
  this->AddCacheEntry("CMAKE_CACHE_PATCH_VERSION",
                      std::to_string(cmVersion::GetPatchVersion()).c_str(),
                      "Patch version of cmake used to create the "
                      "current loaded cache",
                      cmStateEnums::INTERNAL);

  // Let us store the current working directory so that if somebody
  // Copies it, he will not be surprised
  std::string currentcwd = path;
  if (currentcwd[0] >= 'A' && currentcwd[0] <= 'Z' && currentcwd[1] == ':') {
    // Cast added to avoid compiler warning. Cast is ok because
    // value is guaranteed to fit in char by the above if...
    currentcwd[0] = static_cast<char>(currentcwd[0] - 'A' + 'a');
  }
  cmSystemTools::ConvertToUnixSlashes(currentcwd);
  this->AddCacheEntry("CMAKE_CACHEFILE_DIR", currentcwd.c_str(),
                      "This is the directory where this CMakeCache.txt"
                      " was created",
                      cmStateEnums::INTERNAL);

  /* clang-format off */
  fout << "# This is the CMakeCache file.\n"
       << "# For build in directory: " << currentcwd << "\n"
       << "# It was generated by CMake: "
       << cmSystemTools::GetCMakeCommand() << std::endl;
  /* clang-format on */

  /* clang-format off */
  fout << "# You can edit this file to change values found and used by cmake."
       << std::endl
       << "# If you do not want to change any of the values, simply exit the "
       "editor." << std::endl
       << "# If you do want to change a value, simply edit, save, and exit "
       "the editor." << std::endl
       << "# The syntax for the file is as follows:\n"
       << "# KEY:TYPE=VALUE\n"
       << "# KEY is the name of a variable in the cache.\n"
       << "# TYPE is a hint to GUIs for the type of VALUE, DO NOT EDIT "
       "TYPE!." << std::endl
       << "# VALUE is the current value for the KEY.\n\n";
  /* clang-format on */

  fout << "########################\n";
  fout << "# EXTERNAL cache entries\n";
  fout << "########################\n";
  fout << "\n";

  for (auto const& i : this->Cache) {
    CacheEntry const& ce = i.second;
    cmStateEnums::CacheEntryType t = ce.Type;
    if (!ce.Initialized) {
      /*
        // This should be added in, but is not for now.
      cmSystemTools::Error("Cache entry \"", (*i).first.c_str(),
                           "\" is uninitialized");
      */
    } else if (t != cmStateEnums::INTERNAL) {
      // Format is key:type=value
      if (const char* help = ce.GetProperty("HELPSTRING")) {
        cmCacheManager::OutputHelpString(fout, help);
      } else {
        cmCacheManager::OutputHelpString(fout, "Missing description");
      }
      this->OutputKey(fout, i.first);
      fout << ":" << cmState::CacheEntryTypeToString(t) << "=";
      this->OutputValue(fout, ce.Value);
      fout << "\n";
      cmCacheManager::OutputNewlineTruncationWarning(fout, i.first, ce.Value,
                                                     messenger);
      fout << "\n";
    }
  }

  fout << "\n";
  fout << "########################\n";
  fout << "# INTERNAL cache entries\n";
  fout << "########################\n";
  fout << "\n";

  for (cmCacheManager::CacheIterator i = this->NewIterator(); !i.IsAtEnd();
       i.Next()) {
    if (!i.Initialized()) {
      continue;
    }

    cmStateEnums::CacheEntryType t = i.GetType();
    this->WritePropertyEntries(fout, i, messenger);
    if (t == cmStateEnums::INTERNAL) {
      // Format is key:type=value
      if (const char* help = i.GetProperty("HELPSTRING")) {
        this->OutputHelpString(fout, help);
      }
      this->OutputKey(fout, i.GetName());
      fout << ":" << cmState::CacheEntryTypeToString(t) << "=";
      this->OutputValue(fout, i.GetValue());
      fout << "\n";
      cmCacheManager::OutputNewlineTruncationWarning(fout, i.GetName(),
                                                     i.GetValue(), messenger);
    }
  }
  fout << "\n";
  fout.Close();
  std::string checkCacheFile = path;
  checkCacheFile += cmake::GetCMakeFilesDirectory();
  cmSystemTools::MakeDirectory(checkCacheFile);
  checkCacheFile += "/cmake.check_cache";
  cmsys::ofstream checkCache(checkCacheFile.c_str());
  if (!checkCache) {
    cmSystemTools::Error("Unable to open check cache file for write. ",
                         checkCacheFile.c_str());
    return false;
  }
  checkCache << "# This file is generated by cmake for dependency checking "
                "of the CMakeCache.txt file\n";
  return true;
}

bool cmCacheManager::DeleteCache(const std::string& path)
{
  std::string cacheFile = path;
  cmSystemTools::ConvertToUnixSlashes(cacheFile);
  std::string cmakeFiles = cacheFile;
  cacheFile += "/CMakeCache.txt";
  if (cmSystemTools::FileExists(cacheFile)) {
    cmSystemTools::RemoveFile(cacheFile);
    // now remove the files in the CMakeFiles directory
    // this cleans up language cache files
    cmakeFiles += cmake::GetCMakeFilesDirectory();
    if (cmSystemTools::FileIsDirectory(cmakeFiles)) {
      cmSystemTools::RemoveADirectory(cmakeFiles);
    }
  }
  return true;
}

void cmCacheManager::OutputKey(std::ostream& fout, std::string const& key)
{
  // support : in key name by double quoting
  const char* q =
    (key.find(':') != std::string::npos || key.find("//") == 0) ? "\"" : "";
  fout << q << key << q;
}

void cmCacheManager::OutputValue(std::ostream& fout, std::string const& value)
{
  // look for and truncate newlines
  std::string::size_type newline = value.find('\n');
  if (newline != std::string::npos) {
    std::string truncated = value.substr(0, newline);
    OutputValueNoNewlines(fout, truncated);
  } else {
    OutputValueNoNewlines(fout, value);
  }
}

void cmCacheManager::OutputValueNoNewlines(std::ostream& fout,
                                           std::string const& value)
{
  // if value has trailing space or tab, enclose it in single quotes
  if (!value.empty() &&
      (value[value.size() - 1] == ' ' || value[value.size() - 1] == '\t')) {
    fout << '\'' << value << '\'';
  } else {
    fout << value;
  }
}

void cmCacheManager::OutputHelpString(std::ostream& fout,
                                      const std::string& helpString)
{
  std::string::size_type end = helpString.size();
  if (end == 0) {
    return;
  }
  std::string oneLine;
  std::string::size_type pos = 0;
  for (std::string::size_type i = 0; i <= end; i++) {
    if ((i == end) || (helpString[i] == '\n') ||
        ((i - pos >= 60) && (helpString[i] == ' '))) {
      fout << "//";
      if (helpString[pos] == '\n') {
        pos++;
        fout << "\\n";
      }
      oneLine = helpString.substr(pos, i - pos);
      fout << oneLine << "\n";
      pos = i;
    }
  }
}

void cmCacheManager::OutputWarningComment(std::ostream& fout,
                                          std::string const& message,
                                          bool wrapSpaces)
{
  std::string::size_type end = message.size();
  std::string oneLine;
  std::string::size_type pos = 0;
  for (std::string::size_type i = 0; i <= end; i++) {
    if ((i == end) || (message[i] == '\n') ||
        ((i - pos >= 60) && (message[i] == ' ') && wrapSpaces)) {
      fout << "# ";
      if (message[pos] == '\n') {
        pos++;
        fout << "\\n";
      }
      oneLine = message.substr(pos, i - pos);
      fout << oneLine << "\n";
      pos = i;
    }
  }
}

void cmCacheManager::OutputNewlineTruncationWarning(std::ostream& fout,
                                                    std::string const& key,
                                                    std::string const& value,
                                                    cmMessenger* messenger)
{
  if (value.find('\n') != std::string::npos) {
    if (messenger) {
      std::string message = "Value of ";
      message += key;
      message += " contained a newline; truncating";
      messenger->IssueMessage(cmake::WARNING, message);
    }

    std::string comment = "WARNING: Value of ";
    comment += key;
    comment += " contained a newline and was truncated. Original value:";

    OutputWarningComment(fout, comment, true);
    OutputWarningComment(fout, value, false);
  }
}

void cmCacheManager::RemoveCacheEntry(const std::string& key)
{
  CacheEntryMap::iterator i = this->Cache.find(key);
  if (i != this->Cache.end()) {
    this->Cache.erase(i);
  }
}

cmCacheManager::CacheEntry* cmCacheManager::GetCacheEntry(
  const std::string& key)
{
  CacheEntryMap::iterator i = this->Cache.find(key);
  if (i != this->Cache.end()) {
    return &i->second;
  }
  return nullptr;
}

cmCacheManager::CacheIterator cmCacheManager::GetCacheIterator(const char* key)
{
  return CacheIterator(*this, key);
}

const std::string* cmCacheManager::GetInitializedCacheValue(
  const std::string& key) const
{
  CacheEntryMap::const_iterator i = this->Cache.find(key);
  if (i != this->Cache.end() && i->second.Initialized) {
    return &i->second.Value;
  }
  return nullptr;
}

void cmCacheManager::PrintCache(std::ostream& out) const
{
  out << "=================================================" << std::endl;
  out << "CMakeCache Contents:" << std::endl;
  for (auto const& i : this->Cache) {
    if (i.second.Type != cmStateEnums::INTERNAL) {
      out << i.first << " = " << i.second.Value << std::endl;
    }
  }
  out << "\n\n";
  out << "To change values in the CMakeCache, " << std::endl
      << "edit CMakeCache.txt in your output directory.\n";
  out << "=================================================" << std::endl;
}

void cmCacheManager::AddCacheEntry(const std::string& key, const char* value,
                                   const char* helpString,
                                   cmStateEnums::CacheEntryType type)
{
  CacheEntry& e = this->Cache[key];
  if (value) {
    e.Value = value;
    e.Initialized = true;
  } else {
    e.Value.clear();
  }
  e.Type = type;
  // make sure we only use unix style paths
  if (type == cmStateEnums::FILEPATH || type == cmStateEnums::PATH) {
    if (e.Value.find(';') != std::string::npos) {
      std::vector<std::string> paths;
      cmSystemTools::ExpandListArgument(e.Value, paths);
      const char* sep = "";
      e.Value = "";
      for (std::string& i : paths) {
        cmSystemTools::ConvertToUnixSlashes(i);
        e.Value += sep;
        e.Value += i;
        sep = ";";
      }
    } else {
      cmSystemTools::ConvertToUnixSlashes(e.Value);
    }
  }
  e.SetProperty("HELPSTRING",
                helpString
                  ? helpString
                  : "(This variable does not exist and should not be used)");
}

bool cmCacheManager::CacheIterator::IsAtEnd() const
{
  return this->Position == this->Container.Cache.end();
}

void cmCacheManager::CacheIterator::Begin()
{
  this->Position = this->Container.Cache.begin();
}

bool cmCacheManager::CacheIterator::Find(const std::string& key)
{
  this->Position = this->Container.Cache.find(key);
  return !this->IsAtEnd();
}

void cmCacheManager::CacheIterator::Next()
{
  if (!this->IsAtEnd()) {
    ++this->Position;
  }
}

std::vector<std::string> cmCacheManager::CacheIterator::GetPropertyList() const
{
  return this->GetEntry().GetPropertyList();
}

void cmCacheManager::CacheIterator::SetValue(const char* value)
{
  if (this->IsAtEnd()) {
    return;
  }
  CacheEntry* entry = &this->GetEntry();
  if (value) {
    entry->Value = value;
    entry->Initialized = true;
  } else {
    entry->Value.clear();
  }
}

bool cmCacheManager::CacheIterator::GetValueAsBool() const
{
  return cmSystemTools::IsOn(this->GetEntry().Value);
}

std::vector<std::string> cmCacheManager::CacheEntry::GetPropertyList() const
{
  return this->Properties.GetPropertyList();
}

const char* cmCacheManager::CacheEntry::GetProperty(
  const std::string& prop) const
{
  if (prop == "TYPE") {
    return cmState::CacheEntryTypeToString(this->Type);
  }
  if (prop == "VALUE") {
    return this->Value.c_str();
  }
  return this->Properties.GetPropertyValue(prop);
}

void cmCacheManager::CacheEntry::SetProperty(const std::string& prop,
                                             const char* value)
{
  if (prop == "TYPE") {
    this->Type = cmState::StringToCacheEntryType(value ? value : "STRING");
  } else if (prop == "VALUE") {
    this->Value = value ? value : "";
  } else {
    this->Properties.SetProperty(prop, value);
  }
}

void cmCacheManager::CacheEntry::AppendProperty(const std::string& prop,
                                                const char* value,
                                                bool asString)
{
  if (prop == "TYPE") {
    this->Type = cmState::StringToCacheEntryType(value ? value : "STRING");
  } else if (prop == "VALUE") {
    if (value) {
      if (!this->Value.empty() && *value && !asString) {
        this->Value += ";";
      }
      this->Value += value;
    }
  } else {
    this->Properties.AppendProperty(prop, value, asString);
  }
}

const char* cmCacheManager::CacheIterator::GetProperty(
  const std::string& prop) const
{
  if (!this->IsAtEnd()) {
    return this->GetEntry().GetProperty(prop);
  }
  return nullptr;
}

void cmCacheManager::CacheIterator::SetProperty(const std::string& p,
                                                const char* v)
{
  if (!this->IsAtEnd()) {
    this->GetEntry().SetProperty(p, v);
  }
}

void cmCacheManager::CacheIterator::AppendProperty(const std::string& p,
                                                   const char* v,
                                                   bool asString)
{
  if (!this->IsAtEnd()) {
    this->GetEntry().AppendProperty(p, v, asString);
  }
}

bool cmCacheManager::CacheIterator::GetPropertyAsBool(
  const std::string& prop) const
{
  if (const char* value = this->GetProperty(prop)) {
    return cmSystemTools::IsOn(value);
  }
  return false;
}

void cmCacheManager::CacheIterator::SetProperty(const std::string& p, bool v)
{
  this->SetProperty(p, v ? "ON" : "OFF");
}

bool cmCacheManager::CacheIterator::PropertyExists(
  const std::string& prop) const
{
  return this->GetProperty(prop) != nullptr;
}
