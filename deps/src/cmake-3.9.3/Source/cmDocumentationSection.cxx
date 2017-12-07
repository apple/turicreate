/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDocumentationSection.h"

void cmDocumentationSection::Append(const char* data[][2])
{
  int i = 0;
  while (data[i][1]) {
    this->Entries.push_back(cmDocumentationEntry(data[i][0], data[i][1]));
    data += 1;
  }
}

void cmDocumentationSection::Prepend(const char* data[][2])
{
  std::vector<cmDocumentationEntry> tmp;
  int i = 0;
  while (data[i][1]) {
    tmp.push_back(cmDocumentationEntry(data[i][0], data[i][1]));
    data += 1;
  }
  this->Entries.insert(this->Entries.begin(), tmp.begin(), tmp.end());
}

void cmDocumentationSection::Append(const char* n, const char* b)
{
  this->Entries.push_back(cmDocumentationEntry(n, b));
}
