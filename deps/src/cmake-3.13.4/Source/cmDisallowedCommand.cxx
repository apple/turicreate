/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDisallowedCommand.h"

#include "cmMakefile.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmDisallowedCommand::InitialPass(std::vector<std::string> const& args,
                                      cmExecutionStatus& status)
{
  switch (this->Makefile->GetPolicyStatus(this->Policy)) {
    case cmPolicies::WARN:
      this->Makefile->IssueMessage(cmake::AUTHOR_WARNING,
                                   cmPolicies::GetPolicyWarning(this->Policy));
      break;
    case cmPolicies::OLD:
      break;
    case cmPolicies::REQUIRED_IF_USED:
    case cmPolicies::REQUIRED_ALWAYS:
    case cmPolicies::NEW:
      this->Makefile->IssueMessage(cmake::FATAL_ERROR, this->Message);
      return true;
  }

  this->Command->SetMakefile(this->GetMakefile());
  bool const ret = this->Command->InitialPass(args, status);
  this->SetError(this->Command->GetError());
  return ret;
}
