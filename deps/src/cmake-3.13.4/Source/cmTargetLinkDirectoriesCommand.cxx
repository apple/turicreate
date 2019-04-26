/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTargetLinkDirectoriesCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmGeneratorExpression.h"
#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmTargetLinkDirectoriesCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  return this->HandleArguments(args, "LINK_DIRECTORIES", PROCESS_BEFORE);
}

void cmTargetLinkDirectoriesCommand::HandleMissingTarget(
  const std::string& name)
{
  std::ostringstream e;
  e << "Cannot specify link directories for target \"" << name
    << "\" which is not built by this project.";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

std::string cmTargetLinkDirectoriesCommand::Join(
  const std::vector<std::string>& content)
{
  std::vector<std::string> directories;

  for (const auto& dir : content) {
    auto unixPath = dir;
    cmSystemTools::ConvertToUnixSlashes(unixPath);
    if (!cmSystemTools::FileIsFullPath(unixPath) &&
        !cmGeneratorExpression::StartsWithGeneratorExpression(unixPath)) {
      auto tmp = this->Makefile->GetCurrentSourceDirectory();
      tmp += "/";
      tmp += unixPath;
      unixPath = tmp;
    }
    directories.push_back(unixPath);
  }

  return cmJoin(directories, ";");
}

bool cmTargetLinkDirectoriesCommand::HandleDirectContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool prepend, bool)
{
  cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();

  tgt->InsertLinkDirectory(this->Join(content), lfbt, prepend);

  return true; // Successfully handled.
}
