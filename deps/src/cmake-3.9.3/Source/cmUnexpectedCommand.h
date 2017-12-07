/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmUnexpectedCommand_h
#define cmUnexpectedCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

class cmUnexpectedCommand : public cmCommand
{
public:
  cmUnexpectedCommand(std::string const& name, const char* error)
    : Name(name)
    , Error(error)
  {
  }

  cmCommand* Clone() CM_OVERRIDE
  {
    return new cmUnexpectedCommand(this->Name, this->Error);
  }

  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  std::string Name;
  const char* Error;
};

#endif
