/****************************************************************************
 * Copyright (c) 1998 Free Software Foundation, Inc.                        *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *   Author: Juergen Pfeifer <juergen.pfeifer@gmx.net> 1995,1997            *
 ****************************************************************************/

#include "form.priv.h"

MODULE_ID("$Id$")

extern int winnstr(WINDOW *, char *, int);

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  bool data_behind(const FORM *form)
|   
|   Description   :  Check for off-screen data behind. This is nearly trivial
|                    becose the begin of a field is fixed.
|
|   Return Values :  TRUE   - there are off-screen data behind
|                    FALSE  - there are no off-screen data behind
+--------------------------------------------------------------------------*/
bool data_behind(const FORM *form)
{
  bool result = FALSE;

  if (form && (form->status & _POSTED) && form->current)
    {
      FIELD *field;

      field = form->current;
      if (!Single_Line_Field(field))
	{
	  result = (form->toprow==0) ? FALSE : TRUE;
	}
      else
	{
	  result = (form->begincol==0) ? FALSE : TRUE;
	}
    }
  return(result);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static char * After_Last_Non_Pad_Position(
|                                    char *buffer,
|                                    int len,
|                                    int pad)
|   
|   Description   :  Find the last position in the buffer that doesn't
|                    contain a padding character.
|
|   Return Values :  The pointer to this position 
+--------------------------------------------------------------------------*/
INLINE 
static char * After_Last_Non_Pad_Position(char *buffer, int len, int pad)
{
  char *end = buffer + len;

  assert(buffer && len>=0);
  while ( (buffer < end) && (*(end-1)==pad) )
    end--;

  return end;
}

#define SMALL_BUFFER_SIZE (80)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  bool data_ahead(const FORM *form)
|   
|   Description   :  Check for off-screen data ahead. This is more difficult
|                    because a dynamic field has a variable end. 
|
|   Return Values :  TRUE   - there are off-screen data ahead
|                    FALSE  - there are no off-screen data ahead
+--------------------------------------------------------------------------*/
bool data_ahead(const FORM *form)
{
  bool result = FALSE;

  if (form && (form->status & _POSTED) && form->current)
    {
      static char buffer[SMALL_BUFFER_SIZE + 1];
      FIELD *field;
      bool large_buffer;
      bool cursor_moved = FALSE;
      char *bp;
      char *found_content;
      int pos;

      field = form->current;
      assert(form->w != 0);

      large_buffer = (field->cols > SMALL_BUFFER_SIZE);
      if (large_buffer)
	bp = (char *)malloc((size_t)(field->cols) + 1);
      else
	bp = buffer;

      assert(bp != 0);
      
      if (Single_Line_Field(field))
	{
	  int check_len;

	  pos = form->begincol + field->cols;
	  while (pos < field->dcols)
	    {
	      check_len = field->dcols - pos;
	      if ( check_len >= field->cols )
		check_len = field->cols;
	      cursor_moved = TRUE;
	      wmove(form->w,0,pos);
	      winnstr(form->w,bp,check_len);
	      found_content = 
		After_Last_Non_Pad_Position(bp,check_len,field->pad);
	      if (found_content==bp)
		  pos += field->cols;		  
	      else
		{
		  result = TRUE;
		  break;
		}
	    }
	}
      else
	{
	  pos = form->toprow + field->rows;
	  while (pos < field->drows)
	    {
	      cursor_moved = TRUE;
	      wmove(form->w,pos,0);
	      pos++;
	      winnstr(form->w,bp,field->cols);
	      found_content = 
		After_Last_Non_Pad_Position(bp,field->cols,field->pad);
	      if (found_content!=bp)
		{
		  result = TRUE;
		  break;
		}
	    }
	}

      if (large_buffer)
	free(bp);

      if (cursor_moved)
	wmove(form->w,form->currow,form->curcol);
    }
  return(result);
}

/* frm_data.c ends here */
