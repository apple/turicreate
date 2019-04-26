/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTargetCompileDefinitionsCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmTargetCompileDefinitionsCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  return this->HandleArguments(args, "COMPILE_DEFINITIONS");
}

void cmTargetCompileDefinitionsCommand::HandleMissingTarget(
  const std::string& name)
{
  std::ostringstream e;
  e << "Cannot specify compile definitions for target \"" << name
    << "\" "
       "which is not built by this project.";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

std::string cmTargetCompileDefinitionsCommand::Join(
  const std::vector<std::string>& content)
{
  std::string defs;
  std::string sep;
  for (std::string const& it : content) {
    if (cmHasLiteralPrefix(it, "-D")) {
      defs += sep + it.substr(2);
    } else {
      defs += sep + it;
    }
    sep = ";";
  }
  return defs;
}

bool cmTargetCompileDefinitionsCommand::HandleDirectContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool, bool)
{
  tgt->AppendProperty("COMPILE_DEFINITIONS", this->Join(content).c_str());
  return true; // Successfully handled.
}
