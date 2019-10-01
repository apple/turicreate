/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmIncludeGuardCommand_h
#define cmIncludeGuardCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmIncludeGuardCommand
 * \brief cmIncludeGuardCommand identical to C++ #pragma_once command
 * Can work in 3 modes: GLOBAL (works on global properties),
 * DIRECTORY(use directory property), VARIABLE(unnamed overload without
 * arguments) define an ordinary variable to be used as include guard checker
 */
class cmIncludeGuardCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmIncludeGuardCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;
};

#endif
