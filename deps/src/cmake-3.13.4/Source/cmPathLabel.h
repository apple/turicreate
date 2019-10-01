/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmPathLabel_h
#define cmPathLabel_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

/** \class cmPathLabel
 * \brief Helper class for text based labels
 *
 * cmPathLabel is extended in different classes to act as an inheritable
 * enum.  Comparisons are done on a precomputed Jenkins hash of the string
 * label for indexing and searchig.
 */
class cmPathLabel
{
public:
  cmPathLabel(const std::string& label);

  // The comparison operators are only for quick sorting and searching and
  // in no way imply any lexicographical order of the label
  bool operator<(const cmPathLabel& l) const;
  bool operator==(const cmPathLabel& l) const;

  const std::string& GetLabel() const { return this->Label; }
  const unsigned int& GetHash() const { return this->Hash; }

protected:
  cmPathLabel();

  std::string Label;
  unsigned int Hash;
};

#endif
