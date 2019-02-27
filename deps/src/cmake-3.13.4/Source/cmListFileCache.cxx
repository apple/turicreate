/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmListFileCache.h"

#include "cmListFileLexer.h"
#include "cmMessenger.h"
#include "cmOutputConverter.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmake.h"

#include <algorithm>
#include <assert.h>
#include <memory>
#include <sstream>

cmCommandContext::cmCommandName& cmCommandContext::cmCommandName::operator=(
  std::string const& name)
{
  this->Original = name;
  this->Lower = cmSystemTools::LowerCase(name);
  return *this;
}

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
    cmListFileLexer_SetFileName(this->Lexer, nullptr, nullptr);
    this->IssueFileOpenError("Error while reading Byte-Order-Mark. "
                             "File not seekable?");
    return false;
  }

  // Verify the Byte-Order-Mark, if any.
  if (bom != cmListFileLexer_BOM_None && bom != cmListFileLexer_BOM_UTF8) {
    cmListFileLexer_SetFileName(this->Lexer, nullptr, nullptr);
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
  this->Function.Arguments.emplace_back(token->text, delim, token->line);
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

// We hold either the bottom scope of a directory or a call/file context.
// Discriminate these cases via the parent pointer.
struct cmListFileBacktrace::Entry
{
  Entry(cmStateSnapshot bottom)
    : Bottom(bottom)
  {
  }

  Entry(std::shared_ptr<Entry const> parent, cmListFileContext lfc)
    : Context(std::move(lfc))
    , Parent(std::move(parent))
  {
  }

  ~Entry()
  {
    if (this->Parent) {
      this->Context.~cmListFileContext();
    } else {
      this->Bottom.~cmStateSnapshot();
    }
  }

  bool IsBottom() const { return !this->Parent; }

  union
  {
    cmStateSnapshot Bottom;
    cmListFileContext Context;
  };
  std::shared_ptr<Entry const> Parent;
};

cmListFileBacktrace::cmListFileBacktrace(cmStateSnapshot const& snapshot)
  : TopEntry(std::make_shared<Entry const>(snapshot.GetCallStackBottom()))
{
}

cmListFileBacktrace::cmListFileBacktrace(std::shared_ptr<Entry const> parent,
                                         cmListFileContext const& lfc)
  : TopEntry(std::make_shared<Entry const>(std::move(parent), lfc))
{
}

cmListFileBacktrace::cmListFileBacktrace(std::shared_ptr<Entry const> top)
  : TopEntry(std::move(top))
{
}

cmStateSnapshot cmListFileBacktrace::GetBottom() const
{
  cmStateSnapshot bottom;
  if (Entry const* cur = this->TopEntry.get()) {
    while (Entry const* parent = cur->Parent.get()) {
      cur = parent;
    }
    bottom = cur->Bottom;
  }
  return bottom;
}

cmListFileBacktrace cmListFileBacktrace::Push(std::string const& file) const
{
  // We are entering a file-level scope but have not yet reached
  // any specific line or command invocation within it.  This context
  // is useful to print when it is at the top but otherwise can be
  // skipped during call stack printing.
  cmListFileContext lfc;
  lfc.FilePath = file;
  return this->Push(lfc);
}

cmListFileBacktrace cmListFileBacktrace::Push(
  cmListFileContext const& lfc) const
{
  assert(this->TopEntry);
  assert(!this->TopEntry->IsBottom() || this->TopEntry->Bottom.IsValid());
  return cmListFileBacktrace(this->TopEntry, lfc);
}

cmListFileBacktrace cmListFileBacktrace::Pop() const
{
  assert(this->TopEntry);
  assert(!this->TopEntry->IsBottom());
  return cmListFileBacktrace(this->TopEntry->Parent);
}

cmListFileContext const& cmListFileBacktrace::Top() const
{
  assert(this->TopEntry);
  assert(!this->TopEntry->IsBottom());
  return this->TopEntry->Context;
}

void cmListFileBacktrace::PrintTitle(std::ostream& out) const
{
  // The title exists only if we have a call on top of the bottom.
  if (!this->TopEntry || this->TopEntry->IsBottom()) {
    return;
  }
  cmListFileContext lfc = this->TopEntry->Context;
  cmStateSnapshot bottom = this->GetBottom();
  cmOutputConverter converter(bottom);
  if (!bottom.GetState()->GetIsInTryCompile()) {
    lfc.FilePath = converter.ConvertToRelativePath(
      bottom.GetState()->GetSourceDirectory(), lfc.FilePath);
  }
  out << (lfc.Line ? " at " : " in ") << lfc;
}

void cmListFileBacktrace::PrintCallStack(std::ostream& out) const
{
  // The call stack exists only if we have at least two calls on top
  // of the bottom.
  if (!this->TopEntry || this->TopEntry->IsBottom() ||
      this->TopEntry->Parent->IsBottom()) {
    return;
  }

  bool first = true;
  cmStateSnapshot bottom = this->GetBottom();
  cmOutputConverter converter(bottom);
  for (Entry const* cur = this->TopEntry->Parent.get(); !cur->IsBottom();
       cur = cur->Parent.get()) {
    if (cur->Context.Name.empty()) {
      // Skip this whole-file scope.  When we get here we already will
      // have printed a more-specific context within the file.
      continue;
    }
    if (first) {
      first = false;
      out << "Call Stack (most recent call first):\n";
    }
    cmListFileContext lfc = cur->Context;
    if (!bottom.GetState()->GetIsInTryCompile()) {
      lfc.FilePath = converter.ConvertToRelativePath(
        bottom.GetState()->GetSourceDirectory(), lfc.FilePath);
    }
    out << "  " << lfc << "\n";
  }
}

size_t cmListFileBacktrace::Depth() const
{
  size_t depth = 0;
  if (Entry const* cur = this->TopEntry.get()) {
    for (; !cur->IsBottom(); cur = cur->Parent.get()) {
      ++depth;
    }
  }
  return depth;
}

bool cmListFileBacktrace::Empty() const
{
  return !this->TopEntry || this->TopEntry->IsBottom();
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
