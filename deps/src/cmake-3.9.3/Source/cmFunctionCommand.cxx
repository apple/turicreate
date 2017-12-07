/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFunctionCommand.h"

#include <sstream>

#include "cmAlgorithms.h"
#include "cmExecutionStatus.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmState.h"
#include "cmSystemTools.h"

// define the class for function commands
class cmFunctionHelperCommand : public cmCommand
{
public:
  cmFunctionHelperCommand() {}

  ///! clean up any memory allocated by the function
  ~cmFunctionHelperCommand() CM_OVERRIDE {}

  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE
  {
    cmFunctionHelperCommand* newC = new cmFunctionHelperCommand;
    // we must copy when we clone
    newC->Args = this->Args;
    newC->Functions = this->Functions;
    newC->Policies = this->Policies;
    newC->FilePath = this->FilePath;
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

bool cmFunctionHelperCommand::InvokeInitialPass(
  const std::vector<cmListFileArgument>& args, cmExecutionStatus& inStatus)
{
  // Expand the argument list to the function.
  std::vector<std::string> expandedArgs;
  this->Makefile->ExpandArguments(args, expandedArgs);

  // make sure the number of arguments passed is at least the number
  // required by the signature
  if (expandedArgs.size() < this->Args.size() - 1) {
    std::string errorMsg =
      "Function invoked with incorrect arguments for function named: ";
    errorMsg += this->Args[0];
    this->SetError(errorMsg);
    return false;
  }

  cmMakefile::FunctionPushPop functionScope(this->Makefile, this->FilePath,
                                            this->Policies);

  // set the value of argc
  std::ostringstream strStream;
  strStream << expandedArgs.size();
  this->Makefile->AddDefinition("ARGC", strStream.str().c_str());
  this->Makefile->MarkVariableAsUsed("ARGC");

  // set the values for ARGV0 ARGV1 ...
  for (unsigned int t = 0; t < expandedArgs.size(); ++t) {
    std::ostringstream tmpStream;
    tmpStream << "ARGV" << t;
    this->Makefile->AddDefinition(tmpStream.str(), expandedArgs[t].c_str());
    this->Makefile->MarkVariableAsUsed(tmpStream.str());
  }

  // define the formal arguments
  for (unsigned int j = 1; j < this->Args.size(); ++j) {
    this->Makefile->AddDefinition(this->Args[j], expandedArgs[j - 1].c_str());
  }

  // define ARGV and ARGN
  std::string argvDef = cmJoin(expandedArgs, ";");
  std::vector<std::string>::const_iterator eit =
    expandedArgs.begin() + (this->Args.size() - 1);
  std::string argnDef = cmJoin(cmMakeRange(eit, expandedArgs.end()), ";");
  this->Makefile->AddDefinition("ARGV", argvDef.c_str());
  this->Makefile->MarkVariableAsUsed("ARGV");
  this->Makefile->AddDefinition("ARGN", argnDef.c_str());
  this->Makefile->MarkVariableAsUsed("ARGN");

  // Invoke all the functions that were collected in the block.
  // for each function
  for (unsigned int c = 0; c < this->Functions.size(); ++c) {
    cmExecutionStatus status;
    if (!this->Makefile->ExecuteCommand(this->Functions[c], status) ||
        status.GetNestedError()) {
      // The error message should have already included the call stack
      // so we do not need to report an error here.
      functionScope.Quiet();
      inStatus.SetNestedError();
      return false;
    }
    if (status.GetReturnInvoked()) {
      return true;
    }
  }

  // pop scope on the makefile
  return true;
}

bool cmFunctionFunctionBlocker::IsFunctionBlocked(
  const cmListFileFunction& lff, cmMakefile& mf, cmExecutionStatus&)
{
  // record commands until we hit the ENDFUNCTION
  // at the ENDFUNCTION call we shift gears and start looking for invocations
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "function")) {
    this->Depth++;
  } else if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endfunction")) {
    // if this is the endfunction for this function then execute
    if (!this->Depth) {
      // create a new command and add it to cmake
      cmFunctionHelperCommand* f = new cmFunctionHelperCommand();
      f->Args = this->Args;
      f->Functions = this->Functions;
      f->FilePath = this->GetStartingContext().FilePath;
      mf.RecordPolicies(f->Policies);
      mf.GetState()->AddScriptedCommand(this->Args[0], f);
      // remove the function blocker now that the function is defined
      mf.RemoveFunctionBlocker(this, lff);
      return true;
    }
    // decrement for each nested function that ends
    this->Depth--;
  }

  // if it wasn't an endfunction and we are not executing then we must be
  // recording
  this->Functions.push_back(lff);
  return true;
}

bool cmFunctionFunctionBlocker::ShouldRemove(const cmListFileFunction& lff,
                                             cmMakefile& mf)
{
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endfunction")) {
    std::vector<std::string> expandedArguments;
    mf.ExpandArguments(lff.Arguments, expandedArguments,
                       this->GetStartingContext().FilePath.c_str());
    // if the endfunction has arguments then make sure
    // they match the ones in the opening function command
    if ((expandedArguments.empty() ||
         (expandedArguments[0] == this->Args[0]))) {
      return true;
    }
  }

  return false;
}

bool cmFunctionCommand::InitialPass(std::vector<std::string> const& args,
                                    cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // create a function blocker
  cmFunctionFunctionBlocker* f = new cmFunctionFunctionBlocker();
  f->Args.insert(f->Args.end(), args.begin(), args.end());
  this->Makefile->AddFunctionBlocker(f);
  return true;
}
