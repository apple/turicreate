/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmPropertyDefinitionMap.h"

#include <utility>

void cmPropertyDefinitionMap::DefineProperty(const std::string& name,
                                             cmProperty::ScopeType scope,
                                             const char* ShortDescription,
                                             const char* FullDescription,
                                             bool chain)
{
  cmPropertyDefinitionMap::iterator it = this->find(name);
  cmPropertyDefinition* prop;
  if (it == this->end()) {
    prop = &(*this)[name];
    prop->DefineProperty(name, scope, ShortDescription, FullDescription,
                         chain);
  }
}

bool cmPropertyDefinitionMap::IsPropertyDefined(const std::string& name) const
{
  return this->find(name) != this->end();
}

bool cmPropertyDefinitionMap::IsPropertyChained(const std::string& name) const
{
  cmPropertyDefinitionMap::const_iterator it = this->find(name);
  if (it == this->end()) {
    return false;
  }

  return it->second.IsChained();
}
