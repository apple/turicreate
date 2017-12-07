/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallCommand_h
#define cmInstallCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmInstallCommand
 * \brief Specifies where to install some files
 *
 * cmInstallCommand is a general-purpose interface command for
 * specifying install rules.
 */
class cmInstallCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmInstallCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  bool HandleScriptMode(std::vector<std::string> const& args);
  bool HandleTargetsMode(std::vector<std::string> const& args);
  bool HandleFilesMode(std::vector<std::string> const& args);
  bool HandleDirectoryMode(std::vector<std::string> const& args);
  bool HandleExportMode(std::vector<std::string> const& args);
  bool HandleExportAndroidMKMode(std::vector<std::string> const& args);
  bool MakeFilesFullPath(const char* modeName,
                         const std::vector<std::string>& relFiles,
                         std::vector<std::string>& absFiles);
  bool CheckCMP0006(bool& failure);

  std::string DefaultComponentName;
};

#endif
