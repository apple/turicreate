/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestScriptHandler.h"

#include "cmsys/Directory.hxx"
#include "cmsys/Process.h"
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

#include "cmCTest.h"
#include "cmCTestBuildCommand.h"
#include "cmCTestCommand.h"
#include "cmCTestConfigureCommand.h"
#include "cmCTestCoverageCommand.h"
#include "cmCTestEmptyBinaryDirectoryCommand.h"
#include "cmCTestMemCheckCommand.h"
#include "cmCTestReadCustomFilesCommand.h"
#include "cmCTestRunScriptCommand.h"
#include "cmCTestSleepCommand.h"
#include "cmCTestStartCommand.h"
#include "cmCTestSubmitCommand.h"
#include "cmCTestTestCommand.h"
#include "cmCTestUpdateCommand.h"
#include "cmCTestUploadCommand.h"
#include "cmFunctionBlocker.h"
#include "cmGeneratedFileStream.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmake.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

class cmExecutionStatus;
struct cmListFileFunction;

#define CTEST_INITIAL_CMAKE_OUTPUT_FILE_NAME "CTestInitialCMakeOutput.log"

// used to keep elapsed time up to date
class cmCTestScriptFunctionBlocker : public cmFunctionBlocker
{
public:
  cmCTestScriptFunctionBlocker() {}
  ~cmCTestScriptFunctionBlocker() CM_OVERRIDE {}
  bool IsFunctionBlocked(const cmListFileFunction& lff, cmMakefile& mf,
                         cmExecutionStatus& /*status*/) CM_OVERRIDE;
  // virtual bool ShouldRemove(const cmListFileFunction& lff, cmMakefile &mf);
  // virtual void ScopeEnded(cmMakefile &mf);

  cmCTestScriptHandler* CTestScriptHandler;
};

// simply update the time and don't block anything
bool cmCTestScriptFunctionBlocker::IsFunctionBlocked(
  const cmListFileFunction& /*lff*/, cmMakefile& /*mf*/,
  cmExecutionStatus& /*status*/)
{
  this->CTestScriptHandler->UpdateElapsedTime();
  return false;
}

cmCTestScriptHandler::cmCTestScriptHandler()
{
  this->Backup = false;
  this->EmptyBinDir = false;
  this->EmptyBinDirOnce = false;
  this->Makefile = CM_NULLPTR;
  this->CMake = CM_NULLPTR;
  this->GlobalGenerator = CM_NULLPTR;

  this->ScriptStartTime = 0;

  // the *60 is becuase the settings are in minutes but GetTime is seconds
  this->MinimumInterval = 30 * 60;
  this->ContinuousDuration = -1;
}

void cmCTestScriptHandler::Initialize()
{
  this->Superclass::Initialize();
  this->Backup = false;
  this->EmptyBinDir = false;
  this->EmptyBinDirOnce = false;

  this->SourceDir = "";
  this->BinaryDir = "";
  this->BackupSourceDir = "";
  this->BackupBinaryDir = "";
  this->CTestRoot = "";
  this->CVSCheckOut = "";
  this->CTestCmd = "";
  this->UpdateCmd = "";
  this->CTestEnv = "";
  this->InitialCache = "";
  this->CMakeCmd = "";
  this->CMOutFile = "";
  this->ExtraUpdates.clear();

  this->MinimumInterval = 20 * 60;
  this->ContinuousDuration = -1;

  // what time in seconds did this script start running
  this->ScriptStartTime = 0;

  delete this->Makefile;
  this->Makefile = CM_NULLPTR;

  delete this->GlobalGenerator;
  this->GlobalGenerator = CM_NULLPTR;

  delete this->CMake;
}

cmCTestScriptHandler::~cmCTestScriptHandler()
{
  delete this->Makefile;
  delete this->GlobalGenerator;
  delete this->CMake;
}

// just adds an argument to the vector
void cmCTestScriptHandler::AddConfigurationScript(const char* script,
                                                  bool pscope)
{
  this->ConfigurationScripts.push_back(script);
  this->ScriptProcessScope.push_back(pscope);
}

// the generic entry point for handling scripts, this routine will run all
// the scripts provides a -S arguments
int cmCTestScriptHandler::ProcessHandler()
{
  int res = 0;
  for (size_t i = 0; i < this->ConfigurationScripts.size(); ++i) {
    // for each script run it
    res |= this->RunConfigurationScript(
      cmSystemTools::CollapseFullPath(this->ConfigurationScripts[i]),
      this->ScriptProcessScope[i]);
  }
  if (res) {
    return -1;
  }
  return 0;
}

void cmCTestScriptHandler::UpdateElapsedTime()
{
  if (this->Makefile) {
    // set the current elapsed time
    char timeString[20];
    int itime = static_cast<unsigned int>(cmSystemTools::GetTime() -
                                          this->ScriptStartTime);
    sprintf(timeString, "%i", itime);
    this->Makefile->AddDefinition("CTEST_ELAPSED_TIME", timeString);
  }
}

void cmCTestScriptHandler::AddCTestCommand(std::string const& name,
                                           cmCTestCommand* command)
{
  command->CTest = this->CTest;
  command->CTestScriptHandler = this;
  this->CMake->GetState()->AddBuiltinCommand(name, command);
}

int cmCTestScriptHandler::ExecuteScript(const std::string& total_script_arg)
{
  // execute the script passing in the arguments to the script as well as the
  // arguments from this invocation of cmake
  std::vector<const char*> argv;
  argv.push_back(cmSystemTools::GetCTestCommand().c_str());
  argv.push_back("-SR");
  argv.push_back(total_script_arg.c_str());

  cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT, "Executable for CTest is: "
               << cmSystemTools::GetCTestCommand() << "\n");

  // now pass through all the other arguments
  std::vector<std::string>& initArgs =
    this->CTest->GetInitialCommandLineArguments();
  //*** need to make sure this does not have the current script ***
  for (size_t i = 1; i < initArgs.size(); ++i) {
    argv.push_back(initArgs[i].c_str());
  }
  argv.push_back(CM_NULLPTR);

  // Now create process object
  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*argv.begin());
  // cmsysProcess_SetWorkingDirectory(cp, dir);
  cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  // cmsysProcess_SetTimeout(cp, timeout);
  cmsysProcess_Execute(cp);

  std::vector<char> out;
  std::vector<char> err;
  std::string line;
  int pipe = cmSystemTools::WaitForLine(cp, line, 100.0, out, err);
  while (pipe != cmsysProcess_Pipe_None) {
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT, "Output: " << line
                                                               << "\n");
    if (pipe == cmsysProcess_Pipe_STDERR) {
      cmCTestLog(this->CTest, ERROR_MESSAGE, line << "\n");
    } else if (pipe == cmsysProcess_Pipe_STDOUT) {
      cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT, line << "\n");
    }
    pipe = cmSystemTools::WaitForLine(cp, line, 100, out, err);
  }

  // Properly handle output of the build command
  cmsysProcess_WaitForExit(cp, CM_NULLPTR);
  int result = cmsysProcess_GetState(cp);
  int retVal = 0;
  bool failed = false;
  if (result == cmsysProcess_State_Exited) {
    retVal = cmsysProcess_GetExitValue(cp);
  } else if (result == cmsysProcess_State_Exception) {
    retVal = cmsysProcess_GetExitException(cp);
    cmCTestLog(this->CTest, ERROR_MESSAGE, "\tThere was an exception: "
                 << cmsysProcess_GetExceptionString(cp) << " " << retVal
                 << std::endl);
    failed = true;
  } else if (result == cmsysProcess_State_Expired) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "\tThere was a timeout"
                 << std::endl);
    failed = true;
  } else if (result == cmsysProcess_State_Error) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "\tError executing ctest: "
                 << cmsysProcess_GetErrorString(cp) << std::endl);
    failed = true;
  }
  cmsysProcess_Delete(cp);
  if (failed) {
    std::ostringstream message;
    message << "Error running command: [";
    message << result << "] ";
    for (std::vector<const char*>::iterator i = argv.begin(); i != argv.end();
         ++i) {
      if (*i) {
        message << *i << " ";
      }
    }
    cmCTestLog(this->CTest, ERROR_MESSAGE, message.str() << argv[0]
                                                         << std::endl);
    return -1;
  }
  return retVal;
}

static void ctestScriptProgressCallback(const char* m, float /*unused*/,
                                        void* cd)
{
  cmCTest* ctest = static_cast<cmCTest*>(cd);
  if (m && *m) {
    cmCTestLog(ctest, HANDLER_OUTPUT, "-- " << m << std::endl);
  }
}

void cmCTestScriptHandler::CreateCMake()
{
  // create a cmake instance to read the configuration script
  if (this->CMake) {
    delete this->CMake;
    delete this->GlobalGenerator;
    delete this->Makefile;
  }
  this->CMake = new cmake(cmake::RoleScript);
  this->CMake->SetHomeDirectory("");
  this->CMake->SetHomeOutputDirectory("");
  this->CMake->GetCurrentSnapshot().SetDefaultDefinitions();
  this->CMake->AddCMakePaths();
  this->GlobalGenerator = new cmGlobalGenerator(this->CMake);

  cmStateSnapshot snapshot = this->CMake->GetCurrentSnapshot();
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
  snapshot.GetDirectory().SetCurrentSource(cwd);
  snapshot.GetDirectory().SetCurrentBinary(cwd);
  this->Makefile = new cmMakefile(this->GlobalGenerator, snapshot);

  this->CMake->SetProgressCallback(ctestScriptProgressCallback, this->CTest);

  this->AddCTestCommand("ctest_build", new cmCTestBuildCommand);
  this->AddCTestCommand("ctest_configure", new cmCTestConfigureCommand);
  this->AddCTestCommand("ctest_coverage", new cmCTestCoverageCommand);
  this->AddCTestCommand("ctest_empty_binary_directory",
                        new cmCTestEmptyBinaryDirectoryCommand);
  this->AddCTestCommand("ctest_memcheck", new cmCTestMemCheckCommand);
  this->AddCTestCommand("ctest_read_custom_files",
                        new cmCTestReadCustomFilesCommand);
  this->AddCTestCommand("ctest_run_script", new cmCTestRunScriptCommand);
  this->AddCTestCommand("ctest_sleep", new cmCTestSleepCommand);
  this->AddCTestCommand("ctest_start", new cmCTestStartCommand);
  this->AddCTestCommand("ctest_submit", new cmCTestSubmitCommand);
  this->AddCTestCommand("ctest_test", new cmCTestTestCommand);
  this->AddCTestCommand("ctest_update", new cmCTestUpdateCommand);
  this->AddCTestCommand("ctest_upload", new cmCTestUploadCommand);
}

// this sets up some variables for the script to use, creates the required
// cmake instance and generators, and then reads in the script
int cmCTestScriptHandler::ReadInScript(const std::string& total_script_arg)
{
  // Reset the error flag so that the script is read in no matter what
  cmSystemTools::ResetErrorOccuredFlag();

  // if the argument has a , in it then it needs to be broken into the fist
  // argument (which is the script) and the second argument which will be
  // passed into the scripts as S_ARG
  std::string script = total_script_arg;
  std::string script_arg;
  const std::string::size_type comma_pos = total_script_arg.find(',');
  if (comma_pos != std::string::npos) {
    script = total_script_arg.substr(0, comma_pos);
    script_arg = total_script_arg.substr(comma_pos + 1);
  }
  // make sure the file exists
  if (!cmSystemTools::FileExists(script.c_str())) {
    cmSystemTools::Error("Cannot find file: ", script.c_str());
    return 1;
  }

  // read in the list file to fill the cache
  // create a cmake instance to read the configuration script
  this->CreateCMake();

  // set a variable with the path to the current script
  this->Makefile->AddDefinition(
    "CTEST_SCRIPT_DIRECTORY", cmSystemTools::GetFilenamePath(script).c_str());
  this->Makefile->AddDefinition(
    "CTEST_SCRIPT_NAME", cmSystemTools::GetFilenameName(script).c_str());
  this->Makefile->AddDefinition("CTEST_EXECUTABLE_NAME",
                                cmSystemTools::GetCTestCommand().c_str());
  this->Makefile->AddDefinition("CMAKE_EXECUTABLE_NAME",
                                cmSystemTools::GetCMakeCommand().c_str());
  this->Makefile->AddDefinition("CTEST_RUN_CURRENT_SCRIPT", true);
  this->UpdateElapsedTime();

  // add the script arg if defined
  if (!script_arg.empty()) {
    this->Makefile->AddDefinition("CTEST_SCRIPT_ARG", script_arg.c_str());
  }

#if defined(__CYGWIN__)
  this->Makefile->AddDefinition("CMAKE_LEGACY_CYGWIN_WIN32", "0");
#endif

  // always add a function blocker to update the elapsed time
  cmCTestScriptFunctionBlocker* f = new cmCTestScriptFunctionBlocker();
  f->CTestScriptHandler = this;
  this->Makefile->AddFunctionBlocker(f);

  /* Execute CTestScriptMode.cmake, which loads CMakeDetermineSystem and
  CMakeSystemSpecificInformation, so
  that variables like CMAKE_SYSTEM and also the search paths for libraries,
  header and executables are set correctly and can be used. Makes new-style
  ctest scripting easier. */
  std::string systemFile =
    this->Makefile->GetModulesFile("CTestScriptMode.cmake");
  if (!this->Makefile->ReadListFile(systemFile.c_str()) ||
      cmSystemTools::GetErrorOccuredFlag()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Error in read:" << systemFile
                                                            << "\n");
    return 2;
  }

  // Add definitions of variables passed in on the command line:
  const std::map<std::string, std::string>& defs =
    this->CTest->GetDefinitions();
  for (std::map<std::string, std::string>::const_iterator it = defs.begin();
       it != defs.end(); ++it) {
    this->Makefile->AddDefinition(it->first, it->second.c_str());
  }

  // finally read in the script
  if (!this->Makefile->ReadListFile(script.c_str()) ||
      cmSystemTools::GetErrorOccuredFlag()) {
    // Reset the error flag so that it can run more than
    // one script with an error when you use ctest_run_script.
    cmSystemTools::ResetErrorOccuredFlag();
    return 2;
  }

  return 0;
}

// extract variabels from the script to set ivars
int cmCTestScriptHandler::ExtractVariables()
{
  // Temporary variables
  const char* minInterval;
  const char* contDuration;

  this->SourceDir =
    this->Makefile->GetSafeDefinition("CTEST_SOURCE_DIRECTORY");
  this->BinaryDir =
    this->Makefile->GetSafeDefinition("CTEST_BINARY_DIRECTORY");

  // add in translations for src and bin
  cmSystemTools::AddKeepPath(this->SourceDir);
  cmSystemTools::AddKeepPath(this->BinaryDir);

  this->CTestCmd = this->Makefile->GetSafeDefinition("CTEST_COMMAND");
  this->CVSCheckOut = this->Makefile->GetSafeDefinition("CTEST_CVS_CHECKOUT");
  this->CTestRoot = this->Makefile->GetSafeDefinition("CTEST_DASHBOARD_ROOT");
  this->UpdateCmd = this->Makefile->GetSafeDefinition("CTEST_UPDATE_COMMAND");
  if (this->UpdateCmd.empty()) {
    this->UpdateCmd = this->Makefile->GetSafeDefinition("CTEST_CVS_COMMAND");
  }
  this->CTestEnv = this->Makefile->GetSafeDefinition("CTEST_ENVIRONMENT");
  this->InitialCache =
    this->Makefile->GetSafeDefinition("CTEST_INITIAL_CACHE");
  this->CMakeCmd = this->Makefile->GetSafeDefinition("CTEST_CMAKE_COMMAND");
  this->CMOutFile =
    this->Makefile->GetSafeDefinition("CTEST_CMAKE_OUTPUT_FILE_NAME");

  this->Backup = this->Makefile->IsOn("CTEST_BACKUP_AND_RESTORE");
  this->EmptyBinDir =
    this->Makefile->IsOn("CTEST_START_WITH_EMPTY_BINARY_DIRECTORY");
  this->EmptyBinDirOnce =
    this->Makefile->IsOn("CTEST_START_WITH_EMPTY_BINARY_DIRECTORY_ONCE");

  minInterval =
    this->Makefile->GetDefinition("CTEST_CONTINUOUS_MINIMUM_INTERVAL");
  contDuration = this->Makefile->GetDefinition("CTEST_CONTINUOUS_DURATION");

  char updateVar[40];
  int i;
  for (i = 1; i < 10; ++i) {
    sprintf(updateVar, "CTEST_EXTRA_UPDATES_%i", i);
    const char* updateVal = this->Makefile->GetDefinition(updateVar);
    if (updateVal) {
      if (this->UpdateCmd.empty()) {
        cmSystemTools::Error(
          updateVar, " specified without specifying CTEST_CVS_COMMAND.");
        return 12;
      }
      this->ExtraUpdates.push_back(updateVal);
    }
  }

  // in order to backup and restore we also must have the cvs root
  if (this->Backup && this->CVSCheckOut.empty()) {
    cmSystemTools::Error(
      "Backup was requested without specifying CTEST_CVS_CHECKOUT.");
    return 3;
  }

  // make sure the required info is here
  if (this->SourceDir.empty() || this->BinaryDir.empty() ||
      this->CTestCmd.empty()) {
    std::string msg = "CTEST_SOURCE_DIRECTORY = ";
    msg += (!this->SourceDir.empty()) ? this->SourceDir.c_str() : "(Null)";
    msg += "\nCTEST_BINARY_DIRECTORY = ";
    msg += (!this->BinaryDir.empty()) ? this->BinaryDir.c_str() : "(Null)";
    msg += "\nCTEST_COMMAND = ";
    msg += (!this->CTestCmd.empty()) ? this->CTestCmd.c_str() : "(Null)";
    cmSystemTools::Error(
      "Some required settings in the configuration file were missing:\n",
      msg.c_str());
    return 4;
  }

  // if the dashboard root isn't specified then we can compute it from the
  // this->SourceDir
  if (this->CTestRoot.empty()) {
    this->CTestRoot = cmSystemTools::GetFilenamePath(this->SourceDir);
  }

  // the script may override the minimum continuous interval
  if (minInterval) {
    this->MinimumInterval = 60 * atof(minInterval);
  }
  if (contDuration) {
    this->ContinuousDuration = 60.0 * atof(contDuration);
  }

  this->UpdateElapsedTime();

  return 0;
}

void cmCTestScriptHandler::SleepInSeconds(unsigned int secondsToWait)
{
#if defined(_WIN32)
  Sleep(1000 * secondsToWait);
#else
  sleep(secondsToWait);
#endif
}

// run a specific script
int cmCTestScriptHandler::RunConfigurationScript(
  const std::string& total_script_arg, bool pscope)
{
#ifdef CMAKE_BUILD_WITH_CMAKE
  cmSystemTools::SaveRestoreEnvironment sre;
#endif

  int result;

  this->ScriptStartTime = cmSystemTools::GetTime();

  // read in the script
  if (pscope) {
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
               "Reading Script: " << total_script_arg << std::endl);
    result = this->ReadInScript(total_script_arg);
  } else {
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
               "Executing Script: " << total_script_arg << std::endl);
    result = this->ExecuteScript(total_script_arg);
  }
  if (result) {
    return result;
  }

  // only run the curent script if we should
  if (this->Makefile && this->Makefile->IsOn("CTEST_RUN_CURRENT_SCRIPT")) {
    return this->RunCurrentScript();
  }
  return result;
}

int cmCTestScriptHandler::RunCurrentScript()
{
  int result;

  // do not run twice
  this->Makefile->AddDefinition("CTEST_RUN_CURRENT_SCRIPT", false);

  // no popup widows
  cmSystemTools::SetRunCommandHideConsole(true);

  // extract the vars from the cache and store in ivars
  result = this->ExtractVariables();
  if (result) {
    return result;
  }

  // set any environment variables
  if (!this->CTestEnv.empty()) {
    std::vector<std::string> envArgs;
    cmSystemTools::ExpandListArgument(this->CTestEnv, envArgs);
    cmSystemTools::AppendEnv(envArgs);
  }

  // now that we have done most of the error checking finally run the
  // dashboard, we may be asked to repeatedly run this dashboard, such as
  // for a continuous, do we ned to run it more than once?
  if (this->ContinuousDuration >= 0) {
    this->UpdateElapsedTime();
    double ending_time = cmSystemTools::GetTime() + this->ContinuousDuration;
    if (this->EmptyBinDirOnce) {
      this->EmptyBinDir = true;
    }
    do {
      double interval = cmSystemTools::GetTime();
      result = this->RunConfigurationDashboard();
      interval = cmSystemTools::GetTime() - interval;
      if (interval < this->MinimumInterval) {
        this->SleepInSeconds(
          static_cast<unsigned int>(this->MinimumInterval - interval));
      }
      if (this->EmptyBinDirOnce) {
        this->EmptyBinDir = false;
      }
    } while (cmSystemTools::GetTime() < ending_time);
  }
  // otherwise just run it once
  else {
    result = this->RunConfigurationDashboard();
  }

  return result;
}

int cmCTestScriptHandler::CheckOutSourceDir()
{
  std::string command;
  std::string output;
  int retVal;
  bool res;

  if (!cmSystemTools::FileExists(this->SourceDir.c_str()) &&
      !this->CVSCheckOut.empty()) {
    // we must now checkout the src dir
    output = "";
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
               "Run cvs: " << this->CVSCheckOut << std::endl);
    res = cmSystemTools::RunSingleCommand(
      this->CVSCheckOut.c_str(), &output, &output, &retVal,
      this->CTestRoot.c_str(), this->HandlerVerbose, 0 /*this->TimeOut*/);
    if (!res || retVal != 0) {
      cmSystemTools::Error("Unable to perform cvs checkout:\n",
                           output.c_str());
      return 6;
    }
  }
  return 0;
}

int cmCTestScriptHandler::BackupDirectories()
{
  int retVal;

  // compute the backup names
  this->BackupSourceDir = this->SourceDir;
  this->BackupSourceDir += "_CMakeBackup";
  this->BackupBinaryDir = this->BinaryDir;
  this->BackupBinaryDir += "_CMakeBackup";

  // backup the binary and src directories if requested
  if (this->Backup) {
    // if for some reason those directories exist then first delete them
    if (cmSystemTools::FileExists(this->BackupSourceDir.c_str())) {
      cmSystemTools::RemoveADirectory(this->BackupSourceDir);
    }
    if (cmSystemTools::FileExists(this->BackupBinaryDir.c_str())) {
      cmSystemTools::RemoveADirectory(this->BackupBinaryDir);
    }

    // first rename the src and binary directories
    rename(this->SourceDir.c_str(), this->BackupSourceDir.c_str());
    rename(this->BinaryDir.c_str(), this->BackupBinaryDir.c_str());

    // we must now checkout the src dir
    retVal = this->CheckOutSourceDir();
    if (retVal) {
      this->RestoreBackupDirectories();
      return retVal;
    }
  }

  return 0;
}

int cmCTestScriptHandler::PerformExtraUpdates()
{
  std::string command;
  std::string output;
  int retVal;
  bool res;

  // do an initial cvs update as required
  command = this->UpdateCmd;
  std::vector<std::string>::iterator it;
  for (it = this->ExtraUpdates.begin(); it != this->ExtraUpdates.end(); ++it) {
    std::vector<std::string> cvsArgs;
    cmSystemTools::ExpandListArgument(*it, cvsArgs);
    if (cvsArgs.size() == 2) {
      std::string fullCommand = command;
      fullCommand += " update ";
      fullCommand += cvsArgs[1];
      output = "";
      retVal = 0;
      cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                 "Run Update: " << fullCommand << std::endl);
      res = cmSystemTools::RunSingleCommand(
        fullCommand.c_str(), &output, &output, &retVal, cvsArgs[0].c_str(),
        this->HandlerVerbose, 0 /*this->TimeOut*/);
      if (!res || retVal != 0) {
        cmSystemTools::Error("Unable to perform extra updates:\n", it->c_str(),
                             "\nWith output:\n", output.c_str());
        return 0;
      }
    }
  }
  return 0;
}

// run a single dashboard entry
int cmCTestScriptHandler::RunConfigurationDashboard()
{
  // local variables
  std::string command;
  std::string output;
  int retVal;
  bool res;

  // make sure the src directory is there, if it isn't then we might be able
  // to check it out from cvs
  retVal = this->CheckOutSourceDir();
  if (retVal) {
    return retVal;
  }

  // backup the dirs if requested
  retVal = this->BackupDirectories();
  if (retVal) {
    return retVal;
  }

  // clear the binary directory?
  if (this->EmptyBinDir) {
    if (!cmCTestScriptHandler::EmptyBinaryDirectory(this->BinaryDir.c_str())) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Problem removing the binary directory" << std::endl);
    }
  }

  // make sure the binary directory exists if it isn't the srcdir
  if (!cmSystemTools::FileExists(this->BinaryDir.c_str()) &&
      this->SourceDir != this->BinaryDir) {
    if (!cmSystemTools::MakeDirectory(this->BinaryDir.c_str())) {
      cmSystemTools::Error("Unable to create the binary directory:\n",
                           this->BinaryDir.c_str());
      this->RestoreBackupDirectories();
      return 7;
    }
  }

  // if the binary directory and the source directory are the same,
  // and we are starting with an empty binary directory, then that means
  // we must check out the source tree
  if (this->EmptyBinDir && this->SourceDir == this->BinaryDir) {
    // make sure we have the required info
    if (this->CVSCheckOut.empty()) {
      cmSystemTools::Error(
        "You have specified the source and binary "
        "directories to be the same (an in source build). You have also "
        "specified that the binary directory is to be erased. This means "
        "that the source will have to be checked out from CVS. But you have "
        "not specified CTEST_CVS_CHECKOUT");
      return 8;
    }

    // we must now checkout the src dir
    retVal = this->CheckOutSourceDir();
    if (retVal) {
      this->RestoreBackupDirectories();
      return retVal;
    }
  }

  // backup the dirs if requested
  retVal = this->PerformExtraUpdates();
  if (retVal) {
    return retVal;
  }

  // put the initial cache into the bin dir
  if (!this->InitialCache.empty()) {
    if (!this->WriteInitialCache(this->BinaryDir.c_str(),
                                 this->InitialCache.c_str())) {
      this->RestoreBackupDirectories();
      return 9;
    }
  }

  // do an initial cmake to setup the DartConfig file
  int cmakeFailed = 0;
  std::string cmakeFailedOuput;
  if (!this->CMakeCmd.empty()) {
    command = this->CMakeCmd;
    command += " \"";
    command += this->SourceDir;
    output = "";
    command += "\"";
    retVal = 0;
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
               "Run cmake command: " << command << std::endl);
    res = cmSystemTools::RunSingleCommand(
      command.c_str(), &output, &output, &retVal, this->BinaryDir.c_str(),
      this->HandlerVerbose, 0 /*this->TimeOut*/);

    if (!this->CMOutFile.empty()) {
      std::string cmakeOutputFile = this->CMOutFile;
      if (!cmSystemTools::FileIsFullPath(cmakeOutputFile.c_str())) {
        cmakeOutputFile = this->BinaryDir + "/" + cmakeOutputFile;
      }

      cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                 "Write CMake output to file: " << cmakeOutputFile
                                                << std::endl);
      cmGeneratedFileStream fout(cmakeOutputFile.c_str());
      if (fout) {
        fout << output.c_str();
      } else {
        cmCTestLog(this->CTest, ERROR_MESSAGE,
                   "Cannot open CMake output file: "
                     << cmakeOutputFile << " for writing" << std::endl);
      }
    }
    if (!res || retVal != 0) {
      // even if this fails continue to the next step
      cmakeFailed = 1;
      cmakeFailedOuput = output;
    }
  }

  // run ctest, it may be more than one command in here
  std::vector<std::string> ctestCommands;
  cmSystemTools::ExpandListArgument(this->CTestCmd, ctestCommands);
  // for each variable/argument do a putenv
  for (unsigned i = 0; i < ctestCommands.size(); ++i) {
    command = ctestCommands[i];
    output = "";
    retVal = 0;
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
               "Run ctest command: " << command << std::endl);
    res = cmSystemTools::RunSingleCommand(
      command.c_str(), &output, &output, &retVal, this->BinaryDir.c_str(),
      this->HandlerVerbose, 0 /*this->TimeOut*/);

    // did something critical fail in ctest
    if (!res || cmakeFailed || retVal & cmCTest::BUILD_ERRORS) {
      this->RestoreBackupDirectories();
      if (cmakeFailed) {
        cmCTestLog(this->CTest, ERROR_MESSAGE,
                   "Unable to run cmake:" << std::endl
                                          << cmakeFailedOuput << std::endl);
        return 10;
      }
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Unable to run ctest:" << std::endl
                                        << "command: " << command << std::endl
                                        << "output: " << output << std::endl);
      if (!res) {
        return 11;
      }
      return retVal * 100;
    }
  }

  // if all was succesful, delete the backup dirs to free up disk space
  if (this->Backup) {
    cmSystemTools::RemoveADirectory(this->BackupSourceDir);
    cmSystemTools::RemoveADirectory(this->BackupBinaryDir);
  }

  return 0;
}

bool cmCTestScriptHandler::WriteInitialCache(const char* directory,
                                             const char* text)
{
  std::string cacheFile = directory;
  cacheFile += "/CMakeCache.txt";
  cmGeneratedFileStream fout(cacheFile.c_str());
  if (!fout) {
    return false;
  }

  if (text != CM_NULLPTR) {
    fout.write(text, strlen(text));
  }

  // Make sure the operating system has finished writing the file
  // before closing it.  This will ensure the file is finished before
  // the check below.
  fout.flush();
  fout.close();
  return true;
}

void cmCTestScriptHandler::RestoreBackupDirectories()
{
  // if we backed up the dirs and the build failed, then restore
  // the backed up dirs
  if (this->Backup) {
    // if for some reason those directories exist then first delete them
    if (cmSystemTools::FileExists(this->SourceDir.c_str())) {
      cmSystemTools::RemoveADirectory(this->SourceDir);
    }
    if (cmSystemTools::FileExists(this->BinaryDir.c_str())) {
      cmSystemTools::RemoveADirectory(this->BinaryDir);
    }
    // rename the src and binary directories
    rename(this->BackupSourceDir.c_str(), this->SourceDir.c_str());
    rename(this->BackupBinaryDir.c_str(), this->BinaryDir.c_str());
  }
}

bool cmCTestScriptHandler::RunScript(cmCTest* ctest, const char* sname,
                                     bool InProcess, int* returnValue)
{
  cmCTestScriptHandler* sh = new cmCTestScriptHandler();
  sh->SetCTestInstance(ctest);
  sh->AddConfigurationScript(sname, InProcess);
  int res = sh->ProcessHandler();
  if (returnValue) {
    *returnValue = res;
  }
  delete sh;
  return true;
}

bool cmCTestScriptHandler::EmptyBinaryDirectory(const char* sname)
{
  // try to avoid deleting root
  if (!sname || strlen(sname) < 2) {
    return false;
  }

  // consider non existing target directory a success
  if (!cmSystemTools::FileExists(sname)) {
    return true;
  }

  // try to avoid deleting directories that we shouldn't
  std::string check = sname;
  check += "/CMakeCache.txt";

  if (!cmSystemTools::FileExists(check.c_str())) {
    return false;
  }

  for (int i = 0; i < 5; ++i) {
    if (TryToRemoveBinaryDirectoryOnce(sname)) {
      return true;
    }
    cmSystemTools::Delay(100);
  }

  return false;
}

bool cmCTestScriptHandler::TryToRemoveBinaryDirectoryOnce(
  const std::string& directoryPath)
{
  cmsys::Directory directory;
  directory.Load(directoryPath);

  for (unsigned long i = 0; i < directory.GetNumberOfFiles(); ++i) {
    std::string path = directory.GetFile(i);

    if (path == "." || path == ".." || path == "CMakeCache.txt") {
      continue;
    }

    std::string fullPath = directoryPath + std::string("/") + path;

    bool isDirectory = cmSystemTools::FileIsDirectory(fullPath) &&
      !cmSystemTools::FileIsSymlink(fullPath);

    if (isDirectory) {
      if (!cmSystemTools::RemoveADirectory(fullPath)) {
        return false;
      }
    } else {
      if (!cmSystemTools::RemoveFile(fullPath)) {
        return false;
      }
    }
  }

  return cmSystemTools::RemoveADirectory(directoryPath);
}

double cmCTestScriptHandler::GetRemainingTimeAllowed()
{
  if (!this->Makefile) {
    return 1.0e7;
  }

  const char* timelimitS = this->Makefile->GetDefinition("CTEST_TIME_LIMIT");

  if (!timelimitS) {
    return 1.0e7;
  }

  double timelimit = atof(timelimitS);

  return timelimit - cmSystemTools::GetTime() + this->ScriptStartTime;
}
