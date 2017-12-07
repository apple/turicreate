/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmVariableRequiresCommand.h"

#include "cmMakefile.h"
#include "cmState.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmLibraryCommand
bool cmVariableRequiresCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() < 3) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  std::string const& testVariable = args[0];
  if (!this->Makefile->IsOn(testVariable)) {
    return true;
  }
  std::string const& resultVariable = args[1];
  bool requirementsMet = true;
  std::string notSet;
  bool hasAdvanced = false;
  cmState* state = this->Makefile->GetState();
  for (unsigned int i = 2; i < args.size(); ++i) {
    if (!this->Makefile->IsOn(args[i])) {
      requirementsMet = false;
      notSet += args[i];
      notSet += "\n";
      if (state->GetCacheEntryValue(args[i]) &&
          state->GetCacheEntryPropertyAsBool(args[i], "ADVANCED")) {
        hasAdvanced = true;
      }
    }
  }
  const char* reqVar = this->Makefile->GetDefinition(resultVariable);
  // if reqVar is unset, then set it to requirementsMet
  // if reqVar is set to true, but requirementsMet is false , then
  // set reqVar to false.
  if (!reqVar || (!requirementsMet && this->Makefile->IsOn(reqVar))) {
    this->Makefile->AddDefinition(resultVariable, requirementsMet);
  }

  if (!requirementsMet) {
    std::string message = "Variable assertion failed:\n";
    message +=
      testVariable + " Requires that the following unset variables are set:\n";
    message += notSet;
    message += "\nPlease set them, or set ";
    message += testVariable + " to false, and re-configure.\n";
    if (hasAdvanced) {
      message +=
        "One or more of the required variables is advanced."
        "  To set the variable, you must turn on advanced mode in cmake.";
    }
    cmSystemTools::Error(message.c_str());
  }

  return true;
}
