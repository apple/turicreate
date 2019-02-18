/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTargetCompileFeaturesCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmake.h"

class cmExecutionStatus;
class cmTarget;

bool cmTargetCompileFeaturesCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  return this->HandleArguments(args, "COMPILE_FEATURES", NO_FLAGS);
}

void cmTargetCompileFeaturesCommand::HandleMissingTarget(
  const std::string& name)
{
  std::ostringstream e;
  e << "Cannot specify compile features for target \"" << name
    << "\" "
       "which is not built by this project.";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

std::string cmTargetCompileFeaturesCommand::Join(
  const std::vector<std::string>& content)
{
  return cmJoin(content, ";");
}

bool cmTargetCompileFeaturesCommand::HandleDirectContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool, bool)
{
  for (std::string const& it : content) {
    std::string error;
    if (!this->Makefile->AddRequiredTargetFeature(tgt, it, &error)) {
      this->SetError(error);
      return false; // Not (successfully) handled.
    }
  }
  return true; // Successfully handled.
}
