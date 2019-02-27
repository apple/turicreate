/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTargetSourcesCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmGeneratorExpression.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmTargetSourcesCommand::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  return this->HandleArguments(args, "SOURCES");
}

void cmTargetSourcesCommand::HandleInterfaceContent(
  cmTarget* tgt, const std::vector<std::string>& content, bool prepend,
  bool system)
{
  cmTargetPropCommandBase::HandleInterfaceContent(
    tgt, ConvertToAbsoluteContent(tgt, content, true), prepend, system);
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
  tgt->AppendProperty(
    "SOURCES",
    this->Join(ConvertToAbsoluteContent(tgt, content, false)).c_str());
  return true; // Successfully handled.
}

std::vector<std::string> cmTargetSourcesCommand::ConvertToAbsoluteContent(
  cmTarget* tgt, const std::vector<std::string>& content,
  bool isInterfaceContent)
{
  // Skip conversion in case old behavior has been explicitly requested
  if (this->Makefile->GetPolicyStatus(cmPolicies::CMP0076) ==
      cmPolicies::OLD) {
    return content;
  }

  bool changedPath = false;
  std::vector<std::string> absoluteContent;
  absoluteContent.reserve(content.size());
  for (std::string const& src : content) {
    std::string absoluteSrc;
    if (cmSystemTools::FileIsFullPath(src) ||
        cmGeneratorExpression::Find(src) == 0 ||
        (!isInterfaceContent &&
         (this->Makefile->GetCurrentSourceDirectory() ==
          tgt->GetMakefile()->GetCurrentSourceDirectory()))) {
      absoluteSrc = src;
    } else {
      changedPath = true;
      absoluteSrc = this->Makefile->GetCurrentSourceDirectory();
      absoluteSrc += "/";
      absoluteSrc += src;
    }
    absoluteContent.push_back(absoluteSrc);
  }

  if (!changedPath) {
    return content;
  }

  bool issueMessage = true;
  bool useAbsoluteContent = false;
  std::ostringstream e;
  switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0076)) {
    case cmPolicies::WARN:
      e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0076) << "\n";
      break;
    case cmPolicies::OLD:
      issueMessage = false;
      break;
    case cmPolicies::REQUIRED_ALWAYS:
    case cmPolicies::REQUIRED_IF_USED:
      this->Makefile->IssueMessage(
        cmake::FATAL_ERROR,
        cmPolicies::GetRequiredPolicyError(cmPolicies::CMP0076));
      break;
    case cmPolicies::NEW: {
      issueMessage = false;
      useAbsoluteContent = true;
      break;
    }
  }

  if (issueMessage) {
    if (isInterfaceContent) {
      e << "An interface source of target \"" << tgt->GetName()
        << "\" has a relative path.";
    } else {
      e << "A private source from a directory other than that of target \""
        << tgt->GetName() << "\" has a relative path.";
    }
    this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, e.str());
  }

  return useAbsoluteContent ? absoluteContent : content;
}
