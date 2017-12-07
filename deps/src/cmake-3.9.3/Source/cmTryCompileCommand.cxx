/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTryCompileCommand.h"

#include "cmMakefile.h"
#include "cmake.h"

class cmExecutionStatus;

// cmTryCompileCommand
bool cmTryCompileCommand::InitialPass(std::vector<std::string> const& argv,
                                      cmExecutionStatus&)
{
  if (argv.size() < 3) {
    return false;
  }

  if (this->Makefile->GetCMakeInstance()->GetWorkingMode() ==
      cmake::FIND_PACKAGE_MODE) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR,
      "The TRY_COMPILE() command is not supported in --find-package mode.");
    return false;
  }

  this->TryCompileCode(argv, false);

  // if They specified clean then we clean up what we can
  if (this->SrcFileSignature) {
    if (!this->Makefile->GetCMakeInstance()->GetDebugTryCompile()) {
      this->CleanupFiles(this->BinaryDirectory.c_str());
    }
  }
  return true;
}
