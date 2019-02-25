/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmAddCustomCommandCommand_h
#define cmAddCustomCommandCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmAddCustomCommandCommand
 * \brief cmAddCustomCommandCommand defines a new command (rule) that can
 *  be executed within the build process
 *
 */

class cmAddCustomCommandCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmAddCustomCommandCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

protected:
  bool CheckOutputs(const std::vector<std::string>& outputs);
};

#endif
