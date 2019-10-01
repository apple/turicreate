/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20170709

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0

#ifndef yyparse
#define yyparse    cmFortran_yyparse
#endif /* yyparse */

#ifndef yylex
#define yylex      cmFortran_yylex
#endif /* yylex */

#ifndef yyerror
#define yyerror    cmFortran_yyerror
#endif /* yyerror */

#ifndef yychar
#define yychar     cmFortran_yychar
#endif /* yychar */

#ifndef yyval
#define yyval      cmFortran_yyval
#endif /* yyval */

#ifndef yylval
#define yylval     cmFortran_yylval
#endif /* yylval */

#ifndef yydebug
#define yydebug    cmFortran_yydebug
#endif /* yydebug */

#ifndef yynerrs
#define yynerrs    cmFortran_yynerrs
#endif /* yynerrs */

#ifndef yyerrflag
#define yyerrflag  cmFortran_yyerrflag
#endif /* yyerrflag */

#ifndef yylhs
#define yylhs      cmFortran_yylhs
#endif /* yylhs */

#ifndef yylen
#define yylen      cmFortran_yylen
#endif /* yylen */

#ifndef yydefred
#define yydefred   cmFortran_yydefred
#endif /* yydefred */

#ifndef yydgoto
#define yydgoto    cmFortran_yydgoto
#endif /* yydgoto */

#ifndef yysindex
#define yysindex   cmFortran_yysindex
#endif /* yysindex */

#ifndef yyrindex
#define yyrindex   cmFortran_yyrindex
#endif /* yyrindex */

#ifndef yygindex
#define yygindex   cmFortran_yygindex
#endif /* yygindex */

#ifndef yytable
#define yytable    cmFortran_yytable
#endif /* yytable */

#ifndef yycheck
#define yycheck    cmFortran_yycheck
#endif /* yycheck */

#ifndef yyname
#define yyname     cmFortran_yyname
#endif /* yyname */

#ifndef yyrule
#define yyrule     cmFortran_yyrule
#endif /* yyrule */
#define YYPREFIX "cmFortran_yy"

#define YYPURE 1

#line 2 "cmFortranParser.y"
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

This file must be translated to C and modified to build everywhere.

Run bison like this:

  bison --yacc --name-prefix=cmFortran_yy
        --defines=cmFortranParserTokens.h
         -ocmFortranParser.cxx
          cmFortranParser.y

Modify cmFortranParser.cxx:
  - "#if 0" out yyerrorlab block in range ["goto yyerrlab1", "yyerrlab1:"]
*/

#include "cmConfigure.h" /* IWYU pragma: keep*/

#include "cmsys/String.h"
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*/
#define cmFortranParser_cxx
#include "cmFortranParser.h" /* Interface to parser object.  */
#include "cmFortranParserTokens.h" /* Need YYSTYPE for YY_DECL.  */

/* Forward declare the lexer entry point.  */
YY_DECL;

/* Helper function to forward error callback from parser.  */
static void cmFortran_yyerror(yyscan_t yyscanner, const char* message)
{
  cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
  cmFortranParser_Error(parser, message);
}

/* Disable some warnings in the generated code.  */
#ifdef _MSC_VER
# pragma warning (disable: 4102) /* Unused goto label.  */
# pragma warning (disable: 4065) /* Switch contains default but no case. */
# pragma warning (disable: 4701) /* Local variable may not be initialized.  */
# pragma warning (disable: 4702) /* Unreachable code.  */
# pragma warning (disable: 4127) /* Conditional expression is constant.  */
# pragma warning (disable: 4244) /* Conversion to smaller type, data loss. */
#endif
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#line 70 "cmFortranParser.y"
typedef union {
  char* string;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 170 "cmFortranParser.cxx"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(yyscan_t yyscanner)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# ifdef YYLEX_PARAM_TYPE
#  define YYLEX_DECL() yylex(YYSTYPE *yylval, YYLEX_PARAM_TYPE YYLEX_PARAM)
# else
#  define YYLEX_DECL() yylex(YYSTYPE *yylval, void * YYLEX_PARAM)
# endif
# define YYLEX yylex(&yylval, YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(YYSTYPE *yylval, yyscan_t yyscanner)
# define YYLEX yylex(&yylval, yyscanner)
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(yyscan_t yyscanner, const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(yyscanner, msg)
#endif

extern int YYPARSE_DECL();

#define EOSTMT 257
#define ASSIGNMENT_OP 258
#define GARBAGE 259
#define CPP_LINE_DIRECTIVE 260
#define CPP_INCLUDE 261
#define F90PPR_INCLUDE 262
#define COCO_INCLUDE 263
#define F90PPR_DEFINE 264
#define CPP_DEFINE 265
#define F90PPR_UNDEF 266
#define CPP_UNDEF 267
#define CPP_IFDEF 268
#define CPP_IFNDEF 269
#define CPP_IF 270
#define CPP_ELSE 271
#define CPP_ELIF 272
#define CPP_ENDIF 273
#define F90PPR_IFDEF 274
#define F90PPR_IFNDEF 275
#define F90PPR_IF 276
#define F90PPR_ELSE 277
#define F90PPR_ELIF 278
#define F90PPR_ENDIF 279
#define COMMA 280
#define COLON 281
#define DCOLON 282
#define LPAREN 283
#define RPAREN 284
#define UNTERMINATED_STRING 285
#define STRING 286
#define WORD 287
#define CPP_INCLUDE_ANGLE 288
#define END 289
#define INCLUDE 290
#define INTERFACE 291
#define MODULE 292
#define SUBMODULE 293
#define USE 294
#define YYERRCODE 256
typedef short YYINT;
static const YYINT cmFortran_yylhs[] = {                 -1,
    0,    0,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    3,    3,    3,    4,    4,
    5,    5,    6,    6,    7,    7,    8,    8,    9,    9,
   10,   10,   11,   11,    2,    2,   12,   12,   12,   12,
   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
   12,   12,
};
static const YYINT cmFortran_yylen[] = {                  2,
    0,    2,    2,    4,    4,    7,    9,    4,    4,    5,
    7,    4,    4,    3,    4,    4,    4,    4,    4,    3,
    3,    3,    3,    1,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    0,    2,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,
};
static const YYINT cmFortran_yydefred[] = {               1,
    0,    0,   24,    0,   26,   27,   28,   30,   29,   32,
   31,   33,   35,   37,   41,   39,   43,   34,   36,   38,
   42,   40,   44,   45,    0,    0,    0,    0,    0,    0,
    2,    0,    0,    0,    0,    0,   45,   45,   45,   45,
   25,   45,    0,   45,   45,    3,   45,   45,    0,    0,
    0,   45,   45,   45,   45,   45,   45,    0,    0,    0,
    0,    0,   14,   56,   55,   61,   57,   58,   59,   60,
   62,   54,   47,   48,   49,   50,   51,   52,   53,   46,
    0,    0,    0,    0,    0,    0,   45,    0,    0,    0,
    0,    0,    0,   20,   21,   22,   23,   13,    9,   12,
    8,    5,    0,    0,    0,    0,    4,   15,   16,   17,
   18,   19,    0,   45,   45,   10,    0,    0,    0,   45,
    6,   11,    0,    7,
};
static const YYINT cmFortran_yydgoto[] = {                1,
   31,   43,   32,   33,   34,   35,   36,   37,   38,   39,
   40,   80,
};
static const YYINT cmFortran_yysindex[] = {               0,
 -235, -253,    0, -274,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -278, -272, -212, -264, -259, -216,
    0, -240, -239, -238, -236, -227,    0,    0,    0,    0,
    0,    0, -196,    0,    0,    0,    0,    0, -222, -220,
 -219,    0,    0,    0,    0,    0,    0, -178, -158, -140,
 -120, -102,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -82,  -64,  -44,  -26, -275, -230,    0,   -6,   12,   32,
   50,   70,   88,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -218, -217, -215,  108,    0,    0,    0,    0,
    0,    0, -237,    0,    0,    0, -214,  126,  146,    0,
    0,    0,  164,    0,
};
static const YYINT cmFortran_yyrindex[] = {               0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,
};
static const YYINT cmFortran_yygindex[] = {               0,
    0,  -37,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
#define YYTABLESIZE 458
static const YYINT cmFortran_yytable[] = {               58,
   59,   60,   61,   41,   62,  103,   81,   82,  104,   83,
   84,   42,   44,   45,   88,   89,   90,   91,   92,   93,
    2,    3,   48,   49,    4,    5,    6,    7,    8,    9,
   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
   20,   21,   22,   23,   46,   53,  117,   54,   55,  106,
   56,  105,   24,   25,   26,   27,   28,   29,   30,   57,
   63,   64,   65,   50,   85,   51,   86,   87,  113,  114,
   52,  115,  120,    0,   47,    0,  118,  119,   94,   64,
   65,    0,  123,   66,   67,   68,   69,   70,   71,   72,
   73,    0,   74,   75,   76,   77,   78,   79,   95,   64,
   65,   66,   67,   68,   69,   70,   71,   72,   73,    0,
   74,   75,   76,   77,   78,   79,   96,   64,   65,    0,
    0,   66,   67,   68,   69,   70,   71,   72,   73,    0,
   74,   75,   76,   77,   78,   79,   97,   64,   65,   66,
   67,   68,   69,   70,   71,   72,   73,    0,   74,   75,
   76,   77,   78,   79,   98,   64,   65,    0,    0,   66,
   67,   68,   69,   70,   71,   72,   73,    0,   74,   75,
   76,   77,   78,   79,   99,   64,   65,   66,   67,   68,
   69,   70,   71,   72,   73,    0,   74,   75,   76,   77,
   78,   79,  100,   64,   65,    0,    0,   66,   67,   68,
   69,   70,   71,   72,   73,    0,   74,   75,   76,   77,
   78,   79,  101,   64,   65,   66,   67,   68,   69,   70,
   71,   72,   73,    0,   74,   75,   76,   77,   78,   79,
  102,   64,   65,    0,    0,   66,   67,   68,   69,   70,
   71,   72,   73,    0,   74,   75,   76,   77,   78,   79,
  107,   64,   65,   66,   67,   68,   69,   70,   71,   72,
   73,    0,   74,   75,   76,   77,   78,   79,  108,   64,
   65,    0,    0,   66,   67,   68,   69,   70,   71,   72,
   73,    0,   74,   75,   76,   77,   78,   79,  109,   64,
   65,   66,   67,   68,   69,   70,   71,   72,   73,    0,
   74,   75,   76,   77,   78,   79,  110,   64,   65,    0,
    0,   66,   67,   68,   69,   70,   71,   72,   73,    0,
   74,   75,   76,   77,   78,   79,  111,   64,   65,   66,
   67,   68,   69,   70,   71,   72,   73,    0,   74,   75,
   76,   77,   78,   79,  112,   64,   65,    0,    0,   66,
   67,   68,   69,   70,   71,   72,   73,    0,   74,   75,
   76,   77,   78,   79,  116,   64,   65,   66,   67,   68,
   69,   70,   71,   72,   73,    0,   74,   75,   76,   77,
   78,   79,  121,   64,   65,    0,    0,   66,   67,   68,
   69,   70,   71,   72,   73,    0,   74,   75,   76,   77,
   78,   79,  122,   64,   65,   66,   67,   68,   69,   70,
   71,   72,   73,    0,   74,   75,   76,   77,   78,   79,
  124,   64,   65,    0,    0,   66,   67,   68,   69,   70,
   71,   72,   73,    0,   74,   75,   76,   77,   78,   79,
    0,    0,    0,   66,   67,   68,   69,   70,   71,   72,
   73,    0,   74,   75,   76,   77,   78,   79,
};
static const YYINT cmFortran_yycheck[] = {               37,
   38,   39,   40,  257,   42,  281,   44,   45,  284,   47,
   48,  286,  291,  286,   52,   53,   54,   55,   56,   57,
  256,  257,  287,  283,  260,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  277,  278,  279,  257,  286,  284,  287,  287,   87,
  287,  282,  288,  289,  290,  291,  292,  293,  294,  287,
  257,  258,  259,  280,  287,  282,  287,  287,  287,  287,
  287,  287,  287,   -1,  287,   -1,  114,  115,  257,  258,
  259,   -1,  120,  280,  281,  282,  283,  284,  285,  286,
  287,   -1,  289,  290,  291,  292,  293,  294,  257,  258,
  259,  280,  281,  282,  283,  284,  285,  286,  287,   -1,
  289,  290,  291,  292,  293,  294,  257,  258,  259,   -1,
   -1,  280,  281,  282,  283,  284,  285,  286,  287,   -1,
  289,  290,  291,  292,  293,  294,  257,  258,  259,  280,
  281,  282,  283,  284,  285,  286,  287,   -1,  289,  290,
  291,  292,  293,  294,  257,  258,  259,   -1,   -1,  280,
  281,  282,  283,  284,  285,  286,  287,   -1,  289,  290,
  291,  292,  293,  294,  257,  258,  259,  280,  281,  282,
  283,  284,  285,  286,  287,   -1,  289,  290,  291,  292,
  293,  294,  257,  258,  259,   -1,   -1,  280,  281,  282,
  283,  284,  285,  286,  287,   -1,  289,  290,  291,  292,
  293,  294,  257,  258,  259,  280,  281,  282,  283,  284,
  285,  286,  287,   -1,  289,  290,  291,  292,  293,  294,
  257,  258,  259,   -1,   -1,  280,  281,  282,  283,  284,
  285,  286,  287,   -1,  289,  290,  291,  292,  293,  294,
  257,  258,  259,  280,  281,  282,  283,  284,  285,  286,
  287,   -1,  289,  290,  291,  292,  293,  294,  257,  258,
  259,   -1,   -1,  280,  281,  282,  283,  284,  285,  286,
  287,   -1,  289,  290,  291,  292,  293,  294,  257,  258,
  259,  280,  281,  282,  283,  284,  285,  286,  287,   -1,
  289,  290,  291,  292,  293,  294,  257,  258,  259,   -1,
   -1,  280,  281,  282,  283,  284,  285,  286,  287,   -1,
  289,  290,  291,  292,  293,  294,  257,  258,  259,  280,
  281,  282,  283,  284,  285,  286,  287,   -1,  289,  290,
  291,  292,  293,  294,  257,  258,  259,   -1,   -1,  280,
  281,  282,  283,  284,  285,  286,  287,   -1,  289,  290,
  291,  292,  293,  294,  257,  258,  259,  280,  281,  282,
  283,  284,  285,  286,  287,   -1,  289,  290,  291,  292,
  293,  294,  257,  258,  259,   -1,   -1,  280,  281,  282,
  283,  284,  285,  286,  287,   -1,  289,  290,  291,  292,
  293,  294,  257,  258,  259,  280,  281,  282,  283,  284,
  285,  286,  287,   -1,  289,  290,  291,  292,  293,  294,
  257,  258,  259,   -1,   -1,  280,  281,  282,  283,  284,
  285,  286,  287,   -1,  289,  290,  291,  292,  293,  294,
   -1,   -1,   -1,  280,  281,  282,  283,  284,  285,  286,
  287,   -1,  289,  290,  291,  292,  293,  294,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 294
#define YYUNDFTOKEN 309
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const cmFortran_yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"EOSTMT","ASSIGNMENT_OP",
"GARBAGE","CPP_LINE_DIRECTIVE","CPP_INCLUDE","F90PPR_INCLUDE","COCO_INCLUDE",
"F90PPR_DEFINE","CPP_DEFINE","F90PPR_UNDEF","CPP_UNDEF","CPP_IFDEF",
"CPP_IFNDEF","CPP_IF","CPP_ELSE","CPP_ELIF","CPP_ENDIF","F90PPR_IFDEF",
"F90PPR_IFNDEF","F90PPR_IF","F90PPR_ELSE","F90PPR_ELIF","F90PPR_ENDIF","COMMA",
"COLON","DCOLON","LPAREN","RPAREN","UNTERMINATED_STRING","STRING","WORD",
"CPP_INCLUDE_ANGLE","END","INCLUDE","INTERFACE","MODULE","SUBMODULE","USE",0,0,
0,0,0,0,0,0,0,0,0,0,0,0,"illegal-symbol",
};
static const char *const cmFortran_yyrule[] = {
"$accept : code",
"code :",
"code : code stmt",
"stmt : INTERFACE EOSTMT",
"stmt : USE WORD other EOSTMT",
"stmt : MODULE WORD other EOSTMT",
"stmt : SUBMODULE LPAREN WORD RPAREN WORD other EOSTMT",
"stmt : SUBMODULE LPAREN WORD COLON WORD RPAREN WORD other EOSTMT",
"stmt : INTERFACE WORD other EOSTMT",
"stmt : END INTERFACE other EOSTMT",
"stmt : USE DCOLON WORD other EOSTMT",
"stmt : USE COMMA WORD DCOLON WORD other EOSTMT",
"stmt : INCLUDE STRING other EOSTMT",
"stmt : CPP_LINE_DIRECTIVE STRING other EOSTMT",
"stmt : CPP_INCLUDE_ANGLE other EOSTMT",
"stmt : include STRING other EOSTMT",
"stmt : define WORD other EOSTMT",
"stmt : undef WORD other EOSTMT",
"stmt : ifdef WORD other EOSTMT",
"stmt : ifndef WORD other EOSTMT",
"stmt : if other EOSTMT",
"stmt : elif other EOSTMT",
"stmt : else other EOSTMT",
"stmt : endif other EOSTMT",
"stmt : EOSTMT",
"stmt : error EOSTMT",
"include : CPP_INCLUDE",
"include : F90PPR_INCLUDE",
"include : COCO_INCLUDE",
"define : CPP_DEFINE",
"define : F90PPR_DEFINE",
"undef : CPP_UNDEF",
"undef : F90PPR_UNDEF",
"ifdef : CPP_IFDEF",
"ifdef : F90PPR_IFDEF",
"ifndef : CPP_IFNDEF",
"ifndef : F90PPR_IFNDEF",
"if : CPP_IF",
"if : F90PPR_IF",
"elif : CPP_ELIF",
"elif : F90PPR_ELIF",
"else : CPP_ELSE",
"else : F90PPR_ELSE",
"endif : CPP_ENDIF",
"endif : F90PPR_ENDIF",
"other :",
"other : other misc_code",
"misc_code : WORD",
"misc_code : END",
"misc_code : INCLUDE",
"misc_code : INTERFACE",
"misc_code : MODULE",
"misc_code : SUBMODULE",
"misc_code : USE",
"misc_code : STRING",
"misc_code : GARBAGE",
"misc_code : ASSIGNMENT_OP",
"misc_code : COLON",
"misc_code : DCOLON",
"misc_code : LPAREN",
"misc_code : RPAREN",
"misc_code : COMMA",
"misc_code : UNTERMINATED_STRING",

};
#endif

int      yydebug;
int      yynerrs;

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#define YYINITSTACKSIZE 200

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
#line 247 "cmFortranParser.y"
/* End of grammar */
#line 536 "cmFortranParser.cxx"

#if YYDEBUG
#include <stdio.h>	/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    YYINT *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

    data->l_base = newvs;
    data->l_mark = newvs + i;

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab

int
YYPARSE_DECL()
{
    int      yyerrflag;
    int      yychar;
    YYSTYPE  yyval;
    YYSTYPE  yylval;

    /* variables for the parser stack */
    YYSTACKDATA yystack;
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    memset(&yyval,  0, sizeof(yyval));
    memset(&yylval, 0, sizeof(yylval));

    yym = 0;
    yyn = 0;
    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        yychar = YYLEX;
        if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if (((yyn = yysindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if (((yyn = yyrindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag != 0) goto yyinrecovery;

    YYERROR_CALL("syntax error");

    goto yyerrlab; /* redundant goto avoids 'unused label' warning */
yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if (((yyn = yysindex[*yystack.s_mark]) != 0) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
                --yystack.s_mark;
                --yystack.l_mark;
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym > 0)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);

    switch (yyn)
    {
case 3:
#line 101 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_SetInInterface(parser, true);
  }
break;
case 4:
#line 105 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleUse(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 5:
#line 110 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    if (cmsysString_strcasecmp(yystack.l_mark[-2].string, "function") != 0 &&
        cmsysString_strcasecmp(yystack.l_mark[-2].string, "procedure") != 0 &&
        cmsysString_strcasecmp(yystack.l_mark[-2].string, "subroutine") != 0) {
      cmFortranParser_RuleModule(parser, yystack.l_mark[-2].string);
    }
    free(yystack.l_mark[-2].string);
  }
break;
case 6:
#line 119 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleUse(parser, yystack.l_mark[-4].string);
    free(yystack.l_mark[-4].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 7:
#line 125 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleUse(parser, yystack.l_mark[-6].string);
    free(yystack.l_mark[-6].string);
    free(yystack.l_mark[-4].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 8:
#line 132 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_SetInInterface(parser, true);
    free(yystack.l_mark[-2].string);
  }
break;
case 9:
#line 137 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_SetInInterface(parser, false);
  }
break;
case 10:
#line 141 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleUse(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 11:
#line 146 "cmFortranParser.y"
	{
    if (cmsysString_strcasecmp(yystack.l_mark[-4].string, "non_intrinsic") == 0) {
      cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
      cmFortranParser_RuleUse(parser, yystack.l_mark[-2].string);
    }
    free(yystack.l_mark[-4].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 12:
#line 154 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleInclude(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 13:
#line 159 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleLineDirective(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 14:
#line 164 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleInclude(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 15:
#line 169 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleInclude(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 16:
#line 174 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleDefine(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 17:
#line 179 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleUndef(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 18:
#line 184 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleIfdef(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 19:
#line 189 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleIfndef(parser, yystack.l_mark[-2].string);
    free(yystack.l_mark[-2].string);
  }
break;
case 20:
#line 194 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleIf(parser);
  }
break;
case 21:
#line 198 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleElif(parser);
  }
break;
case 22:
#line 202 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleElse(parser);
  }
break;
case 23:
#line 206 "cmFortranParser.y"
	{
    cmFortranParser* parser = cmFortran_yyget_extra(yyscanner);
    cmFortranParser_RuleEndif(parser);
  }
break;
case 47:
#line 228 "cmFortranParser.y"
	{ free (yystack.l_mark[0].string); }
break;
case 54:
#line 235 "cmFortranParser.y"
	{ free (yystack.l_mark[0].string); }
break;
#line 925 "cmFortranParser.cxx"
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
        if (yychar < 0)
        {
            yychar = YYLEX;
            if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if (((yyn = yygindex[yym]) != 0) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    YYERROR_CALL("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
