%{
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*-------------------------------------------------------------------------
  Portions of this source have been derived from makedepf90 version 2.8.8,

   Copyright (C) 2000--2006 Erik Edelmann <erik.edelmann@iki.fi>

  The code was originally distributed under the GPL but permission
  from the copyright holder has been obtained to distribute this
  derived work under the CMake license.
-------------------------------------------------------------------------*/

/*

This file must be translated to C++ and modified to build everywhere.

Run flex >= 2.6 like this:

  flex -i --nounistd -DFLEXINT_H --prefix=cmFortran_yy --header-file=cmFortranLexer.h -ocmFortranLexer.cxx cmFortranLexer.in.l

Modify cmFortranLexer.cxx:
  - remove trailing whitespace: sed -i 's/\s*$//' cmFortranLexer.h cmFortranLexer.cxx
  - remove blank lines at end of file
  - #include "cmStandardLexer.h" at the top
  - add cast in yy_scan_bytes for loop condition of _yybytes_len to size_t

*/

/* IWYU pragma: no_forward_declare yyguts_t */

#undef YY_NO_UNPUT

#define cmFortranLexer_cxx
#include "cmFortranParser.h" /* Interface to parser object.  */

/* Replace the lexer input function.  */
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) \
  { result = cmFortranParser_Input(yyextra, buf, max_size); }

/* Include the set of tokens from the parser.  */
#include "cmFortranParserTokens.h"

/*--------------------------------------------------------------------------*/
%}


%option reentrant
%option noyywrap
%pointer

%s free_fmt fixed_fmt
%x str_sq str_dq

%%

\"              {
  cmFortranParser_StringStart(yyextra);
  cmFortranParser_SetOldStartcond(yyextra, YY_START);
  BEGIN(str_dq);
}

'               {
  cmFortranParser_StringStart(yyextra);
  cmFortranParser_SetOldStartcond(yyextra, YY_START);
  BEGIN(str_sq);
}

<str_dq>\" |
<str_sq>'  {
  BEGIN(cmFortranParser_GetOldStartcond(yyextra) );
  yylvalp->string = strdup(cmFortranParser_StringEnd(yyextra));
  return STRING;
}

<str_dq,str_sq>&[ \t]*\n |
<str_dq,str_sq>&[ \t]*\n[ \t]*&  /* Ignore (continued strings, free fmt) */

<fixed_fmt,str_dq,str_sq>\n[ ]{5}[^ \t\n] {
  if (cmFortranParser_GetOldStartcond(yyextra) == fixed_fmt)
    ; /* Ignore (cont. strings, fixed fmt) */
  else
    {
    unput(yytext[strlen(yytext)-1]);
    }
}


<str_dq,str_sq>\n {
  unput ('\n');
  BEGIN(INITIAL);
  return UNTERMINATED_STRING;
}

<str_sq,str_dq>. {
  cmFortranParser_StringAppend(yyextra, yytext[0]);
}

!.*\n                   { return EOSTMT; } /* Treat comments like */
<fixed_fmt>^[cC*dD].*\n { return EOSTMT; } /* empty lines */

^[ \t]*#([ \t]*line)?[ \t]*[0-9]+[ \t]* { return CPP_LINE_DIRECTIVE; }
^[ \t]*#[ \t]*include[ \t]*<[^>]+> {
  yytext[yyleng-1] = 0;
  yylvalp->string = strdup(strchr(yytext, '<')+1);
  return CPP_INCLUDE_ANGLE;
}
^[ \t]*#[ \t]*include  { return CPP_INCLUDE; }
\$[ \t]*include { return F90PPR_INCLUDE; }
\?\?[ \t]*include { return COCO_INCLUDE; }

^[ \t]*#[ \t]*define   { return CPP_DEFINE; }
\$[ \t]*DEFINE   { return F90PPR_DEFINE; }

^[ \t]*#[ \t]*undef    { return CPP_UNDEF; }
\$[ \t]*UNDEF   { return F90PPR_UNDEF; }

^[ \t]*#[ \t]*ifdef    { return CPP_IFDEF; }
^[ \t]*#[ \t]*ifndef   { return CPP_IFNDEF; }
^[ \t]*#[ \t]*if       { return CPP_IF; }
^[ \t]*#[ \t]*elif     { return CPP_ELIF; }
^[ \t]*#[ \t]*else     { return CPP_ELSE; }
^[ \t]*#[ \t]*endif    { return CPP_ENDIF; }

$[ \t]*ifdef    { return F90PPR_IFDEF; }
$[ \t]*ifndef   { return F90PPR_IFNDEF; }
$[ \t]*if       { return F90PPR_IF; }
$[ \t]*elif     { return F90PPR_ELIF; }
$[ \t]*else     { return F90PPR_ELSE; }
$[ \t]*endif    { return F90PPR_ENDIF; }

 /* Line continuations, possible involving comments.  */
&([ \t\n]*|!.*)*
&([ \t\n]*|!.*)*&

, { return COMMA; }

:: { return DCOLON; }
: { return COLON; }

<fixed_fmt>\n[ ]{5}[^ ]  { return GARBAGE; }

=|=>                     { return ASSIGNMENT_OP; }

[Ee][Nn][Dd] { return END; }
[Ii][Nn][Cc][Ll][Uu][Dd][Ee] { return INCLUDE; }
[Ii][Nn][Tt][Ee][Rr][Ff][Aa][Cc][Ee] { return INTERFACE; }
[Mm][Oo][Dd][Uu][Ll][Ee] { return MODULE; }
[Ss][Uu][bb][Mm][Oo][Dd][Uu][Ll][Ee] { return SUBMODULE; }
[Uu][Ss][Ee] { return USE; }

[a-zA-Z_][a-zA-Z_0-9]* {
  yylvalp->string = strdup(yytext);
  return WORD;
}

\( { return LPAREN; }
\) { return RPAREN; }

[^ \t\n\r:;,!'"a-zA-Z=&()]+ { return GARBAGE; }

;|\n { return EOSTMT; }


[ \t\r,]         /* Ignore */
\\[ \t]*\n       /* Ignore line-endings preceded by \ */

. { return *yytext; }

<<EOF>> {
  if(!cmFortranParser_FilePop(yyextra) )
    {
    return YY_NULL;
    }
}

%%

/*--------------------------------------------------------------------------*/
YY_BUFFER_STATE cmFortranLexer_GetCurrentBuffer(yyscan_t yyscanner)
{
  /* Hack into the internal flex-generated scanner to get the buffer.  */
  struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
  return YY_CURRENT_BUFFER;
}
