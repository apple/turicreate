#include "cmPolicies.h"

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmVersion.h"
#include "cmake.h"

#include <assert.h>
#include <ctype.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>

static bool stringToId(const char* input, cmPolicies::PolicyID& pid)
{
  assert(input);
  if (strlen(input) != 7) {
    return false;
  }
  if (!cmHasLiteralPrefix(input, "CMP")) {
    return false;
  }
  if (cmHasLiteralSuffix(input, "0000")) {
    pid = cmPolicies::CMP0000;
    return true;
  }
  for (int i = 3; i < 7; ++i) {
    if (!isdigit(*(input + i))) {
      return false;
    }
  }
  long id;
  if (!cmSystemTools::StringToLong(input + 3, &id)) {
    return false;
  }
  if (id >= cmPolicies::CMPCOUNT) {
    return false;
  }
  pid = cmPolicies::PolicyID(id);
  return true;
}

#define CM_SELECT_ID_VERSION(F, A1, A2, A3, A4, A5, A6) F(A1, A3, A4, A5)
#define CM_FOR_EACH_POLICY_ID_VERSION(POLICY)                                 \
  CM_FOR_EACH_POLICY_TABLE(POLICY, CM_SELECT_ID_VERSION)

#define CM_SELECT_ID_DOC(F, A1, A2, A3, A4, A5, A6) F(A1, A2)
#define CM_FOR_EACH_POLICY_ID_DOC(POLICY)                                     \
  CM_FOR_EACH_POLICY_TABLE(POLICY, CM_SELECT_ID_DOC)

static const char* idToString(cmPolicies::PolicyID id)
{
  switch (id) {
#define POLICY_CASE(ID)                                                       \
  case cmPolicies::ID:                                                        \
    return #ID;
    CM_FOR_EACH_POLICY_ID(POLICY_CASE)
#undef POLICY_CASE
    case cmPolicies::CMPCOUNT:
      return nullptr;
  }
  return nullptr;
}

static const char* idToVersion(cmPolicies::PolicyID id)
{
  switch (id) {
#define POLICY_CASE(ID, V_MAJOR, V_MINOR, V_PATCH)                            \
  case cmPolicies::ID:                                                        \
    return #V_MAJOR "." #V_MINOR "." #V_PATCH;
    CM_FOR_EACH_POLICY_ID_VERSION(POLICY_CASE)
#undef POLICY_CASE
    case cmPolicies::CMPCOUNT:
      return nullptr;
  }
  return nullptr;
}

static bool isPolicyNewerThan(cmPolicies::PolicyID id, unsigned int majorV,
                              unsigned int minorV, unsigned int patchV)
{
  switch (id) {
#define POLICY_CASE(ID, V_MAJOR, V_MINOR, V_PATCH)                            \
  case cmPolicies::ID:                                                        \
    return (majorV < (V_MAJOR) ||                                             \
            (majorV == (V_MAJOR) && minorV + 1 < (V_MINOR) + 1) ||            \
            (majorV == (V_MAJOR) && minorV == (V_MINOR) &&                    \
             patchV + 1 < (V_PATCH) + 1));
    CM_FOR_EACH_POLICY_ID_VERSION(POLICY_CASE)
#undef POLICY_CASE
    case cmPolicies::CMPCOUNT:
      return false;
  }
  return false;
}

const char* idToShortDescription(cmPolicies::PolicyID id)
{
  switch (id) {
#define POLICY_CASE(ID, SHORT_DESCRIPTION)                                    \
  case cmPolicies::ID:                                                        \
    return SHORT_DESCRIPTION;
    CM_FOR_EACH_POLICY_ID_DOC(POLICY_CASE)
#undef POLICY_CASE
    case cmPolicies::CMPCOUNT:
      return nullptr;
  }
  return nullptr;
}

static void DiagnoseAncientPolicies(
  std::vector<cmPolicies::PolicyID> const& ancient, unsigned int majorVer,
  unsigned int minorVer, unsigned int patchVer, cmMakefile* mf)
{
  std::ostringstream e;
  e << "The project requests behavior compatible with CMake version \""
    << majorVer << "." << minorVer << "." << patchVer
    << "\", which requires the OLD behavior for some policies:\n";
  for (cmPolicies::PolicyID i : ancient) {
    e << "  " << idToString(i) << ": " << idToShortDescription(i) << "\n";
  }
  e << "However, this version of CMake no longer supports the OLD "
    << "behavior for these policies.  "
    << "Please either update your CMakeLists.txt files to conform to "
    << "the new behavior or use an older version of CMake that still "
    << "supports the old behavior.";
  mf->IssueMessage(cmake::FATAL_ERROR, e.str());
}

static bool GetPolicyDefault(cmMakefile* mf, std::string const& policy,
                             cmPolicies::PolicyStatus* defaultSetting)
{
  std::string defaultVar = "CMAKE_POLICY_DEFAULT_" + policy;
  std::string const& defaultValue = mf->GetSafeDefinition(defaultVar);
  if (defaultValue == "NEW") {
    *defaultSetting = cmPolicies::NEW;
  } else if (defaultValue == "OLD") {
    *defaultSetting = cmPolicies::OLD;
  } else if (defaultValue.empty()) {
    *defaultSetting = cmPolicies::WARN;
  } else {
    std::ostringstream e;
    e << defaultVar << " has value \"" << defaultValue
      << "\" but must be \"OLD\", \"NEW\", or \"\" (empty).";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  return true;
}

bool cmPolicies::ApplyPolicyVersion(cmMakefile* mf,
                                    std::string const& version_min,
                                    std::string const& version_max)
{
  // Parse components of the minimum version.
  unsigned int minMajor = 2;
  unsigned int minMinor = 0;
  unsigned int minPatch = 0;
  unsigned int minTweak = 0;
  if (sscanf(version_min.c_str(), "%u.%u.%u.%u", &minMajor, &minMinor,
             &minPatch, &minTweak) < 2) {
    std::ostringstream e;
    e << "Invalid policy version value \"" << version_min << "\".  "
      << "A numeric major.minor[.patch[.tweak]] must be given.";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  // it is an error if the policy version is less than 2.4
  if (minMajor < 2 || (minMajor == 2 && minMinor < 4)) {
    mf->IssueMessage(
      cmake::FATAL_ERROR,
      "Compatibility with CMake < 2.4 is not supported by CMake >= 3.0.  "
      "For compatibility with older versions please use any CMake 2.8.x "
      "release or lower.");
    return false;
  }

  // It is an error if the policy version is greater than the running
  // CMake.
  if (minMajor > cmVersion::GetMajorVersion() ||
      (minMajor == cmVersion::GetMajorVersion() &&
       minMinor > cmVersion::GetMinorVersion()) ||
      (minMajor == cmVersion::GetMajorVersion() &&
       minMinor == cmVersion::GetMinorVersion() &&
       minPatch > cmVersion::GetPatchVersion()) ||
      (minMajor == cmVersion::GetMajorVersion() &&
       minMinor == cmVersion::GetMinorVersion() &&
       minPatch == cmVersion::GetPatchVersion() &&
       minTweak > cmVersion::GetTweakVersion())) {
    std::ostringstream e;
    e << "An attempt was made to set the policy version of CMake to \""
      << version_min << "\" which is greater than this version of CMake.  "
      << "This is not allowed because the greater version may have new "
      << "policies not known to this CMake.  "
      << "You may need a newer CMake version to build this project.";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  unsigned int polMajor = minMajor;
  unsigned int polMinor = minMinor;
  unsigned int polPatch = minPatch;

  if (!version_max.empty()) {
    // Parse components of the maximum version.
    unsigned int maxMajor = 0;
    unsigned int maxMinor = 0;
    unsigned int maxPatch = 0;
    unsigned int maxTweak = 0;
    if (sscanf(version_max.c_str(), "%u.%u.%u.%u", &maxMajor, &maxMinor,
               &maxPatch, &maxTweak) < 2) {
      std::ostringstream e;
      e << "Invalid policy max version value \"" << version_max << "\".  "
        << "A numeric major.minor[.patch[.tweak]] must be given.";
      mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }

    // It is an error if the min version is greater than the max version.
    if (minMajor > maxMajor || (minMajor == maxMajor && minMinor > maxMinor) ||
        (minMajor == maxMajor && minMinor == maxMinor &&
         minPatch > maxPatch) ||
        (minMajor == maxMajor && minMinor == maxMinor &&
         minPatch == maxPatch && minTweak > maxTweak)) {
      std::ostringstream e;
      e << "Policy VERSION range \"" << version_min << "..." << version_max
        << "\""
        << " specifies a larger minimum than maximum.";
      mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }

    // Use the max version as the policy version.
    polMajor = maxMajor;
    polMinor = maxMinor;
    polPatch = maxPatch;
  }

  return cmPolicies::ApplyPolicyVersion(mf, polMajor, polMinor, polPatch);
}

bool cmPolicies::ApplyPolicyVersion(cmMakefile* mf, unsigned int majorVer,
                                    unsigned int minorVer,
                                    unsigned int patchVer)
{
  // now loop over all the policies and set them as appropriate
  std::vector<cmPolicies::PolicyID> ancientPolicies;
  for (PolicyID pid = cmPolicies::CMP0000; pid != cmPolicies::CMPCOUNT;
       pid = PolicyID(pid + 1)) {
    if (isPolicyNewerThan(pid, majorVer, minorVer, patchVer)) {
      if (cmPolicies::GetPolicyStatus(pid) == cmPolicies::REQUIRED_ALWAYS) {
        ancientPolicies.push_back(pid);
      } else {
        cmPolicies::PolicyStatus status = cmPolicies::WARN;
        if (!GetPolicyDefault(mf, idToString(pid), &status) ||
            !mf->SetPolicy(pid, status)) {
          return false;
        }
        if (pid == cmPolicies::CMP0001 &&
            (status == cmPolicies::WARN || status == cmPolicies::OLD)) {
          if (!(mf->GetState()->GetInitializedCacheValue(
                "CMAKE_BACKWARDS_COMPATIBILITY"))) {
            // Set it to 2.4 because that is the last version where the
            // variable had meaning.
            mf->AddCacheDefinition(
              "CMAKE_BACKWARDS_COMPATIBILITY", "2.4",
              "For backwards compatibility, what version of CMake "
              "commands and "
              "syntax should this version of CMake try to support.",
              cmStateEnums::STRING);
          }
        }
      }
    } else {
      if (!mf->SetPolicy(pid, cmPolicies::NEW)) {
        return false;
      }
    }
  }

  // Make sure the project does not use any ancient policies.
  if (!ancientPolicies.empty()) {
    DiagnoseAncientPolicies(ancientPolicies, majorVer, minorVer, patchVer, mf);
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }

  return true;
}

bool cmPolicies::GetPolicyID(const char* id, cmPolicies::PolicyID& pid)
{
  return stringToId(id, pid);
}

///! return a warning string for a given policy
std::string cmPolicies::GetPolicyWarning(cmPolicies::PolicyID id)
{
  std::ostringstream msg;
  msg << "Policy " << idToString(id)
      << " is not set: "
         ""
      << idToShortDescription(id)
      << "  "
         "Run \"cmake --help-policy "
      << idToString(id)
      << "\" for "
         "policy details.  "
         "Use the cmake_policy command to set the policy "
         "and suppress this warning.";
  return msg.str();
}

std::string cmPolicies::GetPolicyDeprecatedWarning(cmPolicies::PolicyID id)
{
  std::ostringstream msg;
  /* clang-format off */
  msg <<
    "The OLD behavior for policy " << idToString(id) << " "
    "will be removed from a future version of CMake.\n"
    "The cmake-policies(7) manual explains that the OLD behaviors of all "
    "policies are deprecated and that a policy should be set to OLD only "
    "under specific short-term circumstances.  Projects should be ported "
    "to the NEW behavior and not rely on setting a policy to OLD."
    ;
  /* clang-format on */
  return msg.str();
}

///! return an error string for when a required policy is unspecified
std::string cmPolicies::GetRequiredPolicyError(cmPolicies::PolicyID id)
{
  std::ostringstream error;
  error << "Policy " << idToString(id)
        << " is not set to NEW: "
           ""
        << idToShortDescription(id)
        << "  "
           "Run \"cmake --help-policy "
        << idToString(id)
        << "\" for "
           "policy details.  "
           "CMake now requires this policy to be set to NEW by the project.  "
           "The policy may be set explicitly using the code\n"
           "  cmake_policy(SET "
        << idToString(id)
        << " NEW)\n"
           "or by upgrading all policies with the code\n"
           "  cmake_policy(VERSION "
        << idToVersion(id)
        << ") # or later\n"
           "Run \"cmake --help-command cmake_policy\" for more information.";
  return error.str();
}

///! Get the default status for a policy
cmPolicies::PolicyStatus cmPolicies::GetPolicyStatus(
  cmPolicies::PolicyID /*unused*/)
{
  return cmPolicies::WARN;
}

std::string cmPolicies::GetRequiredAlwaysPolicyError(cmPolicies::PolicyID id)
{
  std::string pid = idToString(id);
  std::ostringstream e;
  e << "Policy " << pid << " may not be set to OLD behavior because this "
    << "version of CMake no longer supports it.  "
    << "The policy was introduced in "
    << "CMake version " << idToVersion(id)
    << ", and use of NEW behavior is now required."
    << "\n"
    << "Please either update your CMakeLists.txt files to conform to "
    << "the new behavior or use an older version of CMake that still "
    << "supports the old behavior.  "
    << "Run cmake --help-policy " << pid << " for more information.";
  return e.str();
}

cmPolicies::PolicyStatus cmPolicies::PolicyMap::Get(
  cmPolicies::PolicyID id) const
{
  PolicyStatus status = cmPolicies::WARN;

  if (this->Status[(POLICY_STATUS_COUNT * id) + OLD]) {
    status = cmPolicies::OLD;
  } else if (this->Status[(POLICY_STATUS_COUNT * id) + NEW]) {
    status = cmPolicies::NEW;
  }
  return status;
}

void cmPolicies::PolicyMap::Set(cmPolicies::PolicyID id,
                                cmPolicies::PolicyStatus status)
{
  this->Status[(POLICY_STATUS_COUNT * id) + OLD] = (status == OLD);
  this->Status[(POLICY_STATUS_COUNT * id) + WARN] = (status == WARN);
  this->Status[(POLICY_STATUS_COUNT * id) + NEW] = (status == NEW);
}

bool cmPolicies::PolicyMap::IsDefined(cmPolicies::PolicyID id) const
{
  return this->Status[(POLICY_STATUS_COUNT * id) + OLD] ||
    this->Status[(POLICY_STATUS_COUNT * id) + WARN] ||
    this->Status[(POLICY_STATUS_COUNT * id) + NEW];
}

bool cmPolicies::PolicyMap::IsEmpty() const
{
  return this->Status.none();
}
