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

/* AIX seems to define this */
#undef lines
#undef columns

MODULE_ID("$Id$")

/* These declarations are missing from curses.h on some platforms.  */
extern int winnstr(WINDOW *, char *, int);
#if defined(__DECCXX_VER) || (defined(__GNUC__) && defined(__osf__))
extern int waddnstr(WINDOW *,const char *const,int);
extern void wbkgdset(WINDOW *,chtype);
#ifndef untouchwin
extern int untouchwin(WINDOW *);
#endif
extern void wcursyncup(WINDOW *);
extern int copywin(const WINDOW*,WINDOW*,int,int,int,int,int,int,int);
extern bool is_linetouched(WINDOW *,int);
extern void wsyncup(WINDOW *);
extern WINDOW *derwin(WINDOW *,int,int,int,int);
extern int winsnstr(WINDOW *, const char *,int);
extern int winsdelln(WINDOW *,int);
#endif

/*----------------------------------------------------------------------------
  This is the core module of the form library. It contains the majority
  of the driver routines as well as the form_driver function. 

  Essentially this module is nearly the whole library. This is because
  all the functions in this module depends on some others in the module,
  so it makes no sense to split them into separate files because they
  will always be linked together. The only acceptable concern is turnaround
  time for this module, but now we have all Pentiums or Riscs, so what!

  The driver routines are grouped into nine generic categories:

   a)   Page Navigation            ( all functions prefixed by PN_ )
        The current page of the form is left and some new page is
        entered.
   b)   Inter-Field Navigation     ( all functions prefixed by FN_ )
        The current field of the form is left and some new field is
        entered.
   c)   Intra-Field Navigation     ( all functions prefixed by IFN_ )
        The current position in the current field is changed. 
   d)   Vertical Scrolling         ( all functions prefixed by VSC_ )
        Esseantially this is a specialization of Intra-Field navigation.
        It has to check for a multi-line field.
   e)   Horizontal Scrolling       ( all functions prefixed by HSC_ )
        Esseantially this is a specialization of Intra-Field navigation.
        It has to check for a single-line field.
   f)   Field Editing              ( all functions prefixed by FE_ )
        The content of the current field is changed
   g)   Edit Mode requests         ( all functions prefixed by EM_ )
        Switching between insert and overlay mode
   h)   Field-Validation requests  ( all functions prefixed by FV_ )
        Perform verifications of the field.
   i)   Choice requests            ( all functions prefixed by CR_ )
        Requests to enumerate possible field values
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Some remarks on the placements of assert() macros :
  I use them only on "strategic" places, i.e. top level entries where
  I want to make sure that things are set correctly. Throughout subordinate
  routines I omit them mostly.
  --------------------------------------------------------------------------*/

/*
Some options that may effect compatibility in behavior to SVr4 forms,
but they are here to allow a more intuitive and user friendly behaviour of
our form implementation. This doesn't affect the API, so we feel it is
uncritical.

The initial implementation tries to stay very close with the behaviour
of the original SVr4 implementation, although in some areas it is quite
clear that this isn't the most appropriate way. As far as possible this
sources will allow you to build a forms lib that behaves quite similar
to SVr4, but now and in the future we will give you better options. 
Perhaps at some time we will make this configurable at runtime.
*/

/* Implement a more user-friendly previous/next word behaviour */
#define FRIENDLY_PREV_NEXT_WORD (1)
/* Fix the wrong behaviour for forms with all fields inactive */
#define FIX_FORM_INACTIVE_BUG (1)
/* Allow dynamic field growth also when navigating past the end */
#define GROW_IF_NAVIGATE (1)

/*----------------------------------------------------------------------------
  Forward references to some internally used static functions
  --------------------------------------------------------------------------*/
static int Inter_Field_Navigation ( int (* const fct) (FORM *), FORM * form );
static int FN_Next_Field (FORM * form);
static int FN_Previous_Field (FORM * form);
static int FE_New_Line(FORM *);
static int FE_Delete_Previous(FORM *);

/*----------------------------------------------------------------------------
  Macro Definitions.

  Some Remarks on that: I use the convention to use UPPERCASE for constants
  defined by Macros. If I provide a macro as a kind of inline routine to
  provide some logic, I use my Upper_Lower case style.
  --------------------------------------------------------------------------*/

/* Calculate the position of a single row in a field buffer */
#define Position_Of_Row_In_Buffer(field,row) ((row)*(field)->dcols)

/* Calculate start address for the fields buffer# N */
#define Address_Of_Nth_Buffer(field,N) \
  ((field)->buf + (N)*(1+Buffer_Length(field)))

/* Calculate the start address of the row in the fields specified buffer# N */
#define Address_Of_Row_In_Nth_Buffer(field,N,row) \
  (Address_Of_Nth_Buffer(field,N) + Position_Of_Row_In_Buffer(field,row))

/* Calculate the start address of the row in the fields primary buffer */
#define Address_Of_Row_In_Buffer(field,row) \
  Address_Of_Row_In_Nth_Buffer(field,0,row)

/* Calculate the start address of the row in the forms current field
   buffer# N */
#define Address_Of_Current_Row_In_Nth_Buffer(form,N) \
   Address_Of_Row_In_Nth_Buffer((form)->current,N,(form)->currow)

/* Calculate the start address of the row in the forms current field
   primary buffer */
#define Address_Of_Current_Row_In_Buffer(form) \
   Address_Of_Current_Row_In_Nth_Buffer(form,0)

/* Calculate the address of the cursor in the forms current field
   primary buffer */
#define Address_Of_Current_Position_In_Nth_Buffer(form,N) \
   (Address_Of_Current_Row_In_Nth_Buffer(form,N) + (form)->curcol)

/* Calculate the address of the cursor in the forms current field
   buffer# N */
#define Address_Of_Current_Position_In_Buffer(form) \
  Address_Of_Current_Position_In_Nth_Buffer(form,0)

/* Logic to decide whether or not a field is actually a field with
   vertical or horizontal scrolling */
#define Is_Scroll_Field(field)          \
   (((field)->drows > (field)->rows) || \
    ((field)->dcols > (field)->cols))

/* Logic to decide whether or not a field needs to have an individual window
   instead of a derived window because it contains invisible parts.
   This is true for non-public fields and for scrollable fields. */
#define Has_Invisible_Parts(field)     \
  (!((field)->opts & O_PUBLIC)      || \
   Is_Scroll_Field(field))

/* Logic to decide whether or not a field needs justification */
#define Justification_Allowed(field)        \
   (((field)->just != NO_JUSTIFICATION)  && \
    (Single_Line_Field(field))           && \
    (((field)->dcols == (field)->cols)   && \
    ((field)->opts & O_STATIC))             )

/* Logic to determine whether or not a dynamic field may still grow */
#define Growable(field) ((field)->status & _MAY_GROW)

/* Macro to set the attributes for a fields window */
#define Set_Field_Window_Attributes(field,win) \
(  wbkgdset((win),(chtype)((field)->pad | (field)->back)), \
   wattrset((win),(field)->fore) )

/* Logic to decide whether or not a field really appears on the form */
#define Field_Really_Appears(field)         \
  ((field->form)                          &&\
   (field->form->status & _POSTED)        &&\
   (field->opts & O_VISIBLE)              &&\
   (field->page == field->form->curpage))

/* Logic to determine whether or not we are on the first position in the
   current field */
#define First_Position_In_Current_Field(form) \
  (((form)->currow==0) && ((form)->curcol==0))


#define Minimum(a,b) (((a)<=(b)) ? (a) : (b))
#define Maximum(a,b) (((a)>=(b)) ? (a) : (b))

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static char *Get_Start_Of_Data(char * buf, int blen)
|   
|   Description   :  Return pointer to first non-blank position in buffer.
|                    If buffer is empty return pointer to buffer itself.
|
|   Return Values :  Pointer to first non-blank position in buffer
+--------------------------------------------------------------------------*/
INLINE static char *Get_Start_Of_Data(char * buf, int blen)
{
  char *p   = buf;
  char *end = &buf[blen];

  assert(buf && blen>=0);
  while( (p < end) && is_blank(*p) ) 
    p++;
  return( (p==end) ? buf : p );
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static char *After_End_Of_Data(char * buf, int blen)
|   
|   Description   :  Return pointer after last non-blank position in buffer.
|                    If buffer is empty, return pointer to buffer itself.
|
|   Return Values :  Pointer to position after last non-blank position in 
|                    buffer.
+--------------------------------------------------------------------------*/
INLINE static char *After_End_Of_Data(char * buf,int blen)
{
  char *p   = &buf[blen];
  
  assert(buf && blen>=0);
  while( (p>buf) && is_blank(p[-1]) ) 
    p--;
  return( p );
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static char *Get_First_Whitespace_Character(
|                                     char * buf, int   blen)
|   
|   Description   :  Position to the first whitespace character.
|
|   Return Values :  Pointer to first whitespace character in buffer.
+--------------------------------------------------------------------------*/
INLINE static char *Get_First_Whitespace_Character(char * buf, int blen)
{
  char *p   = buf;
  char *end = &p[blen];
  
  assert(buf && blen>=0);
  while( (p < end) && !is_blank(*p)) 
    p++;
  return( (p==end) ? buf : p );
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static char *After_Last_Whitespace_Character(
|                                     char * buf, int blen)
|   
|   Description   :  Get the position after the last whitespace character.
|
|   Return Values :  Pointer to position after last whitespace character in 
|                    buffer.
+--------------------------------------------------------------------------*/
INLINE static char *After_Last_Whitespace_Character(char * buf, int blen)
{
  char *p   = &buf[blen];
  
  assert(buf && blen>=0);
  while( (p>buf) && !is_blank(p[-1]) ) 
    p--;
  return( p );
}

/* Set this to 1 to use the div_t version. This is a good idea if your
   compiler has an intrinsic div() support. Unfortunately GNU-C has it
   not yet. 
   N.B.: This only works if form->curcol follows immediately form->currow
         and both are of type int. 
*/
#define USE_DIV_T (0)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Adjust_Cursor_Position(
|                                       FORM * form, const char * pos)
|   
|   Description   :  Set current row and column of the form to values 
|                    corresponding to the buffer position.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
INLINE static void Adjust_Cursor_Position(FORM * form, const char * pos)
{
  FIELD *field;
  int idx;

  field = form->current;
  assert( pos >= field->buf && field->dcols > 0);
  idx = (int)( pos - field->buf );
#if USE_DIV_T
  *((div_t *)&(form->currow)) = div(idx,field->dcols);
#else
  form->currow = idx / field->dcols;
  form->curcol = idx - field->cols * form->currow;
#endif  
  if ( field->drows < form->currow )
    form->currow = 0;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Buffer_To_Window(
|                                      const FIELD  * field,
|                                      WINDOW * win)
|   
|   Description   :  Copy the buffer to the window. If its a multiline
|                    field, the buffer is split to the lines of the
|                    window without any editing.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Buffer_To_Window(const FIELD  * field, WINDOW * win)
{
  int width, height;
  int len;
  int row;
  char *pBuffer;

  assert(win && field);

  getmaxyx(win, height, width);

  for(row=0, pBuffer=field->buf; 
      row < height; 
      row++, pBuffer += width )
    {
      if ((len = (int)( After_End_Of_Data( pBuffer, width ) - pBuffer )) > 0)
        {
          wmove( win, row, 0 );
          waddnstr( win, pBuffer, len );
        }
    }   
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Window_To_Buffer(
|                                          WINDOW * win,
|                                          FIELD  * field)
|   
|   Description   :  Copy the content of the window into the buffer.
|                    The multiple lines of a window are simply
|                    concatenated into the buffer. Pad characters in
|                    the window will be replaced by blanks in the buffer.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Window_To_Buffer(WINDOW * win, FIELD  * field)
{
  int pad;
  int len = 0;
  char *p;
  int row, height, width;
  
  assert(win && field && field->buf );

  pad = field->pad;
  p = field->buf;
  getmaxyx(win, height, width);

  for(row=0; (row < height) && (row < field->drows); row++ )
    {
      wmove( win, row, 0 );
      len += winnstr( win, p+len, field->dcols );
    }
  p[len] = '\0';

  /* replace visual padding character by blanks in buffer */
  if (pad != C_BLANK)
    {
      int i;
      for(i=0; i<len; i++, p++)
        {
          if (*p==pad) 
            *p = C_BLANK;
        }
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Synchronize_Buffer(FORM * form)
|   
|   Description   :  If there was a change, copy the content of the
|                    window into the buffer, so the buffer is synchronized
|                    with the windows content. We have to indicate that the
|                    buffer needs validation due to the change.
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
INLINE static void Synchronize_Buffer(FORM * form)
{
  if (form->status & _WINDOW_MODIFIED)
    {
      form->status &= ~_WINDOW_MODIFIED;
      form->status |=  _FCHECK_REQUIRED;
      Window_To_Buffer(form->w,form->current);
      wmove(form->w,form->currow,form->curcol);
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Field_Grown( FIELD *field, int amount)
|   
|   Description   :  This function is called for growable dynamic fields
|                    only. It has to increase the buffers and to allocate
|                    a new window for this field.
|                    This function has the side effect to set a new
|                    field-buffer pointer, the dcols and drows values
|                    as well as a new current Window for the field.
|
|   Return Values :  TRUE     - field successfully increased
|                    FALSE    - there was some error
+--------------------------------------------------------------------------*/
static bool Field_Grown(FIELD * field, int amount)
{
  bool result = FALSE;

  if (field && Growable(field))
    {
      bool single_line_field = Single_Line_Field(field);
      int old_buflen = Buffer_Length(field);
      int new_buflen;
      int old_dcols = field->dcols;
      int old_drows = field->drows;
      char *oldbuf  = field->buf;
      char *newbuf;

      int growth;
      FORM *form = field->form;
      bool need_visual_update = ((form != (FORM *)0)      &&
                                 (form->status & _POSTED) &&
                                 (form->current==field));
      
      if (need_visual_update)
        Synchronize_Buffer(form);
      
      if (single_line_field)
        {
          growth = field->cols * amount;
          if (field->maxgrow)
            growth = Minimum(field->maxgrow - field->dcols,growth);
          field->dcols += growth;
          if (field->dcols == field->maxgrow)
            field->status &= ~_MAY_GROW;
        }
      else
        {
          growth = (field->rows + field->nrow) * amount;
          if (field->maxgrow)
            growth = Minimum(field->maxgrow - field->drows,growth);
          field->drows += growth;
          if (field->drows == field->maxgrow)
            field->status &= ~_MAY_GROW;
        }
      /* drows, dcols changed, so we get really the new buffer length */
      new_buflen = Buffer_Length(field);
      newbuf=(char *)malloc((size_t)Total_Buffer_Size(field));
      if (!newbuf)
        { /* restore to previous state */
          field->dcols = old_dcols;
          field->drows = old_drows;
          if (( single_line_field && (field->dcols!=field->maxgrow)) ||
              (!single_line_field && (field->drows!=field->maxgrow)))
            field->status |= _MAY_GROW;
          return FALSE;
        }
      else
        { /* Copy all the buffers. This is the reason why we can't
             just use realloc().
             */
          int i;
          char *old_bp;
          char *new_bp;
          
          field->buf = newbuf;
          for(i=0;i<=field->nbuf;i++)
            {
              new_bp = Address_Of_Nth_Buffer(field,i);
              old_bp = oldbuf + i*(1+old_buflen);
              memcpy(new_bp,old_bp,(size_t)old_buflen);
              if (new_buflen > old_buflen)
                memset(new_bp + old_buflen,C_BLANK,
                       (size_t)(new_buflen - old_buflen));
              *(new_bp + new_buflen) = '\0';
            }

          if (need_visual_update)
            {         
              WINDOW *new_window = newpad(field->drows,field->dcols);
              if (!new_window)
                { /* restore old state */
                  field->dcols = old_dcols;
                  field->drows = old_drows;
                  field->buf   = oldbuf;
                  if (( single_line_field              && 
                        (field->dcols!=field->maxgrow)) ||
                      (!single_line_field              && 
                       (field->drows!=field->maxgrow)))
                    field->status |= _MAY_GROW;
                  free( newbuf );
                  return FALSE;
                }
              assert(form!=(FORM *)0);
              delwin(form->w);
              form->w = new_window;
              Set_Field_Window_Attributes(field,form->w);
              werase(form->w);
              Buffer_To_Window(field,form->w);
              untouchwin(form->w);
              wmove(form->w,form->currow,form->curcol);
            }

          free(oldbuf);
          /* reflect changes in linked fields */
          if (field != field->link)
            {
              FIELD *linked_field;
              for(linked_field = field->link;
                  linked_field!= field;
                  linked_field = linked_field->link)
                {
                  linked_field->buf   = field->buf;
                  linked_field->drows = field->drows;
                  linked_field->dcols = field->dcols;
                }
            }
          result = TRUE;
        }       
    }
  return(result);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int _nc_Position_Form_Cursor(FORM * form)
|   
|   Description   :  Position the cursor in the window for the current
|                    field to be in sync. with the currow and curcol 
|                    values.
|
|   Return Values :  E_OK              - success
|                    E_BAD_ARGUMENT    - invalid form pointer
|                    E_SYSTEM_ERROR    - form has no current field or
|                                        field-window
+--------------------------------------------------------------------------*/
int
_nc_Position_Form_Cursor(FORM * form)
{
  FIELD  *field;
  WINDOW *formwin;
  
  if (!form)
    return(E_BAD_ARGUMENT);

  if (!form->w || !form->current) 
    return(E_SYSTEM_ERROR);

  field    = form->current;
  formwin  = Get_Form_Window(form);

  wmove( form->w, form->currow, form->curcol );
  if ( Has_Invisible_Parts(field) )
    {
      /* in this case fieldwin isn't derived from formwin, so we have
         to move the cursor in formwin by hand... */
      wmove(formwin,
            field->frow + form->currow - form->toprow,
            field->fcol + form->curcol - form->begincol);
      wcursyncup(formwin);
    }
  else 
    wcursyncup(form->w);
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int _nc_Refresh_Current_Field(FORM * form)
|   
|   Description   :  Propagate the changes in the fields window to the
|                    window of the form.
|
|   Return Values :  E_OK              - on success
|                    E_BAD_ARGUMENT    - invalid form pointer
|                    E_SYSTEM_ERROR    - general error
+--------------------------------------------------------------------------*/
int
_nc_Refresh_Current_Field(FORM * form)
{
  WINDOW *formwin;
  FIELD  *field;

  if (!form)
    RETURN(E_BAD_ARGUMENT);

  if (!form->w || !form->current) 
    RETURN(E_SYSTEM_ERROR);

  field    = form->current;
  formwin  = Get_Form_Window(form);

  if (field->opts & O_PUBLIC)
    {
      if (Is_Scroll_Field(field))
        {
          /* Again, in this case the fieldwin isn't derived from formwin,
             so we have to perform a copy operation. */
          if (Single_Line_Field(field))
            { /* horizontal scrolling */
              if (form->curcol < form->begincol)
                  form->begincol = form->curcol;
              else
                {
                  if (form->curcol >= (form->begincol + field->cols))
                      form->begincol = form->curcol - field->cols + 1;
                }
              copywin(form->w,
                      formwin,
                      0,
                      form->begincol,
                      field->frow,
                      field->fcol,
                      field->frow,
                      field->cols + field->fcol - 1,
                      0);
            }
          else
            { /* A multiline, i.e. vertical scrolling field */
              int row_after_bottom,first_modified_row,first_unmodified_row;

              if (field->drows > field->rows)
                {
                  row_after_bottom = form->toprow + field->rows;
                  if (form->currow < form->toprow)
                    {
                      form->toprow = form->currow;
                      field->status |= _NEWTOP;
                    }
                  if (form->currow >= row_after_bottom)
                    {
                      form->toprow = form->currow - field->rows + 1;
                      field->status |= _NEWTOP;
                    }
                  if (field->status & _NEWTOP)
                    { /* means we have to copy whole range */
                      first_modified_row = form->toprow;
                      first_unmodified_row = first_modified_row + field->rows;
                      field->status &= ~_NEWTOP;
                    }
                  else 
                    { /* we try to optimize : finding the range of touched
                         lines */
                      first_modified_row = form->toprow;
                      while(first_modified_row < row_after_bottom)
                        {
                          if (is_linetouched(form->w,first_modified_row)) 
                            break;
                          first_modified_row++;
                        }
                      first_unmodified_row = first_modified_row;
                      while(first_unmodified_row < row_after_bottom)
                        {
                          if (!is_linetouched(form->w,first_unmodified_row)) 
                            break;
                          first_unmodified_row++;
                        }
                    }
                }
              else
                {
                  first_modified_row   = form->toprow;
                  first_unmodified_row = first_modified_row + field->rows;
                }
              if (first_unmodified_row != first_modified_row)
                copywin(form->w,
                        formwin,
                        first_modified_row,
                        0,
                        field->frow + first_modified_row - form->toprow,
                        field->fcol,
                        field->frow + first_unmodified_row - form->toprow - 1,
                        field->cols + field->fcol - 1,
                        0);
            }
          wsyncup(formwin);
        }
      else
        { /* if the field-window is simply a derived window, i.e. contains
             no invisible parts, the whole thing is trivial 
          */
          wsyncup(form->w);
        }
    }
  untouchwin(form->w);
  return _nc_Position_Form_Cursor(form);
}
        
/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Perform_Justification(
|                                        FIELD  * field,
|                                        WINDOW * win)
|   
|   Description   :  Output field with requested justification 
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Perform_Justification(FIELD  * field, WINDOW * win)
{
  char *bp;
  int len;
  int col  = 0;

  bp  = Get_Start_Of_Data(field->buf,Buffer_Length(field));
  len = (int)(After_End_Of_Data(field->buf,Buffer_Length(field)) - bp);

  if (len>0)
    {
      assert(win && (field->drows == 1) && (field->dcols == field->cols));

      switch(field->just)
        {
        case JUSTIFY_LEFT:
          break;
        case JUSTIFY_CENTER:
          col = (field->cols - len)/2;
          break;
        case JUSTIFY_RIGHT:
          col = field->cols - len;
          break;
        default:
          break;
        }

      wmove(win,0,col);
      waddnstr(win,bp,len);
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static void Undo_Justification(
|                                     FIELD  * field,
|                                     WINDOW * win)
|   
|   Description   :  Display field without any justification, i.e.
|                    left justified
|
|   Return Values :  -
+--------------------------------------------------------------------------*/
static void Undo_Justification(FIELD  * field, WINDOW * win)
{
  char *bp;
  int len;

  bp  = Get_Start_Of_Data(field->buf,Buffer_Length(field));
  len = (int)(After_End_Of_Data(field->buf,Buffer_Length(field))-bp);

  if (len>0)
    {
      assert(win != 0);
      wmove(win,0,0);
      waddnstr(win,bp,len);
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_Char(
|                                           FIELDTYPE * typ,
|                                           int ch,
|                                           TypeArgument *argp)
|   
|   Description   :  Perform a single character check for character ch
|                    according to the fieldtype instance.  
|
|   Return Values :  TRUE             - Character is valid
|                    FALSE            - Character is invalid
+--------------------------------------------------------------------------*/
static bool Check_Char(FIELDTYPE * typ, int ch, TypeArgument *argp)
{
  if (typ) 
    {
      if (typ->status & _LINKED_TYPE)
        {
          assert(argp != 0);
          return(
            Check_Char(typ->left ,ch,argp->left ) ||
            Check_Char(typ->right,ch,argp->right) );
        } 
      else 
        {
          if (typ->ccheck)
            return typ->ccheck(ch,(void *)argp);
        }
    }
  return (isprint((unsigned char)ch) ? TRUE : FALSE);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Display_Or_Erase_Field(
|                                           FIELD * field,
|                                           bool bEraseFlag)
|   
|   Description   :  Create a subwindow for the field and display the
|                    buffer contents (apply justification if required)
|                    or simply erase the field.
|
|   Return Values :  E_OK           - on success
|                    E_SYSTEM_ERROR - some error (typical no memory)
+--------------------------------------------------------------------------*/
static int Display_Or_Erase_Field(FIELD * field, bool bEraseFlag)
{
  WINDOW *win;
  WINDOW *fwin;

  if (!field)
    return E_SYSTEM_ERROR;

  fwin = Get_Form_Window(field->form);
  win  = derwin(fwin,
                field->rows,field->cols,field->frow,field->fcol);

  if (!win) 
    return E_SYSTEM_ERROR;
  else
    {
      if (field->opts & O_VISIBLE)
        Set_Field_Window_Attributes(field,win);
      else
        {
#if defined(__LSB_VERSION__)
        /* getattrs() would be handy, but it is not part of LSB 4.0 */
        attr_t fwinAttrs;
        short  fwinPair;
        wattr_get(fwin, &fwinAttrs, &fwinPair, 0);
        wattr_set(win, fwinAttrs, fwinPair, 0);
#else
        wattrset(win,getattrs(fwin));
#endif
        }
      werase(win);
    }

  if (!bEraseFlag)
    {
      if (field->opts & O_PUBLIC)
        {
          if (Justification_Allowed(field))
            Perform_Justification(field,win);
          else
            Buffer_To_Window(field,win);
        }
      field->status &= ~_NEWTOP;
    }
  wsyncup(win);
  delwin(win);
  return E_OK;
}

/* Macros to preset the bEraseFlag */
#define Display_Field(field) Display_Or_Erase_Field(field,FALSE)
#define Erase_Field(field)   Display_Or_Erase_Field(field,TRUE)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Synchronize_Field(FIELD * field)
|   
|   Description   :  Synchronize the windows content with the value in
|                    the buffer.
|
|   Return Values :  E_OK                - success
|                    E_BAD_ARGUMENT      - invalid field pointer 
|                    E_SYSTEM_ERROR      - some severe basic error
+--------------------------------------------------------------------------*/
static int Synchronize_Field(FIELD * field)
{
  FORM *form;
  int res = E_OK;

  if (!field)
    return(E_BAD_ARGUMENT);

  if (((form=field->form) != (FORM *)0)
      && Field_Really_Appears(field))
    {
      if (field == form->current)
        { 
          form->currow  = form->curcol = form->toprow = form->begincol = 0;
          werase(form->w);
      
          if ( (field->opts & O_PUBLIC) && Justification_Allowed(field) )
            Undo_Justification( field, form->w );
          else
            Buffer_To_Window( field, form->w );
          
          field->status |= _NEWTOP;
          res = _nc_Refresh_Current_Field( form );
        }
      else
        res = Display_Field( field );
    }
  field->status |= _CHANGED;
  return(res);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Synchronize_Linked_Fields(FIELD * field)
|   
|   Description   :  Propagate the Synchronize_Field function to all linked
|                    fields. The first error that occurs in the sequence
|                    of updates is the returnvalue.
|
|   Return Values :  E_OK                - success
|                    E_BAD_ARGUMENT      - invalid field pointer 
|                    E_SYSTEM_ERROR      - some severe basic error
+--------------------------------------------------------------------------*/
static int Synchronize_Linked_Fields(FIELD * field)
{
  FIELD *linked_field;
  int res = E_OK;
  int syncres;

  if (!field)
    return(E_BAD_ARGUMENT);

  if (!field->link)
    return(E_SYSTEM_ERROR);

  for(linked_field = field->link; 
      linked_field!= field;
      linked_field = linked_field->link )
    {
      if (((syncres=Synchronize_Field(linked_field)) != E_OK) &&
          (res==E_OK))
        res = syncres;
    }
  return(res);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int _nc_Synchronize_Attributes(FIELD * field)
|   
|   Description   :  If a fields visual attributes have changed, this
|                    routine is called to propagate those changes to the
|                    screen.  
|
|   Return Values :  E_OK             - success
|                    E_BAD_ARGUMENT   - invalid field pointer
|                    E_SYSTEM_ERROR   - some severe basic error
+--------------------------------------------------------------------------*/
int _nc_Synchronize_Attributes(FIELD * field)
{
  FORM *form;
  int res = E_OK;
  WINDOW *formwin;

  if (!field)
    return(E_BAD_ARGUMENT);

  if (((form=field->form) != (FORM *)0)
      && Field_Really_Appears(field))
    {    
      if (form->current==field)
        {
          Synchronize_Buffer(form);
          Set_Field_Window_Attributes(field,form->w);
          werase(form->w);
          if (field->opts & O_PUBLIC)
            {
              if (Justification_Allowed(field))
                Undo_Justification(field,form->w);
              else 
                Buffer_To_Window(field,form->w);
            }
          else 
            {
              formwin = Get_Form_Window(form); 
              copywin(form->w,formwin,
                      0,0,
                      field->frow,field->fcol,
                      field->rows-1,field->cols-1,0);
              wsyncup(formwin);
              Buffer_To_Window(field,form->w);
              field->status |= _NEWTOP; /* fake refresh to paint all */
              _nc_Refresh_Current_Field(form);
            }
        }
      else 
        {
          res = Display_Field(field);
        }
    }
  return(res);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int _nc_Synchronize_Options(FIELD * field,
|                                                Field_Options newopts)
|   
|   Description   :  If a fields options have changed, this routine is
|                    called to propagate these changes to the screen and
|                    to really change the behaviour of the field.
|
|   Return Values :  E_OK                - success
|                    E_BAD_ARGUMENT      - invalid field pointer 
|                    E_SYSTEM_ERROR      - some severe basic error
+--------------------------------------------------------------------------*/
int
_nc_Synchronize_Options(FIELD *field, Field_Options newopts)
{
  Field_Options oldopts;
  Field_Options changed_opts;
  FORM *form;
  int res = E_OK;

  if (!field)
    return(E_BAD_ARGUMENT);

  oldopts      = field->opts;
  changed_opts = oldopts ^ newopts;
  field->opts  = newopts;
  form         = field->form;

  if (form)
    {
      if (form->current == field)
        {
          field->opts = oldopts;
          return(E_CURRENT);
        }

      if (form->status & _POSTED)
        {
          if (form->curpage == field->page)
            {
              if (changed_opts & O_VISIBLE)
                {
                  if (newopts & O_VISIBLE)
                    res = Display_Field(field);
                  else
                    res = Erase_Field(field);
                }
              else
                {
                  if ((changed_opts & O_PUBLIC) &&
                      (newopts & O_VISIBLE))
                    res = Display_Field(field);
                }
            }
        }
    }

  if (changed_opts & O_STATIC)
    {
      bool single_line_field = Single_Line_Field(field);
      int res2 = E_OK;

      if (newopts & O_STATIC)
        { /* the field becomes now static */
          field->status &= ~_MAY_GROW;
          /* if actually we have no hidden columns, justification may
             occur again */
          if (single_line_field                 &&
              (field->cols == field->dcols)     &&
              (field->just != NO_JUSTIFICATION) &&
              Field_Really_Appears(field))
            {
              res2 = Display_Field(field);
            }
        }
      else
        { /* field is no longer static */
          if ((field->maxgrow==0) ||
              ( single_line_field && (field->dcols < field->maxgrow)) ||
              (!single_line_field && (field->drows < field->maxgrow)))
            {
              field->status |= _MAY_GROW;
              /* a field with justification now changes its behaviour,
                 so we must redisplay it */
              if (single_line_field                 &&
                  (field->just != NO_JUSTIFICATION) &&
                  Field_Really_Appears(field))
                {
                  res2 = Display_Field(field);
                }        
            }     
        }
      if (res2 != E_OK)
        res = res2;
    }

  return(res);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int _nc_Set_Current_Field(FORM  * form,
|                                              FIELD * newfield)
|   
|   Description   :  Make the newfield the new current field.
|
|   Return Values :  E_OK                - success
|                    E_BAD_ARGUMENT      - invalid form or field pointer 
|                    E_SYSTEM_ERROR      - some severe basic error
+--------------------------------------------------------------------------*/
int
_nc_Set_Current_Field(FORM  *form, FIELD *newfield)
{
  FIELD  *field;
  WINDOW *new_window;

  if (!form || !newfield || !form->current || (newfield->form!=form))
    return(E_BAD_ARGUMENT);

  if ( (form->status & _IN_DRIVER) )
    return(E_BAD_STATE);

  if (!(form->field))
    return(E_NOT_CONNECTED);

  field = form->current;
 
  if ((field!=newfield) || 
      !(form->status & _POSTED))
    {
      if ((form->w) && 
          (field->opts & O_VISIBLE) &&
          (field->form->curpage == field->page))
        {
          _nc_Refresh_Current_Field(form);
          if (field->opts & O_PUBLIC)
            {
              if (field->drows > field->rows)
                {
                  if (form->toprow==0)
                    field->status &= ~_NEWTOP;
                  else 
                    field->status |= _NEWTOP;
                } 
              else 
                {
                  if (Justification_Allowed(field))
                    {
                      Window_To_Buffer(form->w,field);
                      werase(form->w);
                      Perform_Justification(field,form->w);
                      wsyncup(form->w);
                    }
                }
            }
          delwin(form->w);
        }
      
      field = newfield;

      if (Has_Invisible_Parts(field))
        new_window = newpad(field->drows,field->dcols);
      else 
        new_window = derwin(Get_Form_Window(form),
                            field->rows,field->cols,field->frow,field->fcol);

      if (!new_window) 
        return(E_SYSTEM_ERROR);

      form->current = field;
      form->w       = new_window;
      form->status &= ~_WINDOW_MODIFIED;
      Set_Field_Window_Attributes(field,form->w);

      if (Has_Invisible_Parts(field))
        {
          werase(form->w);
          Buffer_To_Window(field,form->w);
        } 
      else 
        {
          if (Justification_Allowed(field))
            {
              werase(form->w);
              Undo_Justification(field,form->w);
              wsyncup(form->w);
            }
        }

      untouchwin(form->w);
    }

  form->currow = form->curcol = form->toprow = form->begincol = 0;
  return(E_OK);
}

/*----------------------------------------------------------------------------
  Intra-Field Navigation routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Next_Character(FORM * form)
|   
|   Description   :  Move to the next character in the field. In a multiline
|                    field this wraps at the end of the line.
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - at the rightmost position
+--------------------------------------------------------------------------*/
static int IFN_Next_Character(FORM * form)
{
  FIELD *field = form->current;
  
  if ((++(form->curcol))==field->dcols)
    {
      if ((++(form->currow))==field->drows)
        {
#if GROW_IF_NAVIGATE
          if (!Single_Line_Field(field) && Field_Grown(field,1)) {
            form->curcol = 0;
            return(E_OK);
          }
#endif
          form->currow--;
#if GROW_IF_NAVIGATE
          if (Single_Line_Field(field) && Field_Grown(field,1))
            return(E_OK);
#endif
          form->curcol--;
          return(E_REQUEST_DENIED);
        }
      form->curcol = 0;
    }
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Previous_Character(FORM * form)
|   
|   Description   :  Move to the previous character in the field. In a 
|                    multiline field this wraps and the beginning of the 
|                    line.
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - at the leftmost position
+--------------------------------------------------------------------------*/
static int IFN_Previous_Character(FORM * form)
{
  if ((--(form->curcol))<0)
    {
      if ((--(form->currow))<0)
        {
          form->currow++;
          form->curcol++;
          return(E_REQUEST_DENIED);
        }
      form->curcol = form->current->dcols - 1;
    }
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Next_Line(FORM * form)
|   
|   Description   :  Move to the beginning of the next line in the field
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - at the last line
+--------------------------------------------------------------------------*/
static int IFN_Next_Line(FORM * form)
{
  FIELD *field = form->current;

  if ((++(form->currow))==field->drows)
    {
#if GROW_IF_NAVIGATE
      if (!Single_Line_Field(field) && Field_Grown(field,1))
        return(E_OK);
#endif
      form->currow--;
      return(E_REQUEST_DENIED);
    }
  form->curcol = 0;
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Previous_Line(FORM * form)
|   
|   Description   :  Move to the beginning of the previous line in the field
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - at the first line
+--------------------------------------------------------------------------*/
static int IFN_Previous_Line(FORM * form)
{
  if ( (--(form->currow)) < 0 )
    {
      form->currow++;
      return(E_REQUEST_DENIED);
    }
  form->curcol = 0;
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Next_Word(FORM * form)
|   
|   Description   :  Move to the beginning of the next word in the field.
|
|   Return Values :  E_OK             - success
|                    E_REQUEST_DENIED - there is no next word
+--------------------------------------------------------------------------*/
static int IFN_Next_Word(FORM * form)
{
  FIELD *field = form->current;
  char  *bp    = Address_Of_Current_Position_In_Buffer(form);
  char  *s;
  char  *t;

  /* We really need access to the data, so we have to synchronize */
  Synchronize_Buffer(form);

  /* Go to the first whitespace after the current position (including
     current position). This is then the startpoint to look for the
    next non-blank data */
  s = Get_First_Whitespace_Character(bp,Buffer_Length(field) -
                                     (int)(bp - field->buf));

  /* Find the start of the next word */
  t = Get_Start_Of_Data(s,Buffer_Length(field) -
                        (int)(s - field->buf));
#if !FRIENDLY_PREV_NEXT_WORD
  if (s==t) 
    return(E_REQUEST_DENIED);
  else
#endif
    {
      Adjust_Cursor_Position(form,t);
      return(E_OK);
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Previous_Word(FORM * form)
|   
|   Description   :  Move to the beginning of the previous word in the field.
|
|   Return Values :  E_OK             - success
|                    E_REQUEST_DENIED - there is no previous word
+--------------------------------------------------------------------------*/
static int IFN_Previous_Word(FORM * form)
{
  FIELD *field = form->current;
  char  *bp    = Address_Of_Current_Position_In_Buffer(form);
  char  *s;
  char  *t;
  bool  again = FALSE;

  /* We really need access to the data, so we have to synchronize */
  Synchronize_Buffer(form);

  s = After_End_Of_Data(field->buf,(int)(bp-field->buf));
  /* s points now right after the last non-blank in the buffer before bp.
     If bp was in a word, s equals bp. In this case we must find the last
     whitespace in the buffer before bp and repeat the game to really find
     the previous word! */
  if (s==bp)
    again = TRUE;
  
  /* And next call now goes backward to look for the last whitespace
     before that, pointing right after this, so it points to the begin
     of the previous word. 
  */
  t = After_Last_Whitespace_Character(field->buf,(int)(s - field->buf));
#if !FRIENDLY_PREV_NEXT_WORD
  if (s==t) 
    return(E_REQUEST_DENIED);
#endif
  if (again)
    { /* and do it again, replacing bp by t */
      s = After_End_Of_Data(field->buf,(int)(t - field->buf));
      t = After_Last_Whitespace_Character(field->buf,(int)(s - field->buf));
#if !FRIENDLY_PREV_NEXT_WORD
      if (s==t) 
        return(E_REQUEST_DENIED);
#endif
    }
  Adjust_Cursor_Position(form,t);
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Beginning_Of_Field(FORM * form)
|   
|   Description   :  Place the cursor at the first non-pad character in
|                    the field. 
|
|   Return Values :  E_OK             - success            
+--------------------------------------------------------------------------*/
static int IFN_Beginning_Of_Field(FORM * form)
{
  FIELD *field = form->current;

  Synchronize_Buffer(form);
  Adjust_Cursor_Position(form,
                 Get_Start_Of_Data(field->buf,Buffer_Length(field)));
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_End_Of_Field(FORM * form)
|   
|   Description   :  Place the cursor after the last non-pad character in
|                    the field. If the field occupies the last position in
|                    the buffer, the cursos is positioned on the last 
|                    character.
|
|   Return Values :  E_OK              - success
+--------------------------------------------------------------------------*/
static int IFN_End_Of_Field(FORM * form)
{
  FIELD *field = form->current;
  char *pos;

  Synchronize_Buffer(form);
  pos = After_End_Of_Data(field->buf,Buffer_Length(field));
  if (pos==(field->buf + Buffer_Length(field)))
    pos--;
  Adjust_Cursor_Position(form,pos);
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Beginning_Of_Line(FORM * form)
|   
|   Description   :  Place the cursor on the first non-pad character in
|                    the current line of the field.
|
|   Return Values :  E_OK         - success
+--------------------------------------------------------------------------*/
static int IFN_Beginning_Of_Line(FORM * form)
{
  FIELD *field = form->current;

  Synchronize_Buffer(form);
  Adjust_Cursor_Position(form,
                 Get_Start_Of_Data(Address_Of_Current_Row_In_Buffer(form),
                                   field->dcols));
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_End_Of_Line(FORM * form)
|   
|   Description   :  Place the cursor after the last non-pad character in the
|                    current line of the field. If the field occupies the 
|                    last column in the line, the cursor is positioned on the
|                    last character of the line.
|
|   Return Values :  E_OK        - success
+--------------------------------------------------------------------------*/
static int IFN_End_Of_Line(FORM * form)
{
  FIELD *field = form->current;
  char *pos;
  char *bp;

  Synchronize_Buffer(form);
  bp  = Address_Of_Current_Row_In_Buffer(form); 
  pos = After_End_Of_Data(bp,field->dcols);
  if (pos == (bp + field->dcols))
    pos--;
  Adjust_Cursor_Position(form,pos);
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Left_Character(FORM * form)
|   
|   Description   :  Move one character to the left in the current line.
|                    This doesn't cycle.  
|
|   Return Values :  E_OK             - success
|                    E_REQUEST_DENIED - already in first column
+--------------------------------------------------------------------------*/
static int IFN_Left_Character(FORM * form)
{
  if ( (--(form->curcol)) < 0 )
    {
      form->curcol++;
      return(E_REQUEST_DENIED);
    }
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Right_Character(FORM * form)
|   
|   Description   :  Move one character to the right in the current line.
|                    This doesn't cycle.
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - already in last column
+--------------------------------------------------------------------------*/
static int IFN_Right_Character(FORM * form)
{
  if ( (++(form->curcol)) == form->current->dcols )
    {
#if GROW_IF_NAVIGATE
      FIELD *field = form->current;
      if (Single_Line_Field(field) && Field_Grown(field,1))
        return(E_OK);
#endif
      --(form->curcol);
      return(E_REQUEST_DENIED);
    }
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Up_Character(FORM * form)
|   
|   Description   :  Move one line up. This doesn't cycle through the lines
|                    of the field.
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - already in last column
+--------------------------------------------------------------------------*/
static int IFN_Up_Character(FORM * form)
{
  if ( (--(form->currow)) < 0 )
    {
      form->currow++;
      return(E_REQUEST_DENIED);
    }
  return(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int IFN_Down_Character(FORM * form)
|   
|   Description   :  Move one line down. This doesn't cycle through the
|                    lines of the field.
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - already in last column
+--------------------------------------------------------------------------*/
static int IFN_Down_Character(FORM * form)
{
  FIELD *field = form->current;

  if ( (++(form->currow)) == field->drows )
    {
#if GROW_IF_NAVIGATE
      if (!Single_Line_Field(field) && Field_Grown(field,1))
        return(E_OK);
#endif
      --(form->currow);
      return(E_REQUEST_DENIED);
    }
  return(E_OK);
}
/*----------------------------------------------------------------------------
  END of Intra-Field Navigation routines 
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Vertical scrolling helper routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int VSC_Generic(FORM *form, int lines)
|
|   Description   :  Scroll multi-line field forward (lines>0) or
|                    backward (lines<0) this many lines.
|
|   Return Values :  E_OK              - success 
|                    E_REQUEST_DENIED  - can't scroll
+--------------------------------------------------------------------------*/
static int VSC_Generic(FORM *form, int lines)
{
  FIELD *field = form->current;
  int res = E_REQUEST_DENIED;
  int rows_to_go = (lines > 0 ? lines : -lines);

  if (lines > 0)
    {
      if ( (rows_to_go + form->toprow) > (field->drows - field->rows) )
        rows_to_go = (field->drows - field->rows - form->toprow);

      if (rows_to_go > 0)
        {
          form->currow += rows_to_go;
          form->toprow += rows_to_go;
          res = E_OK;
        }
    }
  else
    {
      if (rows_to_go > form->toprow)
        rows_to_go = form->toprow;
      
      if (rows_to_go > 0)
        {
          form->currow -= rows_to_go;
          form->toprow -= rows_to_go;
          res = E_OK;
        }
    }
  return(res);
}
/*----------------------------------------------------------------------------
  End of Vertical scrolling helper routines
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Vertical scrolling routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Vertical_Scrolling(
|                                           int (* const fct) (FORM *),
|                                           FORM * form)
|   
|   Description   :  Performs the generic vertical scrolling routines. 
|                    This has to check for a multi-line field and to set
|                    the _NEWTOP flag if scrolling really occurred.
|
|   Return Values :  Propagated error code from low-level driver calls
+--------------------------------------------------------------------------*/
static int Vertical_Scrolling(int (* const fct) (FORM *), FORM * form)
{
  int res = E_REQUEST_DENIED;

  if (!Single_Line_Field(form->current))
    {
      res = fct(form);
      if (res == E_OK)
        form->current->status |= _NEWTOP;
    }
  return(res);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int VSC_Scroll_Line_Forward(FORM * form)
|   
|   Description   :  Scroll multi-line field forward a line
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - no data ahead
+--------------------------------------------------------------------------*/
static int VSC_Scroll_Line_Forward(FORM * form)
{
  return VSC_Generic(form,1);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int VSC_Scroll_Line_Backward(FORM * form)
|   
|   Description   :  Scroll multi-line field backward a line
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - no data behind
+--------------------------------------------------------------------------*/
static int VSC_Scroll_Line_Backward(FORM * form)
{
  return VSC_Generic(form,-1);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int VSC_Scroll_Page_Forward(FORM * form)
|   
|   Description   :  Scroll a multi-line field forward a page
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - no data ahead
+--------------------------------------------------------------------------*/
static int VSC_Scroll_Page_Forward(FORM * form)
{
  return VSC_Generic(form,form->current->rows);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int VSC_Scroll_Half_Page_Forward(FORM * form)
|   
|   Description   :  Scroll a multi-line field forward half a page
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - no data ahead
+--------------------------------------------------------------------------*/
static int VSC_Scroll_Half_Page_Forward(FORM * form)
{
  return VSC_Generic(form,(form->current->rows + 1)/2);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int VSC_Scroll_Page_Backward(FORM * form)
|   
|   Description   :  Scroll a multi-line field backward a page
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - no data behind
+--------------------------------------------------------------------------*/
static int VSC_Scroll_Page_Backward(FORM * form)
{
  return VSC_Generic(form, -(form->current->rows));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int VSC_Scroll_Half_Page_Backward(FORM * form)
|   
|   Description   :  Scroll a multi-line field backward half a page
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - no data behind
+--------------------------------------------------------------------------*/
static int VSC_Scroll_Half_Page_Backward(FORM * form)
{
  return VSC_Generic(form, -((form->current->rows + 1)/2));
}
/*----------------------------------------------------------------------------
  End of Vertical scrolling routines
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Horizontal scrolling helper routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int HSC_Generic(FORM *form, int columns)
|
|   Description   :  Scroll single-line field forward (columns>0) or
|                    backward (columns<0) this many columns.
|
|   Return Values :  E_OK              - success 
|                    E_REQUEST_DENIED  - can't scroll
+--------------------------------------------------------------------------*/
static int HSC_Generic(FORM *form, int columns)
{
  FIELD *field = form->current;
  int res = E_REQUEST_DENIED;
  int cols_to_go = (columns > 0 ? columns : -columns);

  if (columns > 0)
    {
      if ((cols_to_go + form->begincol) > (field->dcols - field->cols))
        cols_to_go = field->dcols - field->cols - form->begincol;
      
      if (cols_to_go > 0)
        {
          form->curcol   += cols_to_go;
          form->begincol += cols_to_go;
          res = E_OK;
        }
    }
  else
    {
      if ( cols_to_go > form->begincol )
        cols_to_go = form->begincol;

      if (cols_to_go > 0)
        {
          form->curcol   -= cols_to_go;
          form->begincol -= cols_to_go;
          res = E_OK;
        }
    }
  return(res);
}
/*----------------------------------------------------------------------------
  End of Horizontal scrolling helper routines
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Horizontal scrolling routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Horizontal_Scrolling(
|                                          int (* const fct) (FORM *),
|                                          FORM * form)
|   
|   Description   :  Performs the generic horizontal scrolling routines. 
|                    This has to check for a single-line field.
|
|   Return Values :  Propagated error code from low-level driver calls
+--------------------------------------------------------------------------*/
static int Horizontal_Scrolling(int (* const fct) (FORM *), FORM * form)
{
  if (Single_Line_Field(form->current))
    return fct(form);
  else
    return(E_REQUEST_DENIED);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int HSC_Scroll_Char_Forward(FORM * form)
|   
|   Description   :  Scroll single-line field forward a character
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - no data ahead
+--------------------------------------------------------------------------*/
static int HSC_Scroll_Char_Forward(FORM *form)
{
  return HSC_Generic(form,1);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int HSC_Scroll_Char_Backward(FORM * form)
|   
|   Description   :  Scroll single-line field backward a character
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - no data behind
+--------------------------------------------------------------------------*/
static int HSC_Scroll_Char_Backward(FORM *form)
{
  return HSC_Generic(form,-1);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int HSC_Horizontal_Line_Forward(FORM* form)
|   
|   Description   :  Scroll single-line field forward a line
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - no data ahead
+--------------------------------------------------------------------------*/
static int HSC_Horizontal_Line_Forward(FORM * form)
{
  return HSC_Generic(form,form->current->cols);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int HSC_Horizontal_Half_Line_Forward(FORM* form)
|   
|   Description   :  Scroll single-line field forward half a line
|
|   Return Values :  E_OK               - success
|                    E_REQUEST_DENIED   - no data ahead
+--------------------------------------------------------------------------*/
static int HSC_Horizontal_Half_Line_Forward(FORM * form)
{
  return HSC_Generic(form,(form->current->cols + 1)/2);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int HSC_Horizontal_Line_Backward(FORM* form)
|   
|   Description   :  Scroll single-line field backward a line
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - no data behind
+--------------------------------------------------------------------------*/
static int HSC_Horizontal_Line_Backward(FORM * form)
{
  return HSC_Generic(form,-(form->current->cols));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int HSC_Horizontal_Half_Line_Backward(FORM* form)
|   
|   Description   :  Scroll single-line field backward half a line
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - no data behind
+--------------------------------------------------------------------------*/
static int HSC_Horizontal_Half_Line_Backward(FORM * form)
{
  return HSC_Generic(form,-((form->current->cols + 1)/2));
}

/*----------------------------------------------------------------------------
  End of Horizontal scrolling routines
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Helper routines for Field Editing
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Is_There_Room_For_A_Line(FORM * form)
|   
|   Description   :  Check whether or not there is enough room in the
|                    buffer to enter a whole line.
|
|   Return Values :  TRUE   - there is enough space
|                    FALSE  - there is not enough space
+--------------------------------------------------------------------------*/
INLINE static bool Is_There_Room_For_A_Line(FORM * form)
{
  FIELD *field = form->current;
  char *begin_of_last_line, *s;
  
  Synchronize_Buffer(form);
  begin_of_last_line = Address_Of_Row_In_Buffer(field,(field->drows-1));
  s  = After_End_Of_Data(begin_of_last_line,field->dcols);
  return ((s==begin_of_last_line) ? TRUE : FALSE);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Is_There_Room_For_A_Char_In_Line(FORM * form)
|   
|   Description   :  Checks whether or not there is room for a new character
|                    in the current line.
|
|   Return Values :  TRUE    - there is room
|                    FALSE   - there is not enough room (line full)
+--------------------------------------------------------------------------*/
INLINE static bool Is_There_Room_For_A_Char_In_Line(FORM * form)
{
  int last_char_in_line;

  wmove(form->w,form->currow,form->current->dcols-1);
  last_char_in_line  = (int)(winch(form->w) & A_CHARTEXT);
  wmove(form->w,form->currow,form->curcol);
  return (((last_char_in_line == form->current->pad) ||
           is_blank(last_char_in_line)) ? TRUE : FALSE);
}

#define There_Is_No_Room_For_A_Char_In_Line(f) \
  !Is_There_Room_For_A_Char_In_Line(f)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Insert_String(
|                                             FORM * form,
|                                             int row,
|                                             char *txt,
|                                             int  len )
|   
|   Description   :  Insert the 'len' characters beginning at pointer 'txt'
|                    into the 'row' of the 'form'. The insertion occurs
|                    on the beginning of the row, all other characters are
|                    moved to the right. After the text a pad character will 
|                    be inserted to separate the text from the rest. If
|                    necessary the insertion moves characters on the next
|                    line to make place for the requested insertion string.
|
|   Return Values :  E_OK              - success 
|                    E_REQUEST_DENIED  -
|                    E_SYSTEM_ERROR    - system error
+--------------------------------------------------------------------------*/
static int Insert_String(FORM *form, int row, char *txt, int len)
{ 
  FIELD  *field    = form->current;
  char *bp         = Address_Of_Row_In_Buffer(field,row);
  int datalen      = (int)(After_End_Of_Data(bp,field->dcols) - bp);
  int freelen      = field->dcols - datalen;
  int requiredlen  = len+1;
  char *split;
  int result = E_REQUEST_DENIED;
  char *Space;

  Space = (char*)malloc(2*sizeof(char));
  strcpy(Space, " ");

  if (freelen >= requiredlen)
    {
      wmove(form->w,row,0);
      winsnstr(form->w,txt,len);
      wmove(form->w,row,len);
      winsnstr(form->w,Space,1);
      free(Space);
      return E_OK;
    }
  else
    { /* we have to move characters on the next line. If we are on the
         last line this may work, if the field is growable */
      if ((row == (field->drows - 1)) && Growable(field))
        {
          if (!Field_Grown(field,1))
            {
            free(Space);
            return(E_SYSTEM_ERROR);
            }
          /* !!!Side-Effect : might be changed due to growth!!! */
          bp = Address_Of_Row_In_Buffer(field,row); 
        }

      if (row < (field->drows - 1)) 
        { 
          split = After_Last_Whitespace_Character(bp,
                    (int)(Get_Start_Of_Data(bp + field->dcols - requiredlen ,
                                            requiredlen) - bp));
          /* split points now to the first character of the portion of the
             line that must be moved to the next line */
          datalen = (int)(split-bp); /* + freelen has to stay on this line   */
          freelen = field->dcols - (datalen + freelen); /* for the next line */

          if ((result=Insert_String(form,row+1,split,freelen))==E_OK) 
            {
              wmove(form->w,row,datalen);
              wclrtoeol(form->w);
              wmove(form->w,row,0);
              winsnstr(form->w,txt,len);
              wmove(form->w,row,len);
              winsnstr(form->w,Space,1);
              free(Space);
              return E_OK;
            }
        }
      free(Space);
      return(result);
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Wrapping_Not_Necessary_Or_Wrapping_Ok(
|                                             FORM * form)
|   
|   Description   :  If a character has been entered into a field, it may
|                    be that wrapping has to occur. This routine checks
|                    whether or not wrapping is required and if so, performs
|                    the wrapping.
|
|   Return Values :  E_OK              - no wrapping required or wrapping
|                                        was successful
|                    E_REQUEST_DENIED  -
|                    E_SYSTEM_ERROR    - some system error
+--------------------------------------------------------------------------*/
static int Wrapping_Not_Necessary_Or_Wrapping_Ok(FORM * form)
{
  FIELD  *field = form->current;
  int result = E_REQUEST_DENIED;
  bool Last_Row = ((field->drows - 1) == form->currow);

  if ( (field->opts & O_WRAP)                     &&  /* wrapping wanted     */
      (!Single_Line_Field(field))                 &&  /* must be multi-line  */
      (There_Is_No_Room_For_A_Char_In_Line(form)) &&  /* line is full        */
      (!Last_Row || Growable(field))               )  /* there are more lines*/
    {
      char *bp;
      char *split;
      int chars_to_be_wrapped;
      int chars_to_remain_on_line;
      if (Last_Row)
        { /* the above logic already ensures, that in this case the field
             is growable */
          if (!Field_Grown(field,1))
            return E_SYSTEM_ERROR;
        }
      bp = Address_Of_Current_Row_In_Buffer(form);
      Window_To_Buffer(form->w,field);
      split = After_Last_Whitespace_Character(bp,field->dcols);
      /* split points to the first character of the sequence to be brought
         on the next line */
      chars_to_remain_on_line = (int)(split - bp);
      chars_to_be_wrapped     = field->dcols - chars_to_remain_on_line;
      if (chars_to_remain_on_line > 0)
        {
          if ((result=Insert_String(form,form->currow+1,split,
                                    chars_to_be_wrapped)) == E_OK)
            {
              wmove(form->w,form->currow,chars_to_remain_on_line);
              wclrtoeol(form->w);
              if (form->curcol >= chars_to_remain_on_line)
                {
                  form->currow++;
                  form->curcol -= chars_to_remain_on_line;
                }
              return E_OK;
            }
        }
      else
        return E_OK;
      if (result!=E_OK)
        {
          wmove(form->w,form->currow,form->curcol);
          wdelch(form->w);
          Window_To_Buffer(form->w,field);
          result = E_REQUEST_DENIED;
        }
    }
  else
    result = E_OK; /* wrapping was not necessary */
  return(result);
}

/*----------------------------------------------------------------------------
  Field Editing routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Field_Editing(
|                                    int (* const fct) (FORM *),
|                                    FORM * form)
|   
|   Description   :  Generic routine for field editing requests. The driver
|                    routines are only called for editable fields, the
|                    _WINDOW_MODIFIED flag is set if editing occurred.
|                    This is somewhat special due to the overload semantics
|                    of the NEW_LINE and DEL_PREV requests.
|
|   Return Values :  Error code from low level drivers.
+--------------------------------------------------------------------------*/
static int Field_Editing(int (* const fct) (FORM *), FORM * form)
{
  int res = E_REQUEST_DENIED;

  /* We have to deal here with the specific case of the overloaded 
     behaviour of New_Line and Delete_Previous requests.
     They may end up in navigational requests if we are on the first
     character in a field. But navigation is also allowed on non-
     editable fields.
  */ 
  if ((fct==FE_Delete_Previous)            && 
      (form->opts & O_BS_OVERLOAD)         &&
      First_Position_In_Current_Field(form) )
    {
      res = Inter_Field_Navigation(FN_Previous_Field,form);
    }
  else
    {
      if (fct==FE_New_Line)
        {
          if ((form->opts & O_NL_OVERLOAD)         &&
              First_Position_In_Current_Field(form))
            {
              res = Inter_Field_Navigation(FN_Next_Field,form);
            }
          else
            /* FE_New_Line deals itself with the _WINDOW_MODIFIED flag */
            res = fct(form);
        }
      else
        {
          /* From now on, everything must be editable */
          if (form->current->opts & O_EDIT)
            {
              res = fct(form);
              if (res==E_OK)
                form->status |= _WINDOW_MODIFIED;
            }
        }
    }
  return res;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_New_Line(FORM * form)
|   
|   Description   :  Perform a new line request. This is rather complex
|                    compared to other routines in this code due to the 
|                    rather difficult to understand description in the
|                    manuals.
|
|   Return Values :  E_OK               - success
|                    E_REQUEST_DENIED   - new line not allowed
|                    E_SYSTEM_ERROR     - system error
+--------------------------------------------------------------------------*/
static int FE_New_Line(FORM * form)
{
  FIELD  *field = form->current;
  char *bp, *t;
  bool Last_Row = ((field->drows - 1)==form->currow);
  
  if (form->status & _OVLMODE) 
    {
      if (Last_Row && 
          (!(Growable(field) && !Single_Line_Field(field))))
        {
          if (!(form->opts & O_NL_OVERLOAD))
            return(E_REQUEST_DENIED);
          wclrtoeol(form->w);
          /* we have to set this here, although it is also
             handled in the generic routine. The reason is,
             that FN_Next_Field may fail, but the form is
             definitively changed */
          form->status |= _WINDOW_MODIFIED;
          return Inter_Field_Navigation(FN_Next_Field,form);
        }
      else 
        {
          if (Last_Row && !Field_Grown(field,1))
            { /* N.B.: due to the logic in the 'if', LastRow==TRUE
                 means here that the field is growable and not
                 a single-line field */
              return(E_SYSTEM_ERROR);
            }
          wclrtoeol(form->w);
          form->currow++;
          form->curcol = 0;
          form->status |= _WINDOW_MODIFIED;
          return(E_OK);
        }
    }
  else 
    { /* Insert Mode */
      if (Last_Row &&
          !(Growable(field) && !Single_Line_Field(field)))
        {
          if (!(form->opts & O_NL_OVERLOAD))
            return(E_REQUEST_DENIED);
          return Inter_Field_Navigation(FN_Next_Field,form);
        }
      else 
        {
          bool May_Do_It = !Last_Row && Is_There_Room_For_A_Line(form);
          
          if (!(May_Do_It || Growable(field)))
            return(E_REQUEST_DENIED);
          if (!May_Do_It && !Field_Grown(field,1))
            return(E_SYSTEM_ERROR);
          
          bp= Address_Of_Current_Position_In_Buffer(form);
          t = After_End_Of_Data(bp,field->dcols - form->curcol);
          wclrtoeol(form->w);
          form->currow++;
          form->curcol=0;
          wmove(form->w,form->currow,form->curcol);
          winsertln(form->w);
          waddnstr(form->w,bp,(int)(t-bp));
          form->status |= _WINDOW_MODIFIED;
          return E_OK;
        }
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Insert_Character(FORM * form)
|   
|   Description   :  Insert blank character at the cursor position
|
|   Return Values :  E_OK
|                    E_REQUEST_DENIED
+--------------------------------------------------------------------------*/
static int FE_Insert_Character(FORM * form)
{
  FIELD *field = form->current;
  int result = E_REQUEST_DENIED;

  if (Check_Char(field->type,(int)C_BLANK,(TypeArgument *)(field->arg)))
    {
      bool There_Is_Room = Is_There_Room_For_A_Char_In_Line(form);

      if (There_Is_Room ||
          ((Single_Line_Field(field) && Growable(field))))
        {
          if (!There_Is_Room && !Field_Grown(field,1))
            result =  E_SYSTEM_ERROR;
          else
            {
              winsch(form->w,(chtype)C_BLANK);
              result = Wrapping_Not_Necessary_Or_Wrapping_Ok(form);
            }
        }
    }
  return result;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Insert_Line(FORM * form)
|   
|   Description   :  Insert a blank line at the cursor position
|
|   Return Values :  E_OK               - success
|                    E_REQUEST_DENIED   - line can not be inserted
+--------------------------------------------------------------------------*/
static int FE_Insert_Line(FORM * form)
{
  FIELD *field = form->current;
  int result = E_REQUEST_DENIED;

  if (Check_Char(field->type,(int)C_BLANK,(TypeArgument *)(field->arg)))
    {
      bool Maybe_Done = (form->currow!=(field->drows-1)) && 
                        Is_There_Room_For_A_Line(form);

      if (!Single_Line_Field(field) &&
          (Maybe_Done || Growable(field)))
        {
          if (!Maybe_Done && !Field_Grown(field,1))
            result = E_SYSTEM_ERROR;
          else
            {
              form->curcol = 0;
              winsertln(form->w);
              result = E_OK;
            }
        }
    }
  return result;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Delete_Character(FORM * form)
|   
|   Description   :  Delete character at the cursor position
|
|   Return Values :  E_OK    - success
+--------------------------------------------------------------------------*/
static int FE_Delete_Character(FORM * form)
{
  wdelch(form->w);
  return E_OK;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Delete_Previous(FORM * form)
|   
|   Description   :  Delete character before cursor. Again this is a rather
|                    difficult piece compared to others due to the overloading
|                    semantics of backspace.
|                    N.B.: The case of overloaded BS on first field position
|                          is already handled in the generic routine.
|
|   Return Values :  E_OK                - success
|                    E_REQUEST_DENIED    - Character can't be deleted
+--------------------------------------------------------------------------*/
static int FE_Delete_Previous(FORM * form)
{
  FIELD  *field = form->current;
  
  if (First_Position_In_Current_Field(form))
    return E_REQUEST_DENIED;

  if ( (--(form->curcol))<0 )
    {
      char *this_line, *prev_line, *prev_end, *this_end;
      
      form->curcol++;
      if (form->status & _OVLMODE) 
        return E_REQUEST_DENIED;
      
      prev_line = Address_Of_Row_In_Buffer(field,(form->currow-1));
      this_line = Address_Of_Row_In_Buffer(field,(form->currow));
      Synchronize_Buffer(form);
      prev_end = After_End_Of_Data(prev_line,field->dcols);
      this_end = After_End_Of_Data(this_line,field->dcols);
      if ((int)(this_end-this_line) > 
          (field->cols-(int)(prev_end-prev_line))) 
        return E_REQUEST_DENIED;
      wdeleteln(form->w);
      Adjust_Cursor_Position(form,prev_end);
      wmove(form->w,form->currow,form->curcol);
      waddnstr(form->w,this_line,(int)(this_end-this_line));
    } 
  else 
    {
      wmove(form->w,form->currow,form->curcol);
      wdelch(form->w);
    }
  return E_OK;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Delete_Line(FORM * form)
|   
|   Description   :  Delete line at cursor position.
|
|   Return Values :  E_OK  - success
+--------------------------------------------------------------------------*/
static int FE_Delete_Line(FORM * form)
{
  form->curcol = 0;
  wdeleteln(form->w);
  return E_OK;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Delete_Word(FORM * form)
|   
|   Description   :  Delete word at cursor position
|
|   Return Values :  E_OK               - success
|                    E_REQUEST_DENIED   - failure
+--------------------------------------------------------------------------*/
static int FE_Delete_Word(FORM * form)
{
  FIELD  *field = form->current;
  char   *bp = Address_Of_Current_Row_In_Buffer(form);
  char   *ep = bp + field->dcols;
  char   *cp = bp + form->curcol;
  char *s;
  
  Synchronize_Buffer(form);
  if (is_blank(*cp)) 
    return E_REQUEST_DENIED; /* not in word */

  /* move cursor to begin of word and erase to end of screen-line */
  Adjust_Cursor_Position(form,
                         After_Last_Whitespace_Character(bp,form->curcol)); 
  wmove(form->w,form->currow,form->curcol);
  wclrtoeol(form->w);

  /* skip over word in buffer */
  s = Get_First_Whitespace_Character(cp,(int)(ep-cp)); 
  /* to begin of next word    */
  s = Get_Start_Of_Data(s,(int)(ep - s));
  if ( (s!=cp) && !is_blank(*s))
    {
      /* copy remaining line to window */
      waddnstr(form->w,s,(int)(s - After_End_Of_Data(s,(int)(ep - s))));
    }
  return E_OK;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Clear_To_End_Of_Line(FORM * form)
|   
|   Description   :  Clear to end of current line.
|
|   Return Values :  E_OK   - success
+--------------------------------------------------------------------------*/
static int FE_Clear_To_End_Of_Line(FORM * form)
{
  wclrtoeol(form->w);
  return E_OK;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Clear_To_End_Of_Form(FORM * form)
|   
|   Description   :  Clear to end of form.
|
|   Return Values :  E_OK   - success
+--------------------------------------------------------------------------*/
static int FE_Clear_To_End_Of_Form(FORM * form)
{
  wclrtobot(form->w);
  return E_OK;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FE_Clear_Field(FORM * form)
|   
|   Description   :  Clear entire field.
|
|   Return Values :  E_OK   - success
+--------------------------------------------------------------------------*/
static int FE_Clear_Field(FORM * form)
{
  form->currow = form->curcol = 0;
  werase(form->w);
  return E_OK;
}
/*----------------------------------------------------------------------------
  END of Field Editing routines 
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Edit Mode routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int EM_Overlay_Mode(FORM * form)
|   
|   Description   :  Switch to overlay mode.
|
|   Return Values :  E_OK   - success
+--------------------------------------------------------------------------*/
static int EM_Overlay_Mode(FORM * form)
{
  form->status |= _OVLMODE;
  return E_OK;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int EM_Insert_Mode(FORM * form)
|   
|   Description   :  Switch to insert mode
|
|   Return Values :  E_OK   - success
+--------------------------------------------------------------------------*/
static int EM_Insert_Mode(FORM * form)
{
  form->status &= ~_OVLMODE;
  return E_OK;
}

/*----------------------------------------------------------------------------
  END of Edit Mode routines 
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Helper routines for Choice Requests
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Next_Choice(
|                                            FIELDTYPE * typ,
|                                            FIELD * field,
|                                            TypeArgument *argp)
|   
|   Description   :  Get the next field choice. For linked types this is
|                    done recursively.
|
|   Return Values :  TRUE    - next choice successfully retrieved
|                    FALSE   - couldn't retrieve next choice
+--------------------------------------------------------------------------*/
static bool Next_Choice(FIELDTYPE * typ, FIELD *field, TypeArgument *argp)
{
  if (!typ || !(typ->status & _HAS_CHOICE)) 
    return FALSE;

  if (typ->status & _LINKED_TYPE)
    {
      assert(argp != 0);
      return(
             Next_Choice(typ->left ,field,argp->left) ||
             Next_Choice(typ->right,field,argp->right) );
    } 
  else
    {
      assert(typ->next != 0);
      return typ->next(field,(void *)argp);
    }
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Previous_Choice(
|                                                FIELDTYPE * typ,
|                                                FIELD * field,
|                                                TypeArgument *argp)
|   
|   Description   :  Get the previous field choice. For linked types this
|                    is done recursively.
|
|   Return Values :  TRUE    - previous choice successfully retrieved
|                    FALSE   - couldn't retrieve previous choice
+--------------------------------------------------------------------------*/
static bool Previous_Choice(FIELDTYPE *typ, FIELD *field, TypeArgument *argp)
{
  if (!typ || !(typ->status & _HAS_CHOICE)) 
    return FALSE;

  if (typ->status & _LINKED_TYPE)
    {
      assert(argp != 0);
      return(
             Previous_Choice(typ->left ,field,argp->left) ||
             Previous_Choice(typ->right,field,argp->right));
    } 
  else 
    {
      assert(typ->prev != 0);
      return typ->prev(field,(void *)argp);
    }
}
/*----------------------------------------------------------------------------
  End of Helper routines for Choice Requests
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Routines for Choice Requests
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int CR_Next_Choice(FORM * form)
|   
|   Description   :  Get the next field choice.
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - next choice couldn't be retrieved
+--------------------------------------------------------------------------*/
static int CR_Next_Choice(FORM * form)
{
  FIELD *field = form->current;
  Synchronize_Buffer(form);
  return ((Next_Choice(field->type,field,(TypeArgument *)(field->arg))) ? 
          E_OK : E_REQUEST_DENIED);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int CR_Previous_Choice(FORM * form)
|   
|   Description   :  Get the previous field choice.
|
|   Return Values :  E_OK              - success
|                    E_REQUEST_DENIED  - prev. choice couldn't be retrieved
+--------------------------------------------------------------------------*/
static int CR_Previous_Choice(FORM * form)
{
  FIELD *field = form->current;
  Synchronize_Buffer(form);
  return ((Previous_Choice(field->type,field,(TypeArgument *)(field->arg))) ? 
          E_OK : E_REQUEST_DENIED);
}
/*----------------------------------------------------------------------------
  End of Routines for Choice Requests
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Helper routines for Field Validations.
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static bool Check_Field(
|                                            FIELDTYPE * typ,
|                                            FIELD * field,
|                                            TypeArgument * argp)
|   
|   Description   :  Check the field according to its fieldtype and its
|                    actual arguments. For linked fieldtypes this is done
|                    recursively.
|
|   Return Values :  TRUE       - field is valid
|                    FALSE      - field is invalid.
+--------------------------------------------------------------------------*/
static bool Check_Field(FIELDTYPE *typ, FIELD *field, TypeArgument *argp)
{
  if (typ)
    {
      if (field->opts & O_NULLOK)
        {
          char *bp = field->buf;
          assert(bp != 0);
          while(is_blank(*bp))
            { bp++; }
          if (*bp == '\0') 
            return TRUE;
        }

      if (typ->status & _LINKED_TYPE)
        {
          assert(argp != 0);
          return( 
                 Check_Field(typ->left ,field,argp->left ) ||
                 Check_Field(typ->right,field,argp->right) );
        }
      else 
        {
          if (typ->fcheck)
            return typ->fcheck(field,(void *)argp);
        }
    }
  return TRUE;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  bool _nc_Internal_Validation(FORM * form )
|   
|   Description   :  Validate the current field of the form.  
|
|   Return Values :  TRUE  - field is valid
|                    FALSE - field is invalid
+--------------------------------------------------------------------------*/
bool
_nc_Internal_Validation(FORM *form)
{
  FIELD *field;

  field = form->current; 
  
  Synchronize_Buffer(form);
  if ((form->status & _FCHECK_REQUIRED) ||
      (!(field->opts & O_PASSOK)))
    {
      if (!Check_Field(field->type,field,(TypeArgument *)(field->arg)))
        return FALSE;
      form->status  &= ~_FCHECK_REQUIRED;
      field->status |= _CHANGED;
      Synchronize_Linked_Fields(field);
    }
  return TRUE;
}
/*----------------------------------------------------------------------------
  End of Helper routines for Field Validations.
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Routines for Field Validation.
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FV_Validation(FORM * form)
|   
|   Description   :  Validate the current field of the form.
|
|   Return Values :  E_OK             - field valid
|                    E_INVALID_FIELD  - field not valid
+--------------------------------------------------------------------------*/
static int FV_Validation(FORM * form)
{
  if (_nc_Internal_Validation(form))
    return E_OK;
  else
    return E_INVALID_FIELD;
}
/*----------------------------------------------------------------------------
  End of routines for Field Validation.
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Helper routines for Inter-Field Navigation
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Next_Field_On_Page(FIELD * field)
|   
|   Description   :  Get the next field after the given field on the current 
|                    page. The order of fields is the one defined by the
|                    fields array. Only visible and active fields are
|                    counted.
|
|   Return Values :  Pointer to the next field.
+--------------------------------------------------------------------------*/
INLINE static FIELD *Next_Field_On_Page(FIELD * field)
{
  FORM  *form = field->form;
  FIELD **field_on_page = &form->field[field->index];
  FIELD **first_on_page = &form->field[form->page[form->curpage].pmin];
  FIELD **last_on_page  = &form->field[form->page[form->curpage].pmax];

  do
    {
      field_on_page = 
        (field_on_page==last_on_page) ? first_on_page : field_on_page + 1;
      if (Field_Is_Selectable(*field_on_page))
        break;
    } while(field!=(*field_on_page));  
  return(*field_on_page);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  FIELD* _nc_First_Active_Field(FORM * form)
|   
|   Description   :  Get the first active field on the current page,
|                    if there are such. If there are none, get the first
|                    visible field on the page. If there are also none,
|                    we return the first field on page and hope the best.
|
|   Return Values :  Pointer to calculated field.
+--------------------------------------------------------------------------*/
FIELD*
_nc_First_Active_Field(FORM * form)
{
  FIELD **last_on_page = &form->field[form->page[form->curpage].pmax];
  FIELD *proposed = Next_Field_On_Page(*last_on_page);

  if (proposed == *last_on_page)
    { /* there might be the special situation, where there is no 
         active and visible field on the current page. We then select
         the first visible field on this readonly page
      */
      if (Field_Is_Not_Selectable(proposed))
        {
          FIELD **field = &form->field[proposed->index];
          FIELD **first = &form->field[form->page[form->curpage].pmin];

          do
            {
              field = (field==last_on_page) ? first : field + 1;
              if (((*field)->opts & O_VISIBLE))
                break;
            } while(proposed!=(*field));
          
          proposed = *field;

          if ((proposed == *last_on_page) && !(proposed->opts&O_VISIBLE))
            { /* This means, there is also no visible field on the page.
                 So we propose the first one and hope the very best... 
                 Some very clever user has designed a readonly and invisible
                 page on this form.
               */
              proposed = *first;
            }
        }
    }
  return(proposed);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Previous_Field_On_Page(FIELD * field)
|   
|   Description   :  Get the previous field before the given field on the 
|                    current page. The order of fields is the one defined by 
|                    the fields array. Only visible and active fields are
|                    counted.
|
|   Return Values :  Pointer to the previous field.
+--------------------------------------------------------------------------*/
INLINE static FIELD *Previous_Field_On_Page(FIELD * field)
{
  FORM  *form   = field->form;
  FIELD **field_on_page = &form->field[field->index];
  FIELD **first_on_page = &form->field[form->page[form->curpage].pmin];
  FIELD **last_on_page  = &form->field[form->page[form->curpage].pmax];
  
  do
    {
      field_on_page = 
        (field_on_page==first_on_page) ? last_on_page : field_on_page - 1;
      if (Field_Is_Selectable(*field_on_page))
        break;
    } while(field!=(*field_on_page));
  
  return (*field_on_page);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Sorted_Next_Field(FIELD * field)
|   
|   Description   :  Get the next field after the given field on the current 
|                    page. The order of fields is the one defined by the
|                    (row,column) geometry, rows are major.
|
|   Return Values :  Pointer to the next field.
+--------------------------------------------------------------------------*/
INLINE static FIELD *Sorted_Next_Field(FIELD * field)
{
  FIELD *field_on_page = field;

  do
    {
      field_on_page = field_on_page->snext;
      if (Field_Is_Selectable(field_on_page))
        break;
    } while(field_on_page!=field);
  
  return (field_on_page);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Sorted_Previous_Field(FIELD * field)
|   
|   Description   :  Get the previous field before the given field on the 
|                    current page. The order of fields is the one defined 
|                    by the (row,column) geometry, rows are major.
|
|   Return Values :  Pointer to the previous field.
+--------------------------------------------------------------------------*/
INLINE static FIELD *Sorted_Previous_Field(FIELD * field)
{
  FIELD *field_on_page = field;

  do
    {
      field_on_page = field_on_page->sprev;
      if (Field_Is_Selectable(field_on_page))
        break;
    } while(field_on_page!=field);
  
  return (field_on_page);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Left_Neighbour_Field(FIELD * field)
|   
|   Description   :  Get the left neighbour of the field on the same line
|                    and the same page. Cycles through the line.
|
|   Return Values :  Pointer to left neighbour field.
+--------------------------------------------------------------------------*/
INLINE static FIELD *Left_Neighbour_Field(FIELD * field)
{
  FIELD *field_on_page = field;

  /* For a field that has really a left neighbour, the while clause
     immediately fails and the loop is left, positioned at the right
     neighbour. Otherwise we cycle backwards through the sorted fieldlist
     until we enter the same line (from the right end).
  */
  do
    {
      field_on_page = Sorted_Previous_Field(field_on_page);
    } while(field_on_page->frow != field->frow);
  
  return (field_on_page);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Right_Neighbour_Field(FIELD * field)
|   
|   Description   :  Get the right neighbour of the field on the same line
|                    and the same page.
|
|   Return Values :  Pointer to right neighbour field.
+--------------------------------------------------------------------------*/
INLINE static FIELD *Right_Neighbour_Field(FIELD * field)
{
  FIELD *field_on_page = field;

  /* See the comments on Left_Neighbour_Field to understand how it works */
  do
    {
      field_on_page = Sorted_Next_Field(field_on_page);
    } while(field_on_page->frow != field->frow);
  
  return (field_on_page);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Upper_Neighbour_Field(FIELD * field)
|   
|   Description   :  Because of the row-major nature of sorting the fields,
|                    its more difficult to define whats the upper neighbour
|                    field really means. We define that it must be on a
|                    'previous' line (cyclic order!) and is the rightmost
|                    field laying on the left side of the given field. If
|                    this set is empty, we take the first field on the line.
|
|   Return Values :  Pointer to the upper neighbour field.
+--------------------------------------------------------------------------*/
static FIELD *Upper_Neighbour_Field(FIELD * field)
{
  FIELD *field_on_page = field;
  int frow = field->frow;
  int fcol = field->fcol;

  /* Walk back to the 'previous' line. The second term in the while clause
     just guarantees that we stop if we cycled through the line because
     there might be no 'previous' line if the page has just one line.
  */
  do
    {
      field_on_page = Sorted_Previous_Field(field_on_page);
    } while(field_on_page->frow==frow && field_on_page->fcol!=fcol);
  
  if (field_on_page->frow!=frow)
    { /* We really found a 'previous' line. We are positioned at the
         rightmost field on this line */
      frow = field_on_page->frow; 

      /* We walk to the left as long as we are really right of the 
         field. */
      while(field_on_page->frow==frow && field_on_page->fcol>fcol)
        field_on_page = Sorted_Previous_Field(field_on_page);

      /* If we wrapped, just go to the right which is the first field on 
         the row */
      if (field_on_page->frow!=frow)
        field_on_page = Sorted_Next_Field(field_on_page);
    }
  
  return (field_on_page);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static FIELD *Down_Neighbour_Field(FIELD * field)
|   
|   Description   :  Because of the row-major nature of sorting the fields,
|                    its more difficult to define whats the down neighbour
|                    field really means. We define that it must be on a
|                    'next' line (cyclic order!) and is the leftmost
|                    field laying on the right side of the given field. If
|                    this set is empty, we take the last field on the line.
|
|   Return Values :  Pointer to the upper neighbour field.
+--------------------------------------------------------------------------*/
static FIELD *Down_Neighbour_Field(FIELD * field)
{
  FIELD *field_on_page = field;
  int frow = field->frow;
  int fcol = field->fcol;

  /* Walk forward to the 'next' line. The second term in the while clause
     just guarantees that we stop if we cycled through the line because
     there might be no 'next' line if the page has just one line.
  */
  do
    {
      field_on_page = Sorted_Next_Field(field_on_page);
    } while(field_on_page->frow==frow && field_on_page->fcol!=fcol);

  if (field_on_page->frow!=frow)
    { /* We really found a 'next' line. We are positioned at the rightmost
         field on this line */
      frow = field_on_page->frow;

      /* We walk to the right as long as we are really left of the 
         field. */
      while(field_on_page->frow==frow && field_on_page->fcol<fcol)
        field_on_page = Sorted_Next_Field(field_on_page);

      /* If we wrapped, just go to the left which is the last field on 
         the row */
      if (field_on_page->frow!=frow)
        field_on_page = Sorted_Previous_Field(field_on_page);
    }
  
  return(field_on_page);
}

/*----------------------------------------------------------------------------
  Inter-Field Navigation routines
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Inter_Field_Navigation(
|                                           int (* const fct) (FORM *),
|                                           FORM * form)
|   
|   Description   :  Generic behaviour for changing the current field, the
|                    field is left and a new field is entered. So the field
|                    must be validated and the field init/term hooks must
|                    be called.
|
|   Return Values :  E_OK                - success
|                    E_INVALID_FIELD     - field is invalid
|                    some other          - error from subordinate call
+--------------------------------------------------------------------------*/
static int Inter_Field_Navigation(int (* const fct) (FORM *),FORM *form)
{
  int res;

  if (!_nc_Internal_Validation(form)) 
    res = E_INVALID_FIELD;
  else
    {
      Call_Hook(form,fieldterm);
      res = fct(form);
      Call_Hook(form,fieldinit);
    }
  return res;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Next_Field(FORM * form)
|   
|   Description   :  Move to the next field on the current page of the form
|
|   Return Values :  E_OK                 - success
|                    != E_OK              - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Next_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Next_Field_On_Page(form->current));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Previous_Field(FORM * form)
|   
|   Description   :  Move to the previous field on the current page of the 
|                    form
|
|   Return Values :  E_OK                 - success
|                    != E_OK              - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Previous_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Previous_Field_On_Page(form->current));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_First_Field(FORM * form)
|   
|   Description   :  Move to the first field on the current page of the form
|
|   Return Values :  E_OK                 - success
|                    != E_OK              - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_First_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
      Next_Field_On_Page(form->field[form->page[form->curpage].pmax]));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Last_Field(FORM * form)
|   
|   Description   :  Move to the last field on the current page of the form
|
|   Return Values :  E_OK                 - success
|                    != E_OK              - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Last_Field(FORM * form)
{
  return 
    _nc_Set_Current_Field(form,
       Previous_Field_On_Page(form->field[form->page[form->curpage].pmin]));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Sorted_Next_Field(FORM * form)
|   
|   Description   :  Move to the sorted next field on the current page
|                    of the form.
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Sorted_Next_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Sorted_Next_Field(form->current));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Sorted_Previous_Field(FORM * form)
|   
|   Description   :  Move to the sorted previous field on the current page
|                    of the form.
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Sorted_Previous_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Sorted_Previous_Field(form->current));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Sorted_First_Field(FORM * form)
|   
|   Description   :  Move to the sorted first field on the current page
|                    of the form.
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Sorted_First_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
              Sorted_Next_Field(form->field[form->page[form->curpage].smax]));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Sorted_Last_Field(FORM * form)
|   
|   Description   :  Move to the sorted last field on the current page
|                    of the form.
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Sorted_Last_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
           Sorted_Previous_Field(form->field[form->page[form->curpage].smin]));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Left_Field(FORM * form)
|   
|   Description   :  Get the field on the left of the current field on the
|                    same line and the same page. Cycles through the line.
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Left_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Left_Neighbour_Field(form->current));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Right_Field(FORM * form)
|   
|   Description   :  Get the field on the right of the current field on the
|                    same line and the same page. Cycles through the line.
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Right_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Right_Neighbour_Field(form->current));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Up_Field(FORM * form)
|   
|   Description   :  Get the upper neighbour of the current field. This
|                    cycles through the page. See the comments of the
|                    Upper_Neighbour_Field function to understand how
|                    'upper' is defined. 
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Up_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Upper_Neighbour_Field(form->current));
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int FN_Down_Field(FORM * form)
|   
|   Description   :  Get the down neighbour of the current field. This
|                    cycles through the page. See the comments of the
|                    Down_Neighbour_Field function to understand how
|                    'down' is defined. 
|
|   Return Values :  E_OK            - success
|                    != E_OK         - error from subordinate call
+--------------------------------------------------------------------------*/
static int FN_Down_Field(FORM * form)
{
  return _nc_Set_Current_Field(form,
                               Down_Neighbour_Field(form->current));
}
/*----------------------------------------------------------------------------
  END of Field Navigation routines 
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Helper routines for Page Navigation
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int _nc_Set_Form_Page(FORM * form,
|                                          int page,
|                                          FIELD * field)
|   
|   Description   :  Make the given page nr. the current page and make
|                    the given field the current field on the page. If
|                    for the field NULL is given, make the first field on
|                    the page the current field. The routine acts only
|                    if the requested page is not the current page.
|
|   Return Values :  E_OK                - success
|                    != E_OK             - error from subordinate call
+--------------------------------------------------------------------------*/
int
_nc_Set_Form_Page(FORM * form, int page, FIELD * field)
{
  int res = E_OK;

  if ((form->curpage!=page))
    {
      FIELD *last_field, *field_on_page;

      werase(Get_Form_Window(form));
      form->curpage = page;
      last_field = field_on_page = form->field[form->page[page].smin];
      do
        {
          if (field_on_page->opts & O_VISIBLE)
            if ((res=Display_Field(field_on_page))!=E_OK) 
              return(res);
          field_on_page = field_on_page->snext;
        } while(field_on_page != last_field);

      if (field)
        res = _nc_Set_Current_Field(form,field);
      else
        /* N.B.: we don't encapsulate this by Inter_Field_Navigation(),
           because this is already executed in a page navigation
           context that contains field navigation 
         */
        res = FN_First_Field(form);
    }
  return(res);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Next_Page_Number(const FORM * form)
|   
|   Description   :  Calculate the page number following the current page
|                    number. This cycles if the highest page number is
|                    reached.  
|
|   Return Values :  The next page number
+--------------------------------------------------------------------------*/
INLINE static int Next_Page_Number(const FORM * form)
{
  return (form->curpage + 1) % form->maxpage;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Previous_Page_Number(const FORM * form)
|   
|   Description   :  Calculate the page number before the current page
|                    number. This cycles if the first page number is
|                    reached.  
|
|   Return Values :  The previous page number
+--------------------------------------------------------------------------*/
INLINE static int Previous_Page_Number(const FORM * form)
{
  return (form->curpage!=0 ? form->curpage - 1 : form->maxpage - 1);
}

/*----------------------------------------------------------------------------
  Page Navigation routines 
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Page_Navigation(
|                                               int (* const fct) (FORM *),
|                                               FORM * form)
|   
|   Description   :  Generic behaviour for changing a page. This means
|                    that the field is left and a new field is entered.
|                    So the field must be validated and the field init/term
|                    hooks must be called. Because also the page is changed,
|                    the forms init/term hooks must be called also.
|
|   Return Values :  E_OK                - success
|                    E_INVALID_FIELD     - field is invalid
|                    some other          - error from subordinate call
+--------------------------------------------------------------------------*/
static int Page_Navigation(int (* const fct) (FORM *), FORM * form)
{
  int res;

  if (!_nc_Internal_Validation(form)) 
    res = E_INVALID_FIELD;
  else
    {
      Call_Hook(form,fieldterm);
      Call_Hook(form,formterm);
      res = fct(form);
      Call_Hook(form,forminit);
      Call_Hook(form,fieldinit);
    }
  return res;
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int PN_Next_Page(FORM * form)
|   
|   Description   :  Move to the next page of the form
|
|   Return Values :  E_OK                - success
|                    != E_OK             - error from subordinate call
+--------------------------------------------------------------------------*/
static int PN_Next_Page(FORM * form)
{ 
  return _nc_Set_Form_Page(form,Next_Page_Number(form),(FIELD *)0);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int PN_Previous_Page(FORM * form)
|   
|   Description   :  Move to the previous page of the form
|
|   Return Values :  E_OK              - success
|                    != E_OK           - error from subordinate call
+--------------------------------------------------------------------------*/
static int PN_Previous_Page(FORM * form)
{
  return _nc_Set_Form_Page(form,Previous_Page_Number(form),(FIELD *)0);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int PN_First_Page(FORM * form)
|   
|   Description   :  Move to the first page of the form
|
|   Return Values :  E_OK              - success
|                    != E_OK           - error from subordinate call
+--------------------------------------------------------------------------*/
static int PN_First_Page(FORM * form)
{
  return _nc_Set_Form_Page(form,0,(FIELD *)0);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int PN_Last_Page(FORM * form)
|   
|   Description   :  Move to the last page of the form
|
|   Return Values :  E_OK              - success
|                    != E_OK           - error from subordinate call
+--------------------------------------------------------------------------*/
static int PN_Last_Page(FORM * form)
{
  return _nc_Set_Form_Page(form,form->maxpage-1,(FIELD *)0);
}
/*----------------------------------------------------------------------------
  END of Field Navigation routines 
  --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Helper routines for the core form driver.
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  static int Data_Entry(FORM * form,int c)
|   
|   Description   :  Enter character c into at the current position of the
|                    current field of the form.
|
|   Return Values :  E_OK              -
|                    E_REQUEST_DENIED  -
|                    E_SYSTEM_ERROR    -
+--------------------------------------------------------------------------*/
static int Data_Entry(FORM * form, int c)
{
  FIELD  *field = form->current;
  int result = E_REQUEST_DENIED;

  if ( (field->opts & O_EDIT) 
#if FIX_FORM_INACTIVE_BUG
       && (field->opts & O_ACTIVE) 
#endif
       )
    {
      if ( (field->opts & O_BLANK) &&
           First_Position_In_Current_Field(form) &&
           !(form->status & _FCHECK_REQUIRED) && 
           !(form->status & _WINDOW_MODIFIED) )
        werase(form->w);

      if (form->status & _OVLMODE)
        {
          waddch(form->w,(chtype)c);
        } 
      else /* no _OVLMODE */ 
        {
          bool There_Is_Room = Is_There_Room_For_A_Char_In_Line(form);

          if (!(There_Is_Room ||
                ((Single_Line_Field(field) && Growable(field)))))
              return E_REQUEST_DENIED;

          if (!There_Is_Room && !Field_Grown(field,1))
            return E_SYSTEM_ERROR;

          winsch(form->w,(chtype)c);
        }

      if ((result=Wrapping_Not_Necessary_Or_Wrapping_Ok(form))==E_OK)
        {
          bool End_Of_Field= (((field->drows-1)==form->currow) &&
                              ((field->dcols-1)==form->curcol));
          form->status |= _WINDOW_MODIFIED;
          if (End_Of_Field && !Growable(field) && (field->opts & O_AUTOSKIP))
            result = Inter_Field_Navigation(FN_Next_Field,form);
          else
            {
              if (End_Of_Field && Growable(field) && !Field_Grown(field,1))
                result = E_SYSTEM_ERROR;
              else
                {
                  IFN_Next_Character(form);
                  result = E_OK;
                }
            }
        }
    }
  return result;
}

/* Structure to describe the binding of a request code to a function.
   The member keycode codes the request value as well as the generic
   routine to use for the request. The code for the generic routine
   is coded in the upper 16 Bits while the request code is coded in
   the lower 16 bits. 

   In terms of C++ you might think of a request as a class with a
   virtual method "perform". The different types of request are
   derived from this base class and overload (or not) the base class
   implementation of perform.
*/
typedef struct {
  int keycode;           /* must be at least 32 bit: hi:mode, lo: key */
  int (*cmd)(FORM *);    /* low level driver routine for this key     */
} Binding_Info;

/* You may see this is the class-id of the request type class */
#define ID_PN    (0x00000000)    /* Page navigation           */
#define ID_FN    (0x00010000)    /* Inter-Field navigation    */
#define ID_IFN   (0x00020000)    /* Intra-Field navigation    */
#define ID_VSC   (0x00030000)    /* Vertical Scrolling        */
#define ID_HSC   (0x00040000)    /* Horizontal Scrolling      */
#define ID_FE    (0x00050000)    /* Field Editing             */
#define ID_EM    (0x00060000)    /* Edit Mode                 */
#define ID_FV    (0x00070000)    /* Field Validation          */
#define ID_CH    (0x00080000)    /* Choice                    */
#define ID_Mask  (0xffff0000)
#define Key_Mask (0x0000ffff)
#define ID_Shft  (16)

/* This array holds all the Binding Infos */
static const Binding_Info bindings[MAX_FORM_COMMAND - MIN_FORM_COMMAND + 1] = 
{
  { REQ_NEXT_PAGE    |ID_PN  ,PN_Next_Page},
  { REQ_PREV_PAGE    |ID_PN  ,PN_Previous_Page},
  { REQ_FIRST_PAGE   |ID_PN  ,PN_First_Page},
  { REQ_LAST_PAGE    |ID_PN  ,PN_Last_Page},
  
  { REQ_NEXT_FIELD   |ID_FN  ,FN_Next_Field},
  { REQ_PREV_FIELD   |ID_FN  ,FN_Previous_Field},
  { REQ_FIRST_FIELD  |ID_FN  ,FN_First_Field},
  { REQ_LAST_FIELD   |ID_FN  ,FN_Last_Field},
  { REQ_SNEXT_FIELD  |ID_FN  ,FN_Sorted_Next_Field},
  { REQ_SPREV_FIELD  |ID_FN  ,FN_Sorted_Previous_Field},
  { REQ_SFIRST_FIELD |ID_FN  ,FN_Sorted_First_Field},
  { REQ_SLAST_FIELD  |ID_FN  ,FN_Sorted_Last_Field},
  { REQ_LEFT_FIELD   |ID_FN  ,FN_Left_Field},
  { REQ_RIGHT_FIELD  |ID_FN  ,FN_Right_Field},
  { REQ_UP_FIELD     |ID_FN  ,FN_Up_Field},
  { REQ_DOWN_FIELD   |ID_FN  ,FN_Down_Field},
  
  { REQ_NEXT_CHAR    |ID_IFN ,IFN_Next_Character},
  { REQ_PREV_CHAR    |ID_IFN ,IFN_Previous_Character},
  { REQ_NEXT_LINE    |ID_IFN ,IFN_Next_Line},
  { REQ_PREV_LINE    |ID_IFN ,IFN_Previous_Line},
  { REQ_NEXT_WORD    |ID_IFN ,IFN_Next_Word},
  { REQ_PREV_WORD    |ID_IFN ,IFN_Previous_Word},
  { REQ_BEG_FIELD    |ID_IFN ,IFN_Beginning_Of_Field},
  { REQ_END_FIELD    |ID_IFN ,IFN_End_Of_Field},
  { REQ_BEG_LINE     |ID_IFN ,IFN_Beginning_Of_Line},
  { REQ_END_LINE     |ID_IFN ,IFN_End_Of_Line},
  { REQ_LEFT_CHAR    |ID_IFN ,IFN_Left_Character},
  { REQ_RIGHT_CHAR   |ID_IFN ,IFN_Right_Character},
  { REQ_UP_CHAR      |ID_IFN ,IFN_Up_Character},
  { REQ_DOWN_CHAR    |ID_IFN ,IFN_Down_Character},
  
  { REQ_NEW_LINE     |ID_FE  ,FE_New_Line},
  { REQ_INS_CHAR     |ID_FE  ,FE_Insert_Character},
  { REQ_INS_LINE     |ID_FE  ,FE_Insert_Line},
  { REQ_DEL_CHAR     |ID_FE  ,FE_Delete_Character},
  { REQ_DEL_PREV     |ID_FE  ,FE_Delete_Previous},
  { REQ_DEL_LINE     |ID_FE  ,FE_Delete_Line},
  { REQ_DEL_WORD     |ID_FE  ,FE_Delete_Word},
  { REQ_CLR_EOL      |ID_FE  ,FE_Clear_To_End_Of_Line},
  { REQ_CLR_EOF      |ID_FE  ,FE_Clear_To_End_Of_Form},
  { REQ_CLR_FIELD    |ID_FE  ,FE_Clear_Field},
  
  { REQ_OVL_MODE     |ID_EM  ,EM_Overlay_Mode},
  { REQ_INS_MODE     |ID_EM  ,EM_Insert_Mode},
  
  { REQ_SCR_FLINE    |ID_VSC ,VSC_Scroll_Line_Forward},
  { REQ_SCR_BLINE    |ID_VSC ,VSC_Scroll_Line_Backward},
  { REQ_SCR_FPAGE    |ID_VSC ,VSC_Scroll_Page_Forward},
  { REQ_SCR_BPAGE    |ID_VSC ,VSC_Scroll_Page_Backward},
  { REQ_SCR_FHPAGE   |ID_VSC ,VSC_Scroll_Half_Page_Forward},
  { REQ_SCR_BHPAGE   |ID_VSC ,VSC_Scroll_Half_Page_Backward},
  
  { REQ_SCR_FCHAR    |ID_HSC ,HSC_Scroll_Char_Forward},
  { REQ_SCR_BCHAR    |ID_HSC ,HSC_Scroll_Char_Backward},
  { REQ_SCR_HFLINE   |ID_HSC ,HSC_Horizontal_Line_Forward},
  { REQ_SCR_HBLINE   |ID_HSC ,HSC_Horizontal_Line_Backward},
  { REQ_SCR_HFHALF   |ID_HSC ,HSC_Horizontal_Half_Line_Forward},
  { REQ_SCR_HBHALF   |ID_HSC ,HSC_Horizontal_Half_Line_Backward},
  
  { REQ_VALIDATION   |ID_FV  ,FV_Validation},

  { REQ_NEXT_CHOICE  |ID_CH  ,CR_Next_Choice},
  { REQ_PREV_CHOICE  |ID_CH  ,CR_Previous_Choice}
};

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int form_driver(FORM * form,int  c)
|   
|   Description   :  This is the workhorse of the forms system. It checks
|                    to determine whether the character c is a request or
|                    data. If it is a request, the form driver executes
|                    the request and returns the result. If it is data
|                    (printable character), it enters the data into the
|                    current position in the current field. If it is not
|                    recognized, the form driver assumes it is an application
|                    defined command and returns E_UNKNOWN_COMMAND.
|                    Application defined command should be defined relative
|                    to MAX_FORM_COMMAND, the maximum value of a request.
|
|   Return Values :  E_OK              - success
|                    E_SYSTEM_ERROR    - system error
|                    E_BAD_ARGUMENT    - an argument is incorrect
|                    E_NOT_POSTED      - form is not posted
|                    E_INVALID_FIELD   - field contents are invalid
|                    E_BAD_STATE       - called from inside a hook routine
|                    E_REQUEST_DENIED  - request failed
|                    E_UNKNOWN_COMMAND - command not known
+--------------------------------------------------------------------------*/
int form_driver(FORM * form, int  c)
{
  const Binding_Info* BI = (Binding_Info *)0;
  int res = E_UNKNOWN_COMMAND;

  if (!form)
    RETURN(E_BAD_ARGUMENT);

  if (!(form->field))
    RETURN(E_NOT_CONNECTED);
  
  assert(form->page != 0);
  
  if (c==FIRST_ACTIVE_MAGIC)
    {
      form->current = _nc_First_Active_Field(form);
      return E_OK;
    }
  
  assert(form->current && 
         form->current->buf && 
         (form->current->form == form)
        );
  
  if ( form->status & _IN_DRIVER )
    RETURN(E_BAD_STATE);

  if ( !( form->status & _POSTED ) ) 
    RETURN(E_NOT_POSTED);
  
  if ((c>=MIN_FORM_COMMAND && c<=MAX_FORM_COMMAND) &&
      ((bindings[c-MIN_FORM_COMMAND].keycode & Key_Mask) == c))
    BI = &(bindings[c-MIN_FORM_COMMAND]);
  
  if (BI)
    {
      typedef int (*Generic_Method)(int (* const)(FORM *),FORM *);
      static const Generic_Method Generic_Methods[] = 
        {
          Page_Navigation,         /* overloaded to call field&form hooks */
          Inter_Field_Navigation,  /* overloaded to call field hooks      */
          NULL,                    /* Intra-Field is generic              */
          Vertical_Scrolling,      /* Overloaded to check multi-line      */
          Horizontal_Scrolling,    /* Overloaded to check single-line     */
          Field_Editing,           /* Overloaded to mark modification     */
          NULL,                    /* Edit Mode is generic                */
          NULL,                    /* Field Validation is generic         */
          NULL                     /* Choice Request is generic           */
        };
      size_t nMethods = (sizeof(Generic_Methods)/sizeof(Generic_Methods[0]));
      size_t method   = ((BI->keycode & ID_Mask) >> ID_Shft) & 0xffff;
      
      if ( (method >= nMethods) || !(BI->cmd) )
        res = E_SYSTEM_ERROR;
      else
        {
          Generic_Method fct = Generic_Methods[method];
          if (fct)
            res = fct(BI->cmd,form);
          else
            res = (BI->cmd)(form);
        }
    } 
  else 
    {
      if (!(c & (~(int)MAX_REGULAR_CHARACTER)) &&
          isprint((unsigned char)c) &&                      
          Check_Char(form->current->type,c,
                     (TypeArgument *)(form->current->arg)))
        res = Data_Entry(form,c);
    }
  _nc_Refresh_Current_Field(form);
  RETURN(res);
}

/*----------------------------------------------------------------------------
  Field-Buffer manipulation routines.
  The effects of setting a buffer is tightly coupled to the core of the form
  driver logic. This is especially true in the case of growable fields.
  So I don't separate this into an own module. 
  --------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int set_field_buffer(FIELD *field,
|                                         int buffer, char *value)
|   
|   Description   :  Set the given buffer of the field to the given value.
|                    Buffer 0 stores the displayed content of the field.
|                    For dynamic fields this may grow the fieldbuffers if
|                    the length of the value exceeds the current buffer
|                    length. For buffer 0 only printable values are allowed.
|                    For static fields, the value needs not to be zero ter-
|                    minated. It is copied up to the length of the buffer.   
|
|   Return Values :  E_OK            - success
|                    E_BAD_ARGUMENT  - invalid argument
|                    E_SYSTEM_ERROR  - system error
+--------------------------------------------------------------------------*/
int set_field_buffer(FIELD * field, int buffer, const char * value)
{
  char *s, *p;
  int res = E_OK;
  unsigned int len;

  if ( !field || !value || ((buffer < 0)||(buffer > field->nbuf)) )
    RETURN(E_BAD_ARGUMENT);

  len  = Buffer_Length(field);

  if (buffer==0)
    {
      const char *v;
      unsigned int i = 0;

      for(v=value; *v && (i<len); v++,i++)
        {
          if (!isprint((unsigned char)*v))
            RETURN(E_BAD_ARGUMENT);
        }
    }

  if (Growable(field))
    {
      /* for a growable field we must assume zero terminated strings, because
         somehow we have to detect the length of what should be copied.
      */
      unsigned int vlen = strlen(value);
      if (vlen > len)
        {
          if (!Field_Grown(field,
                           (int)(1 + (vlen-len)/((field->rows+field->nrow)*field->cols))))
            RETURN(E_SYSTEM_ERROR);

          /* in this case we also have to check, whether or not the remaining
             characters in value are also printable for buffer 0. */
          if (buffer==0)
            {
              unsigned int i;
          
              for(i=len; i<vlen; i++)
                if (!isprint((int)(value[i])))
                  RETURN(E_BAD_ARGUMENT);
            }
          len = vlen;
        }
    }
  
  p   = Address_Of_Nth_Buffer(field,buffer);

#if HAVE_MEMCCPY
  s = memccpy(p,value,0,len);
#else
  for(s=(char *)value; *s && (s < (value+len)); s++)
    p[s-value] = *s;
  if (s < (value+len))
    {
      int off = s-value;
      p[off] = *s++;
      s = p + (s-value);
    }
  else 
    s=(char *)0;
#endif

  if (s) 
    { /* this means, value was null terminated and not greater than the
         buffer. We have to pad with blanks. Please note that due to memccpy
         logic s points after the terminating null. */
      s--; /* now we point to the terminator. */
      assert(len >= (unsigned int)(s-p));
      if (len > (unsigned int)(s-p))
        memset(s,C_BLANK,len-(unsigned int)(s-p));
    }

  if (buffer==0)
    {
      int syncres;
      if (((syncres=Synchronize_Field( field ))!=E_OK) && 
          (res==E_OK))
        res = syncres;
      if (((syncres=Synchronize_Linked_Fields(field ))!=E_OK) &&
          (res==E_OK))
        res = syncres;
    }
  RETURN(res);
}               

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  char *field_buffer(const FIELD *field,int buffer)
|   
|   Description   :  Return the address of the buffer for the field.
|
|   Return Values :  Pointer to buffer or NULL if arguments were invalid.
+--------------------------------------------------------------------------*/
char *field_buffer(const FIELD * field, int  buffer)
{
  if (field && (buffer >= 0) && (buffer <= field->nbuf))
    return Address_Of_Nth_Buffer(field,buffer);
  else
    return (char *)0;
}

/* frm_driver.c ends here */
