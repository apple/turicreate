/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestBuildAndTestHandler.h"

#include "cmCTest.h"
#include "cmCTestTestHandler.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmWorkingDirectory.h"
#include "cmake.h"

#include "cmsys/Process.h"
#include <chrono>
#include <cstring>
#include <ratio>
#include <stdlib.h>

cmCTestBuildAndTestHandler::cmCTestBuildAndTestHandler()
{
  this->BuildTwoConfig = false;
  this->BuildNoClean = false;
  this->BuildNoCMake = false;
  this->Timeout = cmDuration::zero();
}

void cmCTestBuildAndTestHandler::Initialize()
{
  this->BuildTargets.clear();
  this->Superclass::Initialize();
}

const char* cmCTestBuildAndTestHandler::GetOutput()
{
  return this->Output.c_str();
}
int cmCTestBuildAndTestHandler::ProcessHandler()
{
  this->Output.clear();
  std::string output;
  cmSystemTools::ResetErrorOccuredFlag();
  int retv = this->RunCMakeAndTest(&this->Output);
  cmSystemTools::ResetErrorOccuredFlag();
  return retv;
}

int cmCTestBuildAndTestHandler::RunCMake(std::string* outstring,
                                         std::ostringstream& out,
                                         std::string& cmakeOutString,
                                         cmake* cm)
{
  std::vector<std::string> args;
  args.push_back(cmSystemTools::GetCMakeCommand());
  args.push_back(this->SourceDir);
  if (!this->BuildGenerator.empty()) {
    args.push_back("-G" + this->BuildGenerator);
  }
  if (!this->BuildGeneratorPlatform.empty()) {
    args.push_back("-A" + this->BuildGeneratorPlatform);
  }
  if (!this->BuildGeneratorToolset.empty()) {
    args.push_back("-T" + this->BuildGeneratorToolset);
  }

  const char* config = nullptr;
  if (!this->CTest->GetConfigType().empty()) {
    config = this->CTest->GetConfigType().c_str();
  }
#ifdef CMAKE_INTDIR
  if (!config) {
    config = CMAKE_INTDIR;
  }
#endif

  if (config) {
    args.push_back("-DCMAKE_BUILD_TYPE:STRING=" + std::string(config));
  }

  for (std::string const& opt : this->BuildOptions) {
    args.push_back(opt);
  }
  if (cm->Run(args) != 0) {
    out << "Error: cmake execution failed\n";
    out << cmakeOutString << "\n";
    if (outstring) {
      *outstring = out.str();
    } else {
      cmCTestLog(this->CTest, ERROR_MESSAGE, out.str() << std::endl);
    }
    return 1;
  }
  // do another config?
  if (this->BuildTwoConfig) {
    if (cm->Run(args) != 0) {
      out << "Error: cmake execution failed\n";
      out << cmakeOutString << "\n";
      if (outstring) {
        *outstring = out.str();
      } else {
        cmCTestLog(this->CTest, ERROR_MESSAGE, out.str() << std::endl);
      }
      return 1;
    }
  }
  out << "======== CMake output     ======\n";
  out << cmakeOutString;
  out << "======== End CMake output ======\n";
  return 0;
}

void CMakeMessageCallback(const char* m, const char* /*unused*/,
                          bool& /*unused*/, void* s)
{
  std::string* out = static_cast<std::string*>(s);
  *out += m;
  *out += "\n";
}

void CMakeProgressCallback(const char* msg, float /*unused*/, void* s)
{
  std::string* out = static_cast<std::string*>(s);
  *out += msg;
  *out += "\n";
}

void CMakeOutputCallback(const char* m, size_t len, void* s)
{
  std::string* out = static_cast<std::string*>(s);
  out->append(m, len);
}

class cmCTestBuildAndTestCaptureRAII
{
  cmake& CM;

public:
  cmCTestBuildAndTestCaptureRAII(cmake& cm, std::string& s)
    : CM(cm)
  {
    cmSystemTools::SetMessageCallback(CMakeMessageCallback, &s);
    cmSystemTools::SetStdoutCallback(CMakeOutputCallback, &s);
    cmSystemTools::SetStderrCallback(CMakeOutputCallback, &s);
    this->CM.SetProgressCallback(CMakeProgressCallback, &s);
  }
  ~cmCTestBuildAndTestCaptureRAII()
  {
    this->CM.SetProgressCallback(nullptr, nullptr);
    cmSystemTools::SetStderrCallback(nullptr, nullptr);
    cmSystemTools::SetStdoutCallback(nullptr, nullptr);
    cmSystemTools::SetMessageCallback(nullptr, nullptr);
  }
};

int cmCTestBuildAndTestHandler::RunCMakeAndTest(std::string* outstring)
{
  // if the generator and make program are not specified then it is an error
  if (this->BuildGenerator.empty()) {
    if (outstring) {
      *outstring = "--build-and-test requires that the generator "
                   "be provided using the --build-generator "
                   "command line option. ";
    }
    return 1;
  }

  cmake cm(cmake::RoleProject);
  cm.SetHomeDirectory("");
  cm.SetHomeOutputDirectory("");
  std::string cmakeOutString;
  cmCTestBuildAndTestCaptureRAII captureRAII(cm, cmakeOutString);
  static_cast<void>(captureRAII);
  std::ostringstream out;

  if (this->CTest->GetConfigType().empty() && !this->ConfigSample.empty()) {
    // use the config sample to set the ConfigType
    std::string fullPath;
    std::string resultingConfig;
    std::vector<std::string> extraPaths;
    std::vector<std::string> failed;
    fullPath = cmCTestTestHandler::FindExecutable(
      this->CTest, this->ConfigSample.c_str(), resultingConfig, extraPaths,
      failed);
    if (!fullPath.empty() && !resultingConfig.empty()) {
      this->CTest->SetConfigType(resultingConfig.c_str());
    }
    out << "Using config sample with results: " << fullPath << " and "
        << resultingConfig << std::endl;
  }

  // we need to honor the timeout specified, the timeout include cmake, build
  // and test time
  auto clock_start = std::chrono::steady_clock::now();

  // make sure the binary dir is there
  out << "Internal cmake changing into directory: " << this->BinaryDir
      << std::endl;
  if (!cmSystemTools::FileIsDirectory(this->BinaryDir)) {
    cmSystemTools::MakeDirectory(this->BinaryDir);
  }
  cmWorkingDirectory workdir(this->BinaryDir);
  if (workdir.Failed()) {
    auto msg = "Failed to change working directory to " + this->BinaryDir +
      " : " + std::strerror(workdir.GetLastResult()) + "\n";
    if (outstring) {
      *outstring = msg;
    } else {
      cmCTestLog(this->CTest, ERROR_MESSAGE, msg);
    }
    return 1;
  }

  if (this->BuildNoCMake) {
    // Make the generator available for the Build call below.
    cmGlobalGenerator* gen = cm.CreateGlobalGenerator(this->BuildGenerator);
    cm.SetGlobalGenerator(gen);
    if (!this->BuildGeneratorPlatform.empty()) {
      cmMakefile mf(gen, cm.GetCurrentSnapshot());
      if (!gen->SetGeneratorPlatform(this->BuildGeneratorPlatform, &mf)) {
        return 1;
      }
    }

    // Load the cache to make CMAKE_MAKE_PROGRAM available.
    cm.LoadCache(this->BinaryDir);
  } else {
    // do the cmake step, no timeout here since it is not a sub process
    if (this->RunCMake(outstring, out, cmakeOutString, &cm)) {
      return 1;
    }
  }

  // do the build
  if (this->BuildTargets.empty()) {
    this->BuildTargets.push_back("");
  }
  for (std::string const& tar : this->BuildTargets) {
    cmDuration remainingTime = std::chrono::seconds(0);
    if (this->Timeout > cmDuration::zero()) {
      remainingTime =
        this->Timeout - (std::chrono::steady_clock::now() - clock_start);
      if (remainingTime <= std::chrono::seconds(0)) {
        if (outstring) {
          *outstring = "--build-and-test timeout exceeded. ";
        }
        return 1;
      }
    }
    std::string output;
    const char* config = nullptr;
    if (!this->CTest->GetConfigType().empty()) {
      config = this->CTest->GetConfigType().c_str();
    }
#ifdef CMAKE_INTDIR
    if (!config) {
      config = CMAKE_INTDIR;
    }
#endif
    if (!config) {
      config = "Debug";
    }
    int retVal = cm.GetGlobalGenerator()->Build(
      cmake::NO_BUILD_PARALLEL_LEVEL, this->SourceDir, this->BinaryDir,
      this->BuildProject, tar, output, this->BuildMakeProgram, config,
      !this->BuildNoClean, false, false, remainingTime);
    out << output;
    // if the build failed then return
    if (retVal) {
      if (outstring) {
        *outstring = out.str();
      }
      return 1;
    }
  }
  if (outstring) {
    *outstring = out.str();
  }

  // if no test was specified then we are done
  if (this->TestCommand.empty()) {
    return 0;
  }

  // now run the compiled test if we can find it
  // store the final location in fullPath
  std::string fullPath;
  std::string resultingConfig;
  std::vector<std::string> extraPaths;
  // if this->ExecutableDirectory is set try that as well
  if (!this->ExecutableDirectory.empty()) {
    std::string tempPath = this->ExecutableDirectory;
    tempPath += "/";
    tempPath += this->TestCommand;
    extraPaths.push_back(tempPath);
  }
  std::vector<std::string> failed;
  fullPath =
    cmCTestTestHandler::FindExecutable(this->CTest, this->TestCommand.c_str(),
                                       resultingConfig, extraPaths, failed);

  if (!cmSystemTools::FileExists(fullPath)) {
    out << "Could not find path to executable, perhaps it was not built: "
        << this->TestCommand << "\n";
    out << "tried to find it in these places:\n";
    out << fullPath << "\n";
    for (std::string const& fail : failed) {
      out << fail << "\n";
    }
    if (outstring) {
      *outstring = out.str();
    } else {
      cmCTestLog(this->CTest, ERROR_MESSAGE, out.str());
    }
    return 1;
  }

  std::vector<const char*> testCommand;
  testCommand.push_back(fullPath.c_str());
  for (std::string const& testCommandArg : this->TestCommandArgs) {
    testCommand.push_back(testCommandArg.c_str());
  }
  testCommand.push_back(nullptr);
  std::string outs;
  int retval = 0;
  // run the test from the this->BuildRunDir if set
  if (!this->BuildRunDir.empty()) {
    out << "Run test in directory: " << this->BuildRunDir << "\n";
    if (!workdir.SetDirectory(this->BuildRunDir)) {
      out << "Failed to change working directory : "
          << std::strerror(workdir.GetLastResult()) << "\n";
      if (outstring) {
        *outstring = out.str();
      } else {
        cmCTestLog(this->CTest, ERROR_MESSAGE, out.str());
      }
      return 1;
    }
  }
  out << "Running test command: \"" << fullPath << "\"";
  for (std::string const& testCommandArg : this->TestCommandArgs) {
    out << " \"" << testCommandArg << "\"";
  }
  out << "\n";

  // how much time is remaining
  cmDuration remainingTime = std::chrono::seconds(0);
  if (this->Timeout > cmDuration::zero()) {
    remainingTime =
      this->Timeout - (std::chrono::steady_clock::now() - clock_start);
    if (remainingTime <= std::chrono::seconds(0)) {
      if (outstring) {
        *outstring = "--build-and-test timeout exceeded. ";
      }
      return 1;
    }
  }

  int runTestRes = this->CTest->RunTest(testCommand, &outs, &retval, nullptr,
                                        remainingTime, nullptr);

  if (runTestRes != cmsysProcess_State_Exited || retval != 0) {
    out << "Test command failed: " << testCommand[0] << "\n";
    retval = 1;
  }

  out << outs << "\n";
  if (outstring) {
    *outstring = out.str();
  } else {
    cmCTestLog(this->CTest, OUTPUT, out.str() << std::endl);
  }
  return retval;
}

int cmCTestBuildAndTestHandler::ProcessCommandLineArguments(
  const std::string& currentArg, size_t& idx,
  const std::vector<std::string>& allArgs)
{
  // --build-and-test options
  if (currentArg.find("--build-and-test", 0) == 0 &&
      idx < allArgs.size() - 1) {
    if (idx + 2 < allArgs.size()) {
      idx++;
      this->SourceDir = allArgs[idx];
      idx++;
      this->BinaryDir = allArgs[idx];
      // dir must exist before CollapseFullPath is called
      cmSystemTools::MakeDirectory(this->BinaryDir);
      this->BinaryDir = cmSystemTools::CollapseFullPath(this->BinaryDir);
      this->SourceDir = cmSystemTools::CollapseFullPath(this->SourceDir);
    } else {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "--build-and-test must have source and binary dir"
                   << std::endl);
      return 0;
    }
  }
  if (currentArg.find("--build-target", 0) == 0 && idx < allArgs.size() - 1) {
    idx++;
    this->BuildTargets.push_back(allArgs[idx]);
  }
  if (currentArg.find("--build-nocmake", 0) == 0) {
    this->BuildNoCMake = true;
  }
  if (currentArg.find("--build-run-dir", 0) == 0 && idx < allArgs.size() - 1) {
    idx++;
    this->BuildRunDir = allArgs[idx];
  }
  if (currentArg.find("--build-two-config", 0) == 0) {
    this->BuildTwoConfig = true;
  }
  if (currentArg.find("--build-exe-dir", 0) == 0 && idx < allArgs.size() - 1) {
    idx++;
    this->ExecutableDirectory = allArgs[idx];
  }
  if (currentArg.find("--test-timeout", 0) == 0 && idx < allArgs.size() - 1) {
    idx++;
    this->Timeout = cmDuration(atof(allArgs[idx].c_str()));
  }
  if (currentArg == "--build-generator" && idx < allArgs.size() - 1) {
    idx++;
    this->BuildGenerator = allArgs[idx];
  }
  if (currentArg == "--build-generator-platform" && idx < allArgs.size() - 1) {
    idx++;
    this->BuildGeneratorPlatform = allArgs[idx];
  }
  if (currentArg == "--build-generator-toolset" && idx < allArgs.size() - 1) {
    idx++;
    this->BuildGeneratorToolset = allArgs[idx];
  }
  if (currentArg.find("--build-project", 0) == 0 && idx < allArgs.size() - 1) {
    idx++;
    this->BuildProject = allArgs[idx];
  }
  if (currentArg.find("--build-makeprogram", 0) == 0 &&
      idx < allArgs.size() - 1) {
    idx++;
    this->BuildMakeProgram = allArgs[idx];
  }
  if (currentArg.find("--build-config-sample", 0) == 0 &&
      idx < allArgs.size() - 1) {
    idx++;
    this->ConfigSample = allArgs[idx];
  }
  if (currentArg.find("--build-noclean", 0) == 0) {
    this->BuildNoClean = true;
  }
  if (currentArg.find("--build-options", 0) == 0) {
    while (idx + 1 < allArgs.size() && allArgs[idx + 1] != "--build-target" &&
           allArgs[idx + 1] != "--test-command") {
      ++idx;
      this->BuildOptions.push_back(allArgs[idx]);
    }
  }
  if (currentArg.find("--test-command", 0) == 0 && idx < allArgs.size() - 1) {
    ++idx;
    this->TestCommand = allArgs[idx];
    while (idx + 1 < allArgs.size()) {
      ++idx;
      this->TestCommandArgs.push_back(allArgs[idx]);
    }
  }
  return 1;
}
