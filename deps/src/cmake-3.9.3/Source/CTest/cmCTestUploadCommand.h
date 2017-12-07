/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestUploadCommand_h
#define cmCTestUploadCommand_h

#include "cmConfigure.h"

#include "cmCTest.h"
#include "cmCTestHandlerCommand.h"

#include <string>

class cmCTestGenericHandler;
class cmCommand;

/** \class cmCTestUpload
 * \brief Run a ctest script
 *
 * cmCTestUploadCommand defines the command to upload result files for
 * the project.
 */
class cmCTestUploadCommand : public cmCTestHandlerCommand
{
public:
  cmCTestUploadCommand() {}

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE
  {
    cmCTestUploadCommand* ni = new cmCTestUploadCommand;
    ni->CTest = this->CTest;
    ni->CTestScriptHandler = this->CTestScriptHandler;
    return ni;
  }

  /**
   * The name of the command as specified in CMakeList.txt.
   */
  std::string GetName() const CM_OVERRIDE { return "ctest_upload"; }

  typedef cmCTestHandlerCommand Superclass;

protected:
  cmCTestGenericHandler* InitializeHandler() CM_OVERRIDE;

  bool CheckArgumentKeyword(std::string const& arg) CM_OVERRIDE;
  bool CheckArgumentValue(std::string const& arg) CM_OVERRIDE;

  enum
  {
    ArgumentDoingFiles = Superclass::ArgumentDoingLast1,
    ArgumentDoingCaptureCMakeError,
    ArgumentDoingLast2
  };

  cmCTest::SetOfStrings Files;
};

#endif
