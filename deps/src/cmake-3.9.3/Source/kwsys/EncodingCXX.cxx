/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifdef __osf__
#define _OSF_SOURCE
#define _POSIX_C_SOURCE 199506L
#define _XOPEN_SOURCE_EXTENDED
#endif

#include "kwsysPrivate.h"
#include KWSYS_HEADER(Encoding.hxx)
#include KWSYS_HEADER(Encoding.h)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "Encoding.h.in"
#include "Encoding.hxx.in"
#endif

#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

// Windows API.
#if defined(_WIN32)
#include <windows.h>

#include <ctype.h>
#include <shellapi.h>
#endif

namespace KWSYS_NAMESPACE {

Encoding::CommandLineArguments Encoding::CommandLineArguments::Main(
  int argc, char const* const* argv)
{
#ifdef _WIN32
  (void)argc;
  (void)argv;

  int ac;
  LPWSTR* w_av = CommandLineToArgvW(GetCommandLineW(), &ac);

  std::vector<std::string> av1(ac);
  std::vector<char const*> av2(ac);
  for (int i = 0; i < ac; i++) {
    av1[i] = ToNarrow(w_av[i]);
    av2[i] = av1[i].c_str();
  }
  LocalFree(w_av);
  return CommandLineArguments(ac, &av2[0]);
#else
  return CommandLineArguments(argc, argv);
#endif
}

Encoding::CommandLineArguments::CommandLineArguments(int ac,
                                                     char const* const* av)
{
  this->argv_.resize(ac + 1);
  for (int i = 0; i < ac; i++) {
    this->argv_[i] = strdup(av[i]);
  }
  this->argv_[ac] = 0;
}

Encoding::CommandLineArguments::CommandLineArguments(int ac,
                                                     wchar_t const* const* av)
{
  this->argv_.resize(ac + 1);
  for (int i = 0; i < ac; i++) {
    this->argv_[i] = kwsysEncoding_DupToNarrow(av[i]);
  }
  this->argv_[ac] = 0;
}

Encoding::CommandLineArguments::~CommandLineArguments()
{
  for (size_t i = 0; i < this->argv_.size(); i++) {
    free(argv_[i]);
  }
}

Encoding::CommandLineArguments::CommandLineArguments(
  const CommandLineArguments& other)
{
  this->argv_.resize(other.argv_.size());
  for (size_t i = 0; i < this->argv_.size(); i++) {
    this->argv_[i] = other.argv_[i] ? strdup(other.argv_[i]) : 0;
  }
}

Encoding::CommandLineArguments& Encoding::CommandLineArguments::operator=(
  const CommandLineArguments& other)
{
  if (this != &other) {
    size_t i;
    for (i = 0; i < this->argv_.size(); i++) {
      free(this->argv_[i]);
    }

    this->argv_.resize(other.argv_.size());
    for (i = 0; i < this->argv_.size(); i++) {
      this->argv_[i] = other.argv_[i] ? strdup(other.argv_[i]) : 0;
    }
  }

  return *this;
}

int Encoding::CommandLineArguments::argc() const
{
  return static_cast<int>(this->argv_.size() - 1);
}

char const* const* Encoding::CommandLineArguments::argv() const
{
  return &this->argv_[0];
}

#if KWSYS_STL_HAS_WSTRING

std::wstring Encoding::ToWide(const std::string& str)
{
  std::wstring wstr;
#if defined(_WIN32)
  const int wlength = MultiByteToWideChar(
    KWSYS_ENCODING_DEFAULT_CODEPAGE, 0, str.data(), int(str.size()), NULL, 0);
  if (wlength > 0) {
    wchar_t* wdata = new wchar_t[wlength];
    int r = MultiByteToWideChar(KWSYS_ENCODING_DEFAULT_CODEPAGE, 0, str.data(),
                                int(str.size()), wdata, wlength);
    if (r > 0) {
      wstr = std::wstring(wdata, wlength);
    }
    delete[] wdata;
  }
#else
  size_t pos = 0;
  size_t nullPos = 0;
  do {
    if (pos < str.size() && str.at(pos) != '\0') {
      wstr += ToWide(str.c_str() + pos);
    }
    nullPos = str.find('\0', pos);
    if (nullPos != std::string::npos) {
      pos = nullPos + 1;
      wstr += wchar_t('\0');
    }
  } while (nullPos != std::string::npos);
#endif
  return wstr;
}

std::string Encoding::ToNarrow(const std::wstring& str)
{
  std::string nstr;
#if defined(_WIN32)
  int length =
    WideCharToMultiByte(KWSYS_ENCODING_DEFAULT_CODEPAGE, 0, str.c_str(),
                        int(str.size()), NULL, 0, NULL, NULL);
  if (length > 0) {
    char* data = new char[length];
    int r =
      WideCharToMultiByte(KWSYS_ENCODING_DEFAULT_CODEPAGE, 0, str.c_str(),
                          int(str.size()), data, length, NULL, NULL);
    if (r > 0) {
      nstr = std::string(data, length);
    }
    delete[] data;
  }
#else
  size_t pos = 0;
  size_t nullPos = 0;
  do {
    if (pos < str.size() && str.at(pos) != '\0') {
      nstr += ToNarrow(str.c_str() + pos);
    }
    nullPos = str.find(wchar_t('\0'), pos);
    if (nullPos != std::string::npos) {
      pos = nullPos + 1;
      nstr += '\0';
    }
  } while (nullPos != std::string::npos);
#endif
  return nstr;
}

std::wstring Encoding::ToWide(const char* cstr)
{
  std::wstring wstr;
  size_t length = kwsysEncoding_mbstowcs(0, cstr, 0) + 1;
  if (length > 0) {
    std::vector<wchar_t> wchars(length);
    if (kwsysEncoding_mbstowcs(&wchars[0], cstr, length) > 0) {
      wstr = &wchars[0];
    }
  }
  return wstr;
}

std::string Encoding::ToNarrow(const wchar_t* wcstr)
{
  std::string str;
  size_t length = kwsysEncoding_wcstombs(0, wcstr, 0) + 1;
  if (length > 0) {
    std::vector<char> chars(length);
    if (kwsysEncoding_wcstombs(&chars[0], wcstr, length) > 0) {
      str = &chars[0];
    }
  }
  return str;
}

#if defined(_WIN32)
// Convert local paths to UNC style paths
std::wstring Encoding::ToWindowsExtendedPath(std::string const& source)
{
  std::wstring wsource = Encoding::ToWide(source);

  // Resolve any relative paths
  DWORD wfull_len;

  /* The +3 is a workaround for a bug in some versions of GetFullPathNameW that
   * won't return a large enough buffer size if the input is too small */
  wfull_len = GetFullPathNameW(wsource.c_str(), 0, NULL, NULL) + 3;
  std::vector<wchar_t> wfull(wfull_len);
  GetFullPathNameW(wsource.c_str(), wfull_len, &wfull[0], NULL);

  /* This should get the correct size without any extra padding from the
   * previous size workaround. */
  wfull_len = static_cast<DWORD>(wcslen(&wfull[0]));

  if (wfull_len >= 2 && isalpha(wfull[0]) &&
      wfull[1] == L':') { /* C:\Foo\bar\FooBar.txt */
    return L"\\\\?\\" + std::wstring(&wfull[0]);
  } else if (wfull_len >= 2 && wfull[0] == L'\\' &&
             wfull[1] == L'\\') { /* Starts with \\ */
    if (wfull_len >= 4 && wfull[2] == L'?' &&
        wfull[3] == L'\\') { /* Starts with \\?\ */
      if (wfull_len >= 8 && wfull[4] == L'U' && wfull[5] == L'N' &&
          wfull[6] == L'C' &&
          wfull[7] == L'\\') { /* \\?\UNC\Foo\bar\FooBar.txt */
        return std::wstring(&wfull[0]);
      } else if (wfull_len >= 6 && isalpha(wfull[4]) &&
                 wfull[5] == L':') { /* \\?\C:\Foo\bar\FooBar.txt */
        return std::wstring(&wfull[0]);
      } else if (wfull_len >= 5) { /* \\?\Foo\bar\FooBar.txt */
        return L"\\\\?\\UNC\\" + std::wstring(&wfull[4]);
      }
    } else if (wfull_len >= 4 && wfull[2] == L'.' &&
               wfull[3] == L'\\') { /* Starts with \\.\ a device name */
      if (wfull_len >= 6 && isalpha(wfull[4]) &&
          wfull[5] == L':') { /* \\.\C:\Foo\bar\FooBar.txt */
        return L"\\\\?\\" + std::wstring(&wfull[4]);
      } else if (wfull_len >=
                 5) { /* \\.\Foo\bar\ Device name is left unchanged */
        return std::wstring(&wfull[0]);
      }
    } else if (wfull_len >= 3) { /* \\Foo\bar\FooBar.txt */
      return L"\\\\?\\UNC\\" + std::wstring(&wfull[2]);
    }
  }

  // If this case has been reached, then the path is invalid.  Leave it
  // unchanged
  return Encoding::ToWide(source);
}
#endif

#endif // KWSYS_STL_HAS_WSTRING

} // namespace KWSYS_NAMESPACE
