/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmIDEOptions.h"

#include "cmsys/String.h"
#include <iterator>
#include <string.h>

#include "cmIDEFlagTable.h"
#include "cmSystemTools.h"

cmIDEOptions::cmIDEOptions()
{
  this->DoingDefine = false;
  this->AllowDefine = true;
  this->AllowSlash = false;
  this->DoingFollowing = 0;
  for (int i = 0; i < FlagTableCount; ++i) {
    this->FlagTable[i] = 0;
  }
}

cmIDEOptions::~cmIDEOptions()
{
}

void cmIDEOptions::HandleFlag(const char* flag)
{
  // If the last option was -D then this option is the definition.
  if (this->DoingDefine) {
    this->DoingDefine = false;
    this->Defines.push_back(flag);
    return;
  }

  // If the last option expected a following value, this is it.
  if (this->DoingFollowing) {
    this->FlagMapUpdate(this->DoingFollowing, flag);
    this->DoingFollowing = 0;
    return;
  }

  // Look for known arguments.
  if (flag[0] == '-' || (this->AllowSlash && flag[0] == '/')) {
    // Look for preprocessor definitions.
    if (this->AllowDefine && flag[1] == 'D') {
      if (flag[2] == '\0') {
        // The next argument will have the definition.
        this->DoingDefine = true;
      } else {
        // Store this definition.
        this->Defines.push_back(flag + 2);
      }
      return;
    }

    // Look through the available flag tables.
    bool flag_handled = false;
    for (int i = 0; i < FlagTableCount && this->FlagTable[i]; ++i) {
      if (this->CheckFlagTable(this->FlagTable[i], flag, flag_handled)) {
        return;
      }
    }

    // If any map entry handled the flag we are done.
    if (flag_handled) {
      return;
    }
  }

  // This option is not known.  Store it in the output flags.
  this->StoreUnknownFlag(flag);
}

bool cmIDEOptions::CheckFlagTable(cmIDEFlagTable const* table,
                                  const char* flag, bool& flag_handled)
{
  // Look for an entry in the flag table matching this flag.
  for (cmIDEFlagTable const* entry = table; entry->IDEName; ++entry) {
    bool entry_found = false;
    if (entry->special & cmIDEFlagTable::UserValue) {
      // This flag table entry accepts a user-specified value.  If
      // the entry specifies UserRequired we must match only if a
      // non-empty value is given.
      int n = static_cast<int>(strlen(entry->commandFlag));
      if ((strncmp(flag + 1, entry->commandFlag, n) == 0 ||
           (entry->special & cmIDEFlagTable::CaseInsensitive &&
            cmsysString_strncasecmp(flag + 1, entry->commandFlag, n))) &&
          (!(entry->special & cmIDEFlagTable::UserRequired) ||
           static_cast<int>(strlen(flag + 1)) > n)) {
        this->FlagMapUpdate(entry, flag + n + 1);
        entry_found = true;
      }
    } else if (strcmp(flag + 1, entry->commandFlag) == 0 ||
               (entry->special & cmIDEFlagTable::CaseInsensitive &&
                cmsysString_strcasecmp(flag + 1, entry->commandFlag) == 0)) {
      if (entry->special & cmIDEFlagTable::UserFollowing) {
        // This flag expects a value in the following argument.
        this->DoingFollowing = entry;
      } else {
        // This flag table entry provides a fixed value.
        this->FlagMap[entry->IDEName] = entry->value;
      }
      entry_found = true;
    }

    // If the flag has been handled by an entry not requesting a
    // search continuation we are done.
    if (entry_found && !(entry->special & cmIDEFlagTable::Continue)) {
      return true;
    }

    // If the entry was found the flag has been handled.
    flag_handled = flag_handled || entry_found;
  }

  return false;
}

void cmIDEOptions::FlagMapUpdate(cmIDEFlagTable const* entry,
                                 const char* new_value)
{
  if (entry->special & cmIDEFlagTable::UserIgnored) {
    // Ignore the user-specified value.
    this->FlagMap[entry->IDEName] = entry->value;
  } else if (entry->special & cmIDEFlagTable::SemicolonAppendable) {
    this->FlagMap[entry->IDEName].push_back(new_value);
  } else if (entry->special & cmIDEFlagTable::SpaceAppendable) {
    this->FlagMap[entry->IDEName].append_with_space(new_value);
  } else {
    // Use the user-specified value.
    this->FlagMap[entry->IDEName] = new_value;
  }
}

void cmIDEOptions::AddDefine(const std::string& def)
{
  this->Defines.push_back(def);
}

void cmIDEOptions::AddDefines(const char* defines)
{
  if (defines) {
    // Expand the list of definitions.
    cmSystemTools::ExpandListArgument(defines, this->Defines);
  }
}
void cmIDEOptions::AddDefines(const std::vector<std::string>& defines)
{
  this->Defines.insert(this->Defines.end(), defines.begin(), defines.end());
}

std::vector<std::string> const& cmIDEOptions::GetDefines() const
{
  return this->Defines;
}

void cmIDEOptions::AddFlag(const char* flag, const char* value)
{
  this->FlagMap[flag] = value;
}

void cmIDEOptions::AddFlag(const char* flag,
                           std::vector<std::string> const& value)
{
  this->FlagMap[flag] = value;
}

void cmIDEOptions::AppendFlag(std::string const& flag,
                              std::string const& value)
{
  this->FlagMap[flag].push_back(value);
}

void cmIDEOptions::AppendFlag(std::string const& flag,
                              std::vector<std::string> const& value)
{
  FlagValue& fv = this->FlagMap[flag];
  std::copy(value.begin(), value.end(), std::back_inserter(fv));
}

void cmIDEOptions::AppendFlagString(std::string const& flag,
                                    std::string const& value)
{
  this->FlagMap[flag].append_with_space(value);
}

void cmIDEOptions::RemoveFlag(const char* flag)
{
  this->FlagMap.erase(flag);
}

bool cmIDEOptions::HasFlag(std::string const& flag) const
{
  return this->FlagMap.find(flag) != this->FlagMap.end();
}

const char* cmIDEOptions::GetFlag(const char* flag)
{
  // This method works only for single-valued flags!
  std::map<std::string, FlagValue>::iterator i = this->FlagMap.find(flag);
  if (i != this->FlagMap.end() && i->second.size() == 1) {
    return i->second[0].c_str();
  }
  return 0;
}
