/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAddCompileDefinitionsCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

bool cmAddCompileDefinitionsCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.empty()) {
    return true;
  }

  for (std::string const& i : args) {
    this->Makefile->AddCompileDefinition(i);
  }
  return true;
}
