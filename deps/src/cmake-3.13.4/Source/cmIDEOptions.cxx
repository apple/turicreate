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
  this->DoingInclude = false;
  this->AllowSlash = false;
  this->DoingFollowing = 0;
  for (int i = 0; i < FlagTableCount; ++i) {
    this->FlagTable[i] = 0;
  }
}

cmIDEOptions::~cmIDEOptions()
{
}

void cmIDEOptions::HandleFlag(std::string const& flag)
{
  // If the last option was -D then this option is the definition.
  if (this->DoingDefine) {
    this->DoingDefine = false;
    this->Defines.push_back(flag);
    return;
  }

  // If the last option was -I then this option is the include directory.
  if (this->DoingInclude) {
    this->DoingInclude = false;
    this->Includes.push_back(flag);
    return;
  }

  // If the last option expected a following value, this is it.
  if (this->DoingFollowing) {
    this->FlagMapUpdate(this->DoingFollowing, flag);
    this->DoingFollowing = 0;
    return;
  }

  // Look for known arguments.
  size_t len = flag.length();
  if (len > 0 && (flag[0] == '-' || (this->AllowSlash && flag[0] == '/'))) {
    // Look for preprocessor definitions.
    if (this->AllowDefine && len > 1 && flag[1] == 'D') {
      if (len <= 2) {
        // The next argument will have the definition.
        this->DoingDefine = true;
      } else {
        // Store this definition.
        this->Defines.push_back(flag.substr(2));
      }
      return;
    }
    // Look for include directory.
    if (this->AllowInclude && len > 1 && flag[1] == 'I') {
      if (len <= 2) {
        // The next argument will have the include directory.
        this->DoingInclude = true;
      } else {
        // Store this include directory.
        this->Includes.push_back(flag.substr(2));
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
                                  std::string const& flag, bool& flag_handled)
{
  const char* pf = flag.c_str() + 1;
  // Look for an entry in the flag table matching this flag.
  for (cmIDEFlagTable const* entry = table; entry->IDEName; ++entry) {
    bool entry_found = false;
    if (entry->special & cmIDEFlagTable::UserValue) {
      // This flag table entry accepts a user-specified value.  If
      // the entry specifies UserRequired we must match only if a
      // non-empty value is given.
      int n = static_cast<int>(strlen(entry->commandFlag));
      if ((strncmp(pf, entry->commandFlag, n) == 0 ||
           (entry->special & cmIDEFlagTable::CaseInsensitive &&
            cmsysString_strncasecmp(pf, entry->commandFlag, n))) &&
          (!(entry->special & cmIDEFlagTable::UserRequired) ||
           static_cast<int>(strlen(pf)) > n)) {
        this->FlagMapUpdate(entry, std::string(pf + n));
        entry_found = true;
      }
    } else if (strcmp(pf, entry->commandFlag) == 0 ||
               (entry->special & cmIDEFlagTable::CaseInsensitive &&
                cmsysString_strcasecmp(pf, entry->commandFlag) == 0)) {
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
                                 std::string const& new_value)
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

void cmIDEOptions::AddDefines(std::string const& defines)
{
  if (!defines.empty()) {
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

void cmIDEOptions::AddInclude(const std::string& include)
{
  this->Includes.push_back(include);
}

void cmIDEOptions::AddIncludes(std::string const& includes)
{
  if (!includes.empty()) {
    // Expand the list of includes.
    cmSystemTools::ExpandListArgument(includes, this->Includes);
  }
}
void cmIDEOptions::AddIncludes(const std::vector<std::string>& includes)
{
  this->Includes.insert(this->Includes.end(), includes.begin(),
                        includes.end());
}

std::vector<std::string> const& cmIDEOptions::GetIncludes() const
{
  return this->Includes;
}

void cmIDEOptions::AddFlag(std::string const& flag, std::string const& value)
{
  this->FlagMap[flag] = value;
}

void cmIDEOptions::AddFlag(std::string const& flag,
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

void cmIDEOptions::RemoveFlag(std::string const& flag)
{
  this->FlagMap.erase(flag);
}

bool cmIDEOptions::HasFlag(std::string const& flag) const
{
  return this->FlagMap.find(flag) != this->FlagMap.end();
}

const char* cmIDEOptions::GetFlag(std::string const& flag) const
{
  // This method works only for single-valued flags!
  std::map<std::string, FlagValue>::const_iterator i =
    this->FlagMap.find(flag);
  if (i != this->FlagMap.cend() && i->second.size() == 1) {
    return i->second[0].c_str();
  }
  return nullptr;
}
