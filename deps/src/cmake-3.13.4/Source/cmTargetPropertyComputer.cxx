/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmTargetPropertyComputer.h"

#include <cctype>
#include <sstream>
#include <unordered_set>

#include "cmMessenger.h"
#include "cmPolicies.h"
#include "cmStateSnapshot.h"
#include "cmake.h"

bool cmTargetPropertyComputer::HandleLocationPropertyPolicy(
  std::string const& tgtName, cmMessenger* messenger,
  cmListFileBacktrace const& context)
{
  std::ostringstream e;
  const char* modal = nullptr;
  cmake::MessageType messageType = cmake::AUTHOR_WARNING;
  switch (context.GetBottom().GetPolicy(cmPolicies::CMP0026)) {
    case cmPolicies::WARN:
      e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0026) << "\n";
      modal = "should";
    case cmPolicies::OLD:
      break;
    case cmPolicies::REQUIRED_ALWAYS:
    case cmPolicies::REQUIRED_IF_USED:
    case cmPolicies::NEW:
      modal = "may";
      messageType = cmake::FATAL_ERROR;
  }

  if (modal) {
    e << "The LOCATION property " << modal << " not be read from target \""
      << tgtName
      << "\".  Use the target name directly with "
         "add_custom_command, or use the generator expression $<TARGET_FILE>, "
         "as appropriate.\n";
    messenger->IssueMessage(messageType, e.str(), context);
  }

  return messageType != cmake::FATAL_ERROR;
}

bool cmTargetPropertyComputer::WhiteListedInterfaceProperty(
  const std::string& prop)
{
  if (cmHasLiteralPrefix(prop, "INTERFACE_")) {
    return true;
  }
  if (cmHasLiteralPrefix(prop, "_")) {
    return true;
  }
  if (std::islower(prop[0])) {
    return true;
  }
  static std::unordered_set<std::string> builtIns;
  if (builtIns.empty()) {
    builtIns.insert("COMPATIBLE_INTERFACE_BOOL");
    builtIns.insert("COMPATIBLE_INTERFACE_NUMBER_MAX");
    builtIns.insert("COMPATIBLE_INTERFACE_NUMBER_MIN");
    builtIns.insert("COMPATIBLE_INTERFACE_STRING");
    builtIns.insert("EXPORT_NAME");
    builtIns.insert("IMPORTED");
    builtIns.insert("IMPORTED_GLOBAL");
    builtIns.insert("NAME");
    builtIns.insert("TYPE");
  }

  if (builtIns.count(prop)) {
    return true;
  }

  if (prop == "IMPORTED_CONFIGURATIONS" || prop == "IMPORTED_LIBNAME" ||
      cmHasLiteralPrefix(prop, "IMPORTED_LIBNAME_") ||
      cmHasLiteralPrefix(prop, "MAP_IMPORTED_CONFIG_")) {
    return true;
  }

  // This property should not be allowed but was incorrectly added in
  // CMake 3.8.  We can't remove it from the whitelist without breaking
  // projects that try to set it.  One day we could warn about this, but
  // for now silently accept it.
  if (prop == "NO_SYSTEM_FROM_IMPORTED") {
    return true;
  }

  return false;
}

bool cmTargetPropertyComputer::PassesWhitelist(
  cmStateEnums::TargetType tgtType, std::string const& prop,
  cmMessenger* messenger, cmListFileBacktrace const& context)
{
  if (tgtType == cmStateEnums::INTERFACE_LIBRARY &&
      !WhiteListedInterfaceProperty(prop)) {
    std::ostringstream e;
    e << "INTERFACE_LIBRARY targets may only have whitelisted properties.  "
         "The property \""
      << prop << "\" is not allowed.";
    messenger->IssueMessage(cmake::FATAL_ERROR, e.str(), context);
    return false;
  }
  return true;
}
