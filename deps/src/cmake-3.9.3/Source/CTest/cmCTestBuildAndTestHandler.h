/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestBuildAndTestHandler_h
#define cmCTestBuildAndTestHandler_h

#include "cmConfigure.h"

#include "cmCTestGenericHandler.h"

#include <sstream>
#include <stddef.h>
#include <string>
#include <vector>

class cmake;

/** \class cmCTestBuildAndTestHandler
 * \brief A class that handles ctest -S invocations
 *
 */
class cmCTestBuildAndTestHandler : public cmCTestGenericHandler
{
public:
  typedef cmCTestGenericHandler Superclass;

  /*
   * The main entry point for this class
   */
  int ProcessHandler() CM_OVERRIDE;

  //! Set all the build and test arguments
  int ProcessCommandLineArguments(const std::string& currentArg, size_t& idx,
                                  const std::vector<std::string>& allArgs)
    CM_OVERRIDE;

  /*
   * Get the output variable
   */
  const char* GetOutput();

  cmCTestBuildAndTestHandler();

  void Initialize() CM_OVERRIDE;

protected:
  ///! Run CMake and build a test and then run it as a single test.
  int RunCMakeAndTest(std::string* output);
  int RunCMake(std::string* outstring, std::ostringstream& out,
               std::string& cmakeOutString, cmake* cm);

  std::string Output;

  std::string BuildGenerator;
  std::string BuildGeneratorPlatform;
  std::string BuildGeneratorToolset;
  std::vector<std::string> BuildOptions;
  bool BuildTwoConfig;
  std::string BuildMakeProgram;
  std::string ConfigSample;
  std::string SourceDir;
  std::string BinaryDir;
  std::string BuildProject;
  std::string TestCommand;
  bool BuildNoClean;
  std::string BuildRunDir;
  std::string ExecutableDirectory;
  std::vector<std::string> TestCommandArgs;
  std::vector<std::string> BuildTargets;
  bool BuildNoCMake;
  double Timeout;
};

#endif
