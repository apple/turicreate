/*
 * Copyright (c) 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef cmcompress__h_
#define cmcompress__h_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /*
   * Set USERMEM to the maximum amount of physical user memory available
   * in bytes.  USERMEM is used to determine the maximum BITS that can be used
   * for compression.
   *
   * SACREDMEM is the amount of physical memory saved for others; compress
   * will hog the rest.
   */
#ifndef SACREDMEM
#define SACREDMEM  0
#endif

#ifndef USERMEM
# define USERMEM   450000  /* default user memory */
#endif

#ifdef pdp11
# define BITS   12  /* max bits/code for 16-bit machine */
# define NO_UCHAR  /* also if "unsigned char" functions as signed char */
# undef USERMEM
#endif /* pdp11 */  /* don't forget to compile with -i */

#ifdef USERMEM
# if USERMEM >= (433484+SACREDMEM)
#  define PBITS  16
# else
#  if USERMEM >= (229600+SACREDMEM)
#   define PBITS  15
#  else
#   if USERMEM >= (127536+SACREDMEM)
#    define PBITS  14
#   else
#    if USERMEM >= (73464+SACREDMEM)
#     define PBITS  13
#    else
#     define PBITS  12
#    endif
#   endif
#  endif
# endif
# undef USERMEM
#endif /* USERMEM */

#ifdef PBITS    /* Preferred BITS for this memory size */
# ifndef BITS
#  define BITS PBITS
# endif /* BITS */
#endif /* PBITS */

#if BITS == 16
# define HSIZE  69001    /* 95% occupancy */
#endif
#if BITS == 15
# define HSIZE  35023    /* 94% occupancy */
#endif
#if BITS == 14
# define HSIZE  18013    /* 91% occupancy */
#endif
#if BITS == 13
# define HSIZE  9001    /* 91% occupancy */
#endif
#if BITS <= 12
# define HSIZE  5003    /* 80% occupancy */
#endif

  /*
   * a code_int must be able to hold 2**BITS values of type int, and also -1
   */
#if BITS > 15
  typedef long int  code_int;
#else
  typedef int    code_int;
#endif

#ifdef SIGNED_COMPARE_SLOW
  typedef unsigned long int count_int;
  typedef unsigned short int count_short;
#else
  typedef long int    count_int;
#endif

#ifdef NO_UCHAR
  typedef char  char_type;
#else
  typedef  unsigned char  char_type;
#endif /* UCHAR */



  struct cmcompress_stream
    {
    int n_bits;        /* number of bits/code */
    int maxbits;      /* user settable max # bits/code */
    code_int maxcode;      /* maximum code, given n_bits */
    code_int maxmaxcode;  /* should NEVER generate this code */

    count_int htab [HSIZE];
    unsigned short codetab [HSIZE];

    code_int hsize;      /* for dynamic table sizing */
    code_int free_ent;      /* first unused entry */
    int nomagic;  /* Use a 3-byte magic number header, unless old file */

    /*
     * block compression parameters -- after all codes are used up,
     * and compression rate changes, start over.
     */
    int block_compress;
    int clear_flg;
    long int ratio;
    count_int checkpoint;

#ifdef DEBUG
    int debug;
    int verbose;
#endif

    /* compress internals */
    int offset;
    long int in_count;      /* length of input */
    long int bytes_out;      /* length of compressed output */
    long int out_count;      /* # of codes output (for debugging) */

    /* internals */
    code_int ent;
    code_int hsize_reg;
    int hshift;

    long fcode;
    int first_pass;

    /* For input and output */
    int (*input_stream)(void*);
    int (*output_stream)(void*, const char*,int);
    void* client_data;
    };

  int cmcompress_compress_initialize(struct cmcompress_stream* cdata);
  int cmcompress_compress_start(struct cmcompress_stream* cdata);
  int cmcompress_compress(struct cmcompress_stream* cdata, void* buff, size_t n);
  int cmcompress_compress_finalize(struct cmcompress_stream* cdata);

#ifdef __cplusplus
}
#endif


#endif /* cmcompress__h_ */
