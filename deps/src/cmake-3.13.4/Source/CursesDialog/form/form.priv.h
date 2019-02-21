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

#include "mf_common.h"
#include "form.h"

/* form  status values */
#define _OVLMODE         (0x04) /* Form is in overlay mode                */
#define _WINDOW_MODIFIED (0x10) /* Current field window has been modified */
#define _FCHECK_REQUIRED (0x20) /* Current field needs validation         */

/* field status values */
#define _CHANGED         (0x01) /* Field has been changed                 */
#define _NEWTOP          (0x02) /* Vertical scrolling occurred             */
#define _NEWPAGE         (0x04) /* field begins new page of form          */
#define _MAY_GROW        (0x08) /* dynamic field may still grow           */

/* fieldtype status values */
#define _LINKED_TYPE     (0x01) /* Type is a linked type                  */
#define _HAS_ARGS        (0x02) /* Type has arguments                     */
#define _HAS_CHOICE      (0x04) /* Type has choice methods                */
#define _RESIDENT        (0x08) /* Type is builtin                        */

/* This are the field options required to be a selectable field in field
   navigation requests */
#define O_SELECTABLE (O_ACTIVE | O_VISIBLE)

/* If form is NULL replace form argument by default-form */
#define Normalize_Form(form)  ((form)=(form)?(form):_nc_Default_Form)

/* If field is NULL replace field argument by default-field */
#define Normalize_Field(field)  ((field)=(field)?(field):_nc_Default_Field)

/* Retrieve forms window */
#define Get_Form_Window(form) \
  ((form)->sub?(form)->sub:((form)->win?(form)->win:stdscr))

/* Calculate the size for a single buffer for this field */
#define Buffer_Length(field) ((field)->drows * (field)->dcols)

/* Calculate the total size of all buffers for this field */
#define Total_Buffer_Size(field) \
   ( (Buffer_Length(field) + 1) * (1+(field)->nbuf) )

/* Logic to determine whether or not a field is single lined */
#define Single_Line_Field(field) \
   (((field)->rows + (field)->nrow) == 1)

/* Logic to determine whether or not a field is selectable */
#define Field_Is_Selectable(f)     (((f)->opts & O_SELECTABLE)==O_SELECTABLE)
#define Field_Is_Not_Selectable(f) (((f)->opts & O_SELECTABLE)!=O_SELECTABLE)

typedef struct typearg {
  struct typearg *left;
  struct typearg *right;
} TypeArgument;

/* This is a dummy request code (normally invalid) to be used internally
   with the form_driver() routine to position to the first active field
   on the form
*/
#define FIRST_ACTIVE_MAGIC (-291056)

#define ALL_FORM_OPTS  (                \
                        O_NL_OVERLOAD  |\
                        O_BS_OVERLOAD   )

#define ALL_FIELD_OPTS (           \
                        O_VISIBLE |\
                        O_ACTIVE  |\
                        O_PUBLIC  |\
                        O_EDIT    |\
                        O_WRAP    |\
                        O_BLANK   |\
                        O_AUTOSKIP|\
                        O_NULLOK  |\
                        O_PASSOK  |\
                        O_STATIC   )


#define C_BLANK ' '
#define is_blank(c) ((c)==C_BLANK)

extern const FIELDTYPE* _nc_Default_FieldType;

extern TypeArgument* _nc_Make_Argument(const FIELDTYPE*,va_list*,int*);
extern TypeArgument *_nc_Copy_Argument(const FIELDTYPE*,const TypeArgument*, int*);
extern void _nc_Free_Argument(const FIELDTYPE*,TypeArgument*);
extern bool _nc_Copy_Type(FIELD*, FIELD const *);
extern void _nc_Free_Type(FIELD *);

extern int _nc_Synchronize_Attributes(FIELD*);
extern int _nc_Synchronize_Options(FIELD*,Field_Options);
extern int _nc_Set_Form_Page(FORM*,int,FIELD*);
extern int _nc_Refresh_Current_Field(FORM*);
extern FIELD* _nc_First_Active_Field(FORM*);
extern bool _nc_Internal_Validation(FORM*);
extern int _nc_Set_Current_Field(FORM*,FIELD*);
extern int _nc_Position_Form_Cursor(FORM*);
