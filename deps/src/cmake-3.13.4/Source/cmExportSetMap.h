/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportSetMap_h
#define cmExportSetMap_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>

class cmExportSet;

/// A name -> cmExportSet map with overloaded operator[].
class cmExportSetMap : public std::map<std::string, cmExportSet*>
{
  typedef std::map<std::string, cmExportSet*> derived;

public:
  /** \brief Overloaded operator[].
   *
   * The operator is overloaded because cmExportSet has no default constructor:
   * we do not want unnamed export sets.
   */
  cmExportSet* operator[](const std::string& name);

  void clear();

  /// Overloaded destructor deletes all member export sets.
  ~cmExportSetMap();
};

#endif
