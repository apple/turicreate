/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAddCustomCommandCommand.h"

#include <sstream>

#include "cmCustomCommand.h"
#include "cmCustomCommandLines.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

// cmAddCustomCommandCommand
bool cmAddCustomCommandCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  /* Let's complain at the end of this function about the lack of a particular
     arg. For the moment, let's say that COMMAND, and either TARGET or SOURCE
     are required.
  */
  if (args.size() < 4) {
    this->SetError("called with wrong number of arguments.");
    return false;
  }

  std::string source, target, main_dependency, working, depfile;
  std::string comment_buffer;
  const char* comment = CM_NULLPTR;
  std::vector<std::string> depends, outputs, output, byproducts;
  bool verbatim = false;
  bool append = false;
  bool uses_terminal = false;
  bool command_expand_lists = false;
  std::string implicit_depends_lang;
  cmCustomCommand::ImplicitDependsList implicit_depends;

  // Accumulate one command line at a time.
  cmCustomCommandLine currentLine;

  // Save all command lines.
  cmCustomCommandLines commandLines;

  cmTarget::CustomCommandType cctype = cmTarget::POST_BUILD;

  enum tdoing
  {
    doing_source,
    doing_command,
    doing_target,
    doing_depends,
    doing_implicit_depends_lang,
    doing_implicit_depends_file,
    doing_main_dependency,
    doing_output,
    doing_outputs,
    doing_byproducts,
    doing_comment,
    doing_working_directory,
    doing_depfile,
    doing_nothing
  };

  tdoing doing = doing_nothing;

  for (unsigned int j = 0; j < args.size(); ++j) {
    std::string const& copy = args[j];

    if (copy == "SOURCE") {
      doing = doing_source;
    } else if (copy == "COMMAND") {
      doing = doing_command;

      // Save the current command before starting the next command.
      if (!currentLine.empty()) {
        commandLines.push_back(currentLine);
        currentLine.clear();
      }
    } else if (copy == "PRE_BUILD") {
      cctype = cmTarget::PRE_BUILD;
    } else if (copy == "PRE_LINK") {
      cctype = cmTarget::PRE_LINK;
    } else if (copy == "POST_BUILD") {
      cctype = cmTarget::POST_BUILD;
    } else if (copy == "VERBATIM") {
      verbatim = true;
    } else if (copy == "APPEND") {
      append = true;
    } else if (copy == "USES_TERMINAL") {
      uses_terminal = true;
    } else if (copy == "COMMAND_EXPAND_LISTS") {
      command_expand_lists = true;
    } else if (copy == "TARGET") {
      doing = doing_target;
    } else if (copy == "ARGS") {
      // Ignore this old keyword.
    } else if (copy == "DEPENDS") {
      doing = doing_depends;
    } else if (copy == "OUTPUTS") {
      doing = doing_outputs;
    } else if (copy == "OUTPUT") {
      doing = doing_output;
    } else if (copy == "BYPRODUCTS") {
      doing = doing_byproducts;
    } else if (copy == "WORKING_DIRECTORY") {
      doing = doing_working_directory;
    } else if (copy == "MAIN_DEPENDENCY") {
      doing = doing_main_dependency;
    } else if (copy == "IMPLICIT_DEPENDS") {
      doing = doing_implicit_depends_lang;
    } else if (copy == "COMMENT") {
      doing = doing_comment;
    } else if (copy == "DEPFILE") {
      doing = doing_depfile;
      if (this->Makefile->GetGlobalGenerator()->GetName() != "Ninja") {
        this->SetError("Option DEPFILE not supported by " +
                       this->Makefile->GetGlobalGenerator()->GetName());
        return false;
      }
    } else {
      std::string filename;
      switch (doing) {
        case doing_output:
        case doing_outputs:
        case doing_byproducts:
          if (!cmSystemTools::FileIsFullPath(copy.c_str())) {
            // This is an output to be generated, so it should be
            // under the build tree.  CMake 2.4 placed this under the
            // source tree.  However the only case that this change
            // will break is when someone writes
            //
            //   add_custom_command(OUTPUT out.txt ...)
            //
            // and later references "${CMAKE_CURRENT_SOURCE_DIR}/out.txt".
            // This is fairly obscure so we can wait for someone to
            // complain.
            filename = this->Makefile->GetCurrentBinaryDirectory();
            filename += "/";
          }
          filename += copy;
          cmSystemTools::ConvertToUnixSlashes(filename);
          break;
        case doing_source:
        // We do not want to convert the argument to SOURCE because
        // that option is only available for backward compatibility.
        // Old-style use of this command may use the SOURCE==TARGET
        // trick which we must preserve.  If we convert the source
        // to a full path then it will no longer equal the target.
        default:
          break;
      }

      if (cmSystemTools::FileIsFullPath(filename.c_str())) {
        filename = cmSystemTools::CollapseFullPath(filename);
      }
      switch (doing) {
        case doing_depfile:
          depfile = copy;
          break;
        case doing_working_directory:
          working = copy;
          break;
        case doing_source:
          source = copy;
          break;
        case doing_output:
          output.push_back(filename);
          break;
        case doing_main_dependency:
          main_dependency = copy;
          break;
        case doing_implicit_depends_lang:
          implicit_depends_lang = copy;
          doing = doing_implicit_depends_file;
          break;
        case doing_implicit_depends_file: {
          // An implicit dependency starting point is also an
          // explicit dependency.
          std::string dep = copy;
          cmSystemTools::ConvertToUnixSlashes(dep);
          depends.push_back(dep);

          // Add the implicit dependency language and file.
          cmCustomCommand::ImplicitDependsPair entry(implicit_depends_lang,
                                                     dep);
          implicit_depends.push_back(entry);

          // Switch back to looking for a language.
          doing = doing_implicit_depends_lang;
        } break;
        case doing_command:
          currentLine.push_back(copy);
          break;
        case doing_target:
          target = copy;
          break;
        case doing_depends: {
          std::string dep = copy;
          cmSystemTools::ConvertToUnixSlashes(dep);
          depends.push_back(dep);
        } break;
        case doing_outputs:
          outputs.push_back(filename);
          break;
        case doing_byproducts:
          byproducts.push_back(filename);
          break;
        case doing_comment:
          comment_buffer = copy;
          comment = comment_buffer.c_str();
          break;
        default:
          this->SetError("Wrong syntax. Unknown type of argument.");
          return false;
      }
    }
  }

  // Store the last command line finished.
  if (!currentLine.empty()) {
    commandLines.push_back(currentLine);
    currentLine.clear();
  }

  // At this point we could complain about the lack of arguments.  For
  // the moment, let's say that COMMAND, TARGET are always required.
  if (output.empty() && target.empty()) {
    this->SetError("Wrong syntax. A TARGET or OUTPUT must be specified.");
    return false;
  }

  if (source.empty() && !target.empty() && !output.empty()) {
    this->SetError(
      "Wrong syntax. A TARGET and OUTPUT can not both be specified.");
    return false;
  }
  if (append && output.empty()) {
    this->SetError("given APPEND option with no OUTPUT.");
    return false;
  }

  // Make sure the output names and locations are safe.
  if (!this->CheckOutputs(output) || !this->CheckOutputs(outputs) ||
      !this->CheckOutputs(byproducts)) {
    return false;
  }

  // Check for an append request.
  if (append) {
    // Lookup an existing command.
    if (cmSourceFile* sf =
          this->Makefile->GetSourceFileWithOutput(output[0])) {
      if (cmCustomCommand* cc = sf->GetCustomCommand()) {
        cc->AppendCommands(commandLines);
        cc->AppendDepends(depends);
        cc->AppendImplicitDepends(implicit_depends);
        return true;
      }
    }

    // No command for this output exists.
    std::ostringstream e;
    e << "given APPEND option with output\n\"" << output[0]
      << "\"\nwhich is not already a custom command output.";
    this->SetError(e.str());
    return false;
  }

  // Convert working directory to a full path.
  if (!working.empty()) {
    const char* build_dir = this->Makefile->GetCurrentBinaryDirectory();
    working = cmSystemTools::CollapseFullPath(working, build_dir);
  }

  // Choose which mode of the command to use.
  bool escapeOldStyle = !verbatim;
  if (source.empty() && output.empty()) {
    // Source is empty, use the target.
    std::vector<std::string> no_depends;
    this->Makefile->AddCustomCommandToTarget(
      target, byproducts, no_depends, commandLines, cctype, comment,
      working.c_str(), escapeOldStyle, uses_terminal, depfile,
      command_expand_lists);
  } else if (target.empty()) {
    // Target is empty, use the output.
    this->Makefile->AddCustomCommandToOutput(
      output, byproducts, depends, main_dependency, commandLines, comment,
      working.c_str(), false, escapeOldStyle, uses_terminal,
      command_expand_lists, depfile);

    // Add implicit dependency scanning requests if any were given.
    if (!implicit_depends.empty()) {
      bool okay = false;
      if (cmSourceFile* sf =
            this->Makefile->GetSourceFileWithOutput(output[0])) {
        if (cmCustomCommand* cc = sf->GetCustomCommand()) {
          okay = true;
          cc->SetImplicitDepends(implicit_depends);
        }
      }
      if (!okay) {
        std::ostringstream e;
        e << "could not locate source file with a custom command producing \""
          << output[0] << "\" even though this command tried to create it!";
        this->SetError(e.str());
        return false;
      }
    }
  } else if (!byproducts.empty()) {
    this->SetError("BYPRODUCTS may not be specified with SOURCE signatures");
    return false;
  } else if (uses_terminal) {
    this->SetError("USES_TERMINAL may not be used with SOURCE signatures");
    return false;
  } else {
    bool issueMessage = true;
    std::ostringstream e;
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0050)) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0050) << "\n";
        break;
      case cmPolicies::OLD:
        issueMessage = false;
        break;
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::NEW:
        messageType = cmake::FATAL_ERROR;
        break;
    }

    if (issueMessage) {
      e << "The SOURCE signatures of add_custom_command are no longer "
           "supported.";
      this->Makefile->IssueMessage(messageType, e.str());
      if (messageType == cmake::FATAL_ERROR) {
        return false;
      }
    }

    // Use the old-style mode for backward compatibility.
    this->Makefile->AddCustomCommandOldStyle(target, outputs, depends, source,
                                             commandLines, comment);
  }

  return true;
}

bool cmAddCustomCommandCommand::CheckOutputs(
  const std::vector<std::string>& outputs)
{
  for (std::vector<std::string>::const_iterator o = outputs.begin();
       o != outputs.end(); ++o) {
    // Make sure the file will not be generated into the source
    // directory during an out of source build.
    if (!this->Makefile->CanIWriteThisFile(o->c_str())) {
      std::string e = "attempted to have a file \"" + *o +
        "\" in a source directory as an output of custom command.";
      this->SetError(e);
      cmSystemTools::SetFatalErrorOccured();
      return false;
    }

    // Make sure the output file name has no invalid characters.
    std::string::size_type pos = o->find_first_of("#<>");
    if (pos != std::string::npos) {
      std::ostringstream msg;
      msg << "called with OUTPUT containing a \"" << (*o)[pos]
          << "\".  This character is not allowed.";
      this->SetError(msg.str());
      return false;
    }
  }
  return true;
}
