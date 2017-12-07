/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestMemCheckCommand_h
#define cmCTestMemCheckCommand_h

#include "cmConfigure.h"

#include "cmCTestTestCommand.h"

class cmCTestGenericHandler;
class cmCommand;

/** \class cmCTestMemCheck
 * \brief Run a ctest script
 *
 * cmCTestMemCheckCommand defineds the command to test the project.
 */
class cmCTestMemCheckCommand : public cmCTestTestCommand
{
public:
  cmCTestMemCheckCommand();

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE
  {
    cmCTestMemCheckCommand* ni = new cmCTestMemCheckCommand;
    ni->CTest = this->CTest;
    ni->CTestScriptHandler = this->CTestScriptHandler;
    return ni;
  }

protected:
  cmCTestGenericHandler* InitializeActualHandler() CM_OVERRIDE;

  void ProcessAdditionalValues(cmCTestGenericHandler* handler) CM_OVERRIDE;

  enum
  {
    ctm_DEFECT_COUNT = ctt_LAST,
    ctm_LAST
  };
};

#endif
