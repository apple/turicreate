/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMacroCommand.h"

#include <sstream>
#include <stdio.h>

#include "cmAlgorithms.h"
#include "cmExecutionStatus.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmState.h"
#include "cmSystemTools.h"

// define the class for macro commands
class cmMacroHelperCommand : public cmCommand
{
public:
  cmMacroHelperCommand() {}

  ///! clean up any memory allocated by the macro
  ~cmMacroHelperCommand() CM_OVERRIDE {}

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE
  {
    cmMacroHelperCommand* newC = new cmMacroHelperCommand;
    // we must copy when we clone
    newC->Args = this->Args;
    newC->Functions = this->Functions;
    newC->FilePath = this->FilePath;
    newC->Policies = this->Policies;
    return newC;
  }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InvokeInitialPass(const std::vector<cmListFileArgument>& args,
                         cmExecutionStatus&) CM_OVERRIDE;

  bool InitialPass(std::vector<std::string> const&,
                   cmExecutionStatus&) CM_OVERRIDE
  {
    return false;
  }

  std::vector<std::string> Args;
  std::vector<cmListFileFunction> Functions;
  cmPolicies::PolicyMap Policies;
  std::string FilePath;
};

bool cmMacroHelperCommand::InvokeInitialPass(
  const std::vector<cmListFileArgument>& args, cmExecutionStatus& inStatus)
{
  // Expand the argument list to the macro.
  std::vector<std::string> expandedArgs;
  this->Makefile->ExpandArguments(args, expandedArgs);

  // make sure the number of arguments passed is at least the number
  // required by the signature
  if (expandedArgs.size() < this->Args.size() - 1) {
    std::string errorMsg =
      "Macro invoked with incorrect arguments for macro named: ";
    errorMsg += this->Args[0];
    this->SetError(errorMsg);
    return false;
  }

  cmMakefile::MacroPushPop macroScope(this->Makefile, this->FilePath,
                                      this->Policies);

  // set the value of argc
  std::ostringstream argcDefStream;
  argcDefStream << expandedArgs.size();
  std::string argcDef = argcDefStream.str();

  std::vector<std::string>::const_iterator eit =
    expandedArgs.begin() + (this->Args.size() - 1);
  std::string expandedArgn = cmJoin(cmMakeRange(eit, expandedArgs.end()), ";");
  std::string expandedArgv = cmJoin(expandedArgs, ";");
  std::vector<std::string> variables;
  variables.reserve(this->Args.size() - 1);
  for (unsigned int j = 1; j < this->Args.size(); ++j) {
    variables.push_back("${" + this->Args[j] + "}");
  }
  std::vector<std::string> argVs;
  argVs.reserve(expandedArgs.size());
  char argvName[60];
  for (unsigned int j = 0; j < expandedArgs.size(); ++j) {
    sprintf(argvName, "${ARGV%i}", j);
    argVs.push_back(argvName);
  }
  // Invoke all the functions that were collected in the block.
  cmListFileFunction newLFF;
  // for each function
  for (unsigned int c = 0; c < this->Functions.size(); ++c) {
    // Replace the formal arguments and then invoke the command.
    newLFF.Arguments.clear();
    newLFF.Arguments.reserve(this->Functions[c].Arguments.size());
    newLFF.Name = this->Functions[c].Name;
    newLFF.Line = this->Functions[c].Line;

    // for each argument of the current function
    for (std::vector<cmListFileArgument>::iterator k =
           this->Functions[c].Arguments.begin();
         k != this->Functions[c].Arguments.end(); ++k) {
      cmListFileArgument arg;
      arg.Value = k->Value;
      if (k->Delim != cmListFileArgument::Bracket) {
        // replace formal arguments
        for (unsigned int j = 0; j < variables.size(); ++j) {
          cmSystemTools::ReplaceString(arg.Value, variables[j],
                                       expandedArgs[j]);
        }
        // replace argc
        cmSystemTools::ReplaceString(arg.Value, "${ARGC}", argcDef);

        cmSystemTools::ReplaceString(arg.Value, "${ARGN}", expandedArgn);
        cmSystemTools::ReplaceString(arg.Value, "${ARGV}", expandedArgv);

        // if the current argument of the current function has ${ARGV in it
        // then try replacing ARGV values
        if (arg.Value.find("${ARGV") != std::string::npos) {
          for (unsigned int t = 0; t < expandedArgs.size(); ++t) {
            cmSystemTools::ReplaceString(arg.Value, argVs[t], expandedArgs[t]);
          }
        }
      }
      arg.Delim = k->Delim;
      arg.Line = k->Line;
      newLFF.Arguments.push_back(arg);
    }
    cmExecutionStatus status;
    if (!this->Makefile->ExecuteCommand(newLFF, status) ||
        status.GetNestedError()) {
      // The error message should have already included the call stack
      // so we do not need to report an error here.
      macroScope.Quiet();
      inStatus.SetNestedError();
      return false;
    }
    if (status.GetReturnInvoked()) {
      inStatus.SetReturnInvoked();
      return true;
    }
    if (status.GetBreakInvoked()) {
      inStatus.SetBreakInvoked();
      return true;
    }
  }
  return true;
}

bool cmMacroFunctionBlocker::IsFunctionBlocked(const cmListFileFunction& lff,
                                               cmMakefile& mf,
                                               cmExecutionStatus&)
{
  // record commands until we hit the ENDMACRO
  // at the ENDMACRO call we shift gears and start looking for invocations
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "macro")) {
    this->Depth++;
  } else if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endmacro")) {
    // if this is the endmacro for this macro then execute
    if (!this->Depth) {
      mf.AppendProperty("MACROS", this->Args[0].c_str());
      // create a new command and add it to cmake
      cmMacroHelperCommand* f = new cmMacroHelperCommand();
      f->Args = this->Args;
      f->Functions = this->Functions;
      f->FilePath = this->GetStartingContext().FilePath;
      mf.RecordPolicies(f->Policies);
      mf.GetState()->AddScriptedCommand(this->Args[0], f);
      // remove the function blocker now that the macro is defined
      mf.RemoveFunctionBlocker(this, lff);
      return true;
    }
    // decrement for each nested macro that ends
    this->Depth--;
  }

  // if it wasn't an endmacro and we are not executing then we must be
  // recording
  this->Functions.push_back(lff);
  return true;
}

bool cmMacroFunctionBlocker::ShouldRemove(const cmListFileFunction& lff,
                                          cmMakefile& mf)
{
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endmacro")) {
    std::vector<std::string> expandedArguments;
    mf.ExpandArguments(lff.Arguments, expandedArguments,
                       this->GetStartingContext().FilePath.c_str());
    // if the endmacro has arguments make sure they
    // match the arguments of the macro
    if ((expandedArguments.empty() ||
         (expandedArguments[0] == this->Args[0]))) {
      return true;
    }
  }

  return false;
}

bool cmMacroCommand::InitialPass(std::vector<std::string> const& args,
                                 cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // create a function blocker
  cmMacroFunctionBlocker* f = new cmMacroFunctionBlocker();
  f->Args.insert(f->Args.end(), args.begin(), args.end());
  this->Makefile->AddFunctionBlocker(f);
  return true;
}
