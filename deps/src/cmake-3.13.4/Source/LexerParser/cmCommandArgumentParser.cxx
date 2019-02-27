/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20180609

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0

#ifndef yyparse
#define yyparse    cmCommandArgument_yyparse
#endif /* yyparse */

#ifndef yylex
#define yylex      cmCommandArgument_yylex
#endif /* yylex */

#ifndef yyerror
#define yyerror    cmCommandArgument_yyerror
#endif /* yyerror */

#ifndef yychar
#define yychar     cmCommandArgument_yychar
#endif /* yychar */

#ifndef yyval
#define yyval      cmCommandArgument_yyval
#endif /* yyval */

#ifndef yylval
#define yylval     cmCommandArgument_yylval
#endif /* yylval */

#ifndef yydebug
#define yydebug    cmCommandArgument_yydebug
#endif /* yydebug */

#ifndef yynerrs
#define yynerrs    cmCommandArgument_yynerrs
#endif /* yynerrs */

#ifndef yyerrflag
#define yyerrflag  cmCommandArgument_yyerrflag
#endif /* yyerrflag */

#ifndef yylhs
#define yylhs      cmCommandArgument_yylhs
#endif /* yylhs */

#ifndef yylen
#define yylen      cmCommandArgument_yylen
#endif /* yylen */

#ifndef yydefred
#define yydefred   cmCommandArgument_yydefred
#endif /* yydefred */

#ifndef yydgoto
#define yydgoto    cmCommandArgument_yydgoto
#endif /* yydgoto */

#ifndef yysindex
#define yysindex   cmCommandArgument_yysindex
#endif /* yysindex */

#ifndef yyrindex
#define yyrindex   cmCommandArgument_yyrindex
#endif /* yyrindex */

#ifndef yygindex
#define yygindex   cmCommandArgument_yygindex
#endif /* yygindex */

#ifndef yytable
#define yytable    cmCommandArgument_yytable
#endif /* yytable */

#ifndef yycheck
#define yycheck    cmCommandArgument_yycheck
#endif /* yycheck */

#ifndef yyname
#define yyname     cmCommandArgument_yyname
#endif /* yyname */

#ifndef yyrule
#define yyrule     cmCommandArgument_yyrule
#endif /* yyrule */
#define YYPREFIX "cmCommandArgument_yy"

#define YYPURE 1

#line 2 "cmCommandArgumentParser.y"
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C and modified to build everywhere.

Run bison like this:

  bison --yacc --name-prefix=cmCommandArgument_yy --defines=cmCommandArgumentParserTokens.h -ocmCommandArgumentParser.cxx cmCommandArgumentParser.y

Modify cmCommandArgumentParser.cxx:
  - "#if 0" out yyerrorlab block in range ["goto yyerrlab1", "yyerrlab1:"]

*/

#include "cmConfigure.h" /* IWYU pragma: keep*/

#include <string.h>

#define yyGetParser (cmCommandArgument_yyget_extra(yyscanner))

/* Make sure malloc and free are available on QNX.  */
#ifdef __QNX__
# include <malloc.h>
#endif

/* Make sure the parser uses standard memory allocation.  The default
   generated parser malloc/free declarations do not work on all
   platforms.  */
#include <stdlib.h>
#define YYMALLOC malloc
#define YYFREE free

/*-------------------------------------------------------------------------*/
#include "cmCommandArgumentParserHelper.h" /* Interface to parser object.  */
#include "cmCommandArgumentLexer.h"  /* Interface to lexer object.  */
#include "cmCommandArgumentParserTokens.h" /* Need YYSTYPE for YY_DECL.  */

/* Forward declare the lexer entry point.  */
YY_DECL;

/* Helper function to forward error callback from parser.  */
static void cmCommandArgument_yyerror(yyscan_t yyscanner, const char* message);

/* Configure the parser to support large input.  */
#define YYMAXDEPTH 100000
#define YYINITDEPTH 10000

/* Disable some warnings in the generated code.  */
#ifdef _MSC_VER
# pragma warning (disable: 4102) /* Unused goto label.  */
# pragma warning (disable: 4065) /* Switch statement contains default but no
                                    case. */
# pragma warning (disable: 4244) /* loss of precision */
# pragma warning (disable: 4702) /* unreachable code */
#endif
#if defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic ignored "-Wconversion"
#endif
#line 161 "cmCommandArgumentParser.cxx"

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

#define cal_ENVCURLY 257
#define cal_NCURLY 258
#define cal_DCURLY 259
#define cal_DOLLAR 260
#define cal_LCURLY 261
#define cal_RCURLY 262
#define cal_NAME 263
#define cal_BSLASH 264
#define cal_SYMBOL 265
#define cal_AT 266
#define cal_ERROR 267
#define cal_ATNAME 268
#define YYERRCODE 256
typedef short YYINT;
static const YYINT cmCommandArgument_yylhs[] = {         -1,
    0,    1,    1,    2,    2,    3,    3,    4,    4,    4,
    4,    4,    4,    5,    5,    5,    5,    6,    6,    7,
    7,    8,    8,
};
static const YYINT cmCommandArgument_yylen[] = {          2,
    1,    1,    2,    0,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    3,    3,    3,    1,    1,    2,    0,
    2,    1,    1,
};
static const YYINT cmCommandArgument_yydefred[] = {       0,
    0,    0,    0,   10,   11,   12,    8,   13,    9,   17,
    0,    1,    0,    0,    6,    7,   22,    0,   23,    0,
   18,    0,    0,    0,    3,    5,   19,   14,   21,   15,
   16,
};
static const YYINT cmCommandArgument_yydgoto[] = {       11,
   12,   13,   14,   15,   19,   20,   21,   22,
};
static const YYINT cmCommandArgument_yysindex[] = {    -250,
 -238, -226, -226,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -264, -250,    0,    0,    0, -238,    0, -260,
    0, -226, -256, -248,    0,    0,    0,    0,    0,    0,
    0,
};
static const YYINT cmCommandArgument_yyrindex[] = {       1,
 -240, -240, -240,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   23,    1,    0,    0,    0, -240,    0,    0,
    0, -240,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
static const YYINT cmCommandArgument_yygindex[] = {       0,
    0,   12,    0,    0,    3,   10,    2,    0,
};
#define YYTABLESIZE 265
static const YYINT cmCommandArgument_yytable[] = {       25,
    4,   28,   16,   23,   24,   30,    1,    2,    3,    4,
    5,    6,    7,   31,    8,    9,   16,   10,    1,    2,
    3,   20,    2,   29,   17,   26,   18,   27,    0,   10,
    1,    2,    3,    0,    0,    0,   17,    0,    0,    0,
    0,   10,    0,    0,    0,    0,    0,    0,    0,    0,
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
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    4,
};
static const YYINT cmCommandArgument_yycheck[] = {      264,
    0,  262,    0,    2,    3,  262,  257,  258,  259,  260,
  261,  262,  263,  262,  265,  266,   14,  268,  257,  258,
  259,  262,    0,   22,  263,   14,  265,   18,   -1,  268,
  257,  258,  259,   -1,   -1,   -1,  263,   -1,   -1,   -1,
   -1,  268,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
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
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  264,
};
#define YYFINAL 11
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 268
#define YYUNDFTOKEN 279
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const cmCommandArgument_yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,"'$'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'@'",0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'\\\\'",0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"cal_ENVCURLY","cal_NCURLY","cal_DCURLY","cal_DOLLAR","cal_LCURLY","cal_RCURLY",
"cal_NAME","cal_BSLASH","cal_SYMBOL","cal_AT","cal_ERROR","cal_ATNAME",0,0,0,0,
0,0,0,0,0,0,"illegal-symbol",
};
static const char *const cmCommandArgument_yyrule[] = {
"$accept : Start",
"Start : GoalWithOptionalBackSlash",
"GoalWithOptionalBackSlash : Goal",
"GoalWithOptionalBackSlash : Goal cal_BSLASH",
"Goal :",
"Goal : String Goal",
"String : OuterText",
"String : Variable",
"OuterText : cal_NAME",
"OuterText : cal_AT",
"OuterText : cal_DOLLAR",
"OuterText : cal_LCURLY",
"OuterText : cal_RCURLY",
"OuterText : cal_SYMBOL",
"Variable : cal_ENVCURLY EnvVarName cal_RCURLY",
"Variable : cal_NCURLY MultipleIds cal_RCURLY",
"Variable : cal_DCURLY MultipleIds cal_RCURLY",
"Variable : cal_ATNAME",
"EnvVarName : MultipleIds",
"EnvVarName : cal_SYMBOL EnvVarName",
"MultipleIds :",
"MultipleIds : ID MultipleIds",
"ID : cal_NAME",
"ID : Variable",

};
#endif

#if YYDEBUG
int      yydebug;
#endif

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
#line 188 "cmCommandArgumentParser.y"
/* End of grammar */

/*--------------------------------------------------------------------------*/
void cmCommandArgument_yyerror(yyscan_t yyscanner, const char* message)
{
  yyGetParser->Error(message);
}
#line 390 "cmCommandArgumentParser.cxx"

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
    int      yynerrs;

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
#line 99 "cmCommandArgumentParser.y"
	{
    yyval.str = 0;
    yyGetParser->SetResult(yystack.l_mark[0].str);
  }
break;
case 2:
#line 105 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 3:
#line 108 "cmCommandArgumentParser.y"
	{
    yyval.str = yyGetParser->CombineUnions(yystack.l_mark[-1].str, yystack.l_mark[0].str);
  }
break;
case 4:
#line 113 "cmCommandArgumentParser.y"
	{
    yyval.str = 0;
  }
break;
case 5:
#line 116 "cmCommandArgumentParser.y"
	{
    yyval.str = yyGetParser->CombineUnions(yystack.l_mark[-1].str, yystack.l_mark[0].str);
  }
break;
case 6:
#line 121 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 7:
#line 124 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 8:
#line 129 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 9:
#line 132 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 10:
#line 135 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 11:
#line 138 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 12:
#line 141 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 13:
#line 144 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 14:
#line 149 "cmCommandArgumentParser.y"
	{
    yyval.str = yyGetParser->ExpandSpecialVariable(yystack.l_mark[-2].str, yystack.l_mark[-1].str);
  }
break;
case 15:
#line 152 "cmCommandArgumentParser.y"
	{
    yyval.str = yyGetParser->ExpandSpecialVariable(yystack.l_mark[-2].str, yystack.l_mark[-1].str);
  }
break;
case 16:
#line 155 "cmCommandArgumentParser.y"
	{
    yyval.str = yyGetParser->ExpandVariable(yystack.l_mark[-1].str);
  }
break;
case 17:
#line 158 "cmCommandArgumentParser.y"
	{
    yyval.str = yyGetParser->ExpandVariableForAt(yystack.l_mark[0].str);
  }
break;
case 18:
#line 163 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 19:
#line 166 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[-1].str;
  }
break;
case 20:
#line 171 "cmCommandArgumentParser.y"
	{
    yyval.str = 0;
  }
break;
case 21:
#line 174 "cmCommandArgumentParser.y"
	{
    yyval.str = yyGetParser->CombineUnions(yystack.l_mark[-1].str, yystack.l_mark[0].str);
  }
break;
case 22:
#line 179 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
case 23:
#line 182 "cmCommandArgumentParser.y"
	{
    yyval.str = yystack.l_mark[0].str;
  }
break;
#line 739 "cmCommandArgumentParser.cxx"
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
