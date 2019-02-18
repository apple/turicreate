/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFindProgramCommand_h
#define cmFindProgramCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmFindBase.h"

class cmCommand;
class cmExecutionStatus;

/** \class cmFindProgramCommand
 * \brief Define a command to search for an executable program.
 *
 * cmFindProgramCommand is used to define a CMake variable
 * that specifies an executable program. The command searches
 * in the current path (e.g., PATH environment variable) for
 * an executable that matches one of the supplied names.
 */
class cmFindProgramCommand : public cmFindBase
{
public:
  cmFindProgramCommand();
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmFindProgramCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  std::string FindProgram();
  std::string FindNormalProgram();
  std::string FindNormalProgramDirsPerName();
  std::string FindNormalProgramNamesPerDir();
  std::string FindAppBundle();
  std::string GetBundleExecutable(std::string const& bundlePath);
};

#endif
