/* hex.c - conversion for hexadecimal and base32 strings.
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
#include <ctype.h>
#include "hex.h"

/**
* Convert a byte to a hexadecimal number. The result, consisting of two
* hexadecimal digits is stored into a buffer.
 *
 * @param dest  the buffer to receive two symbols of hex representation
 * @param byte the byte to decode
 * @param upper_case flag to print string in uppercase
 * @return pointer to the chararcter just after the written number (dest + 2)
 */
char* rhash_print_hex_byte(char *dest, const unsigned char byte, int upper_case)
{
	const char add = (upper_case ? 'A' - 10 : 'a' - 10);
	unsigned char c = (byte >> 4) & 15;
	*dest++ = (c > 9 ? c + add : c + '0');
	c = byte & 15;
	*dest++ = (c > 9 ? c + add : c + '0');
	return dest;
}

/**
 * Store hexadecimal representation of a binary string to given buffer.
 *
 * @param dest the buffer to receive hexadecimal representation
 * @param src binary string
 * @param len string length
 * @param upper_case flag to print string in uppercase
 */
void rhash_byte_to_hex(char *dest, const unsigned char *src, unsigned len, int upper_case)
{
	while (len-- > 0) {
		dest = rhash_print_hex_byte(dest, *src++, upper_case);
	}
	*dest = '\0';
}

/**
 * Encode a binary string to base32.
 *
 * @param dest the buffer to store result
 * @param src binary string
 * @param len string length
 * @param upper_case flag to print string in uppercase
 */
void rhash_byte_to_base32(char* dest, const unsigned char* src, unsigned len, int upper_case)
{
	const char a = (upper_case ? 'A' : 'a');
	unsigned shift = 0;
	unsigned char word;
	const unsigned char* e = src + len;
	while (src < e) {
		if (shift > 3) {
			word = (*src & (0xFF >> shift));
			shift = (shift + 5) % 8;
			word <<= shift;
			if (src + 1 < e)
				word |= *(src + 1) >> (8 - shift);
			++src;
		} else {
			shift = (shift + 5) % 8;
			word = ( *src >> ( (8 - shift) & 7 ) ) & 0x1F;
			if (shift == 0) src++;
		}
		*dest++ = ( word < 26 ? word + a : word + '2' - 26 );
	}
	*dest = '\0';
}

/**
 * Encode a binary string to base64.
 * Encoded output length is always a multiple of 4 bytes.
 *
 * @param dest the buffer to store result
 * @param src binary string
 * @param len string length
 */
void rhash_byte_to_base64(char* dest, const unsigned char* src, unsigned len)
{
	static const char* tail = "0123456789+/";
	unsigned shift = 0;
	unsigned char word;
	const unsigned char* e = src + len;
	while (src < e) {
		if (shift > 2) {
			word = (*src & (0xFF >> shift));
			shift = (shift + 6) % 8;
			word <<= shift;
			if (src + 1 < e)
				word |= *(src + 1) >> (8 - shift);
			++src;
		} else {
			shift = (shift + 6) % 8;
			word = ( *src >> ( (8 - shift) & 7 ) ) & 0x3F;
			if (shift == 0) src++;
		}
		*dest++ = ( word < 52 ? (word < 26 ? word + 'A' : word - 26 + 'a') : tail[word - 52]);
	}
	if (shift > 0) {
		*dest++ = '=';
		if (shift == 4) *dest++ = '=';
	}
	*dest = '\0';
}

/* unsafe characters are "<>{}[]%#/|\^~`@:;?=&+ */
#define IS_GOOD_URL_CHAR(c) (isalnum((unsigned char)c) || strchr("$-_.!'(),", c))

/**
 * URL-encode a string.
 *
 * @param dst buffer to receive result or NULL to calculate
 *    the lengths of encoded string
 * @param filename the file name
 * @return the length of the result string
 */
int rhash_urlencode(char *dst, const char *name)
{
	const char *start;
	if (!dst) {
		int len;
		for (len = 0; *name; name++) len += (IS_GOOD_URL_CHAR(*name) ? 1 : 3);
		return len;
	}
	/* encode URL as specified by RFC 1738 */
	for (start = dst; *name; name++) {
		if ( IS_GOOD_URL_CHAR(*name) ) {
			*dst++ = *name;
		} else {
			*dst++ = '%';
			dst = rhash_print_hex_byte(dst, *name, 'A');
		}
	}
	*dst = 0;
	return (int)(dst - start);
}

/**
 * Print 64-bit number with trailing '\0' to a string buffer.
 * if dst is NULL, then just return the length of the number.
 *
 * @param dst output buffer
 * @param number the number to print
 * @return length of the printed number (without trailing '\0')
 */
int rhash_sprintI64(char *dst, uint64_t number)
{
	/* The biggest number has 20 digits: 2^64 = 18 446 744 073 709 551 616 */
	char buf[24], *p;
	size_t length;

	if (dst == NULL) {
		/* just calculate the length of the number */
		if (number == 0) return 1;
		for (length = 0; number != 0; number /= 10) length++;
		return (int)length;
	}

	p = buf + 23;
	*p = '\0'; /* last symbol should be '\0' */
	if (number == 0) {
		*(--p) = '0';
	} else {
		for (; p >= buf && number != 0; number /= 10) {
			*(--p) = '0' + (char)(number % 10);
		}
	}
	length = buf + 23 - p;
	memcpy(dst, p, length + 1);
	return (int)length;
}
