/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmListCommand.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <assert.h>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <stdlib.h> // required for atoi

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmListCommand::InitialPass(std::vector<std::string> const& args,
                                cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("must be called with at least two arguments.");
    return false;
  }

  const std::string& subCommand = args[0];
  if (subCommand == "LENGTH") {
    return this->HandleLengthCommand(args);
  }
  if (subCommand == "GET") {
    return this->HandleGetCommand(args);
  }
  if (subCommand == "APPEND") {
    return this->HandleAppendCommand(args);
  }
  if (subCommand == "FIND") {
    return this->HandleFindCommand(args);
  }
  if (subCommand == "INSERT") {
    return this->HandleInsertCommand(args);
  }
  if (subCommand == "REMOVE_AT") {
    return this->HandleRemoveAtCommand(args);
  }
  if (subCommand == "REMOVE_ITEM") {
    return this->HandleRemoveItemCommand(args);
  }
  if (subCommand == "REMOVE_DUPLICATES") {
    return this->HandleRemoveDuplicatesCommand(args);
  }
  if (subCommand == "SORT") {
    return this->HandleSortCommand(args);
  }
  if (subCommand == "REVERSE") {
    return this->HandleReverseCommand(args);
  }
  if (subCommand == "FILTER") {
    return this->HandleFilterCommand(args);
  }

  std::string e = "does not recognize sub-command " + subCommand;
  this->SetError(e);
  return false;
}

bool cmListCommand::GetListString(std::string& listString,
                                  const std::string& var)
{
  // get the old value
  const char* cacheValue = this->Makefile->GetDefinition(var);
  if (!cacheValue) {
    return false;
  }
  listString = cacheValue;
  return true;
}

bool cmListCommand::GetList(std::vector<std::string>& list,
                            const std::string& var)
{
  std::string listString;
  if (!this->GetListString(listString, var)) {
    return false;
  }
  // if the size of the list
  if (listString.empty()) {
    return true;
  }
  // expand the variable into a list
  cmSystemTools::ExpandListArgument(listString, list, true);
  // if no empty elements then just return
  if (std::find(list.begin(), list.end(), std::string()) == list.end()) {
    return true;
  }
  // if we have empty elements we need to check policy CMP0007
  switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0007)) {
    case cmPolicies::WARN: {
      // Default is to warn and use old behavior
      // OLD behavior is to allow compatibility, so recall
      // ExpandListArgument without the true which will remove
      // empty values
      list.clear();
      cmSystemTools::ExpandListArgument(listString, list);
      std::string warn = cmPolicies::GetPolicyWarning(cmPolicies::CMP0007);
      warn += " List has value = [";
      warn += listString;
      warn += "].";
      this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, warn);
      return true;
    }
    case cmPolicies::OLD:
      // OLD behavior is to allow compatibility, so recall
      // ExpandListArgument without the true which will remove
      // empty values
      list.clear();
      cmSystemTools::ExpandListArgument(listString, list);
      return true;
    case cmPolicies::NEW:
      return true;
    case cmPolicies::REQUIRED_IF_USED:
    case cmPolicies::REQUIRED_ALWAYS:
      this->Makefile->IssueMessage(
        cmake::FATAL_ERROR,
        cmPolicies::GetRequiredPolicyError(cmPolicies::CMP0007));
      return false;
  }
  return true;
}

bool cmListCommand::HandleLengthCommand(std::vector<std::string> const& args)
{
  if (args.size() != 3) {
    this->SetError("sub-command LENGTH requires two arguments.");
    return false;
  }

  const std::string& listName = args[1];
  const std::string& variableName = args[args.size() - 1];
  std::vector<std::string> varArgsExpanded;
  // do not check the return value here
  // if the list var is not found varArgsExpanded will have size 0
  // and we will return 0
  this->GetList(varArgsExpanded, listName);
  size_t length = varArgsExpanded.size();
  char buffer[1024];
  sprintf(buffer, "%d", static_cast<int>(length));

  this->Makefile->AddDefinition(variableName, buffer);
  return true;
}

bool cmListCommand::HandleGetCommand(std::vector<std::string> const& args)
{
  if (args.size() < 4) {
    this->SetError("sub-command GET requires at least three arguments.");
    return false;
  }

  const std::string& listName = args[1];
  const std::string& variableName = args[args.size() - 1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->Makefile->AddDefinition(variableName, "NOTFOUND");
    return true;
  }
  // FIXME: Add policy to make non-existing lists an error like empty lists.
  if (varArgsExpanded.empty()) {
    this->SetError("GET given empty list");
    return false;
  }

  std::string value;
  size_t cc;
  const char* sep = "";
  size_t nitem = varArgsExpanded.size();
  for (cc = 2; cc < args.size() - 1; cc++) {
    int item = atoi(args[cc].c_str());
    value += sep;
    sep = ";";
    if (item < 0) {
      item = (int)nitem + item;
    }
    if (item < 0 || nitem <= (size_t)item) {
      std::ostringstream str;
      str << "index: " << item << " out of range (-" << nitem << ", "
          << nitem - 1 << ")";
      this->SetError(str.str());
      return false;
    }
    value += varArgsExpanded[item];
  }

  this->Makefile->AddDefinition(variableName, value.c_str());
  return true;
}

bool cmListCommand::HandleAppendCommand(std::vector<std::string> const& args)
{
  assert(args.size() >= 2);

  // Skip if nothing to append.
  if (args.size() < 3) {
    return true;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::string listString;
  this->GetListString(listString, listName);

  if (!listString.empty() && !args.empty()) {
    listString += ";";
  }
  listString += cmJoin(cmMakeRange(args).advance(2), ";");

  this->Makefile->AddDefinition(listName, listString.c_str());
  return true;
}

bool cmListCommand::HandleFindCommand(std::vector<std::string> const& args)
{
  if (args.size() != 4) {
    this->SetError("sub-command FIND requires three arguments.");
    return false;
  }

  const std::string& listName = args[1];
  const std::string& variableName = args[args.size() - 1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->Makefile->AddDefinition(variableName, "-1");
    return true;
  }

  std::vector<std::string>::iterator it =
    std::find(varArgsExpanded.begin(), varArgsExpanded.end(), args[2]);
  if (it != varArgsExpanded.end()) {
    std::ostringstream indexStream;
    indexStream << std::distance(varArgsExpanded.begin(), it);
    this->Makefile->AddDefinition(variableName, indexStream.str().c_str());
    return true;
  }

  this->Makefile->AddDefinition(variableName, "-1");
  return true;
}

bool cmListCommand::HandleInsertCommand(std::vector<std::string> const& args)
{
  if (args.size() < 4) {
    this->SetError("sub-command INSERT requires at least three arguments.");
    return false;
  }

  const std::string& listName = args[1];

  // expand the variable
  int item = atoi(args[2].c_str());
  std::vector<std::string> varArgsExpanded;
  if ((!this->GetList(varArgsExpanded, listName) || varArgsExpanded.empty()) &&
      item != 0) {
    std::ostringstream str;
    str << "index: " << item << " out of range (0, 0)";
    this->SetError(str.str());
    return false;
  }

  if (!varArgsExpanded.empty()) {
    size_t nitem = varArgsExpanded.size();
    if (item < 0) {
      item = (int)nitem + item;
    }
    if (item < 0 || nitem <= (size_t)item) {
      std::ostringstream str;
      str << "index: " << item << " out of range (-" << varArgsExpanded.size()
          << ", "
          << (varArgsExpanded.empty() ? 0 : (varArgsExpanded.size() - 1))
          << ")";
      this->SetError(str.str());
      return false;
    }
  }

  varArgsExpanded.insert(varArgsExpanded.begin() + item, args.begin() + 3,
                         args.end());

  std::string value = cmJoin(varArgsExpanded, ";");
  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}

bool cmListCommand::HandleRemoveItemCommand(
  std::vector<std::string> const& args)
{
  if (args.size() < 3) {
    this->SetError("sub-command REMOVE_ITEM requires two or more arguments.");
    return false;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->SetError("sub-command REMOVE_ITEM requires list to be present.");
    return false;
  }

  std::vector<std::string> remove(args.begin() + 2, args.end());
  std::sort(remove.begin(), remove.end());
  std::vector<std::string>::const_iterator remEnd =
    std::unique(remove.begin(), remove.end());
  std::vector<std::string>::const_iterator remBegin = remove.begin();

  std::vector<std::string>::const_iterator argsEnd =
    cmRemoveMatching(varArgsExpanded, cmMakeRange(remBegin, remEnd));
  std::vector<std::string>::const_iterator argsBegin = varArgsExpanded.begin();
  std::string value = cmJoin(cmMakeRange(argsBegin, argsEnd), ";");
  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}

bool cmListCommand::HandleReverseCommand(std::vector<std::string> const& args)
{
  assert(args.size() >= 2);
  if (args.size() > 2) {
    this->SetError("sub-command REVERSE only takes one argument.");
    return false;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->SetError("sub-command REVERSE requires list to be present.");
    return false;
  }

  std::string value = cmJoin(cmReverseRange(varArgsExpanded), ";");

  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}

bool cmListCommand::HandleRemoveDuplicatesCommand(
  std::vector<std::string> const& args)
{
  assert(args.size() >= 2);
  if (args.size() > 2) {
    this->SetError("sub-command REMOVE_DUPLICATES only takes one argument.");
    return false;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->SetError(
      "sub-command REMOVE_DUPLICATES requires list to be present.");
    return false;
  }

  std::vector<std::string>::const_iterator argsEnd =
    cmRemoveDuplicates(varArgsExpanded);
  std::vector<std::string>::const_iterator argsBegin = varArgsExpanded.begin();
  std::string value = cmJoin(cmMakeRange(argsBegin, argsEnd), ";");

  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}

bool cmListCommand::HandleSortCommand(std::vector<std::string> const& args)
{
  assert(args.size() >= 2);
  if (args.size() > 2) {
    this->SetError("sub-command SORT only takes one argument.");
    return false;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->SetError("sub-command SORT requires list to be present.");
    return false;
  }

  std::sort(varArgsExpanded.begin(), varArgsExpanded.end());

  std::string value = cmJoin(varArgsExpanded, ";");
  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}

bool cmListCommand::HandleRemoveAtCommand(std::vector<std::string> const& args)
{
  if (args.size() < 3) {
    this->SetError("sub-command REMOVE_AT requires at least "
                   "two arguments.");
    return false;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->SetError("sub-command REMOVE_AT requires list to be present.");
    return false;
  }
  // FIXME: Add policy to make non-existing lists an error like empty lists.
  if (varArgsExpanded.empty()) {
    this->SetError("REMOVE_AT given empty list");
    return false;
  }

  size_t cc;
  std::vector<size_t> removed;
  size_t nitem = varArgsExpanded.size();
  for (cc = 2; cc < args.size(); ++cc) {
    int item = atoi(args[cc].c_str());
    if (item < 0) {
      item = (int)nitem + item;
    }
    if (item < 0 || nitem <= (size_t)item) {
      std::ostringstream str;
      str << "index: " << item << " out of range (-" << nitem << ", "
          << nitem - 1 << ")";
      this->SetError(str.str());
      return false;
    }
    removed.push_back(static_cast<size_t>(item));
  }

  std::sort(removed.begin(), removed.end());
  std::vector<size_t>::const_iterator remEnd =
    std::unique(removed.begin(), removed.end());
  std::vector<size_t>::const_iterator remBegin = removed.begin();

  std::vector<std::string>::const_iterator argsEnd =
    cmRemoveIndices(varArgsExpanded, cmMakeRange(remBegin, remEnd));
  std::vector<std::string>::const_iterator argsBegin = varArgsExpanded.begin();
  std::string value = cmJoin(cmMakeRange(argsBegin, argsEnd), ";");

  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}

bool cmListCommand::HandleFilterCommand(std::vector<std::string> const& args)
{
  if (args.size() < 2) {
    this->SetError("sub-command FILTER requires a list to be specified.");
    return false;
  }

  if (args.size() < 3) {
    this->SetError("sub-command FILTER requires an operator to be specified.");
    return false;
  }

  if (args.size() < 4) {
    this->SetError("sub-command FILTER requires a mode to be specified.");
    return false;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->SetError("sub-command FILTER requires list to be present.");
    return false;
  }

  const std::string& op = args[2];
  bool includeMatches;
  if (op == "INCLUDE") {
    includeMatches = true;
  } else if (op == "EXCLUDE") {
    includeMatches = false;
  } else {
    this->SetError("sub-command FILTER does not recognize operator " + op);
    return false;
  }

  const std::string& mode = args[3];
  if (mode == "REGEX") {
    if (args.size() != 5) {
      this->SetError("sub-command FILTER, mode REGEX "
                     "requires five arguments.");
      return false;
    }
    return this->FilterRegex(args, includeMatches, listName, varArgsExpanded);
  }

  this->SetError("sub-command FILTER does not recognize mode " + mode);
  return false;
}

class MatchesRegex
{
public:
  MatchesRegex(cmsys::RegularExpression& in_regex, bool in_includeMatches)
    : regex(in_regex)
    , includeMatches(in_includeMatches)
  {
  }

  bool operator()(const std::string& target)
  {
    return regex.find(target) ^ includeMatches;
  }

private:
  cmsys::RegularExpression& regex;
  const bool includeMatches;
};

bool cmListCommand::FilterRegex(std::vector<std::string> const& args,
                                bool includeMatches,
                                std::string const& listName,
                                std::vector<std::string>& varArgsExpanded)
{
  const std::string& pattern = args[4];
  cmsys::RegularExpression regex(pattern);
  if (!regex.is_valid()) {
    std::string error = "sub-command FILTER, mode REGEX ";
    error += "failed to compile regex \"";
    error += pattern;
    error += "\".";
    this->SetError(error);
    return false;
  }

  std::vector<std::string>::iterator argsBegin = varArgsExpanded.begin();
  std::vector<std::string>::iterator argsEnd = varArgsExpanded.end();
  std::vector<std::string>::iterator newArgsEnd =
    std::remove_if(argsBegin, argsEnd, MatchesRegex(regex, includeMatches));

  std::string value = cmJoin(cmMakeRange(argsBegin, newArgsEnd), ";");
  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}
