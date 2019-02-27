/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCLocaleEnvironmentScope_h
#define cmCLocaleEnvironmentScope_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>

class cmCLocaleEnvironmentScope
{
  CM_DISABLE_COPY(cmCLocaleEnvironmentScope)

public:
  cmCLocaleEnvironmentScope();
  ~cmCLocaleEnvironmentScope();

private:
  std::string GetEnv(std::string const& key);
  void SetEnv(std::string const& key, std::string const& value);

  typedef std::map<std::string, std::string> backup_map_t;
  backup_map_t EnvironmentBackup;
};

#endif
