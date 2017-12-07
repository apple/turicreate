#ifndef lint
static const char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20140101

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)

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

#line 61 "y.tab.c"

#ifndef YYSTYPE
typedef int YYSTYPE;
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
static const short yylhs[] = {                           -1,
    0,    0,    2,    2,    1,    1,    1,    3,    6,    6,
    7,    7,    9,    9,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    4,   16,   17,    4,   11,   11,   11,   11,   13,   13,
   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,
   13,   13,   12,   12,   18,    8,    8,    5,   19,   19,
   10,   21,   10,   20,   20,   20,   14,   14,   22,   22,
   22,   22,   22,   22,   15,   15,
};
static const short yylen[] = {                            2,
    0,    1,    1,    1,    1,    2,    5,    0,    2,    1,
    3,    0,    1,    0,    3,    3,    3,    4,    6,    3,
    2,    2,    8,    5,    5,    5,    5,    5,    7,    5,
    3,    0,    0,    9,    1,    1,    1,    2,    1,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    2,    3,    0,    2,    4,    1,    3,    1,    0,    2,
    1,    0,    4,    2,    4,    4,    0,    2,    1,    1,
    1,    1,    1,    1,    0,    2,
};
static const short yydefred[] = {                         0,
   62,   67,    0,   59,    0,    0,    0,   59,    0,   59,
    0,   59,   59,    0,    0,    0,    0,    2,    0,    0,
    0,    0,    0,   21,    0,    0,    0,   22,   13,    0,
    0,    0,   61,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    4,    0,    3,    0,    6,    0,   36,   35,
   37,    0,   59,   59,    0,   59,    0,   74,   71,   73,
   72,   70,   69,    0,   68,   59,    0,   60,    0,   51,
    0,   59,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   16,   59,   10,    0,    0,   31,   20,
    0,    0,   15,   17,    0,   38,    0,    0,    0,   64,
   63,   59,    0,   57,    0,   59,   52,   50,    0,    0,
    0,   42,   43,    0,   44,   45,    0,    0,    0,    9,
    0,    0,    0,    0,    0,    0,   59,    0,   59,   18,
   59,   59,   76,   32,   27,    0,    0,    7,   26,    0,
   24,   54,   28,    0,   30,    0,   66,   65,    0,    0,
    0,    0,   11,   19,   33,    0,   29,   55,    0,   23,
   34,
};
static const short yydgoto[] = {                         17,
   43,   44,   45,   19,   25,   87,  128,   26,   20,   34,
   54,  124,   35,   23,  103,  149,  159,  125,   27,   57,
   22,   65,
};
static const short yysindex[] = {                        64,
    0,    0, -241,    0, -239, -255, -245,    0,    0,    0,
 -267,    0,    0, -245,   64,    0,    0,    0,   64, -233,
 -201, -265,  -34,    0, -199, -221, -267,    0,    0, -237,
 -245, -245,    0, -195,  -87, -173, -170, -206,   92, -156,
 -179,  -67,    0, -184,    0, -138,    0, -174,    0,    0,
    0, -140,    0,    0, -267,    0, -142,    0,    0,    0,
    0,    0,    0, -139,    0,    0,   64,    0, -148,    0,
  -51,    0, -245, -245, -245, -245, -245, -245, -245, -245,
   64, -245, -245,    0,    0,    0, -123,   64,    0,    0,
 -131,   64,    0,    0, -113,    0,  -27, -116, -293,    0,
    0,    0, -149,    0, -151,    0,    0,    0,  -24,   -2,
   -2,    0,    0,  -24,    0,    0, -144,  134,  134,    0,
   64, -133, -114, -124, -131, -112,    0,   92,    0,    0,
    0,    0,    0,    0,    0, -111,  -94,    0,    0,  -70,
    0,    0,    0,  -58,    0,  -60,    0,    0, -109,   64,
   92,   64,    0,    0,    0,  -96,    0,    0,  -92,    0,
    0,
};
static const short yyrindex[] = {                         3,
    0,    0,    0,    0,    0,  -79,    0,    0,  -42,    0,
    0,    0,    0,    0, -181,  -43,    0,    0,    4,    0,
    0,    0,    0,    0, -185,    0, -103,    0,    0,    0,
    0,    0,    0, -215,    0,    0,  -38,    0,  -63,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -64,    0,    0, -181,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -181,    0,    0,    0,    0,    0,    0, -181,    0,    0,
  -62, -181,    0,    0,   36,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -228, -251,
 -115,    0,    0, -125,    0,    0,    0, -225, -175,    0,
   15,    0,    0,    0,  -62,    0,    0,  -63,    0,    0,
    0,    0,    0,    0,    0,    0,    1,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -181,
  -63, -162,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
static const short yygindex[] = {                         0,
   81,  -53,  201,  -32,   10,    0,    0,  -10,  233,    2,
  157,  130,   28,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
#define YYTABLESIZE 404
static const short yytable[] = {                        131,
   25,   21,    1,    5,    1,   46,   89,   46,   46,  132,
   46,   31,   39,  105,    8,   32,   21,   36,   37,   38,
   21,   40,   41,   24,   55,   28,    1,  117,   68,   41,
   41,   41,   29,   41,  122,   33,   48,   56,  126,   41,
   21,   42,   39,   39,   39,  100,   39,   46,   46,   46,
   39,   39,   39,   39,   39,  104,   99,   33,   70,   71,
   48,   49,   97,   98,   66,   69,   50,  138,   21,   51,
   41,   41,   41,   48,   48,   48,   56,   67,   52,   56,
   18,  108,   21,   39,   39,   39,   49,   56,   53,   21,
   72,   84,   88,   21,  120,  145,  156,   85,  158,   47,
  109,  110,  111,  112,  113,  114,  115,  116,   90,  118,
  119,  133,   14,   56,    8,  136,  144,   93,  157,   91,
    8,  148,   21,   49,   49,   49,   94,   96,   95,   21,
  101,   14,   40,   40,   40,  102,   40,  106,  146,    8,
  147,  121,   40,   47,   47,  123,   47,  127,  130,  134,
  135,   21,   21,   21,   58,   58,   58,  137,   58,   58,
   58,   58,   58,   58,   58,   58,   58,   58,  139,   58,
   73,   74,   75,   40,   40,   40,   58,  141,   76,   77,
   78,   79,   80,   47,   47,   47,  151,  150,  140,  143,
   73,   74,   75,  152,  155,   58,   58,   58,   76,   77,
   78,   79,   80,  153,  154,  160,   73,   74,   75,  161,
  107,   81,   82,   83,   76,   77,   78,   79,   80,   61,
   59,   59,   59,   14,   61,   59,    8,   61,   59,   59,
   14,   92,   82,   83,   75,   49,   61,   86,   30,   53,
   50,   76,   77,   51,   79,   80,   61,   58,   82,   83,
   59,   13,   52,  129,  142,   73,   60,   61,    0,   59,
   59,   62,   63,   76,   77,   78,   79,   80,   64,    0,
    0,    0,   25,    0,   25,    0,   25,   25,   25,   25,
    5,    0,    0,   25,   25,    0,    0,   25,   25,   25,
   25,    8,    0,   25,   25,   25,   14,   14,   25,   25,
    0,    0,   25,   25,    0,    5,    0,   12,   14,   12,
    0,   12,    0,   12,   12,    0,    8,    0,   12,   12,
    0,    0,   12,   12,   12,   12,    0,    0,   12,   12,
   12,    0,    0,   12,   12,    1,    0,    2,   12,    3,
    0,    4,    5,    0,    0,    0,    6,    7,    0,    0,
    8,    9,   10,   11,    0,    0,   12,    0,   13,    0,
    0,   14,   15,    1,    0,    2,   16,    3,    0,    4,
    5,    0,    0,    0,    6,    7,    0,    0,    8,   29,
   10,   11,    0,    0,   12,    0,   13,    0,    0,   14,
   15,   73,   74,   75,   16,    0,    0,    0,    0,   76,
   77,   78,   79,   80,
};
static const short yycheck[] = {                        293,
    0,    0,    0,    0,  272,   16,   39,  259,  260,  303,
  262,  257,   11,   67,    0,  261,   15,    8,    9,   10,
   19,   12,   13,  265,  290,  265,  272,   81,   27,  258,
  259,  260,  288,  262,   88,  303,  262,  303,   92,  268,
   39,   14,  258,  259,  260,   56,  262,  299,  300,  301,
  266,  267,  268,  269,  270,   66,   55,  303,   31,   32,
  294,  263,   53,   54,  264,  303,  268,  121,   67,  271,
  299,  300,  301,  299,  300,  301,  262,  299,  280,  265,
    0,   72,   81,  299,  300,  301,  262,  273,  290,   88,
  286,  265,  299,   92,   85,  128,  150,  268,  152,   19,
   73,   74,   75,   76,   77,   78,   79,   80,  265,   82,
   83,  102,  294,  299,  277,  106,  127,  302,  151,  299,
  302,  132,  121,  299,  300,  301,  265,  268,  303,  128,
  273,  294,  258,  259,  260,  275,  262,  286,  129,  302,
  131,  265,  268,  259,  260,  277,  262,  261,  265,  299,
  302,  150,  151,  152,  258,  259,  260,  302,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  271,  302,  273,
  258,  259,  260,  299,  300,  301,  280,  302,  266,  267,
  268,  269,  270,  299,  300,  301,  281,  299,  303,  302,
  258,  259,  260,  264,  304,  299,  300,  301,  266,  267,
  268,  269,  270,  262,  265,  302,  258,  259,  260,  302,
  262,  299,  300,  301,  266,  267,  268,  269,  270,  263,
  264,  265,  265,  303,  268,  268,  265,  271,  272,  272,
  294,  299,  300,  301,  299,  263,  280,   37,    6,  302,
  268,  266,  267,  271,  269,  270,  290,  282,  300,  301,
  285,  294,  280,   97,  125,  258,  291,  292,   -1,  303,
  303,  296,  297,  266,  267,  268,  269,  270,  303,   -1,
   -1,   -1,  272,   -1,  274,   -1,  276,  277,  278,  279,
  277,   -1,   -1,  283,  284,   -1,   -1,  287,  288,  289,
  290,  277,   -1,  293,  294,  295,  294,  294,  298,  299,
   -1,   -1,  302,  303,   -1,  302,   -1,  272,  294,  274,
   -1,  276,   -1,  278,  279,   -1,  302,   -1,  283,  284,
   -1,   -1,  287,  288,  289,  290,   -1,   -1,  293,  294,
  295,   -1,   -1,  298,  299,  272,   -1,  274,  303,  276,
   -1,  278,  279,   -1,   -1,   -1,  283,  284,   -1,   -1,
  287,  288,  289,  290,   -1,   -1,  293,   -1,  295,   -1,
   -1,  298,  299,  272,   -1,  274,  303,  276,   -1,  278,
  279,   -1,   -1,   -1,  283,  284,   -1,   -1,  287,  288,
  289,  290,   -1,   -1,  293,   -1,  295,   -1,   -1,  298,
  299,  258,  259,  260,  303,   -1,   -1,   -1,   -1,  266,
  267,  268,  269,  270,
};
#define YYFINAL 17
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 304
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? (YYMAXTOKEN + 1) : (a))
#if YYDEBUG
static const char *yyname[] = {

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
"STRING","illegal-symbol",
};
static const char *yyrule[] = {
"$accept : run",
"run :",
"run : rules",
"block : null",
"block : rules",
"rules : rule",
"rules : rule rules",
"rules : LOCAL_t list assign_list_opt _SEMIC_t block",
"null :",
"assign_list_opt : _EQUALS_t list",
"assign_list_opt : null",
"arglist_opt : _LPAREN_t lol _RPAREN_t",
"arglist_opt :",
"local_opt : LOCAL_t",
"local_opt :",
"rule : _LBRACE_t block _RBRACE_t",
"rule : INCLUDE_t list _SEMIC_t",
"rule : ARG lol _SEMIC_t",
"rule : arg assign list _SEMIC_t",
"rule : arg ON_t list assign list _SEMIC_t",
"rule : RETURN_t list _SEMIC_t",
"rule : BREAK_t _SEMIC_t",
"rule : CONTINUE_t _SEMIC_t",
"rule : FOR_t local_opt ARG IN_t list _LBRACE_t block _RBRACE_t",
"rule : SWITCH_t list _LBRACE_t cases _RBRACE_t",
"rule : IF_t expr _LBRACE_t block _RBRACE_t",
"rule : MODULE_t list _LBRACE_t block _RBRACE_t",
"rule : CLASS_t lol _LBRACE_t block _RBRACE_t",
"rule : WHILE_t expr _LBRACE_t block _RBRACE_t",
"rule : IF_t expr _LBRACE_t block _RBRACE_t ELSE_t rule",
"rule : local_opt RULE_t ARG arglist_opt rule",
"rule : ON_t arg rule",
"$$1 :",
"$$2 :",
"rule : ACTIONS_t eflags ARG bindlist _LBRACE_t $$1 STRING $$2 _RBRACE_t",
"assign : _EQUALS_t",
"assign : _PLUS_EQUALS_t",
"assign : _QUESTION_EQUALS_t",
"assign : DEFAULT_t _EQUALS_t",
"expr : arg",
"expr : expr _EQUALS_t expr",
"expr : expr _BANG_EQUALS_t expr",
"expr : expr _LANGLE_t expr",
"expr : expr _LANGLE_EQUALS_t expr",
"expr : expr _RANGLE_t expr",
"expr : expr _RANGLE_EQUALS_t expr",
"expr : expr _AMPER_t expr",
"expr : expr _AMPERAMPER_t expr",
"expr : expr _BAR_t expr",
"expr : expr _BARBAR_t expr",
"expr : arg IN_t list",
"expr : _BANG_t expr",
"expr : _LPAREN_t expr _RPAREN_t",
"cases :",
"cases : case cases",
"case : CASE_t ARG _COLON_t block",
"lol : list",
"lol : list _COLON_t lol",
"list : listp",
"listp :",
"listp : listp arg",
"arg : ARG",
"$$3 :",
"arg : _LBRACKET_t $$3 func _RBRACKET_t",
"func : ARG lol",
"func : ON_t arg ARG lol",
"func : ON_t arg RETURN_t list",
"eflags :",
"eflags : eflags eflag",
"eflag : UPDATED_t",
"eflag : TOGETHER_t",
"eflag : IGNORE_t",
"eflag : QUIETLY_t",
"eflag : PIECEMEAL_t",
"eflag : EXISTING_t",
"bindlist :",
"bindlist : BIND_t list",

};
#endif

int      yydebug;
int      yynerrs;

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;

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
    short    *s_base;
    short    *s_mark;
    short    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
/* variables for the parser stack */
static YYSTACKDATA yystack;

#if YYDEBUG
#include <stdio.h>		/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (short *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return -1;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return -1;

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

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack)) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    yyerror("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yystack.s_mark]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
                {
                    goto yyoverflow;
                }
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
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
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
    if (yym)
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
#line 168 "jamgram.y"
	{ yyval.parse = plocal( yystack.l_mark[-3].parse, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 8:
#line 172 "jamgram.y"
	{ yyval.parse = pnull(); }
break;
case 9:
#line 176 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; yyval.number = ASSIGN_SET; }
break;
case 10:
#line 178 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; yyval.number = ASSIGN_APPEND; }
break;
case 11:
#line 182 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; }
break;
case 12:
#line 184 "jamgram.y"
	{ yyval.parse = P0; }
break;
case 13:
#line 188 "jamgram.y"
	{ yyval.number = 1; }
break;
case 14:
#line 190 "jamgram.y"
	{ yyval.number = 0; }
break;
case 15:
#line 194 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; }
break;
case 16:
#line 196 "jamgram.y"
	{ yyval.parse = pincl( yystack.l_mark[-1].parse ); }
break;
case 17:
#line 198 "jamgram.y"
	{ yyval.parse = prule( yystack.l_mark[-2].string, yystack.l_mark[-1].parse ); }
break;
case 18:
#line 200 "jamgram.y"
	{ yyval.parse = pset( yystack.l_mark[-3].parse, yystack.l_mark[-1].parse, yystack.l_mark[-2].number ); }
break;
case 19:
#line 202 "jamgram.y"
	{ yyval.parse = pset1( yystack.l_mark[-5].parse, yystack.l_mark[-3].parse, yystack.l_mark[-1].parse, yystack.l_mark[-2].number ); }
break;
case 20:
#line 204 "jamgram.y"
	{ yyval.parse = preturn( yystack.l_mark[-1].parse ); }
break;
case 21:
#line 206 "jamgram.y"
	{ yyval.parse = pbreak(); }
break;
case 22:
#line 208 "jamgram.y"
	{ yyval.parse = pcontinue(); }
break;
case 23:
#line 210 "jamgram.y"
	{ yyval.parse = pfor( yystack.l_mark[-5].string, yystack.l_mark[-3].parse, yystack.l_mark[-1].parse, yystack.l_mark[-6].number ); }
break;
case 24:
#line 212 "jamgram.y"
	{ yyval.parse = pswitch( yystack.l_mark[-3].parse, yystack.l_mark[-1].parse ); }
break;
case 25:
#line 214 "jamgram.y"
	{ yyval.parse = pif( yystack.l_mark[-3].parse, yystack.l_mark[-1].parse, pnull() ); }
break;
case 26:
#line 216 "jamgram.y"
	{ yyval.parse = pmodule( yystack.l_mark[-3].parse, yystack.l_mark[-1].parse ); }
break;
case 27:
#line 218 "jamgram.y"
	{ yyval.parse = pclass( yystack.l_mark[-3].parse, yystack.l_mark[-1].parse ); }
break;
case 28:
#line 220 "jamgram.y"
	{ yyval.parse = pwhile( yystack.l_mark[-3].parse, yystack.l_mark[-1].parse ); }
break;
case 29:
#line 222 "jamgram.y"
	{ yyval.parse = pif( yystack.l_mark[-5].parse, yystack.l_mark[-3].parse, yystack.l_mark[0].parse ); }
break;
case 30:
#line 224 "jamgram.y"
	{ yyval.parse = psetc( yystack.l_mark[-2].string, yystack.l_mark[0].parse, yystack.l_mark[-1].parse, yystack.l_mark[-4].number ); }
break;
case 31:
#line 226 "jamgram.y"
	{ yyval.parse = pon( yystack.l_mark[-1].parse, yystack.l_mark[0].parse ); }
break;
case 32:
#line 228 "jamgram.y"
	{ yymode( SCAN_STRING ); }
break;
case 33:
#line 230 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 34:
#line 232 "jamgram.y"
	{ yyval.parse = psete( yystack.l_mark[-6].string,yystack.l_mark[-5].parse,yystack.l_mark[-2].string,yystack.l_mark[-7].number ); }
break;
case 35:
#line 240 "jamgram.y"
	{ yyval.number = ASSIGN_SET; }
break;
case 36:
#line 242 "jamgram.y"
	{ yyval.number = ASSIGN_APPEND; }
break;
case 37:
#line 244 "jamgram.y"
	{ yyval.number = ASSIGN_DEFAULT; }
break;
case 38:
#line 246 "jamgram.y"
	{ yyval.number = ASSIGN_DEFAULT; }
break;
case 39:
#line 253 "jamgram.y"
	{ yyval.parse = peval( EXPR_EXISTS, yystack.l_mark[0].parse, pnull() ); }
break;
case 40:
#line 255 "jamgram.y"
	{ yyval.parse = peval( EXPR_EQUALS, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 41:
#line 257 "jamgram.y"
	{ yyval.parse = peval( EXPR_NOTEQ, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 42:
#line 259 "jamgram.y"
	{ yyval.parse = peval( EXPR_LESS, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 43:
#line 261 "jamgram.y"
	{ yyval.parse = peval( EXPR_LESSEQ, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 44:
#line 263 "jamgram.y"
	{ yyval.parse = peval( EXPR_MORE, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 45:
#line 265 "jamgram.y"
	{ yyval.parse = peval( EXPR_MOREEQ, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 46:
#line 267 "jamgram.y"
	{ yyval.parse = peval( EXPR_AND, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 47:
#line 269 "jamgram.y"
	{ yyval.parse = peval( EXPR_AND, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 48:
#line 271 "jamgram.y"
	{ yyval.parse = peval( EXPR_OR, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 49:
#line 273 "jamgram.y"
	{ yyval.parse = peval( EXPR_OR, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 50:
#line 275 "jamgram.y"
	{ yyval.parse = peval( EXPR_IN, yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 51:
#line 277 "jamgram.y"
	{ yyval.parse = peval( EXPR_NOT, yystack.l_mark[0].parse, pnull() ); }
break;
case 52:
#line 279 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; }
break;
case 53:
#line 290 "jamgram.y"
	{ yyval.parse = P0; }
break;
case 54:
#line 292 "jamgram.y"
	{ yyval.parse = pnode( yystack.l_mark[-1].parse, yystack.l_mark[0].parse ); }
break;
case 55:
#line 296 "jamgram.y"
	{ yyval.parse = psnode( yystack.l_mark[-2].string, yystack.l_mark[0].parse ); }
break;
case 56:
#line 305 "jamgram.y"
	{ yyval.parse = pnode( P0, yystack.l_mark[0].parse ); }
break;
case 57:
#line 307 "jamgram.y"
	{ yyval.parse = pnode( yystack.l_mark[0].parse, yystack.l_mark[-2].parse ); }
break;
case 58:
#line 317 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; yymode( SCAN_NORMAL ); }
break;
case 59:
#line 321 "jamgram.y"
	{ yyval.parse = pnull(); yymode( SCAN_PUNCT ); }
break;
case 60:
#line 323 "jamgram.y"
	{ yyval.parse = pappend( yystack.l_mark[-1].parse, yystack.l_mark[0].parse ); }
break;
case 61:
#line 327 "jamgram.y"
	{ yyval.parse = plist( yystack.l_mark[0].string ); }
break;
case 62:
#line 328 "jamgram.y"
	{ yymode( SCAN_NORMAL ); }
break;
case 63:
#line 329 "jamgram.y"
	{ yyval.parse = yystack.l_mark[-1].parse; }
break;
case 64:
#line 338 "jamgram.y"
	{ yyval.parse = prule( yystack.l_mark[-1].string, yystack.l_mark[0].parse ); }
break;
case 65:
#line 340 "jamgram.y"
	{ yyval.parse = pon( yystack.l_mark[-2].parse, prule( yystack.l_mark[-1].string, yystack.l_mark[0].parse ) ); }
break;
case 66:
#line 342 "jamgram.y"
	{ yyval.parse = pon( yystack.l_mark[-2].parse, yystack.l_mark[0].parse ); }
break;
case 67:
#line 352 "jamgram.y"
	{ yyval.number = 0; }
break;
case 68:
#line 354 "jamgram.y"
	{ yyval.number = yystack.l_mark[-1].number | yystack.l_mark[0].number; }
break;
case 69:
#line 358 "jamgram.y"
	{ yyval.number = EXEC_UPDATED; }
break;
case 70:
#line 360 "jamgram.y"
	{ yyval.number = EXEC_TOGETHER; }
break;
case 71:
#line 362 "jamgram.y"
	{ yyval.number = EXEC_IGNORE; }
break;
case 72:
#line 364 "jamgram.y"
	{ yyval.number = EXEC_QUIETLY; }
break;
case 73:
#line 366 "jamgram.y"
	{ yyval.number = EXEC_PIECEMEAL; }
break;
case 74:
#line 368 "jamgram.y"
	{ yyval.number = EXEC_EXISTING; }
break;
case 75:
#line 377 "jamgram.y"
	{ yyval.parse = pnull(); }
break;
case 76:
#line 379 "jamgram.y"
	{ yyval.parse = yystack.l_mark[0].parse; }
break;
#line 961 "y.tab.c"
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
            if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = yyname[YYTRANSLATE(yychar)];
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (short) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    yyerror("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
