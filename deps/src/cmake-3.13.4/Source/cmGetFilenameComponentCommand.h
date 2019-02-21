/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGetFilenameComponentCommand_h
#define cmGetFilenameComponentCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmGetFilenameComponentCommand
 * \brief Get a specific component of a filename.
 *
 * cmGetFilenameComponentCommand is a utility command used to get the path,
 * name, extension or name without extension of a full filename.
 */
class cmGetFilenameComponentCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmGetFilenameComponentCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;
};

#endif
