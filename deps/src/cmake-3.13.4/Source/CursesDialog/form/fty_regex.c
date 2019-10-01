
/*
 * THIS CODE IS SPECIFICALLY EXEMPTED FROM THE NCURSES PACKAGE COPYRIGHT.
 * You may freely copy it for use as a template for your own field types.
 * If you develop a field type that might be of general use, please send
 * it back to the ncurses maintainers for inclusion in the next version.
 */
/***************************************************************************
*                                                                          *
*  Author : Juergen Pfeifer, juergen.pfeifer@gmx.net                       *
*                                                                          *
***************************************************************************/

#include "form.priv.h"

MODULE_ID("$Id$")

#if HAVE_REGEX_H_FUNCS	/* We prefer POSIX regex */
#include <regex.h>

typedef struct
{
  regex_t *pRegExp;
  unsigned long *refCount;
} RegExp_Arg;

#elif HAVE_REGEXP_H_FUNCS | HAVE_REGEXPR_H_FUNCS
#undef RETURN
static int reg_errno;

static char *RegEx_Init(char *instring)
{
	reg_errno = 0;
	return instring;
}

static char *RegEx_Error(int code)
{
	reg_errno = code;
	return 0;
}

#define INIT 		char *sp = RegEx_Init(instring);
#define GETC()		(*sp++)
#define PEEKC()		(*sp)
#define UNGETC(c)	(--sp)
#define RETURN(c)	return(c)
#define ERROR(c)	return RegEx_Error(c)

#if HAVE_REGEXP_H_FUNCS
#include <regexp.h>
#else
#include <regexpr.h>
#endif

typedef struct
{
  char *compiled_expression;
  unsigned long *refCount;
} RegExp_Arg;

/* Maximum Length we allow for a compiled regular expression */
#define MAX_RX_LEN   (2048)
#define RX_INCREMENT (256)

#endif

/*---------------------------------------------------------------------------
|   Facility      :  libnform
|   Function      :  static void *Make_RegularExpression_Type(va_list * ap)
|
|   Description   :  Allocate structure for regex type argument.
|
|   Return Values :  Pointer to argument structure or NULL on error
+--------------------------------------------------------------------------*/
static void *Make_RegularExpression_Type(va_list * ap)
{
#if HAVE_REGEX_H_FUNCS
  char *rx = va_arg(*ap,char *);
  RegExp_Arg *preg;

  preg = (RegExp_Arg*)malloc(sizeof(RegExp_Arg));
  if (preg)
    {
      if (((preg->pRegExp = (regex_t*)malloc(sizeof(regex_t))) != (regex_t*)0)
       && !regcomp(preg->pRegExp,rx,
		   (REG_EXTENDED | REG_NOSUB | REG_NEWLINE) ))
	{
	  preg->refCount = (unsigned long *)malloc(sizeof(unsigned long));
	  *(preg->refCount) = 1;
	}
      else
	{
	  if (preg->pRegExp)
	    free(preg->pRegExp);
	  free(preg);
	  preg = (RegExp_Arg*)0;
	}
    }
  return((void *)preg);
#elif HAVE_REGEXP_H_FUNCS | HAVE_REGEXPR_H_FUNCS
  char *rx = va_arg(*ap,char *);
  RegExp_Arg *pArg;

  pArg = (RegExp_Arg *)malloc(sizeof(RegExp_Arg));

  if (pArg)
    {
      int blen = RX_INCREMENT;
      pArg->compiled_expression = NULL;
      pArg->refCount = (unsigned long *)malloc(sizeof(unsigned long));
      *(pArg->refCount) = 1;

      do {
	char *buf = (char *)malloc(blen);
	if (buf)
	  {
#if HAVE_REGEXP_H_FUNCS
	    char *last_pos = compile (rx, buf, &buf[blen], '\0');
#else /* HAVE_REGEXPR_H_FUNCS */
	    char *last_pos = compile (rx, buf, &buf[blen]);
#endif
	    if (reg_errno)
	      {
		free(buf);
		if (reg_errno==50)
		  blen += RX_INCREMENT;
		else
		  {
		    free(pArg);
		    pArg = NULL;
		    break;
		  }
	      }
	    else
	      {
		pArg->compiled_expression = buf;
		break;
	      }
	  }
      } while( blen <= MAX_RX_LEN );
    }
  if (pArg && !pArg->compiled_expression)
    {
      free(pArg);
      pArg = NULL;
    }
  return (void *)pArg;
#else
  ap=0; /* Silence unused parameter warning.  */
  return 0;
#endif
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform
|   Function      :  static void *Copy_RegularExpression_Type(
|                                      const void * argp)
|
|   Description   :  Copy structure for regex type argument.
|
|   Return Values :  Pointer to argument structure or NULL on error.
+--------------------------------------------------------------------------*/
static void *Copy_RegularExpression_Type(const void * argp)
{
#if (HAVE_REGEX_H_FUNCS | HAVE_REGEXP_H_FUNCS | HAVE_REGEXPR_H_FUNCS)
  const RegExp_Arg *ap = (const RegExp_Arg *)argp;
  const RegExp_Arg *result = (const RegExp_Arg *)0;

  if (ap)
    {
      *(ap->refCount) += 1;
      result = ap;
    }
  return (void *)result;
#else
  argp=0; /* Silence unused parameter warning.  */
  return 0;
#endif
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform
|   Function      :  static void Free_RegularExpression_Type(void * argp)
|
|   Description   :  Free structure for regex type argument.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Free_RegularExpression_Type(void * argp)
{
#if HAVE_REGEX_H_FUNCS | HAVE_REGEXP_H_FUNCS | HAVE_REGEXPR_H_FUNCS
  RegExp_Arg *ap = (RegExp_Arg *)argp;
  if (ap)
    {
      if (--(*(ap->refCount)) == 0)
	{
#if HAVE_REGEX_H_FUNCS
	  if (ap->pRegExp)
	    {
	      free(ap->refCount);
	      regfree(ap->pRegExp);
	    }
#elif HAVE_REGEXP_H_FUNCS | HAVE_REGEXPR_H_FUNCS
	  if (ap->compiled_expression)
	    {
	      free(ap->refCount);
	      free(ap->compiled_expression);
	    }
#endif
	  free(ap);
	}
    }
#else
  argp=0; /* Silence unused parameter warning.  */
#endif
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform
|   Function      :  static bool Check_RegularExpression_Field(
|                                      FIELD * field,
|                                      const void  * argp)
|
|   Description   :  Validate buffer content to be a valid regular expression
|
|   Return Values :  TRUE  - field is valid
|                    FALSE - field is invalid
+--------------------------------------------------------------------------*/
static bool Check_RegularExpression_Field(FIELD * field, const void  * argp)
{
  bool match = FALSE;
#if HAVE_REGEX_H_FUNCS
  const RegExp_Arg *ap = (const RegExp_Arg*)argp;
  if (ap && ap->pRegExp)
    match = (regexec(ap->pRegExp,field_buffer(field,0),0,NULL,0) ? FALSE:TRUE);
#elif HAVE_REGEXP_H_FUNCS | HAVE_REGEXPR_H_FUNCS
  RegExp_Arg *ap = (RegExp_Arg *)argp;
  if (ap && ap->compiled_expression)
    match = (step(field_buffer(field,0),ap->compiled_expression) ? TRUE:FALSE);
#else
  argp=0;  /* Silence unused parameter warning.  */
  field=0; /* Silence unused parameter warning.  */
#endif
  return match;
}

static FIELDTYPE typeREGEXP = {
  _HAS_ARGS | _RESIDENT,
  1,                           /* this is mutable, so we can't be const */
  (FIELDTYPE *)0,
  (FIELDTYPE *)0,
  Make_RegularExpression_Type,
  Copy_RegularExpression_Type,
  Free_RegularExpression_Type,
  Check_RegularExpression_Field,
  NULL,
  NULL,
  NULL
};

FIELDTYPE* TYPE_REGEXP = &typeREGEXP;

/* fty_regex.c ends here */
