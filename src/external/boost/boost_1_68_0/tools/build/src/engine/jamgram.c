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
#define YYPREFIX "yy"

#define YYPURE 0

#line 99 "jamgram.y"
#include "jam.h"

#include "lists.h"
#include "parse.h"
#include "scan.h"
#include "compile.h"
#include "object.h"
#include "rules.h"

# define YYMAXDEPTH 10000	/* for OSF and other less endowed yaccs */

# define F0 -1
# define P0 (PARSE *)0
# define S0 (OBJECT *)0

# define pappend( l,r )    	parse_make( PARSE_APPEND,l,r,P0,S0,S0,0 )
# define peval( c,l,r )	parse_make( PARSE_EVAL,l,r,P0,S0,S0,c )
# define pfor( s,l,r,x )    	parse_make( PARSE_FOREACH,l,r,P0,s,S0,x )
# define pif( l,r,t )	  	parse_make( PARSE_IF,l,r,t,S0,S0,0 )
# define pincl( l )       	parse_make( PARSE_INCLUDE,l,P0,P0,S0,S0,0 )
# define plist( s )	  	parse_make( PARSE_LIST,P0,P0,P0,s,S0,0 )
# define plocal( l,r,t )  	parse_make( PARSE_LOCAL,l,r,t,S0,S0,0 )
# define pmodule( l,r )	  	parse_make( PARSE_MODULE,l,r,P0,S0,S0,0 )
# define pclass( l,r )	  	parse_make( PARSE_CLASS,l,r,P0,S0,S0,0 )
# define pnull()	  	parse_make( PARSE_NULL,P0,P0,P0,S0,S0,0 )
# define pon( l,r )	  	parse_make( PARSE_ON,l,r,P0,S0,S0,0 )
# define prule( s,p )     	parse_make( PARSE_RULE,p,P0,P0,s,S0,0 )
# define prules( l,r )	  	parse_make( PARSE_RULES,l,r,P0,S0,S0,0 )
# define pset( l,r,a )          parse_make( PARSE_SET,l,r,P0,S0,S0,a )
# define pset1( l,r,t,a )	parse_make( PARSE_SETTINGS,l,r,t,S0,S0,a )
# define psetc( s,p,a,l )     	parse_make( PARSE_SETCOMP,p,a,P0,s,S0,l )
# define psete( s,l,s1,f ) 	parse_make( PARSE_SETEXEC,l,P0,P0,s,s1,f )
# define pswitch( l,r )   	parse_make( PARSE_SWITCH,l,r,P0,S0,S0,0 )
# define pwhile( l,r )   	parse_make( PARSE_WHILE,l,r,P0,S0,S0,0 )
# define preturn( l )       parse_make( PARSE_RETURN,l,P0,P0,S0,S0,0 )
# define pbreak()           parse_make( PARSE_BREAK,P0,P0,P0,S0,S0,0 )
# define pcontinue()        parse_make( PARSE_CONTINUE,P0,P0,P0,S0,S0,0 )

# define pnode( l,r )    	parse_make( F0,l,r,P0,S0,S0,0 )
# define psnode( s,l )     	parse_make( F0,l,P0,P0,s,S0,0 )

#line 63 "y.tab.c"

#if ! defined(YYSTYPE) && ! defined(YYSTYPE_IS_DECLARED)
/* Default: YYSTYPE is the semantic value type. */
typedef int YYSTYPE;
# define YYSTYPE_IS_DECLARED 1
#endif

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

#if !(defined(yylex) || defined(YYSTATE))
int YYLEX_DECL();
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define _BANG_t 257
#define _BANG_EQUALS_t 258
#define _AMPER_t 259
#define _AMPERAMPER_t 260
#define _LPAREN_t 261
#define _RPAREN_t 262
#define _PLUS_EQUALS_t 263
#define _COLON_t 264
#define _SEMIC_t 265
#define _LANGLE_t 266
#define _LANGLE_EQUALS_t 267
#define _EQUALS_t 268
#define _RANGLE_t 269
#define _RANGLE_EQUALS_t 270
#define _QUESTION_EQUALS_t 271
#define _LBRACKET_t 272
#define _RBRACKET_t 273
#define ACTIONS_t 274
#define BIND_t 275
#define BREAK_t 276
#define CASE_t 277
#define CLASS_t 278
#define CONTINUE_t 279
#define DEFAULT_t 280
#define ELSE_t 281
#define EXISTING_t 282
#define FOR_t 283
#define IF_t 284
#define IGNORE_t 285
#define IN_t 286
#define INCLUDE_t 287
#define LOCAL_t 288
#define MODULE_t 289
#define ON_t 290
#define PIECEMEAL_t 291
#define QUIETLY_t 292
#define RETURN_t 293
#define RULE_t 294
#define SWITCH_t 295
#define TOGETHER_t 296
#define UPDATED_t 297
#define WHILE_t 298
#define _LBRACE_t 299
#define _BAR_t 300
#define _BARBAR_t 301
#define _RBRACE_t 302
#define ARG 303
#define STRING 304
#define YYERRCODE 256
typedef short YYINT;
static const YYINT yylhs[] = {                           -1,
    0,    0,    2,    2,    1,    1,    6,    8,    1,    3,
    9,    7,    7,   10,   10,   12,   12,   13,   13,    4,
   14,    4,   15,    4,   18,    4,   19,   20,    4,   21,
    4,    4,    4,   22,   23,    4,   24,   26,    4,   28,
   29,    4,   30,   31,    4,   32,   33,    4,   34,   35,
    4,   36,   37,   38,    4,    4,   41,   42,    4,   17,
   17,   17,   17,   27,   43,   27,   44,   27,   45,   27,
   46,   27,   47,   27,   48,   27,   49,   27,   50,   27,
   51,   27,   52,   27,   53,   27,   54,   27,   55,   27,
   25,   25,   57,   58,   56,   11,   11,    5,   59,   59,
   16,   61,   16,   62,   60,   63,   60,   64,   60,   39,
   39,   65,   65,   65,   65,   65,   65,   40,   66,   40,
};
static const YYINT yylen[] = {                            2,
    0,    1,    1,    1,    1,    2,    0,    0,    7,    0,
    0,    3,    1,    3,    0,    1,    0,    2,    0,    3,
    0,    4,    0,    4,    0,    5,    0,    0,    8,    0,
    4,    2,    2,    0,    0,   10,    0,    0,    7,    0,
    0,    8,    0,    0,    7,    0,    0,    7,    0,    0,
    7,    0,    0,    0,    8,    3,    0,    0,    9,    1,
    1,    1,    2,    1,    0,    4,    0,    4,    0,    4,
    0,    4,    0,    4,    0,    4,    0,    4,    0,    4,
    0,    4,    0,    4,    0,    4,    0,    3,    0,    4,
    0,    2,    0,    0,    6,    1,    3,    1,    0,    2,
    1,    0,    4,    0,    3,    0,    5,    0,    5,    0,
    2,    1,    1,    1,    1,    1,    1,    0,    0,    3,
};
static const YYINT yydefred[] = {                         0,
  102,  110,    0,   46,    0,    0,   40,   21,    0,   43,
    0,   30,   37,   49,    0,    0,    0,    2,    0,    0,
    0,    0,    0,   32,   99,   33,   16,    0,    0,   99,
   99,   99,  101,    0,   99,   99,    0,    4,    0,    3,
   99,    6,   52,   61,   60,   62,    0,   27,   25,    0,
  104,    0,  117,  114,  116,  115,  113,  112,    0,  111,
    0,    0,    0,    0,   87,   89,    0,    0,    0,    0,
    0,   56,    0,    0,    0,   20,    0,    0,   63,   99,
   99,    0,   99,  103,  119,    0,   99,   47,  100,   34,
    0,    0,   85,   67,   77,   79,   69,   71,   65,   73,
   75,   41,   81,   83,   22,   11,   13,    0,   44,   31,
   38,    0,   24,   53,    0,    0,  108,  106,  105,   99,
   57,   97,    0,   99,   88,    0,   99,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   99,    8,
    0,    0,    0,    0,   28,   26,   99,   99,  120,    0,
    0,    0,   90,   86,    0,    0,    0,   70,   72,    0,
   74,   76,    0,    0,    0,   12,    0,    0,   93,    0,
    0,    0,   99,   54,   99,  109,  107,   58,   48,   35,
    0,    9,   45,    0,   39,   92,   51,    0,    0,    0,
    0,    0,    0,   42,    0,   14,   55,   29,   59,    0,
   18,   94,   36,    0,   95,
};
static const YYINT yydgoto[] = {                         17,
   38,   39,   40,   19,   61,   31,  108,  167,  139,  174,
   62,   20,  194,   30,   41,   67,   49,   81,   80,  175,
   35,  124,  192,   36,  170,  142,   68,   29,  136,   32,
  141,   25,  123,   37,  112,   78,  144,  189,   23,   86,
  150,  191,  133,  128,  131,  132,  134,  135,  129,  130,
  137,  138,  127,   91,   92,  171,  184,  204,   63,   52,
   22,   83,  148,  147,   60,  120,
};
static const YYINT yysindex[] = {                       112,
    0,    0, -235,    0, -223, -244,    0,    0,    0,    0,
 -263,    0,    0,    0,  112,    0,    0,    0,  112, -251,
  -31, -265,  -27,    0,    0,    0,    0, -256, -245,    0,
    0,    0,    0,  140,    0,    0, -245,    0, -249,    0,
    0,    0,    0,    0,    0,    0, -217,    0,    0, -263,
    0, -211,    0,    0,    0,    0,    0,    0, -203,    0,
 -200, -216, -263, -194,    0,    0, -188,  -88, -164, -165,
 -195,    0, -160, -191,  -39,    0, -148, -185,    0,    0,
    0, -275,    0,    0,    0, -180,    0,    0,    0,    0,
 -245, -245,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -144,    0,    0,
    0, -176,    0,    0, -119, -141,    0,    0,    0,    0,
    0,    0,  112,    0,    0,  -44,    0, -245, -245, -245,
 -245, -245, -245, -245, -245,  112, -245, -245,    0,    0,
  112, -143,  112, -124,    0,    0,    0,    0,    0, -163,
 -149, -145,    0,    0, -173,  -22,  -22,    0,    0, -173,
    0,    0, -140,  -83,  -83,    0,  112, -139,    0, -129,
 -143, -128,    0,    0,    0,    0,    0,    0,    0,    0,
 -142,    0,    0, -115,    0,    0,    0,  -73,  140,  -75,
 -110,  112,  140,    0,  -71,    0,    0,    0,    0, -106,
    0,    0,    0,  112,    0,
};
static const YYINT yyrindex[] = {                         3,
    0,    0,    0,    0,    0, -100,    0,    0, -152,    0,
    0,    0,    0,    0, -268,   58,    0,    0,    4,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -89,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -92,    0,
 -242,    0, -189,    0,    0,    0, -102,    0,    0,  -57,
    0,    0,    0,    0,  -90,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -268,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -268,    0,    0,    0,    0,
 -268,  -85, -268,   84,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -58, -214, -153,    0,    0,   52,
    0,    0,    0, -240, -233,    0,   15,    0,    0,    0,
  -85,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    1,    0,    0,    0,    0,    0,    0,    0,  -89,    0,
    0, -268,  -89,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -253,    0,
};
static const YYINT yygindex[] = {                         0,
   14, -104,  163,  -34,  -25,    0,    0,    0,    0,    0,
  -33,  228,    0,    0,    0,    2,  120,    0,    0,    0,
    0,    0,    0,    0,   67,    0,   -2,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 443
static const YYINT yytable[] = {                         72,
   19,   21,    1,    5,   69,   70,   71,   77,    1,   73,
   74,   65,   34,   18,   10,   66,   21,  117,  151,   96,
   21,   82,   96,   10,   50,   17,    1,  118,   84,   24,
   96,  163,   42,   10,   75,   21,  168,   51,  172,   33,
   17,   26,   43,   27,   78,   78,   64,   78,   10,  119,
   79,   82,   76,  122,  115,  116,   96,   33,   82,   82,
   82,   84,  182,   87,   89,   84,   84,   84,   98,   98,
   98,   85,   98,   98,   98,   98,   98,   98,   98,   98,
   98,   98,   88,   98,   78,   78,   78,  200,  125,  126,
   98,   90,   97,   98,  149,  100,  101,   93,  152,  205,
  105,  154,  106,  109,  110,   80,   80,  111,   80,   98,
   98,   98,    7,  166,  177,    7,  113,  114,  121,    7,
  140,  176,  143,  146,   21,  155,  156,  157,  158,  159,
  160,  161,  162,  169,  164,  165,  173,   21,  193,  188,
  178,   16,   21,   44,   21,   80,   80,   80,   45,  190,
    7,   46,  179,  180,  197,   64,   64,   64,  201,   64,
   47,  181,  183,   64,   64,   64,   64,   64,   21,   94,
   95,   96,  185,  187,   94,   95,   96,   97,   98,   99,
  100,  101,   97,   98,   99,  100,  101,  195,  196,  198,
   21,  199,  202,   21,   21,  203,   64,   64,   64,   68,
   68,   68,   17,   68,   17,   21,  118,   10,   50,   68,
  102,  103,  104,   94,   95,   96,   91,  153,   94,   95,
   96,   97,   98,   99,  100,  101,   97,   98,   99,  100,
  101,   44,  107,   28,  145,   94,   45,  186,    0,   46,
   68,   68,   68,   97,   98,   99,  100,  101,   47,    0,
    0,    0,    0,    0,   53,  103,  104,   54,   48,    0,
  103,  104,    0,   55,   56,    0,    0,    0,   57,   58,
    0,    0,   19,    0,   19,   59,   19,   19,   19,   19,
    5,    0,    0,   19,   19,    0,    0,   19,   19,   19,
   19,   10,    0,   19,   19,   19,   17,   17,   19,   19,
    0,    0,   19,   19,    0,    5,    0,    0,   17,   66,
   66,   66,    0,   66,    0,    0,   10,    0,    0,   66,
  101,   23,   23,    0,    0,  101,    0,    0,  101,   23,
    0,    0,    0,    0,    0,    0,    0,  101,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  101,    0,    0,
   66,   66,   66,    0,    0,   15,    0,   15,    0,   15,
   23,   15,   15,    0,    0,    0,   15,   15,    0,    0,
   15,   15,   15,   15,    0,    0,   15,   15,   15,    0,
    0,   15,   15,    1,    0,    2,   15,    3,    0,    4,
    5,    0,    0,    0,    6,    7,    0,    0,    8,    9,
   10,   11,    0,    0,   12,    0,   13,    0,    0,   14,
   15,    1,    0,    2,   16,    3,    0,    4,    5,    0,
    0,    0,    6,    7,    0,    0,    8,   27,   10,   11,
    0,    0,   12,    0,   13,    0,    0,   14,   15,    0,
    0,    0,   16,
};
static const YYINT yycheck[] = {                         34,
    0,    0,    0,    0,   30,   31,   32,   41,  272,   35,
   36,  257,   11,    0,    0,  261,   15,  293,  123,  262,
   19,  262,  265,  277,  290,  294,  272,  303,  262,  265,
  273,  136,   19,  302,   37,   34,  141,  303,  143,  303,
  294,  265,  294,  288,  259,  260,  303,  262,  302,   83,
  268,   50,  302,   87,   80,   81,  299,  303,  299,  300,
  301,  273,  167,  264,   63,  299,  300,  301,  258,  259,
  260,  275,  262,  263,  264,  265,  266,  267,  268,  269,
  270,  271,  299,  273,  299,  300,  301,  192,   91,   92,
  280,  286,  266,  267,  120,  269,  270,  286,  124,  204,
  265,  127,  268,  299,  265,  259,  260,  299,  262,  299,
  300,  301,  265,  139,  148,  268,  265,  303,  299,  272,
  265,  147,  299,  265,  123,  128,  129,  130,  131,  132,
  133,  134,  135,  277,  137,  138,  261,  136,  281,  173,
  304,  294,  141,  263,  143,  299,  300,  301,  268,  175,
  303,  271,  302,  299,  189,  258,  259,  260,  193,  262,
  280,  302,  302,  266,  267,  268,  269,  270,  167,  258,
  259,  260,  302,  302,  258,  259,  260,  266,  267,  268,
  269,  270,  266,  267,  268,  269,  270,  303,  262,  265,
  189,  302,  264,  192,  193,  302,  299,  300,  301,  258,
  259,  260,  303,  262,  294,  204,  299,  265,  299,  268,
  299,  300,  301,  258,  259,  260,  302,  262,  258,  259,
  260,  266,  267,  268,  269,  270,  266,  267,  268,  269,
  270,  263,   70,    6,  115,  258,  268,  171,   -1,  271,
  299,  300,  301,  266,  267,  268,  269,  270,  280,   -1,
   -1,   -1,   -1,   -1,  282,  300,  301,  285,  290,   -1,
  300,  301,   -1,  291,  292,   -1,   -1,   -1,  296,  297,
   -1,   -1,  272,   -1,  274,  303,  276,  277,  278,  279,
  277,   -1,   -1,  283,  284,   -1,   -1,  287,  288,  289,
  290,  277,   -1,  293,  294,  295,  294,  294,  298,  299,
   -1,   -1,  302,  303,   -1,  302,   -1,   -1,  294,  258,
  259,  260,   -1,  262,   -1,   -1,  302,   -1,   -1,  268,
  263,  264,  265,   -1,   -1,  268,   -1,   -1,  271,  272,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  280,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  290,   -1,   -1,
  299,  300,  301,   -1,   -1,  272,   -1,  274,   -1,  276,
  303,  278,  279,   -1,   -1,   -1,  283,  284,   -1,   -1,
  287,  288,  289,  290,   -1,   -1,  293,  294,  295,   -1,
   -1,  298,  299,  272,   -1,  274,  303,  276,   -1,  278,
  279,   -1,   -1,   -1,  283,  284,   -1,   -1,  287,  288,
  289,  290,   -1,   -1,  293,   -1,  295,   -1,   -1,  298,
  299,  272,   -1,  274,  303,  276,   -1,  278,  279,   -1,
   -1,   -1,  283,  284,   -1,   -1,  287,  288,  289,  290,
   -1,   -1,  293,   -1,  295,   -1,   -1,  298,  299,   -1,
   -1,   -1,  303,
};
#define YYFINAL 17
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 304
#define YYUNDFTOKEN 373
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"_BANG_t","_BANG_EQUALS_t",
"_AMPER_t","_AMPERAMPER_t","_LPAREN_t","_RPAREN_t","_PLUS_EQUALS_t","_COLON_t",
"_SEMIC_t","_LANGLE_t","_LANGLE_EQUALS_t","_EQUALS_t","_RANGLE_t",
"_RANGLE_EQUALS_t","_QUESTION_EQUALS_t","_LBRACKET_t","_RBRACKET_t","ACTIONS_t",
"BIND_t","BREAK_t","CASE_t","CLASS_t","CONTINUE_t","DEFAULT_t","ELSE_t",
"EXISTING_t","FOR_t","IF_t","IGNORE_t","IN_t","INCLUDE_t","LOCAL_t","MODULE_t",
"ON_t","PIECEMEAL_t","QUIETLY_t","RETURN_t","RULE_t","SWITCH_t","TOGETHER_t",
"UPDATED_t","WHILE_t","_LBRACE_t","_BAR_t","_BARBAR_t","_RBRACE_t","ARG",
"STRING",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : run",
"run :",
"run : rules",
"block : null",
"block : rules",
"rules : rule",
"rules : rule rules",
"$$1 :",
"$$2 :",
"rules : LOCAL_t $$1 list assign_list_opt _SEMIC_t $$2 block",
"null :",
"$$3 :",
"assign_list_opt : _EQUALS_t $$3 list",
"assign_list_opt : null",
"arglist_opt : _LPAREN_t lol _RPAREN_t",
"arglist_opt :",
"local_opt : LOCAL_t",
"local_opt :",
"else_opt : ELSE_t rule",
"else_opt :",
"rule : _LBRACE_t block _RBRACE_t",
"$$4 :",
"rule : INCLUDE_t $$4 list _SEMIC_t",
"$$5 :",
"rule : ARG $$5 lol _SEMIC_t",
"$$6 :",
"rule : arg assign $$6 list _SEMIC_t",
"$$7 :",
"$$8 :",
"rule : arg ON_t $$7 list assign $$8 list _SEMIC_t",
"$$9 :",
"rule : RETURN_t $$9 list _SEMIC_t",
"rule : BREAK_t _SEMIC_t",
"rule : CONTINUE_t _SEMIC_t",
"$$10 :",
"$$11 :",
"rule : FOR_t local_opt ARG IN_t $$10 list _LBRACE_t $$11 block _RBRACE_t",
"$$12 :",
"$$13 :",
"rule : SWITCH_t $$12 list _LBRACE_t $$13 cases _RBRACE_t",
"$$14 :",
"$$15 :",
"rule : IF_t $$14 expr _LBRACE_t $$15 block _RBRACE_t else_opt",
"$$16 :",
"$$17 :",
"rule : MODULE_t $$16 list _LBRACE_t $$17 block _RBRACE_t",
"$$18 :",
"$$19 :",
"rule : CLASS_t $$18 lol _LBRACE_t $$19 block _RBRACE_t",
"$$20 :",
"$$21 :",
"rule : WHILE_t $$20 expr $$21 _LBRACE_t block _RBRACE_t",
"$$22 :",
"$$23 :",
"$$24 :",
"rule : local_opt RULE_t $$22 ARG $$23 arglist_opt $$24 rule",
"rule : ON_t arg rule",
"$$25 :",
"$$26 :",
"rule : ACTIONS_t eflags ARG bindlist _LBRACE_t $$25 STRING $$26 _RBRACE_t",
"assign : _EQUALS_t",
"assign : _PLUS_EQUALS_t",
"assign : _QUESTION_EQUALS_t",
"assign : DEFAULT_t _EQUALS_t",
"expr : arg",
"$$27 :",
"expr : expr _EQUALS_t $$27 expr",
"$$28 :",
"expr : expr _BANG_EQUALS_t $$28 expr",
"$$29 :",
"expr : expr _LANGLE_t $$29 expr",
"$$30 :",
"expr : expr _LANGLE_EQUALS_t $$30 expr",
"$$31 :",
"expr : expr _RANGLE_t $$31 expr",
"$$32 :",
"expr : expr _RANGLE_EQUALS_t $$32 expr",
"$$33 :",
"expr : expr _AMPER_t $$33 expr",
"$$34 :",
"expr : expr _AMPERAMPER_t $$34 expr",
"$$35 :",
"expr : expr _BAR_t $$35 expr",
"$$36 :",
"expr : expr _BARBAR_t $$36 expr",
"$$37 :",
"expr : arg IN_t $$37 list",
"$$38 :",
"expr : _BANG_t $$38 expr",
"$$39 :",
"expr : _LPAREN_t $$39 expr _RPAREN_t",
"cases :",
"cases : case cases",
"$$40 :",
"$$41 :",
"case : CASE_t $$40 ARG _COLON_t $$41 block",
"lol : list",
"lol : list _COLON_t lol",
"list : listp",
"listp :",
"listp : listp arg",
"arg : ARG",
"$$42 :",
"arg : _LBRACKET_t $$42 func _RBRACKET_t",
"$$43 :",
"func : ARG $$43 lol",
"$$44 :",
"func : ON_t arg ARG $$44 lol",
"$$45 :",
"func : ON_t arg RETURN_t $$45 list",
"eflags :",
"eflags : eflags eflag",
"eflag : UPDATED_t",
"eflag : TOGETHER_t",
"eflag : IGNORE_t",
"eflag : QUIETLY_t",
"eflag : PIECEMEAL_t",
"eflag : EXISTING_t",
"bindlist :",
"$$46 :",
"bindlist : BIND_t $$46 list",

};
#endif

#if YYDEBUG
int      yydebug;
#endif

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;
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
/* variables for the parser stack */
static YYSTACKDATA yystack;

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
case 2:
#line 147 "jamgram.y"
	{ parse_save( yystack.l_mark[0].parse ); }
break;
case 3:
#line 158 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; }
break;
case 4:
#line 160 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; }
break;
case 5:
#line 164 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; }
break;
case 6:
#line 166 "jamgram.y"
	{ yyval.parse = prules( yystack.l_mark[-1].parse, yystack.l_mark[0].parse ); }
break;
case 7:
#line 167 "jamgram.y"
	{ yymode( SCAN_ASSIGN ); }
break;
case 8:
#line 167 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 9:
#line 168 "jamgram.y"
	{ yyval.parse = plocal( yystack.l_mark[-4].parse, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 10:
#line 172 "jamgram.y"
	{ yyval.parse = pnull(); }
break;
case 11:
#line 175 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 12:
#line 176 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; yyval.number = ASSIGN_SET; }
break;
case 13:
#line 178 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; yyval.number = ASSIGN_APPEND; }
break;
case 14:
#line 182 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; }
break;
case 15:
#line 184 "jamgram.y"
	{ yyval.parse = P0; }
break;
case 16:
#line 188 "jamgram.y"
	{ yyval.number = 1; }
break;
case 17:
#line 190 "jamgram.y"
	{ yyval.number = 0; }
break;
case 18:
#line 194 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; }
break;
case 19:
#line 196 "jamgram.y"
	{ yyval.parse = pnull(); }
break;
case 20:
#line 199 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; }
break;
case 21:
#line 200 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 22:
#line 201 "jamgram.y"
	{ yyval.parse = pincl( yystack.l_mark[-1].parse ); yymode( SCAN_NORMAL ); }
break;
case 23:
#line 202 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 24:
#line 203 "jamgram.y"
	{ yyval.parse = prule( yystack.l_mark[-3].string, yystack.l_mark[-1].parse ); yymode( SCAN_NORMAL ); }
break;
case 25:
#line 204 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 26:
#line 205 "jamgram.y"
	{ yyval.parse = pset( yystack.l_mark[-4].parse, yystack.l_mark[-1].parse, yystack.l_mark[-3].number ); yymode( SCAN_NORMAL ); }
break;
case 27:
#line 206 "jamgram.y"
	{ yymode( SCAN_ASSIGN ); }
break;
case 28:
#line 206 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 29:
#line 207 "jamgram.y"
	{ yyval.parse = pset1( yystack.l_mark[-7].parse, yystack.l_mark[-4].parse, yystack.l_mark[-1].parse, yystack.l_mark[-3].number ); yymode( SCAN_NORMAL ); }
break;
case 30:
#line 208 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 31:
#line 209 "jamgram.y"
	{ yyval.parse = preturn( yystack.l_mark[-1].parse ); yymode( SCAN_NORMAL ); }
break;
case 32:
#line 211 "jamgram.y"
	{ yyval.parse = pbreak(); }
break;
case 33:
#line 213 "jamgram.y"
	{ yyval.parse = pcontinue(); }
break;
case 34:
#line 214 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 35:
#line 214 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 36:
#line 215 "jamgram.y"
	{ yyval.parse = pfor( yystack.l_mark[-7].string, yystack.l_mark[-4].parse, yystack.l_mark[-1].parse, yystack.l_mark[-8].number ); }
break;
case 37:
#line 216 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 38:
#line 216 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 39:
#line 217 "jamgram.y"
	{ yyval.parse = pswitch( yystack.l_mark[-4].parse, yystack.l_mark[-1].parse ); }
break;
case 40:
#line 218 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 41:
#line 218 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 42:
#line 219 "jamgram.y"
	{ yyval.parse = pif( yystack.l_mark[-5].parse, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 43:
#line 220 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 44:
#line 220 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 45:
#line 221 "jamgram.y"
	{ yyval.parse = pmodule( yystack.l_mark[-4].parse, yystack.l_mark[-1].parse ); }
break;
case 46:
#line 222 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 47:
#line 222 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 48:
#line 223 "jamgram.y"
	{ yyval.parse = pclass( yystack.l_mark[-4].parse, yystack.l_mark[-1].parse ); }
break;
case 49:
#line 224 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 50:
#line 224 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 51:
#line 225 "jamgram.y"
	{ yyval.parse = pwhile( yystack.l_mark[-4].parse, yystack.l_mark[-1].parse ); }
break;
case 52:
#line 226 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 53:
#line 226 "jamgram.y"
	{ yymode( SCAN_PARAMS ); }
break;
case 54:
#line 226 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 55:
#line 227 "jamgram.y"
	{ yyval.parse = psetc( yystack.l_mark[-4].string, yystack.l_mark[0].parse, yystack.l_mark[-2].parse, yystack.l_mark[-7].number ); }
break;
case 56:
#line 229 "jamgram.y"
	{ yyval.parse = pon( yystack.l_mark[-1].parse, yystack.l_mark[0].parse ); }
break;
case 57:
#line 231 "jamgram.y"
	{ yymode( SCAN_STRING ); }
break;
case 58:
#line 233 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 59:
#line 235 "jamgram.y"
	{ yyval.parse = psete( yystack.l_mark[-6].string,yystack.l_mark[-5].parse,yystack.l_mark[-2].string,yystack.l_mark[-7].number ); }
break;
case 60:
#line 243 "jamgram.y"
	{ yyval.number = ASSIGN_SET; }
break;
case 61:
#line 245 "jamgram.y"
	{ yyval.number = ASSIGN_APPEND; }
break;
case 62:
#line 247 "jamgram.y"
	{ yyval.number = ASSIGN_DEFAULT; }
break;
case 63:
#line 249 "jamgram.y"
	{ yyval.number = ASSIGN_DEFAULT; }
break;
case 64:
#line 256 "jamgram.y"
	{ yyval.parse = peval( EXPR_EXISTS, yystack.l_mark[0].parse, pnull() ); yymode( SCAN_COND ); }
break;
case 65:
#line 257 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 66:
#line 258 "jamgram.y"
	{ yyval.parse = peval( EXPR_EQUALS, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 67:
#line 259 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 68:
#line 260 "jamgram.y"
	{ yyval.parse = peval( EXPR_NOTEQ, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 69:
#line 261 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 70:
#line 262 "jamgram.y"
	{ yyval.parse = peval( EXPR_LESS, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 71:
#line 263 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 72:
#line 264 "jamgram.y"
	{ yyval.parse = peval( EXPR_LESSEQ, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 73:
#line 265 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 74:
#line 266 "jamgram.y"
	{ yyval.parse = peval( EXPR_MORE, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 75:
#line 267 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 76:
#line 268 "jamgram.y"
	{ yyval.parse = peval( EXPR_MOREEQ, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 77:
#line 269 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 78:
#line 270 "jamgram.y"
	{ yyval.parse = peval( EXPR_AND, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 79:
#line 271 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 80:
#line 272 "jamgram.y"
	{ yyval.parse = peval( EXPR_AND, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 81:
#line 273 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 82:
#line 274 "jamgram.y"
	{ yyval.parse = peval( EXPR_OR, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 83:
#line 275 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 84:
#line 276 "jamgram.y"
	{ yyval.parse = peval( EXPR_OR, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 85:
#line 277 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 86:
#line 278 "jamgram.y"
	{ yyval.parse = peval( EXPR_IN, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); yymode( SCAN_COND ); }
break;
case 87:
#line 279 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 88:
#line 280 "jamgram.y"
	{ yyval.parse = peval( EXPR_NOT, yystack.l_mark[0].parse, pnull() ); }
break;
case 89:
#line 281 "jamgram.y"
	{ yymode( SCAN_CONDB ); }
break;
case 90:
#line 282 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; }
break;
case 91:
#line 293 "jamgram.y"
	{ yyval.parse = P0; }
break;
case 92:
#line 295 "jamgram.y"
	{ yyval.parse = pnode( yystack.l_mark[-1].parse, yystack.l_mark[0].parse ); }
break;
case 93:
#line 298 "jamgram.y"
	{ yymode( SCAN_CASE ); }
break;
case 94:
#line 298 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 95:
#line 299 "jamgram.y"
	{ yyval.parse = psnode( yystack.l_mark[-3].string, yystack.l_mark[0].parse ); }
break;
case 96:
#line 308 "jamgram.y"
	{ yyval.parse = pnode( P0, yystack.l_mark[0].parse ); }
break;
case 97:
#line 310 "jamgram.y"
	{ yyval.parse = pnode( yystack.l_mark[0].parse, yystack.l_mark[-2].parse ); }
break;
case 98:
#line 320 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; }
break;
case 99:
#line 324 "jamgram.y"
	{ yyval.parse = pnull(); }
break;
case 100:
#line 326 "jamgram.y"
	{ yyval.parse = pappend( yystack.l_mark[-1].parse, yystack.l_mark[0].parse ); }
break;
case 101:
#line 330 "jamgram.y"
	{ yyval.parse = plist( yystack.l_mark[0].string ); }
break;
case 102:
#line 331 "jamgram.y"
	{ yyval.number = yymode( SCAN_CALL ); }
break;
case 103:
#line 332 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; yymode( yystack.l_mark[-2].number ); }
break;
case 104:
#line 340 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 105:
#line 341 "jamgram.y"
	{ yyval.parse = prule( yystack.l_mark[-2].string, yystack.l_mark[0].parse ); }
break;
case 106:
#line 342 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 107:
#line 343 "jamgram.y"
	{ yyval.parse = pon( yystack.l_mark[-3].parse, prule( yystack.l_mark[-2].string, yystack.l_mark[0].parse ) ); }
break;
case 108:
#line 344 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 109:
#line 345 "jamgram.y"
	{ yyval.parse = pon( yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 110:
#line 355 "jamgram.y"
	{ yyval.number = 0; }
break;
case 111:
#line 357 "jamgram.y"
	{ yyval.number = yystack.l_mark[-1].number | yystack.l_mark[0].number; }
break;
case 112:
#line 361 "jamgram.y"
	{ yyval.number = EXEC_UPDATED; }
break;
case 113:
#line 363 "jamgram.y"
	{ yyval.number = EXEC_TOGETHER; }
break;
case 114:
#line 365 "jamgram.y"
	{ yyval.number = EXEC_IGNORE; }
break;
case 115:
#line 367 "jamgram.y"
	{ yyval.number = EXEC_QUIETLY; }
break;
case 116:
#line 369 "jamgram.y"
	{ yyval.number = EXEC_PIECEMEAL; }
break;
case 117:
#line 371 "jamgram.y"
	{ yyval.number = EXEC_EXISTING; }
break;
case 118:
#line 380 "jamgram.y"
	{ yyval.parse = pnull(); }
break;
case 119:
#line 381 "jamgram.y"
	{ yymode( SCAN_PUNCT ); }
break;
case 120:
#line 382 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; }
break;
#line 1229 "y.tab.c"
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
