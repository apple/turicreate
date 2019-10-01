/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLinkDirectoriesCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmGeneratorExpression.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

// cmLinkDirectoriesCommand
bool cmLinkDirectoriesCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.empty()) {
    return true;
  }

  bool before = this->Makefile->IsOn("CMAKE_LINK_DIRECTORIES_BEFORE");

  auto i = args.cbegin();
  if ((*i) == "BEFORE") {
    before = true;
    ++i;
  } else if ((*i) == "AFTER") {
    before = false;
    ++i;
  }

  std::vector<std::string> directories;
  for (; i != args.cend(); ++i) {
    this->AddLinkDir(*i, directories);
  }

  this->Makefile->AddLinkDirectory(cmJoin(directories, ";"), before);

  return true;
}

void cmLinkDirectoriesCommand::AddLinkDir(
  std::string const& dir, std::vector<std::string>& directories)
{
  std::string unixPath = dir;
  cmSystemTools::ConvertToUnixSlashes(unixPath);
  if (!cmSystemTools::FileIsFullPath(unixPath) &&
      !cmGeneratorExpression::StartsWithGeneratorExpression(unixPath)) {
    bool convertToAbsolute = false;
    std::ostringstream e;
    /* clang-format off */
    e << "This command specifies the relative path\n"
      << "  " << unixPath << "\n"
      << "as a link directory.\n";
    /* clang-format on */
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0015)) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0015);
        this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, e.str());
        break;
      case cmPolicies::OLD:
        // OLD behavior does not convert
        break;
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS:
        e << cmPolicies::GetRequiredPolicyError(cmPolicies::CMP0015);
        this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
        CM_FALLTHROUGH;
      case cmPolicies::NEW:
        // NEW behavior converts
        convertToAbsolute = true;
        break;
    }
    if (convertToAbsolute) {
      std::string tmp = this->Makefile->GetCurrentSourceDirectory();
      tmp += "/";
      tmp += unixPath;
      unixPath = tmp;
    }
  }
  directories.push_back(unixPath);
}
