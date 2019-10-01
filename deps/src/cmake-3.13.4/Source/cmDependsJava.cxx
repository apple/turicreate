/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDependsJava.h"

#include "cmSystemTools.h"

cmDependsJava::cmDependsJava()
{
}

cmDependsJava::~cmDependsJava()
{
}

bool cmDependsJava::WriteDependencies(const std::set<std::string>& sources,
                                      const std::string& /*obj*/,
                                      std::ostream& /*makeDepends*/,
                                      std::ostream& /*internalDepends*/)
{
  // Make sure this is a scanning instance.
  if (sources.empty() || sources.begin()->empty()) {
    cmSystemTools::Error("Cannot scan dependencies without an source file.");
    return false;
  }

  return true;
}

bool cmDependsJava::CheckDependencies(
  std::istream& /*internalDepends*/, const char* /*internalDependsFileName*/,
  std::map<std::string, DependencyVector>& /*validDeps*/)
{
  return true;
}
