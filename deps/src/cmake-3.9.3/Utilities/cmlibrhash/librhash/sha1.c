/* sha1.c - an implementation of Secure Hash Algorithm 1 (SHA1)
 * based on RFC 3174.
 *
 * Copyright: 2008-2012 Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,  including without limitation
 * the rights to  use, copy, modify,  merge, publish, distribute, sublicense,
 * and/or sell copies  of  the Software,  and to permit  persons  to whom the
 * Software is furnished to do so.
 *
 * This program  is  distributed  in  the  hope  that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  Use this program  at  your own risk!
 */

#include <string.h>
#include "byte_order.h"
#include "sha1.h"

/**
 * Initialize context before calculaing hash.
 *
 * @param ctx context to initialize
 */
void rhash_sha1_init(sha1_ctx *ctx)
{
	ctx->length = 0;

	/* initialize algorithm state */
	ctx->hash[0] = 0x67452301;
	ctx->hash[1] = 0xefcdab89;
	ctx->hash[2] = 0x98badcfe;
	ctx->hash[3] = 0x10325476;
	ctx->hash[4] = 0xc3d2e1f0;
}

/**
 * The core transformation. Process a 512-bit block.
 * The function has been taken from RFC 3174 with little changes.
 *
 * @param hash algorithm state
 * @param block the message block to process
 */
static void rhash_sha1_process_block(unsigned* hash, const unsigned* block)
{
	int           t;                 /* Loop counter */
	uint32_t      temp;              /* Temporary word value */
	uint32_t      W[80];             /* Word sequence */
	uint32_t      A, B, C, D, E;     /* Word buffers */

	/* initialize the first 16 words in the array W */
	for (t = 0; t < 16; t++) {
		/* note: it is much faster to apply be2me here, then using be32_copy */
		W[t] = be2me_32(block[t]);
	}

	/* initialize the rest */
	for (t = 16; t < 80; t++) {
		W[t] = ROTL32(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);
	}

	A = hash[0];
	B = hash[1];
	C = hash[2];
	D = hash[3];
	E = hash[4];

	for (t = 0; t < 20; t++) {
		/* the following is faster than ((B & C) | ((~B) & D)) */
		temp =  ROTL32(A, 5) + (((C ^ D) & B) ^ D)
			+ E + W[t] + 0x5A827999;
		E = D;
		D = C;
		C = ROTL32(B, 30);
		B = A;
		A = temp;
	}

	for (t = 20; t < 40; t++) {
		temp = ROTL32(A, 5) + (B ^ C ^ D) + E + W[t] + 0x6ED9EBA1;
		E = D;
		D = C;
		C = ROTL32(B, 30);
		B = A;
		A = temp;
	}

	for (t = 40; t < 60; t++) {
		temp = ROTL32(A, 5) + ((B & C) | (B & D) | (C & D))
			+ E + W[t] + 0x8F1BBCDC;
		E = D;
		D = C;
		C = ROTL32(B, 30);
		B = A;
		A = temp;
	}

	for (t = 60; t < 80; t++) {
		temp = ROTL32(A, 5) + (B ^ C ^ D) + E + W[t] + 0xCA62C1D6;
		E = D;
		D = C;
		C = ROTL32(B, 30);
		B = A;
		A = temp;
	}

	hash[0] += A;
	hash[1] += B;
	hash[2] += C;
	hash[3] += D;
	hash[4] += E;
}

/**
 * Calculate message hash.
 * Can be called repeatedly with chunks of the message to be hashed.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param msg message chunk
 * @param size length of the message chunk
 */
void rhash_sha1_update(sha1_ctx *ctx, const unsigned char* msg, size_t size)
{
	unsigned index = (unsigned)ctx->length & 63;
	ctx->length += size;

	/* fill partial block */
	if (index) {
		unsigned left = sha1_block_size - index;
		memcpy(ctx->message + index, msg, (size < left ? size : left));
		if (size < left) return;

		/* process partial block */
		rhash_sha1_process_block(ctx->hash, (unsigned*)ctx->message);
		msg  += left;
		size -= left;
	}
	while (size >= sha1_block_size) {
		unsigned* aligned_message_block;
		if (IS_ALIGNED_32(msg)) {
			/* the most common case is processing of an already aligned message
			without copying it */
			aligned_message_block = (unsigned*)msg;
		} else {
			memcpy(ctx->message, msg, sha1_block_size);
			aligned_message_block = (unsigned*)ctx->message;
		}

		rhash_sha1_process_block(ctx->hash, aligned_message_block);
		msg  += sha1_block_size;
		size -= sha1_block_size;
	}
	if (size) {
		/* save leftovers */
		memcpy(ctx->message, msg, size);
	}
}

/**
 * Store calculated hash into the given array.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param result calculated hash in binary form
 */
void rhash_sha1_final(sha1_ctx *ctx, unsigned char* result)
{
	unsigned  index = (unsigned)ctx->length & 63;
	unsigned* msg32 = (unsigned*)ctx->message;

	/* pad message and run for last block */
	ctx->message[index++] = 0x80;
	while ((index & 3) != 0) {
		ctx->message[index++] = 0;
	}
	index >>= 2;

	/* if no room left in the message to store 64-bit message length */
	if (index > 14) {
		/* then fill the rest with zeros and process it */
		while (index < 16) {
			msg32[index++] = 0;
		}
		rhash_sha1_process_block(ctx->hash, msg32);
		index = 0;
	}
	while (index < 14) {
		msg32[index++] = 0;
	}
	msg32[14] = be2me_32( (unsigned)(ctx->length >> 29) );
	msg32[15] = be2me_32( (unsigned)(ctx->length << 3) );
	rhash_sha1_process_block(ctx->hash, msg32);

	if (result) be32_copy(result, 0, &ctx->hash, sha1_hash_size);
}
