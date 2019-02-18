/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCLocaleEnvironmentScope.h"

#include "cmSystemTools.h"

#include <sstream>
#include <utility>

cmCLocaleEnvironmentScope::cmCLocaleEnvironmentScope()
{
  this->SetEnv("LANGUAGE", "");
  this->SetEnv("LC_MESSAGES", "C");

  std::string lcAll = this->GetEnv("LC_ALL");

  if (!lcAll.empty()) {
    this->SetEnv("LC_ALL", "");
    this->SetEnv("LC_CTYPE", lcAll);
  }
}

std::string cmCLocaleEnvironmentScope::GetEnv(std::string const& key)
{
  std::string value;
  cmSystemTools::GetEnv(key, value);
  return value;
}

void cmCLocaleEnvironmentScope::SetEnv(std::string const& key,
                                       std::string const& value)
{
  std::string oldValue = this->GetEnv(key);

  this->EnvironmentBackup.insert(std::make_pair(key, oldValue));

  if (value.empty()) {
    cmSystemTools::UnsetEnv(key.c_str());
  } else {
    std::ostringstream tmp;
    tmp << key << "=" << value;
    cmSystemTools::PutEnv(tmp.str());
  }
}

cmCLocaleEnvironmentScope::~cmCLocaleEnvironmentScope()
{
  for (auto const& envb : this->EnvironmentBackup) {
    std::ostringstream tmp;
    tmp << envb.first << "=" << envb.second;
    cmSystemTools::PutEnv(tmp.str());
  }
}
