/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmIncludeDirectoryCommand_h
#define cmIncludeDirectoryCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmIncludeDirectoryCommand
 * \brief Add include directories to the build.
 *
 * cmIncludeDirectoryCommand is used to specify directory locations
 * to search for included files.
 */
class cmIncludeDirectoryCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmIncludeDirectoryCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

protected:
  // used internally
  void GetIncludes(const std::string& arg, std::vector<std::string>& incs);
  void NormalizeInclude(std::string& inc);
};

#endif
