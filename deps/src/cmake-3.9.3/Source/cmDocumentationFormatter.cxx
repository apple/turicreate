/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDocumentationFormatter.h"

#include "cmDocumentationEntry.h"
#include "cmDocumentationSection.h"

#include <ostream>
#include <string.h>
#include <string>
#include <vector>

cmDocumentationFormatter::cmDocumentationFormatter()
  : TextWidth(77)
  , TextIndent("")
{
}

cmDocumentationFormatter::~cmDocumentationFormatter()
{
}

void cmDocumentationFormatter::PrintFormatted(std::ostream& os,
                                              const char* text)
{
  if (!text) {
    return;
  }
  const char* ptr = text;
  while (*ptr) {
    // Any ptrs starting in a space are treated as preformatted text.
    std::string preformatted;
    while (*ptr == ' ') {
      for (char ch = *ptr; ch && ch != '\n'; ++ptr, ch = *ptr) {
        preformatted.append(1, ch);
      }
      if (*ptr) {
        ++ptr;
        preformatted.append(1, '\n');
      }
    }
    if (!preformatted.empty()) {
      this->PrintPreformatted(os, preformatted.c_str());
    }

    // Other ptrs are treated as paragraphs.
    std::string paragraph;
    for (char ch = *ptr; ch && ch != '\n'; ++ptr, ch = *ptr) {
      paragraph.append(1, ch);
    }
    if (*ptr) {
      ++ptr;
      paragraph.append(1, '\n');
    }
    if (!paragraph.empty()) {
      this->PrintParagraph(os, paragraph.c_str());
    }
  }
}

void cmDocumentationFormatter::PrintPreformatted(std::ostream& os,
                                                 const char* text)
{
  bool newline = true;
  for (const char* ptr = text; *ptr; ++ptr) {
    if (newline && *ptr != '\n') {
      os << this->TextIndent;
      newline = false;
    }
    os << *ptr;
    if (*ptr == '\n') {
      newline = true;
    }
  }
  os << "\n";
}

void cmDocumentationFormatter::PrintParagraph(std::ostream& os,
                                              const char* text)
{
  os << this->TextIndent;
  this->PrintColumn(os, text);
  os << "\n";
}

void cmDocumentationFormatter::SetIndent(const char* indent)
{
  this->TextIndent = indent;
}

void cmDocumentationFormatter::PrintColumn(std::ostream& os, const char* text)
{
  // Print text arranged in an indented column of fixed witdh.
  const char* l = text;
  long column = 0;
  bool newSentence = false;
  bool firstLine = true;
  int width = this->TextWidth - static_cast<int>(strlen(this->TextIndent));

  // Loop until the end of the text.
  while (*l) {
    // Parse the next word.
    const char* r = l;
    while (*r && (*r != '\n') && (*r != ' ')) {
      ++r;
    }

    // Does it fit on this line?
    if (r - l < (width - column - (newSentence ? 1 : 0))) {
      // Word fits on this line.
      if (r > l) {
        if (column) {
          // Not first word on line.  Separate from the previous word
          // by a space, or two if this is a new sentence.
          if (newSentence) {
            os << "  ";
            column += 2;
          } else {
            os << " ";
            column += 1;
          }
        } else {
          // First word on line.  Print indentation unless this is the
          // first line.
          os << (firstLine ? "" : this->TextIndent);
        }

        // Print the word.
        os.write(l, static_cast<long>(r - l));
        newSentence = (*(r - 1) == '.');
      }

      if (*r == '\n') {
        // Text provided a newline.  Start a new line.
        os << "\n";
        ++r;
        column = 0;
        firstLine = false;
      } else {
        // No provided newline.  Continue this line.
        column += static_cast<long>(r - l);
      }
    } else {
      // Word does not fit on this line.  Start a new line.
      os << "\n";
      firstLine = false;
      if (r > l) {
        os << this->TextIndent;
        os.write(l, static_cast<long>(r - l));
        column = static_cast<long>(r - l);
        newSentence = (*(r - 1) == '.');
      } else {
        column = 0;
      }
    }

    // Move to beginning of next word.  Skip over whitespace.
    l = r;
    while (*l == ' ') {
      ++l;
    }
  }
}

void cmDocumentationFormatter::PrintSection(
  std::ostream& os, cmDocumentationSection const& section)
{
  os << section.GetName() << "\n";

  const std::vector<cmDocumentationEntry>& entries = section.GetEntries();
  for (std::vector<cmDocumentationEntry>::const_iterator op = entries.begin();
       op != entries.end(); ++op) {
    if (!op->Name.empty()) {
      os << "  " << op->Name;
      this->TextIndent = "                                 ";
      int align = static_cast<int>(strlen(this->TextIndent)) - 4;
      for (int i = static_cast<int>(op->Name.size()); i < align; ++i) {
        os << " ";
      }
      if (op->Name.size() > strlen(this->TextIndent) - 4) {
        os << "\n";
        os.write(this->TextIndent, strlen(this->TextIndent) - 2);
      }
      os << "= ";
      this->PrintColumn(os, op->Brief.c_str());
      os << "\n";
    } else {
      os << "\n";
      this->TextIndent = "";
      this->PrintFormatted(os, op->Brief.c_str());
    }
  }
  os << "\n";
}
