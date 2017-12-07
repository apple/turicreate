/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmUnsetCommand.h"

#include <string.h>

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

  const char* variable = args[0].c_str();

  // unset(ENV{VAR})
  if (cmHasLiteralPrefix(variable, "ENV{") && strlen(variable) > 5) {
    // what is the variable name
    char* envVarName = new char[strlen(variable)];
    strncpy(envVarName, variable + 4, strlen(variable) - 5);
    envVarName[strlen(variable) - 5] = '\0';

#ifdef CMAKE_BUILD_WITH_CMAKE
    cmSystemTools::UnsetEnv(envVarName);
#endif
    delete[] envVarName;
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
    this->Makefile->RaiseScope(variable, CM_NULLPTR);
    return true;
  }
  // ERROR: second argument isn't CACHE or PARENT_SCOPE
  this->SetError("called with an invalid second argument");
  return false;
}
