/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmConditionEvaluator.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmSystemTools.h"

class cmCommand;
class cmTest;

static std::string const keyAND = "AND";
static std::string const keyCOMMAND = "COMMAND";
static std::string const keyDEFINED = "DEFINED";
static std::string const keyEQUAL = "EQUAL";
static std::string const keyEXISTS = "EXISTS";
static std::string const keyGREATER = "GREATER";
static std::string const keyGREATER_EQUAL = "GREATER_EQUAL";
static std::string const keyIN_LIST = "IN_LIST";
static std::string const keyIS_ABSOLUTE = "IS_ABSOLUTE";
static std::string const keyIS_DIRECTORY = "IS_DIRECTORY";
static std::string const keyIS_NEWER_THAN = "IS_NEWER_THAN";
static std::string const keyIS_SYMLINK = "IS_SYMLINK";
static std::string const keyLESS = "LESS";
static std::string const keyLESS_EQUAL = "LESS_EQUAL";
static std::string const keyMATCHES = "MATCHES";
static std::string const keyNOT = "NOT";
static std::string const keyOR = "OR";
static std::string const keyParenL = "(";
static std::string const keyParenR = ")";
static std::string const keyPOLICY = "POLICY";
static std::string const keySTREQUAL = "STREQUAL";
static std::string const keySTRGREATER = "STRGREATER";
static std::string const keySTRGREATER_EQUAL = "STRGREATER_EQUAL";
static std::string const keySTRLESS = "STRLESS";
static std::string const keySTRLESS_EQUAL = "STRLESS_EQUAL";
static std::string const keyTARGET = "TARGET";
static std::string const keyTEST = "TEST";
static std::string const keyVERSION_EQUAL = "VERSION_EQUAL";
static std::string const keyVERSION_GREATER = "VERSION_GREATER";
static std::string const keyVERSION_GREATER_EQUAL = "VERSION_GREATER_EQUAL";
static std::string const keyVERSION_LESS = "VERSION_LESS";
static std::string const keyVERSION_LESS_EQUAL = "VERSION_LESS_EQUAL";

cmConditionEvaluator::cmConditionEvaluator(cmMakefile& makefile,
                                           const cmListFileContext& context,
                                           const cmListFileBacktrace& bt)
  : Makefile(makefile)
  , ExecutionContext(context)
  , Backtrace(bt)
  , Policy12Status(makefile.GetPolicyStatus(cmPolicies::CMP0012))
  , Policy54Status(makefile.GetPolicyStatus(cmPolicies::CMP0054))
  , Policy57Status(makefile.GetPolicyStatus(cmPolicies::CMP0057))
  , Policy64Status(makefile.GetPolicyStatus(cmPolicies::CMP0064))
{
}

//=========================================================================
// order of operations,
// 1.   ( )   -- parenthetical groups
// 2.  IS_DIRECTORY EXISTS COMMAND DEFINED etc predicates
// 3. MATCHES LESS GREATER EQUAL STRLESS STRGREATER STREQUAL etc binary ops
// 4. NOT
// 5. AND OR
//
// There is an issue on whether the arguments should be values of references,
// for example IF (FOO AND BAR) should that compare the strings FOO and BAR
// or should it really do IF (${FOO} AND ${BAR}) Currently IS_DIRECTORY
// EXISTS COMMAND and DEFINED all take values. EQUAL, LESS and GREATER can
// take numeric values or variable names. STRLESS and STRGREATER take
// variable names but if the variable name is not found it will use the name
// directly. AND OR take variables or the values 0 or 1.

bool cmConditionEvaluator::IsTrue(
  const std::vector<cmExpandedCommandArgument>& args, std::string& errorString,
  cmake::MessageType& status)
{
  errorString.clear();

  // handle empty invocation
  if (args.empty()) {
    return false;
  }

  // store the reduced args in this vector
  cmArgumentList newArgs;

  // copy to the list structure
  newArgs.insert(newArgs.end(), args.begin(), args.end());

  // now loop through the arguments and see if we can reduce any of them
  // we do this multiple times. Once for each level of precedence
  // parens
  if (!this->HandleLevel0(newArgs, errorString, status)) {
    return false;
  }
  // predicates
  if (!this->HandleLevel1(newArgs, errorString, status)) {
    return false;
  }
  // binary ops
  if (!this->HandleLevel2(newArgs, errorString, status)) {
    return false;
  }

  // NOT
  if (!this->HandleLevel3(newArgs, errorString, status)) {
    return false;
  }
  // AND OR
  if (!this->HandleLevel4(newArgs, errorString, status)) {
    return false;
  }

  // now at the end there should only be one argument left
  if (newArgs.size() != 1) {
    errorString = "Unknown arguments specified";
    status = cmake::FATAL_ERROR;
    return false;
  }

  return this->GetBooleanValueWithAutoDereference(*(newArgs.begin()),
                                                  errorString, status, true);
}

//=========================================================================
const char* cmConditionEvaluator::GetDefinitionIfUnquoted(
  cmExpandedCommandArgument const& argument) const
{
  if ((this->Policy54Status != cmPolicies::WARN &&
       this->Policy54Status != cmPolicies::OLD) &&
      argument.WasQuoted()) {
    return nullptr;
  }

  const char* def = this->Makefile.GetDefinition(argument.GetValue());

  if (def && argument.WasQuoted() &&
      this->Policy54Status == cmPolicies::WARN) {
    if (!this->Makefile.HasCMP0054AlreadyBeenReported(
          this->ExecutionContext)) {
      std::ostringstream e;
      e << (cmPolicies::GetPolicyWarning(cmPolicies::CMP0054)) << "\n";
      e << "Quoted variables like \"" << argument.GetValue()
        << "\" will no longer be dereferenced "
           "when the policy is set to NEW.  "
           "Since the policy is not set the OLD behavior will be used.";

      this->Makefile.GetCMakeInstance()->IssueMessage(
        cmake::AUTHOR_WARNING, e.str(), this->Backtrace);
    }
  }

  return def;
}

//=========================================================================
const char* cmConditionEvaluator::GetVariableOrString(
  const cmExpandedCommandArgument& argument) const
{
  const char* def = this->GetDefinitionIfUnquoted(argument);

  if (!def) {
    def = argument.c_str();
  }

  return def;
}

//=========================================================================
bool cmConditionEvaluator::IsKeyword(std::string const& keyword,
                                     cmExpandedCommandArgument& argument) const
{
  if ((this->Policy54Status != cmPolicies::WARN &&
       this->Policy54Status != cmPolicies::OLD) &&
      argument.WasQuoted()) {
    return false;
  }

  bool isKeyword = argument.GetValue() == keyword;

  if (isKeyword && argument.WasQuoted() &&
      this->Policy54Status == cmPolicies::WARN) {
    if (!this->Makefile.HasCMP0054AlreadyBeenReported(
          this->ExecutionContext)) {
      std::ostringstream e;
      e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0054) << "\n";
      e << "Quoted keywords like \"" << argument.GetValue()
        << "\" will no longer be interpreted as keywords "
           "when the policy is set to NEW.  "
           "Since the policy is not set the OLD behavior will be used.";

      this->Makefile.GetCMakeInstance()->IssueMessage(
        cmake::AUTHOR_WARNING, e.str(), this->Backtrace);
    }
  }

  return isKeyword;
}

//=========================================================================
bool cmConditionEvaluator::GetBooleanValue(
  cmExpandedCommandArgument& arg) const
{
  // Check basic constants.
  if (arg == "0") {
    return false;
  }
  if (arg == "1") {
    return true;
  }

  // Check named constants.
  if (cmSystemTools::IsOn(arg.c_str())) {
    return true;
  }
  if (cmSystemTools::IsOff(arg.c_str())) {
    return false;
  }

  // Check for numbers.
  if (!arg.empty()) {
    char* end;
    double d = strtod(arg.c_str(), &end);
    if (*end == '\0') {
      // The whole string is a number.  Use C conversion to bool.
      return static_cast<bool>(d);
    }
  }

  // Check definition.
  const char* def = this->GetDefinitionIfUnquoted(arg);
  return !cmSystemTools::IsOff(def);
}

//=========================================================================
// Boolean value behavior from CMake 2.6.4 and below.
bool cmConditionEvaluator::GetBooleanValueOld(
  cmExpandedCommandArgument const& arg, bool one) const
{
  if (one) {
    // Old IsTrue behavior for single argument.
    if (arg == "0") {
      return false;
    }
    if (arg == "1") {
      return true;
    }
    const char* def = this->GetDefinitionIfUnquoted(arg);
    return !cmSystemTools::IsOff(def);
  }
  // Old GetVariableOrNumber behavior.
  const char* def = this->GetDefinitionIfUnquoted(arg);
  if (!def && atoi(arg.c_str())) {
    def = arg.c_str();
  }
  return !cmSystemTools::IsOff(def);
}

//=========================================================================
// returns the resulting boolean value
bool cmConditionEvaluator::GetBooleanValueWithAutoDereference(
  cmExpandedCommandArgument& newArg, std::string& errorString,
  cmake::MessageType& status, bool oneArg) const
{
  // Use the policy if it is set.
  if (this->Policy12Status == cmPolicies::NEW) {
    return GetBooleanValue(newArg);
  }
  if (this->Policy12Status == cmPolicies::OLD) {
    return GetBooleanValueOld(newArg, oneArg);
  }

  // Check policy only if old and new results differ.
  bool newResult = this->GetBooleanValue(newArg);
  bool oldResult = this->GetBooleanValueOld(newArg, oneArg);
  if (newResult != oldResult) {
    switch (this->Policy12Status) {
      case cmPolicies::WARN:
        errorString = "An argument named \"" + newArg.GetValue() +
          "\" appears in a conditional statement.  " +
          cmPolicies::GetPolicyWarning(cmPolicies::CMP0012);
        status = cmake::AUTHOR_WARNING;
        CM_FALLTHROUGH;
      case cmPolicies::OLD:
        return oldResult;
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS: {
        errorString = "An argument named \"" + newArg.GetValue() +
          "\" appears in a conditional statement.  " +
          cmPolicies::GetRequiredPolicyError(cmPolicies::CMP0012);
        status = cmake::FATAL_ERROR;
      }
      case cmPolicies::NEW:
        break;
    }
  }
  return newResult;
}

//=========================================================================
void cmConditionEvaluator::IncrementArguments(
  cmArgumentList& newArgs, cmArgumentList::iterator& argP1,
  cmArgumentList::iterator& argP2) const
{
  if (argP1 != newArgs.end()) {
    argP1++;
    argP2 = argP1;
    if (argP1 != newArgs.end()) {
      argP2++;
    }
  }
}

//=========================================================================
// helper function to reduce code duplication
void cmConditionEvaluator::HandlePredicate(
  bool value, int& reducible, cmArgumentList::iterator& arg,
  cmArgumentList& newArgs, cmArgumentList::iterator& argP1,
  cmArgumentList::iterator& argP2) const
{
  if (value) {
    *arg = cmExpandedCommandArgument("1", true);
  } else {
    *arg = cmExpandedCommandArgument("0", true);
  }
  newArgs.erase(argP1);
  argP1 = arg;
  this->IncrementArguments(newArgs, argP1, argP2);
  reducible = 1;
}

//=========================================================================
// helper function to reduce code duplication
void cmConditionEvaluator::HandleBinaryOp(bool value, int& reducible,
                                          cmArgumentList::iterator& arg,
                                          cmArgumentList& newArgs,
                                          cmArgumentList::iterator& argP1,
                                          cmArgumentList::iterator& argP2)
{
  if (value) {
    *arg = cmExpandedCommandArgument("1", true);
  } else {
    *arg = cmExpandedCommandArgument("0", true);
  }
  newArgs.erase(argP2);
  newArgs.erase(argP1);
  argP1 = arg;
  this->IncrementArguments(newArgs, argP1, argP2);
  reducible = 1;
}

//=========================================================================
// level 0 processes parenthetical expressions
bool cmConditionEvaluator::HandleLevel0(cmArgumentList& newArgs,
                                        std::string& errorString,
                                        cmake::MessageType& status)
{
  int reducible;
  do {
    reducible = 0;
    cmArgumentList::iterator arg = newArgs.begin();
    while (arg != newArgs.end()) {
      if (IsKeyword(keyParenL, *arg)) {
        // search for the closing paren for this opening one
        cmArgumentList::iterator argClose;
        argClose = arg;
        argClose++;
        unsigned int depth = 1;
        while (argClose != newArgs.end() && depth) {
          if (this->IsKeyword(keyParenL, *argClose)) {
            depth++;
          }
          if (this->IsKeyword(keyParenR, *argClose)) {
            depth--;
          }
          argClose++;
        }
        if (depth) {
          errorString = "mismatched parenthesis in condition";
          status = cmake::FATAL_ERROR;
          return false;
        }
        // store the reduced args in this vector
        std::vector<cmExpandedCommandArgument> newArgs2;

        // copy to the list structure
        cmArgumentList::iterator argP1 = arg;
        argP1++;
        newArgs2.insert(newArgs2.end(), argP1, argClose);
        newArgs2.pop_back();
        // now recursively invoke IsTrue to handle the values inside the
        // parenthetical expression
        bool value = this->IsTrue(newArgs2, errorString, status);
        if (value) {
          *arg = cmExpandedCommandArgument("1", true);
        } else {
          *arg = cmExpandedCommandArgument("0", true);
        }
        argP1 = arg;
        argP1++;
        // remove the now evaluated parenthetical expression
        newArgs.erase(argP1, argClose);
      }
      ++arg;
    }
  } while (reducible);
  return true;
}

//=========================================================================
// level one handles most predicates except for NOT
bool cmConditionEvaluator::HandleLevel1(cmArgumentList& newArgs, std::string&,
                                        cmake::MessageType&)
{
  int reducible;
  do {
    reducible = 0;
    cmArgumentList::iterator arg = newArgs.begin();
    cmArgumentList::iterator argP1;
    cmArgumentList::iterator argP2;
    while (arg != newArgs.end()) {
      argP1 = arg;
      this->IncrementArguments(newArgs, argP1, argP2);
      // does a file exist
      if (this->IsKeyword(keyEXISTS, *arg) && argP1 != newArgs.end()) {
        this->HandlePredicate(cmSystemTools::FileExists(argP1->c_str()),
                              reducible, arg, newArgs, argP1, argP2);
      }
      // does a directory with this name exist
      if (this->IsKeyword(keyIS_DIRECTORY, *arg) && argP1 != newArgs.end()) {
        this->HandlePredicate(cmSystemTools::FileIsDirectory(argP1->c_str()),
                              reducible, arg, newArgs, argP1, argP2);
      }
      // does a symlink with this name exist
      if (this->IsKeyword(keyIS_SYMLINK, *arg) && argP1 != newArgs.end()) {
        this->HandlePredicate(cmSystemTools::FileIsSymlink(argP1->c_str()),
                              reducible, arg, newArgs, argP1, argP2);
      }
      // is the given path an absolute path ?
      if (this->IsKeyword(keyIS_ABSOLUTE, *arg) && argP1 != newArgs.end()) {
        this->HandlePredicate(cmSystemTools::FileIsFullPath(argP1->c_str()),
                              reducible, arg, newArgs, argP1, argP2);
      }
      // does a command exist
      if (this->IsKeyword(keyCOMMAND, *arg) && argP1 != newArgs.end()) {
        cmCommand* command =
          this->Makefile.GetState()->GetCommand(argP1->c_str());
        this->HandlePredicate(command != nullptr, reducible, arg, newArgs,
                              argP1, argP2);
      }
      // does a policy exist
      if (this->IsKeyword(keyPOLICY, *arg) && argP1 != newArgs.end()) {
        cmPolicies::PolicyID pid;
        this->HandlePredicate(cmPolicies::GetPolicyID(argP1->c_str(), pid),
                              reducible, arg, newArgs, argP1, argP2);
      }
      // does a target exist
      if (this->IsKeyword(keyTARGET, *arg) && argP1 != newArgs.end()) {
        this->HandlePredicate(
          this->Makefile.FindTargetToUse(argP1->GetValue()) != nullptr,
          reducible, arg, newArgs, argP1, argP2);
      }
      // does a test exist
      if (this->Policy64Status != cmPolicies::OLD &&
          this->Policy64Status != cmPolicies::WARN) {
        if (this->IsKeyword(keyTEST, *arg) && argP1 != newArgs.end()) {
          const cmTest* haveTest = this->Makefile.GetTest(argP1->c_str());
          this->HandlePredicate(haveTest != nullptr, reducible, arg, newArgs,
                                argP1, argP2);
        }
      } else if (this->Policy64Status == cmPolicies::WARN &&
                 this->IsKeyword(keyTEST, *arg)) {
        std::ostringstream e;
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0064) << "\n";
        e << "TEST will be interpreted as an operator "
             "when the policy is set to NEW.  "
             "Since the policy is not set the OLD behavior will be used.";

        this->Makefile.IssueMessage(cmake::AUTHOR_WARNING, e.str());
      }
      // is a variable defined
      if (this->IsKeyword(keyDEFINED, *arg) && argP1 != newArgs.end()) {
        size_t argP1len = argP1->GetValue().size();
        bool bdef = false;
        if (argP1len > 4 && argP1->GetValue().substr(0, 4) == "ENV{" &&
            argP1->GetValue().operator[](argP1len - 1) == '}') {
          std::string env = argP1->GetValue().substr(4, argP1len - 5);
          bdef = cmSystemTools::HasEnv(env);
        } else {
          bdef = this->Makefile.IsDefinitionSet(argP1->GetValue());
        }
        this->HandlePredicate(bdef, reducible, arg, newArgs, argP1, argP2);
      }
      ++arg;
    }
  } while (reducible);
  return true;
}

//=========================================================================
// level two handles most binary operations except for AND  OR
bool cmConditionEvaluator::HandleLevel2(cmArgumentList& newArgs,
                                        std::string& errorString,
                                        cmake::MessageType& status)
{
  int reducible;
  std::string def_buf;
  const char* def;
  const char* def2;
  do {
    reducible = 0;
    cmArgumentList::iterator arg = newArgs.begin();
    cmArgumentList::iterator argP1;
    cmArgumentList::iterator argP2;
    while (arg != newArgs.end()) {
      argP1 = arg;
      this->IncrementArguments(newArgs, argP1, argP2);
      if (argP1 != newArgs.end() && argP2 != newArgs.end() &&
          IsKeyword(keyMATCHES, *argP1)) {
        def = this->GetVariableOrString(*arg);
        if (def != arg->c_str() // yes, we compare the pointer value
            && cmHasLiteralPrefix(arg->GetValue(), "CMAKE_MATCH_")) {
          // The string to match is owned by our match result variables.
          // Move it to our own buffer before clearing them.
          def_buf = def;
          def = def_buf.c_str();
        }
        const char* rex = argP2->c_str();
        this->Makefile.ClearMatches();
        cmsys::RegularExpression regEntry;
        if (!regEntry.compile(rex)) {
          std::ostringstream error;
          error << "Regular expression \"" << rex << "\" cannot compile";
          errorString = error.str();
          status = cmake::FATAL_ERROR;
          return false;
        }
        if (regEntry.find(def)) {
          this->Makefile.StoreMatches(regEntry);
          *arg = cmExpandedCommandArgument("1", true);
        } else {
          *arg = cmExpandedCommandArgument("0", true);
        }
        newArgs.erase(argP2);
        newArgs.erase(argP1);
        argP1 = arg;
        this->IncrementArguments(newArgs, argP1, argP2);
        reducible = 1;
      }

      if (argP1 != newArgs.end() && this->IsKeyword(keyMATCHES, *arg)) {
        *arg = cmExpandedCommandArgument("0", true);
        newArgs.erase(argP1);
        argP1 = arg;
        this->IncrementArguments(newArgs, argP1, argP2);
        reducible = 1;
      }

      if (argP1 != newArgs.end() && argP2 != newArgs.end() &&
          (this->IsKeyword(keyLESS, *argP1) ||
           this->IsKeyword(keyLESS_EQUAL, *argP1) ||
           this->IsKeyword(keyGREATER, *argP1) ||
           this->IsKeyword(keyGREATER_EQUAL, *argP1) ||
           this->IsKeyword(keyEQUAL, *argP1))) {
        def = this->GetVariableOrString(*arg);
        def2 = this->GetVariableOrString(*argP2);
        double lhs;
        double rhs;
        bool result;
        if (sscanf(def, "%lg", &lhs) != 1 || sscanf(def2, "%lg", &rhs) != 1) {
          result = false;
        } else if (*(argP1) == keyLESS) {
          result = (lhs < rhs);
        } else if (*(argP1) == keyLESS_EQUAL) {
          result = (lhs <= rhs);
        } else if (*(argP1) == keyGREATER) {
          result = (lhs > rhs);
        } else if (*(argP1) == keyGREATER_EQUAL) {
          result = (lhs >= rhs);
        } else {
          result = (lhs == rhs);
        }
        this->HandleBinaryOp(result, reducible, arg, newArgs, argP1, argP2);
      }

      if (argP1 != newArgs.end() && argP2 != newArgs.end() &&
          (this->IsKeyword(keySTRLESS, *argP1) ||
           this->IsKeyword(keySTRLESS_EQUAL, *argP1) ||
           this->IsKeyword(keySTRGREATER, *argP1) ||
           this->IsKeyword(keySTRGREATER_EQUAL, *argP1) ||
           this->IsKeyword(keySTREQUAL, *argP1))) {
        def = this->GetVariableOrString(*arg);
        def2 = this->GetVariableOrString(*argP2);
        int val = strcmp(def, def2);
        bool result;
        if (*(argP1) == keySTRLESS) {
          result = (val < 0);
        } else if (*(argP1) == keySTRLESS_EQUAL) {
          result = (val <= 0);
        } else if (*(argP1) == keySTRGREATER) {
          result = (val > 0);
        } else if (*(argP1) == keySTRGREATER_EQUAL) {
          result = (val >= 0);
        } else // strequal
        {
          result = (val == 0);
        }
        this->HandleBinaryOp(result, reducible, arg, newArgs, argP1, argP2);
      }

      if (argP1 != newArgs.end() && argP2 != newArgs.end() &&
          (this->IsKeyword(keyVERSION_LESS, *argP1) ||
           this->IsKeyword(keyVERSION_LESS_EQUAL, *argP1) ||
           this->IsKeyword(keyVERSION_GREATER, *argP1) ||
           this->IsKeyword(keyVERSION_GREATER_EQUAL, *argP1) ||
           this->IsKeyword(keyVERSION_EQUAL, *argP1))) {
        def = this->GetVariableOrString(*arg);
        def2 = this->GetVariableOrString(*argP2);
        cmSystemTools::CompareOp op;
        if (*argP1 == keyVERSION_LESS) {
          op = cmSystemTools::OP_LESS;
        } else if (*argP1 == keyVERSION_LESS_EQUAL) {
          op = cmSystemTools::OP_LESS_EQUAL;
        } else if (*argP1 == keyVERSION_GREATER) {
          op = cmSystemTools::OP_GREATER;
        } else if (*argP1 == keyVERSION_GREATER_EQUAL) {
          op = cmSystemTools::OP_GREATER_EQUAL;
        } else { // version_equal
          op = cmSystemTools::OP_EQUAL;
        }
        bool result = cmSystemTools::VersionCompare(op, def, def2);
        this->HandleBinaryOp(result, reducible, arg, newArgs, argP1, argP2);
      }

      // is file A newer than file B
      if (argP1 != newArgs.end() && argP2 != newArgs.end() &&
          this->IsKeyword(keyIS_NEWER_THAN, *argP1)) {
        int fileIsNewer = 0;
        bool success = cmSystemTools::FileTimeCompare(
          arg->GetValue(), (argP2)->GetValue(), &fileIsNewer);
        this->HandleBinaryOp(
          (!success || fileIsNewer == 1 || fileIsNewer == 0), reducible, arg,
          newArgs, argP1, argP2);
      }

      if (argP1 != newArgs.end() && argP2 != newArgs.end() &&
          this->IsKeyword(keyIN_LIST, *argP1)) {
        if (this->Policy57Status != cmPolicies::OLD &&
            this->Policy57Status != cmPolicies::WARN) {
          bool result = false;

          def = this->GetVariableOrString(*arg);
          def2 = this->Makefile.GetDefinition(argP2->GetValue());

          if (def2) {
            std::vector<std::string> list;
            cmSystemTools::ExpandListArgument(def2, list, true);

            result = std::find(list.begin(), list.end(), def) != list.end();
          }

          this->HandleBinaryOp(result, reducible, arg, newArgs, argP1, argP2);
        } else if (this->Policy57Status == cmPolicies::WARN) {
          std::ostringstream e;
          e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0057) << "\n";
          e << "IN_LIST will be interpreted as an operator "
               "when the policy is set to NEW.  "
               "Since the policy is not set the OLD behavior will be used.";

          this->Makefile.IssueMessage(cmake::AUTHOR_WARNING, e.str());
        }
      }

      ++arg;
    }
  } while (reducible);
  return true;
}

//=========================================================================
// level 3 handles NOT
bool cmConditionEvaluator::HandleLevel3(cmArgumentList& newArgs,
                                        std::string& errorString,
                                        cmake::MessageType& status)
{
  int reducible;
  do {
    reducible = 0;
    cmArgumentList::iterator arg = newArgs.begin();
    cmArgumentList::iterator argP1;
    cmArgumentList::iterator argP2;
    while (arg != newArgs.end()) {
      argP1 = arg;
      IncrementArguments(newArgs, argP1, argP2);
      if (argP1 != newArgs.end() && IsKeyword(keyNOT, *arg)) {
        bool rhs = this->GetBooleanValueWithAutoDereference(
          *argP1, errorString, status);
        this->HandlePredicate(!rhs, reducible, arg, newArgs, argP1, argP2);
      }
      ++arg;
    }
  } while (reducible);
  return true;
}

//=========================================================================
// level 4 handles AND OR
bool cmConditionEvaluator::HandleLevel4(cmArgumentList& newArgs,
                                        std::string& errorString,
                                        cmake::MessageType& status)
{
  int reducible;
  bool lhs;
  bool rhs;
  do {
    reducible = 0;
    cmArgumentList::iterator arg = newArgs.begin();
    cmArgumentList::iterator argP1;
    cmArgumentList::iterator argP2;
    while (arg != newArgs.end()) {
      argP1 = arg;
      IncrementArguments(newArgs, argP1, argP2);
      if (argP1 != newArgs.end() && IsKeyword(keyAND, *argP1) &&
          argP2 != newArgs.end()) {
        lhs =
          this->GetBooleanValueWithAutoDereference(*arg, errorString, status);
        rhs = this->GetBooleanValueWithAutoDereference(*argP2, errorString,
                                                       status);
        this->HandleBinaryOp((lhs && rhs), reducible, arg, newArgs, argP1,
                             argP2);
      }

      if (argP1 != newArgs.end() && this->IsKeyword(keyOR, *argP1) &&
          argP2 != newArgs.end()) {
        lhs =
          this->GetBooleanValueWithAutoDereference(*arg, errorString, status);
        rhs = this->GetBooleanValueWithAutoDereference(*argP2, errorString,
                                                       status);
        this->HandleBinaryOp((lhs || rhs), reducible, arg, newArgs, argP1,
                             argP2);
      }
      ++arg;
    }
  } while (reducible);
  return true;
}
