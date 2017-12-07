/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMarkAsAdvancedCommand.h"

#include "cmMakefile.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

// cmMarkAsAdvancedCommand
bool cmMarkAsAdvancedCommand::InitialPass(std::vector<std::string> const& args,
                                          cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  unsigned int i = 0;
  const char* value = "1";
  bool overwrite = false;
  if (args[0] == "CLEAR" || args[0] == "FORCE") {
    overwrite = true;
    if (args[0] == "CLEAR") {
      value = "0";
    }
    i = 1;
  }
  for (; i < args.size(); ++i) {
    std::string const& variable = args[i];
    cmState* state = this->Makefile->GetState();
    if (!state->GetCacheEntryValue(variable)) {
      this->Makefile->GetCMakeInstance()->AddCacheEntry(
        variable, CM_NULLPTR, CM_NULLPTR, cmStateEnums::UNINITIALIZED);
      overwrite = true;
    }
    if (!state->GetCacheEntryValue(variable)) {
      cmSystemTools::Error("This should never happen...");
      return false;
    }
    if (!state->GetCacheEntryProperty(variable, "ADVANCED") || overwrite) {
      state->SetCacheEntryProperty(variable, "ADVANCED", value);
    }
  }
  return true;
}
