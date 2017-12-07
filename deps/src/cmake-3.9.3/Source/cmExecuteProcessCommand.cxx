/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExecuteProcessCommand.h"

#include "cmsys/Process.h"
#include <ctype.h> /* isspace */
#include <sstream>
#include <stdio.h>

#include "cmMakefile.h"
#include "cmProcessOutput.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

static bool cmExecuteProcessCommandIsWhitespace(char c)
{
  return (isspace((int)c) || c == '\n' || c == '\r');
}

void cmExecuteProcessCommandFixText(std::vector<char>& output,
                                    bool strip_trailing_whitespace);
void cmExecuteProcessCommandAppend(std::vector<char>& output, const char* data,
                                   int length);

// cmExecuteProcessCommand
bool cmExecuteProcessCommand::InitialPass(std::vector<std::string> const& args,
                                          cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  std::vector<std::vector<const char*> > cmds;
  std::string arguments;
  bool doing_command = false;
  size_t command_index = 0;
  bool output_quiet = false;
  bool error_quiet = false;
  bool output_strip_trailing_whitespace = false;
  bool error_strip_trailing_whitespace = false;
  std::string timeout_string;
  std::string input_file;
  std::string output_file;
  std::string error_file;
  std::string output_variable;
  std::string error_variable;
  std::string result_variable;
  std::string working_directory;
  cmProcessOutput::Encoding encoding = cmProcessOutput::None;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "COMMAND") {
      doing_command = true;
      command_index = cmds.size();
      cmds.push_back(std::vector<const char*>());
    } else if (args[i] == "OUTPUT_VARIABLE") {
      doing_command = false;
      if (++i < args.size()) {
        output_variable = args[i];
      } else {
        this->SetError(" called with no value for OUTPUT_VARIABLE.");
        return false;
      }
    } else if (args[i] == "ERROR_VARIABLE") {
      doing_command = false;
      if (++i < args.size()) {
        error_variable = args[i];
      } else {
        this->SetError(" called with no value for ERROR_VARIABLE.");
        return false;
      }
    } else if (args[i] == "RESULT_VARIABLE") {
      doing_command = false;
      if (++i < args.size()) {
        result_variable = args[i];
      } else {
        this->SetError(" called with no value for RESULT_VARIABLE.");
        return false;
      }
    } else if (args[i] == "WORKING_DIRECTORY") {
      doing_command = false;
      if (++i < args.size()) {
        working_directory = args[i];
      } else {
        this->SetError(" called with no value for WORKING_DIRECTORY.");
        return false;
      }
    } else if (args[i] == "INPUT_FILE") {
      doing_command = false;
      if (++i < args.size()) {
        input_file = args[i];
      } else {
        this->SetError(" called with no value for INPUT_FILE.");
        return false;
      }
    } else if (args[i] == "OUTPUT_FILE") {
      doing_command = false;
      if (++i < args.size()) {
        output_file = args[i];
      } else {
        this->SetError(" called with no value for OUTPUT_FILE.");
        return false;
      }
    } else if (args[i] == "ERROR_FILE") {
      doing_command = false;
      if (++i < args.size()) {
        error_file = args[i];
      } else {
        this->SetError(" called with no value for ERROR_FILE.");
        return false;
      }
    } else if (args[i] == "TIMEOUT") {
      doing_command = false;
      if (++i < args.size()) {
        timeout_string = args[i];
      } else {
        this->SetError(" called with no value for TIMEOUT.");
        return false;
      }
    } else if (args[i] == "OUTPUT_QUIET") {
      doing_command = false;
      output_quiet = true;
    } else if (args[i] == "ERROR_QUIET") {
      doing_command = false;
      error_quiet = true;
    } else if (args[i] == "OUTPUT_STRIP_TRAILING_WHITESPACE") {
      doing_command = false;
      output_strip_trailing_whitespace = true;
    } else if (args[i] == "ERROR_STRIP_TRAILING_WHITESPACE") {
      doing_command = false;
      error_strip_trailing_whitespace = true;
    } else if (args[i] == "ENCODING") {
      doing_command = false;
      if (++i < args.size()) {
        encoding = cmProcessOutput::FindEncoding(args[i]);
      } else {
        this->SetError(" called with no value for ENCODING.");
        return false;
      }
    } else if (doing_command) {
      cmds[command_index].push_back(args[i].c_str());
    } else {
      std::ostringstream e;
      e << " given unknown argument \"" << args[i] << "\".";
      this->SetError(e.str());
      return false;
    }
  }

  if (!this->Makefile->CanIWriteThisFile(output_file.c_str())) {
    std::string e = "attempted to output into a file: " + output_file +
      " into a source directory.";
    this->SetError(e);
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }

  // Check for commands given.
  if (cmds.empty()) {
    this->SetError(" called with no COMMAND argument.");
    return false;
  }
  for (unsigned int i = 0; i < cmds.size(); ++i) {
    if (cmds[i].empty()) {
      this->SetError(" given COMMAND argument with no value.");
      return false;
    }
    // Add the null terminating pointer to the command argument list.
    cmds[i].push_back(CM_NULLPTR);
  }

  // Parse the timeout string.
  double timeout = -1;
  if (!timeout_string.empty()) {
    if (sscanf(timeout_string.c_str(), "%lg", &timeout) != 1) {
      this->SetError(" called with TIMEOUT value that could not be parsed.");
      return false;
    }
  }

  // Create a process instance.
  cmsysProcess* cp = cmsysProcess_New();

  // Set the command sequence.
  for (unsigned int i = 0; i < cmds.size(); ++i) {
    cmsysProcess_AddCommand(cp, &*cmds[i].begin());
  }

  // Set the process working directory.
  if (!working_directory.empty()) {
    cmsysProcess_SetWorkingDirectory(cp, working_directory.c_str());
  }

  // Always hide the process window.
  cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);

  // Check the output variables.
  bool merge_output = false;
  if (!input_file.empty()) {
    cmsysProcess_SetPipeFile(cp, cmsysProcess_Pipe_STDIN, input_file.c_str());
  }
  if (!output_file.empty()) {
    cmsysProcess_SetPipeFile(cp, cmsysProcess_Pipe_STDOUT,
                             output_file.c_str());
  }
  if (!error_file.empty()) {
    if (error_file == output_file) {
      merge_output = true;
    } else {
      cmsysProcess_SetPipeFile(cp, cmsysProcess_Pipe_STDERR,
                               error_file.c_str());
    }
  }
  if (!output_variable.empty() && output_variable == error_variable) {
    merge_output = true;
  }
  if (merge_output) {
    cmsysProcess_SetOption(cp, cmsysProcess_Option_MergeOutput, 1);
  }

  // Set the timeout if any.
  if (timeout >= 0) {
    cmsysProcess_SetTimeout(cp, timeout);
  }

  // Start the process.
  cmsysProcess_Execute(cp);

  // Read the process output.
  std::vector<char> tempOutput;
  std::vector<char> tempError;
  int length;
  char* data;
  int p;
  cmProcessOutput processOutput(encoding);
  std::string strdata;
  while ((p = cmsysProcess_WaitForData(cp, &data, &length, CM_NULLPTR), p)) {
    // Put the output in the right place.
    if (p == cmsysProcess_Pipe_STDOUT && !output_quiet) {
      if (output_variable.empty()) {
        processOutput.DecodeText(data, length, strdata, 1);
        cmSystemTools::Stdout(strdata.c_str(), strdata.size());
      } else {
        cmExecuteProcessCommandAppend(tempOutput, data, length);
      }
    } else if (p == cmsysProcess_Pipe_STDERR && !error_quiet) {
      if (error_variable.empty()) {
        processOutput.DecodeText(data, length, strdata, 2);
        cmSystemTools::Stderr(strdata.c_str(), strdata.size());
      } else {
        cmExecuteProcessCommandAppend(tempError, data, length);
      }
    }
  }
  if (!output_quiet && output_variable.empty()) {
    processOutput.DecodeText(std::string(), strdata, 1);
    if (!strdata.empty()) {
      cmSystemTools::Stdout(strdata.c_str(), strdata.size());
    }
  }
  if (!error_quiet && error_variable.empty()) {
    processOutput.DecodeText(std::string(), strdata, 2);
    if (!strdata.empty()) {
      cmSystemTools::Stderr(strdata.c_str(), strdata.size());
    }
  }

  // All output has been read.  Wait for the process to exit.
  cmsysProcess_WaitForExit(cp, CM_NULLPTR);
  processOutput.DecodeText(tempOutput, tempOutput);
  processOutput.DecodeText(tempError, tempError);

  // Fix the text in the output strings.
  cmExecuteProcessCommandFixText(tempOutput, output_strip_trailing_whitespace);
  cmExecuteProcessCommandFixText(tempError, error_strip_trailing_whitespace);

  // Store the output obtained.
  if (!output_variable.empty() && !tempOutput.empty()) {
    this->Makefile->AddDefinition(output_variable, &*tempOutput.begin());
  }
  if (!merge_output && !error_variable.empty() && !tempError.empty()) {
    this->Makefile->AddDefinition(error_variable, &*tempError.begin());
  }

  // Store the result of running the process.
  if (!result_variable.empty()) {
    switch (cmsysProcess_GetState(cp)) {
      case cmsysProcess_State_Exited: {
        int v = cmsysProcess_GetExitValue(cp);
        char buf[100];
        sprintf(buf, "%d", v);
        this->Makefile->AddDefinition(result_variable, buf);
      } break;
      case cmsysProcess_State_Exception:
        this->Makefile->AddDefinition(result_variable,
                                      cmsysProcess_GetExceptionString(cp));
        break;
      case cmsysProcess_State_Error:
        this->Makefile->AddDefinition(result_variable,
                                      cmsysProcess_GetErrorString(cp));
        break;
      case cmsysProcess_State_Expired:
        this->Makefile->AddDefinition(result_variable,
                                      "Process terminated due to timeout");
        break;
    }
  }

  // Delete the process instance.
  cmsysProcess_Delete(cp);

  return true;
}

void cmExecuteProcessCommandFixText(std::vector<char>& output,
                                    bool strip_trailing_whitespace)
{
  // Remove \0 characters and the \r part of \r\n pairs.
  unsigned int in_index = 0;
  unsigned int out_index = 0;
  while (in_index < output.size()) {
    char c = output[in_index++];
    if ((c != '\r' ||
         !(in_index < output.size() && output[in_index] == '\n')) &&
        c != '\0') {
      output[out_index++] = c;
    }
  }

  // Remove trailing whitespace if requested.
  if (strip_trailing_whitespace) {
    while (out_index > 0 &&
           cmExecuteProcessCommandIsWhitespace(output[out_index - 1])) {
      --out_index;
    }
  }

  // Shrink the vector to the size needed.
  output.resize(out_index);

  // Put a terminator on the text string.
  output.push_back('\0');
}

void cmExecuteProcessCommandAppend(std::vector<char>& output, const char* data,
                                   int length)
{
#if defined(__APPLE__)
  // HACK on Apple to work around bug with inserting at the
  // end of an empty vector.  This resulted in random failures
  // that were hard to reproduce.
  if (output.empty() && length > 0) {
    output.push_back(data[0]);
    ++data;
    --length;
  }
#endif
  output.insert(output.end(), data, data + length);
}
