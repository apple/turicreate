/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmPropertyDefinition.h"

void cmPropertyDefinition::DefineProperty(const std::string& name,
                                          cmProperty::ScopeType scope,
                                          const char* shortDescription,
                                          const char* fullDescription,
                                          bool chain)
{
  this->Name = name;
  this->Scope = scope;
  this->Chained = chain;
  if (shortDescription) {
    this->ShortDescription = shortDescription;
  }
  if (fullDescription) {
    this->FullDescription = fullDescription;
  }
}
