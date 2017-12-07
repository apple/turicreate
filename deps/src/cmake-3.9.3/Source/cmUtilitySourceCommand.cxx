/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmUtilitySourceCommand.h"

#include <string.h>

#include "cmMakefile.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmUtilitySourceCommand
bool cmUtilitySourceCommand::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  if (args.size() < 3) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  std::vector<std::string>::const_iterator arg = args.begin();

  // The first argument is the cache entry name.
  std::string const& cacheEntry = *arg++;
  const char* cacheValue = this->Makefile->GetDefinition(cacheEntry);
  // If it exists already and appears up to date then we are done.  If
  // the string contains "(IntDir)" but that is not the
  // CMAKE_CFG_INTDIR setting then the value is out of date.
  const char* intDir =
    this->Makefile->GetRequiredDefinition("CMAKE_CFG_INTDIR");

  bool haveCacheValue = false;
  if (this->Makefile->IsOn("CMAKE_CROSSCOMPILING")) {
    haveCacheValue = (cacheValue != CM_NULLPTR);
    if (!haveCacheValue) {
      std::string msg = "UTILITY_SOURCE is used in cross compiling mode for ";
      msg += cacheEntry;
      msg += ". If your intention is to run this executable, you need to "
             "preload the cache with the full path to a version of that "
             "program, which runs on this build machine.";
      cmSystemTools::Message(msg.c_str(), "Warning");
    }
  } else {
    cmState* state = this->Makefile->GetState();
    haveCacheValue =
      (cacheValue && (strstr(cacheValue, "(IntDir)") == CM_NULLPTR ||
                      (intDir && strcmp(intDir, "$(IntDir)") == 0)) &&
       (state->GetCacheMajorVersion() != 0 &&
        state->GetCacheMinorVersion() != 0));
  }

  if (haveCacheValue) {
    return true;
  }

  // The second argument is the utility's executable name, which will be
  // needed later.
  std::string const& utilityName = *arg++;

  // The third argument specifies the relative directory of the source
  // of the utility.
  std::string const& relativeSource = *arg++;
  std::string utilitySource = this->Makefile->GetCurrentSourceDirectory();
  utilitySource = utilitySource + "/" + relativeSource;

  // If the directory doesn't exist, the source has not been included.
  if (!cmSystemTools::FileExists(utilitySource.c_str())) {
    return true;
  }

  // Make sure all the files exist in the source directory.
  while (arg != args.end()) {
    std::string file = utilitySource + "/" + *arg++;
    if (!cmSystemTools::FileExists(file.c_str())) {
      return true;
    }
  }

  // The source exists.
  std::string cmakeCFGout =
    this->Makefile->GetRequiredDefinition("CMAKE_CFG_INTDIR");
  std::string utilityDirectory = this->Makefile->GetCurrentBinaryDirectory();
  std::string exePath;
  if (this->Makefile->GetDefinition("EXECUTABLE_OUTPUT_PATH")) {
    exePath = this->Makefile->GetDefinition("EXECUTABLE_OUTPUT_PATH");
  }
  if (!exePath.empty()) {
    utilityDirectory = exePath;
  } else {
    utilityDirectory += "/" + relativeSource;
  }

  // Construct the cache entry for the executable's location.
  std::string utilityExecutable = utilityDirectory + "/" + cmakeCFGout + "/" +
    utilityName + this->Makefile->GetDefinition("CMAKE_EXECUTABLE_SUFFIX");

  // make sure we remove any /./ in the name
  cmSystemTools::ReplaceString(utilityExecutable, "/./", "/");

  // Enter the value into the cache.
  this->Makefile->AddCacheDefinition(cacheEntry, utilityExecutable.c_str(),
                                     "Path to an internal program.",
                                     cmStateEnums::FILEPATH);
  // add a value into the cache that maps from the
  // full path to the name of the project
  cmSystemTools::ConvertToUnixSlashes(utilityExecutable);
  this->Makefile->AddCacheDefinition(utilityExecutable, utilityName.c_str(),
                                     "Executable to project name.",
                                     cmStateEnums::INTERNAL);

  return true;
}
