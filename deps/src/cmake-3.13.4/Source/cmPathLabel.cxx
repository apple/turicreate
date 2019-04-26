/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmPathLabel.h"

cmPathLabel::cmPathLabel(const std::string& label)
  : Label(label)
  , Hash(0)
{
  // Use a Jenkins one-at-a-time hash with under/over-flow protection
  for (char i : this->Label) {
    this->Hash += i;
    this->Hash += ((this->Hash & 0x003FFFFF) << 10);
    this->Hash ^= ((this->Hash & 0xFFFFFFC0) >> 6);
  }
  this->Hash += ((this->Hash & 0x1FFFFFFF) << 3);
  this->Hash ^= ((this->Hash & 0xFFFFF800) >> 11);
  this->Hash += ((this->Hash & 0x0001FFFF) << 15);
}

bool cmPathLabel::operator<(const cmPathLabel& l) const
{
  return this->Hash < l.Hash;
}

bool cmPathLabel::operator==(const cmPathLabel& l) const
{
  return this->Hash == l.Hash;
}
