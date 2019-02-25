/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmListCommand.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <assert.h>
#include <functional>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h> // required for atoi

#include "cmAlgorithms.h"
#include "cmGeneratorExpression.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmStringReplaceHelper.h"
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
  if (subCommand == "JOIN") {
    return this->HandleJoinCommand(args);
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
  if (subCommand == "TRANSFORM") {
    return this->HandleTransformCommand(args);
  }
  if (subCommand == "SORT") {
    return this->HandleSortCommand(args);
  }
  if (subCommand == "SUBLIST") {
    return this->HandleSublistCommand(args);
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
      item = static_cast<int>(nitem) + item;
    }
    if (item < 0 || nitem <= static_cast<size_t>(item)) {
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
      item = static_cast<int>(nitem) + item;
    }
    if (item < 0 || nitem < static_cast<size_t>(item)) {
      std::ostringstream str;
      str << "index: " << item << " out of range (-" << varArgsExpanded.size()
          << ", " << varArgsExpanded.size() << ")";
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

bool cmListCommand::HandleJoinCommand(std::vector<std::string> const& args)
{
  if (args.size() != 4) {
    std::ostringstream error;
    error << "sub-command JOIN requires three arguments (" << args.size() - 1
          << " found).";
    this->SetError(error.str());
    return false;
  }

  const std::string& listName = args[1];
  const std::string& glue = args[2];
  const std::string& variableName = args[3];

  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->Makefile->AddDefinition(variableName, "");
    return true;
  }

  std::string value =
    cmJoin(cmMakeRange(varArgsExpanded.begin(), varArgsExpanded.end()), glue);

  this->Makefile->AddDefinition(variableName, value.c_str());
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

// Helpers for list(TRANSFORM <list> ...)
namespace {
using transform_type = std::function<std::string(const std::string&)>;

class transform_error : public std::runtime_error
{
public:
  transform_error(const std::string& error)
    : std::runtime_error(error)
  {
  }
};

class TransformSelector
{
public:
  virtual ~TransformSelector() {}

  std::string Tag;

  virtual bool Validate(std::size_t count = 0) = 0;

  virtual bool InSelection(const std::string&) = 0;

  virtual void Transform(std::vector<std::string>& list,
                         const transform_type& transform)
  {
    std::transform(list.begin(), list.end(), list.begin(), transform);
  }

protected:
  TransformSelector(std::string&& tag)
    : Tag(std::move(tag))
  {
  }
};
class TransformNoSelector : public TransformSelector
{
public:
  TransformNoSelector()
    : TransformSelector("NO SELECTOR")
  {
  }

  bool Validate(std::size_t) override { return true; }

  bool InSelection(const std::string&) override { return true; }
};
class TransformSelectorRegex : public TransformSelector
{
public:
  TransformSelectorRegex(const std::string& regex)
    : TransformSelector("REGEX")
    , Regex(regex)
  {
  }

  bool Validate(std::size_t) override { return this->Regex.is_valid(); }

  bool InSelection(const std::string& value) override
  {
    return this->Regex.find(value);
  }

  cmsys::RegularExpression Regex;
};
class TransformSelectorIndexes : public TransformSelector
{
public:
  std::vector<int> Indexes;

  bool InSelection(const std::string&) override { return true; }

  void Transform(std::vector<std::string>& list,
                 const transform_type& transform) override
  {
    this->Validate(list.size());

    for (auto index : this->Indexes) {
      list[index] = transform(list[index]);
    }
  }

protected:
  TransformSelectorIndexes(std::string&& tag)
    : TransformSelector(std::move(tag))
  {
  }
  TransformSelectorIndexes(std::string&& tag, std::vector<int>&& indexes)
    : TransformSelector(std::move(tag))
    , Indexes(indexes)
  {
  }

  int NormalizeIndex(int index, std::size_t count)
  {
    if (index < 0) {
      index = static_cast<int>(count) + index;
    }
    if (index < 0 || count <= static_cast<std::size_t>(index)) {
      std::ostringstream str;
      str << "sub-command TRANSFORM, selector " << this->Tag
          << ", index: " << index << " out of range (-" << count << ", "
          << count - 1 << ").";
      throw transform_error(str.str());
    }
    return index;
  }
};
class TransformSelectorAt : public TransformSelectorIndexes
{
public:
  TransformSelectorAt(std::vector<int>&& indexes)
    : TransformSelectorIndexes("AT", std::move(indexes))
  {
  }

  bool Validate(std::size_t count) override
  {
    decltype(Indexes) indexes;

    for (auto index : Indexes) {
      indexes.push_back(this->NormalizeIndex(index, count));
    }
    this->Indexes = std::move(indexes);

    return true;
  }
};
class TransformSelectorFor : public TransformSelectorIndexes
{
public:
  TransformSelectorFor(int start, int stop, int step)
    : TransformSelectorIndexes("FOR")
    , Start(start)
    , Stop(stop)
    , Step(step)
  {
  }

  bool Validate(std::size_t count) override
  {
    this->Start = this->NormalizeIndex(this->Start, count);
    this->Stop = this->NormalizeIndex(this->Stop, count);

    // compute indexes
    auto size = (this->Stop - this->Start + 1) / this->Step;
    if ((this->Stop - this->Start + 1) % this->Step != 0) {
      size += 1;
    }

    this->Indexes.resize(size);
    auto start = this->Start, step = this->Step;
    std::generate(this->Indexes.begin(), this->Indexes.end(),
                  [&start, step]() -> int {
                    auto r = start;
                    start += step;
                    return r;
                  });

    return true;
  }

private:
  int Start, Stop, Step;
};

class TransformAction
{
public:
  virtual ~TransformAction() {}

  virtual std::string Transform(const std::string& input) = 0;
};
class TransformReplace : public TransformAction
{
public:
  TransformReplace(const std::vector<std::string>& arguments,
                   cmMakefile* makefile)
    : ReplaceHelper(arguments[0], arguments[1], makefile)
  {
    makefile->ClearMatches();

    if (!this->ReplaceHelper.IsRegularExpressionValid()) {
      std::ostringstream error;
      error
        << "sub-command TRANSFORM, action REPLACE: Failed to compile regex \""
        << arguments[0] << "\".";
      throw transform_error(error.str());
    }
    if (!this->ReplaceHelper.IsReplaceExpressionValid()) {
      std::ostringstream error;
      error << "sub-command TRANSFORM, action REPLACE: "
            << this->ReplaceHelper.GetError() << ".";
      throw transform_error(error.str());
    }
  }

  std::string Transform(const std::string& input) override
  {
    // Scan through the input for all matches.
    std::string output;

    if (!this->ReplaceHelper.Replace(input, output)) {
      std::ostringstream error;
      error << "sub-command TRANSFORM, action REPLACE: "
            << this->ReplaceHelper.GetError() << ".";
      throw transform_error(error.str());
    }

    return output;
  }

private:
  cmStringReplaceHelper ReplaceHelper;
};
}

bool cmListCommand::HandleTransformCommand(
  std::vector<std::string> const& args)
{
  if (args.size() < 3) {
    this->SetError(
      "sub-command TRANSFORM requires an action to be specified.");
    return false;
  }

  // Structure collecting all elements of the command
  struct Command
  {
    Command(const std::string& listName)
      : ListName(listName)
      , OutputName(listName)
    {
    }

    std::string Name;
    std::string ListName;
    std::vector<std::string> Arguments;
    std::unique_ptr<TransformAction> Action;
    std::unique_ptr<TransformSelector> Selector;
    std::string OutputName;
  } command(args[1]);

  // Descriptor of action
  // Arity: number of arguments required for the action
  // Transform: lambda function implementing the action
  struct ActionDescriptor
  {
    ActionDescriptor(const std::string& name)
      : Name(name)
    {
    }
    ActionDescriptor(const std::string& name, int arity,
                     const transform_type& transform)
      : Name(name)
      , Arity(arity)
      , Transform(transform)
    {
    }

    operator const std::string&() const { return Name; }

    std::string Name;
    int Arity = 0;
    transform_type Transform;
  };

  // Build a set of supported actions.
  std::set<ActionDescriptor,
           std::function<bool(const std::string&, const std::string&)>>
    descriptors(
      [](const std::string& x, const std::string& y) { return x < y; });
  descriptors = { { "APPEND", 1,
                    [&command](const std::string& s) -> std::string {
                      if (command.Selector->InSelection(s)) {
                        return s + command.Arguments[0];
                      }

                      return s;
                    } },
                  { "PREPEND", 1,
                    [&command](const std::string& s) -> std::string {
                      if (command.Selector->InSelection(s)) {
                        return command.Arguments[0] + s;
                      }

                      return s;
                    } },
                  { "TOUPPER", 0,
                    [&command](const std::string& s) -> std::string {
                      if (command.Selector->InSelection(s)) {
                        return cmSystemTools::UpperCase(s);
                      }

                      return s;
                    } },
                  { "TOLOWER", 0,
                    [&command](const std::string& s) -> std::string {
                      if (command.Selector->InSelection(s)) {
                        return cmSystemTools::LowerCase(s);
                      }

                      return s;
                    } },
                  { "STRIP", 0,
                    [&command](const std::string& s) -> std::string {
                      if (command.Selector->InSelection(s)) {
                        return cmSystemTools::TrimWhitespace(s);
                      }

                      return s;
                    } },
                  { "GENEX_STRIP", 0,
                    [&command](const std::string& s) -> std::string {
                      if (command.Selector->InSelection(s)) {
                        return cmGeneratorExpression::Preprocess(
                          s,
                          cmGeneratorExpression::StripAllGeneratorExpressions);
                      }

                      return s;
                    } },
                  { "REPLACE", 2,
                    [&command](const std::string& s) -> std::string {
                      if (command.Selector->InSelection(s)) {
                        return command.Action->Transform(s);
                      }

                      return s;
                    } } };

  using size_type = std::vector<std::string>::size_type;
  size_type index = 2;

  // Parse all possible function parameters
  auto descriptor = descriptors.find(args[index]);

  if (descriptor == descriptors.end()) {
    std::ostringstream error;
    error << " sub-command TRANSFORM, " << args[index] << " invalid action.";
    this->SetError(error.str());
    return false;
  }

  // Action arguments
  index += 1;
  if (args.size() < index + descriptor->Arity) {
    std::ostringstream error;
    error << "sub-command TRANSFORM, action " << descriptor->Name
          << " expects " << descriptor->Arity << " argument(s).";
    this->SetError(error.str());
    return false;
  }

  command.Name = descriptor->Name;
  index += descriptor->Arity;
  if (descriptor->Arity > 0) {
    command.Arguments =
      std::vector<std::string>(args.begin() + 3, args.begin() + index);
  }

  if (command.Name == "REPLACE") {
    try {
      command.Action =
        cm::make_unique<TransformReplace>(command.Arguments, this->Makefile);
    } catch (const transform_error& e) {
      this->SetError(e.what());
      return false;
    }
  }

  const std::string REGEX{ "REGEX" }, AT{ "AT" }, FOR{ "FOR" },
    OUTPUT_VARIABLE{ "OUTPUT_VARIABLE" };

  // handle optional arguments
  while (args.size() > index) {
    if ((args[index] == REGEX || args[index] == AT || args[index] == FOR) &&
        command.Selector) {
      std::ostringstream error;
      error << "sub-command TRANSFORM, selector already specified ("
            << command.Selector->Tag << ").";
      this->SetError(error.str());
      return false;
    }

    // REGEX selector
    if (args[index] == REGEX) {
      if (args.size() == ++index) {
        this->SetError("sub-command TRANSFORM, selector REGEX expects "
                       "'regular expression' argument.");
        return false;
      }

      command.Selector = cm::make_unique<TransformSelectorRegex>(args[index]);
      if (!command.Selector->Validate()) {
        std::ostringstream error;
        error << "sub-command TRANSFORM, selector REGEX failed to compile "
                 "regex \"";
        error << args[index] << "\".";
        this->SetError(error.str());
        return false;
      }

      index += 1;
      continue;
    }

    // AT selector
    if (args[index] == AT) {
      // get all specified indexes
      std::vector<int> indexes;
      while (args.size() > ++index) {
        std::size_t pos;
        int value;

        try {
          value = std::stoi(args[index], &pos);
          if (pos != args[index].length()) {
            // this is not a number, stop processing
            break;
          }
          indexes.push_back(value);
        } catch (const std::invalid_argument&) {
          // this is not a number, stop processing
          break;
        }
      }

      if (indexes.empty()) {
        this->SetError(
          "sub-command TRANSFORM, selector AT expects at least one "
          "numeric value.");
        return false;
      }

      command.Selector =
        cm::make_unique<TransformSelectorAt>(std::move(indexes));

      continue;
    }

    // FOR selector
    if (args[index] == FOR) {
      if (args.size() <= ++index + 1) {
        this->SetError("sub-command TRANSFORM, selector FOR expects, at least,"
                       " two arguments.");
        return false;
      }

      int start = 0, stop = 0, step = 1;
      bool valid = true;
      try {
        std::size_t pos;

        start = std::stoi(args[index], &pos);
        if (pos != args[index].length()) {
          // this is not a number
          valid = false;
        } else {
          stop = std::stoi(args[++index], &pos);
          if (pos != args[index].length()) {
            // this is not a number
            valid = false;
          }
        }
      } catch (const std::invalid_argument&) {
        // this is not numbers
        valid = false;
      }
      if (!valid) {
        this->SetError("sub-command TRANSFORM, selector FOR expects, "
                       "at least, two numeric values.");
        return false;
      }
      // try to read a third numeric value for step
      if (args.size() > ++index) {
        try {
          std::size_t pos;

          step = std::stoi(args[index], &pos);
          if (pos != args[index].length()) {
            // this is not a number
            step = 1;
          } else {
            index += 1;
          }
        } catch (const std::invalid_argument&) {
          // this is not number, ignore exception
        }
      }

      if (step < 0) {
        this->SetError("sub-command TRANSFORM, selector FOR expects "
                       "non negative numeric value for <step>.");
      }

      command.Selector =
        cm::make_unique<TransformSelectorFor>(start, stop, step);

      continue;
    }

    // output variable
    if (args[index] == OUTPUT_VARIABLE) {
      if (args.size() == ++index) {
        this->SetError("sub-command TRANSFORM, OUTPUT_VARIABLE "
                       "expects variable name argument.");
        return false;
      }

      command.OutputName = args[index++];
      continue;
    }

    std::ostringstream error;
    error << "sub-command TRANSFORM, '"
          << cmJoin(cmMakeRange(args).advance(index), " ")
          << "': unexpected argument(s).";
    this->SetError(error.str());
    return false;
  }

  // expand the list variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, command.ListName)) {
    this->Makefile->AddDefinition(command.OutputName, "");
    return true;
  }

  if (!command.Selector) {
    // no selector specified, apply transformation to all elements
    command.Selector = cm::make_unique<TransformNoSelector>();
  }

  try {
    command.Selector->Transform(varArgsExpanded, descriptor->Transform);
  } catch (const transform_error& e) {
    this->SetError(e.what());
    return false;
  }

  this->Makefile->AddDefinition(command.OutputName,
                                cmJoin(varArgsExpanded, ";").c_str());

  return true;
}

class cmStringSorter
{
public:
  enum class Order
  {
    UNINITIALIZED,
    ASCENDING,
    DESCENDING,
  };

  enum class Compare
  {
    UNINITIALIZED,
    STRING,
    FILE_BASENAME,
  };
  enum class CaseSensitivity
  {
    UNINITIALIZED,
    SENSITIVE,
    INSENSITIVE,
  };

protected:
  typedef std::string (*StringFilter)(const std::string& in);
  StringFilter GetCompareFilter(Compare compare)
  {
    return (compare == Compare::FILE_BASENAME) ? cmSystemTools::GetFilenameName
                                               : nullptr;
  }

  StringFilter GetCaseFilter(CaseSensitivity sensitivity)
  {
    return (sensitivity == CaseSensitivity::INSENSITIVE)
      ? cmSystemTools::LowerCase
      : nullptr;
  }

public:
  cmStringSorter(Compare compare, CaseSensitivity caseSensitivity,
                 Order desc = Order::ASCENDING)
    : filters{ GetCompareFilter(compare), GetCaseFilter(caseSensitivity) }
    , descending(desc == Order::DESCENDING)
  {
  }

  std::string ApplyFilter(const std::string& argument)
  {
    std::string result = argument;
    for (auto filter : filters) {
      if (filter != nullptr) {
        result = filter(result);
      }
    }
    return result;
  }

  bool operator()(const std::string& a, const std::string& b)
  {
    std::string af = ApplyFilter(a);
    std::string bf = ApplyFilter(b);
    bool result;
    if (descending) {
      result = bf < af;
    } else {
      result = af < bf;
    }
    return result;
  }

protected:
  StringFilter filters[2] = { nullptr, nullptr };
  bool descending;
};

bool cmListCommand::HandleSortCommand(std::vector<std::string> const& args)
{
  assert(args.size() >= 2);
  if (args.size() > 8) {
    this->SetError("sub-command SORT only takes up to six arguments.");
    return false;
  }

  auto sortCompare = cmStringSorter::Compare::UNINITIALIZED;
  auto sortCaseSensitivity = cmStringSorter::CaseSensitivity::UNINITIALIZED;
  auto sortOrder = cmStringSorter::Order::UNINITIALIZED;

  size_t argumentIndex = 2;
  const std::string messageHint = "sub-command SORT ";

  while (argumentIndex < args.size()) {
    const std::string option = args[argumentIndex++];
    if (option == "COMPARE") {
      if (sortCompare != cmStringSorter::Compare::UNINITIALIZED) {
        std::string error = messageHint + "option \"" + option +
          "\" has been specified multiple times.";
        this->SetError(error);
        return false;
      }
      if (argumentIndex < args.size()) {
        const std::string argument = args[argumentIndex++];
        if (argument == "STRING") {
          sortCompare = cmStringSorter::Compare::STRING;
        } else if (argument == "FILE_BASENAME") {
          sortCompare = cmStringSorter::Compare::FILE_BASENAME;
        } else {
          std::string error = messageHint + "value \"" + argument +
            "\" for option \"" + option + "\" is invalid.";
          this->SetError(error);
          return false;
        }
      } else {
        std::string error =
          messageHint + "missing argument for option \"" + option + "\".";
        this->SetError(error);
        return false;
      }
    } else if (option == "CASE") {
      if (sortCaseSensitivity !=
          cmStringSorter::CaseSensitivity::UNINITIALIZED) {
        std::string error = messageHint + "option \"" + option +
          "\" has been specified multiple times.";
        this->SetError(error);
        return false;
      }
      if (argumentIndex < args.size()) {
        const std::string argument = args[argumentIndex++];
        if (argument == "SENSITIVE") {
          sortCaseSensitivity = cmStringSorter::CaseSensitivity::SENSITIVE;
        } else if (argument == "INSENSITIVE") {
          sortCaseSensitivity = cmStringSorter::CaseSensitivity::INSENSITIVE;
        } else {
          std::string error = messageHint + "value \"" + argument +
            "\" for option \"" + option + "\" is invalid.";
          this->SetError(error);
          return false;
        }
      } else {
        std::string error =
          messageHint + "missing argument for option \"" + option + "\".";
        this->SetError(error);
        return false;
      }
    } else if (option == "ORDER") {

      if (sortOrder != cmStringSorter::Order::UNINITIALIZED) {
        std::string error = messageHint + "option \"" + option +
          "\" has been specified multiple times.";
        this->SetError(error);
        return false;
      }
      if (argumentIndex < args.size()) {
        const std::string argument = args[argumentIndex++];
        if (argument == "ASCENDING") {
          sortOrder = cmStringSorter::Order::ASCENDING;
        } else if (argument == "DESCENDING") {
          sortOrder = cmStringSorter::Order::DESCENDING;
        } else {
          std::string error = messageHint + "value \"" + argument +
            "\" for option \"" + option + "\" is invalid.";
          this->SetError(error);
          return false;
        }
      } else {
        std::string error =
          messageHint + "missing argument for option \"" + option + "\".";
        this->SetError(error);
        return false;
      }
    } else {
      std::string error =
        messageHint + "option \"" + option + "\" is unknown.";
      this->SetError(error);
      return false;
    }
  }
  // set Default Values if Option is not given
  if (sortCompare == cmStringSorter::Compare::UNINITIALIZED) {
    sortCompare = cmStringSorter::Compare::STRING;
  }
  if (sortCaseSensitivity == cmStringSorter::CaseSensitivity::UNINITIALIZED) {
    sortCaseSensitivity = cmStringSorter::CaseSensitivity::SENSITIVE;
  }
  if (sortOrder == cmStringSorter::Order::UNINITIALIZED) {
    sortOrder = cmStringSorter::Order::ASCENDING;
  }

  const std::string& listName = args[1];
  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName)) {
    this->SetError("sub-command SORT requires list to be present.");
    return false;
  }

  if ((sortCompare == cmStringSorter::Compare::STRING) &&
      (sortCaseSensitivity == cmStringSorter::CaseSensitivity::SENSITIVE) &&
      (sortOrder == cmStringSorter::Order::ASCENDING)) {
    std::sort(varArgsExpanded.begin(), varArgsExpanded.end());
  } else {
    cmStringSorter sorter(sortCompare, sortCaseSensitivity, sortOrder);
    std::sort(varArgsExpanded.begin(), varArgsExpanded.end(), sorter);
  }

  std::string value = cmJoin(varArgsExpanded, ";");
  this->Makefile->AddDefinition(listName, value.c_str());
  return true;
}

bool cmListCommand::HandleSublistCommand(std::vector<std::string> const& args)
{
  if (args.size() != 5) {
    std::ostringstream error;
    error << "sub-command SUBLIST requires four arguments (" << args.size() - 1
          << " found).";
    this->SetError(error.str());
    return false;
  }

  const std::string& listName = args[1];
  const std::string& variableName = args[args.size() - 1];

  // expand the variable
  std::vector<std::string> varArgsExpanded;
  if (!this->GetList(varArgsExpanded, listName) || varArgsExpanded.empty()) {
    this->Makefile->AddDefinition(variableName, "");
    return true;
  }

  const int start = atoi(args[2].c_str());
  const int length = atoi(args[3].c_str());

  using size_type = decltype(varArgsExpanded)::size_type;

  if (start < 0 || size_type(start) >= varArgsExpanded.size()) {
    std::ostringstream error;
    error << "begin index: " << start << " is out of range 0 - "
          << varArgsExpanded.size() - 1;
    this->SetError(error.str());
    return false;
  }
  if (length < -1) {
    std::ostringstream error;
    error << "length: " << length << " should be -1 or greater";
    this->SetError(error.str());
    return false;
  }

  const size_type end =
    (length == -1 || size_type(start + length) > varArgsExpanded.size())
    ? varArgsExpanded.size()
    : size_type(start + length);
  std::vector<std::string> sublist(varArgsExpanded.begin() + start,
                                   varArgsExpanded.begin() + end);
  this->Makefile->AddDefinition(variableName, cmJoin(sublist, ";").c_str());
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
      item = static_cast<int>(nitem) + item;
    }
    if (item < 0 || nitem <= static_cast<size_t>(item)) {
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
