
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

typedef struct {
  int width;
} alnumARG;

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void *Make_AlphaNumeric_Type(va_list *ap)
|   
|   Description   :  Allocate structure for alphanumeric type argument.
|
|   Return Values :  Pointer to argument structure or NULL on error
+--------------------------------------------------------------------------*/
static void *Make_AlphaNumeric_Type(va_list * ap)
{
  alnumARG *argp = (alnumARG *)malloc(sizeof(alnumARG));

  if (argp)
    argp->width = va_arg(*ap,int);

  return ((void *)argp);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void *Copy_AlphaNumericType(const void *argp)
|   
|   Description   :  Copy structure for alphanumeric type argument.  
|
|   Return Values :  Pointer to argument structure or NULL on error.
+--------------------------------------------------------------------------*/
static void *Copy_AlphaNumeric_Type(const void *argp)
{
  const alnumARG *ap = (const alnumARG *)argp;
  alnumARG *result = (alnumARG *)malloc(sizeof(alnumARG));

  if (result)
    *result = *ap;

  return ((void *)result);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Free_AlphaNumeric_Type(void *argp)
|   
|   Description   :  Free structure for alphanumeric type argument.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Free_AlphaNumeric_Type(void * argp)
{
  if (argp) 
    free(argp);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_AlphaNumeric_Field(
|                                      FIELD *field,
|                                      const void *argp)
|   
|   Description   :  Validate buffer content to be a valid alphanumeric value
|
|   Return Values :  TRUE  - field is valid
|                    FALSE - field is invalid
+--------------------------------------------------------------------------*/
static bool Check_AlphaNumeric_Field(FIELD * field, const void * argp)
{
  int width = ((const alnumARG *)argp)->width;
  unsigned char *bp  = (unsigned char *)field_buffer(field,0);
  int  l = -1;
  unsigned char *s;

  while(*bp && *bp==' ') 
    bp++;
  if (*bp)
    {
      s = bp;
      while(*bp && isalnum(*bp)) 
	bp++;
      l = (int)(bp-s);
      while(*bp && *bp==' ') 
	bp++;
    }
  return ((*bp || (l < width)) ? FALSE : TRUE);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_AlphaNumeric_Character(
|                                      int c, 
|                                      const void *argp )
|   
|   Description   :  Check a character for the alphanumeric type.
|
|   Return Values :  TRUE  - character is valid
|                    FALSE - character is invalid
+--------------------------------------------------------------------------*/
static bool Check_AlphaNumeric_Character(int c, const void * argp)
{
  argp=0; /* Silence unused parameter warning. */
  return (isalnum(c) ? TRUE : FALSE);
}

static FIELDTYPE typeALNUM = {
  _HAS_ARGS | _RESIDENT,
  1,                           /* this is mutable, so we can't be const */
  (FIELDTYPE *)0,
  (FIELDTYPE *)0,
  Make_AlphaNumeric_Type,
  Copy_AlphaNumeric_Type,
  Free_AlphaNumeric_Type,
  Check_AlphaNumeric_Field,
  Check_AlphaNumeric_Character,
  NULL,
  NULL
};

FIELDTYPE* TYPE_ALNUM = &typeALNUM;

/* fty_alnum.c ends here */
