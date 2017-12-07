/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCMakeMinimumRequired.h"

#include <sstream>
#include <stdio.h>

#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmVersion.h"
#include "cmake.h"

class cmExecutionStatus;

// cmCMakeMinimumRequired
bool cmCMakeMinimumRequired::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  // Process arguments.
  std::string version_string;
  bool doing_version = false;
  for (unsigned int i = 0; i < args.size(); ++i) {
    if (args[i] == "VERSION") {
      doing_version = true;
    } else if (args[i] == "FATAL_ERROR") {
      if (doing_version) {
        this->SetError("called with no value for VERSION.");
        return false;
      }
      doing_version = false;
    } else if (doing_version) {
      doing_version = false;
      version_string = args[i];
    } else {
      this->UnknownArguments.push_back(args[i]);
    }
  }
  if (doing_version) {
    this->SetError("called with no value for VERSION.");
    return false;
  }

  // Make sure there was a version to check.
  if (version_string.empty()) {
    return this->EnforceUnknownArguments();
  }

  // Save the required version string.
  this->Makefile->AddDefinition("CMAKE_MINIMUM_REQUIRED_VERSION",
                                version_string.c_str());

  // Get the current version number.
  unsigned int current_major = cmVersion::GetMajorVersion();
  unsigned int current_minor = cmVersion::GetMinorVersion();
  unsigned int current_patch = cmVersion::GetPatchVersion();
  unsigned int current_tweak = cmVersion::GetTweakVersion();

  // Parse at least two components of the version number.
  // Use zero for those not specified.
  unsigned int required_major = 0;
  unsigned int required_minor = 0;
  unsigned int required_patch = 0;
  unsigned int required_tweak = 0;
  if (sscanf(version_string.c_str(), "%u.%u.%u.%u", &required_major,
             &required_minor, &required_patch, &required_tweak) < 2) {
    std::ostringstream e;
    e << "could not parse VERSION \"" << version_string << "\".";
    this->SetError(e.str());
    return false;
  }

  // Compare the version numbers.
  if ((current_major < required_major) ||
      (current_major == required_major && current_minor < required_minor) ||
      (current_major == required_major && current_minor == required_minor &&
       current_patch < required_patch) ||
      (current_major == required_major && current_minor == required_minor &&
       current_patch == required_patch && current_tweak < required_tweak)) {
    // The current version is too low.
    std::ostringstream e;
    e << "CMake " << version_string
      << " or higher is required.  You are running version "
      << cmVersion::GetCMakeVersion();
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    cmSystemTools::SetFatalErrorOccured();
    return true;
  }

  // The version is not from the future, so enforce unknown arguments.
  if (!this->EnforceUnknownArguments()) {
    return false;
  }

  if (required_major < 2 || (required_major == 2 && required_minor < 4)) {
    this->Makefile->IssueMessage(
      cmake::AUTHOR_WARNING,
      "Compatibility with CMake < 2.4 is not supported by CMake >= 3.0.");
    this->Makefile->SetPolicyVersion("2.4");
  } else {
    this->Makefile->SetPolicyVersion(version_string.c_str());
  }

  return true;
}

bool cmCMakeMinimumRequired::EnforceUnknownArguments()
{
  if (!this->UnknownArguments.empty()) {
    std::ostringstream e;
    e << "called with unknown argument \"" << this->UnknownArguments[0]
      << "\".";
    this->SetError(e.str());
    return false;
  }
  return true;
}
