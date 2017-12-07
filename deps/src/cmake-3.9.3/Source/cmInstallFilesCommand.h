/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallFilesCommand_h
#define cmInstallFilesCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmInstallFilesCommand
 * \brief Specifies where to install some files
 *
 * cmInstallFilesCommand specifies the relative path where a list of
 * files should be installed.
 */
class cmInstallFilesCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmInstallFilesCommand; }

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
  bool HasFinalPass() const CM_OVERRIDE { return !this->IsFilesForm; }

protected:
  void CreateInstallGenerator() const;
  std::string FindInstallSource(const char* name) const;

private:
  std::vector<std::string> FinalArgs;
  bool IsFilesForm;
  std::string Destination;
  std::vector<std::string> Files;
};

#endif
