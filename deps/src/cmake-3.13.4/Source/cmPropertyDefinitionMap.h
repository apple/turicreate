/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmPropertyDefinitionMap_h
#define cmPropertyDefinitionMap_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmProperty.h"
#include "cmPropertyDefinition.h"

#include <map>
#include <string>

class cmPropertyDefinitionMap
  : public std::map<std::string, cmPropertyDefinition>
{
public:
  // define the property
  void DefineProperty(const std::string& name, cmProperty::ScopeType scope,
                      const char* ShortDescription,
                      const char* FullDescription, bool chain);

  // has a named property been defined
  bool IsPropertyDefined(const std::string& name) const;

  // is a named property set to chain
  bool IsPropertyChained(const std::string& name) const;
};

#endif
