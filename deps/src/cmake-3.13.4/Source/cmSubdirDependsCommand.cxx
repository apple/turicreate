/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSubdirDependsCommand.h"

class cmExecutionStatus;

bool cmSubdirDependsCommand::InitialPass(std::vector<std::string> const&,
                                         cmExecutionStatus&)
{
  return true;
}
