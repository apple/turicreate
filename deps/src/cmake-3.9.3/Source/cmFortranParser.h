/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFortranParser_h
#define cmFortranParser_h

#if !defined(cmFortranLexer_cxx) && !defined(cmFortranParser_cxx)
#include "cmConfigure.h"

#include <set>
#include <string>
#include <vector>
#endif

#include <stddef.h> /* size_t */

/* Forward declare parser object type.  */
typedef struct cmFortranParser_s cmFortranParser;

/* Functions to enter/exit #include'd files in order.  */
bool cmFortranParser_FilePush(cmFortranParser* parser, const char* fname);
bool cmFortranParser_FilePop(cmFortranParser* parser);

/* Callbacks for lexer.  */
int cmFortranParser_Input(cmFortranParser* parser, char* buffer,
                          size_t bufferSize);

void cmFortranParser_StringStart(cmFortranParser* parser);
const char* cmFortranParser_StringEnd(cmFortranParser* parser);
void cmFortranParser_StringAppend(cmFortranParser* parser, char c);

void cmFortranParser_SetInInterface(cmFortranParser* parser, bool is_in);
bool cmFortranParser_GetInInterface(cmFortranParser* parser);

void cmFortranParser_SetInPPFalseBranch(cmFortranParser* parser, bool is_in);
bool cmFortranParser_GetInPPFalseBranch(cmFortranParser* parser);

void cmFortranParser_SetOldStartcond(cmFortranParser* parser, int arg);
int cmFortranParser_GetOldStartcond(cmFortranParser* parser);

/* Callbacks for parser.  */
void cmFortranParser_Error(cmFortranParser* parser, const char* message);
void cmFortranParser_RuleUse(cmFortranParser* parser, const char* name);
void cmFortranParser_RuleLineDirective(cmFortranParser* parser,
                                       const char* filename);
void cmFortranParser_RuleInclude(cmFortranParser* parser, const char* name);
void cmFortranParser_RuleModule(cmFortranParser* parser, const char* name);
void cmFortranParser_RuleDefine(cmFortranParser* parser, const char* name);
void cmFortranParser_RuleUndef(cmFortranParser* parser, const char* name);
void cmFortranParser_RuleIfdef(cmFortranParser* parser, const char* name);
void cmFortranParser_RuleIfndef(cmFortranParser* parser, const char* name);
void cmFortranParser_RuleIf(cmFortranParser* parser);
void cmFortranParser_RuleElif(cmFortranParser* parser);
void cmFortranParser_RuleElse(cmFortranParser* parser);
void cmFortranParser_RuleEndif(cmFortranParser* parser);

/* Define the parser stack element type.  */
struct cmFortran_yystype
{
  char* string;
};

/* Setup the proper yylex interface.  */
#define YY_EXTRA_TYPE cmFortranParser*
#define YY_DECL int cmFortran_yylex(YYSTYPE* yylvalp, yyscan_t yyscanner)
#define YYSTYPE cmFortran_yystype
#define YYSTYPE_IS_DECLARED 1
#if !defined(cmFortranLexer_cxx)
#define YY_NO_UNISTD_H
#include "cmFortranLexer.h"
#endif
#if !defined(cmFortranLexer_cxx)
#if !defined(cmFortranParser_cxx)
#undef YY_EXTRA_TYPE
#undef YY_DECL
#undef YYSTYPE
#undef YYSTYPE_IS_DECLARED
#endif
#endif

#if !defined(cmFortranLexer_cxx) && !defined(cmFortranParser_cxx)
#include <stack>

// Information about a single source file.
class cmFortranSourceInfo
{
public:
  // The name of the source file.
  std::string Source;

  // Set of provided and required modules.
  std::set<std::string> Provides;
  std::set<std::string> Requires;

  // Set of files included in the translation unit.
  std::set<std::string> Includes;
};

// Parser methods not included in generated interface.

// Get the current buffer processed by the lexer.
YY_BUFFER_STATE cmFortranLexer_GetCurrentBuffer(yyscan_t yyscanner);

// The parser entry point.
int cmFortran_yyparse(yyscan_t);

// Define parser object internal structure.
struct cmFortranFile
{
  cmFortranFile(FILE* file, YY_BUFFER_STATE buffer, const std::string& dir)
    : File(file)
    , Buffer(buffer)
    , Directory(dir)
    , LastCharWasNewline(false)
  {
  }
  FILE* File;
  YY_BUFFER_STATE Buffer;
  std::string Directory;
  bool LastCharWasNewline;
};

struct cmFortranParser_s
{
  cmFortranParser_s(std::vector<std::string> const& includes,
                    std::set<std::string> const& defines,
                    cmFortranSourceInfo& info);
  ~cmFortranParser_s();

  bool FindIncludeFile(const char* dir, const char* includeName,
                       std::string& fileName);

  // The include file search path.
  std::vector<std::string> IncludePath;

  // Lexical scanner instance.
  yyscan_t Scanner;

  // Stack of open files in the translation unit.
  std::stack<cmFortranFile> FileStack;

  // Buffer for string literals.
  std::string TokenString;

  // Error message text if a parser error occurs.
  std::string Error;

  // Flag for whether lexer is reading from inside an interface.
  bool InInterface;

  int OldStartcond;
  std::set<std::string> PPDefinitions;
  size_t InPPFalseBranch;
  std::stack<bool> SkipToEnd;

  // Information about the parsed source.
  cmFortranSourceInfo& Info;
};
#endif

#endif
