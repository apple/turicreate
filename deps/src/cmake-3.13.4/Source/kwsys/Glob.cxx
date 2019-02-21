/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(Glob.hxx)

#include KWSYS_HEADER(Configure.hxx)

#include KWSYS_HEADER(RegularExpression.hxx)
#include KWSYS_HEADER(SystemTools.hxx)
#include KWSYS_HEADER(Directory.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#  include "Configure.hxx.in"
#  include "Directory.hxx.in"
#  include "Glob.hxx.in"
#  include "RegularExpression.hxx.in"
#  include "SystemTools.hxx.in"
#endif

#include <algorithm>
#include <string>
#include <vector>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
namespace KWSYS_NAMESPACE {
#if defined(_WIN32) || defined(__APPLE__) || defined(__CYGWIN__)
// On Windows and Apple, no difference between lower and upper case
#  define KWSYS_GLOB_CASE_INDEPENDENT
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
// Handle network paths
#  define KWSYS_GLOB_SUPPORT_NETWORK_PATHS
#endif

class GlobInternals
{
public:
  std::vector<std::string> Files;
  std::vector<kwsys::RegularExpression> Expressions;
};

Glob::Glob()
{
  this->Internals = new GlobInternals;
  this->Recurse = false;
  this->Relative = "";

  this->RecurseThroughSymlinks = true;
  // RecurseThroughSymlinks is true by default for backwards compatibility,
  // not because it's a good idea...
  this->FollowedSymlinkCount = 0;

  // Keep separate variables for directory listing for back compatibility
  this->ListDirs = true;
  this->RecurseListDirs = false;
}

Glob::~Glob()
{
  delete this->Internals;
}

std::vector<std::string>& Glob::GetFiles()
{
  return this->Internals->Files;
}

std::string Glob::PatternToRegex(const std::string& pattern,
                                 bool require_whole_string, bool preserve_case)
{
  // Incrementally build the regular expression from the pattern.
  std::string regex = require_whole_string ? "^" : "";
  std::string::const_iterator pattern_first = pattern.begin();
  std::string::const_iterator pattern_last = pattern.end();
  for (std::string::const_iterator i = pattern_first; i != pattern_last; ++i) {
    int c = *i;
    if (c == '*') {
      // A '*' (not between brackets) matches any string.
      // We modify this to not match slashes since the original glob
      // pattern documentation was meant for matching file name
      // components separated by slashes.
      regex += "[^/]*";
    } else if (c == '?') {
      // A '?' (not between brackets) matches any single character.
      // We modify this to not match slashes since the original glob
      // pattern documentation was meant for matching file name
      // components separated by slashes.
      regex += "[^/]";
    } else if (c == '[') {
      // Parse out the bracket expression.  It begins just after the
      // opening character.
      std::string::const_iterator bracket_first = i + 1;
      std::string::const_iterator bracket_last = bracket_first;

      // The first character may be complementation '!' or '^'.
      if (bracket_last != pattern_last &&
          (*bracket_last == '!' || *bracket_last == '^')) {
        ++bracket_last;
      }

      // If the next character is a ']' it is included in the brackets
      // because the bracket string may not be empty.
      if (bracket_last != pattern_last && *bracket_last == ']') {
        ++bracket_last;
      }

      // Search for the closing ']'.
      while (bracket_last != pattern_last && *bracket_last != ']') {
        ++bracket_last;
      }

      // Check whether we have a complete bracket string.
      if (bracket_last == pattern_last) {
        // The bracket string did not end, so it was opened simply by
        // a '[' that is supposed to be matched literally.
        regex += "\\[";
      } else {
        // Convert the bracket string to its regex equivalent.
        std::string::const_iterator k = bracket_first;

        // Open the regex block.
        regex += "[";

        // A regex range complement uses '^' instead of '!'.
        if (k != bracket_last && *k == '!') {
          regex += "^";
          ++k;
        }

        // Convert the remaining characters.
        for (; k != bracket_last; ++k) {
          // Backslashes must be escaped.
          if (*k == '\\') {
            regex += "\\";
          }

          // Store this character.
          regex += *k;
        }

        // Close the regex block.
        regex += "]";

        // Jump to the end of the bracket string.
        i = bracket_last;
      }
    } else {
      // A single character matches itself.
      int ch = c;
      if (!(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') ||
            ('0' <= ch && ch <= '9'))) {
        // Escape the non-alphanumeric character.
        regex += "\\";
      }
#if defined(KWSYS_GLOB_CASE_INDEPENDENT)
      else {
        // On case-insensitive systems file names are converted to lower
        // case before matching.
        if (!preserve_case) {
          ch = tolower(ch);
        }
      }
#endif
      (void)preserve_case;
      // Store the character.
      regex.append(1, static_cast<char>(ch));
    }
  }

  if (require_whole_string) {
    regex += "$";
  }
  return regex;
}

bool Glob::RecurseDirectory(std::string::size_type start,
                            const std::string& dir, GlobMessages* messages)
{
  kwsys::Directory d;
  if (!d.Load(dir)) {
    return true;
  }
  unsigned long cc;
  std::string realname;
  std::string fname;
  for (cc = 0; cc < d.GetNumberOfFiles(); cc++) {
    fname = d.GetFile(cc);
    if (fname == "." || fname == "..") {
      continue;
    }

    if (start == 0) {
      realname = dir + fname;
    } else {
      realname = dir + "/" + fname;
    }

#if defined(KWSYS_GLOB_CASE_INDEPENDENT)
    // On Windows and Apple, no difference between lower and upper case
    fname = kwsys::SystemTools::LowerCase(fname);
#endif

    bool isDir = kwsys::SystemTools::FileIsDirectory(realname);
    bool isSymLink = kwsys::SystemTools::FileIsSymlink(realname);

    if (isDir && (!isSymLink || this->RecurseThroughSymlinks)) {
      if (isSymLink) {
        ++this->FollowedSymlinkCount;
        std::string realPathErrorMessage;
        std::string canonicalPath(
          SystemTools::GetRealPath(dir, &realPathErrorMessage));

        if (!realPathErrorMessage.empty()) {
          if (messages) {
            messages->push_back(
              Message(Glob::error,
                      "Canonical path generation from path '" + dir +
                        "' failed! Reason: '" + realPathErrorMessage + "'"));
          }
          return false;
        }

        if (std::find(this->VisitedSymlinks.begin(),
                      this->VisitedSymlinks.end(),
                      canonicalPath) == this->VisitedSymlinks.end()) {
          if (this->RecurseListDirs) {
            // symlinks are treated as directories
            this->AddFile(this->Internals->Files, realname);
          }

          this->VisitedSymlinks.push_back(canonicalPath);
          if (!this->RecurseDirectory(start + 1, realname, messages)) {
            this->VisitedSymlinks.pop_back();

            return false;
          }
          this->VisitedSymlinks.pop_back();
        }
        // else we have already visited this symlink - prevent cyclic recursion
        else if (messages) {
          std::string message;
          for (std::vector<std::string>::const_iterator pathIt =
                 std::find(this->VisitedSymlinks.begin(),
                           this->VisitedSymlinks.end(), canonicalPath);
               pathIt != this->VisitedSymlinks.end(); ++pathIt) {
            message += *pathIt + "\n";
          }
          message += canonicalPath + "/" + fname;
          messages->push_back(Message(Glob::cyclicRecursion, message));
        }
      } else {
        if (this->RecurseListDirs) {
          this->AddFile(this->Internals->Files, realname);
        }
        if (!this->RecurseDirectory(start + 1, realname, messages)) {
          return false;
        }
      }
    } else {
      if (!this->Internals->Expressions.empty() &&
          this->Internals->Expressions.rbegin()->find(fname)) {
        this->AddFile(this->Internals->Files, realname);
      }
    }
  }

  return true;
}

void Glob::ProcessDirectory(std::string::size_type start,
                            const std::string& dir, GlobMessages* messages)
{
  // std::cout << "ProcessDirectory: " << dir << std::endl;
  bool last = (start == this->Internals->Expressions.size() - 1);
  if (last && this->Recurse) {
    this->RecurseDirectory(start, dir, messages);
    return;
  }

  if (start >= this->Internals->Expressions.size()) {
    return;
  }

  kwsys::Directory d;
  if (!d.Load(dir)) {
    return;
  }
  unsigned long cc;
  std::string realname;
  std::string fname;
  for (cc = 0; cc < d.GetNumberOfFiles(); cc++) {
    fname = d.GetFile(cc);
    if (fname == "." || fname == "..") {
      continue;
    }

    if (start == 0) {
      realname = dir + fname;
    } else {
      realname = dir + "/" + fname;
    }

#if defined(KWSYS_GLOB_CASE_INDEPENDENT)
    // On case-insensitive file systems convert to lower case for matching.
    fname = kwsys::SystemTools::LowerCase(fname);
#endif

    // std::cout << "Look at file: " << fname << std::endl;
    // std::cout << "Match: "
    // << this->Internals->TextExpressions[start].c_str() << std::endl;
    // std::cout << "Real name: " << realname << std::endl;

    if ((!last && !kwsys::SystemTools::FileIsDirectory(realname)) ||
        (!this->ListDirs && last &&
         kwsys::SystemTools::FileIsDirectory(realname))) {
      continue;
    }

    if (this->Internals->Expressions[start].find(fname)) {
      if (last) {
        this->AddFile(this->Internals->Files, realname);
      } else {
        this->ProcessDirectory(start + 1, realname, messages);
      }
    }
  }
}

bool Glob::FindFiles(const std::string& inexpr, GlobMessages* messages)
{
  std::string cexpr;
  std::string::size_type cc;
  std::string expr = inexpr;

  this->Internals->Expressions.clear();
  this->Internals->Files.clear();

  if (!kwsys::SystemTools::FileIsFullPath(expr)) {
    expr = kwsys::SystemTools::GetCurrentWorkingDirectory();
    expr += "/" + inexpr;
  }
  std::string fexpr = expr;

  std::string::size_type skip = 0;
  std::string::size_type last_slash = 0;
  for (cc = 0; cc < expr.size(); cc++) {
    if (cc > 0 && expr[cc] == '/' && expr[cc - 1] != '\\') {
      last_slash = cc;
    }
    if (cc > 0 && (expr[cc] == '[' || expr[cc] == '?' || expr[cc] == '*') &&
        expr[cc - 1] != '\\') {
      break;
    }
  }
  if (last_slash > 0) {
    // std::cout << "I can skip: " << fexpr.substr(0, last_slash)
    // << std::endl;
    skip = last_slash;
  }
  if (skip == 0) {
#if defined(KWSYS_GLOB_SUPPORT_NETWORK_PATHS)
    // Handle network paths
    if (expr[0] == '/' && expr[1] == '/') {
      int cnt = 0;
      for (cc = 2; cc < expr.size(); cc++) {
        if (expr[cc] == '/') {
          cnt++;
          if (cnt == 2) {
            break;
          }
        }
      }
      skip = int(cc + 1);
    } else
#endif
      // Handle drive letters on Windows
      if (expr[1] == ':' && expr[0] != '/') {
      skip = 2;
    }
  }

  if (skip > 0) {
    expr = expr.substr(skip);
  }

  cexpr = "";
  for (cc = 0; cc < expr.size(); cc++) {
    int ch = expr[cc];
    if (ch == '/') {
      if (!cexpr.empty()) {
        this->AddExpression(cexpr);
      }
      cexpr = "";
    } else {
      cexpr.append(1, static_cast<char>(ch));
    }
  }
  if (!cexpr.empty()) {
    this->AddExpression(cexpr);
  }

  // Handle network paths
  if (skip > 0) {
    this->ProcessDirectory(0, fexpr.substr(0, skip) + "/", messages);
  } else {
    this->ProcessDirectory(0, "/", messages);
  }
  return true;
}

void Glob::AddExpression(const std::string& expr)
{
  this->Internals->Expressions.push_back(
    kwsys::RegularExpression(this->PatternToRegex(expr)));
}

void Glob::SetRelative(const char* dir)
{
  if (!dir) {
    this->Relative = "";
    return;
  }
  this->Relative = dir;
}

const char* Glob::GetRelative()
{
  if (this->Relative.empty()) {
    return KWSYS_NULLPTR;
  }
  return this->Relative.c_str();
}

void Glob::AddFile(std::vector<std::string>& files, const std::string& file)
{
  if (!this->Relative.empty()) {
    files.push_back(kwsys::SystemTools::RelativePath(this->Relative, file));
  } else {
    files.push_back(file);
  }
}

} // namespace KWSYS_NAMESPACE
