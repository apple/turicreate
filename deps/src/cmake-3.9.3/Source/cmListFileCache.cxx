/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmListFileCache.h"

#include "cmListFileLexer.h"
#include "cmMessenger.h"
#include "cmOutputConverter.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmake.h"

#include "cmConfigure.h"
#include <algorithm>
#include <assert.h>
#include <sstream>

struct cmListFileParser
{
  cmListFileParser(cmListFile* lf, cmListFileBacktrace const& lfbt,
                   cmMessenger* messenger, const char* filename);
  ~cmListFileParser();
  void IssueFileOpenError(std::string const& text) const;
  void IssueError(std::string const& text) const;
  bool ParseFile();
  bool ParseFunction(const char* name, long line);
  bool AddArgument(cmListFileLexer_Token* token,
                   cmListFileArgument::Delimiter delim);
  cmListFile* ListFile;
  cmListFileBacktrace Backtrace;
  cmMessenger* Messenger;
  const char* FileName;
  cmListFileLexer* Lexer;
  cmListFileFunction Function;
  enum
  {
    SeparationOkay,
    SeparationWarning,
    SeparationError
  } Separation;
};

cmListFileParser::cmListFileParser(cmListFile* lf,
                                   cmListFileBacktrace const& lfbt,
                                   cmMessenger* messenger,
                                   const char* filename)
  : ListFile(lf)
  , Backtrace(lfbt)
  , Messenger(messenger)
  , FileName(filename)
  , Lexer(cmListFileLexer_New())
{
}

cmListFileParser::~cmListFileParser()
{
  cmListFileLexer_Delete(this->Lexer);
}

void cmListFileParser::IssueFileOpenError(const std::string& text) const
{
  this->Messenger->IssueMessage(cmake::FATAL_ERROR, text, this->Backtrace);
}

void cmListFileParser::IssueError(const std::string& text) const
{
  cmListFileContext lfc;
  lfc.FilePath = this->FileName;
  lfc.Line = cmListFileLexer_GetCurrentLine(this->Lexer);
  cmListFileBacktrace lfbt = this->Backtrace;
  lfbt = lfbt.Push(lfc);
  this->Messenger->IssueMessage(cmake::FATAL_ERROR, text, lfbt);
  cmSystemTools::SetFatalErrorOccured();
}

bool cmListFileParser::ParseFile()
{
  // Open the file.
  cmListFileLexer_BOM bom;
  if (!cmListFileLexer_SetFileName(this->Lexer, this->FileName, &bom)) {
    this->IssueFileOpenError("cmListFileCache: error can not open file.");
    return false;
  }

  if (bom == cmListFileLexer_BOM_Broken) {
    cmListFileLexer_SetFileName(this->Lexer, CM_NULLPTR, CM_NULLPTR);
    this->IssueFileOpenError("Error while reading Byte-Order-Mark. "
                             "File not seekable?");
    return false;
  }

  // Verify the Byte-Order-Mark, if any.
  if (bom != cmListFileLexer_BOM_None && bom != cmListFileLexer_BOM_UTF8) {
    cmListFileLexer_SetFileName(this->Lexer, CM_NULLPTR, CM_NULLPTR);
    this->IssueFileOpenError(
      "File starts with a Byte-Order-Mark that is not UTF-8.");
    return false;
  }

  // Use a simple recursive-descent parser to process the token
  // stream.
  bool haveNewline = true;
  while (cmListFileLexer_Token* token = cmListFileLexer_Scan(this->Lexer)) {
    if (token->type == cmListFileLexer_Token_Space) {
    } else if (token->type == cmListFileLexer_Token_Newline) {
      haveNewline = true;
    } else if (token->type == cmListFileLexer_Token_CommentBracket) {
      haveNewline = false;
    } else if (token->type == cmListFileLexer_Token_Identifier) {
      if (haveNewline) {
        haveNewline = false;
        if (this->ParseFunction(token->text, token->line)) {
          this->ListFile->Functions.push_back(this->Function);
        } else {
          return false;
        }
      } else {
        std::ostringstream error;
        error << "Parse error.  Expected a newline, got "
              << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
              << " with text \"" << token->text << "\".";
        this->IssueError(error.str());
        return false;
      }
    } else {
      std::ostringstream error;
      error << "Parse error.  Expected a command name, got "
            << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
            << " with text \"" << token->text << "\".";
      this->IssueError(error.str());
      return false;
    }
  }
  return true;
}

bool cmListFile::ParseFile(const char* filename, cmMessenger* messenger,
                           cmListFileBacktrace const& lfbt)
{
  if (!cmSystemTools::FileExists(filename) ||
      cmSystemTools::FileIsDirectory(filename)) {
    return false;
  }

  bool parseError = false;

  {
    cmListFileParser parser(this, lfbt, messenger, filename);
    parseError = !parser.ParseFile();
  }

  return !parseError;
}

bool cmListFileParser::ParseFunction(const char* name, long line)
{
  // Ininitialize a new function call.
  this->Function = cmListFileFunction();
  this->Function.Name = name;
  this->Function.Line = line;

  // Command name has already been parsed.  Read the left paren.
  cmListFileLexer_Token* token;
  while ((token = cmListFileLexer_Scan(this->Lexer)) &&
         token->type == cmListFileLexer_Token_Space) {
  }
  if (!token) {
    std::ostringstream error;
    /* clang-format off */
    error << "Unexpected end of file.\n"
          << "Parse error.  Function missing opening \"(\".";
    /* clang-format on */
    this->IssueError(error.str());
    return false;
  }
  if (token->type != cmListFileLexer_Token_ParenLeft) {
    std::ostringstream error;
    error << "Parse error.  Expected \"(\", got "
          << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
          << " with text \"" << token->text << "\".";
    this->IssueError(error.str());
    return false;
  }

  // Arguments.
  unsigned long lastLine;
  unsigned long parenDepth = 0;
  this->Separation = SeparationOkay;
  while ((lastLine = cmListFileLexer_GetCurrentLine(this->Lexer),
          token = cmListFileLexer_Scan(this->Lexer))) {
    if (token->type == cmListFileLexer_Token_Space ||
        token->type == cmListFileLexer_Token_Newline) {
      this->Separation = SeparationOkay;
      continue;
    }
    if (token->type == cmListFileLexer_Token_ParenLeft) {
      parenDepth++;
      this->Separation = SeparationOkay;
      if (!this->AddArgument(token, cmListFileArgument::Unquoted)) {
        return false;
      }
    } else if (token->type == cmListFileLexer_Token_ParenRight) {
      if (parenDepth == 0) {
        return true;
      }
      parenDepth--;
      this->Separation = SeparationOkay;
      if (!this->AddArgument(token, cmListFileArgument::Unquoted)) {
        return false;
      }
      this->Separation = SeparationWarning;
    } else if (token->type == cmListFileLexer_Token_Identifier ||
               token->type == cmListFileLexer_Token_ArgumentUnquoted) {
      if (!this->AddArgument(token, cmListFileArgument::Unquoted)) {
        return false;
      }
      this->Separation = SeparationWarning;
    } else if (token->type == cmListFileLexer_Token_ArgumentQuoted) {
      if (!this->AddArgument(token, cmListFileArgument::Quoted)) {
        return false;
      }
      this->Separation = SeparationWarning;
    } else if (token->type == cmListFileLexer_Token_ArgumentBracket) {
      if (!this->AddArgument(token, cmListFileArgument::Bracket)) {
        return false;
      }
      this->Separation = SeparationError;
    } else if (token->type == cmListFileLexer_Token_CommentBracket) {
      this->Separation = SeparationError;
    } else {
      // Error.
      std::ostringstream error;
      error << "Parse error.  Function missing ending \")\".  "
            << "Instead found "
            << cmListFileLexer_GetTypeAsString(this->Lexer, token->type)
            << " with text \"" << token->text << "\".";
      this->IssueError(error.str());
      return false;
    }
  }

  std::ostringstream error;
  cmListFileContext lfc;
  lfc.FilePath = this->FileName;
  lfc.Line = lastLine;
  cmListFileBacktrace lfbt = this->Backtrace;
  lfbt = lfbt.Push(lfc);
  error << "Parse error.  Function missing ending \")\".  "
        << "End of file reached.";
  this->Messenger->IssueMessage(cmake::FATAL_ERROR, error.str(), lfbt);
  return false;
}

bool cmListFileParser::AddArgument(cmListFileLexer_Token* token,
                                   cmListFileArgument::Delimiter delim)
{
  cmListFileArgument a(token->text, delim, token->line);
  this->Function.Arguments.push_back(a);
  if (this->Separation == SeparationOkay) {
    return true;
  }
  bool isError = (this->Separation == SeparationError ||
                  delim == cmListFileArgument::Bracket);
  std::ostringstream m;
  cmListFileContext lfc;
  lfc.FilePath = this->FileName;
  lfc.Line = token->line;
  cmListFileBacktrace lfbt = this->Backtrace;
  lfbt = lfbt.Push(lfc);

  m << "Syntax " << (isError ? "Error" : "Warning") << " in cmake code at "
    << "column " << token->column << "\n"
    << "Argument not separated from preceding token by whitespace.";
  /* clang-format on */
  if (isError) {
    this->Messenger->IssueMessage(cmake::FATAL_ERROR, m.str(), lfbt);
    return false;
  }
  this->Messenger->IssueMessage(cmake::AUTHOR_WARNING, m.str(), lfbt);
  return true;
}

struct cmListFileBacktrace::Entry : public cmListFileContext
{
  Entry(cmListFileContext const& lfc, Entry* up)
    : cmListFileContext(lfc)
    , Up(up)
    , RefCount(0)
  {
    if (this->Up) {
      this->Up->Ref();
    }
  }
  ~Entry()
  {
    if (this->Up) {
      this->Up->Unref();
    }
  }
  void Ref() { ++this->RefCount; }
  void Unref()
  {
    if (--this->RefCount == 0) {
      delete this;
    }
  }
  Entry* Up;
  unsigned int RefCount;
};

cmListFileBacktrace::cmListFileBacktrace(cmStateSnapshot const& bottom,
                                         Entry* up,
                                         cmListFileContext const& lfc)
  : Bottom(bottom)
  , Cur(new Entry(lfc, up))
{
  assert(this->Bottom.IsValid());
  this->Cur->Ref();
}

cmListFileBacktrace::cmListFileBacktrace(cmStateSnapshot const& bottom,
                                         Entry* cur)
  : Bottom(bottom)
  , Cur(cur)
{
  if (this->Cur) {
    assert(this->Bottom.IsValid());
    this->Cur->Ref();
  }
}

cmListFileBacktrace::cmListFileBacktrace()
  : Bottom()
  , Cur(CM_NULLPTR)
{
}

cmListFileBacktrace::cmListFileBacktrace(cmStateSnapshot const& snapshot)
  : Bottom(snapshot.GetCallStackBottom())
  , Cur(CM_NULLPTR)
{
}

cmListFileBacktrace::cmListFileBacktrace(cmListFileBacktrace const& r)
  : Bottom(r.Bottom)
  , Cur(r.Cur)
{
  if (this->Cur) {
    assert(this->Bottom.IsValid());
    this->Cur->Ref();
  }
}

cmListFileBacktrace& cmListFileBacktrace::operator=(
  cmListFileBacktrace const& r)
{
  cmListFileBacktrace tmp(r);
  std::swap(this->Cur, tmp.Cur);
  std::swap(this->Bottom, tmp.Bottom);
  return *this;
}

cmListFileBacktrace::~cmListFileBacktrace()
{
  if (this->Cur) {
    this->Cur->Unref();
  }
}

cmListFileBacktrace cmListFileBacktrace::Push(std::string const& file) const
{
  // We are entering a file-level scope but have not yet reached
  // any specific line or command invocation within it.  This context
  // is useful to print when it is at the top but otherwise can be
  // skipped during call stack printing.
  cmListFileContext lfc;
  lfc.FilePath = file;
  return cmListFileBacktrace(this->Bottom, this->Cur, lfc);
}

cmListFileBacktrace cmListFileBacktrace::Push(
  cmListFileContext const& lfc) const
{
  return cmListFileBacktrace(this->Bottom, this->Cur, lfc);
}

cmListFileBacktrace cmListFileBacktrace::Pop() const
{
  assert(this->Cur);
  return cmListFileBacktrace(this->Bottom, this->Cur->Up);
}

cmListFileContext const& cmListFileBacktrace::Top() const
{
  if (this->Cur) {
    return *this->Cur;
  }
  static cmListFileContext const empty;
  return empty;
}

void cmListFileBacktrace::PrintTitle(std::ostream& out) const
{
  if (!this->Cur) {
    return;
  }
  cmOutputConverter converter(this->Bottom);
  cmListFileContext lfc = *this->Cur;
  if (!this->Bottom.GetState()->GetIsInTryCompile()) {
    lfc.FilePath = converter.ConvertToRelativePath(
      this->Bottom.GetState()->GetSourceDirectory(), lfc.FilePath);
  }
  out << (lfc.Line ? " at " : " in ") << lfc;
}

void cmListFileBacktrace::PrintCallStack(std::ostream& out) const
{
  if (!this->Cur || !this->Cur->Up) {
    return;
  }

  bool first = true;
  cmOutputConverter converter(this->Bottom);
  for (Entry* i = this->Cur->Up; i; i = i->Up) {
    if (i->Name.empty()) {
      // Skip this whole-file scope.  When we get here we already will
      // have printed a more-specific context within the file.
      continue;
    }
    if (first) {
      first = false;
      out << "Call Stack (most recent call first):\n";
    }
    cmListFileContext lfc = *i;
    if (!this->Bottom.GetState()->GetIsInTryCompile()) {
      lfc.FilePath = converter.ConvertToRelativePath(
        this->Bottom.GetState()->GetSourceDirectory(), lfc.FilePath);
    }
    out << "  " << lfc << "\n";
  }
}

std::ostream& operator<<(std::ostream& os, cmListFileContext const& lfc)
{
  os << lfc.FilePath;
  if (lfc.Line) {
    os << ":" << lfc.Line;
    if (!lfc.Name.empty()) {
      os << " (" << lfc.Name << ")";
    }
  }
  return os;
}

bool operator<(const cmListFileContext& lhs, const cmListFileContext& rhs)
{
  if (lhs.Line != rhs.Line) {
    return lhs.Line < rhs.Line;
  }
  return lhs.FilePath < rhs.FilePath;
}

bool operator==(const cmListFileContext& lhs, const cmListFileContext& rhs)
{
  return lhs.Line == rhs.Line && lhs.FilePath == rhs.FilePath;
}

bool operator!=(const cmListFileContext& lhs, const cmListFileContext& rhs)
{
  return !(lhs == rhs);
}
