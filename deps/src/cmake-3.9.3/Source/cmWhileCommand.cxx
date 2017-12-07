/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWhileCommand.h"

#include "cmConditionEvaluator.h"
#include "cmExecutionStatus.h"
#include "cmExpandedCommandArgument.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

cmWhileFunctionBlocker::cmWhileFunctionBlocker(cmMakefile* mf)
  : Makefile(mf)
  , Depth(0)
{
  this->Makefile->PushLoopBlock();
}

cmWhileFunctionBlocker::~cmWhileFunctionBlocker()
{
  this->Makefile->PopLoopBlock();
}

bool cmWhileFunctionBlocker::IsFunctionBlocked(const cmListFileFunction& lff,
                                               cmMakefile& mf,
                                               cmExecutionStatus& inStatus)
{
  // at end of for each execute recorded commands
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "while")) {
    // record the number of while commands past this one
    this->Depth++;
  } else if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endwhile")) {
    // if this is the endwhile for this while loop then execute
    if (!this->Depth) {
      // Remove the function blocker for this scope or bail.
      CM_AUTO_PTR<cmFunctionBlocker> fb(mf.RemoveFunctionBlocker(this, lff));
      if (!fb.get()) {
        return false;
      }

      std::string errorString;

      std::vector<cmExpandedCommandArgument> expandedArguments;
      mf.ExpandArguments(this->Args, expandedArguments);
      cmake::MessageType messageType;

      cmListFileContext execContext = this->GetStartingContext();

      cmCommandContext commandContext;
      commandContext.Line = execContext.Line;
      commandContext.Name = execContext.Name;

      cmConditionEvaluator conditionEvaluator(mf, this->GetStartingContext(),
                                              mf.GetBacktrace(commandContext));

      bool isTrue =
        conditionEvaluator.IsTrue(expandedArguments, errorString, messageType);

      while (isTrue) {
        if (!errorString.empty()) {
          std::string err = "had incorrect arguments: ";
          unsigned int i;
          for (i = 0; i < this->Args.size(); ++i) {
            err += (this->Args[i].Delim ? "\"" : "");
            err += this->Args[i].Value;
            err += (this->Args[i].Delim ? "\"" : "");
            err += " ";
          }
          err += "(";
          err += errorString;
          err += ").";
          mf.IssueMessage(messageType, err);
          if (messageType == cmake::FATAL_ERROR) {
            cmSystemTools::SetFatalErrorOccured();
            return true;
          }
        }

        // Invoke all the functions that were collected in the block.
        for (unsigned int c = 0; c < this->Functions.size(); ++c) {
          cmExecutionStatus status;
          mf.ExecuteCommand(this->Functions[c], status);
          if (status.GetReturnInvoked()) {
            inStatus.SetReturnInvoked();
            return true;
          }
          if (status.GetBreakInvoked()) {
            return true;
          }
          if (status.GetContinueInvoked()) {
            break;
          }
          if (cmSystemTools::GetFatalErrorOccured()) {
            return true;
          }
        }
        expandedArguments.clear();
        mf.ExpandArguments(this->Args, expandedArguments);
        isTrue = conditionEvaluator.IsTrue(expandedArguments, errorString,
                                           messageType);
      }
      return true;
    }
    // decrement for each nested while that ends
    this->Depth--;
  }

  // record the command
  this->Functions.push_back(lff);

  // always return true
  return true;
}

bool cmWhileFunctionBlocker::ShouldRemove(const cmListFileFunction& lff,
                                          cmMakefile&)
{
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endwhile")) {
    // if the endwhile has arguments, then make sure
    // they match the arguments of the matching while
    if (lff.Arguments.empty() || lff.Arguments == this->Args) {
      return true;
    }
  }
  return false;
}

bool cmWhileCommand::InvokeInitialPass(
  const std::vector<cmListFileArgument>& args, cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // create a function blocker
  cmWhileFunctionBlocker* f = new cmWhileFunctionBlocker(this->Makefile);
  f->Args = args;
  this->Makefile->AddFunctionBlocker(f);

  return true;
}
