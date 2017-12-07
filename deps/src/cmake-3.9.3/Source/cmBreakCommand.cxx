/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmBreakCommand.h"

#include <sstream>

#include "cmExecutionStatus.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmake.h"

// cmBreakCommand
bool cmBreakCommand::InitialPass(std::vector<std::string> const& args,
                                 cmExecutionStatus& status)
{
  if (!this->Makefile->IsLoopBlock()) {
    bool issueMessage = true;
    std::ostringstream e;
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0055)) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0055) << "\n";
        break;
      case cmPolicies::OLD:
        issueMessage = false;
        break;
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::NEW:
        messageType = cmake::FATAL_ERROR;
        break;
    }

    if (issueMessage) {
      e << "A BREAK command was found outside of a proper "
           "FOREACH or WHILE loop scope.";
      this->Makefile->IssueMessage(messageType, e.str());
      if (messageType == cmake::FATAL_ERROR) {
        return false;
      }
    }
  }

  status.SetBreakInvoked();

  if (!args.empty()) {
    bool issueMessage = true;
    std::ostringstream e;
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0055)) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0055) << "\n";
        break;
      case cmPolicies::OLD:
        issueMessage = false;
        break;
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::NEW:
        messageType = cmake::FATAL_ERROR;
        break;
    }

    if (issueMessage) {
      e << "The BREAK command does not accept any arguments.";
      this->Makefile->IssueMessage(messageType, e.str());
      if (messageType == cmake::FATAL_ERROR) {
        return false;
      }
    }
  }

  return true;
}
