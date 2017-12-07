/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestHandlerCommand.h"

#include "cmCTest.h"
#include "cmCTestGenericHandler.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmWorkingDirectory.h"
#include "cmake.h"

#include <sstream>
#include <stdlib.h>

class cmExecutionStatus;

cmCTestHandlerCommand::cmCTestHandlerCommand()
{
  const size_t INIT_SIZE = 100;
  size_t cc;
  this->Arguments.reserve(INIT_SIZE);
  for (cc = 0; cc < INIT_SIZE; ++cc) {
    this->Arguments.push_back(CM_NULLPTR);
  }
  this->Arguments[ct_RETURN_VALUE] = "RETURN_VALUE";
  this->Arguments[ct_CAPTURE_CMAKE_ERROR] = "CAPTURE_CMAKE_ERROR";
  this->Arguments[ct_SOURCE] = "SOURCE";
  this->Arguments[ct_BUILD] = "BUILD";
  this->Arguments[ct_SUBMIT_INDEX] = "SUBMIT_INDEX";
  this->Last = ct_LAST;
  this->AppendXML = false;
  this->Quiet = false;
}

namespace {
// class to save and restore the error state for ctest_* commands
// if a ctest_* command has a CAPTURE_CMAKE_ERROR then put the error
// state into there and restore the system wide error to what
// it was before the command ran
class SaveRestoreErrorState
{
public:
  SaveRestoreErrorState()
  {
    this->InitialErrorState = cmSystemTools::GetErrorOccuredFlag();
    cmSystemTools::ResetErrorOccuredFlag(); // rest the error state
    this->CaptureCMakeErrorValue = false;
  }
  // if the function has a CAPTURE_CMAKE_ERROR then we should restore
  // the error state to what it was before the function was run
  // if not then let the error state be what it is
  void CaptureCMakeError() { this->CaptureCMakeErrorValue = true; }
  ~SaveRestoreErrorState()
  {
    // if we are not saving the return value then make sure
    // if it was in error it goes back to being in error
    // otherwise leave it be what it is
    if (!this->CaptureCMakeErrorValue) {
      if (this->InitialErrorState) {
        cmSystemTools::SetErrorOccured();
      }
      return;
    }
    // if we have saved the error in a return variable
    // then put things back exactly like they were
    bool currentState = cmSystemTools::GetErrorOccuredFlag();
    // if the state changed during this command we need
    // to handle it, if not then nothing needs to be done
    if (currentState != this->InitialErrorState) {
      // restore the initial error state
      if (this->InitialErrorState) {
        cmSystemTools::SetErrorOccured();
      } else {
        cmSystemTools::ResetErrorOccuredFlag();
      }
    }
  }

private:
  bool InitialErrorState;
  bool CaptureCMakeErrorValue;
};
}

bool cmCTestHandlerCommand::InitialPass(std::vector<std::string> const& args,
                                        cmExecutionStatus& /*unused*/)
{
  // save error state and restore it if needed
  SaveRestoreErrorState errorState;
  // Allocate space for argument values.
  this->Values.clear();
  this->Values.resize(this->Last, CM_NULLPTR);

  // Process input arguments.
  this->ArgumentDoing = ArgumentDoingNone;
  // look at all arguments and do not short circuit on the first
  // bad one so that CAPTURE_CMAKE_ERROR can override setting the
  // global error state
  bool foundBadArgument = false;
  for (unsigned int i = 0; i < args.size(); ++i) {
    // Check this argument.
    if (!this->CheckArgumentKeyword(args[i]) &&
        !this->CheckArgumentValue(args[i])) {
      std::ostringstream e;
      e << "called with unknown argument \"" << args[i] << "\".";
      this->SetError(e.str());
      foundBadArgument = true;
    }
    // note bad argument
    if (this->ArgumentDoing == ArgumentDoingError) {
      foundBadArgument = true;
    }
  }
  bool capureCMakeError = (this->Values[ct_CAPTURE_CMAKE_ERROR] &&
                           *this->Values[ct_CAPTURE_CMAKE_ERROR]);
  // now that arguments are parsed check to see if there is a
  // CAPTURE_CMAKE_ERROR specified let the errorState object know.
  if (capureCMakeError) {
    errorState.CaptureCMakeError();
  }
  // if we found a bad argument then exit before running command
  if (foundBadArgument) {
    // store the cmake error
    if (capureCMakeError) {
      this->Makefile->AddDefinition(this->Values[ct_CAPTURE_CMAKE_ERROR],
                                    "-1");
      std::string const err = this->GetName() + " " + this->GetError();
      if (!cmSystemTools::FindLastString(err.c_str(), "unknown error.")) {
        cmCTestLog(this->CTest, ERROR_MESSAGE, err << " error from command\n");
      }
      // return success because failure is recorded in CAPTURE_CMAKE_ERROR
      return true;
    }
    // return failure because of bad argument
    return false;
  }

  // Set the config type of this ctest to the current value of the
  // CTEST_CONFIGURATION_TYPE script variable if it is defined.
  // The current script value trumps the -C argument on the command
  // line.
  const char* ctestConfigType =
    this->Makefile->GetDefinition("CTEST_CONFIGURATION_TYPE");
  if (ctestConfigType) {
    this->CTest->SetConfigType(ctestConfigType);
  }

  if (this->Values[ct_BUILD]) {
    this->CTest->SetCTestConfiguration(
      "BuildDirectory",
      cmSystemTools::CollapseFullPath(this->Values[ct_BUILD]).c_str(),
      this->Quiet);
  } else {
    const char* bdir =
      this->Makefile->GetSafeDefinition("CTEST_BINARY_DIRECTORY");
    if (bdir) {
      this->CTest->SetCTestConfiguration(
        "BuildDirectory", cmSystemTools::CollapseFullPath(bdir).c_str(),
        this->Quiet);
    } else {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "CTEST_BINARY_DIRECTORY not set"
                   << std::endl;);
    }
  }
  if (this->Values[ct_SOURCE]) {
    cmCTestLog(this->CTest, DEBUG, "Set source directory to: "
                 << this->Values[ct_SOURCE] << std::endl);
    this->CTest->SetCTestConfiguration(
      "SourceDirectory",
      cmSystemTools::CollapseFullPath(this->Values[ct_SOURCE]).c_str(),
      this->Quiet);
  } else {
    this->CTest->SetCTestConfiguration(
      "SourceDirectory",
      cmSystemTools::CollapseFullPath(
        this->Makefile->GetSafeDefinition("CTEST_SOURCE_DIRECTORY"))
        .c_str(),
      this->Quiet);
  }

  if (const char* changeId =
        this->Makefile->GetDefinition("CTEST_CHANGE_ID")) {
    this->CTest->SetCTestConfiguration("ChangeId", changeId, this->Quiet);
  }

  cmCTestLog(this->CTest, DEBUG, "Initialize handler" << std::endl;);
  cmCTestGenericHandler* handler = this->InitializeHandler();
  if (!handler) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot instantiate test handler "
                 << this->GetName() << std::endl);
    if (capureCMakeError) {
      this->Makefile->AddDefinition(this->Values[ct_CAPTURE_CMAKE_ERROR],
                                    "-1");
      const char* err = this->GetError();
      if (err && !cmSystemTools::FindLastString(err, "unknown error.")) {
        cmCTestLog(this->CTest, ERROR_MESSAGE, err << " error from command\n");
      }
      return true;
    }
    return false;
  }

  handler->SetAppendXML(this->AppendXML);

  handler->PopulateCustomVectors(this->Makefile);
  if (this->Values[ct_SUBMIT_INDEX]) {
    if (!this->CTest->GetDropSiteCDash() &&
        this->CTest->GetDartVersion() <= 1) {
      cmCTestLog(
        this->CTest, ERROR_MESSAGE,
        "Dart before version 2.0 does not support collecting submissions."
          << std::endl
          << "Please upgrade the server to Dart 2 or higher, or do not use "
             "SUBMIT_INDEX."
          << std::endl);
    } else {
      handler->SetSubmitIndex(atoi(this->Values[ct_SUBMIT_INDEX]));
    }
  }
  cmWorkingDirectory workdir(
    this->CTest->GetCTestConfiguration("BuildDirectory"));
  int res = handler->ProcessHandler();
  if (this->Values[ct_RETURN_VALUE] && *this->Values[ct_RETURN_VALUE]) {
    std::ostringstream str;
    str << res;
    this->Makefile->AddDefinition(this->Values[ct_RETURN_VALUE],
                                  str.str().c_str());
  }
  this->ProcessAdditionalValues(handler);
  // log the error message if there was an error
  if (capureCMakeError) {
    const char* returnString = "0";
    if (cmSystemTools::GetErrorOccuredFlag()) {
      returnString = "-1";
      const char* err = this->GetError();
      // print out the error if it is not "unknown error" which means
      // there was no message
      if (err && !cmSystemTools::FindLastString(err, "unknown error.")) {
        cmCTestLog(this->CTest, ERROR_MESSAGE, err);
      }
    }
    // store the captured cmake error state 0 or -1
    this->Makefile->AddDefinition(this->Values[ct_CAPTURE_CMAKE_ERROR],
                                  returnString);
  }
  return true;
}

void cmCTestHandlerCommand::ProcessAdditionalValues(cmCTestGenericHandler*)
{
}

bool cmCTestHandlerCommand::CheckArgumentKeyword(std::string const& arg)
{
  // Look for non-value arguments common to all commands.
  if (arg == "APPEND") {
    this->ArgumentDoing = ArgumentDoingNone;
    this->AppendXML = true;
    return true;
  }
  if (arg == "QUIET") {
    this->ArgumentDoing = ArgumentDoingNone;
    this->Quiet = true;
    return true;
  }

  // Check for a keyword in our argument/value table.
  for (unsigned int k = 0; k < this->Arguments.size(); ++k) {
    if (this->Arguments[k] && arg == this->Arguments[k]) {
      this->ArgumentDoing = ArgumentDoingKeyword;
      this->ArgumentIndex = k;
      return true;
    }
  }
  return false;
}

bool cmCTestHandlerCommand::CheckArgumentValue(std::string const& arg)
{
  if (this->ArgumentDoing == ArgumentDoingKeyword) {
    this->ArgumentDoing = ArgumentDoingNone;
    unsigned int k = this->ArgumentIndex;
    if (this->Values[k]) {
      std::ostringstream e;
      e << "Called with more than one value for " << this->Arguments[k];
      this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
      this->ArgumentDoing = ArgumentDoingError;
      return true;
    }
    this->Values[k] = arg.c_str();
    cmCTestLog(this->CTest, DEBUG, "Set " << this->Arguments[k] << " to "
                                          << arg << "\n");
    return true;
  }
  return false;
}
