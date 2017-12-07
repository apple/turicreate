/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFindPathCommand_h
#define cmFindPathCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmFindBase.h"

class cmCommand;
class cmExecutionStatus;

/** \class cmFindPathCommand
 * \brief Define a command to search for a library.
 *
 * cmFindPathCommand is used to define a CMake variable
 * that specifies a library. The command searches for a given
 * file in a list of directories.
 */
class cmFindPathCommand : public cmFindBase
{
public:
  cmFindPathCommand();
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmFindPathCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

  bool IncludeFileInPath;

private:
  std::string FindHeaderInFramework(std::string const& file,
                                    std::string const& dir);
  std::string FindHeader();
  std::string FindNormalHeader();
  std::string FindFrameworkHeader();
};

#endif
