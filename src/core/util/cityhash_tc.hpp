/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CITYHASH_GL_H_
#define TURI_CITYHASH_GL_H_

#include <vector>
#include <core/util/code_optimization.hpp>

// Copyright (c) 2011 Google, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// CityHash, by Geoff Pike and Jyrki Alakuijala
//
// This file provides CityHash64() and related functions.

// This file is a merged header of the CityHash functions with a set
// of convenience functions.

// From CityHash comments:
//
// It's probably possible to create even faster hash functions by
// writing a program that systematically explores some of the space of
// possible hash functions, by using SIMD instructions, or by
// compromising on hash quality.
//
// http://code.google.com/p/cityhash/
//
// This file provides a few functions for hashing strings.  All of them are
// high-quality functions in the sense that they pass standard tests such
// as Austin Appleby's SMHasher.  They are also fast.
//
// For 64-bit x86 code, on short strings, we don't know of anything faster than
// CityHash64 that is of comparable quality.  We believe our nearest competitor
// is Murmur3.  For 64-bit x86 code, CityHash64 is an excellent choice for hash
// tables and most other hashing (excluding cryptography).
//
// For 64-bit x86 code, on long strings, the picture is more complicated.
// On many recent Intel CPUs, such as Nehalem, Westmere, Sandy Bridge, etc.,
// CityHashCrc128 appears to be faster than all competitors of comparable
// quality.  CityHash128 is also good but not quite as fast.  We believe our
// nearest competitor is Bob Jenkins' Spooky.  We don't have great data for
// other 64-bit CPUs, but for long strings we know that Spooky is slightly
// faster than CityHash on some relatively recent AMD x86-64 CPUs, for example.
// Note that CityHashCrc128 is declared in citycrc.h.
//
// For 32-bit x86 code, we don't know of anything faster than CityHash32 that
// is of comparable quality.  We believe our nearest competitor is Murmur3A.
// (On 64-bit CPUs, it is typically faster to use the other CityHash variants.)
//
// Functions in the CityHash family are not suitable for cryptography.
//
// Please see CityHash's README file for more details on our performance
// measurements and so on.
//
// WARNING: This code has been only lightly tested on big-endian platforms!
// It is known to work well on little-endian platforms that have a small penalty
// for unaligned reads, such as current Intel and AMD moderate-to-high-end CPUs.
// It should work on all 32-bit and 64-bit platforms that allow unaligned reads;
// bug reports are welcome.
//
// By the way, for some hash functions, given strings a and b, the hash
// of a+b is easily derived from the hashes of a and b.  This property
// doesn't hold for any hash functions in this file.

#include <algorithm>
#include <string.h>  // for memcpy and memset
#include <cstdint>

#include "int128_types.hpp"

#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif

// Local macros
#if defined( _MSC_VER) || defined(_WIN32)

#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

#ifdef WORDS_BIGENDIAN
#define uint32_t_in_expected_order(x) (bswap_32(x))
#define uint64_t_in_expected_order(x) (bswap_64(x))
#else
#define uint32_t_in_expected_order(x) (x)
#define uint64_t_in_expected_order(x) (x)
#endif

#define _CH_PERMUTE3(a, b, c) do { std::swap(a, b); std::swap(a, c); } while (0)

namespace turi {

namespace cityhash_local {

typedef std::pair<uint64_t, uint64_t> local_uint128;

static inline uint64_t Uint128Low64(const local_uint128& x) {
  return x.first;
}

static inline uint64_t Uint128High64(const local_uint128& x) {
  return x.second;
}

static inline uint64_t UNALIGNED_LOAD64(const char *p) {
  uint64_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

static inline uint32_t UNALIGNED_LOAD32(const char *p) {
  uint32_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

static inline uint64_t Hash128to64(const local_uint128& x) {
  // Murmur-inspired hashing.
  const uint64_t kMul = 0x9ddfea08eb382d69ULL;
  uint64_t a = (Uint128Low64(x) ^ Uint128High64(x)) * kMul;
  a ^= (a >> 47);
  uint64_t b = (Uint128High64(x) ^ a) * kMul;
  b ^= (b >> 47);
  b *= kMul;
  return b;
}

static inline uint64_t Fetch64(const char *p) {
  return uint64_t_in_expected_order(UNALIGNED_LOAD64(p));
}

static inline uint32_t Fetch32(const char *p) {
  return uint32_t_in_expected_order(UNALIGNED_LOAD32(p));
}

// Some primes between 2^63 and 2^64 for various uses.
static const uint64_t k0 = 0xc3a5c85c97cb3127ULL;
static const uint64_t k1 = 0xb492b66fbe98f273ULL;
static const uint64_t k2 = 0x9ae16a3b2f90404fULL;

// Magic numbers for 32-bit hashing.  Copied from Murmur3.
static const uint32_t c1 = 0xcc9e2d51;
static const uint32_t c2 = 0x1b873593;

// A 32-bit to 32-bit integer hash copied from Murmur3.
static inline uint32_t fmix(uint32_t h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

static inline uint32_t Rotate32(uint32_t val, int shift) {
  // Avoid shifting by 32: doing so yields an undefined result.
  return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

static inline uint32_t Mur(uint32_t a, uint32_t h) {
  // Helper from Murmur3 for combining two 32-bit values.
  a *= c1;
  a = Rotate32(a, 17);
  a *= c2;
  h ^= a;
  h = Rotate32(h, 19);
  return h * 5 + 0xe6546b64;
}

static inline uint32_t Hash32Len13to24(const char *s, size_t len) {
  uint32_t a = Fetch32(s - 4 + (len >> 1));
  uint32_t b = Fetch32(s + 4);
  uint32_t c = Fetch32(s + len - 8);
  uint32_t d = Fetch32(s + (len >> 1));
  uint32_t e = Fetch32(s);
  uint32_t f = Fetch32(s + len - 4);
  uint32_t h = len;

  return fmix(Mur(f, Mur(e, Mur(d, Mur(c, Mur(b, Mur(a, h)))))));
}

static inline uint32_t Hash32Len0to4(const char *s, size_t len) {
  uint32_t b = 0;
  uint32_t c = 9;
  for (size_t i = 0; i < len; i++) {
    signed char v = s[i];
    b = b * c1 + v;
    c ^= b;
  }
  return fmix(Mur(b, Mur(len, c)));
}

static inline uint32_t Hash32Len5to12(const char *s, size_t len) {
  uint32_t a = len, b = len * 5, c = 9, d = b;
  a += Fetch32(s);
  b += Fetch32(s + len - 4);
  c += Fetch32(s + ((len >> 1) & 4));
  return fmix(Mur(c, Mur(b, Mur(a, d))));
}

static inline uint32_t CityHash32(const char *s, size_t len) {
  if (len <= 24) {
    return len <= 12 ?
        (len <= 4 ? Hash32Len0to4(s, len) : Hash32Len5to12(s, len)) :
        Hash32Len13to24(s, len);
  }

  // len > 24
  uint32_t h = len, g = c1 * len, f = g;
  uint32_t a0 = Rotate32(Fetch32(s + len - 4) * c1, 17) * c2;
  uint32_t a1 = Rotate32(Fetch32(s + len - 8) * c1, 17) * c2;
  uint32_t a2 = Rotate32(Fetch32(s + len - 16) * c1, 17) * c2;
  uint32_t a3 = Rotate32(Fetch32(s + len - 12) * c1, 17) * c2;
  uint32_t a4 = Rotate32(Fetch32(s + len - 20) * c1, 17) * c2;
  h ^= a0;
  h = Rotate32(h, 19);
  h = h * 5 + 0xe6546b64;
  h ^= a2;
  h = Rotate32(h, 19);
  h = h * 5 + 0xe6546b64;
  g ^= a1;
  g = Rotate32(g, 19);
  g = g * 5 + 0xe6546b64;
  g ^= a3;
  g = Rotate32(g, 19);
  g = g * 5 + 0xe6546b64;
  f += a4;
  f = Rotate32(f, 19);
  f = f * 5 + 0xe6546b64;
  size_t iters = (len - 1) / 20;
  do {
    uint32_t a0 = Rotate32(Fetch32(s) * c1, 17) * c2;
    uint32_t a1 = Fetch32(s + 4);
    uint32_t a2 = Rotate32(Fetch32(s + 8) * c1, 17) * c2;
    uint32_t a3 = Rotate32(Fetch32(s + 12) * c1, 17) * c2;
    uint32_t a4 = Fetch32(s + 16);
    h ^= a0;
    h = Rotate32(h, 18);
    h = h * 5 + 0xe6546b64;
    f += a1;
    f = Rotate32(f, 19);
    f = f * c1;
    g += a2;
    g = Rotate32(g, 18);
    g = g * 5 + 0xe6546b64;
    h ^= a3 + a1;
    h = Rotate32(h, 19);
    h = h * 5 + 0xe6546b64;
    g ^= a4;
    g = bswap_32(g) * 5;
    h += a4 * 5;
    h = bswap_32(h);
    f += a0;
    _CH_PERMUTE3(f, h, g);
    s += 20;
  } while (--iters != 0);
  g = Rotate32(g, 11) * c1;
  g = Rotate32(g, 17) * c1;
  f = Rotate32(f, 11) * c1;
  f = Rotate32(f, 17) * c1;
  h = Rotate32(h + g, 19);
  h = h * 5 + 0xe6546b64;
  h = Rotate32(h, 17) * c1;
  h = Rotate32(h + f, 19);
  h = h * 5 + 0xe6546b64;
  h = Rotate32(h, 17) * c1;
  return h;
}

// Bitwise right rotate.  Normally this will compile to a single
// instruction, especially if the shift is a manifest constant.
static inline uint64_t Rotate(uint64_t val, int shift) {
  // Avoid shifting by 64: doing so yields an undefined result.
  return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

static inline uint64_t ShiftMix(uint64_t val) {
  return val ^ (val >> 47);
}

static inline uint64_t HashLen16(uint64_t u, uint64_t v) {
  return Hash128to64(local_uint128(u, v));
}

static inline uint64_t HashLen16(uint64_t u, uint64_t v, uint64_t mul) {
  // Murmur-inspired hashing.
  uint64_t a = (u ^ v) * mul;
  a ^= (a >> 47);
  uint64_t b = (v ^ a) * mul;
  b ^= (b >> 47);
  b *= mul;
  return b;
}

static inline uint64_t HashLen0to16(const char *s, size_t len) {
  if (len >= 8) {
    uint64_t mul = k2 + len * 2;
    uint64_t a = Fetch64(s) + k2;
    uint64_t b = Fetch64(s + len - 8);
    uint64_t c = Rotate(b, 37) * mul + a;
    uint64_t d = (Rotate(a, 25) + b) * mul;
    return HashLen16(c, d, mul);
  }
  if (len >= 4) {
    uint64_t mul = k2 + len * 2;
    uint64_t a = Fetch32(s);
    return HashLen16(len + (a << 3), Fetch32(s + len - 4), mul);
  }
  if (len > 0) {
    uint8_t a = s[0];
    uint8_t b = s[len >> 1];
    uint8_t c = s[len - 1];
    uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
    uint32_t z = len + (static_cast<uint32_t>(c) << 2);
    return ShiftMix(y * k2 ^ z * k0) * k2;
  }
  return k2;
}

// This probably works well for 16-byte strings as well, but it may be overkill
// in that case.
static inline uint64_t HashLen17to32(const char *s, size_t len) {
  uint64_t mul = k2 + len * 2;
  uint64_t a = Fetch64(s) * k1;
  uint64_t b = Fetch64(s + 8);
  uint64_t c = Fetch64(s + len - 8) * mul;
  uint64_t d = Fetch64(s + len - 16) * k2;
  return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d,
                   a + Rotate(b + k2, 18) + c, mul);
}

// Return a 16-byte hash for 48 bytes.  Quick and dirty.
// Callers do best to use "random-looking" values for a and b.
static inline std::pair<uint64_t, uint64_t> WeakHashLen32WithSeeds(
    uint64_t w, uint64_t x, uint64_t y, uint64_t z, uint64_t a, uint64_t b) {
  a += w;
  b = Rotate(b + a + z, 21);
  uint64_t c = a;
  a += x;
  a += y;
  b += Rotate(a, 44);
  return std::make_pair(a + z, b + c);
}

// Return a 16-byte hash for s[0] ... s[31], a, and b.  Quick and dirty.
static inline std::pair<uint64_t, uint64_t> WeakHashLen32WithSeeds(
    const char* s, uint64_t a, uint64_t b) {
  return WeakHashLen32WithSeeds(Fetch64(s),
                                Fetch64(s + 8),
                                Fetch64(s + 16),
                                Fetch64(s + 24),
                                a,
                                b);
}

// Return an 8-byte hash for 33 to 64 bytes.
static uint64_t HashLen33to64(const char *s, size_t len) {
  uint64_t mul = k2 + len * 2;
  uint64_t a = Fetch64(s) * k2;
  uint64_t b = Fetch64(s + 8);
  uint64_t c = Fetch64(s + len - 24);
  uint64_t d = Fetch64(s + len - 32);
  uint64_t e = Fetch64(s + 16) * k2;
  uint64_t f = Fetch64(s + 24) * 9;
  uint64_t g = Fetch64(s + len - 8);
  uint64_t h = Fetch64(s + len - 16) * mul;
  uint64_t u = Rotate(a + g, 43) + (Rotate(b, 30) + c) * 9;
  uint64_t v = ((a + g) ^ d) + f + 1;
  uint64_t w = bswap_64((u + v) * mul) + h;
  uint64_t x = Rotate(e + f, 42) + c;
  uint64_t y = (bswap_64((v + w) * mul) + g) * mul;
  uint64_t z = e + f + c;
  a = bswap_64((x + z) * mul + y) + b;
  b = ShiftMix((z + a) * mul + d + h) * mul;
  return b + x;
}

static inline uint64_t CityHash64(const char *s, size_t len) {
  if (len <= 32) {
    if (len <= 16) {
      return HashLen0to16(s, len);
    } else {
      return HashLen17to32(s, len);
    }
  } else if (len <= 64) {
    return HashLen33to64(s, len);
  }

  // For strings over 64 bytes we hash the end first, and then as we
  // loop we keep 56 bytes of state: v, w, x, y, and z.
  uint64_t x = Fetch64(s + len - 40);
  uint64_t y = Fetch64(s + len - 16) + Fetch64(s + len - 56);
  uint64_t z = HashLen16(Fetch64(s + len - 48) + len, Fetch64(s + len - 24));
  std::pair<uint64_t, uint64_t> v = WeakHashLen32WithSeeds(s + len - 64, len, z);
  std::pair<uint64_t, uint64_t> w = WeakHashLen32WithSeeds(s + len - 32, y + k1, x);
  x = x * k1 + Fetch64(s);

  // Decrease len to the nearest multiple of 64, and operate on 64-byte chunks.
  len = (len - 1) & ~static_cast<size_t>(63);
  do {
    x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
    y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
    x ^= w.second;
    y += v.first + Fetch64(s + 40);
    z = Rotate(z + w.first, 33) * k1;
    v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
    w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
    std::swap(z, x);
    s += 64;
    len -= 64;
  } while (len != 0);
  return HashLen16(HashLen16(v.first, w.first) + ShiftMix(y) * k1 + z,
                   HashLen16(v.second, w.second) + x);
}

static inline uint64_t CityHash64WithSeeds(const char *s, size_t len,
                                           uint64_t seed0, uint64_t seed1) {
  return HashLen16(CityHash64(s, len) - seed0, seed1);
}

static inline uint64_t CityHash64WithSeed(const char *s, size_t len, uint64_t seed) {
  return CityHash64WithSeeds(s, len, k2, seed);
}

// A subroutine for CityHash128().  Returns a decent 128-bit hash for strings
// of any length representable in signed long.  Based on City and Murmur.
static inline local_uint128 CityMurmur(const char *s, size_t len, local_uint128 seed) {
  uint64_t a = Uint128Low64(seed);
  uint64_t b = Uint128High64(seed);
  uint64_t c = 0;
  uint64_t d = 0;
  signed long l = len - 16;
  if (l <= 0) {  // len <= 16
    a = ShiftMix(a * k1) * k1;
    c = b * k1 + HashLen0to16(s, len);
    d = ShiftMix(a + (len >= 8 ? Fetch64(s) : c));
  } else {  // len > 16
    c = HashLen16(Fetch64(s + len - 8) + k1, a);
    d = HashLen16(b + len, c + Fetch64(s + len - 16));
    a += d;
    do {
      a ^= ShiftMix(Fetch64(s) * k1) * k1;
      a *= k1;
      b ^= a;
      c ^= ShiftMix(Fetch64(s + 8) * k1) * k1;
      c *= k1;
      d ^= c;
      s += 16;
      l -= 16;
    } while (l > 0);
  }
  a = HashLen16(a, c);
  b = HashLen16(d, b);
  return local_uint128(a ^ b, HashLen16(b, a));
}

static inline local_uint128 CityHash128WithSeed(const char *s, size_t len, local_uint128 seed) {
  if (len < 128) {
    return CityMurmur(s, len, seed);
  }

  // We expect len >= 128 to be the common case.  Keep 56 bytes of state:
  // v, w, x, y, and z.
  std::pair<uint64_t, uint64_t> v, w;
  uint64_t x = Uint128Low64(seed);
  uint64_t y = Uint128High64(seed);
  uint64_t z = len * k1;
  v.first = Rotate(y ^ k1, 49) * k1 + Fetch64(s);
  v.second = Rotate(v.first, 42) * k1 + Fetch64(s + 8);
  w.first = Rotate(y + z, 35) * k1 + x;
  w.second = Rotate(x + Fetch64(s + 88), 53) * k1;

  // This is the same inner loop as CityHash64(), manually unrolled.
  do {
    x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
    y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
    x ^= w.second;
    y += v.first + Fetch64(s + 40);
    z = Rotate(z + w.first, 33) * k1;
    v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
    w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
    std::swap(z, x);
    s += 64;
    x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
    y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
    x ^= w.second;
    y += v.first + Fetch64(s + 40);
    z = Rotate(z + w.first, 33) * k1;
    v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
    w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
    std::swap(z, x);
    s += 64;
    len -= 128;
  } while (LIKELY(len >= 128));
  x += Rotate(v.first + z, 49) * k0;
  y = y * k0 + Rotate(w.second, 37);
  z = z * k0 + Rotate(w.first, 27);
  w.first *= 9;
  v.first *= k0;
  // If 0 < len < 128, hash up to 4 chunks of 32 bytes each from the end of s.
  for (size_t tail_done = 0; tail_done < len; ) {
    tail_done += 32;
    y = Rotate(x + y, 42) * k0 + v.second;
    w.first += Fetch64(s + len - tail_done + 16);
    x = x * k0 + w.first;
    z += w.second + Fetch64(s + len - tail_done);
    w.second += v.first;
    v = WeakHashLen32WithSeeds(s + len - tail_done, v.first + z, v.second);
    v.first *= k0;
  }
  // At this point our 56 bytes of state should contain more than
  // enough information for a strong 128-bit hash.  We use two
  // different 56-byte-to-8-byte hashes to get a 16-byte final result.
  x = HashLen16(x, v.first);
  y = HashLen16(y + z, w.first);
  return local_uint128(HashLen16(x + v.second, w.second) + y,
                       HashLen16(x + w.second, y + v.second));
}

static inline local_uint128 CityHash128(const char *s, size_t len) {
  return len >= 16 ?
      CityHash128WithSeed(s + 16, len - 16,
                          local_uint128(Fetch64(s), Fetch64(s + 8) + k0)) :
      CityHash128WithSeed(s, len, local_uint128(k0, k1));
}

#ifdef __SSE4_2__
// Requires len >= 240.
static void CityHashCrc256Long(const char *s, size_t len,
                               uint32_t seed, uint64_t *result) {
  uint64_t a = Fetch64(s + 56) + k0;
  uint64_t b = Fetch64(s + 96) + k0;
  uint64_t c = result[0] = HashLen16(b, len);
  uint64_t d = result[1] = Fetch64(s + 120) * k0 + len;
  uint64_t e = Fetch64(s + 184) + seed;
  uint64_t f = 0;
  uint64_t g = 0;
  uint64_t h = c + d;
  uint64_t x = seed;
  uint64_t y = 0;
  uint64_t z = 0;

  // 240 bytes of input per iter.
  size_t iters = len / 240;
  len -= iters * 240;
  do {
#undef CHUNK
#define CHUNK(r)                                \
    _CH_PERMUTE3(x, z, y);                      \
    b += Fetch64(s);                            \
    c += Fetch64(s + 8);                        \
    d += Fetch64(s + 16);                       \
    e += Fetch64(s + 24);                       \
    f += Fetch64(s + 32);                       \
    a += b;                                     \
    h += f;                                     \
    b += c;                                     \
    f += d;                                     \
    g += e;                                     \
    e += z;                                     \
    g += x;                                     \
    z = _mm_crc32_u64(z, b + g);                \
    y = _mm_crc32_u64(y, e + h);                \
    x = _mm_crc32_u64(x, f + a);                \
    e = Rotate(e, r);                           \
    c += e;                                     \
    s += 40

    CHUNK(0); _CH_PERMUTE3(a, h, c);
    CHUNK(33); _CH_PERMUTE3(a, h, f);
    CHUNK(0); _CH_PERMUTE3(b, h, f);
    CHUNK(42); _CH_PERMUTE3(b, h, d);
    CHUNK(0); _CH_PERMUTE3(b, h, e);
    CHUNK(33); _CH_PERMUTE3(a, h, e);
  } while (--iters > 0);

  while (len >= 40) {
    CHUNK(29);
    e ^= Rotate(a, 20);
    h += Rotate(b, 30);
    g ^= Rotate(c, 40);
    f += Rotate(d, 34);
    _CH_PERMUTE3(c, h, g);
    len -= 40;
  }
  if (len > 0) {
    s = s + len - 40;
    CHUNK(33);
    e ^= Rotate(a, 43);
    h += Rotate(b, 42);
    g ^= Rotate(c, 41);
    f += Rotate(d, 40);
  }
  result[0] ^= h;
  result[1] ^= g;
  g += h;
  a = HashLen16(a, g + z);
  x += y << 32;
  b += x;
  c = HashLen16(c, z) + h;
  d = HashLen16(d, e + result[0]);
  g += e;
  h += HashLen16(x, f);
  e = HashLen16(a, d) + g;
  z = HashLen16(b, c) + a;
  y = HashLen16(g, h) + c;
  result[0] = e + z + y + x;
  a = ShiftMix((a + y) * k0) * k0 + b;
  result[1] += a + result[0];
  a = ShiftMix(a * k0) * k0 + c;
  result[2] = a + result[1];
  a = ShiftMix((a + e) * k0) * k0;
  result[3] = a + result[2];
}

// Requires len < 240.
static inline void CityHashCrc256Short(const char *s, size_t len, uint64_t *result) {
  char buf[240];
  memcpy(buf, s, len);
  memset(buf + len, 0, 240 - len);
  CityHashCrc256Long(buf, 240, ~static_cast<uint32_t>(len), result);
}

static inline void CityHashCrc256(const char *s, size_t len, uint64_t *result) {
  if (LIKELY(len >= 240)) {
    CityHashCrc256Long(s, len, 0, result);
  } else {
    CityHashCrc256Short(s, len, result);
  }
}

static inline local_uint128 CityHashCrc128WithSeed(const char *s, size_t len, local_uint128 seed) {
  if (len <= 900) {
    return CityHash128WithSeed(s, len, seed);
  } else {
    uint64_t result[4];
    CityHashCrc256(s, len, result);
    uint64_t u = Uint128High64(seed) + result[0];
    uint64_t v = Uint128Low64(seed) + result[1];
    return local_uint128(HashLen16(u, v + result[2]),
                         HashLen16(Rotate(v, 32), u * k0 + result[3]));
  }
}

static inline local_uint128 CityHashCrc128(const char *s, size_t len) {
  if (len <= 900) {
    return CityHash128(s, len);
  } else {
    uint64_t result[4];
    CityHashCrc256(s, len, result);
    return local_uint128(result[2], result[3]);
  }
}

#endif
#undef _CH_PERMUTE3


static inline uint64_t SimpleIntegerHash64(uint64_t s) {

  static const uint64_t m = 0xc6a4a7935bd1e995ULL;
  static const int r = 47;

  // XOR with k0 so the hash that maps to 0 is not that common.  Note
  // that this is a one-to-one map from {0,1}^64 -> {0,1}^64, so one
  // hash must map to 0.


  uint64_t k = (uint64_t(s) ^ k0);

  k *= m;
  k ^= k >> r;
  k *= m;

  return k;
}

static inline local_uint128 SimpleIntegerHash128(uint64_t s1, uint64_t s2) {

  // Simply use the murmer hash 2 inner loop on s and s ^ rand_int_1.
  // This process gives good mixing (by the murmerhash 2 stuff), plus
  // is completely reversible so there are no collisions.

  static const uint64_t m = 0xc6a4a7935bd1e995ULL;
  static const int r = 47;

  local_uint128 k;

  k.first = s1;
  k.second = s2;

  k.first *= m;
  k.second *= m;
  k.first ^= k.first >> r;
  k.second ^= k.second >> r;
  k.first *= m;
  k.second *= m;

  k.second ^= k.first;
  k.second *= m;

  return k;
}


static inline local_uint128 SimpleIntegerHash128(uint64_t s) {

  static const uint64_t rand_int_1 = 0x6e626e7774e95a48ULL;

  return SimpleIntegerHash128(s, rand_int_1 ^ s);
}

static inline local_uint128 Murmor3MixRoutine64(uint64_t x, uint64_t y, uint64_t seed) {
  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = 0x87c37b91114253d5ULL;
  const uint64_t c2 = 0x4cf5ad432745937fULL;

  // Taken exactly from the smhasher 64 bit code for murmur3 at
  // http://code.google.com/p/smhasher/

  x *= c1; x  = Rotate(x,31); x *= c2; h1 ^= x;

  h1 = Rotate(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

  y *= c2; y  = Rotate(y,33); y *= c1; h2 ^= y;

  h2 = Rotate(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;

  return std::make_pair(h1, h2);
}

static inline local_uint128 Murmor3MixRoutine128(uint128_t x, uint128_t y, uint64_t seed) {
  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = 0x87c37b91114253d5ULL;
  const uint64_t c2 = 0x4cf5ad432745937fULL;

  // Taken exactly from the smhasher 64 bit code for murmur3 at
  // http://code.google.com/p/smhasher/

  uint64_t x1 = uint64_t(x >> 64);
  uint64_t x2 = uint64_t(x);

  x1 *= c1; x1  = Rotate(x1,31); x1 *= c2; h1 ^= x1;

  h1 = Rotate(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

  x2 *= c2; x2  = Rotate(x2,33); x2 *= c1; h2 ^= x2;

  h2 = Rotate(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;

  uint64_t y1 = uint64_t(y >> 64);
  uint64_t y2 = uint64_t(y);

  y1 *= c1; y1  = Rotate(y1,31); y1 *= c2; h1 ^= y1;

  h1 = Rotate(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

  y2 *= c2; y  = Rotate(y2,33); y2 *= c1; h2 ^= y2;

  h2 = Rotate(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;

  return std::make_pair(h1, h2);
}

}

// Now, all the external wrappers for the above functions

class flexible_type;

/**
 * \ingroup util
 * \addtogroup Hashing
 * \brief A generic set of hashing functions built around Google's cityhash.
 * \{
 */

/**
 * Returns a 128 bit hash of a string using the city hash function.
 * The hash is strong but not cryptographically secure.
 *
 * \param s   char* pointer to data.
 * \param len Length of data to hash, in bytes.
 */
static inline uint128_t hash128(const char* s, size_t len) {
  cityhash_local::local_uint128 r = cityhash_local::CityHash128(s, len);

  return ((uint128_t(cityhash_local::Uint128High64(r)) << 64)
          + uint128_t(cityhash_local::Uint128Low64(r)));
}

/**
 * Returns a 128 bit hash of a string using the city hash function.
 * The hash is strong but not cryptographically secure.
 *
 * \param s   std::string instance to hash.
 */
static inline uint128_t hash128(const std::string& s) {
  return hash128(s.c_str(), s.size());
}

/**
 * Returns a 128 bit hash of a uint128_t hash value.
 * The hash is unique, has good bit-wise properties, and is very fast.
 *
 * \param v  128 bit integer value to hash.
 */
static inline uint128_t hash128(uint128_t v) {
  cityhash_local::local_uint128 r = cityhash_local::Murmor3MixRoutine64(
      uint64_t(v >> 64), uint64_t(v), 0x8f84e92c0587b7e3ULL);

  return ((uint128_t(cityhash_local::Uint128High64(r)) << 64)
          + uint128_t(cityhash_local::Uint128Low64(r)));
}

/**
 * Returns a 128 bit hash of any integer type up to 64 bits.  The hash
 * is unique, has decent bit-wise properties, and is very fast.
 *
 * \param v   integer value to hash.
 */
template <typename T>
static inline uint128_t hash128(
    const T& v, typename std::enable_if<std::is_integral<T>::value && sizeof(T) <= 8>::type* = 0) {

  cityhash_local::local_uint128 r = cityhash_local::SimpleIntegerHash128(uint64_t(v));

  return ((uint128_t(cityhash_local::Uint128High64(r)) << 64)
          + uint128_t(cityhash_local::Uint128Low64(r)));
}


/**
 * Returns a 128 bit hash of a uint128_t hash value.
 * The hash is unique, has good bit-wise properties, and is very fast.
 *
 * \param v  64 bit integer value to hash.
 */
static inline uint128_t hash128(uint64_t v) {
  cityhash_local::local_uint128 r = cityhash_local::SimpleIntegerHash128(v);

  return ((uint128_t(cityhash_local::Uint128High64(r)) << 64)
          + uint128_t(cityhash_local::Uint128Low64(r)));
}

/**
 * Returns a 128 bit hash of a flexible_type value.  The hash is
 * unique, has good bit-wise properties, and is very fast.
 *
 * \param v flexible_type value to hash.
 */
uint128_t hash128(const flexible_type& v);

/**
 * Returns a 128 bit hash of a vector of flexible_type values.  The
 * hash is unique, has good bit-wise properties, and is very fast.
 *
 * \param v vector of flexible_type values to hash.
 */
uint128_t hash128(const std::vector<flexible_type>& v);

/**
 * Returns a 64 bit hash of a string using the city hash function.
 * The hash is strong but not cryptographically secure.
 *
 * \param s   char* pointer to data.
 * \param len Length of data to hash, in bytes.
 */
static inline uint64_t hash64(const char* s, size_t len) {
  return cityhash_local::CityHash64(s, len);
}

/**
 * Returns a 64 bit hash of a string using the city hash function.
 * The hash is strong but not cryptographically secure.
 *
 * \param s   std::string instance to hash.
 */
static inline uint64_t hash64(const std::string& s) {
  return hash64(s.c_str(), s.size());
}

/**
 * Returns a 64 bit hash of two 64 bit integers.  The hash is unique,
 * has good bit-wise properties, and is fairly fast.  The hash
 * function is subject to change.
 *
 * \param v1  First integer value to hash.
 * \param v2  Second integer value to hash.
 */
static inline uint64_t hash64(uint64_t v1, uint64_t v2) {
  static const uint64_t rand_int = 0x9fa35c8d77b96328ULL;

  cityhash_local::local_uint128 r = cityhash_local::Murmor3MixRoutine64(v1, v2, rand_int);

  return r.first ^ r.second;
}

/**
 * Returns a 64 bit hash of two 64 bit integers.  The hash is unique,
 * has good bit-wise properties, and is fairly fast.  The hash
 * function is subject to change.
 *
 * \param v1  First integer value to hash.
 * \param v2  Second integer value to hash.
 * \param v3  Third integer value to hash.
 */
static inline uint64_t hash64(uint64_t v1, uint64_t v2, uint64_t v3) {
  return hash64(v1, hash64(v2, v3));
}

/**
 * Returns a 64 bit hash of a 128 bit integer.  The hash is unique,
 * has good bit-wise properties, and is fairly fast.  The hash
 * function is subject to change.
 *
 * \param v   integer value to hash.
 */
static inline uint64_t hash64(uint128_t v) {
  static const uint64_t rand_int = 0xf52ef6f00df6f718ULL;

  uint64_t h1 = uint64_t(v >> 64);
  uint64_t h2 = uint64_t(v);

  cityhash_local::local_uint128 r = cityhash_local::Murmor3MixRoutine64(h1, h2, rand_int);

  return r.first ^ r.second;
}

/**
 * Returns a 64 bit hash of any integer up to 64 bits.  The hash is
 * unique, has good bit-wise properties, and is fairly fast.  The hash
 * function is subject to change.
 *
 * \param v   integer value to hash.
 */
template <typename T>
static inline uint64_t hash64(
    const T& v, typename std::enable_if<std::is_integral<T>::value && sizeof(T) <= 8>::type* = 0) {
  return cityhash_local::SimpleIntegerHash64(uint64_t(v));
}

/**
 * Returns a 64 bit hash of a flexible_type value.  The hash is
 * unique, has good bit-wise properties, and is very fast.
 *
 * \param v flexible_type value to hash.
 */
uint64_t hash64(const flexible_type& v);

/**
 * Returns a 64 bit hash of a vector of flexible_type values.  The
 * hash is unique, has good bit-wise properties, and is very fast.
 *
 * \param v vector of flexible_type values to hash.
 */
uint64_t hash64(const std::vector<flexible_type>& v);

/**
 * Updates a hash that is used to track a sequential stream of objects.
 *
 * \param h   A previous hash value.
 * \param v   A new value to hash.
 *
 * \return    The updated hash.
 */
static inline uint128_t hash128_combine(uint128_t h1, uint128_t h2) {

  static const uint64_t rand_int = 0x5b73ff027f14f66aULL;

  // This hash function is okay when there are x1, x2, y1, and y2 are
  // not correlated, which is the case here.
  cityhash_local::local_uint128 r = cityhash_local::Murmor3MixRoutine128(h1, h2, rand_int);

  return ((uint128_t(cityhash_local::Uint128High64(r)) << 64)
          + uint128_t(cityhash_local::Uint128Low64(r)));
}

/**
 * Updates a hash that is used to track a sequential stream of objects.
 *
 * \param h   A previous hash value.
 * \param v   A new value to hash.
 *
 * \return    The updated hash.
 */
template <typename T>
static inline uint128_t hash128_update(uint128_t h, const T& v) {
  return hash128_combine(h, hash128(v));
}

/**
 * Returns a 128 bit hash of a vector of strings using the city hash
 * function.  The hash is strong but not cryptographically secure.
 *
 * \param v   vector of std::strings to hash.
 */
static inline uint128_t hash128(const std::vector<std::string>& v) {
  uint128_t h = hash128(v.size());
  for(const std::string& s : v)
    h = hash128_update(h, s);

  return h;
}


/**
 * Combines two 64 bit hashes in a simple, order dependent way.
 * Produces a new 64 bit hash.
 *
 * \param h1 First hash value
 * \param h2 Second hash value
 */
static inline uint64_t hash64_combine(uint64_t h1, uint64_t h2) {

  static const uint64_t rand_int = 0x73a3916ae45d01e5ULL;

  cityhash_local::local_uint128 r = cityhash_local::Murmor3MixRoutine64(h1, h2, rand_int);

  return r.first ^ r.second;
}

/**
 *  When hash64 is used as a random number function, it is nice to be
 *  able to do the following to get a proportion:
 *
 *  uint64_t threshold = hash64_proportion_cutoff(proportion);
 *  // ...
 *  if(hash64(...) < threshold) {
 *     // do something that happens `proportion` of the time.
 *  }
 *
 *  Unfortunately, this computation of the proportion is prone to
 *  numerical issues due to the 48 bits of precision of the double,
 *  which this function gets around.
 */
uint64_t hash64_proportion_cutoff(double proportion);

/**
 * Updates an existing 64 bit hash in a simple, order dependent way.
 *
 * \param h First hash value
 * \param t New object to hash
 */
template <typename T>
static inline uint64_t hash64_update(uint64_t h1, const T& t) {
  return hash64_combine(h1, hash64(t));
}

/**
 * Returns a 128 bit hash of a vector of strings using the city hash
 * function.  The hash is strong but not cryptographically secure.
 *
 * \param v   vector of std::strings to hash.
 */
static inline uint64_t hash64(const std::vector<std::string>& v) {
  uint64_t h = hash64(v.size());
  for(const std::string& s : v)
    h = hash64_update(h, s);

  return h;
}


/**
 * Returns a 32bit value based on index and seed that has reasonable
 * pseudorandom properties; I.e. for a given seed, each index maps to
 * effectively random values of size_t.
 *
 * \param index Index from which the pseudorandom map value is mapped.
 * \param seed Random seed governing the mapping.
 */
static inline uint32_t simple_random_mapping(size_t index, size_t seed) {
  cityhash_local::local_uint128 r = cityhash_local::SimpleIntegerHash128(index, seed);
  // Mix the last few bits
  uint64_t h = r.first ^ r.second;
  return uint32_t(h ^ (h >> 32));
}

/**
 * Provides a simple, reversable hash for indices.  This hash has
 *  excellent mixing properties, preserves 0 (i.e. 0 maps to 0), and
 *  is reversable by calling reverse_index_hash(...) below.
 *
 *  Taken from the Murmur3 finalizer routine.
 *  Reference: http://code.google.com/p/smhasher/wiki/MurmurHash3.
 */
static inline uint64_t index_hash(uint64_t idx) {

  static constexpr uint64_t m3_final_1     = 0xff51afd7ed558ccdULL;
  static constexpr uint64_t m3_final_2     = 0xc4ceb9fe1a85ec53ULL;

  static constexpr uint64_t r = 33;

  uint64_t h = idx;

  h ^= h >> r;
  h *= m3_final_1;
  h ^= h >> r;
  h *= m3_final_2;
  h ^= h >> r;

  return h;
}

/**
 * The reverse of \ref index_hash. Gets the index from the index_hash
 */
static inline uint64_t reverse_index_hash(uint64_t idx)  {

  // Multaplicative inverses of m3_final_1 and m3_final_2 above
  static constexpr uint64_t m3_final_1_inv = 0x4f74430c22a54005ULL;
  static constexpr uint64_t m3_final_2_inv = 0x9cb4b2f8129337dbULL;

  static constexpr uint64_t r = 33;

  uint64_t h = idx;

  h ^= h >> r;
  h *= m3_final_2_inv;
  h ^= h >> r;
  h *= m3_final_1_inv;
  h ^= h >> r;

  return h;
}

/**
 * \}
 */

} // End namespace turi

namespace std {

// Gets rid of a stupid clang mismatch of things
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

#ifndef HASH_FOR_UINT128_DEFINED

template <> struct hash<uint128_t> {
  size_t operator()(const uint128_t& t) const {
    return size_t(t ^ (t >> 64));
  }
};

#endif

#ifndef HASH_FOR_INT128_DEFINED

template <> struct hash<int128_t> {
  size_t operator()(const int128_t& t) const {
    return size_t(t ^ (t >> 64));
  }
};

#endif



#ifdef __clang__
#pragma clang diagnostic pop
#endif

}


#endif /* _CITYHASH_GL_H_ */
