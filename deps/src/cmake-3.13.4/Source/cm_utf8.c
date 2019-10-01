/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cm_utf8.h"

/*
  RFC 3629
  07-bit: 0xxxxxxx
  11-bit: 110xxxxx 10xxxxxx
  16-bit: 1110xxxx 10xxxxxx 10xxxxxx
  21-bit: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

  Pre-RFC Compatibility
  26-bit: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
  31-bit: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
*/

/* Number of leading ones before a zero in the byte.  */
unsigned char const cm_utf8_ones[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8
};

/* Mask away control bits from bytes with n leading ones.  */
static unsigned char const cm_utf8_mask[7] = { 0xEF, 0x3F, 0x1F, 0x0F,
                                               0x07, 0x03, 0x01 };

/* Minimum allowed value when first byte has n leading ones.  */
static unsigned int const cm_utf8_min[7] = {
  0, 0, 1u << 7, 1u << 11, 1u << 16, 1u << 21, 1u << 26 /*, 1u<<31 */
};

const char* cm_utf8_decode_character(const char* first, const char* last,
                                     unsigned int* pc)
{
  /* Count leading ones in the first byte.  */
  unsigned char c = (unsigned char)*first++;
  unsigned char const ones = cm_utf8_ones[c];
  switch (ones) {
    case 0:
      *pc = c;
      return first; /* One-byte character.  */
    case 1:
    case 7:
    case 8:
      return 0; /* Invalid leading byte.  */
    default:
      break;
  }

  /* Extract bits from this multi-byte character.  */
  {
    unsigned int uc = c & cm_utf8_mask[ones];
    int left;
    for (left = ones - 1; left && first != last; --left) {
      c = (unsigned char)*first++;
      if (cm_utf8_ones[c] != 1) {
        return 0;
      }
      uc = (uc << 6) | (c & cm_utf8_mask[1]);
    }

    if (left > 0 || uc < cm_utf8_min[ones]) {
      return 0;
    }

    *pc = uc;
    return first;
  }
}
