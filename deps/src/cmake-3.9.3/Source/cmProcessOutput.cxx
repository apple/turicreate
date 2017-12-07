/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmProcessOutput.h"

#if defined(_WIN32)
#include <windows.h>
unsigned int cmProcessOutput::defaultCodepage =
  KWSYS_ENCODING_DEFAULT_CODEPAGE;
#endif

cmProcessOutput::Encoding cmProcessOutput::FindEncoding(
  std::string const& name)
{
  Encoding encoding = Auto;
  if (name == "UTF8") {
    encoding = UTF8;
  } else if (name == "NONE") {
    encoding = None;
  } else if (name == "ANSI") {
    encoding = ANSI;
  } else if (name == "OEM") {
    encoding = OEM;
  }
  return encoding;
}

cmProcessOutput::cmProcessOutput(Encoding encoding, unsigned int maxSize)
{
#if defined(_WIN32)
  codepage = 0;
  bufferSize = maxSize;
  if (encoding == None) {
    codepage = defaultCodepage;
  } else if (encoding == Auto) {
    codepage = GetConsoleCP();
  } else if (encoding == UTF8) {
    codepage = CP_UTF8;
  } else if (encoding == OEM) {
    codepage = GetOEMCP();
  }
  if (!codepage || encoding == ANSI) {
    codepage = GetACP();
  }
#else
  static_cast<void>(encoding);
  static_cast<void>(maxSize);
#endif
}

cmProcessOutput::~cmProcessOutput()
{
}

bool cmProcessOutput::DecodeText(std::string raw, std::string& decoded,
                                 size_t id)
{
#if !defined(_WIN32)
  static_cast<void>(id);
  decoded.swap(raw);
  return true;
#else
  bool success = true;
  decoded = raw;
  if (id > 0) {
    if (rawparts.size() < id) {
      rawparts.reserve(id);
      while (rawparts.size() < id)
        rawparts.push_back(std::string());
    }
    raw = rawparts[id - 1] + raw;
    rawparts[id - 1].clear();
    decoded = raw;
  }
  if (raw.size() > 0 && codepage != defaultCodepage) {
    success = false;
    CPINFOEXW cpinfo;
    if (id > 0 && bufferSize > 0 && raw.size() == bufferSize &&
        GetCPInfoExW(codepage, 0, &cpinfo) == 1 && cpinfo.MaxCharSize > 1) {
      if (cpinfo.MaxCharSize == 2 && cpinfo.LeadByte[0] != 0) {
        LPSTR prevChar =
          CharPrevExA(codepage, raw.c_str(), raw.c_str() + raw.size(), 0);
        bool isLeadByte =
          (*(prevChar + 1) == 0) && IsDBCSLeadByteEx(codepage, *prevChar);
        if (isLeadByte) {
          rawparts[id - 1] += *(raw.end() - 1);
          raw.resize(raw.size() - 1);
        }
        success = DoDecodeText(raw, decoded, NULL);
      } else {
        bool restoreDecoded = false;
        std::string firstDecoded = decoded;
        wchar_t lastChar = 0;
        for (UINT i = 0; i < cpinfo.MaxCharSize; i++) {
          success = DoDecodeText(raw, decoded, &lastChar);
          if (success && lastChar != 0) {
            if (i == 0) {
              firstDecoded = decoded;
            }
            if (lastChar == cpinfo.UnicodeDefaultChar) {
              restoreDecoded = true;
              rawparts[id - 1] = *(raw.end() - 1) + rawparts[id - 1];
              raw.resize(raw.size() - 1);
            } else {
              restoreDecoded = false;
              break;
            }
          } else {
            break;
          }
        }
        if (restoreDecoded) {
          decoded = firstDecoded;
          rawparts[id - 1].clear();
        }
      }
    } else {
      success = DoDecodeText(raw, decoded, NULL);
    }
  }
  return success;
#endif
}

bool cmProcessOutput::DecodeText(const char* data, size_t length,
                                 std::string& decoded, size_t id)
{
  return DecodeText(std::string(data, length), decoded, id);
}

bool cmProcessOutput::DecodeText(std::vector<char> raw,
                                 std::vector<char>& decoded, size_t id)
{
  std::string str;
  const bool success =
    DecodeText(std::string(raw.begin(), raw.end()), str, id);
  decoded.assign(str.begin(), str.end());
  return success;
}

#if defined(_WIN32)
bool cmProcessOutput::DoDecodeText(std::string raw, std::string& decoded,
                                   wchar_t* lastChar)
{
  bool success = false;
  const int wlength =
    MultiByteToWideChar(codepage, 0, raw.c_str(), int(raw.size()), NULL, 0);
  wchar_t* wdata = new wchar_t[wlength];
  int r = MultiByteToWideChar(codepage, 0, raw.c_str(), int(raw.size()), wdata,
                              wlength);
  if (r > 0) {
    if (lastChar) {
      *lastChar = 0;
      if ((wlength >= 2 && wdata[wlength - 2] != wdata[wlength - 1]) ||
          wlength >= 1) {
        *lastChar = wdata[wlength - 1];
      }
    }
    int length = WideCharToMultiByte(defaultCodepage, 0, wdata, wlength, NULL,
                                     0, NULL, NULL);
    char* data = new char[length];
    r = WideCharToMultiByte(defaultCodepage, 0, wdata, wlength, data, length,
                            NULL, NULL);
    if (r > 0) {
      decoded = std::string(data, length);
      success = true;
    }
    delete[] data;
  }
  delete[] wdata;
  return success;
}
#endif
