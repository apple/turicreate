/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef bindexplib_h
#define bindexplib_h

#include "cmConfigure.h"

#include <set>
#include <stdio.h>
#include <string>

class bindexplib
{
public:
  bindexplib() {}
  bool AddDefinitionFile(const char* filename);
  bool AddObjectFile(const char* filename);
  void WriteFile(FILE* file);

private:
  std::set<std::string> Symbols;
  std::set<std::string> DataSymbols;
};
#endif
