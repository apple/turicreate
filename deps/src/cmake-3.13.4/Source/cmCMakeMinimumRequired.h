/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCMakeMinimumRequired_h
#define cmCMakeMinimumRequired_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmCMakeMinimumRequired
 * \brief cmake_minimum_required command
 *
 * cmCMakeMinimumRequired implements the cmake_minimum_required CMake command
 */
class cmCMakeMinimumRequired : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmCMakeMinimumRequired; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  std::vector<std::string> UnknownArguments;
  bool EnforceUnknownArguments(std::string const& version_max);
};

#endif
