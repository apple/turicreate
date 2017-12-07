/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDisallowedCommand_h
#define cmDisallowedCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"
#include "cmPolicies.h"

class cmExecutionStatus;

class cmDisallowedCommand : public cmCommand
{
public:
  cmDisallowedCommand(cmCommand* command, cmPolicies::PolicyID policy,
                      const char* message)
    : Command(command)
    , Policy(policy)
    , Message(message)
  {
  }

  ~cmDisallowedCommand() CM_OVERRIDE { delete this->Command; }

  cmCommand* Clone() CM_OVERRIDE
  {
    return new cmDisallowedCommand(this->Command->Clone(), this->Policy,
                                   this->Message);
  }

  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

  void FinalPass() CM_OVERRIDE { this->Command->FinalPass(); }

  bool HasFinalPass() const CM_OVERRIDE
  {
    return this->Command->HasFinalPass();
  }

private:
  cmCommand* Command;
  cmPolicies::PolicyID Policy;
  const char* Message;
};

#endif
