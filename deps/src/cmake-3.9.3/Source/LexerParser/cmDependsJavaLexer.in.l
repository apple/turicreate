%{
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C++ and modified to build everywhere.

Run flex >= 2.6 like this:

  flex --nounistd -DFLEXINT_H --prefix=cmDependsJava_yy --header-file=cmDependsJavaLexer.h -ocmDependsJavaLexer.cxx cmDependsJavaLexer.in.l

Modify cmDependsJavaLexer.cxx:
  - remove trailing whitespace: sed -i 's/\s*$//' cmDependsJavaLexer.h cmDependsJavaLexer.cxx
  - remove blank lines at end of file
  - #include "cmStandardLexer.h" at the top
  - add cast in yy_scan_bytes for loop condition of _yybytes_len to size_t

*/

/* IWYU pragma: no_forward_declare yyguts_t */

#include <iostream>

#include "cmDependsJavaParserHelper.h"

/* Replace the lexer input function.  */
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) \
  { result = yyextra->LexInput(buf, max_size); }

/* Include the set of tokens from the parser.  */
#include "cmDependsJavaParserTokens.h"

#define KEYWORD yylvalp->str = 0
#define SYMBOL yylvalp->str = 0
#define PRIMITIVE  yylvalp->str = 0

/*--------------------------------------------------------------------------*/
%}

%option reentrant
%option noyywrap
%pointer

%x comment
%x string

%%
"/*" { BEGIN(comment); }
<comment>"*/" { BEGIN(INITIAL); }
<comment>.|\n {}

\" { BEGIN(string); }
<string>\" { BEGIN(INITIAL); return jp_STRINGLITERAL; }
<string>. {}

abstract { KEYWORD; return jp_ABSTRACT; }
assert { KEYWORD; return jp_ASSERT; }
boolean { KEYWORD; return jp_BOOLEAN_TYPE; }
break { KEYWORD; return jp_BREAK; }
byte { KEYWORD; return jp_BYTE_TYPE; }
case { KEYWORD; return jp_CASE; }
catch { KEYWORD; return jp_CATCH; }
char { KEYWORD; return jp_CHAR_TYPE; }
class { KEYWORD; return jp_CLASS; }
continue { KEYWORD; return jp_CONTINUE; }
default { KEYWORD; return jp_DEFAULT; }
do { KEYWORD; return jp_DO; }
double { KEYWORD; return jp_DOUBLE_TYPE; }
else { KEYWORD; return jp_ELSE; }
extends { KEYWORD; return jp_EXTENDS; }
final { KEYWORD; return jp_FINAL; }
finally { KEYWORD; return jp_FINALLY; }
float { KEYWORD; return jp_FLOAT_TYPE; }
for { KEYWORD; return jp_FOR; }
if { KEYWORD; return jp_IF; }
implements { KEYWORD; return jp_IMPLEMENTS; }
import { KEYWORD; return jp_IMPORT; }
instanceof { KEYWORD; return jp_INSTANCEOF; }
int { KEYWORD; return jp_INT_TYPE; }
interface { KEYWORD; return jp_INTERFACE; }
long { KEYWORD; return jp_LONG_TYPE; }
native { KEYWORD; return jp_NATIVE; }
new { KEYWORD; return jp_NEW; }
package { KEYWORD; return jp_PACKAGE; }
private { KEYWORD; return jp_PRIVATE; }
protected { KEYWORD; return jp_PROTECTED; }
public { KEYWORD; return jp_PUBLIC; }
return { KEYWORD; return jp_RETURN; }
short { KEYWORD; return jp_SHORT_TYPE; }
static { KEYWORD; return jp_STATIC; }
strictfp { KEYWORD; return jp_STRICTFP; }
super { KEYWORD; return jp_SUPER; }
switch { KEYWORD; return jp_SWITCH; }
synchronized { KEYWORD; return jp_SYNCHRONIZED; }
this { KEYWORD; return jp_THIS; }
throw { KEYWORD; return jp_THROW; }
throws { KEYWORD; return jp_THROWS; }
transient { KEYWORD; return jp_TRANSIENT; }
try { KEYWORD; return jp_TRY; }
void { KEYWORD; return jp_VOID; }
volatile { KEYWORD; return jp_VOLATILE; }
while { KEYWORD; return jp_WHILE; }

(true|false) { PRIMITIVE; return jp_BOOLEANLITERAL; }
\'([^\\]|\\.|\\u[0-9a-fA-F]*|\\[0-7]*)\' { PRIMITIVE; return jp_CHARACTERLITERAL; }
(0|[0-9]+)[lL]? { PRIMITIVE; return jp_DECIMALINTEGERLITERAL; }
([0-9]+\.[0-9]*|\.[0-9]+|[0-9]+)([eE][+\-]?[0-9]+)?[fFdD]? { PRIMITIVE; return jp_FLOATINGPOINTLITERAL; }
0[xX][0-9a-fA-F]+[lL]? { PRIMITIVE; return jp_HEXINTEGERLITERAL; }
null { PRIMITIVE; return jp_NULLLITERAL; }

"&" { SYMBOL; return jp_AND; }
"&&" { SYMBOL; return jp_ANDAND; }
"&=" { SYMBOL; return jp_ANDEQUALS; }
"\]" { SYMBOL; return jp_BRACKETEND; }
"\[" { SYMBOL; return jp_BRACKETSTART; }
"\^" { SYMBOL; return jp_CARROT; }
"\^=" { SYMBOL; return jp_CARROTEQUALS; }
":" { SYMBOL; return jp_COLON; }
"," { SYMBOL; return jp_COMMA; }
"}" { SYMBOL; return jp_CURLYEND; }
"{" { SYMBOL; return jp_CURLYSTART; }
"/" { SYMBOL; return jp_DIVIDE; }
"/=" { SYMBOL; return jp_DIVIDEEQUALS; }
"\$" { SYMBOL; return jp_DOLLAR; }
"\." { SYMBOL; return jp_DOT; }
"=" { SYMBOL; return jp_EQUALS; }
"==" { SYMBOL; return jp_EQUALSEQUALS; }
"\!" { SYMBOL; return jp_EXCLAMATION; }
"\!=" { SYMBOL; return jp_EXCLAMATIONEQUALS; }
">" { SYMBOL; return jp_GREATER; }
">=" { SYMBOL; return jp_GTEQUALS; }
">>" { SYMBOL; return jp_GTGT; }
">>=" { SYMBOL; return jp_GTGTEQUALS; }
">>>" { SYMBOL; return jp_GTGTGT; }
">>>=" { SYMBOL; return jp_GTGTGTEQUALS; }
"<<=" { SYMBOL; return jp_LESLESEQUALS; }
"<" { SYMBOL; return jp_LESSTHAN; }
"<=" { SYMBOL; return jp_LTEQUALS; }
"<<" { SYMBOL; return jp_LTLT; }
"-" { SYMBOL; return jp_MINUS; }
"-=" { SYMBOL; return jp_MINUSEQUALS; }
"--" { SYMBOL; return jp_MINUSMINUS; }
"\)" { SYMBOL; return jp_PAREEND; }
"\(" { SYMBOL; return jp_PARESTART; }
"%" { SYMBOL; return jp_PERCENT; }
"%=" { SYMBOL; return jp_PERCENTEQUALS; }
"\|" { SYMBOL; return jp_PIPE; }
"\|=" { SYMBOL; return jp_PIPEEQUALS; }
"\|\|" { SYMBOL; return jp_PIPEPIPE; }
"\+" { SYMBOL; return jp_PLUS; }
"\+=" { SYMBOL; return jp_PLUSEQUALS; }
"\+\+" { SYMBOL; return jp_PLUSPLUS; }
"\?" { SYMBOL; return jp_QUESTION; }
";" { SYMBOL; return jp_SEMICOL; }
"\~" { SYMBOL; return jp_TILDE; }
"\*" { SYMBOL; return jp_TIMES; }
"\*=" { SYMBOL; return jp_TIMESEQUALS; }

[a-z_A-Z][a-z_0-9A-Z]* {
  yyextra->AllocateParserType(yylvalp, yytext, strlen(yytext));
  return jp_NAME;
}

\/\/.*\n { }
[ \f\t\n\r] { }
. {
  std::cerr << "Unknown character: " << yytext[0]
    << " (" << (int)yytext[0] << ")" << std::endl;
  yyextra->Error("Unknown character");
  return jp_ERROR;
}

%%
