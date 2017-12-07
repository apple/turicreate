/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTargetIncludeDirectoriesCommand.h"

#include <set>
#include <sstream>

#include "cmGeneratorExpression.h"
#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmTargetIncludeDirectoriesCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  return this->HandleArguments(args, "INCLUDE_DIRECTORIES",
                               ArgumentFlags(PROCESS_BEFORE | PROCESS_SYSTEM));
}

void cmTargetIncludeDirectoriesCommand::HandleImportedTarget(
  const std::string& tgt)
{
  std::ostringstream e;
  e << "Cannot specify include directories for imported target \"" << tgt
    << "\".";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

void cmTargetIncludeDirectoriesCommand::HandleMissingTarget(
  const std::string& name)
{
  std::ostringstream e;
  e << "Cannot specify include directories for target \"" << name
    << "\" "
       "which is not built by this project.";
  this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
}

std::string cmTargetIncludeDirectoriesCommand::Join(
  const std::vector<std::string>& content)
{
  std::string dirs;
  std::string sep;
  std::string prefix =
    this->Makefile->GetCurrentSourceDirectory() + std::string("/");
  for (std::vector<std::string>::const_iterator it = content.begin();
       it != content.end(); ++it) {
    if (cmSystemTools::FileIsFullPath(it->c_str()) ||
        cmGeneratorExpression::Find(*it) == 0) {
      dirs += sep + *it;
    } else {
      dirs += sep + prefix + *it;
    }
    sep = ";";
  }
  return dirs;
}

bool cmTargetIncludeDirectoriesCommand::HandleDirectContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool prepend,
  bool system)
{
  cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
  tgt->InsertInclude(this->Join(content), lfbt, prepend);
  if (system) {
    std::string prefix =
      this->Makefile->GetCurrentSourceDirectory() + std::string("/");
    std::set<std::string> sdirs;
    for (std::vector<std::string>::const_iterator it = content.begin();
         it != content.end(); ++it) {
      if (cmSystemTools::FileIsFullPath(it->c_str()) ||
          cmGeneratorExpression::Find(*it) == 0) {
        sdirs.insert(*it);
      } else {
        sdirs.insert(prefix + *it);
      }
    }
    tgt->AddSystemIncludeDirectories(sdirs);
  }
  return true;
}

void cmTargetIncludeDirectoriesCommand::HandleInterfaceContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool prepend,
  bool system)
{
  cmTargetPropCommandBase::HandleInterfaceContent(tgt, content, prepend,
                                                  system);

  if (system) {
    std::string joined = this->Join(content);
    tgt->AppendProperty("INTERFACE_SYSTEM_INCLUDE_DIRECTORIES",
                        joined.c_str());
  }
}
