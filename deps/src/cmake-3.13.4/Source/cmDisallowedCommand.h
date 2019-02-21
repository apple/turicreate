/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDisallowedCommand_h
#define cmDisallowedCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

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

  ~cmDisallowedCommand() override { delete this->Command; }

  cmCommand* Clone() override
  {
    return new cmDisallowedCommand(this->Command->Clone(), this->Policy,
                                   this->Message);
  }

  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

  void FinalPass() override { this->Command->FinalPass(); }

  bool HasFinalPass() const override { return this->Command->HasFinalPass(); }

private:
  cmCommand* Command;
  cmPolicies::PolicyID Policy;
  const char* Message;
};

#endif
