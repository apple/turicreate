/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmUnsetCommand.h"

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmUnsetCommand
bool cmUnsetCommand::InitialPass(std::vector<std::string> const& args,
                                 cmExecutionStatus&)
{
  if (args.empty() || args.size() > 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  auto const& variable = args[0];

  // unset(ENV{VAR})
  if (cmHasLiteralPrefix(variable, "ENV{") && variable.size() > 5) {
    // what is the variable name
    auto const& envVarName = variable.substr(4, variable.size() - 5);

#ifdef CMAKE_BUILD_WITH_CMAKE
    cmSystemTools::UnsetEnv(envVarName.c_str());
#endif
    return true;
  }
  // unset(VAR)
  if (args.size() == 1) {
    this->Makefile->RemoveDefinition(variable);
    return true;
  }
  // unset(VAR CACHE)
  if ((args.size() == 2) && (args[1] == "CACHE")) {
    this->Makefile->RemoveCacheDefinition(variable);
    return true;
  }
  // unset(VAR PARENT_SCOPE)
  if ((args.size() == 2) && (args[1] == "PARENT_SCOPE")) {
    this->Makefile->RaiseScope(variable, nullptr);
    return true;
  }
  // ERROR: second argument isn't CACHE or PARENT_SCOPE
  this->SetError("called with an invalid second argument");
  return false;
}
