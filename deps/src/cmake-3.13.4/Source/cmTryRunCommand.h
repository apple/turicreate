/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTryRunCommand_h
#define cmTryRunCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCoreTryCompile.h"

class cmCommand;
class cmExecutionStatus;

/** \class cmTryRunCommand
 * \brief Specifies where to install some files
 *
 * cmTryRunCommand is used to test if source code can be compiled
 */
class cmTryRunCommand : public cmCoreTryCompile
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmTryRunCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  void RunExecutable(const std::string& runArgs,
                     std::string* runOutputContents);
  void DoNotRunExecutable(const std::string& runArgs,
                          const std::string& srcFile,
                          std::string* runOutputContents);

  std::string CompileResultVariable;
  std::string RunResultVariable;
  std::string OutputVariable;
  std::string RunOutputVariable;
  std::string CompileOutputVariable;
};

#endif
