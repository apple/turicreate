%{
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C and modified to build everywhere.

Run flex >= 2.6 like this:

  flex --nounistd -DFLEXINT_H --prefix=cmListFileLexer_yy -ocmListFileLexer.c cmListFileLexer.in.l

Modify cmListFileLexer.c:
  - remove trailing whitespace: sed -i 's/\s*$//' cmListFileLexer.c
  - remove blank lines at end of file
  - #include "cmStandardLexer.h" at the top
  - add cast in yy_scan_bytes for loop condition of _yybytes_len to size_t
  - change type of variable yyl under yy_find_action from yy_size_t to int

*/

/* IWYU pragma: no_forward_declare yyguts_t */

#ifdef WIN32
#include "cmsys/Encoding.h"
#endif

/* Setup the proper cmListFileLexer_yylex declaration.  */
#define YY_EXTRA_TYPE cmListFileLexer*
#define YY_DECL int cmListFileLexer_yylex (yyscan_t yyscanner, cmListFileLexer* lexer)

#include "cmListFileLexer.h"

/*--------------------------------------------------------------------------*/
struct cmListFileLexer_s
{
  cmListFileLexer_Token token;
  int bracket;
  int comment;
  int line;
  int column;
  int size;
  FILE* file;
  size_t cr;
  char* string_buffer;
  char* string_position;
  int string_left;
  yyscan_t scanner;
};

static void cmListFileLexerSetToken(cmListFileLexer* lexer, const char* text,
                                    int length);
static void cmListFileLexerAppend(cmListFileLexer* lexer, const char* text,
                                  int length);
static int cmListFileLexerInput(cmListFileLexer* lexer, char* buffer,
                                size_t bufferSize);
static void cmListFileLexerInit(cmListFileLexer* lexer);
static void cmListFileLexerDestroy(cmListFileLexer* lexer);

/* Replace the lexer input function.  */
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) \
  { result = cmListFileLexerInput(cmListFileLexer_yyget_extra(yyscanner), buf, max_size); }

/*--------------------------------------------------------------------------*/
%}

%option reentrant
%option yylineno
%option noyywrap
%pointer
%x STRING
%x BRACKET
%x BRACKETEND
%x COMMENT

MAKEVAR \$\([A-Za-z0-9_]*\)
UNQUOTED ([^ \t\r\n\(\)#\\\"[=]|\\.)
LEGACY {MAKEVAR}|{UNQUOTED}|\"({MAKEVAR}|{UNQUOTED}|[ \t[=])*\"

%%

<INITIAL,COMMENT>\n {
  lexer->token.type = cmListFileLexer_Token_Newline;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  ++lexer->line;
  lexer->column = 1;
  BEGIN(INITIAL);
  return 1;
}

#?\[=*\[\n? {
  const char* bracket = yytext;
  lexer->comment = yytext[0] == '#';
  if (lexer->comment) {
    lexer->token.type = cmListFileLexer_Token_CommentBracket;
    bracket += 1;
  } else {
    lexer->token.type = cmListFileLexer_Token_ArgumentBracket;
  }
  cmListFileLexerSetToken(lexer, "", 0);
  lexer->bracket = strchr(bracket+1, '[') - bracket;
  if (yytext[yyleng-1] == '\n') {
    ++lexer->line;
    lexer->column = 1;
  } else {
    lexer->column += yyleng;
  }
  BEGIN(BRACKET);
}

# {
  lexer->column += yyleng;
  BEGIN(COMMENT);
}

<COMMENT>.* {
  lexer->column += yyleng;
}

\( {
  lexer->token.type = cmListFileLexer_Token_ParenLeft;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

\) {
  lexer->token.type = cmListFileLexer_Token_ParenRight;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

[A-Za-z_][A-Za-z0-9_]* {
  lexer->token.type = cmListFileLexer_Token_Identifier;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

<BRACKET>\]=* {
  /* Handle ]]====]=======]*/
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
  if (yyleng == lexer->bracket) {
    BEGIN(BRACKETEND);
  }
}

<BRACKETEND>\] {
  lexer->column += yyleng;
  /* Erase the partial bracket from the token.  */
  lexer->token.length -= lexer->bracket;
  lexer->token.text[lexer->token.length] = 0;
  BEGIN(INITIAL);
  return 1;
}

<BRACKET>([^]\n])+ {
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
}

<BRACKET,BRACKETEND>\n {
  cmListFileLexerAppend(lexer, yytext, yyleng);
  ++lexer->line;
  lexer->column = 1;
  BEGIN(BRACKET);
}

<BRACKET,BRACKETEND>. {
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
  BEGIN(BRACKET);
}

<BRACKET,BRACKETEND><<EOF>> {
  lexer->token.type = cmListFileLexer_Token_BadBracket;
  BEGIN(INITIAL);
  return 1;
}

({UNQUOTED}|=|\[=*{UNQUOTED})({UNQUOTED}|[[=])* {
  lexer->token.type = cmListFileLexer_Token_ArgumentUnquoted;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

({MAKEVAR}|{UNQUOTED}|=|\[=*{LEGACY})({LEGACY}|[[=])* {
  lexer->token.type = cmListFileLexer_Token_ArgumentUnquoted;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

\[ {
  lexer->token.type = cmListFileLexer_Token_ArgumentUnquoted;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

\" {
  lexer->token.type = cmListFileLexer_Token_ArgumentQuoted;
  cmListFileLexerSetToken(lexer, "", 0);
  lexer->column += yyleng;
  BEGIN(STRING);
}

<STRING>([^\\\n\"]|\\.)+ {
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
}

<STRING>\\\n {
  /* Continuation: text is not part of string */
  ++lexer->line;
  lexer->column = 1;
}

<STRING>\n {
  cmListFileLexerAppend(lexer, yytext, yyleng);
  ++lexer->line;
  lexer->column = 1;
}

<STRING>\" {
  lexer->column += yyleng;
  BEGIN(INITIAL);
  return 1;
}

<STRING>. {
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
}

<STRING><<EOF>> {
  lexer->token.type = cmListFileLexer_Token_BadString;
  BEGIN(INITIAL);
  return 1;
}

[ \t\r]+ {
  lexer->token.type = cmListFileLexer_Token_Space;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

. {
  lexer->token.type = cmListFileLexer_Token_BadCharacter;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}

<<EOF>> {
  lexer->token.type = cmListFileLexer_Token_None;
  cmListFileLexerSetToken(lexer, 0, 0);
  return 0;
}

%%

/*--------------------------------------------------------------------------*/
static void cmListFileLexerSetToken(cmListFileLexer* lexer, const char* text,
                                    int length)
{
  /* Set the token line and column number.  */
  lexer->token.line = lexer->line;
  lexer->token.column = lexer->column;

  /* Use the same buffer if possible.  */
  if (lexer->token.text) {
    if (text && length < lexer->size) {
      strcpy(lexer->token.text, text);
      lexer->token.length = length;
      return;
    }
    free(lexer->token.text);
    lexer->token.text = 0;
    lexer->size = 0;
  }

  /* Need to extend the buffer.  */
  if (text) {
    lexer->token.text = strdup(text);
    lexer->token.length = length;
    lexer->size = length + 1;
  } else {
    lexer->token.length = 0;
  }
}

/*--------------------------------------------------------------------------*/
static void cmListFileLexerAppend(cmListFileLexer* lexer, const char* text,
                                  int length)
{
  char* temp;
  int newSize;

  /* If the appended text will fit in the buffer, do not reallocate.  */
  newSize = lexer->token.length + length + 1;
  if (lexer->token.text && newSize <= lexer->size) {
    strcpy(lexer->token.text + lexer->token.length, text);
    lexer->token.length += length;
    return;
  }

  /* We need to extend the buffer.  */
  temp = malloc(newSize);
  if (lexer->token.text) {
    memcpy(temp, lexer->token.text, lexer->token.length);
    free(lexer->token.text);
  }
  memcpy(temp + lexer->token.length, text, length);
  temp[lexer->token.length + length] = 0;
  lexer->token.text = temp;
  lexer->token.length += length;
  lexer->size = newSize;
}

/*--------------------------------------------------------------------------*/
static int cmListFileLexerInput(cmListFileLexer* lexer, char* buffer,
                                size_t bufferSize)
{
  if (lexer) {
    if (lexer->file) {
      /* Convert CRLF -> LF explicitly.  The C FILE "t"ext mode
         does not convert newlines on all platforms.  Move any
         trailing CR to the start of the buffer for the next read. */
      size_t cr = lexer->cr;
      size_t n;
      buffer[0] = '\r';
      n = fread(buffer + cr, 1, bufferSize - cr, lexer->file);
      if (n) {
        char* o = buffer;
        const char* i = buffer;
        const char* e;
        n += cr;
        cr = (buffer[n - 1] == '\r') ? 1 : 0;
        e = buffer + n - cr;
        while (i != e) {
          if (i[0] == '\r' && i[1] == '\n') {
            ++i;
          }
          *o++ = *i++;
        }
        n = o - buffer;
      } else {
        n = cr;
        cr = 0;
      }
      lexer->cr = cr;
      return n;
    } else if (lexer->string_left) {
      int length = lexer->string_left;
      if ((int)bufferSize < length) {
        length = (int)bufferSize;
      }
      memcpy(buffer, lexer->string_position, length);
      lexer->string_position += length;
      lexer->string_left -= length;
      return length;
    }
  }
  return 0;
}

/*--------------------------------------------------------------------------*/
static void cmListFileLexerInit(cmListFileLexer* lexer)
{
  if (lexer->file || lexer->string_buffer) {
    cmListFileLexer_yylex_init(&lexer->scanner);
    cmListFileLexer_yyset_extra(lexer, lexer->scanner);
  }
}

/*--------------------------------------------------------------------------*/
static void cmListFileLexerDestroy(cmListFileLexer* lexer)
{
  cmListFileLexerSetToken(lexer, 0, 0);
  if (lexer->file || lexer->string_buffer) {
    cmListFileLexer_yylex_destroy(lexer->scanner);
    if (lexer->file) {
      fclose(lexer->file);
      lexer->file = 0;
    }
    if (lexer->string_buffer) {
      free(lexer->string_buffer);
      lexer->string_buffer = 0;
      lexer->string_left = 0;
      lexer->string_position = 0;
    }
  }
}

/*--------------------------------------------------------------------------*/
cmListFileLexer* cmListFileLexer_New(void)
{
  cmListFileLexer* lexer = (cmListFileLexer*)malloc(sizeof(cmListFileLexer));
  if (!lexer) {
    return 0;
  }
  memset(lexer, 0, sizeof(*lexer));
  lexer->line = 1;
  lexer->column = 1;
  return lexer;
}

/*--------------------------------------------------------------------------*/
void cmListFileLexer_Delete(cmListFileLexer* lexer)
{
  cmListFileLexer_SetFileName(lexer, 0, 0);
  free(lexer);
}

/*--------------------------------------------------------------------------*/
static cmListFileLexer_BOM cmListFileLexer_ReadBOM(FILE* f)
{
  unsigned char b[2];
  if (fread(b, 1, 2, f) == 2) {
    if (b[0] == 0xEF && b[1] == 0xBB) {
      if (fread(b, 1, 1, f) == 1 && b[0] == 0xBF) {
        return cmListFileLexer_BOM_UTF8;
      }
    } else if (b[0] == 0xFE && b[1] == 0xFF) {
      /* UTF-16 BE */
      return cmListFileLexer_BOM_UTF16BE;
    } else if (b[0] == 0 && b[1] == 0) {
      if (fread(b, 1, 2, f) == 2 && b[0] == 0xFE && b[1] == 0xFF) {
        return cmListFileLexer_BOM_UTF32BE;
      }
    } else if (b[0] == 0xFF && b[1] == 0xFE) {
      fpos_t p;
      fgetpos(f, &p);
      if (fread(b, 1, 2, f) == 2 && b[0] == 0 && b[1] == 0) {
        return cmListFileLexer_BOM_UTF32LE;
      }
      fsetpos(f, &p);
      return cmListFileLexer_BOM_UTF16LE;
    }
  }
  rewind(f);
  return cmListFileLexer_BOM_None;
}

/*--------------------------------------------------------------------------*/
int cmListFileLexer_SetFileName(cmListFileLexer* lexer, const char* name,
                                cmListFileLexer_BOM* bom)
{
  int result = 1;
  cmListFileLexerDestroy(lexer);
  if (name) {
#ifdef _WIN32
    wchar_t* wname = cmsysEncoding_DupToWide(name);
    lexer->file = _wfopen(wname, L"rb");
    free(wname);
#else
    lexer->file = fopen(name, "rb");
#endif
    if (lexer->file) {
      if (bom) {
        *bom = cmListFileLexer_ReadBOM(lexer->file);
      }
    } else {
      result = 0;
    }
  }
  cmListFileLexerInit(lexer);
  return result;
}

/*--------------------------------------------------------------------------*/
int cmListFileLexer_SetString(cmListFileLexer* lexer, const char* text)
{
  int result = 1;
  cmListFileLexerDestroy(lexer);
  if (text) {
    int length = (int)strlen(text);
    lexer->string_buffer = (char*)malloc(length + 1);
    if (lexer->string_buffer) {
      strcpy(lexer->string_buffer, text);
      lexer->string_position = lexer->string_buffer;
      lexer->string_left = length;
    } else {
      result = 0;
    }
  }
  cmListFileLexerInit(lexer);
  return result;
}

/*--------------------------------------------------------------------------*/
cmListFileLexer_Token* cmListFileLexer_Scan(cmListFileLexer* lexer)
{
  if (!lexer->file) {
    return 0;
  }
  if (cmListFileLexer_yylex(lexer->scanner, lexer)) {
    return &lexer->token;
  } else {
    cmListFileLexer_SetFileName(lexer, 0, 0);
    return 0;
  }
}

/*--------------------------------------------------------------------------*/
long cmListFileLexer_GetCurrentLine(cmListFileLexer* lexer)
{
  if (lexer->file) {
    return lexer->line;
  } else {
    return 0;
  }
}

/*--------------------------------------------------------------------------*/
long cmListFileLexer_GetCurrentColumn(cmListFileLexer* lexer)
{
  if (lexer->file) {
    return lexer->column;
  } else {
    return 0;
  }
}

/*--------------------------------------------------------------------------*/
const char* cmListFileLexer_GetTypeAsString(cmListFileLexer* lexer,
                                            cmListFileLexer_Type type)
{
  (void)lexer;
  switch (type) {
    case cmListFileLexer_Token_None:
      return "nothing";
    case cmListFileLexer_Token_Space:
      return "space";
    case cmListFileLexer_Token_Newline:
      return "newline";
    case cmListFileLexer_Token_Identifier:
      return "identifier";
    case cmListFileLexer_Token_ParenLeft:
      return "left paren";
    case cmListFileLexer_Token_ParenRight:
      return "right paren";
    case cmListFileLexer_Token_ArgumentUnquoted:
      return "unquoted argument";
    case cmListFileLexer_Token_ArgumentQuoted:
      return "quoted argument";
    case cmListFileLexer_Token_ArgumentBracket:
      return "bracket argument";
    case cmListFileLexer_Token_CommentBracket:
      return "bracket comment";
    case cmListFileLexer_Token_BadCharacter:
      return "bad character";
    case cmListFileLexer_Token_BadBracket:
      return "unterminated bracket";
    case cmListFileLexer_Token_BadString:
      return "unterminated string";
  }
  return "unknown token";
}
