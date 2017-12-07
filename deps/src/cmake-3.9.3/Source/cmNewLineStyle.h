/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmNewLineStyle_h
#define cmNewLineStyle_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

class cmNewLineStyle
{
public:
  cmNewLineStyle();

  enum Style
  {
    Invalid,
    // LF = '\n', 0x0A, 10
    // CR = '\r', 0x0D, 13
    LF,  // Unix
    CRLF // Dos
  };

  void SetStyle(Style);
  Style GetStyle() const;

  bool IsValid() const;

  bool ReadFromArguments(const std::vector<std::string>& args,
                         std::string& errorString);

  const std::string GetCharacters() const;

private:
  Style NewLineStyle;
};

#endif
