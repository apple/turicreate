/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmXCode21Object_h
#define cmXCode21Object_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <vector>

#include "cmXCodeObject.h"

class cmXCode21Object : public cmXCodeObject
{
public:
  cmXCode21Object(PBXType ptype, Type type);
  void PrintComment(std::ostream&) override;
  static void PrintList(std::vector<cmXCodeObject*> const&, std::ostream& out,
                        PBXType t);
  static void PrintList(std::vector<cmXCodeObject*> const&, std::ostream& out);
};
#endif
