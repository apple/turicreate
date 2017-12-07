/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallProgramsCommand_h
#define cmInstallProgramsCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmInstallProgramsCommand
 * \brief Specifies where to install some programs
 *
 * cmInstallProgramsCommand specifies the relative path where a list of
 * programs should be installed.
 */
class cmInstallProgramsCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmInstallProgramsCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

  /**
   * This is called at the end after all the information
   * specified by the command is accumulated. Most commands do
   * not implement this method.  At this point, reading and
   * writing to the cache can be done.
   */
  void FinalPass() CM_OVERRIDE;

  bool HasFinalPass() const CM_OVERRIDE { return true; }

protected:
  std::string FindInstallSource(const char* name) const;

private:
  std::vector<std::string> FinalArgs;
  std::string Destination;
  std::vector<std::string> Files;
};

#endif
