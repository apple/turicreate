/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestRunTest.h"

#include "cmCTest.h"
#include "cmCTestMemCheckHandler.h"
#include "cmCTestMultiProcessHandler.h"
#include "cmProcess.h"
#include "cmSystemTools.h"
#include "cmWorkingDirectory.h"

#include "cm_zlib.h"
#include "cmsys/Base64.h"
#include "cmsys/RegularExpression.hxx"
#include <chrono>
#include <cmAlgorithms.h>
#include <cstring>
#include <iomanip>
#include <ratio>
#include <sstream>
#include <stdio.h>
#include <utility>

cmCTestRunTest::cmCTestRunTest(cmCTestMultiProcessHandler& multiHandler)
  : MultiTestHandler(multiHandler)
{
  this->CTest = multiHandler.CTest;
  this->TestHandler = multiHandler.TestHandler;
  this->TestResult.ExecutionTime = cmDuration::zero();
  this->TestResult.ReturnValue = 0;
  this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
  this->TestResult.TestCount = 0;
  this->TestResult.Properties = nullptr;
  this->ProcessOutput.clear();
  this->CompressedOutput.clear();
  this->CompressionRatio = 2;
  this->NumberOfRunsLeft = 1; // default to 1 run of the test
  this->RunUntilFail = false; // default to run the test once
  this->RunAgain = false;     // default to not having to run again
}

void cmCTestRunTest::CheckOutput(std::string const& line)
{
  cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
             this->GetIndex() << ": " << line << std::endl);
  this->ProcessOutput += line;
  this->ProcessOutput += "\n";

  // Check for TIMEOUT_AFTER_MATCH property.
  if (!this->TestProperties->TimeoutRegularExpressions.empty()) {
    for (auto& reg : this->TestProperties->TimeoutRegularExpressions) {
      if (reg.first.find(this->ProcessOutput)) {
        cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                   this->GetIndex()
                     << ": "
                     << "Test timeout changed to "
                     << std::chrono::duration_cast<std::chrono::seconds>(
                          this->TestProperties->AlternateTimeout)
                          .count()
                     << std::endl);
        this->TestProcess->ResetStartTime();
        this->TestProcess->ChangeTimeout(
          this->TestProperties->AlternateTimeout);
        this->TestProperties->TimeoutRegularExpressions.clear();
        break;
      }
    }
  }
}

// Streamed compression of test output.  The compressed data
// is appended to this->CompressedOutput
void cmCTestRunTest::CompressOutput()
{
  int ret;
  z_stream strm;

  unsigned char* in = reinterpret_cast<unsigned char*>(
    const_cast<char*>(this->ProcessOutput.c_str()));
  // zlib makes the guarantee that this is the maximum output size
  int outSize = static_cast<int>(
    static_cast<double>(this->ProcessOutput.size()) * 1.001 + 13.0);
  unsigned char* out = new unsigned char[outSize];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, -1); // default compression level
  if (ret != Z_OK) {
    delete[] out;
    return;
  }

  strm.avail_in = static_cast<uInt>(this->ProcessOutput.size());
  strm.next_in = in;
  strm.avail_out = outSize;
  strm.next_out = out;
  ret = deflate(&strm, Z_FINISH);

  if (ret != Z_STREAM_END) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Error during output compression. Sending uncompressed output."
                 << std::endl);
    delete[] out;
    return;
  }

  (void)deflateEnd(&strm);

  unsigned char* encoded_buffer =
    new unsigned char[static_cast<int>(outSize * 1.5)];

  size_t rlen = cmsysBase64_Encode(out, strm.total_out, encoded_buffer, 1);

  this->CompressedOutput.clear();
  for (size_t i = 0; i < rlen; i++) {
    this->CompressedOutput += encoded_buffer[i];
  }

  if (strm.total_in) {
    this->CompressionRatio =
      static_cast<double>(strm.total_out) / static_cast<double>(strm.total_in);
  }

  delete[] encoded_buffer;
  delete[] out;
}

bool cmCTestRunTest::EndTest(size_t completed, size_t total, bool started)
{
  if ((!this->TestHandler->MemCheck &&
       this->CTest->ShouldCompressTestOutput()) ||
      (this->TestHandler->MemCheck &&
       this->CTest->ShouldCompressTestOutput())) {
    this->CompressOutput();
  }

  this->WriteLogOutputTop(completed, total);
  std::string reason;
  bool passed = true;
  cmProcess::State res =
    started ? this->TestProcess->GetProcessStatus() : cmProcess::State::Error;
  if (res != cmProcess::State::Expired) {
    this->TimeoutIsForStopTime = false;
  }
  int retVal = this->TestProcess->GetExitValue();
  bool forceFail = false;
  bool skipped = false;
  bool outputTestErrorsToConsole = false;
  if (!this->TestProperties->RequiredRegularExpressions.empty() &&
      this->FailedDependencies.empty()) {
    bool found = false;
    for (auto& pass : this->TestProperties->RequiredRegularExpressions) {
      if (pass.first.find(this->ProcessOutput)) {
        found = true;
        reason = "Required regular expression found.";
        break;
      }
    }
    if (!found) {
      reason = "Required regular expression not found.";
      forceFail = true;
    }
    reason += "Regex=[";
    for (auto& pass : this->TestProperties->RequiredRegularExpressions) {
      reason += pass.second;
      reason += "\n";
    }
    reason += "]";
  }
  if (!this->TestProperties->ErrorRegularExpressions.empty() &&
      this->FailedDependencies.empty()) {
    for (auto& pass : this->TestProperties->ErrorRegularExpressions) {
      if (pass.first.find(this->ProcessOutput)) {
        reason = "Error regular expression found in output.";
        reason += " Regex=[";
        reason += pass.second;
        reason += "]";
        forceFail = true;
        break;
      }
    }
  }
  std::ostringstream outputStream;
  if (res == cmProcess::State::Exited) {
    bool success = !forceFail &&
      (retVal == 0 ||
       !this->TestProperties->RequiredRegularExpressions.empty());
    if (this->TestProperties->SkipReturnCode >= 0 &&
        this->TestProperties->SkipReturnCode == retVal) {
      this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
      std::ostringstream s;
      s << "SKIP_RETURN_CODE=" << this->TestProperties->SkipReturnCode;
      this->TestResult.CompletionStatus = s.str();
      cmCTestLog(this->CTest, HANDLER_OUTPUT, "***Skipped ");
      skipped = true;
    } else if ((success && !this->TestProperties->WillFail) ||
               (!success && this->TestProperties->WillFail)) {
      this->TestResult.Status = cmCTestTestHandler::COMPLETED;
      outputStream << "   Passed  ";
    } else {
      this->TestResult.Status = cmCTestTestHandler::FAILED;
      outputStream << "***Failed  " << reason;
      outputTestErrorsToConsole = this->CTest->OutputTestOutputOnTestFailure;
    }
  } else if (res == cmProcess::State::Expired) {
    outputStream << "***Timeout ";
    this->TestResult.Status = cmCTestTestHandler::TIMEOUT;
    outputTestErrorsToConsole = this->CTest->OutputTestOutputOnTestFailure;
  } else if (res == cmProcess::State::Exception) {
    outputTestErrorsToConsole = this->CTest->OutputTestOutputOnTestFailure;
    outputStream << "***Exception: ";
    this->TestResult.ExceptionStatus =
      this->TestProcess->GetExitExceptionString();
    switch (this->TestProcess->GetExitException()) {
      case cmProcess::Exception::Fault:
        outputStream << "SegFault";
        this->TestResult.Status = cmCTestTestHandler::SEGFAULT;
        break;
      case cmProcess::Exception::Illegal:
        outputStream << "Illegal";
        this->TestResult.Status = cmCTestTestHandler::ILLEGAL;
        break;
      case cmProcess::Exception::Interrupt:
        outputStream << "Interrupt";
        this->TestResult.Status = cmCTestTestHandler::INTERRUPT;
        break;
      case cmProcess::Exception::Numerical:
        outputStream << "Numerical";
        this->TestResult.Status = cmCTestTestHandler::NUMERICAL;
        break;
      default:
        cmCTestLog(this->CTest, HANDLER_OUTPUT,
                   this->TestResult.ExceptionStatus);
        this->TestResult.Status = cmCTestTestHandler::OTHER_FAULT;
    }
  } else if ("Disabled" == this->TestResult.CompletionStatus) {
    outputStream << "***Not Run (Disabled) ";
  } else // cmProcess::State::Error
  {
    outputStream << "***Not Run ";
  }

  passed = this->TestResult.Status == cmCTestTestHandler::COMPLETED;
  char buf[1024];
  sprintf(buf, "%6.2f sec", this->TestProcess->GetTotalTime().count());
  outputStream << buf << "\n";

  if (this->CTest->GetTestProgressOutput()) {
    if (!passed) {
      // If the test did not pass, reprint test name and error
      std::string output = GetTestPrefix(completed, total);
      std::string testName = this->TestProperties->Name;
      const int maxTestNameWidth = this->CTest->GetMaxTestNameWidth();
      testName.resize(maxTestNameWidth + 4, '.');

      output += testName;
      output += outputStream.str();
      outputStream.str("");
      outputStream.clear();
      outputStream << output;
      cmCTestLog(this->CTest, HANDLER_TEST_PROGRESS_OUTPUT, "\n"); // flush
    }
    if (completed == total) {
      std::string testName =
        GetTestPrefix(completed, total) + this->TestProperties->Name + "\n";
      cmCTestLog(this->CTest, HANDLER_TEST_PROGRESS_OUTPUT, testName);
    }
  }
  if (!this->CTest->GetTestProgressOutput() || !passed) {
    cmCTestLog(this->CTest, HANDLER_OUTPUT, outputStream.str());
  }

  if (outputTestErrorsToConsole) {
    cmCTestLog(this->CTest, HANDLER_OUTPUT, this->ProcessOutput << std::endl);
  }

  if (this->TestHandler->LogFile) {
    *this->TestHandler->LogFile << "Test time = " << buf << std::endl;
  }

  this->DartProcessing();

  // if this is doing MemCheck then all the output needs to be put into
  // Output since that is what is parsed by cmCTestMemCheckHandler
  if (!this->TestHandler->MemCheck && started) {
    this->TestHandler->CleanTestOutput(
      this->ProcessOutput,
      static_cast<size_t>(
        this->TestResult.Status == cmCTestTestHandler::COMPLETED
          ? this->TestHandler->CustomMaximumPassedTestOutputSize
          : this->TestHandler->CustomMaximumFailedTestOutputSize));
  }
  this->TestResult.Reason = reason;
  if (this->TestHandler->LogFile) {
    bool pass = true;
    const char* reasonType = "Test Pass Reason";
    if (this->TestResult.Status != cmCTestTestHandler::COMPLETED &&
        this->TestResult.Status != cmCTestTestHandler::NOT_RUN) {
      reasonType = "Test Fail Reason";
      pass = false;
    }
    auto ttime = this->TestProcess->GetTotalTime();
    auto hours = std::chrono::duration_cast<std::chrono::hours>(ttime);
    ttime -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(ttime);
    ttime -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ttime);
    char buffer[100];
    sprintf(buffer, "%02d:%02d:%02d", static_cast<unsigned>(hours.count()),
            static_cast<unsigned>(minutes.count()),
            static_cast<unsigned>(seconds.count()));
    *this->TestHandler->LogFile
      << "----------------------------------------------------------"
      << std::endl;
    if (!this->TestResult.Reason.empty()) {
      *this->TestHandler->LogFile << reasonType << ":\n"
                                  << this->TestResult.Reason << "\n";
    } else {
      if (pass) {
        *this->TestHandler->LogFile << "Test Passed.\n";
      } else {
        *this->TestHandler->LogFile << "Test Failed.\n";
      }
    }
    *this->TestHandler->LogFile
      << "\"" << this->TestProperties->Name
      << "\" end time: " << this->CTest->CurrentTime() << std::endl
      << "\"" << this->TestProperties->Name << "\" time elapsed: " << buffer
      << std::endl
      << "----------------------------------------------------------"
      << std::endl
      << std::endl;
  }
  // if the test actually started and ran
  // record the results in TestResult
  if (started) {
    bool compress = !this->TestHandler->MemCheck &&
      this->CompressionRatio < 1 && this->CTest->ShouldCompressTestOutput();
    this->TestResult.Output =
      compress ? this->CompressedOutput : this->ProcessOutput;
    this->TestResult.CompressOutput = compress;
    this->TestResult.ReturnValue = this->TestProcess->GetExitValue();
    if (!skipped) {
      this->TestResult.CompletionStatus = "Completed";
    }
    this->TestResult.ExecutionTime = this->TestProcess->GetTotalTime();
    this->MemCheckPostProcess();
    this->ComputeWeightedCost();
  }
  // If the test does not need to rerun push the current TestResult onto the
  // TestHandler vector
  if (!this->NeedsToRerun()) {
    this->TestHandler->TestResults.push_back(this->TestResult);
  }
  this->TestProcess.reset();
  return passed || skipped;
}

bool cmCTestRunTest::StartAgain(size_t completed)
{
  if (!this->RunAgain) {
    return false;
  }
  this->RunAgain = false; // reset
  // change to tests directory
  cmWorkingDirectory workdir(this->TestProperties->Directory);
  if (workdir.Failed()) {
    this->StartFailure("Failed to change working directory to " +
                       this->TestProperties->Directory + " : " +
                       std::strerror(workdir.GetLastResult()));
    return true;
  }

  this->StartTest(completed, this->TotalNumberOfTests);
  return true;
}

bool cmCTestRunTest::NeedsToRerun()
{
  this->NumberOfRunsLeft--;
  if (this->NumberOfRunsLeft == 0) {
    return false;
  }
  // if number of runs left is not 0, and we are running until
  // we find a failed test, then return true so the test can be
  // restarted
  if (this->RunUntilFail &&
      this->TestResult.Status == cmCTestTestHandler::COMPLETED) {
    this->RunAgain = true;
    return true;
  }
  return false;
}
void cmCTestRunTest::ComputeWeightedCost()
{
  double prev = static_cast<double>(this->TestProperties->PreviousRuns);
  double avgcost = static_cast<double>(this->TestProperties->Cost);
  double current = this->TestResult.ExecutionTime.count();

  if (this->TestResult.Status == cmCTestTestHandler::COMPLETED) {
    this->TestProperties->Cost =
      static_cast<float>(((prev * avgcost) + current) / (prev + 1.0));
    this->TestProperties->PreviousRuns++;
  }
}

void cmCTestRunTest::MemCheckPostProcess()
{
  if (!this->TestHandler->MemCheck) {
    return;
  }
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     this->Index << ": process test output now: "
                                 << this->TestProperties->Name << " "
                                 << this->TestResult.Name << std::endl,
                     this->TestHandler->GetQuiet());
  cmCTestMemCheckHandler* handler =
    static_cast<cmCTestMemCheckHandler*>(this->TestHandler);
  handler->PostProcessTest(this->TestResult, this->Index);
}

void cmCTestRunTest::StartFailure(std::string const& output)
{
  // Still need to log the Start message so the test summary records our
  // attempt to start this test
  if (!this->CTest->GetTestProgressOutput()) {
    cmCTestLog(this->CTest, HANDLER_OUTPUT,
               std::setw(2 * getNumWidth(this->TotalNumberOfTests) + 8)
                 << "Start "
                 << std::setw(getNumWidth(this->TestHandler->GetMaxIndex()))
                 << this->TestProperties->Index << ": "
                 << this->TestProperties->Name << std::endl);
  }

  this->ProcessOutput.clear();
  if (!output.empty()) {
    *this->TestHandler->LogFile << output << std::endl;
    cmCTestLog(this->CTest, ERROR_MESSAGE, output << std::endl);
  }

  this->TestResult.Properties = this->TestProperties;
  this->TestResult.ExecutionTime = cmDuration::zero();
  this->TestResult.CompressOutput = false;
  this->TestResult.ReturnValue = -1;
  this->TestResult.CompletionStatus = "Failed to start";
  this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
  this->TestResult.TestCount = this->TestProperties->Index;
  this->TestResult.Name = this->TestProperties->Name;
  this->TestResult.Path = this->TestProperties->Directory;
  this->TestResult.Output = output;
  this->TestResult.FullCommandLine.clear();
  this->TestProcess = cm::make_unique<cmProcess>(*this);
}

std::string cmCTestRunTest::GetTestPrefix(size_t completed, size_t total) const
{
  std::ostringstream outputStream;
  outputStream << std::setw(getNumWidth(total)) << completed << "/";
  outputStream << std::setw(getNumWidth(total)) << total << " ";

  if (this->TestHandler->MemCheck) {
    outputStream << "MemCheck";
  } else {
    outputStream << "Test";
  }

  std::ostringstream indexStr;
  indexStr << " #" << this->Index << ":";
  outputStream << std::setw(3 + getNumWidth(this->TestHandler->GetMaxIndex()))
               << indexStr.str();
  outputStream << " ";

  return outputStream.str();
}

// Starts the execution of a test.  Returns once it has started
bool cmCTestRunTest::StartTest(size_t completed, size_t total)
{
  this->TotalNumberOfTests = total; // save for rerun case
  if (!this->CTest->GetTestProgressOutput()) {
    cmCTestLog(this->CTest, HANDLER_OUTPUT,
               std::setw(2 * getNumWidth(total) + 8)
                 << "Start "
                 << std::setw(getNumWidth(this->TestHandler->GetMaxIndex()))
                 << this->TestProperties->Index << ": "
                 << this->TestProperties->Name << std::endl);
  } else {
    std::string testName =
      GetTestPrefix(completed, total) + this->TestProperties->Name + "\n";
    cmCTestLog(this->CTest, HANDLER_TEST_PROGRESS_OUTPUT, testName);
  }

  this->ProcessOutput.clear();

  // Return immediately if test is disabled
  if (this->TestProperties->Disabled) {
    this->TestResult.Properties = this->TestProperties;
    this->TestResult.ExecutionTime = cmDuration::zero();
    this->TestResult.CompressOutput = false;
    this->TestResult.ReturnValue = -1;
    this->TestResult.CompletionStatus = "Disabled";
    this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
    this->TestResult.TestCount = this->TestProperties->Index;
    this->TestResult.Name = this->TestProperties->Name;
    this->TestResult.Path = this->TestProperties->Directory;
    this->TestProcess = cm::make_unique<cmProcess>(*this);
    this->TestResult.Output = "Disabled";
    this->TestResult.FullCommandLine.clear();
    return false;
  }

  this->TestResult.Properties = this->TestProperties;
  this->TestResult.ExecutionTime = cmDuration::zero();
  this->TestResult.CompressOutput = false;
  this->TestResult.ReturnValue = -1;
  this->TestResult.CompletionStatus = "Failed to start";
  this->TestResult.Status = cmCTestTestHandler::BAD_COMMAND;
  this->TestResult.TestCount = this->TestProperties->Index;
  this->TestResult.Name = this->TestProperties->Name;
  this->TestResult.Path = this->TestProperties->Directory;

  // Check for failed fixture dependencies before we even look at the command
  // arguments because if we are not going to run the test, the command and
  // its arguments are irrelevant. This matters for the case where a fixture
  // dependency might be creating the executable we want to run.
  if (!this->FailedDependencies.empty()) {
    this->TestProcess = cm::make_unique<cmProcess>(*this);
    std::string msg = "Failed test dependencies:";
    for (std::string const& failedDep : this->FailedDependencies) {
      msg += " " + failedDep;
    }
    *this->TestHandler->LogFile << msg << std::endl;
    cmCTestLog(this->CTest, HANDLER_OUTPUT, msg << std::endl);
    this->TestResult.Output = msg;
    this->TestResult.FullCommandLine.clear();
    this->TestResult.CompletionStatus = "Fixture dependency failed";
    this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
    return false;
  }

  this->ComputeArguments();
  std::vector<std::string>& args = this->TestProperties->Args;
  if (args.size() >= 2 && args[1] == "NOT_AVAILABLE") {
    this->TestProcess = cm::make_unique<cmProcess>(*this);
    std::string msg;
    if (this->CTest->GetConfigType().empty()) {
      msg = "Test not available without configuration.";
      msg += "  (Missing \"-C <config>\"?)";
    } else {
      msg = "Test not available in configuration \"";
      msg += this->CTest->GetConfigType();
      msg += "\".";
    }
    *this->TestHandler->LogFile << msg << std::endl;
    cmCTestLog(this->CTest, ERROR_MESSAGE, msg << std::endl);
    this->TestResult.Output = msg;
    this->TestResult.FullCommandLine.clear();
    this->TestResult.CompletionStatus = "Missing Configuration";
    this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
    return false;
  }

  // Check if all required files exist
  for (std::string const& file : this->TestProperties->RequiredFiles) {
    if (!cmSystemTools::FileExists(file)) {
      // Required file was not found
      this->TestProcess = cm::make_unique<cmProcess>(*this);
      *this->TestHandler->LogFile << "Unable to find required file: " << file
                                  << std::endl;
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Unable to find required file: " << file << std::endl);
      this->TestResult.Output = "Unable to find required file: " + file;
      this->TestResult.FullCommandLine.clear();
      this->TestResult.CompletionStatus = "Required Files Missing";
      this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
      return false;
    }
  }
  // log and return if we did not find the executable
  if (this->ActualCommand.empty()) {
    // if the command was not found create a TestResult object
    // that has that information
    this->TestProcess = cm::make_unique<cmProcess>(*this);
    *this->TestHandler->LogFile << "Unable to find executable: " << args[1]
                                << std::endl;
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Unable to find executable: " << args[1] << std::endl);
    this->TestResult.Output = "Unable to find executable: " + args[1];
    this->TestResult.FullCommandLine.clear();
    this->TestResult.CompletionStatus = "Unable to find executable";
    this->TestResult.Status = cmCTestTestHandler::NOT_RUN;
    return false;
  }
  this->StartTime = this->CTest->CurrentTime();

  auto timeout = this->TestProperties->Timeout;

  this->TimeoutIsForStopTime = false;
  std::chrono::system_clock::time_point stop_time = this->CTest->GetStopTime();
  if (stop_time != std::chrono::system_clock::time_point()) {
    std::chrono::duration<double> stop_timeout =
      (stop_time - std::chrono::system_clock::now()) % std::chrono::hours(24);

    if (stop_timeout <= std::chrono::duration<double>::zero()) {
      stop_timeout = std::chrono::duration<double>::zero();
    }
    if (timeout == std::chrono::duration<double>::zero() ||
        stop_timeout < timeout) {
      this->TimeoutIsForStopTime = true;
      timeout = stop_timeout;
    }
  }

  return this->ForkProcess(timeout, this->TestProperties->ExplicitTimeout,
                           &this->TestProperties->Environment,
                           &this->TestProperties->Affinity);
}

void cmCTestRunTest::ComputeArguments()
{
  this->Arguments.clear(); // reset because this might be a rerun
  std::vector<std::string>::const_iterator j =
    this->TestProperties->Args.begin();
  ++j; // skip test name
  // find the test executable
  if (this->TestHandler->MemCheck) {
    cmCTestMemCheckHandler* handler =
      static_cast<cmCTestMemCheckHandler*>(this->TestHandler);
    this->ActualCommand = handler->MemoryTester;
    this->TestProperties->Args[1] = this->TestHandler->FindTheExecutable(
      this->TestProperties->Args[1].c_str());
  } else {
    this->ActualCommand = this->TestHandler->FindTheExecutable(
      this->TestProperties->Args[1].c_str());
    ++j; // skip the executable (it will be actualCommand)
  }
  std::string testCommand =
    cmSystemTools::ConvertToOutputPath(this->ActualCommand);

  // Prepends memcheck args to our command string
  this->TestHandler->GenerateTestCommand(this->Arguments, this->Index);
  for (std::string const& arg : this->Arguments) {
    testCommand += " \"";
    testCommand += arg;
    testCommand += "\"";
  }

  for (; j != this->TestProperties->Args.end(); ++j) {
    testCommand += " \"";
    testCommand += *j;
    testCommand += "\"";
    this->Arguments.push_back(*j);
  }
  this->TestResult.FullCommandLine = testCommand;

  // Print the test command in verbose mode
  cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
             std::endl
               << this->Index << ": "
               << (this->TestHandler->MemCheck ? "MemCheck" : "Test")
               << " command: " << testCommand << std::endl);

  // Print any test-specific env vars in verbose mode
  if (!this->TestProperties->Environment.empty()) {
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
               this->Index << ": "
                           << "Environment variables: " << std::endl);
  }
  for (std::string const& env : this->TestProperties->Environment) {
    cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
               this->Index << ":  " << env << std::endl);
  }
}

void cmCTestRunTest::DartProcessing()
{
  if (!this->ProcessOutput.empty() &&
      this->ProcessOutput.find("<DartMeasurement") != std::string::npos) {
    if (this->TestHandler->DartStuff.find(this->ProcessOutput)) {
      this->TestResult.DartString = this->TestHandler->DartStuff.match(1);
      // keep searching and replacing until none are left
      while (this->TestHandler->DartStuff1.find(this->ProcessOutput)) {
        // replace the exact match for the string
        cmSystemTools::ReplaceString(
          this->ProcessOutput, this->TestHandler->DartStuff1.match(1).c_str(),
          "");
      }
    }
  }
}

bool cmCTestRunTest::ForkProcess(cmDuration testTimeOut, bool explicitTimeout,
                                 std::vector<std::string>* environment,
                                 std::vector<size_t>* affinity)
{
  this->TestProcess = cm::make_unique<cmProcess>(*this);
  this->TestProcess->SetId(this->Index);
  this->TestProcess->SetWorkingDirectory(
    this->TestProperties->Directory.c_str());
  this->TestProcess->SetCommand(this->ActualCommand.c_str());
  this->TestProcess->SetCommandArguments(this->Arguments);

  // determine how much time we have
  cmDuration timeout = this->CTest->GetRemainingTimeAllowed();
  if (timeout != cmCTest::MaxDuration()) {
    timeout -= std::chrono::minutes(2);
  }
  if (this->CTest->GetTimeOut() > cmDuration::zero() &&
      this->CTest->GetTimeOut() < timeout) {
    timeout = this->CTest->GetTimeOut();
  }
  if (testTimeOut > cmDuration::zero() &&
      testTimeOut < this->CTest->GetRemainingTimeAllowed()) {
    timeout = testTimeOut;
  }
  // always have at least 1 second if we got to here
  if (timeout <= cmDuration::zero()) {
    timeout = std::chrono::seconds(1);
  }
  // handle timeout explicitly set to 0
  if (testTimeOut == cmDuration::zero() && explicitTimeout) {
    timeout = cmDuration::zero();
  }
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     this->Index << ": "
                                 << "Test timeout computed to be: "
                                 << cmDurationTo<unsigned int>(timeout)
                                 << "\n",
                     this->TestHandler->GetQuiet());

  this->TestProcess->SetTimeout(timeout);

#ifdef CMAKE_BUILD_WITH_CMAKE
  cmSystemTools::SaveRestoreEnvironment sre;
#endif

  if (environment && !environment->empty()) {
    cmSystemTools::AppendEnv(*environment);
  }

  return this->TestProcess->StartProcess(this->MultiTestHandler.Loop,
                                         affinity);
}

void cmCTestRunTest::WriteLogOutputTop(size_t completed, size_t total)
{
  std::ostringstream outputStream;

  // If this is the last or only run of this test, or progress output is
  // requested, then print out completed / total.
  // Only issue is if a test fails and we are running until fail
  // then it will never print out the completed / total, same would
  // got for run until pass.  Trick is when this is called we don't
  // yet know if we are passing or failing.
  if (this->NumberOfRunsLeft == 1 || this->CTest->GetTestProgressOutput()) {
    outputStream << std::setw(getNumWidth(total)) << completed << "/";
    outputStream << std::setw(getNumWidth(total)) << total << " ";
  }
  // if this is one of several runs of a test just print blank space
  // to keep things neat
  else {
    outputStream << std::setw(getNumWidth(total)) << "  ";
    outputStream << std::setw(getNumWidth(total)) << "  ";
  }

  if (this->TestHandler->MemCheck) {
    outputStream << "MemCheck";
  } else {
    outputStream << "Test";
  }

  std::ostringstream indexStr;
  indexStr << " #" << this->Index << ":";
  outputStream << std::setw(3 + getNumWidth(this->TestHandler->GetMaxIndex()))
               << indexStr.str();
  outputStream << " ";

  const int maxTestNameWidth = this->CTest->GetMaxTestNameWidth();
  std::string outname = this->TestProperties->Name + " ";
  outname.resize(maxTestNameWidth + 4, '.');
  outputStream << outname;

  *this->TestHandler->LogFile << this->TestProperties->Index << "/"
                              << this->TestHandler->TotalNumberOfTests
                              << " Testing: " << this->TestProperties->Name
                              << std::endl;
  *this->TestHandler->LogFile << this->TestProperties->Index << "/"
                              << this->TestHandler->TotalNumberOfTests
                              << " Test: " << this->TestProperties->Name
                              << std::endl;
  *this->TestHandler->LogFile << "Command: \"" << this->ActualCommand << "\"";

  for (std::string const& arg : this->Arguments) {
    *this->TestHandler->LogFile << " \"" << arg << "\"";
  }
  *this->TestHandler->LogFile
    << std::endl
    << "Directory: " << this->TestProperties->Directory << std::endl
    << "\"" << this->TestProperties->Name
    << "\" start time: " << this->StartTime << std::endl;

  *this->TestHandler->LogFile
    << "Output:" << std::endl
    << "----------------------------------------------------------"
    << std::endl;
  *this->TestHandler->LogFile << this->ProcessOutput << "<end of output>"
                              << std::endl;

  if (!this->CTest->GetTestProgressOutput()) {
    cmCTestLog(this->CTest, HANDLER_OUTPUT, outputStream.str());
  }

  cmCTestLog(this->CTest, DEBUG,
             "Testing " << this->TestProperties->Name << " ... ");
}

void cmCTestRunTest::FinalizeTest()
{
  this->MultiTestHandler.FinishTestProcess(this, true);
}
