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

#include "cmcompress.h"

#include <errno.h>
#include <string.h>

static const char_type magic_header[] = { "\037\235" };  /* 1F 9D */

/* Defines for third byte of header */
#define BIT_MASK  0x1f
#define BLOCK_MASK  0x80
#define CHECK_GAP 10000  /* ratio check interval */
/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
   */
#define INIT_BITS 9      /* initial number of bits/code */

#ifdef COMPATIBLE    /* But wrong! */
# define MAXCODE(n_bits)  (1 << (n_bits) - 1)
#else
# define MAXCODE(n_bits)  ((1 << (n_bits)) - 1)
#endif /* COMPATIBLE */

#define htabof(i)  cdata->htab[i]
#define codetabof(i)  cdata->codetab[i]

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */
#define FIRST  257  /* first free entry */
#define  CLEAR  256  /* table clear output code */

#ifdef DEBUG
static void prratio( FILE *stream, long int num, long int den);
#endif

int cmcompress_compress_initialize(struct cmcompress_stream* cdata)
{
  cdata->maxbits = BITS;      /* user settable max # bits/code */
  cdata->maxmaxcode = 1 << BITS;  /* should NEVER generate this code */
  cdata->hsize = HSIZE;      /* for dynamic table sizing */
  cdata->free_ent = 0;      /* first unused entry */
  cdata->nomagic = 0;  /* Use a 3-byte magic number header, unless old file */
  cdata->block_compress = BLOCK_MASK;
  cdata->clear_flg = 0;
  cdata->ratio = 0;
  cdata->checkpoint = CHECK_GAP;

  cdata->input_stream = 0;
  cdata->output_stream = 0;
  cdata->client_data = 0;
  return 1;
}

static void cl_hash(struct cmcompress_stream* cdata, count_int hsize)    /* reset code table */
{
  register count_int *htab_p = cdata->htab+hsize;
  register long i;
  register long m1 = -1;

  i = hsize - 16;
  do
    {        /* might use Sys V memset(3) here */
    *(htab_p-16) = m1;
    *(htab_p-15) = m1;
    *(htab_p-14) = m1;
    *(htab_p-13) = m1;
    *(htab_p-12) = m1;
    *(htab_p-11) = m1;
    *(htab_p-10) = m1;
    *(htab_p-9) = m1;
    *(htab_p-8) = m1;
    *(htab_p-7) = m1;
    *(htab_p-6) = m1;
    *(htab_p-5) = m1;
    *(htab_p-4) = m1;
    *(htab_p-3) = m1;
    *(htab_p-2) = m1;
    *(htab_p-1) = m1;
    htab_p -= 16;
    }
  while ((i -= 16) >= 0);
  for ( i += 16; i > 0; i-- )
    {
    *--htab_p = m1;
    }
}

/*-
 * Output the given code.
 * Inputs:
 *   code:  A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *    that n_bits =< (long)wordsize - 1.
 * Outputs:
 *   Outputs code to the file.
 * Assumptions:
 *  Chars are 8 bits long.
 * Algorithm:
 *   Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static char buf[BITS];

#ifndef vax
char_type lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
#endif /* vax */

static int output(struct cmcompress_stream* cdata, code_int  code)
{
#ifdef DEBUG
  static int col = 0;
#endif /* DEBUG */

  /*
   * On the VAX, it is important to have the register declarations
   * in exactly the order given, or the asm will break.
   */
  register int r_off = cdata->offset, bits= cdata->n_bits;
  register char * bp = buf;

#ifdef DEBUG
  if ( verbose )
    {
    fprintf( stderr, "%5d%c", code,
      (col+=6) >= 74 ? (col = 0, '\n') : ' ' );
    }
#endif /* DEBUG */
  if ( code >= 0 )
    {
#if defined(vax) && !defined(__GNUC__)
    /*
     * VAX and PCC DEPENDENT!! Implementation on other machines is
     * below.
     *
     * Translation: Insert BITS bits from the argument starting at
     * cdata->offset bits from the beginning of buf.
     */
    0;  /* Work around for pcc -O bug with asm and if stmt */
    asm( "insv  4(ap),r11,r10,(r9)" );
#else
    /*
     * byte/bit numbering on the VAX is simulated by the following code
     */
    /*
     * Get to the first byte.
     */
    bp += (r_off >> 3);
    r_off &= 7;
    /*
     * Since code is always >= 8 bits, only need to mask the first
     * hunk on the left.
     */
    *bp = (char)((*bp & rmask[r_off]) | ((code << r_off) & lmask[r_off]));
    bp++;
    bits -= (8 - r_off);
    code >>= 8 - r_off;
    /* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
    if ( bits >= 8 )
      {
      *bp++ = (char)(code);
      code >>= 8;
      bits -= 8;
      }
    /* Last bits. */
    if(bits)
      {
      *bp = (char)(code);
      }
#endif /* vax */
    cdata->offset += cdata->n_bits;
    if ( cdata->offset == (cdata->n_bits << 3) )
      {
      bp = buf;
      bits = cdata->n_bits;
      cdata->bytes_out += bits;
      do
        {
        if ( cdata->output_stream(cdata, bp, 1) != 1 )
          {
          return 0;
          }
        bp++;
        }
      while(--bits);
      cdata->offset = 0;
      }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
    if ( cdata->free_ent > cdata->maxcode || (cdata->clear_flg > 0))
      {
      /*
       * Write the whole buffer, because the input side won't
       * discover the size increase until after it has read it.
       */
      if ( cdata->offset > 0 )
        {
        if ( cdata->output_stream(cdata, buf, cdata->n_bits) != cdata->n_bits )
          {
          return 0;
          }
        cdata->bytes_out += cdata->n_bits;
        }
      cdata->offset = 0;

      if ( cdata->clear_flg )
        {
        cdata->maxcode = MAXCODE (cdata->n_bits = INIT_BITS);
        cdata->clear_flg = 0;
        }
      else
        {
        cdata->n_bits++;
        if ( cdata->n_bits == cdata->maxbits )
          {
          cdata->maxcode = cdata->maxmaxcode;
          }
        else
          {
          cdata->maxcode = MAXCODE(cdata->n_bits);
          }
        }
#ifdef DEBUG
      if ( debug )
        {
        fprintf( stderr, "\nChange to %d bits\n", cdata->n_bits );
        col = 0;
        }
#endif /* DEBUG */
      }
    }
  else
    {
    /*
     * At EOF, write the rest of the buffer.
     */
    if ( cdata->offset > 0 )
      {
      cdata->offset = (cdata->offset + 7) / 8;
      if ( cdata->output_stream(cdata, buf, cdata->offset ) != cdata->offset )
        {
        return 0;
        }
      cdata->bytes_out += cdata->offset;
      }
    cdata->offset = 0;
    (void)fflush( stdout );
    if( ferror( stdout ) )
      {
      return 0;
      }
#ifdef DEBUG
    if ( verbose )
      {
      fprintf( stderr, "\n" );
      }
#endif
    }
  return 1;
}

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

int cmcompress_compress_start(struct cmcompress_stream* cdata)
{
#ifndef COMPATIBLE
  if (cdata->nomagic == 0)
    {
    char headLast = (char)(cdata->maxbits | cdata->block_compress);
    cdata->output_stream(cdata, (const char*)magic_header, 2);
    cdata->output_stream(cdata, &headLast, 1);
    if(ferror(stdout))
      {
      printf("Error...\n");
      }
    }
#endif /* COMPATIBLE */

  cdata->offset = 0;
  cdata->bytes_out = 3;    /* includes 3-byte header mojo */
  cdata->out_count = 0;
  cdata->clear_flg = 0;
  cdata->ratio = 0;
  cdata->in_count = 1;
  cdata->checkpoint = CHECK_GAP;
  cdata->maxcode = MAXCODE(cdata->n_bits = INIT_BITS);
  cdata->free_ent = ((cdata->block_compress) ? FIRST : 256 );

  cdata->first_pass = 1;

  cdata->hshift = 0;
  for ( cdata->fcode = (long) cdata->hsize;  cdata->fcode < 65536L; cdata->fcode *= 2L )
    {
    cdata->hshift++;
    }
  cdata->hshift = 8 - cdata->hshift;    /* set hash code range bound */

  cdata->hsize_reg = cdata->hsize;
  cl_hash(cdata, (count_int) cdata->hsize_reg);    /* clear hash table */

  return 1;
}

static int cl_block (struct cmcompress_stream* cdata)    /* table clear for block compress */
{
  register long int rat;

  cdata->checkpoint = cdata->in_count + CHECK_GAP;
#ifdef DEBUG
  if ( cdata->debug )
    {
    fprintf ( stderr, "count: %ld, ratio: ", cdata->in_count );
    prratio ( stderr, cdata->in_count, cdata->bytes_out );
    fprintf ( stderr, "\n");
    }
#endif /* DEBUG */

  if(cdata->in_count > 0x007fffff)
    {  /* shift will overflow */
    rat = cdata->bytes_out >> 8;
    if(rat == 0)
      {    /* Don't divide by zero */
      rat = 0x7fffffff;
      }
    else
      {
      rat = cdata->in_count / rat;
      }
    }
  else
    {
    rat = (cdata->in_count << 8) / cdata->bytes_out;  /* 8 fractional bits */
    }
  if ( rat > cdata->ratio )
    {
    cdata->ratio = rat;
    }
  else
    {
    cdata->ratio = 0;
#ifdef DEBUG
    if(cdata->verbose)
      {
      dump_tab();  /* dump string table */
      }
#endif
    cl_hash (cdata, (count_int) cdata->hsize );
    cdata->free_ent = FIRST;
    cdata->clear_flg = 1;
    if ( !output (cdata, (code_int) CLEAR ) )
      {
      return 0;
      }
#ifdef DEBUG
    if(cdata->debug)
      {
      fprintf ( stderr, "clear\n" );
      }
#endif /* DEBUG */
    }
  return 1;
}


int cmcompress_compress(struct cmcompress_stream* cdata, void* buff, size_t n)
{
  register code_int i;
  register int c;
  register int disp;

  unsigned char* input_buffer = (unsigned char*)buff;

  size_t cc;

  /*printf("cmcompress_compress(%p, %p, %d)\n", cdata, buff, n);*/

  if ( cdata->first_pass )
    {
    cdata->ent = input_buffer[0];
    ++ input_buffer;
    -- n;
    cdata->first_pass = 0;
    }

  for ( cc = 0; cc < n; ++ cc )
    {
    c = input_buffer[cc];
    cdata->in_count++;
    cdata->fcode = (long) (((long) c << cdata->maxbits) + cdata->ent);
    i = ((c << cdata->hshift) ^ cdata->ent);  /* xor hashing */

    if ( htabof (i) == cdata->fcode )
      {
      cdata->ent = codetabof (i);
      continue;
      }
    else if ( (long)htabof (i) < 0 )  /* empty slot */
      {
      goto nomatch;
      }
    disp = (int)(cdata->hsize_reg - i);    /* secondary hash (after G. Knott) */
    if ( i == 0 )
      {
      disp = 1;
      }
probe:
    if ( (i -= disp) < 0 )
      {
      i += cdata->hsize_reg;
      }

    if ( htabof (i) == cdata->fcode )
      {
      cdata->ent = codetabof (i);
      continue;
      }
    if ( (long)htabof (i) > 0 )
      {
      goto probe;
      }
nomatch:
    if ( !output(cdata, (code_int) cdata->ent ) )
      {
      return 0;
      }
    cdata->out_count++;
    cdata->ent = c;
    if (
#ifdef SIGNED_COMPARE_SLOW
      (unsigned) cdata->free_ent < (unsigned) cdata->maxmaxcode
#else
      cdata->free_ent < cdata->maxmaxcode
#endif
    )
      {
      codetabof (i) = (unsigned short)(cdata->free_ent++);  /* code -> hashtable */
      htabof (i) = cdata->fcode;
      }
    else if ( (count_int)cdata->in_count >= cdata->checkpoint && cdata->block_compress )
      {
      if ( !cl_block (cdata) )
        {
        return 0;
        }
      }
    }

  return 1;
}

int cmcompress_compress_finalize(struct cmcompress_stream* cdata)
{
  /*
   * Put out the final code.
   */
  if ( !output(cdata, (code_int)cdata->ent ) )
    {
    return 0;
    }
  cdata->out_count++;
  if ( !output(cdata, (code_int)-1 ) )
    {
    return 0;
    }

  if(cdata->bytes_out > cdata->in_count)  /* exit(2) if no savings */
    {
    return 0;
    }
  return 1;
}


#if defined(DEBUG)
static void prratio(FILE *stream, long int num, long int den)
{
  register int q;      /* Doesn't need to be long */

  if(num > 214748L)
    {    /* 2147483647/10000 */
    q = num / (den / 10000L);
    }
  else
    {
    q = 10000L * num / den;    /* Long calculations, though */
    }
  if (q < 0)
    {
    putc('-', stream);
    q = -q;
    }
  fprintf(stream, "%d.%02d%%", q / 100, q % 100);
}
#endif

