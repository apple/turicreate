/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLinkDirectoriesCommand.h"

#include <sstream>

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

  for (std::vector<std::string>::const_iterator i = args.begin();
       i != args.end(); ++i) {
    this->AddLinkDir(*i);
  }
  return true;
}

void cmLinkDirectoriesCommand::AddLinkDir(std::string const& dir)
{
  std::string unixPath = dir;
  cmSystemTools::ConvertToUnixSlashes(unixPath);
  if (!cmSystemTools::FileIsFullPath(unixPath.c_str())) {
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
  this->Makefile->AppendProperty("LINK_DIRECTORIES", unixPath.c_str());
}
