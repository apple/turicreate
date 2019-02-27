/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTargetIncludeDirectoriesCommand_h
#define cmTargetIncludeDirectoriesCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmTargetPropCommandBase.h"

class cmCommand;
class cmExecutionStatus;
class cmTarget;

class cmTargetIncludeDirectoriesCommand : public cmTargetPropCommandBase
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmTargetIncludeDirectoriesCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  void HandleMissingTarget(const std::string& name) override;

  bool HandleDirectContent(cmTarget* tgt,
                           const std::vector<std::string>& content,
                           bool prepend, bool system) override;
  void HandleInterfaceContent(cmTarget* tgt,
                              const std::vector<std::string>& content,
                              bool prepend, bool system) override;

  std::string Join(const std::vector<std::string>& content) override;
};

#endif
