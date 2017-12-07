/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmIfCommand_h
#define cmIfCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"
#include "cmFunctionBlocker.h"
#include "cmListFileCache.h"

class cmExecutionStatus;
class cmExpandedCommandArgument;
class cmMakefile;

class cmIfFunctionBlocker : public cmFunctionBlocker
{
public:
  cmIfFunctionBlocker()
  {
    this->HasRun = false;
    this->ElseSeen = false;
    this->ScopeDepth = 0;
  }
  ~cmIfFunctionBlocker() CM_OVERRIDE {}
  bool IsFunctionBlocked(const cmListFileFunction& lff, cmMakefile& mf,
                         cmExecutionStatus&) CM_OVERRIDE;
  bool ShouldRemove(const cmListFileFunction& lff, cmMakefile& mf) CM_OVERRIDE;

  std::vector<cmListFileArgument> Args;
  std::vector<cmListFileFunction> Functions;
  bool IsBlocking;
  bool HasRun;
  bool ElseSeen;
  unsigned int ScopeDepth;
};

/// Starts an if block
class cmIfCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmIfCommand; }

  /**
   * This overrides the default InvokeInitialPass implementation.
   * It records the arguments before expansion.
   */
  bool InvokeInitialPass(const std::vector<cmListFileArgument>& args,
                         cmExecutionStatus&) CM_OVERRIDE;

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const&,
                   cmExecutionStatus&) CM_OVERRIDE
  {
    return false;
  }

  // Filter the given variable definition based on policy CMP0054.
  static const char* GetDefinitionIfUnquoted(
    const cmMakefile* mf, cmExpandedCommandArgument const& argument);
};

#endif
