/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmAddSubDirectoryCommand_h
#define cmAddSubDirectoryCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmAddSubDirectoryCommand
 * \brief Specify a subdirectory to build
 *
 * cmAddSubDirectoryCommand specifies a subdirectory to process
 * by CMake. CMake will descend
 * into the specified source directory and process any CMakeLists.txt found.
 */
class cmAddSubDirectoryCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmAddSubDirectoryCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;
};

#endif
