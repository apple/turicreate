/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmAddCustomTargetCommand_h
#define cmAddCustomTargetCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmAddCustomTargetCommand
 * \brief Command that adds a target to the build system.
 *
 * cmAddCustomTargetCommand adds an extra target to the build system.
 * This is useful when you would like to add special
 * targets like "install,", "clean," and so on.
 */
class cmAddCustomTargetCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmAddCustomTargetCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;
};

#endif
