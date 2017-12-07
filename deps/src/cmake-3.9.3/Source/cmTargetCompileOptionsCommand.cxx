/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTargetCompileOptionsCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmTargetCompileOptionsCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  return this->HandleArguments(args, "COMPILE_OPTIONS", PROCESS_BEFORE);
}

void cmTargetCompileOptionsCommand::HandleImportedTarget(
  const std::string& tgt)
{
  std::ostringstream e;
  e << "Cannot specify compile options for imported target \"" << tgt << "\".";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

void cmTargetCompileOptionsCommand::HandleMissingTarget(
  const std::string& name)
{
  std::ostringstream e;
  e << "Cannot specify compile options for target \"" << name
    << "\" "
       "which is not built by this project.";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

std::string cmTargetCompileOptionsCommand::Join(
  const std::vector<std::string>& content)
{
  return cmJoin(content, ";");
}

bool cmTargetCompileOptionsCommand::HandleDirectContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool, bool)
{
  cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
  tgt->InsertCompileOption(this->Join(content), lfbt);
  return true;
}
