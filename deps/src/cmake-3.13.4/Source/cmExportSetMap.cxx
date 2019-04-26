/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExportSetMap.h"

#include "cmAlgorithms.h"
#include "cmExportSet.h"

#include <utility>

cmExportSet* cmExportSetMap::operator[](const std::string& name)
{
  std::map<std::string, cmExportSet*>::iterator it = this->find(name);
  if (it == this->end()) // Export set not found
  {
    it = this->insert(std::make_pair(name, new cmExportSet(name))).first;
  }
  return it->second;
}

void cmExportSetMap::clear()
{
  cmDeleteAll(*this);
  this->derived::clear();
}

cmExportSetMap::~cmExportSetMap()
{
  this->clear();
}
