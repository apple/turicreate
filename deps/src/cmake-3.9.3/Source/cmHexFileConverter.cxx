/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmHexFileConverter.h"

#include "cmConfigure.h"
#include <stdio.h>
#include <string.h>

#include "cmSystemTools.h"

#define INTEL_HEX_MIN_LINE_LENGTH (1 + 8 + 2)
#define INTEL_HEX_MAX_LINE_LENGTH (1 + 8 + (256 * 2) + 2)
#define MOTOROLA_SREC_MIN_LINE_LENGTH (2 + 2 + 4 + 2)
#define MOTOROLA_SREC_MAX_LINE_LENGTH (2 + 2 + 8 + (256 * 2) + 2)

// might go to SystemTools ?
static bool cm_IsHexChar(char c)
{
  return (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) ||
          ((c >= 'A') && (c <= 'F')));
}

static unsigned int ChompStrlen(const char* line)
{
  if (line == CM_NULLPTR) {
    return 0;
  }
  unsigned int length = static_cast<unsigned int>(strlen(line));
  if ((line[length - 1] == '\n') || (line[length - 1] == '\r')) {
    length--;
  }
  if ((line[length - 1] == '\n') || (line[length - 1] == '\r')) {
    length--;
  }
  return length;
}

static bool OutputBin(FILE* file, const char* buf, unsigned int startIndex,
                      unsigned int stopIndex)
{
  bool success = true;
  char hexNumber[3];
  hexNumber[2] = '\0';
  char outBuf[256];
  unsigned int outBufCount = 0;
  for (unsigned int i = startIndex; i < stopIndex; i += 2) {
    hexNumber[0] = buf[i];
    hexNumber[1] = buf[i + 1];
    unsigned int convertedByte = 0;
    if (sscanf(hexNumber, "%x", &convertedByte) != 1) {
      success = false;
      break;
    }
    outBuf[outBufCount] = static_cast<char>(convertedByte & 0xff);
    outBufCount++;
  }
  if (success) {
    success = (fwrite(outBuf, 1, outBufCount, file) == outBufCount);
  }
  return success;
}

// see http://www.die.net/doc/linux/man/man5/srec.5.html
static bool ConvertMotorolaSrecLine(const char* buf, FILE* outFile)
{
  unsigned int slen = ChompStrlen(buf);
  if ((slen < MOTOROLA_SREC_MIN_LINE_LENGTH) ||
      (slen > MOTOROLA_SREC_MAX_LINE_LENGTH)) {
    return false;
  }

  // line length must be even
  if (slen % 2 == 1) {
    return false;
  }

  if (buf[0] != 'S') {
    return false;
  }

  unsigned int dataStart = 0;
  // ignore extra address records
  if ((buf[1] == '5') || (buf[1] == '7') || (buf[1] == '8') ||
      (buf[1] == '9')) {
    return true;
  }
  if (buf[1] == '1') {
    dataStart = 8;
  } else if (buf[1] == '2') {
    dataStart = 10;
  } else if (buf[1] == '3') {
    dataStart = 12;
  } else // unknown record type
  {
    return false;
  }

  // ignore the last two bytes  (checksum)
  return OutputBin(outFile, buf, dataStart, slen - 2);
}

// see http://en.wikipedia.org/wiki/Intel_hex
static bool ConvertIntelHexLine(const char* buf, FILE* outFile)
{
  unsigned int slen = ChompStrlen(buf);
  if ((slen < INTEL_HEX_MIN_LINE_LENGTH) ||
      (slen > INTEL_HEX_MAX_LINE_LENGTH)) {
    return false;
  }

  // line length must be odd
  if (slen % 2 == 0) {
    return false;
  }

  if ((buf[0] != ':') || (buf[7] != '0')) {
    return false;
  }

  unsigned int dataStart = 0;
  if ((buf[8] == '0') || (buf[8] == '1')) {
    dataStart = 9;
  }
  // ignore extra address records
  else if ((buf[8] == '2') || (buf[8] == '3') || (buf[8] == '4') ||
           (buf[8] == '5')) {
    return true;
  } else // unknown record type
  {
    return false;
  }

  // ignore the last two bytes  (checksum)
  return OutputBin(outFile, buf, dataStart, slen - 2);
}

cmHexFileConverter::FileType cmHexFileConverter::DetermineFileType(
  const char* inFileName)
{
  char buf[1024];
  FILE* inFile = cmsys::SystemTools::Fopen(inFileName, "rb");
  if (inFile == CM_NULLPTR) {
    return Binary;
  }

  if (!fgets(buf, 1024, inFile)) {
    buf[0] = 0;
  }
  fclose(inFile);
  FileType type = Binary;
  unsigned int minLineLength = 0;
  unsigned int maxLineLength = 0;
  if (buf[0] == ':') // might be an intel hex file
  {
    type = IntelHex;
    minLineLength = INTEL_HEX_MIN_LINE_LENGTH;
    maxLineLength = INTEL_HEX_MAX_LINE_LENGTH;
  } else if (buf[0] == 'S') // might be a motorola srec file
  {
    type = MotorolaSrec;
    minLineLength = MOTOROLA_SREC_MIN_LINE_LENGTH;
    maxLineLength = MOTOROLA_SREC_MAX_LINE_LENGTH;
  } else {
    return Binary;
  }

  unsigned int slen = ChompStrlen(buf);
  if ((slen < minLineLength) || (slen > maxLineLength)) {
    return Binary;
  }

  for (unsigned int i = 1; i < slen; i++) {
    if (!cm_IsHexChar(buf[i])) {
      return Binary;
    }
  }
  return type;
}

bool cmHexFileConverter::TryConvert(const char* inFileName,
                                    const char* outFileName)
{
  FileType type = DetermineFileType(inFileName);
  if (type == Binary) {
    return false;
  }

  // try to open the file
  FILE* inFile = cmsys::SystemTools::Fopen(inFileName, "rb");
  FILE* outFile = cmsys::SystemTools::Fopen(outFileName, "wb");
  if ((inFile == CM_NULLPTR) || (outFile == CM_NULLPTR)) {
    if (inFile != CM_NULLPTR) {
      fclose(inFile);
    }
    if (outFile != CM_NULLPTR) {
      fclose(outFile);
    }
    return false;
  }

  // convert them line by line
  bool success = false;
  char buf[1024];
  while (fgets(buf, 1024, inFile) != CM_NULLPTR) {
    if (type == MotorolaSrec) {
      success = ConvertMotorolaSrecLine(buf, outFile);
    } else if (type == IntelHex) {
      success = ConvertIntelHexLine(buf, outFile);
    }
    if (!success) {
      break;
    }
  }

  // close them again
  fclose(inFile);
  fclose(outFile);
  return success;
}
