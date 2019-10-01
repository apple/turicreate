/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmBase32.h"

// -- Static functions

static const unsigned char Base32EncodeTable[33] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

inline unsigned char Base32EncodeChar(int schar)
{
  return Base32EncodeTable[schar];
}

void Base32Encode5(const unsigned char src[5], char dst[8])
{
  // [0]:5 bits
  dst[0] = Base32EncodeChar((src[0] >> 3) & 0x1F);
  // [0]:3 bits + [1]:2 bits
  dst[1] = Base32EncodeChar(((src[0] << 2) & 0x1C) + ((src[1] >> 6) & 0x03));
  // [1]:5 bits
  dst[2] = Base32EncodeChar((src[1] >> 1) & 0x1F);
  // [1]:1 bit + [2]:4 bits
  dst[3] = Base32EncodeChar(((src[1] << 4) & 0x10) + ((src[2] >> 4) & 0x0F));
  // [2]:4 bits + [3]:1 bit
  dst[4] = Base32EncodeChar(((src[2] << 1) & 0x1E) + ((src[3] >> 7) & 0x01));
  // [3]:5 bits
  dst[5] = Base32EncodeChar((src[3] >> 2) & 0x1F);
  // [3]:2 bits + [4]:3 bit
  dst[6] = Base32EncodeChar(((src[3] << 3) & 0x18) + ((src[4] >> 5) & 0x07));
  // [4]:5 bits
  dst[7] = Base32EncodeChar((src[4] << 0) & 0x1F);
}

// -- Class methods

cmBase32Encoder::cmBase32Encoder()
{
}

cmBase32Encoder::~cmBase32Encoder()
{
}

std::string cmBase32Encoder::encodeString(const unsigned char* input,
                                          size_t len, bool padding)
{
  std::string res;

  static const size_t blockSize = 5;
  static const size_t bufferSize = 8;
  char buffer[bufferSize];

  const unsigned char* end = input + len;
  while ((input + blockSize) <= end) {
    Base32Encode5(input, buffer);
    res.append(buffer, bufferSize);
    input += blockSize;
  }

  size_t remain = static_cast<size_t>(end - input);
  if (remain != 0) {
    // Temporary source buffer filled up with 0s
    unsigned char extended[blockSize];
    for (size_t ii = 0; ii != remain; ++ii) {
      extended[ii] = input[ii];
    }
    for (size_t ii = remain; ii != blockSize; ++ii) {
      extended[ii] = 0;
    }

    Base32Encode5(extended, buffer);
    size_t numPad(0);
    switch (remain) {
      case 1:
        numPad = 6;
        break;
      case 2:
        numPad = 4;
        break;
      case 3:
        numPad = 3;
        break;
      case 4:
        numPad = 1;
        break;
      default:
        break;
    }
    res.append(buffer, bufferSize - numPad);
    if (padding) {
      for (size_t ii = 0; ii != numPad; ++ii) {
        res.push_back(paddingChar);
      }
    }
  }

  return res;
}
