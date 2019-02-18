/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmXMLSafe.h"

#include "cm_utf8.h"

#include <sstream>
#include <stdio.h>
#include <string.h>

cmXMLSafe::cmXMLSafe(const char* s)
  : Data(s)
  , Size(static_cast<unsigned long>(strlen(s)))
  , DoQuotes(true)
{
}

cmXMLSafe::cmXMLSafe(std::string const& s)
  : Data(s.c_str())
  , Size(static_cast<unsigned long>(s.length()))
  , DoQuotes(true)
{
}

cmXMLSafe& cmXMLSafe::Quotes(bool b)
{
  this->DoQuotes = b;
  return *this;
}

std::string cmXMLSafe::str()
{
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, cmXMLSafe const& self)
{
  char const* first = self.Data;
  char const* last = self.Data + self.Size;
  while (first != last) {
    unsigned int ch;
    if (const char* next = cm_utf8_decode_character(first, last, &ch)) {
      // http://www.w3.org/TR/REC-xml/#NT-Char
      if ((ch >= 0x20 && ch <= 0xD7FF) || (ch >= 0xE000 && ch <= 0xFFFD) ||
          (ch >= 0x10000 && ch <= 0x10FFFF) || ch == 0x9 || ch == 0xA ||
          ch == 0xD) {
        switch (ch) {
          // Escape XML control characters.
          case '&':
            os << "&amp;";
            break;
          case '<':
            os << "&lt;";
            break;
          case '>':
            os << "&gt;";
            break;
          case '"':
            os << (self.DoQuotes ? "&quot;" : "\"");
            break;
          case '\'':
            os << (self.DoQuotes ? "&apos;" : "'");
            break;
          case '\r':
            break; // Ignore CR
          // Print the UTF-8 character.
          default:
            os.write(first, next - first);
            break;
        }
      } else {
        // Use a human-readable hex value for this invalid character.
        char buf[16];
        sprintf(buf, "%X", ch);
        os << "[NON-XML-CHAR-0x" << buf << "]";
      }

      first = next;
    } else {
      ch = static_cast<unsigned char>(*first++);
      // Use a human-readable hex value for this invalid byte.
      char buf[16];
      sprintf(buf, "%X", ch);
      os << "[NON-UTF-8-BYTE-0x" << buf << "]";
    }
  }
  return os;
}
