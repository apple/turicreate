/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmPropertyMap.h"

#include <algorithm>
#include <assert.h>
#include <utility>

cmProperty* cmPropertyMap::GetOrCreateProperty(const std::string& name)
{
  cmPropertyMap::iterator it = this->find(name);
  cmProperty* prop;
  if (it == this->end()) {
    prop = &(*this)[name];
  } else {
    prop = &(it->second);
  }
  return prop;
}

std::vector<std::string> cmPropertyMap::GetPropertyList() const
{
  std::vector<std::string> keyList;
  for (auto const& i : *this) {
    keyList.push_back(i.first);
  }
  std::sort(keyList.begin(), keyList.end());
  return keyList;
}

void cmPropertyMap::SetProperty(const std::string& name, const char* value)
{
  if (!value) {
    this->erase(name);
    return;
  }

  cmProperty* prop = this->GetOrCreateProperty(name);
  prop->Set(value);
}

void cmPropertyMap::AppendProperty(const std::string& name, const char* value,
                                   bool asString)
{
  // Skip if nothing to append.
  if (!value || !*value) {
    return;
  }

  cmProperty* prop = this->GetOrCreateProperty(name);
  prop->Append(value, asString);
}

const char* cmPropertyMap::GetPropertyValue(const std::string& name) const
{
  assert(!name.empty());

  cmPropertyMap::const_iterator it = this->find(name);
  if (it == this->end()) {
    return nullptr;
  }
  return it->second.GetValue();
}
