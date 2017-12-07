/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFileCommand_h
#define cmFileCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmFileCommand
 * \brief Command for manipulation of files
 *
 */
class cmFileCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmFileCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

protected:
  bool HandleRename(std::vector<std::string> const& args);
  bool HandleRemove(std::vector<std::string> const& args, bool recurse);
  bool HandleWriteCommand(std::vector<std::string> const& args, bool append);
  bool HandleReadCommand(std::vector<std::string> const& args);
  bool HandleHashCommand(std::vector<std::string> const& args);
  bool HandleStringsCommand(std::vector<std::string> const& args);
  bool HandleGlobCommand(std::vector<std::string> const& args, bool recurse);
  bool HandleMakeDirectoryCommand(std::vector<std::string> const& args);

  bool HandleRelativePathCommand(std::vector<std::string> const& args);
  bool HandleCMakePathCommand(std::vector<std::string> const& args,
                              bool nativePath);
  bool HandleReadElfCommand(std::vector<std::string> const& args);
  bool HandleRPathChangeCommand(std::vector<std::string> const& args);
  bool HandleRPathCheckCommand(std::vector<std::string> const& args);
  bool HandleRPathRemoveCommand(std::vector<std::string> const& args);
  bool HandleDifferentCommand(std::vector<std::string> const& args);

  bool HandleCopyCommand(std::vector<std::string> const& args);
  bool HandleInstallCommand(std::vector<std::string> const& args);
  bool HandleDownloadCommand(std::vector<std::string> const& args);
  bool HandleUploadCommand(std::vector<std::string> const& args);

  bool HandleTimestampCommand(std::vector<std::string> const& args);
  bool HandleGenerateCommand(std::vector<std::string> const& args);
  bool HandleLockCommand(std::vector<std::string> const& args);

private:
  void AddEvaluationFile(const std::string& inputName,
                         const std::string& outputExpr,
                         const std::string& condition, bool inputIsContent);
};

#endif
