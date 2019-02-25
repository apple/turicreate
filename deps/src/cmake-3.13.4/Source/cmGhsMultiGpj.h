/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGhsMultiGpj_h
#define cmGhsMultiGpj_h

#include "cmConfigure.h" // IWYU pragma: keep

class cmGeneratedFileStream;

class GhsMultiGpj
{
public:
  enum Types
  {
    INTERGRITY_APPLICATION,
    LIBRARY,
    PROJECT,
    PROGRAM,
    REFERENCE,
    SUBPROJECT
  };

  static void WriteGpjTag(Types const gpjType,
                          cmGeneratedFileStream* filestream);
};

#endif // ! cmGhsMultiGpjType_h
