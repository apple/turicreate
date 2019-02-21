/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCommandArgumentsHelper.h"

cmCommandArgument::cmCommandArgument(cmCommandArgumentsHelper* args,
                                     const char* key,
                                     cmCommandArgumentGroup* group)
  : Key(key)
  , Group(group)
  , WasActive(false)
  , ArgumentsBeforeEmpty(true)
  , CurrentIndex(0)
{
  if (args != nullptr) {
    args->AddArgument(this);
  }

  if (this->Group != nullptr) {
    this->Group->ContainedArguments.push_back(this);
  }
}

void cmCommandArgument::Reset()
{
  this->WasActive = false;
  this->CurrentIndex = 0;
  this->DoReset();
}

void cmCommandArgument::Follows(const cmCommandArgument* arg)
{
  this->ArgumentsBeforeEmpty = false;
  this->ArgumentsBefore.insert(arg);
}

void cmCommandArgument::FollowsGroup(const cmCommandArgumentGroup* group)
{
  if (group != nullptr) {
    this->ArgumentsBeforeEmpty = false;
    this->ArgumentsBefore.insert(group->ContainedArguments.begin(),
                                 group->ContainedArguments.end());
  }
}

bool cmCommandArgument::MayFollow(const cmCommandArgument* current) const
{
  if (this->ArgumentsBeforeEmpty) {
    return true;
  }
  return this->ArgumentsBefore.find(current) != this->ArgumentsBefore.end();
}

bool cmCommandArgument::KeyMatches(const std::string& key) const
{
  if ((this->Key == nullptr) || (this->Key[0] == '\0')) {
    return true;
  }
  return (key == this->Key);
}

void cmCommandArgument::ApplyOwnGroup()
{
  if (this->Group != nullptr) {
    for (cmCommandArgument* cargs : this->Group->ContainedArguments) {
      if (cargs != this) {
        this->ArgumentsBefore.insert(cargs);
      }
    }
  }
}

void cmCommandArgument::Activate()
{
  this->WasActive = true;
  this->CurrentIndex = 0;
}

bool cmCommandArgument::Consume(const std::string& arg)
{
  bool res = this->DoConsume(arg, this->CurrentIndex);
  this->CurrentIndex++;
  return res;
}

cmCAStringVector::cmCAStringVector(cmCommandArgumentsHelper* args,
                                   const char* key,
                                   cmCommandArgumentGroup* group)
  : cmCommandArgument(args, key, group)
  , Ignore(nullptr)
{
  if ((key == nullptr) || (*key == 0)) {
    this->DataStart = 0;
  } else {
    this->DataStart = 1;
  }
}

bool cmCAStringVector::DoConsume(const std::string& arg, unsigned int index)
{
  if (index >= this->DataStart) {
    if ((this->Ignore == nullptr) || (arg != this->Ignore)) {
      this->Vector.push_back(arg);
    }
  }

  return false;
}

void cmCAStringVector::DoReset()
{
  this->Vector.clear();
}

cmCAString::cmCAString(cmCommandArgumentsHelper* args, const char* key,
                       cmCommandArgumentGroup* group)
  : cmCommandArgument(args, key, group)
{
  if ((key == nullptr) || (*key == 0)) {
    this->DataStart = 0;
  } else {
    this->DataStart = 1;
  }
}

bool cmCAString::DoConsume(const std::string& arg, unsigned int index)
{
  if (index == this->DataStart) {
    this->String = arg;
  }

  return index >= this->DataStart;
}

void cmCAString::DoReset()
{
  this->String.clear();
}

cmCAEnabler::cmCAEnabler(cmCommandArgumentsHelper* args, const char* key,
                         cmCommandArgumentGroup* group)
  : cmCommandArgument(args, key, group)
  , Enabled(false)
{
}

bool cmCAEnabler::DoConsume(const std::string&, unsigned int index)
{
  if (index == 0) {
    this->Enabled = true;
  }
  return true;
}

void cmCAEnabler::DoReset()
{
  this->Enabled = false;
}

cmCADisabler::cmCADisabler(cmCommandArgumentsHelper* args, const char* key,
                           cmCommandArgumentGroup* group)
  : cmCommandArgument(args, key, group)
  , Enabled(true)
{
}

bool cmCADisabler::DoConsume(const std::string&, unsigned int index)
{
  if (index == 0) {
    this->Enabled = false;
  }
  return true;
}

void cmCADisabler::DoReset()
{
  this->Enabled = true;
}

void cmCommandArgumentGroup::Follows(const cmCommandArgument* arg)
{
  for (cmCommandArgument* ca : this->ContainedArguments) {
    ca->Follows(arg);
  }
}

void cmCommandArgumentGroup::FollowsGroup(const cmCommandArgumentGroup* group)
{
  for (cmCommandArgument* ca : this->ContainedArguments) {
    ca->FollowsGroup(group);
  }
}

void cmCommandArgumentsHelper::Parse(const std::vector<std::string>* args,
                                     std::vector<std::string>* unconsumedArgs)
{
  if (args == nullptr) {
    return;
  }

  for (cmCommandArgument* ca : this->Arguments) {
    ca->ApplyOwnGroup();
    ca->Reset();
  }

  cmCommandArgument* activeArgument = nullptr;
  const cmCommandArgument* previousArgument = nullptr;
  for (std::string const& it : *args) {
    for (cmCommandArgument* ca : this->Arguments) {
      if (ca->KeyMatches(it) && (ca->MayFollow(previousArgument))) {
        activeArgument = ca;
        activeArgument->Activate();
        break;
      }
    }

    if (activeArgument) {
      bool argDone = activeArgument->Consume(it);
      previousArgument = activeArgument;
      if (argDone) {
        activeArgument = nullptr;
      }
    } else {
      if (unconsumedArgs != nullptr) {
        unconsumedArgs->push_back(it);
      }
    }
  }
}

void cmCommandArgumentsHelper::AddArgument(cmCommandArgument* arg)
{
  this->Arguments.push_back(arg);
}
