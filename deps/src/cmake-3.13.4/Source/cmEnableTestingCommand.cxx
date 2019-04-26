/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmEnableTestingCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

// we do this in the final pass so that we now the subdirs have all
// been defined
bool cmEnableTestingCommand::InitialPass(std::vector<std::string> const&,
                                         cmExecutionStatus&)
{
  this->Makefile->AddDefinition("CMAKE_TESTING_ENABLED", "1");
  return true;
}
