/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExecProgramCommand.h"

#include "cmsys/Process.h"
#include <stdio.h>

#include "cmMakefile.h"
#include "cmProcessOutput.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmExecProgramCommand
bool cmExecProgramCommand::InitialPass(std::vector<std::string> const& args,
                                       cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  std::string arguments;
  bool doingargs = false;
  int count = 0;
  std::string output_variable;
  bool haveoutput_variable = false;
  std::string return_variable;
  bool havereturn_variable = false;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "OUTPUT_VARIABLE") {
      count++;
      doingargs = false;
      havereturn_variable = false;
      haveoutput_variable = true;
    } else if (haveoutput_variable) {
      if (!output_variable.empty()) {
        this->SetError("called with incorrect number of arguments");
        return false;
      }
      output_variable = args[i];
      haveoutput_variable = false;
      count++;
    } else if (args[i] == "RETURN_VALUE") {
      count++;
      doingargs = false;
      haveoutput_variable = false;
      havereturn_variable = true;
    } else if (havereturn_variable) {
      if (!return_variable.empty()) {
        this->SetError("called with incorrect number of arguments");
        return false;
      }
      return_variable = args[i];
      havereturn_variable = false;
      count++;
    } else if (args[i] == "ARGS") {
      count++;
      havereturn_variable = false;
      haveoutput_variable = false;
      doingargs = true;
    } else if (doingargs) {
      arguments += args[i];
      arguments += " ";
      count++;
    }
  }

  std::string command;
  if (!arguments.empty()) {
    command = cmSystemTools::ConvertToRunCommandPath(args[0].c_str());
    command += " ";
    command += arguments;
  } else {
    command = args[0];
  }
  bool verbose = true;
  if (!output_variable.empty()) {
    verbose = false;
  }
  int retVal = 0;
  std::string output;
  bool result = true;
  if (args.size() - count == 2) {
    cmSystemTools::MakeDirectory(args[1].c_str());
    result = cmExecProgramCommand::RunCommand(command.c_str(), output, retVal,
                                              args[1].c_str(), verbose);
  } else {
    result = cmExecProgramCommand::RunCommand(command.c_str(), output, retVal,
                                              CM_NULLPTR, verbose);
  }
  if (!result) {
    retVal = -1;
  }

  if (!output_variable.empty()) {
    std::string::size_type first = output.find_first_not_of(" \n\t\r");
    std::string::size_type last = output.find_last_not_of(" \n\t\r");
    if (first == std::string::npos) {
      first = 0;
    }
    if (last == std::string::npos) {
      last = output.size() - 1;
    }

    std::string coutput = std::string(output, first, last - first + 1);
    this->Makefile->AddDefinition(output_variable, coutput.c_str());
  }

  if (!return_variable.empty()) {
    char buffer[100];
    sprintf(buffer, "%d", retVal);
    this->Makefile->AddDefinition(return_variable, buffer);
  }

  return true;
}

bool cmExecProgramCommand::RunCommand(const char* command, std::string& output,
                                      int& retVal, const char* dir,
                                      bool verbose, Encoding encoding)
{
  if (cmSystemTools::GetRunCommandOutput()) {
    verbose = false;
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  // if the command does not start with a quote, then
  // try to find the program, and if the program can not be
  // found use system to run the command as it must be a built in
  // shell command like echo or dir
  int count = 0;
  std::string shortCmd;
  if (command[0] == '\"') {
    // count the number of quotes
    for (const char* s = command; *s != 0; ++s) {
      if (*s == '\"') {
        count++;
        if (count > 2) {
          break;
        }
      }
    }
    // if there are more than two double quotes use
    // GetShortPathName, the cmd.exe program in windows which
    // is used by system fails to execute if there are more than
    // one set of quotes in the arguments
    if (count > 2) {
      cmsys::RegularExpression quoted("^\"([^\"]*)\"[ \t](.*)");
      if (quoted.find(command)) {
        std::string cmd = quoted.match(1);
        std::string args = quoted.match(2);
        if (!cmSystemTools::FileExists(cmd.c_str())) {
          shortCmd = cmd;
        } else if (!cmSystemTools::GetShortPath(cmd.c_str(), shortCmd)) {
          cmSystemTools::Error("GetShortPath failed for ", cmd.c_str());
          return false;
        }
        shortCmd += " ";
        shortCmd += args;

        command = shortCmd.c_str();
      } else {
        cmSystemTools::Error("Could not parse command line with quotes ",
                             command);
      }
    }
  }
#endif

  // Allocate a process instance.
  cmsysProcess* cp = cmsysProcess_New();
  if (!cp) {
    cmSystemTools::Error("Error allocating process instance.");
    return false;
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  if (dir) {
    cmsysProcess_SetWorkingDirectory(cp, dir);
  }
  if (cmSystemTools::GetRunCommandHideConsole()) {
    cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  }
  cmsysProcess_SetOption(cp, cmsysProcess_Option_Verbatim, 1);
  const char* cmd[] = { command, 0 };
  cmsysProcess_SetCommand(cp, cmd);
#else
  std::string commandInDir;
  if (dir) {
    commandInDir = "cd \"";
    commandInDir += dir;
    commandInDir += "\" && ";
    commandInDir += command;
  } else {
    commandInDir = command;
  }
#ifndef __VMS
  commandInDir += " 2>&1";
#endif
  command = commandInDir.c_str();
  if (verbose) {
    cmSystemTools::Stdout("running ");
    cmSystemTools::Stdout(command);
    cmSystemTools::Stdout("\n");
  }
  fflush(stdout);
  fflush(stderr);
  const char* cmd[] = { "/bin/sh", "-c", command, CM_NULLPTR };
  cmsysProcess_SetCommand(cp, cmd);
#endif

  cmsysProcess_Execute(cp);

  // Read the process output.
  int length;
  char* data;
  int p;
  cmProcessOutput processOutput(encoding);
  std::string strdata;
  while ((p = cmsysProcess_WaitForData(cp, &data, &length, CM_NULLPTR), p)) {
    if (p == cmsysProcess_Pipe_STDOUT || p == cmsysProcess_Pipe_STDERR) {
      if (verbose) {
        processOutput.DecodeText(data, length, strdata);
        cmSystemTools::Stdout(strdata.c_str(), strdata.size());
      }
      output.append(data, length);
    }
  }

  if (verbose) {
    processOutput.DecodeText(std::string(), strdata);
    if (!strdata.empty()) {
      cmSystemTools::Stdout(strdata.c_str(), strdata.size());
    }
  }

  // All output has been read.  Wait for the process to exit.
  cmsysProcess_WaitForExit(cp, CM_NULLPTR);
  processOutput.DecodeText(output, output);

  // Check the result of running the process.
  std::string msg;
  switch (cmsysProcess_GetState(cp)) {
    case cmsysProcess_State_Exited:
      retVal = cmsysProcess_GetExitValue(cp);
      break;
    case cmsysProcess_State_Exception:
      retVal = -1;
      msg += "\nProcess terminated due to: ";
      msg += cmsysProcess_GetExceptionString(cp);
      break;
    case cmsysProcess_State_Error:
      retVal = -1;
      msg += "\nProcess failed because: ";
      msg += cmsysProcess_GetErrorString(cp);
      break;
    case cmsysProcess_State_Expired:
      retVal = -1;
      msg += "\nProcess terminated due to timeout.";
      break;
  }
  if (!msg.empty()) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    // Old Windows process execution printed this info.
    msg += "\n\nfor command: ";
    msg += command;
    if (dir) {
      msg += "\nin dir: ";
      msg += dir;
    }
    msg += "\n";
    if (verbose) {
      cmSystemTools::Stdout(msg.c_str());
    }
    output += msg;
#else
    // Old UNIX process execution only put message in output.
    output += msg;
#endif
  }

  // Delete the process instance.
  cmsysProcess_Delete(cp);

  return true;
}
