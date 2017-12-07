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
#define yyparse    cmExpr_yyparse
#endif /* yyparse */

#ifndef yylex
#define yylex      cmExpr_yylex
#endif /* yylex */

#ifndef yyerror
#define yyerror    cmExpr_yyerror
#endif /* yyerror */

#ifndef yychar
#define yychar     cmExpr_yychar
#endif /* yychar */

#ifndef yyval
#define yyval      cmExpr_yyval
#endif /* yyval */

#ifndef yylval
#define yylval     cmExpr_yylval
#endif /* yylval */

#ifndef yydebug
#define yydebug    cmExpr_yydebug
#endif /* yydebug */

#ifndef yynerrs
#define yynerrs    cmExpr_yynerrs
#endif /* yynerrs */

#ifndef yyerrflag
#define yyerrflag  cmExpr_yyerrflag
#endif /* yyerrflag */

#ifndef yylhs
#define yylhs      cmExpr_yylhs
#endif /* yylhs */

#ifndef yylen
#define yylen      cmExpr_yylen
#endif /* yylen */

#ifndef yydefred
#define yydefred   cmExpr_yydefred
#endif /* yydefred */

#ifndef yydgoto
#define yydgoto    cmExpr_yydgoto
#endif /* yydgoto */

#ifndef yysindex
#define yysindex   cmExpr_yysindex
#endif /* yysindex */

#ifndef yyrindex
#define yyrindex   cmExpr_yyrindex
#endif /* yyrindex */

#ifndef yygindex
#define yygindex   cmExpr_yygindex
#endif /* yygindex */

#ifndef yytable
#define yytable    cmExpr_yytable
#endif /* yytable */

#ifndef yycheck
#define yycheck    cmExpr_yycheck
#endif /* yycheck */

#ifndef yyname
#define yyname     cmExpr_yyname
#endif /* yyname */

#ifndef yyrule
#define yyrule     cmExpr_yyrule
#endif /* yyrule */
#define YYPREFIX "cmExpr_yy"

#define YYPURE 1

#line 2 "cmExprParser.y"
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C and modified to build everywhere.

Run bison like this:

  bison --yacc --name-prefix=cmExpr_yy --defines=cmExprParserTokens.h -ocmExprParser.cxx cmExprParser.y

Modify cmExprParser.cxx:
  - "#if 0" out yyerrorlab block in range ["goto yyerrlab1", "yyerrlab1:"]

*/

#include "cmConfigure.h" /* IWYU pragma: keep*/

#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*/
#define YYDEBUG 1
#include "cmExprParserHelper.h" /* Interface to parser object.  */
#include "cmExprLexer.h"  /* Interface to lexer object.  */
#include "cmExprParserTokens.h" /* Need YYSTYPE for YY_DECL.  */

/* Forward declare the lexer entry point.  */
YY_DECL;

/* Helper function to forward error callback from parser.  */
static void cmExpr_yyerror(yyscan_t yyscanner, const char* message);

/* Disable some warnings in the generated code.  */
#ifdef _MSC_VER
# pragma warning (disable: 4102) /* Unused goto label.  */
# pragma warning (disable: 4065) /* Switch statement contains default but no case. */
#endif
#line 139 "cmExprParser.cxx"

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

#define exp_PLUS 257
#define exp_MINUS 258
#define exp_TIMES 259
#define exp_DIVIDE 260
#define exp_MOD 261
#define exp_SHIFTLEFT 262
#define exp_SHIFTRIGHT 263
#define exp_OPENPARENT 264
#define exp_CLOSEPARENT 265
#define exp_OR 266
#define exp_AND 267
#define exp_XOR 268
#define exp_NOT 269
#define exp_NUMBER 270
#define YYERRCODE 256
typedef short YYINT;
static const YYINT cmExpr_yylhs[] = {                    -1,
    0,    1,    1,    2,    2,    3,    3,    4,    4,    4,
    5,    5,    5,    6,    6,    6,    6,    7,    7,    7,
    8,    8,
};
static const YYINT cmExpr_yylen[] = {                     2,
    1,    1,    3,    1,    3,    1,    3,    1,    3,    3,
    1,    3,    3,    1,    3,    3,    3,    1,    2,    2,
    1,    3,
};
static const YYINT cmExpr_yydefred[] = {                  0,
    0,    0,    0,   21,    0,    0,    0,    0,    0,    0,
    0,   14,   18,   19,   20,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   22,    0,    0,    0,
    0,    0,    0,    0,   15,   16,   17,
};
static const YYINT cmExpr_yydgoto[] = {                   5,
    6,    7,    8,    9,   10,   11,   12,   13,
};
static const YYINT cmExpr_yysindex[] = {               -252,
 -252, -252, -252,    0,    0, -266, -257, -265, -248, -241,
 -251,    0,    0,    0,    0, -245, -252, -252, -252, -252,
 -252, -252, -252, -252, -252, -252,    0, -257, -265, -248,
 -241, -241, -251, -251,    0,    0,    0,
};
static const YYINT cmExpr_yyrindex[] = {                  0,
    0,    0,    0,    0,    0,   22,    7,   58,   50,   32,
    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   19,   62,   54,
   39,   46,   13,   25,    0,    0,    0,
};
static const YYINT cmExpr_yygindex[] = {                  0,
   26,   16,   17,   15,    3,    8,    2,    0,
};
#define YYTABLESIZE 330
static const YYINT cmExpr_yytable[] = {                  17,
   11,   19,   14,   15,    1,    2,    2,   24,   25,   26,
   18,    3,   12,   20,   21,   22,   23,    4,    3,   27,
   17,    1,   31,   32,   13,   35,   36,   37,   16,   33,
   34,    8,   28,   30,   29,    0,    0,    0,    9,    0,
    0,    0,    0,    0,    0,   10,    0,    0,    0,    6,
    0,    0,    0,    7,    0,    0,    0,    4,    0,    0,
    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,
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
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   11,   11,    0,
    0,    0,   11,   11,    0,   11,   11,   11,   11,   12,
   12,    2,    2,    0,   12,   12,    0,   12,   12,   12,
   12,   13,   13,    3,    3,    0,   13,   13,    0,   13,
   13,   13,   13,    8,    8,    0,    8,    8,    8,    8,
    9,    9,    0,    9,    9,    9,    9,   10,   10,    0,
   10,   10,   10,   10,    6,    6,    6,    6,    7,    7,
    7,    7,    4,    4,    0,    4,    5,    5,    0,    5,
};
static const YYINT cmExpr_yycheck[] = {                 266,
    0,  267,    1,    2,  257,  258,    0,  259,  260,  261,
  268,  264,    0,  262,  263,  257,  258,  270,    0,  265,
  266,    0,   20,   21,    0,   24,   25,   26,    3,   22,
   23,    0,   17,   19,   18,   -1,   -1,   -1,    0,   -1,
   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,    0,
   -1,   -1,   -1,    0,   -1,   -1,   -1,    0,   -1,   -1,
   -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  257,  258,   -1,
   -1,   -1,  262,  263,   -1,  265,  266,  267,  268,  257,
  258,  265,  266,   -1,  262,  263,   -1,  265,  266,  267,
  268,  257,  258,  265,  266,   -1,  262,  263,   -1,  265,
  266,  267,  268,  262,  263,   -1,  265,  266,  267,  268,
  262,  263,   -1,  265,  266,  267,  268,  262,  263,   -1,
  265,  266,  267,  268,  265,  266,  267,  268,  265,  266,
  267,  268,  265,  266,   -1,  268,  265,  266,   -1,  268,
};
#define YYFINAL 5
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 270
#define YYUNDFTOKEN 281
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const cmExpr_yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"exp_PLUS","exp_MINUS",
"exp_TIMES","exp_DIVIDE","exp_MOD","exp_SHIFTLEFT","exp_SHIFTRIGHT",
"exp_OPENPARENT","exp_CLOSEPARENT","exp_OR","exp_AND","exp_XOR","exp_NOT",
"exp_NUMBER",0,0,0,0,0,0,0,0,0,0,"illegal-symbol",
};
static const char *const cmExpr_yyrule[] = {
"$accept : start",
"start : exp",
"exp : bitwiseor",
"exp : exp exp_OR bitwiseor",
"bitwiseor : bitwisexor",
"bitwiseor : bitwiseor exp_XOR bitwisexor",
"bitwisexor : bitwiseand",
"bitwisexor : bitwisexor exp_AND bitwiseand",
"bitwiseand : shift",
"bitwiseand : bitwiseand exp_SHIFTLEFT shift",
"bitwiseand : bitwiseand exp_SHIFTRIGHT shift",
"shift : term",
"shift : shift exp_PLUS term",
"shift : shift exp_MINUS term",
"term : unary",
"term : term exp_TIMES unary",
"term : term exp_DIVIDE unary",
"term : term exp_MOD unary",
"unary : factor",
"unary : exp_PLUS unary",
"unary : exp_MINUS unary",
"factor : exp_NUMBER",
"factor : exp_OPENPARENT exp exp_CLOSEPARENT",

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
#line 158 "cmExprParser.y"
/* End of grammar */

/*--------------------------------------------------------------------------*/
void cmExpr_yyerror(yyscan_t yyscanner, const char* message)
{
  cmExpr_yyget_extra(yyscanner)->Error(message);
}
#line 380 "cmExprParser.cxx"

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
case 1:
#line 73 "cmExprParser.y"
	{
    cmExpr_yyget_extra(yyscanner)->SetResult(yystack.l_mark[0].Number);
  }
break;
case 2:
#line 78 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 3:
#line 81 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number | yystack.l_mark[0].Number;
  }
break;
case 4:
#line 86 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 5:
#line 89 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number ^ yystack.l_mark[0].Number;
  }
break;
case 6:
#line 94 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 7:
#line 97 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number & yystack.l_mark[0].Number;
  }
break;
case 8:
#line 102 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 9:
#line 105 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number << yystack.l_mark[0].Number;
  }
break;
case 10:
#line 108 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number >> yystack.l_mark[0].Number;
  }
break;
case 11:
#line 113 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 12:
#line 116 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number + yystack.l_mark[0].Number;
  }
break;
case 13:
#line 119 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number - yystack.l_mark[0].Number;
  }
break;
case 14:
#line 124 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 15:
#line 127 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number * yystack.l_mark[0].Number;
  }
break;
case 16:
#line 130 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number / yystack.l_mark[0].Number;
  }
break;
case 17:
#line 133 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-2].Number % yystack.l_mark[0].Number;
  }
break;
case 18:
#line 138 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 19:
#line 141 "cmExprParser.y"
	{
    yyval.Number = + yystack.l_mark[0].Number;
  }
break;
case 20:
#line 144 "cmExprParser.y"
	{
    yyval.Number = - yystack.l_mark[0].Number;
  }
break;
case 21:
#line 149 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[0].Number;
  }
break;
case 22:
#line 152 "cmExprParser.y"
	{
    yyval.Number = yystack.l_mark[-1].Number;
  }
break;
#line 721 "cmExprParser.cxx"
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
