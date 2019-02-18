%{
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C++ and modified to build everywhere.

Run flex >= 2.6 like this:

  flex --nounistd -DFLEXINT_H --noline --header-file=cmExprLexer.h -ocmExprLexer.cxx cmExprLexer.in.l

Modify cmExprLexer.cxx:
  - remove trailing whitespace:              sed -i 's/\s*$//' cmExprLexer.h cmExprLexer.cxx
  - remove blank lines at end of file:       sed -i '${/^$/d;}' cmExprLexer.h cmExprLexer.cxx
  - #include "cmStandardLexer.h" at the top: sed -i '1i#include "cmStandardLexer.h"' cmExprLexer.cxx

*/

/* IWYU pragma: no_forward_declare yyguts_t */

#include "cmExprParserHelper.h"

/* Replace the lexer input function.  */
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) \
  { result = yyextra->LexInput(buf, max_size); }

/* Include the set of tokens from the parser.  */
#include "cmExprParserTokens.h"

#include <string>

/*--------------------------------------------------------------------------*/
%}

%option prefix="cmExpr_yy"

%option reentrant
%option noyywrap
%pointer

%%
[ \t]  {}

[0-9][0-9]* { yylvalp->Number = std::stoll(yytext, nullptr, 10); return exp_NUMBER; }
0[xX][0-9a-fA-F][0-9a-fA-F]* {  yylvalp->Number = std::stoll(yytext, nullptr, 16); return exp_NUMBER; }

"+" { return exp_PLUS; }
"-" { return exp_MINUS; }
"*" { return exp_TIMES; }
"/" { return exp_DIVIDE; }
"%" { return exp_MOD; }
"\|" { return exp_OR; }
"&" { return exp_AND; }
"^" { return exp_XOR; }
"~" { return exp_NOT; }
"<<" { return exp_SHIFTLEFT; }
">>" { return exp_SHIFTRIGHT; }
"(" { return exp_OPENPARENT; }
")" { return exp_CLOSEPARENT; }
. { yyextra->UnexpectedChar(yytext[0]); }

%%
