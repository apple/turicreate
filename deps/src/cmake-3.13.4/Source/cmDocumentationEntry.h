/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDocumentationEntry_h
#define cmDocumentationEntry_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

/** Standard documentation entry for cmDocumentation's formatting.  */
struct cmDocumentationEntry
{
  std::string Name;
  std::string Brief;
  cmDocumentationEntry() {}
  cmDocumentationEntry(const char* doc[2])
  {
    if (doc[0]) {
      this->Name = doc[0];
    }
    if (doc[1]) {
      this->Brief = doc[1];
    }
  }
  cmDocumentationEntry(const char* n, const char* b)
  {
    if (n) {
      this->Name = n;
    }
    if (b) {
      this->Brief = b;
    }
  }
};

#endif
