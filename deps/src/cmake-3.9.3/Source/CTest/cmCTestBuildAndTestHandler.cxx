/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestBuildAndTestHandler.h"

#include "cmCTest.h"
#include "cmCTestTestHandler.h"
#include "cmGlobalGenerator.h"
#include "cmSystemTools.h"
#include "cmWorkingDirectory.h"
#include "cmake.h"

#include "cmsys/Process.h"
#include <stdlib.h>

cmCTestBuildAndTestHandler::cmCTestBuildAndTestHandler()
{
  this->BuildTwoConfig = false;
  this->BuildNoClean = false;
  this->BuildNoCMake = false;
  this->Timeout = 0;
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
  this->Output = "";
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
  unsigned int k;
  std::vector<std::string> args;
  args.push_back(cmSystemTools::GetCMakeCommand());
  args.push_back(this->SourceDir);
  if (!this->BuildGenerator.empty()) {
    std::string generator = "-G";
    generator += this->BuildGenerator;
    args.push_back(generator);
  }
  if (!this->BuildGeneratorPlatform.empty()) {
    std::string platform = "-A";
    platform += this->BuildGeneratorPlatform;
    args.push_back(platform);
  }
  if (!this->BuildGeneratorToolset.empty()) {
    std::string toolset = "-T";
    toolset += this->BuildGeneratorToolset;
    args.push_back(toolset);
  }

  const char* config = CM_NULLPTR;
  if (!this->CTest->GetConfigType().empty()) {
    config = this->CTest->GetConfigType().c_str();
  }
#ifdef CMAKE_INTDIR
  if (!config) {
    config = CMAKE_INTDIR;
  }
#endif

  if (config) {
    std::string btype = "-DCMAKE_BUILD_TYPE:STRING=" + std::string(config);
    args.push_back(btype);
  }

  for (k = 0; k < this->BuildOptions.size(); ++k) {
    args.push_back(this->BuildOptions[k]);
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
  std::string* out = (std::string*)s;
  *out += m;
  *out += "\n";
}

void CMakeProgressCallback(const char* msg, float /*unused*/, void* s)
{
  std::string* out = (std::string*)s;
  *out += msg;
  *out += "\n";
}

void CMakeOutputCallback(const char* m, size_t len, void* s)
{
  std::string* out = (std::string*)s;
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
    this->CM.SetProgressCallback(CM_NULLPTR, CM_NULLPTR);
    cmSystemTools::SetStderrCallback(CM_NULLPTR, CM_NULLPTR);
    cmSystemTools::SetStdoutCallback(CM_NULLPTR, CM_NULLPTR);
    cmSystemTools::SetMessageCallback(CM_NULLPTR, CM_NULLPTR);
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
  double clock_start = cmSystemTools::GetTime();

  // make sure the binary dir is there
  out << "Internal cmake changing into directory: " << this->BinaryDir
      << std::endl;
  if (!cmSystemTools::FileIsDirectory(this->BinaryDir)) {
    cmSystemTools::MakeDirectory(this->BinaryDir.c_str());
  }
  cmWorkingDirectory workdir(this->BinaryDir);

  if (this->BuildNoCMake) {
    // Make the generator available for the Build call below.
    cm.SetGlobalGenerator(cm.CreateGlobalGenerator(this->BuildGenerator));
    cm.SetGeneratorPlatform(this->BuildGeneratorPlatform);
    cm.SetGeneratorToolset(this->BuildGeneratorToolset);

    // Load the cache to make CMAKE_MAKE_PROGRAM available.
    cm.LoadCache(this->BinaryDir);
  } else {
    // do the cmake step, no timeout here since it is not a sub process
    if (this->RunCMake(outstring, out, cmakeOutString, &cm)) {
      return 1;
    }
  }

  // do the build
  std::vector<std::string>::iterator tarIt;
  if (this->BuildTargets.empty()) {
    this->BuildTargets.push_back("");
  }
  for (tarIt = this->BuildTargets.begin(); tarIt != this->BuildTargets.end();
       ++tarIt) {
    double remainingTime = 0;
    if (this->Timeout > 0) {
      remainingTime = this->Timeout - cmSystemTools::GetTime() + clock_start;
      if (remainingTime <= 0) {
        if (outstring) {
          *outstring = "--build-and-test timeout exceeded. ";
        }
        return 1;
      }
    }
    std::string output;
    const char* config = CM_NULLPTR;
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
      this->SourceDir, this->BinaryDir, this->BuildProject, *tarIt, output,
      this->BuildMakeProgram, config, !this->BuildNoClean, false, false,
      remainingTime);
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

  if (!cmSystemTools::FileExists(fullPath.c_str())) {
    out << "Could not find path to executable, perhaps it was not built: "
        << this->TestCommand << "\n";
    out << "tried to find it in these places:\n";
    out << fullPath << "\n";
    for (unsigned int i = 0; i < failed.size(); ++i) {
      out << failed[i] << "\n";
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
  for (size_t k = 0; k < this->TestCommandArgs.size(); ++k) {
    testCommand.push_back(this->TestCommandArgs[k].c_str());
  }
  testCommand.push_back(CM_NULLPTR);
  std::string outs;
  int retval = 0;
  // run the test from the this->BuildRunDir if set
  if (!this->BuildRunDir.empty()) {
    out << "Run test in directory: " << this->BuildRunDir << "\n";
    cmSystemTools::ChangeDirectory(this->BuildRunDir);
  }
  out << "Running test command: \"" << fullPath << "\"";
  for (size_t k = 0; k < this->TestCommandArgs.size(); ++k) {
    out << " \"" << this->TestCommandArgs[k] << "\"";
  }
  out << "\n";

  // how much time is remaining
  double remainingTime = 0;
  if (this->Timeout > 0) {
    remainingTime = this->Timeout - cmSystemTools::GetTime() + clock_start;
    if (remainingTime <= 0) {
      if (outstring) {
        *outstring = "--build-and-test timeout exceeded. ";
      }
      return 1;
    }
  }

  int runTestRes = this->CTest->RunTest(testCommand, &outs, &retval,
                                        CM_NULLPTR, remainingTime, CM_NULLPTR);

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
      cmSystemTools::MakeDirectory(this->BinaryDir.c_str());
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
    this->Timeout = atof(allArgs[idx].c_str());
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
