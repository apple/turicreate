/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWhileCommand_h
#define cmWhileCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"
#include "cmFunctionBlocker.h"
#include "cmListFileCache.h"

class cmExecutionStatus;
class cmMakefile;

class cmWhileFunctionBlocker : public cmFunctionBlocker
{
public:
  cmWhileFunctionBlocker(cmMakefile* mf);
  ~cmWhileFunctionBlocker() override;
  bool IsFunctionBlocked(const cmListFileFunction& lff, cmMakefile& mf,
                         cmExecutionStatus&) override;
  bool ShouldRemove(const cmListFileFunction& lff, cmMakefile& mf) override;

  std::vector<cmListFileArgument> Args;
  std::vector<cmListFileFunction> Functions;

private:
  cmMakefile* Makefile;
  int Depth;
};

/// \brief Starts a while loop
class cmWhileCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmWhileCommand; }

  /**
   * This overrides the default InvokeInitialPass implementation.
   * It records the arguments before expansion.
   */
  bool InvokeInitialPass(const std::vector<cmListFileArgument>& args,
                         cmExecutionStatus&) override;

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const&,
                   cmExecutionStatus&) override
  {
    return false;
  }
};

#endif
