/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmBuildCommand_h
#define cmBuildCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmBuildCommand
 * \brief build_command command
 *
 * cmBuildCommand implements the build_command CMake command
 */
class cmBuildCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmBuildCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

  /**
   * The primary command signature with optional, KEYWORD-based args.
   */
  virtual bool MainSignature(std::vector<std::string> const& args);

  /**
   * Legacy "exactly 2 args required" signature.
   */
  virtual bool TwoArgsSignature(std::vector<std::string> const& args);

private:
  bool IgnoreErrors() const;
};

#endif
