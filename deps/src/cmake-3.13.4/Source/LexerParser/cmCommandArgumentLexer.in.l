%{
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C++ and modified to build everywhere.

Run flex >= 2.6 like this:

  flex --nounistd -DFLEXINT_H --noline --header-file=cmCommandArgumentLexer.h -ocmCommandArgumentLexer.cxx cmCommandArgumentLexer.in.l

Modify cmCommandArgumentLexer.cxx:
  - remove trailing whitespace:              sed -i 's/\s*$//' cmCommandArgumentLexer.h cmCommandArgumentLexer.cxx
  - remove blank lines at end of file:       sed -i '${/^$/d;}' cmCommandArgumentLexer.h cmCommandArgumentLexer.cxx
  - #include "cmStandardLexer.h" at the top: sed -i '1i#include "cmStandardLexer.h"' cmCommandArgumentLexer.cxx

*/

/* IWYU pragma: no_forward_declare yyguts_t */

#include "cmCommandArgumentParserHelper.h"

/* Replace the lexer input function.  */
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) \
  { result = yyextra->LexInput(buf, max_size); }

/* Include the set of tokens from the parser.  */
#include "cmCommandArgumentParserTokens.h"

static const char *DCURLYVariable = "${";
static const char *RCURLYVariable = "}";
static const char *ATVariable = "@";
static const char *DOLLARVariable = "$";
static const char *LCURLYVariable = "{";
static const char *BSLASHVariable = "\\";

/*--------------------------------------------------------------------------*/
%}

%option prefix="cmCommandArgument_yy"

%option reentrant
%option noyywrap
%option nounput
%pointer
%s ESCAPES
%s NOESCAPES

%%

\$ENV\{ {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  yyextra->AllocateParserType(yylvalp, yytext+1, strlen(yytext)-2);
  return cal_ENVCURLY;
}

\$[A-Za-z0-9/_.+-]+\{ {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  yyextra->AllocateParserType(yylvalp, yytext+1, strlen(yytext)-2);
  return cal_NCURLY;
}

@[A-Za-z0-9/_.+-]+@ {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  yyextra->AllocateParserType(yylvalp, yytext+1, strlen(yytext)-2);
  return cal_ATNAME;
}

"${" {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  //yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  yylvalp->str = DCURLYVariable;
  return cal_DCURLY;
}

"}" {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  //yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  yylvalp->str = RCURLYVariable;
  return cal_RCURLY;
}

"@" {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  //yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  yylvalp->str = ATVariable;
  return cal_AT;
}

[A-Za-z0-9/_.+-]+ {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  return cal_NAME;
}

<ESCAPES>\\. {
  if ( !yyextra->HandleEscapeSymbol(yylvalp, *(yytext+1)) )
    {
    return cal_ERROR;
    }
  return cal_SYMBOL;
}

[^\${}\\@]+ {
  //std::cerr << __LINE__ << " here: [" << yytext << "]" << std::endl;
  yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  return cal_SYMBOL;
}

"$" {
  //yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  yylvalp->str = DOLLARVariable;
  return cal_DOLLAR;
}

"{" {
  //yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  yylvalp->str = LCURLYVariable;
  return cal_LCURLY;
}

<ESCAPES>"\\" {
  //yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  yylvalp->str = BSLASHVariable;
  return cal_BSLASH;
}

<NOESCAPES>"\\" {
  //yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  yylvalp->str = BSLASHVariable;
  return cal_SYMBOL;
}

%%

/*--------------------------------------------------------------------------*/
void cmCommandArgument_SetupEscapes(yyscan_t yyscanner, bool noEscapes)
{
  /* Hack into the internal flex-generated scanner to set the state.  */
  struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
  if(noEscapes) {
    BEGIN(NOESCAPES);
  } else {
    BEGIN(ESCAPES);
  }
}
