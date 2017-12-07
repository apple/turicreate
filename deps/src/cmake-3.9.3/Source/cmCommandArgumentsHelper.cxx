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
  if (args != CM_NULLPTR) {
    args->AddArgument(this);
  }

  if (this->Group != CM_NULLPTR) {
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
  if (group != CM_NULLPTR) {
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
  if ((this->Key == CM_NULLPTR) || (this->Key[0] == '\0')) {
    return true;
  }
  return (key == this->Key);
}

void cmCommandArgument::ApplyOwnGroup()
{
  if (this->Group != CM_NULLPTR) {
    for (std::vector<cmCommandArgument*>::const_iterator it =
           this->Group->ContainedArguments.begin();
         it != this->Group->ContainedArguments.end(); ++it) {
      if (*it != this) {
        this->ArgumentsBefore.insert(*it);
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
  , Ignore(CM_NULLPTR)
{
  if ((key == CM_NULLPTR) || (*key == 0)) {
    this->DataStart = 0;
  } else {
    this->DataStart = 1;
  }
}

bool cmCAStringVector::DoConsume(const std::string& arg, unsigned int index)
{
  if (index >= this->DataStart) {
    if ((this->Ignore == CM_NULLPTR) || (arg != this->Ignore)) {
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
  if ((key == CM_NULLPTR) || (*key == 0)) {
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
  this->String = "";
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
  for (std::vector<cmCommandArgument*>::iterator it =
         this->ContainedArguments.begin();
       it != this->ContainedArguments.end(); ++it) {
    (*it)->Follows(arg);
  }
}

void cmCommandArgumentGroup::FollowsGroup(const cmCommandArgumentGroup* group)
{
  for (std::vector<cmCommandArgument*>::iterator it =
         this->ContainedArguments.begin();
       it != this->ContainedArguments.end(); ++it) {
    (*it)->FollowsGroup(group);
  }
}

void cmCommandArgumentsHelper::Parse(const std::vector<std::string>* args,
                                     std::vector<std::string>* unconsumedArgs)
{
  if (args == CM_NULLPTR) {
    return;
  }

  for (std::vector<cmCommandArgument*>::iterator argIt =
         this->Arguments.begin();
       argIt != this->Arguments.end(); ++argIt) {
    (*argIt)->ApplyOwnGroup();
    (*argIt)->Reset();
  }

  cmCommandArgument* activeArgument = CM_NULLPTR;
  const cmCommandArgument* previousArgument = CM_NULLPTR;
  for (std::vector<std::string>::const_iterator it = args->begin();
       it != args->end(); ++it) {
    for (std::vector<cmCommandArgument*>::iterator argIt =
           this->Arguments.begin();
         argIt != this->Arguments.end(); ++argIt) {
      if ((*argIt)->KeyMatches(*it) &&
          ((*argIt)->MayFollow(previousArgument))) {
        activeArgument = *argIt;
        activeArgument->Activate();
        break;
      }
    }

    if (activeArgument) {
      bool argDone = activeArgument->Consume(*it);
      previousArgument = activeArgument;
      if (argDone) {
        activeArgument = CM_NULLPTR;
      }
    } else {
      if (unconsumedArgs != CM_NULLPTR) {
        unconsumedArgs->push_back(*it);
      }
    }
  }
}

void cmCommandArgumentsHelper::AddArgument(cmCommandArgument* arg)
{
  this->Arguments.push_back(arg);
}
