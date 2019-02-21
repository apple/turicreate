/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmRemoveCommand.h"

#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmRemoveCommand
bool cmRemoveCommand::InitialPass(std::vector<std::string> const& args,
                                  cmExecutionStatus&)
{
  if (args.empty()) {
    return true;
  }

  std::string const& variable = args[0]; // VAR is always first
  // get the old value
  const char* cacheValue = this->Makefile->GetDefinition(variable);

  // if there is no old value then return
  if (!cacheValue) {
    return true;
  }

  // expand the variable
  std::vector<std::string> varArgsExpanded;
  cmSystemTools::ExpandListArgument(cacheValue, varArgsExpanded);

  // expand the args
  // check for REMOVE(VAR v1 v2 ... vn)
  std::vector<std::string> argsExpanded;
  std::vector<std::string> temp;
  temp.insert(temp.end(), args.begin() + 1, args.end());
  cmSystemTools::ExpandList(temp, argsExpanded);

  // now create the new value
  std::string value;
  for (std::string const& varArgExpanded : varArgsExpanded) {
    int found = 0;
    for (std::string const& argExpanded : argsExpanded) {
      if (varArgExpanded == argExpanded) {
        found = 1;
        break;
      }
    }
    if (!found) {
      if (!value.empty()) {
        value += ";";
      }
      value += varArgExpanded;
    }
  }

  // add the definition
  this->Makefile->AddDefinition(variable, value.c_str());

  return true;
}
