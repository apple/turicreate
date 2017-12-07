/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmForEachCommand.h"

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "cmExecutionStatus.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

cmForEachFunctionBlocker::cmForEachFunctionBlocker(cmMakefile* mf)
  : Makefile(mf)
  , Depth(0)
{
  this->Makefile->PushLoopBlock();
}

cmForEachFunctionBlocker::~cmForEachFunctionBlocker()
{
  this->Makefile->PopLoopBlock();
}

bool cmForEachFunctionBlocker::IsFunctionBlocked(const cmListFileFunction& lff,
                                                 cmMakefile& mf,
                                                 cmExecutionStatus& inStatus)
{
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "foreach")) {
    // record the number of nested foreach commands
    this->Depth++;
  } else if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endforeach")) {
    // if this is the endofreach for this statement
    if (!this->Depth) {
      // Remove the function blocker for this scope or bail.
      CM_AUTO_PTR<cmFunctionBlocker> fb(mf.RemoveFunctionBlocker(this, lff));
      if (!fb.get()) {
        return false;
      }

      // at end of for each execute recorded commands
      // store the old value
      std::string oldDef;
      if (mf.GetDefinition(this->Args[0])) {
        oldDef = mf.GetDefinition(this->Args[0]);
      }
      std::vector<std::string>::const_iterator j = this->Args.begin();
      ++j;

      for (; j != this->Args.end(); ++j) {
        // set the variable to the loop value
        mf.AddDefinition(this->Args[0], j->c_str());
        // Invoke all the functions that were collected in the block.
        cmExecutionStatus status;
        for (unsigned int c = 0; c < this->Functions.size(); ++c) {
          status.Clear();
          mf.ExecuteCommand(this->Functions[c], status);
          if (status.GetReturnInvoked()) {
            inStatus.SetReturnInvoked();
            // restore the variable to its prior value
            mf.AddDefinition(this->Args[0], oldDef.c_str());
            return true;
          }
          if (status.GetBreakInvoked()) {
            // restore the variable to its prior value
            mf.AddDefinition(this->Args[0], oldDef.c_str());
            return true;
          }
          if (status.GetContinueInvoked()) {
            break;
          }
          if (cmSystemTools::GetFatalErrorOccured()) {
            return true;
          }
        }
      }

      // restore the variable to its prior value
      mf.AddDefinition(this->Args[0], oldDef.c_str());
      return true;
    }
    // close out a nested foreach
    this->Depth--;
  }

  // record the command
  this->Functions.push_back(lff);

  // always return true
  return true;
}

bool cmForEachFunctionBlocker::ShouldRemove(const cmListFileFunction& lff,
                                            cmMakefile& mf)
{
  if (!cmSystemTools::Strucmp(lff.Name.c_str(), "endforeach")) {
    std::vector<std::string> expandedArguments;
    mf.ExpandArguments(lff.Arguments, expandedArguments);
    // if the endforeach has arguments then make sure
    // they match the begin foreach arguments
    if ((expandedArguments.empty() ||
         (expandedArguments[0] == this->Args[0]))) {
      return true;
    }
  }
  return false;
}

bool cmForEachCommand::InitialPass(std::vector<std::string> const& args,
                                   cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  if (args.size() > 1 && args[1] == "IN") {
    return this->HandleInMode(args);
  }

  // create a function blocker
  cmForEachFunctionBlocker* f = new cmForEachFunctionBlocker(this->Makefile);
  if (args.size() > 1) {
    if (args[1] == "RANGE") {
      int start = 0;
      int stop = 0;
      int step = 0;
      if (args.size() == 3) {
        stop = atoi(args[2].c_str());
      }
      if (args.size() == 4) {
        start = atoi(args[2].c_str());
        stop = atoi(args[3].c_str());
      }
      if (args.size() == 5) {
        start = atoi(args[2].c_str());
        stop = atoi(args[3].c_str());
        step = atoi(args[4].c_str());
      }
      if (step == 0) {
        if (start > stop) {
          step = -1;
        } else {
          step = 1;
        }
      }
      if ((start > stop && step > 0) || (start < stop && step < 0) ||
          step == 0) {
        std::ostringstream str;
        str << "called with incorrect range specification: start ";
        str << start << ", stop " << stop << ", step " << step;
        this->SetError(str.str());
        return false;
      }
      std::vector<std::string> range;
      char buffer[100];
      range.push_back(args[0]);
      int cc;
      for (cc = start;; cc += step) {
        if ((step > 0 && cc > stop) || (step < 0 && cc < stop)) {
          break;
        }
        sprintf(buffer, "%d", cc);
        range.push_back(buffer);
        if (cc == stop) {
          break;
        }
      }
      f->Args = range;
    } else {
      f->Args = args;
    }
  } else {
    f->Args = args;
  }
  this->Makefile->AddFunctionBlocker(f);

  return true;
}

bool cmForEachCommand::HandleInMode(std::vector<std::string> const& args)
{
  CM_AUTO_PTR<cmForEachFunctionBlocker> f(
    new cmForEachFunctionBlocker(this->Makefile));
  f->Args.push_back(args[0]);

  enum Doing
  {
    DoingNone,
    DoingLists,
    DoingItems
  };
  Doing doing = DoingNone;
  for (unsigned int i = 2; i < args.size(); ++i) {
    if (doing == DoingItems) {
      f->Args.push_back(args[i]);
    } else if (args[i] == "LISTS") {
      doing = DoingLists;
    } else if (args[i] == "ITEMS") {
      doing = DoingItems;
    } else if (doing == DoingLists) {
      const char* value = this->Makefile->GetDefinition(args[i]);
      if (value && *value) {
        cmSystemTools::ExpandListArgument(value, f->Args, true);
      }
    } else {
      std::ostringstream e;
      e << "Unknown argument:\n"
        << "  " << args[i] << "\n";
      this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
      return true;
    }
  }

  this->Makefile->AddFunctionBlocker(f.release()); // TODO: pass auto_ptr

  return true;
}
