
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
} alphaARG;

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void *Make_Alpha_Type(va_list *ap)
|   
|   Description   :  Allocate structure for alpha type argument.
|
|   Return Values :  Pointer to argument structure or NULL on error
+--------------------------------------------------------------------------*/
static void *Make_Alpha_Type(va_list * ap)
{
  alphaARG *argp = (alphaARG *)malloc(sizeof(alphaARG));
  if (argp)
    {
      argp->width = va_arg(*ap,int);
    }
  return ((void *)argp);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void *Copy_Alpha_Type(const void * argp)
|   
|   Description   :  Copy structure for alpha type argument.  
|
|   Return Values :  Pointer to argument structure or NULL on error.
+--------------------------------------------------------------------------*/
static void *Copy_Alpha_Type(const void * argp)
{
  const alphaARG *ap = (const alphaARG *)argp;
  alphaARG *result = (alphaARG *)malloc(sizeof(alphaARG));
  
  if (result)
    {
      *result = *ap;
    }
  return ((void *)result);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Free_Alpha_Type( void * argp )
|   
|   Description   :  Free structure for alpha type argument.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Free_Alpha_Type(void * argp)
{
  if (argp) 
    free(argp);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_Alpha_Field(
|                                      FIELD * field,
|                                      const void * argp)
|   
|   Description   :  Validate buffer content to be a valid alpha value
|
|   Return Values :  TRUE  - field is valid
|                    FALSE - field is invalid
+--------------------------------------------------------------------------*/
static bool Check_Alpha_Field(FIELD * field, const void * argp)
{
  int width = ((const alphaARG *)argp)->width;
  unsigned char *bp  = (unsigned char *)field_buffer(field,0);
  int  l = -1;
  unsigned char *s;

  while(*bp && *bp==' ') 
    bp++;
  if (*bp)
    {
      s = bp;
      while(*bp && isalpha(*bp)) 
	bp++;
      l = (int)(bp-s);
      while(*bp && *bp==' ') 
	bp++;
    }
  return ((*bp || (l < width)) ? FALSE : TRUE);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_Alpha_Character(
|                                      int c,
|                                      const void * argp)
|   
|   Description   :  Check a character for the alpha type.
|
|   Return Values :  TRUE  - character is valid
|                    FALSE - character is invalid
+--------------------------------------------------------------------------*/
static bool Check_Alpha_Character(int c, const void * argp)
{
  argp=0; /* Silence unused parameter warning.  */
  return (isalpha(c) ? TRUE : FALSE);
}

static FIELDTYPE typeALPHA = {
  _HAS_ARGS | _RESIDENT,
  1,                           /* this is mutable, so we can't be const */
  (FIELDTYPE *)0,
  (FIELDTYPE *)0,
  Make_Alpha_Type,
  Copy_Alpha_Type,
  Free_Alpha_Type,
  Check_Alpha_Field,
  Check_Alpha_Character,
  NULL,
  NULL
};

FIELDTYPE* TYPE_ALPHA = &typeALPHA;

/* fty_alpha.c ends here */
