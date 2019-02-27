/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(Base64.h)

/* Work-around CMake dependency scanning limitation.  This must
   duplicate the above list of headers.  */
#if 0
#  include "Base64.h.in"
#endif

static const unsigned char kwsysBase64EncodeTable[65] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

static const unsigned char kwsysBase64DecodeTable[256] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
  0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
  0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  /*------------------------------------*/
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static unsigned char kwsysBase64EncodeChar(int c)
{
  return kwsysBase64EncodeTable[(unsigned char)c];
}

static unsigned char kwsysBase64DecodeChar(unsigned char c)
{
  return kwsysBase64DecodeTable[c];
}

/* Encode 3 bytes into a 4 byte string. */
void kwsysBase64_Encode3(const unsigned char* src, unsigned char* dest)
{
  dest[0] = kwsysBase64EncodeChar((src[0] >> 2) & 0x3F);
  dest[1] =
    kwsysBase64EncodeChar(((src[0] << 4) & 0x30) | ((src[1] >> 4) & 0x0F));
  dest[2] =
    kwsysBase64EncodeChar(((src[1] << 2) & 0x3C) | ((src[2] >> 6) & 0x03));
  dest[3] = kwsysBase64EncodeChar(src[2] & 0x3F);
}

/* Encode 2 bytes into a 4 byte string. */
void kwsysBase64_Encode2(const unsigned char* src, unsigned char* dest)
{
  dest[0] = kwsysBase64EncodeChar((src[0] >> 2) & 0x3F);
  dest[1] =
    kwsysBase64EncodeChar(((src[0] << 4) & 0x30) | ((src[1] >> 4) & 0x0F));
  dest[2] = kwsysBase64EncodeChar(((src[1] << 2) & 0x3C));
  dest[3] = '=';
}

/* Encode 1 bytes into a 4 byte string. */
void kwsysBase64_Encode1(const unsigned char* src, unsigned char* dest)
{
  dest[0] = kwsysBase64EncodeChar((src[0] >> 2) & 0x3F);
  dest[1] = kwsysBase64EncodeChar(((src[0] << 4) & 0x30));
  dest[2] = '=';
  dest[3] = '=';
}

/* Encode 'length' bytes from the input buffer and store the
   encoded stream into the output buffer. Return the length of the encoded
   buffer (output). Note that the output buffer must be allocated by the caller
   (length * 1.5 should be a safe estimate).  If 'mark_end' is true than an
   extra set of 4 bytes is added to the end of the stream if the input is a
   multiple of 3 bytes.  These bytes are invalid chars and therefore they will
   stop the decoder thus enabling the caller to decode a stream without
   actually knowing how much data to expect (if the input is not a multiple of
   3 bytes then the extra padding needed to complete the encode 4 bytes will
   stop the decoding anyway).  */
size_t kwsysBase64_Encode(const unsigned char* input, size_t length,
                          unsigned char* output, int mark_end)
{
  const unsigned char* ptr = input;
  const unsigned char* end = input + length;
  unsigned char* optr = output;

  /* Encode complete triplet */

  while ((end - ptr) >= 3) {
    kwsysBase64_Encode3(ptr, optr);
    ptr += 3;
    optr += 4;
  }

  /* Encodes a 2-byte ending into 3 bytes and 1 pad byte and writes. */

  if (end - ptr == 2) {
    kwsysBase64_Encode2(ptr, optr);
    optr += 4;
  }

  /* Encodes a 1-byte ending into 2 bytes and 2 pad bytes */

  else if (end - ptr == 1) {
    kwsysBase64_Encode1(ptr, optr);
    optr += 4;
  }

  /* Do we need to mark the end */

  else if (mark_end) {
    optr[0] = optr[1] = optr[2] = optr[3] = '=';
    optr += 4;
  }

  return (size_t)(optr - output);
}

/* Decode 4 bytes into a 3 byte string. */
int kwsysBase64_Decode3(const unsigned char* src, unsigned char* dest)
{
  unsigned char d0, d1, d2, d3;

  d0 = kwsysBase64DecodeChar(src[0]);
  d1 = kwsysBase64DecodeChar(src[1]);
  d2 = kwsysBase64DecodeChar(src[2]);
  d3 = kwsysBase64DecodeChar(src[3]);

  /* Make sure all characters were valid */

  if (d0 == 0xFF || d1 == 0xFF || d2 == 0xFF || d3 == 0xFF) {
    return 0;
  }

  /* Decode the 3 bytes */

  dest[0] = (unsigned char)(((d0 << 2) & 0xFC) | ((d1 >> 4) & 0x03));
  dest[1] = (unsigned char)(((d1 << 4) & 0xF0) | ((d2 >> 2) & 0x0F));
  dest[2] = (unsigned char)(((d2 << 6) & 0xC0) | ((d3 >> 0) & 0x3F));

  /* Return the number of bytes actually decoded */

  if (src[2] == '=') {
    return 1;
  }
  if (src[3] == '=') {
    return 2;
  }
  return 3;
}

/* Decode bytes from the input buffer and store the decoded stream
   into the output buffer until 'length' bytes have been decoded.  Return the
   real length of the decoded stream (which should be equal to 'length'). Note
   that the output buffer must be allocated by the caller.  If
   'max_input_length' is not null, then it specifies the number of encoded
   bytes that should be at most read from the input buffer. In that case the
   'length' parameter is ignored. This enables the caller to decode a stream
   without actually knowing how much decoded data to expect (of course, the
   buffer must be large enough). */
size_t kwsysBase64_Decode(const unsigned char* input, size_t length,
                          unsigned char* output, size_t max_input_length)
{
  const unsigned char* ptr = input;
  unsigned char* optr = output;

  /* Decode complete triplet */

  if (max_input_length) {
    const unsigned char* end = input + max_input_length;
    while (ptr < end) {
      int len = kwsysBase64_Decode3(ptr, optr);
      optr += len;
      if (len < 3) {
        return (size_t)(optr - output);
      }
      ptr += 4;
    }
  } else {
    unsigned char* oend = output + length;
    while ((oend - optr) >= 3) {
      int len = kwsysBase64_Decode3(ptr, optr);
      optr += len;
      if (len < 3) {
        return (size_t)(optr - output);
      }
      ptr += 4;
    }

    /* Decode the last triplet */

    if (oend - optr == 2) {
      unsigned char temp[3];
      int len = kwsysBase64_Decode3(ptr, temp);
      if (len >= 2) {
        optr[0] = temp[0];
        optr[1] = temp[1];
        optr += 2;
      } else if (len > 0) {
        optr[0] = temp[0];
        optr += 1;
      }
    } else if (oend - optr == 1) {
      unsigned char temp[3];
      int len = kwsysBase64_Decode3(ptr, temp);
      if (len > 0) {
        optr[0] = temp[0];
        optr += 1;
      }
    }
  }

  return (size_t)(optr - output);
}
