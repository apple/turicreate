/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmStringReplaceHelper.h"

#include "cmMakefile.h"
#include <sstream>

cmStringReplaceHelper::cmStringReplaceHelper(const std::string& regex,
                                             const std::string& replace_expr,
                                             cmMakefile* makefile)
  : RegExString(regex)
  , RegularExpression(regex)
  , ReplaceExpression(replace_expr)
  , Makefile(makefile)
{
  this->ParseReplaceExpression();
}

bool cmStringReplaceHelper::Replace(const std::string& input,
                                    std::string& output)
{
  output.clear();

  // Scan through the input for all matches.
  std::string::size_type base = 0;
  while (this->RegularExpression.find(input.c_str() + base)) {
    if (this->Makefile != nullptr) {
      this->Makefile->ClearMatches();
      this->Makefile->StoreMatches(this->RegularExpression);
    }
    auto l2 = this->RegularExpression.start();
    auto r = this->RegularExpression.end();

    // Concatenate the part of the input that was not matched.
    output += input.substr(base, l2);

    // Make sure the match had some text.
    if (r - l2 == 0) {
      std::ostringstream error;
      error << "regex \"" << this->RegExString << "\" matched an empty string";
      this->ErrorString = error.str();
      return false;
    }

    // Concatenate the replacement for the match.
    for (const auto& replacement : this->Replacements) {
      if (replacement.Number < 0) {
        // This is just a plain-text part of the replacement.
        output += replacement.Value;
      } else {
        // Replace with part of the match.
        auto n = replacement.Number;
        auto start = this->RegularExpression.start(n);
        auto end = this->RegularExpression.end(n);
        auto len = input.length() - base;
        if ((start != std::string::npos) && (end != std::string::npos) &&
            (start <= len) && (end <= len)) {
          output += input.substr(base + start, end - start);
        } else {
          std::ostringstream error;
          error << "replace expression \"" << this->ReplaceExpression
                << "\" contains an out-of-range escape for regex \""
                << this->RegExString << "\"";
          this->ErrorString = error.str();
          return false;
        }
      }
    }

    // Move past the match.
    base += r;
  }

  // Concatenate the text after the last match.
  output += input.substr(base, input.length() - base);

  return true;
}

void cmStringReplaceHelper::ParseReplaceExpression()
{
  std::string::size_type l = 0;
  while (l < this->ReplaceExpression.length()) {
    auto r = this->ReplaceExpression.find('\\', l);
    if (r == std::string::npos) {
      r = this->ReplaceExpression.length();
      this->Replacements.push_back(this->ReplaceExpression.substr(l, r - l));
    } else {
      if (r - l > 0) {
        this->Replacements.push_back(this->ReplaceExpression.substr(l, r - l));
      }
      if (r == (this->ReplaceExpression.length() - 1)) {
        this->ValidReplaceExpression = false;
        this->ErrorString = "replace-expression ends in a backslash";
        return;
      }
      if ((this->ReplaceExpression[r + 1] >= '0') &&
          (this->ReplaceExpression[r + 1] <= '9')) {
        this->Replacements.push_back(this->ReplaceExpression[r + 1] - '0');
      } else if (this->ReplaceExpression[r + 1] == 'n') {
        this->Replacements.push_back("\n");
      } else if (this->ReplaceExpression[r + 1] == '\\') {
        this->Replacements.push_back("\\");
      } else {
        this->ValidReplaceExpression = false;
        std::ostringstream error;
        error << "Unknown escape \"" << this->ReplaceExpression.substr(r, 2)
              << "\" in replace-expression";
        this->ErrorString = error.str();
        return;
      }
      r += 2;
    }
    l = r;
  }
}
