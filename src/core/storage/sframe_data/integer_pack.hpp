/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_INTEGER_PACK_HPP
#define TURI_SFRAME_INTEGER_PACK_HPP
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/sframe_data/integer_pack_impl.hpp>
#include <core/util/bitops.hpp>

namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup Compression Integer Compression Routines
 * \{
 */

/**
 * \internal
 * Integer Packing Routines
 */
namespace integer_pack {
/**
 * Performs a byte aligned variable length encode of 8 byte wide integers.
 *
 * It is important to remember that x86(-64) is little endian. i.e. the
 * least significant byte is at the lowest memory address.
 * This makes the presentation a little different from how most other
 * integer compression blogs which generally "prefix" the binary string
 * with stuff, thus in the most significant bits. This
 * makes it very complicated to decode. Here we are instead going to
 * attach stuff to the lowest significant bits which will then appear as a
 * prefix when writing to a stream.
 *
 * The format is relatively straightforward.
 * [... binary representation of number .. ] [0] [as many "1" bits as (length of
 *                                                number in bytes, rounded up. - 1)]
 *
 * Essentially this leaves 8 possible encodings
 *  - [ 7 bit number  ] [0]           --> 1 byte
 *  - [ 14 bit number ] [0] [1]       --> 2 bytes
 *  - [ 21 bit number ] [0] [11]      --> 3 bytes
 *  - [ 28 bit number ] [0] [111]     --> 4 bytes
 *  - [ 35 bit number ] [0] [1111]    --> 5 bytes
 *  - [ 42 bit number ] [0] [11111]   --> 6 bytes
 *  - [ 49 bit number ] [0] [111111]  --> 7 bytes
 *  - [ 64 bit number ] [0] [1111111] --> 9 bytes
 *
 *  Note that there is no code of total length 8 bytes. (since the suffix has
 *  7 "1"'s, the suffix is 1 byte long, and the remaining 8 bytes can hold all
 *  64-bit numbers.
 *
 *  The basic encode algorithm is then as such:
 *  if s >> [...] == 0  (test if s can be encoded with just that many bits)
 *  Then to put in the suffix, (s << ...) | (...)
 *
 * Example codes:
 * [0001100] [0]  -- decodes to --> [00001100] = 12
 * [0001100] [0]  -- decodes to --> [00001100] = 12
 * [00000010] [001100] [0] [1] -- decodes to --> [00000010,00001100] = 524
 */
template <typename OutArcType>
static inline void variable_encode(OutArcType& oarc, uint64_t s) {
  if ((s >> (8-1)) == 0) {
    // 0b00000000 - 0b01111111
    unsigned char trunc_s = s << 1;
    oarc.direct_assign(trunc_s);
  }
  else if ((s >> (16-2)) == 0) {
    // 0b00000000 00000000 -
    // 0b00111111 11111111
    uint16_t trunc_s = s;
    trunc_s = (trunc_s << 2) | 1;
    oarc.direct_assign(trunc_s);
  }
  else if ((s >> (24-3)) == 0) {
    // 0b00000000 00000000 00000000 -
    // 0b00011111 11111111 11111111
    uint32_t trunc_s = s;
    trunc_s = (trunc_s << 3) | 3;
    oarc.write((char*)(&trunc_s), 3);
  }
  else if ((s >> (32-4)) == 0) {
    // 0b00000000 00000000 00000000 00000000 -
    // 0b00001111 11111111 11111111 11111111
    uint32_t trunc_s = s;
    trunc_s = (trunc_s << 4) | 7;
    oarc.direct_assign(trunc_s);
  }
  else if ((s >> (40-5)) == 0) {
    // 0b00000000 00000000 00000000 00000000 00000000 -
    // 0b00000111 11111111 11111111 11111111 11111111
    uint64_t trunc_s = s;
    trunc_s = (trunc_s << 5) | 15;
    oarc.write((char*)(&trunc_s), 5);
  }
  else if ((s >> (48-6)) == 0) {
    // 0b00000000 00000000 00000000 00000000 00000000 00000000 -
    // 0b00000011 11111111 11111111 11111111 11111111 11111111
    uint64_t trunc_s = s;
    trunc_s = (trunc_s << 6) | 31;
    oarc.write((char*)(&trunc_s), 6);
  }
  else if ((s >> (56-7)) == 0) {
    // 0b00000000 00000000 00000000 00000000 00000000 00000000 00000000 -
    // 0b00000001 11111111 11111111 11111111 11111111 11111111 11111111
    uint64_t trunc_s = s;
    trunc_s = (trunc_s << 7) | 63;
    oarc.write((char*)(&trunc_s), 7);
  }
  else {
    // all 8 bytes
    unsigned char c = 127;
    oarc.direct_assign(c);
    oarc.direct_assign(s);
  }
}

/**
 * Performs a byte aligned variable length decode of 8 byte wide integers.
 * See the documentation for \ref variable_encode() for details on the encoding.
 */
template <typename InArcType>
static inline void variable_decode(InArcType& iarc, uint64_t & s) {
  union {
    uint64_t intval;
    char chars[8];
  } c;
  c.intval = 0;
  iarc.read(c.chars, 1);
  if ((c.intval & 1) == 0) {
    s = c.intval >> 1;
  } else if ((c.intval & 3) == 1) {
    // read a second byte
    iarc.read(c.chars + 1, 1);
    s = c.intval >> 2;
  } else if ((c.intval & 7) == 3) {
    // read 3 byte
    iarc.read(c.chars + 1, 2);
    s = c.intval >> 3;
  } else if ((c.intval & 15) == 7) {
    iarc.read(c.chars + 1, 3);
    s = c.intval >> 4;
  } else if ((c.intval & 31) == 15) {
    iarc.read(c.chars + 1, 4);
    s = c.intval >> 5;
  } else if ((c.intval & 63) == 31) {
    iarc.read(c.chars + 1, 5);
    s = c.intval >> 6;
  } else if ((c.intval & 127) == 63) {
    iarc.read(c.chars + 1, 6);
    s = c.intval >> 7;
  } else {
    // read 8 more bytes
    iarc.read(c.chars, 8);
    s = c.intval;
  }
}

/**
 * Maps values [0,-1,1,-2,2,-3,3,-4,4...] to [0,1,2,3,4,5,6,...]
 * return 2*|val| - sign.
 *
 * (This is equivalent Google Protobuf's ZigZag encoding
 * (see https://developers.google.com/protocol-buffers/docs/encoding#types)
 * and can be more compactly written as (n << 1) ^ (n >> 63)
 * (requiring only 3 ops instead of 5 ops here)
 *
 */
inline uint64_t shifted_integer_encode(int64_t val) {
  // if val < 0 sign == -1, else sign = 0;
  int64_t sign = (val >> 63);
  // turning a negative value into a positive value is ~(t - 1)
  uint64_t absval = (val + sign) ^ sign; // compute the absolute value
  return (absval << 1) + sign;
}

/**
 * Reverse of shifted_integer_encode.
 * Maps values  [0,1,2,3,4,5,6,...] to [0,-1,1,-2,2,-3,3,-4,4...]
 */
inline int64_t shifted_integer_decode(uint64_t val) {
  // if val < 0 sign == -1, else sign = 0;
  int64_t sign = -(int64_t)(val & 1);
  uint64_t absval = (val >> 1) - sign;
  // turning a positive value into a negative value is also ~(t - 1)
  return (absval + sign) ^ sign;
}

/**
 * The codec number for "frame of reference" coding.
 */
static constexpr unsigned char FRAME_OF_REFERENCE = 0;

/**
 * The codec number for "frame of reference" delta-coding.
 */
static constexpr unsigned char FRAME_OF_REFERENCE_DELTA = 1;
/**
 * The codec number for "frame of reference" delta-coding with negative numbers.
 */
static constexpr unsigned char FRAME_OF_REFERENCE_DELTA_NEGATIVE = 2;

/**
 * Number of bits used by the header to store the codec number
 */
static constexpr unsigned char FRAME_OF_REFERENCE_HEADER_NUM_BITS = 2;

/**
 * The mask you can apply to the codec header to extract the codec number
 */
static constexpr unsigned char FRAME_OF_REFERENCE_HEADER_MASK = 3;

/**
 * Performs a group encode of a collection of up to 128 64-bit numbers.
 *
 * There are 3 basic strategies for computing the code.
 * - Frame of Reference Coding: (code difference to a minimum value)
 *   Use \ref variable_encode() to code the smallest value. Then compute an
 *   array which is the difference of every number against the smallest value,
 *   and pack it using as few bits as possible. See below for the details
 *   on the packing.
 * - Frame of Reference Delta Coding: (code incremental gaps)
 *   Use \ref variable_encode() to code the first value. Then compute a delta
 *   array (which is the the difference between consecutive values), and
 *   pack that using as few bits as possible. See below for the details on
 *   the packing.
 * - Frame of Reference Delta Negative Coding: (Like delta, but supports negative gaps)
 *   Use \ref variable_encode() to code the first value. Then compute a delta
 *   array (which is the the difference between consecutive values), and
 *   apply the \ref shifted_integer_encode() to the delta array, and
 *   pack that using as few bits as possible. See below for the details on
 *   the packing.
 *
 * Packing
 * -------
 * After the values to be coded are generated (see above), the remaining numbers
 * are packed by finding the maximum number of bits required to represent
 * any value.
 * i.e.
 *
 *     [0,0,0,0,0,0] --> 0 bit maximum
 *     [1,0,1,0,1,0] --> 1 bit maximum
 *     [1,0,1,0,1,7] --> 3 bits maximum
 *     [1,0,16,0,1,7] --> 5 bits maximum
 *
 * This maximum is then rounded up to the nearest power of two (thus ensuring
 * the coding/decoding is simple and is generally word-aligned.)
 * i.e.
 *
 *     [0,0,0,0,0,0] --> 0 bit code
 *     [1,0,1,0,1,0] --> 1 bit code
 *     [1,0,1,0,1,7] --> 4 bit code
 *     [1,0,16,0,1,7] --> 8 bit code
 *
 * Then one of the pack_... functions are used to code the values.
 *
 * Coding
 * ------
 * First there is a 1 byte header:
 * [6 bit: 1 + log2 code length] [2 bits codec type]
 * first 6 bits: If code length == 0 , we write 0 here. Otherwise, 1 + log2 code length;
 * The codec type is one of:
 *   - FRAME_OF_REFERENCE
 *   - FRAME_OF_REFERENCE_DELTA
 *   - FRAME_OF_REFERENCE_DELTA_NEGATIVE
 *
 *
 * \note The coding does not store the number of values stored. The decoder
 * \ref frame_of_reference_decode_128() requires the number of values to
 * decode correctly.
 */
template <typename OutArcType>
GL_HOT
void frame_of_reference_encode_128(const uint64_t* input,
                                   size_t len,
                                   OutArcType& oarc) {
  if (len == 0) return;
  DASSERT_LE(len, 128);
  // 3 possible encodings
  // Frame of Reference (encode delta to the minimum value)
  // Frame of Reference Delta Incremental (encode a minimum value then encode the deltas)
  // Frame of Reference Delta (encode a minimum value then encode the deltas, supporting negative values)
  //
  // In Frame of Reference Delta, negative values are supported by mapping
  // this sequence (0, -1,1,-2,2,-3,3,-4,4...) to the [0,1,2,3,4,5,6,...]
  // (Forward mapping is -> if i positive, return 2i,
  //                        else if i negative, return -2i - 1)
  // To do this in bits interestingly, simply involves making the
  // least significant bit the sign it. i.e.
  // return (abs(t) << 1) + sgn(t)
  // Note that the conversion is 1-1.
  uint64_t minvalue = input[0];
  uint64_t frame[128];
  uint64_t delta[128];
  uint64_t delta_negative[128];
  unsigned char nbits = 0;
  unsigned char nbits_frame = 0, nbits_delta = 255, nbits_delta_negative = 255;
  bool is_incremental = true;
  for (size_t i = 0;i < len; ++i) {
    minvalue = std::min(minvalue, input[i]);
    if (i > 0 && input[i] < input[i-1]) {is_incremental = false; break;}
  }

  // Calculate the frame in the same loop as the rest
  frame[0] = input[0] - minvalue;
  uint64_t all_or_frame = frame[0];

  if (is_incremental) {
    nbits_delta = 0;
    delta[0] = input[0];

    uint64_t all_or = 0;
    for (size_t i = 1; i < len; ++i) {
      delta[i] = input[i] - input[i-1];
      all_or |= delta[i];
      frame[i] = input[i] - minvalue;
      all_or_frame |= frame[i];
    }

    nbits_delta = 64 - n_leading_zeros(all_or);
  } else {
    nbits_delta_negative = 0;
    delta_negative[0] = input[0];

    uint64_t all_or = 0;
    for (size_t i = 1;i < len; ++i) {
      delta_negative[i] = shifted_integer_encode((int64_t)input[i] - (int64_t)input[i-1]);
      all_or |= delta_negative[i];
      frame[i] = input[i] - minvalue;
      all_or_frame |= frame[i];
    }

    nbits_delta_negative = 64 - n_leading_zeros(all_or);
  }
  nbits_frame = 64 - n_leading_zeros(all_or_frame);

  // whats the most efficient encoding?
  unsigned char coding_technique = 0;
  if (nbits_frame <= nbits_delta && nbits_frame <= nbits_delta_negative) {
    nbits = nbits_frame;
    coding_technique = FRAME_OF_REFERENCE;
    input = frame;
  } else if (nbits_delta <= nbits_frame && nbits_delta <= nbits_delta_negative) {
    nbits = nbits_delta;
    coding_technique = FRAME_OF_REFERENCE_DELTA;
    input = delta;
  } else { //if (nbits_delta_negative <= nbits_frame && nbits_delta_negative <= nbits_delta)
    nbits = nbits_delta_negative;
    coding_technique = FRAME_OF_REFERENCE_DELTA_NEGATIVE;
    input = delta_negative;
  }
  // encode the header
  // round nbits to next power of 2.
  --nbits;
  nbits = nbits| (nbits >> 1);
  nbits = nbits| (nbits >> 2);
  nbits = nbits| (nbits >> 4);
  ++nbits;
  unsigned char header = coding_technique;
  if (nbits > 0) {
    unsigned char shiftpos = 64 - n_leading_zeros((uint64_t)nbits);
    header = header + (shiftpos << 2);
  }
  oarc.direct_assign(header);
//   logstream(LOG_INFO) << "Encoding header " << (int)(header) << ": " << len << std::endl;
  if (coding_technique == FRAME_OF_REFERENCE) {
    variable_encode(oarc, minvalue);
  } else if (coding_technique == FRAME_OF_REFERENCE_DELTA ||
             coding_technique == FRAME_OF_REFERENCE_DELTA_NEGATIVE) {
    variable_encode(oarc, input[0]);
    ++input;
    --len;
  }
  if (nbits == 0) return;
//   logstream(LOG_INFO) << "Encoding at bitrate: " << (int)nbits << std::endl;
  uint8_t pack[128*8];
  size_t bytes_used = 0;
  switch(nbits) {
   case 1:
    bytes_used = pack_1(input, len, pack);
    oarc.write((char*)pack, bytes_used);
    break;
   case 2:
    bytes_used = pack_2(input, len, pack);
    oarc.write((char*)pack, bytes_used);
    break;
   case 4:
    bytes_used = pack_4(input, len, pack);
    oarc.write((char*)pack, bytes_used);
    break;
   case 8:
    bytes_used = pack_8(input, len, pack);
    oarc.write((char*)pack, bytes_used);
    break;
   case 16:
    bytes_used = pack_16(input, len, (uint16_t*)pack);
    oarc.write((char*)pack, bytes_used);
    break;
   case 32:
    bytes_used = pack_32(input, len, (uint32_t*)pack);
    oarc.write((char*)pack, bytes_used);
    break;
   case 64:
    oarc.write((char*)input, sizeof(uint64_t)*len);
    break;
   default:
    ASSERT_TRUE(false);
    __builtin_unreachable();
  }
}



/**
 * Performs a group decode of a collection of up to 128 64-bit numbers.
 * See \ref frame_of_reference_encode_128() for the encoding details.
 */
template <typename InArcType>
void frame_of_reference_decode_128(InArcType& iarc,
                                   size_t len,
                                   uint64_t* output) {
  if (len == 0) return;
  DASSERT_LE(len, 128);
  unsigned char header;
  iarc.read_into(header);
  unsigned char nbits = 0;
  unsigned char shiftpos = header >> FRAME_OF_REFERENCE_HEADER_NUM_BITS;
  unsigned char coding_technique = header & FRAME_OF_REFERENCE_HEADER_MASK;
  uint64_t minvalue;
  if (shiftpos > 0) nbits = 1 << (shiftpos - 1);
  if (nbits == 0) {
    // if nbits is 0 it really doesn't matter what the coding technique is.
    // All will produce the same output
    variable_decode(iarc, minvalue);
    for (size_t i = 0;i < len; ++i) output[i] = minvalue;
    return;
  }
//   logstream(LOG_INFO) << "Decoding header " << (int)(header) << ": " << len << std::endl;
  if (coding_technique == FRAME_OF_REFERENCE) {
    variable_decode(iarc, minvalue);
  } else if (coding_technique == FRAME_OF_REFERENCE_DELTA ||
           coding_technique == FRAME_OF_REFERENCE_DELTA_NEGATIVE) {
    variable_decode(iarc, output[0]);
    ++output;
    --len;
  }

  uint8_t pack[128*8];
  size_t nbits_to_read = (size_t)(nbits) * len;
  size_t nbytes_to_read = (nbits_to_read + 7) / 8;
  switch(nbits) {
   case 1:
    iarc.read((char*)pack, nbytes_to_read);
    unpack_1(pack, len, output);
    break;
   case 2:
    iarc.read((char*)pack, nbytes_to_read);
    unpack_2(pack, len, output);
    break;
   case 4:
    iarc.read((char*)pack, nbytes_to_read);
    unpack_4(pack, len, output);
    break;
   case 8:
    iarc.read((char*)pack, nbytes_to_read);
    unpack_8(pack, len, output);
    break;
   case 16:
    iarc.read((char*)pack, nbytes_to_read);
    unpack_16((uint16_t*)pack, len, output);
    break;
   case 32:
    iarc.read((char*)pack, nbytes_to_read);
    unpack_32((uint32_t*)pack, len, output);
    break;
   case 64:
    iarc.read((char*)output, sizeof(uint64_t)*len);
    break;
   default:
    ASSERT_TRUE(false);
    __builtin_unreachable();
  }


  if (coding_technique == FRAME_OF_REFERENCE) {
    for (size_t i = 0;i < len; ++i) {
      output[i] += minvalue;
    }
  } else if (coding_technique == FRAME_OF_REFERENCE_DELTA) {
    for (int i = 0;i < (int)len; ++i) {
      // yes this will will go below 0. yes this is intentional
      output[i] += output[i-1];
    }
  } else if (coding_technique == FRAME_OF_REFERENCE_DELTA_NEGATIVE) {
    for (int i = 0;i < (int)len; ++i) {
      output[i] = shifted_integer_decode(output[i]);
      // yes this will will go below 0. yes this is intentional
      output[i] += output[i-1];
    }
  }
}

} // namespace integer_pack

/// \}
} // namespace turi
#endif
