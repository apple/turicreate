/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCommandArgumentParserHelper.h"

#include "cmCommandArgumentLexer.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmake.h"

#include <iostream>
#include <sstream>
#include <string.h>

int cmCommandArgument_yyparse(yyscan_t yyscanner);
//
cmCommandArgumentParserHelper::cmCommandArgumentParserHelper()
{
  this->WarnUninitialized = false;
  this->CheckSystemVars = false;
  this->FileLine = -1;
  this->FileName = CM_NULLPTR;
  this->RemoveEmpty = true;
  this->EmptyVariable[0] = 0;
  strcpy(this->DCURLYVariable, "${");
  strcpy(this->RCURLYVariable, "}");
  strcpy(this->ATVariable, "@");
  strcpy(this->DOLLARVariable, "$");
  strcpy(this->LCURLYVariable, "{");
  strcpy(this->BSLASHVariable, "\\");

  this->NoEscapeMode = false;
  this->ReplaceAtSyntax = false;
}

cmCommandArgumentParserHelper::~cmCommandArgumentParserHelper()
{
  this->CleanupParser();
}

void cmCommandArgumentParserHelper::SetLineFile(long line, const char* file)
{
  this->FileLine = line;
  this->FileName = file;
}

char* cmCommandArgumentParserHelper::AddString(const std::string& str)
{
  if (str.empty()) {
    return this->EmptyVariable;
  }
  char* stVal = new char[str.size() + 1];
  strcpy(stVal, str.c_str());
  this->Variables.push_back(stVal);
  return stVal;
}

char* cmCommandArgumentParserHelper::ExpandSpecialVariable(const char* key,
                                                           const char* var)
{
  if (!key) {
    return this->ExpandVariable(var);
  }
  if (!var) {
    return this->EmptyVariable;
  }
  if (strcmp(key, "ENV") == 0) {
    std::string str;
    if (cmSystemTools::GetEnv(var, str)) {
      if (this->EscapeQuotes) {
        return this->AddString(cmSystemTools::EscapeQuotes(str));
      }
      return this->AddString(str);
    }
    return this->EmptyVariable;
  }
  if (strcmp(key, "CACHE") == 0) {
    if (const char* c =
          this->Makefile->GetState()->GetInitializedCacheValue(var)) {
      if (this->EscapeQuotes) {
        return this->AddString(cmSystemTools::EscapeQuotes(c));
      }
      return this->AddString(c);
    }
    return this->EmptyVariable;
  }
  std::ostringstream e;
  e << "Syntax $" << key << "{} is not supported.  "
    << "Only ${}, $ENV{}, and $CACHE{} are allowed.";
  this->SetError(e.str());
  return CM_NULLPTR;
}

char* cmCommandArgumentParserHelper::ExpandVariable(const char* var)
{
  if (!var) {
    return CM_NULLPTR;
  }
  if (this->FileLine >= 0 && strcmp(var, "CMAKE_CURRENT_LIST_LINE") == 0) {
    std::ostringstream ostr;
    ostr << this->FileLine;
    return this->AddString(ostr.str());
  }
  const char* value = this->Makefile->GetDefinition(var);
  if (!value && !this->RemoveEmpty) {
    // check to see if we need to print a warning
    // if strict mode is on and the variable has
    // not been "cleared"/initialized with a set(foo ) call
    if (this->WarnUninitialized && !this->Makefile->VariableInitialized(var)) {
      if (this->CheckSystemVars ||
          cmSystemTools::IsSubDirectory(this->FileName,
                                        this->Makefile->GetHomeDirectory()) ||
          cmSystemTools::IsSubDirectory(
            this->FileName, this->Makefile->GetHomeOutputDirectory())) {
        std::ostringstream msg;
        msg << "uninitialized variable \'" << var << "\'";
        this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, msg.str());
      }
    }
    return CM_NULLPTR;
  }
  if (this->EscapeQuotes && value) {
    return this->AddString(cmSystemTools::EscapeQuotes(value));
  }
  return this->AddString(value ? value : "");
}

char* cmCommandArgumentParserHelper::ExpandVariableForAt(const char* var)
{
  if (this->ReplaceAtSyntax) {
    // try to expand the variable
    char* ret = this->ExpandVariable(var);
    // if the return was 0 and we want to replace empty strings
    // then return an empty string
    if (!ret && this->RemoveEmpty) {
      return this->AddString("");
    }
    // if the ret was not 0, then return it
    if (ret) {
      return ret;
    }
  }
  // at this point we want to put it back because of one of these cases:
  // - this->ReplaceAtSyntax is false
  // - this->ReplaceAtSyntax is true, but this->RemoveEmpty is false,
  //   and the variable was not defined
  std::string ref = "@";
  ref += var;
  ref += "@";
  return this->AddString(ref);
}

char* cmCommandArgumentParserHelper::CombineUnions(char* in1, char* in2)
{
  if (!in1) {
    return in2;
  }
  if (!in2) {
    return in1;
  }
  size_t len = strlen(in1) + strlen(in2) + 1;
  char* out = new char[len];
  strcpy(out, in1);
  strcat(out, in2);
  this->Variables.push_back(out);
  return out;
}

void cmCommandArgumentParserHelper::AllocateParserType(
  cmCommandArgumentParserHelper::ParserType* pt, const char* str, int len)
{
  pt->str = CM_NULLPTR;
  if (len == 0) {
    len = static_cast<int>(strlen(str));
  }
  if (len == 0) {
    return;
  }
  pt->str = new char[len + 1];
  strncpy(pt->str, str, len);
  pt->str[len] = 0;
  this->Variables.push_back(pt->str);
}

bool cmCommandArgumentParserHelper::HandleEscapeSymbol(
  cmCommandArgumentParserHelper::ParserType* pt, char symbol)
{
  switch (symbol) {
    case '\\':
    case '"':
    case ' ':
    case '#':
    case '(':
    case ')':
    case '$':
    case '@':
    case '^':
      this->AllocateParserType(pt, &symbol, 1);
      break;
    case ';':
      this->AllocateParserType(pt, "\\;", 2);
      break;
    case 't':
      this->AllocateParserType(pt, "\t", 1);
      break;
    case 'n':
      this->AllocateParserType(pt, "\n", 1);
      break;
    case 'r':
      this->AllocateParserType(pt, "\r", 1);
      break;
    case '0':
      this->AllocateParserType(pt, "\0", 1);
      break;
    default: {
      std::ostringstream e;
      e << "Invalid escape sequence \\" << symbol;
      this->SetError(e.str());
    }
      return false;
  }
  return true;
}

void cmCommandArgument_SetupEscapes(yyscan_t yyscanner, bool noEscapes);

int cmCommandArgumentParserHelper::ParseString(const char* str, int verb)
{
  if (!str) {
    return 0;
  }
  this->Verbose = verb;
  this->InputBuffer = str;
  this->InputBufferPos = 0;
  this->CurrentLine = 0;

  this->Result = "";

  yyscan_t yyscanner;
  cmCommandArgument_yylex_init(&yyscanner);
  cmCommandArgument_yyset_extra(this, yyscanner);
  cmCommandArgument_SetupEscapes(yyscanner, this->NoEscapeMode);
  int res = cmCommandArgument_yyparse(yyscanner);
  cmCommandArgument_yylex_destroy(yyscanner);
  if (res != 0) {
    return 0;
  }

  this->CleanupParser();

  if (Verbose) {
    std::cerr << "Expanding [" << str << "] produced: [" << this->Result << "]"
              << std::endl;
  }
  return 1;
}

void cmCommandArgumentParserHelper::CleanupParser()
{
  std::vector<char*>::iterator sit;
  for (sit = this->Variables.begin(); sit != this->Variables.end(); ++sit) {
    delete[] * sit;
  }
  this->Variables.erase(this->Variables.begin(), this->Variables.end());
}

int cmCommandArgumentParserHelper::LexInput(char* buf, int maxlen)
{
  if (maxlen < 1) {
    return 0;
  }
  if (this->InputBufferPos < this->InputBuffer.size()) {
    buf[0] = this->InputBuffer[this->InputBufferPos++];
    if (buf[0] == '\n') {
      this->CurrentLine++;
    }
    return (1);
  }
  buf[0] = '\n';
  return (0);
}

void cmCommandArgumentParserHelper::Error(const char* str)
{
  unsigned long pos = static_cast<unsigned long>(this->InputBufferPos);
  std::ostringstream ostr;
  ostr << str << " (" << pos << ")";
  this->SetError(ostr.str());
}

void cmCommandArgumentParserHelper::SetMakefile(const cmMakefile* mf)
{
  this->Makefile = mf;
  this->WarnUninitialized = mf->GetCMakeInstance()->GetWarnUninitialized();
  this->CheckSystemVars = mf->GetCMakeInstance()->GetCheckSystemVars();
}

void cmCommandArgumentParserHelper::SetResult(const char* value)
{
  if (!value) {
    this->Result = "";
    return;
  }
  this->Result = value;
}

void cmCommandArgumentParserHelper::SetError(std::string const& msg)
{
  // Keep only the first error.
  if (this->ErrorString.empty()) {
    this->ErrorString = msg;
  }
}
