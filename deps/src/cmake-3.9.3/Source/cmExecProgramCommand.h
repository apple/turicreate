/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExecProgramCommand_h
#define cmExecProgramCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"
#include "cmProcessOutput.h"

class cmExecutionStatus;

/** \class cmExecProgramCommand
 * \brief Command that adds a target to the build system.
 *
 * cmExecProgramCommand adds an extra target to the build system.
 * This is useful when you would like to add special
 * targets like "install,", "clean," and so on.
 */
class cmExecProgramCommand : public cmCommand
{
public:
  typedef cmProcessOutput::Encoding Encoding;
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmExecProgramCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  static bool RunCommand(const char* command, std::string& output, int& retVal,
                         const char* directory = CM_NULLPTR,
                         bool verbose = true,
                         Encoding encoding = cmProcessOutput::Auto);
};

#endif
