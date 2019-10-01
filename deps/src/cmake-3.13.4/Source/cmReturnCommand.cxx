/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmReturnCommand.h"

#include "cmExecutionStatus.h"

// cmReturnCommand
bool cmReturnCommand::InitialPass(std::vector<std::string> const&,
                                  cmExecutionStatus& status)
{
  status.SetReturnInvoked();
  return true;
}
