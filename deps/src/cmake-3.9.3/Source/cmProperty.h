/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmProperty_h
#define cmProperty_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

class cmProperty
{
public:
  enum ScopeType
  {
    TARGET,
    SOURCE_FILE,
    DIRECTORY,
    GLOBAL,
    CACHE,
    TEST,
    VARIABLE,
    CACHED_VARIABLE,
    INSTALL
  };

  // set this property
  void Set(const char* value);

  // append to this property
  void Append(const char* value, bool asString = false);

  // get the value
  const char* GetValue() const;

  // construct with the value not set
  cmProperty() { this->ValueHasBeenSet = false; }

protected:
  std::string Value;
  bool ValueHasBeenSet;
};

#endif
