/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAddCustomTargetCommand.h"

#include <sstream>

#include "cmCustomCommandLines.h"
#include "cmGeneratorExpression.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

// cmAddCustomTargetCommand
bool cmAddCustomTargetCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  std::string const& targetName = args[0];

  // Check the target name.
  if (targetName.find_first_of("/\\") != std::string::npos) {
    std::ostringstream e;
    e << "called with invalid target name \"" << targetName
      << "\".  Target names may not contain a slash.  "
      << "Use ADD_CUSTOM_COMMAND to generate files.";
    this->SetError(e.str());
    return false;
  }

  // Accumulate one command line at a time.
  cmCustomCommandLine currentLine;

  // Save all command lines.
  cmCustomCommandLines commandLines;

  // Accumulate dependencies.
  std::vector<std::string> depends, byproducts;
  std::string working_directory;
  bool verbatim = false;
  bool uses_terminal = false;
  bool command_expand_lists = false;
  std::string comment_buffer;
  const char* comment = CM_NULLPTR;
  std::vector<std::string> sources;

  // Keep track of parser state.
  enum tdoing
  {
    doing_command,
    doing_depends,
    doing_byproducts,
    doing_working_directory,
    doing_comment,
    doing_source,
    doing_nothing
  };
  tdoing doing = doing_command;

  // Look for the ALL option.
  bool excludeFromAll = true;
  unsigned int start = 1;
  if (args.size() > 1) {
    if (args[1] == "ALL") {
      excludeFromAll = false;
      start = 2;
    }
  }

  // Parse the rest of the arguments.
  for (unsigned int j = start; j < args.size(); ++j) {
    std::string const& copy = args[j];

    if (copy == "DEPENDS") {
      doing = doing_depends;
    } else if (copy == "BYPRODUCTS") {
      doing = doing_byproducts;
    } else if (copy == "WORKING_DIRECTORY") {
      doing = doing_working_directory;
    } else if (copy == "VERBATIM") {
      doing = doing_nothing;
      verbatim = true;
    } else if (copy == "USES_TERMINAL") {
      doing = doing_nothing;
      uses_terminal = true;
    } else if (copy == "COMMAND_EXPAND_LISTS") {
      doing = doing_nothing;
      command_expand_lists = true;
    } else if (copy == "COMMENT") {
      doing = doing_comment;
    } else if (copy == "COMMAND") {
      doing = doing_command;

      // Save the current command before starting the next command.
      if (!currentLine.empty()) {
        commandLines.push_back(currentLine);
        currentLine.clear();
      }
    } else if (copy == "SOURCES") {
      doing = doing_source;
    } else {
      switch (doing) {
        case doing_working_directory:
          working_directory = copy;
          break;
        case doing_command:
          currentLine.push_back(copy);
          break;
        case doing_byproducts: {
          std::string filename;
          if (!cmSystemTools::FileIsFullPath(copy.c_str())) {
            filename = this->Makefile->GetCurrentBinaryDirectory();
            filename += "/";
          }
          filename += copy;
          cmSystemTools::ConvertToUnixSlashes(filename);
          byproducts.push_back(filename);
        } break;
        case doing_depends: {
          std::string dep = copy;
          cmSystemTools::ConvertToUnixSlashes(dep);
          depends.push_back(dep);
        } break;
        case doing_comment:
          comment_buffer = copy;
          comment = comment_buffer.c_str();
          break;
        case doing_source:
          sources.push_back(copy);
          break;
        default:
          this->SetError("Wrong syntax. Unknown type of argument.");
          return false;
      }
    }
  }

  std::string::size_type pos = targetName.find_first_of("#<>");
  if (pos != std::string::npos) {
    std::ostringstream msg;
    msg << "called with target name containing a \"" << targetName[pos]
        << "\".  This character is not allowed.";
    this->SetError(msg.str());
    return false;
  }

  // Some requirements on custom target names already exist
  // and have been checked at this point.
  // The following restrictions overlap but depend on policy CMP0037.
  bool nameOk = cmGeneratorExpression::IsValidTargetName(targetName) &&
    !cmGlobalGenerator::IsReservedTarget(targetName);
  if (nameOk) {
    nameOk = targetName.find(':') == std::string::npos;
  }
  if (!nameOk) {
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    std::ostringstream e;
    bool issueMessage = false;
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0037)) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0037) << "\n";
        issueMessage = true;
      case cmPolicies::OLD:
        break;
      case cmPolicies::NEW:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS:
        issueMessage = true;
        messageType = cmake::FATAL_ERROR;
    }
    if (issueMessage) {
      /* clang-format off */
      e << "The target name \"" << targetName <<
          "\" is reserved or not valid for certain "
          "CMake features, such as generator expressions, and may result "
          "in undefined behavior.";
      /* clang-format on */
      this->Makefile->IssueMessage(messageType, e.str());

      if (messageType == cmake::FATAL_ERROR) {
        return false;
      }
    }
  }

  // Store the last command line finished.
  if (!currentLine.empty()) {
    commandLines.push_back(currentLine);
    currentLine.clear();
  }

  // Enforce name uniqueness.
  {
    std::string msg;
    if (!this->Makefile->EnforceUniqueName(targetName, msg, true)) {
      this->SetError(msg);
      return false;
    }
  }

  // Convert working directory to a full path.
  if (!working_directory.empty()) {
    const char* build_dir = this->Makefile->GetCurrentBinaryDirectory();
    working_directory =
      cmSystemTools::CollapseFullPath(working_directory, build_dir);
  }

  if (commandLines.empty() && !byproducts.empty()) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR,
      "BYPRODUCTS may not be specified without any COMMAND");
    return true;
  }
  if (commandLines.empty() && uses_terminal) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR,
      "USES_TERMINAL may not be specified without any COMMAND");
    return true;
  }
  if (commandLines.empty() && command_expand_lists) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR,
      "COMMAND_EXPAND_LISTS may not be specified without any COMMAND");
    return true;
  }

  // Add the utility target to the makefile.
  bool escapeOldStyle = !verbatim;
  cmTarget* target = this->Makefile->AddUtilityCommand(
    targetName, excludeFromAll, working_directory.c_str(), byproducts, depends,
    commandLines, escapeOldStyle, comment, uses_terminal,
    command_expand_lists);

  // Add additional user-specified source files to the target.
  target->AddSources(sources);

  return true;
}
