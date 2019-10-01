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
 *  Author: Thomas E. Dickey <dickey@clark.net> 1996,1997                   *
 ****************************************************************************/
/* $Id$ */

#ifndef NC_ALLOC_included
#define NC_ALLOC_included 1

#if HAVE_LIBDMALLOC
#include <dmalloc.h>    /* Gray Watson's library */
#else
#undef  HAVE_LIBDMALLOC
#define HAVE_LIBDMALLOC 0
#endif

#if HAVE_LIBDBMALLOC
#include <dbmalloc.h>   /* Conor Cahill's library */
#else
#undef  HAVE_LIBDBMALLOC
#define HAVE_LIBDBMALLOC 0
#endif

#ifndef NO_LEAKS
#define NO_LEAKS 0
#endif

#if HAVE_LIBDBMALLOC || HAVE_LIBDMALLOC || NO_LEAKS
#define HAVE_NC_FREEALL 1
struct termtype;
extern void _nc_free_and_exit(int) GCC_NORETURN;
extern void _nc_free_tparm(void);
extern void _nc_leaks_dump_entry(void);
#define ExitProgram(code) _nc_free_and_exit(code)
#endif

#ifndef HAVE_NC_FREEALL
#define HAVE_NC_FREEALL 0
#endif

#ifndef ExitProgram
#define ExitProgram(code) return code
#endif

/* doalloc.c */
extern void *_nc_doalloc(void *, size_t);
#if !HAVE_STRDUP
/* #define strdup _nc_strdup */
extern char *_nc_strdup(const char *);
#endif

#define typeMalloc(type,elts) (type *)malloc((elts)*sizeof(type))
#define typeCalloc(type,elts) (type *)calloc((elts),sizeof(type))
#define typeRealloc(type,elts,ptr) (type *)_nc_doalloc(ptr, (elts)*sizeof(type))

#endif /* NC_ALLOC_included */
