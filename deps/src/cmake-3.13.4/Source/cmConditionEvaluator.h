/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmConditionEvaluator_h
#define cmConditionEvaluator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <list>
#include <string>
#include <vector>

#include "cmExpandedCommandArgument.h"
#include "cmListFileCache.h"
#include "cmPolicies.h"
#include "cmake.h"

class cmMakefile;

class cmConditionEvaluator
{
public:
  typedef std::list<cmExpandedCommandArgument> cmArgumentList;

  cmConditionEvaluator(cmMakefile& makefile, cmListFileContext const& context,
                       cmListFileBacktrace const& bt);

  // this is a shared function for both If and Else to determine if the
  // arguments were valid, and if so, was the response true. If there is
  // an error, the errorString will be set.
  bool IsTrue(const std::vector<cmExpandedCommandArgument>& args,
              std::string& errorString, cmake::MessageType& status);

private:
  // Filter the given variable definition based on policy CMP0054.
  const char* GetDefinitionIfUnquoted(
    const cmExpandedCommandArgument& argument) const;

  const char* GetVariableOrString(
    const cmExpandedCommandArgument& argument) const;

  bool IsKeyword(std::string const& keyword,
                 cmExpandedCommandArgument& argument) const;

  bool GetBooleanValue(cmExpandedCommandArgument& arg) const;

  bool GetBooleanValueOld(cmExpandedCommandArgument const& arg,
                          bool one) const;

  bool GetBooleanValueWithAutoDereference(cmExpandedCommandArgument& newArg,
                                          std::string& errorString,
                                          cmake::MessageType& status,
                                          bool oneArg = false) const;

  void IncrementArguments(cmArgumentList& newArgs,
                          cmArgumentList::iterator& argP1,
                          cmArgumentList::iterator& argP2) const;

  void HandlePredicate(bool value, int& reducible,
                       cmArgumentList::iterator& arg, cmArgumentList& newArgs,
                       cmArgumentList::iterator& argP1,
                       cmArgumentList::iterator& argP2) const;

  void HandleBinaryOp(bool value, int& reducible,
                      cmArgumentList::iterator& arg, cmArgumentList& newArgs,
                      cmArgumentList::iterator& argP1,
                      cmArgumentList::iterator& argP2);

  bool HandleLevel0(cmArgumentList& newArgs, std::string& errorString,
                    cmake::MessageType& status);

  bool HandleLevel1(cmArgumentList& newArgs, std::string&,
                    cmake::MessageType&);

  bool HandleLevel2(cmArgumentList& newArgs, std::string& errorString,
                    cmake::MessageType& status);

  bool HandleLevel3(cmArgumentList& newArgs, std::string& errorString,
                    cmake::MessageType& status);

  bool HandleLevel4(cmArgumentList& newArgs, std::string& errorString,
                    cmake::MessageType& status);

  cmMakefile& Makefile;
  cmListFileContext ExecutionContext;
  cmListFileBacktrace Backtrace;
  cmPolicies::PolicyStatus Policy12Status;
  cmPolicies::PolicyStatus Policy54Status;
  cmPolicies::PolicyStatus Policy57Status;
  cmPolicies::PolicyStatus Policy64Status;
};

#endif
