/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSourceGroupCommand_h
#define cmSourceGroupCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmSourceGroupCommand
 * \brief Adds a cmSourceGroup to the cmMakefile.
 *
 * cmSourceGroupCommand is used to define cmSourceGroups which split up
 * source files in to named, organized groups in the generated makefiles.
 */
class cmSourceGroupCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmSourceGroupCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  bool processTree(const std::vector<std::string>& args,
                   std::string& errorMsg);
  bool checkTreeArgumentsPreconditions(const std::vector<std::string>& args,
                                       std::string& errorMsg) const;
};

#endif
