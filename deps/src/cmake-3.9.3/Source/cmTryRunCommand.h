/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTryRunCommand_h
#define cmTryRunCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCoreTryCompile.h"

class cmCommand;
class cmExecutionStatus;

/** \class cmTryRunCommand
 * \brief Specifies where to install some files
 *
 * cmTryRunCommand is used to test if soucre code can be compiled
 */
class cmTryRunCommand : public cmCoreTryCompile
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmTryRunCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

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
