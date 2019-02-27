/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLoadCacheCommand.h"

#include "cmsys/FStream.hxx"

#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

// cmLoadCacheCommand
bool cmLoadCacheCommand::InitialPass(std::vector<std::string> const& args,
                                     cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with wrong number of arguments.");
  }

  if (args.size() >= 2 && args[1] == "READ_WITH_PREFIX") {
    return this->ReadWithPrefix(args);
  }

  // Cache entries to be excluded from the import list.
  // If this set is empty, all cache entries are brought in
  // and they can not be overridden.
  bool excludeFiles = false;
  std::set<std::string> excludes;

  for (std::string const& arg : args) {
    if (excludeFiles) {
      excludes.insert(arg);
    }
    if (arg == "EXCLUDE") {
      excludeFiles = true;
    }
    if (excludeFiles && (arg == "INCLUDE_INTERNALS")) {
      break;
    }
  }

  // Internal cache entries to be imported.
  // If this set is empty, no internal cache entries are
  // brought in.
  bool includeFiles = false;
  std::set<std::string> includes;

  for (std::string const& arg : args) {
    if (includeFiles) {
      includes.insert(arg);
    }
    if (arg == "INCLUDE_INTERNALS") {
      includeFiles = true;
    }
    if (includeFiles && (arg == "EXCLUDE")) {
      break;
    }
  }

  // Loop over each build directory listed in the arguments.  Each
  // directory has a cache file.
  for (std::string const& arg : args) {
    if ((arg == "EXCLUDE") || (arg == "INCLUDE_INTERNALS")) {
      break;
    }
    this->Makefile->GetCMakeInstance()->LoadCache(arg, false, excludes,
                                                  includes);
  }

  return true;
}

bool cmLoadCacheCommand::ReadWithPrefix(std::vector<std::string> const& args)
{
  // Make sure we have a prefix.
  if (args.size() < 3) {
    this->SetError("READ_WITH_PREFIX form must specify a prefix.");
    return false;
  }

  // Make sure the cache file exists.
  std::string cacheFile = args[0] + "/CMakeCache.txt";
  if (!cmSystemTools::FileExists(cacheFile)) {
    std::string e = "Cannot load cache file from " + cacheFile;
    this->SetError(e);
    return false;
  }

  // Prepare the table of variables to read.
  this->Prefix = args[2];
  this->VariablesToRead.insert(args.begin() + 3, args.end());

  // Read the cache file.
  cmsys::ifstream fin(cacheFile.c_str());

  // This is a big hack read loop to overcome a buggy ifstream
  // implementation on HP-UX.  This should work on all platforms even
  // for small buffer sizes.
  const int bufferSize = 4096;
  char buffer[bufferSize];
  std::string line;
  while (fin) {
    // Read a block of the file.
    fin.read(buffer, bufferSize);
    if (fin.gcount()) {
      // Parse for newlines directly.
      const char* i = buffer;
      const char* end = buffer + fin.gcount();
      while (i != end) {
        const char* begin = i;
        while (i != end && *i != '\n') {
          ++i;
        }
        if (i == begin || *(i - 1) != '\r') {
          // Include this portion of the line.
          line += std::string(begin, i - begin);
        } else {
          // Include this portion of the line.
          // Don't include the \r in a \r\n pair.
          line += std::string(begin, i - 1 - begin);
        }
        if (i != end) {
          // Completed a line.
          this->CheckLine(line.c_str());
          line.clear();

          // Skip the newline character.
          ++i;
        }
      }
    }
  }
  if (!line.empty()) {
    // Partial last line.
    this->CheckLine(line.c_str());
  }

  return true;
}

void cmLoadCacheCommand::CheckLine(const char* line)
{
  // Check one line of the cache file.
  std::string var;
  std::string value;
  cmStateEnums::CacheEntryType type = cmStateEnums::UNINITIALIZED;
  if (cmake::ParseCacheEntry(line, var, value, type)) {
    // Found a real entry.  See if this one was requested.
    if (this->VariablesToRead.find(var) != this->VariablesToRead.end()) {
      // This was requested.  Set this variable locally with the given
      // prefix.
      var = this->Prefix + var;
      if (!value.empty()) {
        this->Makefile->AddDefinition(var, value.c_str());
      } else {
        this->Makefile->RemoveDefinition(var);
      }
    }
  }
}
