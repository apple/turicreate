/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestEmptyBinaryDirectoryCommand_h
#define cmCTestEmptyBinaryDirectoryCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCTestCommand.h"

#include <string>
#include <vector>

class cmCommand;
class cmExecutionStatus;

/** \class cmCTestEmptyBinaryDirectory
 * \brief Run a ctest script
 *
 * cmLibrarysCommand defines a list of executable (i.e., test)
 * programs to create.
 */
class cmCTestEmptyBinaryDirectoryCommand : public cmCTestCommand
{
public:
  cmCTestEmptyBinaryDirectoryCommand() {}

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override
  {
    cmCTestEmptyBinaryDirectoryCommand* ni =
      new cmCTestEmptyBinaryDirectoryCommand;
    ni->CTest = this->CTest;
    ni->CTestScriptHandler = this->CTestScriptHandler;
    return ni;
  }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;
};

#endif
