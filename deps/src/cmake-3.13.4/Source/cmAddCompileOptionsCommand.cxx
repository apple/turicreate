/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAddCompileOptionsCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

bool cmAddCompileOptionsCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.empty()) {
    return true;
  }

  for (std::string const& i : args) {
    this->Makefile->AddCompileOption(i);
  }
  return true;
}
