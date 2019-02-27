/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmBase32_h
#define cmBase32_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <stddef.h>
#include <string>

/** \class cmBase32Encoder
 * \brief Encodes a byte sequence to a Base32 byte sequence according to
 * RFC4648
 *
 */
class cmBase32Encoder
{
public:
  static const char paddingChar = '=';

public:
  cmBase32Encoder();
  ~cmBase32Encoder();

  // Encodes the given input byte sequence into a string
  // @arg input Input data pointer
  // @arg len Input data size
  // @arg padding Flag to append "=" on demand
  std::string encodeString(const unsigned char* input, size_t len,
                           bool padding = true);
};

#endif
