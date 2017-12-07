/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLinkDirectoriesCommand_h
#define cmLinkDirectoriesCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmLinkDirectoriesCommand
 * \brief Define a list of directories containing files to link.
 *
 * cmLinkDirectoriesCommand is used to specify a list
 * of directories containing files to link into executable(s).
 * Note that the command supports the use of CMake built-in variables
 * such as CMAKE_BINARY_DIR and CMAKE_SOURCE_DIR.
 */
class cmLinkDirectoriesCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmLinkDirectoriesCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  void AddLinkDir(std::string const& dir);
};

#endif
