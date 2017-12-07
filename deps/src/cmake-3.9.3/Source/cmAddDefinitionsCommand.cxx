/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAddDefinitionsCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

// cmAddDefinitionsCommand
bool cmAddDefinitionsCommand::InitialPass(std::vector<std::string> const& args,
                                          cmExecutionStatus&)
{
  // it is OK to have no arguments
  if (args.empty()) {
    return true;
  }

  for (std::vector<std::string>::const_iterator i = args.begin();
       i != args.end(); ++i) {
    this->Makefile->AddDefineFlag(i->c_str());
  }
  return true;
}
