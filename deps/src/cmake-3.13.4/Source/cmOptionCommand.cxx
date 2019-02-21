/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmOptionCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmState.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

// cmOptionCommand
bool cmOptionCommand::InitialPass(std::vector<std::string> const& args,
                                  cmExecutionStatus&)
{
  const bool argError = (args.size() < 2) || (args.size() > 3);
  if (argError) {
    std::string m = "called with incorrect number of arguments: ";
    m += cmJoin(args, " ");
    this->SetError(m);
    return false;
  }

  // Determine the state of the option policy
  bool checkAndWarn = false;
  {
    auto status = this->Makefile->GetPolicyStatus(cmPolicies::CMP0077);
    const auto* existsBeforeSet =
      this->Makefile->GetStateSnapshot().GetDefinition(args[0]);
    switch (status) {
      case cmPolicies::WARN:
        checkAndWarn = (existsBeforeSet != nullptr);
        break;
      case cmPolicies::OLD:
        // OLD behavior does not warn.
        break;
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::NEW: {
        // See if a local variable with this name already exists.
        // If so we ignore the option command.
        if (existsBeforeSet) {
          return true;
        }
      } break;
    }
  }

  // See if a cache variable with this name already exists
  // If so just make sure the doc state is correct
  cmState* state = this->Makefile->GetState();
  const char* existingValue = state->GetCacheEntryValue(args[0]);
  if (existingValue &&
      (state->GetCacheEntryType(args[0]) != cmStateEnums::UNINITIALIZED)) {
    state->SetCacheEntryProperty(args[0], "HELPSTRING", args[1]);
    return true;
  }

  // Nothing in the cache so add it
  std::string initialValue = existingValue ? existingValue : "Off";
  if (args.size() == 3) {
    initialValue = args[2];
  }
  bool init = cmSystemTools::IsOn(initialValue);
  this->Makefile->AddCacheDefinition(args[0], init ? "ON" : "OFF",
                                     args[1].c_str(), cmStateEnums::BOOL);

  if (checkAndWarn) {
    const auto* existsAfterSet =
      this->Makefile->GetStateSnapshot().GetDefinition(args[0]);
    if (!existsAfterSet) {
      std::ostringstream w;
      w << cmPolicies::GetPolicyWarning(cmPolicies::CMP0077)
        << "\n"
           "For compatibility with older versions of CMake, option "
           "is clearing the normal variable '"
        << args[0] << "'.";
      this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, w.str());
    }
  }
  return true;
}
