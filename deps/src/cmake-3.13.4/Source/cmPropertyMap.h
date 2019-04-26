/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmPropertyMap_h
#define cmPropertyMap_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmProperty.h"

#include <map>
#include <string>
#include <vector>

class cmPropertyMap : public std::map<std::string, cmProperty>
{
public:
  cmProperty* GetOrCreateProperty(const std::string& name);

  std::vector<std::string> GetPropertyList() const;

  void SetProperty(const std::string& name, const char* value);

  void AppendProperty(const std::string& name, const char* value,
                      bool asString = false);

  const char* GetPropertyValue(const std::string& name) const;
};

#endif
