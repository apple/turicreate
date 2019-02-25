%{
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C and modified to build everywhere.

Run bison like this:

  bison --yacc --name-prefix=cmExpr_yy --defines=cmExprParserTokens.h -ocmExprParser.cxx cmExprParser.y

Modify cmExprParser.cxx:
  - "#if 0" out yyerrorlab block in range ["goto yyerrlab1", "yyerrlab1:"]

*/

#include "cmConfigure.h" // IWYU pragma: keep

#include <stdlib.h>
#include <string.h>
#include <stdexcept>

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
#if defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic ignored "-Wconversion"
#endif
%}

/* Generate a reentrant parser object.  */
/* %define api.pure */

/* Configure the parser to use a lexer object.  */
%lex-param   {yyscan_t yyscanner}
%parse-param {yyscan_t yyscanner}

/* %define parse.error verbose */

/*-------------------------------------------------------------------------*/
/* Tokens */
%token exp_PLUS
%token exp_MINUS
%token exp_TIMES
%token exp_DIVIDE
%token exp_MOD
%token exp_SHIFTLEFT
%token exp_SHIFTRIGHT
%token exp_OPENPARENT
%token exp_CLOSEPARENT
%token exp_OR;
%token exp_AND;
%token exp_XOR;
%token exp_NOT;
%token exp_NUMBER;

/*-------------------------------------------------------------------------*/
/* grammar */
%%


start:
  exp {
    cmExpr_yyget_extra(yyscanner)->SetResult($<Number>1);
  }

exp:
  bitwiseor {
    $<Number>$ = $<Number>1;
  }
| exp exp_OR bitwiseor {
    $<Number>$ = $<Number>1 | $<Number>3;
  }

bitwiseor:
  bitwisexor {
    $<Number>$ = $<Number>1;
  }
| bitwiseor exp_XOR bitwisexor {
    $<Number>$ = $<Number>1 ^ $<Number>3;
  }

bitwisexor:
  bitwiseand {
    $<Number>$ = $<Number>1;
  }
| bitwisexor exp_AND bitwiseand {
    $<Number>$ = $<Number>1 & $<Number>3;
  }

bitwiseand:
  shift {
    $<Number>$ = $<Number>1;
  }
| bitwiseand exp_SHIFTLEFT shift {
    $<Number>$ = $<Number>1 << $<Number>3;
  }
| bitwiseand exp_SHIFTRIGHT shift {
    $<Number>$ = $<Number>1 >> $<Number>3;
  }

shift:
  term {
    $<Number>$ = $<Number>1;
  }
| shift exp_PLUS term {
    $<Number>$ = $<Number>1 + $<Number>3;
  }
| shift exp_MINUS term {
    $<Number>$ = $<Number>1 - $<Number>3;
  }

term:
  unary {
    $<Number>$ = $<Number>1;
  }
| term exp_TIMES unary {
    $<Number>$ = $<Number>1 * $<Number>3;
  }
| term exp_DIVIDE unary {
    if (yyvsp[0].Number == 0) {
      throw std::overflow_error("divide by zero");
    }
    $<Number>$ = $<Number>1 / $<Number>3;
  }
| term exp_MOD unary {
    $<Number>$ = $<Number>1 % $<Number>3;
  }

unary:
  factor {
    $<Number>$ = $<Number>1;
  }
| exp_PLUS unary {
    $<Number>$ = + $<Number>2;
  }
| exp_MINUS unary {
    $<Number>$ = - $<Number>2;
  }

factor:
  exp_NUMBER {
    $<Number>$ = $<Number>1;
  }
| exp_OPENPARENT exp exp_CLOSEPARENT {
    $<Number>$ = $<Number>2;
  }
;

%%
/* End of grammar */

/*--------------------------------------------------------------------------*/
void cmExpr_yyerror(yyscan_t yyscanner, const char* message)
{
  cmExpr_yyget_extra(yyscanner)->Error(message);
}
