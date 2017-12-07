/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestCoverageCommand_h
#define cmCTestCoverageCommand_h

#include "cmConfigure.h"

#include "cmCTestHandlerCommand.h"

#include <set>
#include <string>

class cmCTestGenericHandler;
class cmCommand;

/** \class cmCTestCoverage
 * \brief Run a ctest script
 *
 * cmCTestCoverageCommand defineds the command to test the project.
 */
class cmCTestCoverageCommand : public cmCTestHandlerCommand
{
public:
  cmCTestCoverageCommand();

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE
  {
    cmCTestCoverageCommand* ni = new cmCTestCoverageCommand;
    ni->CTest = this->CTest;
    ni->CTestScriptHandler = this->CTestScriptHandler;
    return ni;
  }

  /**
   * The name of the command as specified in CMakeList.txt.
   */
  std::string GetName() const CM_OVERRIDE { return "ctest_coverage"; }

  typedef cmCTestHandlerCommand Superclass;

protected:
  cmCTestGenericHandler* InitializeHandler() CM_OVERRIDE;

  bool CheckArgumentKeyword(std::string const& arg) CM_OVERRIDE;
  bool CheckArgumentValue(std::string const& arg) CM_OVERRIDE;

  enum
  {
    ArgumentDoingLabels = Superclass::ArgumentDoingLast1,
    ArgumentDoingLast2
  };

  bool LabelsMentioned;
  std::set<std::string> Labels;
};

#endif
