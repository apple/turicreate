/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCMakePolicyCommand_h
#define cmCMakePolicyCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmCMakePolicyCommand
 * \brief Set how CMake should handle policies
 *
 * cmCMakePolicyCommand sets how CMake should deal with backwards
 * compatibility policies.
 */
class cmCMakePolicyCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmCMakePolicyCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  bool HandleSetMode(std::vector<std::string> const& args);
  bool HandleGetMode(std::vector<std::string> const& args);
  bool HandleVersionMode(std::vector<std::string> const& args);
};

#endif
