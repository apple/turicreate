/*
    Copyright (c) 2014 Wirebird Labs LLC.  All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "sha1.h"

#define sha1_rol32(num,bits) ((num << bits) | (num >> (32 - bits)))

void nn_sha1_init (struct nn_sha1 *self)
{
    /*  Detect endianness. */
    union {
        uint32_t i;
        char c[4];
    } test = { 0x00000001 };

    self->is_little_endian = test.c[0];

    /*  Initial state of the hash. */
    self->state [0] = 0x67452301;
    self->state [1] = 0xefcdab89;
    self->state [2] = 0x98badcfe;
    self->state [3] = 0x10325476;
    self->state [4] = 0xc3d2e1f0;
    self->bytes_hashed = 0;
    self->buffer_offset = 0;
}

static void nn_sha1_add (struct nn_sha1 *self, uint8_t data)
{
    uint8_t i;
    uint32_t a, b, c, d, e, t;
    uint8_t * const buf = (uint8_t*) self->buffer;

    if (self->is_little_endian)
        buf [self->buffer_offset ^ 3] = data;
    else
        buf [self->buffer_offset] = data;

    self->buffer_offset++;
    if (self->buffer_offset == SHA1_BLOCK_LEN) {
        a = self->state [0];
        b = self->state [1];
        c = self->state [2];
        d = self->state [3];
        e = self->state [4];
        for (i = 0; i < 80; i++) {
            if (i >= 16) {
                t = self->buffer [(i + 13) & 15] ^
                    self->buffer [(i + 8) & 15] ^
                    self->buffer [(i + 2) & 15] ^
                    self->buffer [i & 15];
                self->buffer [i & 15] = sha1_rol32 (t, 1);
            }

            if (i < 20)
                t = (d ^ (b & (c ^ d))) + 0x5A827999;
            else if (i < 40)
                t = (b ^ c ^ d) + 0x6ED9EBA1;
            else if (i < 60)
                t = ((b & c) | (d & (b | c))) + 0x8F1BBCDC;
            else
                t = (b ^ c ^ d) + 0xCA62C1D6;

            t += sha1_rol32 (a, 5) + e + self->buffer [i & 15];
            e = d;
            d = c;
            c = sha1_rol32 (b, 30);
            b = a;
            a = t;
        }

        self->state [0] += a;
        self->state [1] += b;
        self->state [2] += c;
        self->state [3] += d;
        self->state [4] += e;

        self->buffer_offset = 0;
    }
}

void nn_sha1_hashbyte (struct nn_sha1 *self, uint8_t data)
{
    ++self->bytes_hashed;
    nn_sha1_add (self, data);
}

uint8_t* nn_sha1_result (struct nn_sha1 *self)
{
    int i;

    /*  Pad to complete the last block. */
    nn_sha1_add (self, 0x80);

    while (self->buffer_offset != 56)
        nn_sha1_add (self, 0x00);

    /*  Append length in the last 8 bytes. SHA-1 supports 64-bit hashes, so
        zero-pad the top bits. Shifting to multiply by 8 as SHA-1 supports
        bit- as well as byte-streams. */
    nn_sha1_add (self, 0);
    nn_sha1_add (self, 0);
    nn_sha1_add (self, 0);
    nn_sha1_add (self, self->bytes_hashed >> 29);
    nn_sha1_add (self, self->bytes_hashed >> 21);
    nn_sha1_add (self, self->bytes_hashed >> 13);
    nn_sha1_add (self, self->bytes_hashed >> 5);
    nn_sha1_add (self, self->bytes_hashed << 3);

    /*  Correct byte order for little-endian systems. */
    if (self->is_little_endian) {
        for (i = 0; i < 5; i++) {
            self->state [i] =
                (((self->state [i]) << 24) & 0xFF000000) |
                (((self->state [i]) << 8) & 0x00FF0000) |
                (((self->state [i]) >> 8) & 0x0000FF00) |
                (((self->state [i]) >> 24) & 0x000000FF);
        }
    }

    /* 20-octet pointer to hash. */
    return (uint8_t*) self->state;
}

