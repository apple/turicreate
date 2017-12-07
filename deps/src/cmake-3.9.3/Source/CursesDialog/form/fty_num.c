
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

#if HAVE_LOCALE_H
#include <locale.h>
#endif

typedef struct {
  int    precision;
  double low;
  double high;
  struct lconv* L;
} numericARG;

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void *Make_Numeric_Type(va_list * ap)
|   
|   Description   :  Allocate structure for numeric type argument.
|
|   Return Values :  Pointer to argument structure or NULL on error
+--------------------------------------------------------------------------*/
static void *Make_Numeric_Type(va_list * ap)
{
  numericARG *argn = (numericARG *)malloc(sizeof(numericARG));

  if (argn)
    {
      argn->precision = va_arg(*ap,int);
      argn->low       = va_arg(*ap,double);
      argn->high      = va_arg(*ap,double);
#if HAVE_LOCALE_H
      argn->L         = localeconv();
#else
      argn->L         = NULL;
#endif
    }
  return (void *)argn;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void *Copy_Numeric_Type(const void * argp)
|   
|   Description   :  Copy structure for numeric type argument.  
|
|   Return Values :  Pointer to argument structure or NULL on error.
+--------------------------------------------------------------------------*/
static void *Copy_Numeric_Type(const void * argp)
{
  const numericARG *ap = (const numericARG *)argp;
  numericARG *result = (numericARG *)0;

  if (argp)
    {
      result = (numericARG *)malloc(sizeof(numericARG));
      if (result)
	*result  = *ap;
    }
  return (void *)result;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Free_Numeric_Type(void * argp)
|   
|   Description   :  Free structure for numeric type argument.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Free_Numeric_Type(void * argp)
{
  if (argp) 
    free(argp);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_Numeric_Field(FIELD * field,
|                                                    const void * argp)
|   
|   Description   :  Validate buffer content to be a valid numeric value
|
|   Return Values :  TRUE  - field is valid
|                    FALSE - field is invalid
+--------------------------------------------------------------------------*/
static bool Check_Numeric_Field(FIELD * field, const void * argp)
{
  const numericARG *argn = (const numericARG *)argp;
  double low          = argn->low;
  double high         = argn->high;
  int prec            = argn->precision;
  unsigned char *bp   = (unsigned char *)field_buffer(field,0);
  char *s             = (char *)bp;
  double val          = 0.0;
  char buf[64];

  while(*bp && *bp==' ') bp++;
  if (*bp)
    {
      if (*bp=='-' || *bp=='+')
	bp++;
      while(*bp)
	{
	  if (!isdigit(*bp)) break;
	  bp++;
	}
      if (*bp==(
#if HAVE_LOCALE_H
		(L && L->decimal_point) ? *(L->decimal_point) :
#endif
		'.'))
	{
	  bp++;
	  while(*bp)
	    {
	      if (!isdigit(*bp)) break;
	      bp++;
	    }
	}
      while(*bp && *bp==' ') bp++;
      if (*bp=='\0')
	{
	  val = atof(s);
	  if (low<high)
	    {
	      if (val<low || val>high) return FALSE;
	    }
	  sprintf(buf,"%.*f",(prec>0?prec:0),val);
	  set_field_buffer(field,0,buf);
	  return TRUE;
	}
    }
  return FALSE;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_Numeric_Character(
|                                      int c,
|                                      const void * argp)
|   
|   Description   :  Check a character for the numeric type.
|
|   Return Values :  TRUE  - character is valid
|                    FALSE - character is invalid
+--------------------------------------------------------------------------*/
static bool Check_Numeric_Character(int c, const void * argp)
{
  argp=0; /* Silence unused parameter warning.  */
  return (isdigit(c)  || 
	  c == '+'    || 
	  c == '-'    || 
	  c == (
#if HAVE_LOCALE_H
		(L && L->decimal_point) ? *(L->decimal_point) :
#endif
		'.')
	  ) ? TRUE : FALSE;
}

static FIELDTYPE typeNUMERIC = {
  _HAS_ARGS | _RESIDENT,
  1,                           /* this is mutable, so we can't be const */
  (FIELDTYPE *)0,
  (FIELDTYPE *)0,
  Make_Numeric_Type,
  Copy_Numeric_Type,
  Free_Numeric_Type,
  Check_Numeric_Field,
  Check_Numeric_Character,
  NULL,
  NULL
};

FIELDTYPE* TYPE_NUMERIC = &typeNUMERIC;

/* fty_num.c ends here */
