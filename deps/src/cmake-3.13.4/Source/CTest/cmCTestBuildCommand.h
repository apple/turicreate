/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestBuildCommand_h
#define cmCTestBuildCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCTestHandlerCommand.h"

#include <string>
#include <vector>

class cmCTestBuildHandler;
class cmCTestGenericHandler;
class cmCommand;
class cmExecutionStatus;
class cmGlobalGenerator;

/** \class cmCTestBuild
 * \brief Run a ctest script
 *
 * cmCTestBuildCommand defineds the command to build the project.
 */
class cmCTestBuildCommand : public cmCTestHandlerCommand
{
public:
  cmCTestBuildCommand();
  ~cmCTestBuildCommand() override;

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override
  {
    cmCTestBuildCommand* ni = new cmCTestBuildCommand;
    ni->CTest = this->CTest;
    ni->CTestScriptHandler = this->CTestScriptHandler;
    return ni;
  }

  /**
   * The name of the command as specified in CMakeList.txt.
   */
  std::string GetName() const override { return "ctest_build"; }

  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

  cmGlobalGenerator* GlobalGenerator;

protected:
  cmCTestBuildHandler* Handler;
  enum
  {
    ctb_BUILD = ct_LAST,
    ctb_NUMBER_ERRORS,
    ctb_NUMBER_WARNINGS,
    ctb_TARGET,
    ctb_CONFIGURATION,
    ctb_FLAGS,
    ctb_PROJECT_NAME,
    ctb_LAST
  };

  cmCTestGenericHandler* InitializeHandler() override;
};

#endif
