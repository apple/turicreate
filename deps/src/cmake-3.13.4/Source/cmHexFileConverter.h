/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmHexFileConverter_h
#define cmHexFileConverter_h

#include "cmConfigure.h" // IWYU pragma: keep

/** \class cmHexFileConverter
 * \brief Can detects Intel Hex and Motorola S-record files and convert them
 *        to binary files.
 *
 */
class cmHexFileConverter
{
public:
  enum FileType
  {
    Binary,
    IntelHex,
    MotorolaSrec
  };
  static FileType DetermineFileType(const char* inFileName);
  static bool TryConvert(const char* inFileName, const char* outFileName);
};

#endif
