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

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int set_current_field(FORM  * form,FIELD * field)
|   
|   Description   :  Set the current field of the form to the specified one.
|
|   Return Values :  E_OK              - success
|                    E_BAD_ARGUMENT    - invalid form or field pointer
|                    E_REQUEST_DENIED  - field not selectable
|                    E_BAD_STATE       - called from a hook routine
|                    E_INVALID_FIELD   - current field can't be left
|                    E_SYSTEM_ERROR    - system error
+--------------------------------------------------------------------------*/
int set_current_field(FORM  * form, FIELD * field)
{
  int err = E_OK;

  if ( !form || !field )
    RETURN(E_BAD_ARGUMENT);

  if ( (form != field->form) || Field_Is_Not_Selectable(field) )
    RETURN(E_REQUEST_DENIED);

  if (!(form->status & _POSTED))
    {
      form->current = field;
      form->curpage = field->page;
  }
  else
    {
      if (form->status & _IN_DRIVER) 
	err = E_BAD_STATE;
      else
	{
	  if (form->current != field)
	    {
	      if (!_nc_Internal_Validation(form)) 
	       err = E_INVALID_FIELD;
	      else
		{
		  Call_Hook(form,fieldterm);
		  if (field->page != form->curpage)
		    {
		      Call_Hook(form,formterm);
		      err = _nc_Set_Form_Page(form,field->page,field);
		      Call_Hook(form,forminit);
		    } 
		  else 
		    {
		      err = _nc_Set_Current_Field(form,field);
		    }
		  Call_Hook(form,fieldinit);
		  _nc_Refresh_Current_Field(form);
		}
	    }
	}
    }
  RETURN(err);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  FIELD *current_field(const FORM * form)
|   
|   Description   :  Return the current field.
|
|   Return Values :  Pointer to the current field.
+--------------------------------------------------------------------------*/
FIELD *current_field(const FORM * form)
{
  return Normalize_Form(form)->current;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int field_index(const FIELD * field)
|   
|   Description   :  Return the index of the field in the field-array of
|                    the form.
|
|   Return Values :  >= 0   : field index
|                    -1     : fieldpointer invalid or field not connected
+--------------------------------------------------------------------------*/
int field_index(const FIELD * field)
{
  return ( (field && field->form) ? field->index : -1 );
}

/* fld_current.c ends here */
