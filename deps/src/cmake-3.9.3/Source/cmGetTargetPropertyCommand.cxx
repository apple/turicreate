/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGetTargetPropertyCommand.h"

#include <sstream>

#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmTarget.h"
#include "cmTargetPropertyComputer.h"
#include "cmake.h"

class cmExecutionStatus;
class cmMessenger;

// cmSetTargetPropertyCommand
bool cmGetTargetPropertyCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() != 3) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  std::string const& var = args[0];
  std::string const& targetName = args[1];
  std::string prop;
  bool prop_exists = false;

  if (cmTarget* tgt = this->Makefile->FindTargetToUse(targetName)) {
    if (args[2] == "ALIASED_TARGET") {
      if (this->Makefile->IsAlias(targetName)) {
        prop = tgt->GetName();
        prop_exists = true;
      }
    } else if (!args[2].empty()) {
      const char* prop_cstr = CM_NULLPTR;
      cmListFileBacktrace bt = this->Makefile->GetBacktrace();
      cmMessenger* messenger = this->Makefile->GetMessenger();
      if (cmTargetPropertyComputer::PassesWhitelist(tgt->GetType(), args[2],
                                                    messenger, bt)) {
        prop_cstr = tgt->GetComputedProperty(args[2], messenger, bt);
        if (!prop_cstr) {
          prop_cstr = tgt->GetProperty(args[2]);
        }
      }
      if (prop_cstr) {
        prop = prop_cstr;
        prop_exists = true;
      }
    }
  } else {
    bool issueMessage = false;
    std::ostringstream e;
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0045)) {
      case cmPolicies::WARN:
        issueMessage = true;
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0045) << "\n";
      case cmPolicies::OLD:
        break;
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::NEW:
        issueMessage = true;
        messageType = cmake::FATAL_ERROR;
    }
    if (issueMessage) {
      e << "get_target_property() called with non-existent target \""
        << targetName << "\".";
      this->Makefile->IssueMessage(messageType, e.str());
      if (messageType == cmake::FATAL_ERROR) {
        return false;
      }
    }
  }
  if (prop_exists) {
    this->Makefile->AddDefinition(var, prop.c_str());
    return true;
  }
  this->Makefile->AddDefinition(var, (var + "-NOTFOUND").c_str());
  return true;
}
