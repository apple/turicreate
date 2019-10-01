/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmNewLineStyle.h"

#include <stddef.h>

cmNewLineStyle::cmNewLineStyle()
  : NewLineStyle(Invalid)
{
}

bool cmNewLineStyle::IsValid() const
{
  return NewLineStyle != Invalid;
}

bool cmNewLineStyle::ReadFromArguments(const std::vector<std::string>& args,
                                       std::string& errorString)
{
  NewLineStyle = Invalid;

  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "NEWLINE_STYLE") {
      size_t const styleIndex = i + 1;
      if (args.size() > styleIndex) {
        std::string const& eol = args[styleIndex];
        if (eol == "LF" || eol == "UNIX") {
          NewLineStyle = LF;
          return true;
        }
        if (eol == "CRLF" || eol == "WIN32" || eol == "DOS") {
          NewLineStyle = CRLF;
          return true;
        }
        errorString = "NEWLINE_STYLE sets an unknown style, only LF, "
                      "CRLF, UNIX, DOS, and WIN32 are supported";
        return false;
      }
      errorString = "NEWLINE_STYLE must set a style: "
                    "LF, CRLF, UNIX, DOS, or WIN32";
      return false;
    }
  }
  return true;
}

const std::string cmNewLineStyle::GetCharacters() const
{
  switch (NewLineStyle) {
    case Invalid:
      return "";
    case LF:
      return "\n";
    case CRLF:
      return "\r\n";
  }
  return "";
}

void cmNewLineStyle::SetStyle(Style style)
{
  NewLineStyle = style;
}

cmNewLineStyle::Style cmNewLineStyle::GetStyle() const
{
  return NewLineStyle;
}
