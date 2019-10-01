/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestTestCommand_h
#define cmCTestTestCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCTestHandlerCommand.h"

#include <string>

class cmCTestGenericHandler;
class cmCommand;

/** \class cmCTestTest
 * \brief Run a ctest script
 *
 * cmCTestTestCommand defineds the command to test the project.
 */
class cmCTestTestCommand : public cmCTestHandlerCommand
{
public:
  cmCTestTestCommand();

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override
  {
    cmCTestTestCommand* ni = new cmCTestTestCommand;
    ni->CTest = this->CTest;
    ni->CTestScriptHandler = this->CTestScriptHandler;
    return ni;
  }

  /**
   * The name of the command as specified in CMakeList.txt.
   */
  std::string GetName() const override { return "ctest_test"; }

protected:
  virtual cmCTestGenericHandler* InitializeActualHandler();
  cmCTestGenericHandler* InitializeHandler() override;

  enum
  {
    ctt_BUILD = ct_LAST,
    ctt_RETURN_VALUE,
    ctt_START,
    ctt_END,
    ctt_STRIDE,
    ctt_EXCLUDE,
    ctt_INCLUDE,
    ctt_EXCLUDE_LABEL,
    ctt_INCLUDE_LABEL,
    ctt_EXCLUDE_FIXTURE,
    ctt_EXCLUDE_FIXTURE_SETUP,
    ctt_EXCLUDE_FIXTURE_CLEANUP,
    ctt_PARALLEL_LEVEL,
    ctt_SCHEDULE_RANDOM,
    ctt_STOP_TIME,
    ctt_TEST_LOAD,
    ctt_LAST
  };
};

#endif
