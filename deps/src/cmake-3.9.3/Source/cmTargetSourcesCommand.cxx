/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTargetSourcesCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmTargetSourcesCommand::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  return this->HandleArguments(args, "SOURCES");
}

void cmTargetSourcesCommand::HandleImportedTarget(const std::string& tgt)
{
  std::ostringstream e;
  e << "Cannot specify sources for imported target \"" << tgt << "\".";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

void cmTargetSourcesCommand::HandleMissingTarget(const std::string& name)
{
  std::ostringstream e;
  e << "Cannot specify sources for target \"" << name
    << "\" "
       "which is not built by this project.";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

std::string cmTargetSourcesCommand::Join(
  const std::vector<std::string>& content)
{
  return cmJoin(content, ";");
}

bool cmTargetSourcesCommand::HandleDirectContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool, bool)
{
  tgt->AppendProperty("SOURCES", this->Join(content).c_str());
  return true;
}
