/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGetCMakePropertyCommand.h"

#include <set>

#include "cmAlgorithms.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"

class cmExecutionStatus;

// cmGetCMakePropertyCommand
bool cmGetCMakePropertyCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  std::string const& variable = args[0];
  std::string output = "NOTFOUND";

  if (args[1] == "VARIABLES") {
    if (const char* varsProp = this->Makefile->GetProperty("VARIABLES")) {
      output = varsProp;
    }
  } else if (args[1] == "MACROS") {
    output.clear();
    if (const char* macrosProp = this->Makefile->GetProperty("MACROS")) {
      output = macrosProp;
    }
  } else if (args[1] == "COMPONENTS") {
    const std::set<std::string>* components =
      this->Makefile->GetGlobalGenerator()->GetInstallComponents();
    output = cmJoin(*components, ";");
  } else {
    const char* prop = nullptr;
    if (!args[1].empty()) {
      prop = this->Makefile->GetState()->GetGlobalProperty(args[1]);
    }
    if (prop) {
      output = prop;
    }
  }

  this->Makefile->AddDefinition(variable, output.c_str());

  return true;
}
