/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmIncludeGuardCommand.h"

#include "cmExecutionStatus.h"
#include "cmMakefile.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmake.h"

namespace {

enum IncludeGuardScope
{
  VARIABLE,
  DIRECTORY,
  GLOBAL
};

std::string GetIncludeGuardVariableName(std::string const& filePath)
{
  std::string result = "__INCGUARD_";
#ifdef CMAKE_BUILD_WITH_CMAKE
  result += cmSystemTools::ComputeStringMD5(filePath);
#else
  result += cmSystemTools::MakeCidentifier(filePath);
#endif
  result += "__";
  return result;
}

bool CheckIncludeGuardIsSet(cmMakefile* mf, std::string const& includeGuardVar)
{
  if (mf->GetProperty(includeGuardVar)) {
    return true;
  }
  cmStateSnapshot dirSnapshot =
    mf->GetStateSnapshot().GetBuildsystemDirectoryParent();
  while (dirSnapshot.GetState()) {
    cmStateDirectory stateDir = dirSnapshot.GetDirectory();
    if (stateDir.GetProperty(includeGuardVar)) {
      return true;
    }
    dirSnapshot = dirSnapshot.GetBuildsystemDirectoryParent();
  }
  return false;
}

} // anonymous namespace

// cmIncludeGuardCommand
bool cmIncludeGuardCommand::InitialPass(std::vector<std::string> const& args,
                                        cmExecutionStatus& status)
{
  if (args.size() > 1) {
    this->SetError(
      "given an invalid number of arguments. The command takes at "
      "most 1 argument.");
    return false;
  }

  IncludeGuardScope scope = VARIABLE;

  if (!args.empty()) {
    std::string const& arg = args[0];
    if (arg == "DIRECTORY") {
      scope = DIRECTORY;
    } else if (arg == "GLOBAL") {
      scope = GLOBAL;
    } else {
      this->SetError("given an invalid scope: " + arg);
      return false;
    }
  }

  std::string includeGuardVar = GetIncludeGuardVariableName(
    this->Makefile->GetDefinition("CMAKE_CURRENT_LIST_FILE"));

  cmMakefile* const mf = this->Makefile;

  switch (scope) {
    case VARIABLE:
      if (mf->IsDefinitionSet(includeGuardVar)) {
        status.SetReturnInvoked();
        return true;
      }
      mf->AddDefinition(includeGuardVar, true);
      break;
    case DIRECTORY:
      if (CheckIncludeGuardIsSet(mf, includeGuardVar)) {
        status.SetReturnInvoked();
        return true;
      }
      mf->SetProperty(includeGuardVar, "TRUE");
      break;
    case GLOBAL:
      cmake* const cm = mf->GetCMakeInstance();
      if (cm->GetProperty(includeGuardVar)) {
        status.SetReturnInvoked();
        return true;
      }
      cm->SetProperty(includeGuardVar, "TRUE");
      break;
  }

  return true;
}
