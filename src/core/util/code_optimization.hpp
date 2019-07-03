/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CODE_OPTIMIZATION_H_
#define TURI_CODE_OPTIMIZATION_H_

#ifdef NDEBUG
#define GL_OPT_ATTR(...) __attribute__((__VA_ARGS__))
#else
// put it as hot; this is more-or-less ignored in the ; the "cold" ones have their own attribute def.
#define GL_OPT_ATTR(...) __attribute__((hot))
#endif

#ifdef __clang__
#define GL_GCC_ONLY(...)
#else
#define GL_GCC_ONLY(...) __VA_ARGS__
#endif

// Attributes that enable aggressive optimizations for math-heavy
// functions.

#define _GL_GCC_HOT_FUNCTION_FLAGS              \
  hot,                                          \
    optimize("tree-loop-im"),                   \
    optimize("tree-vectorize"),                 \
    optimize("inline-limit=10000"),             \
    optimize("unsafe-math-optimizations"),      \
    optimize("fast-math"),                      \
    optimize("unroll-loops"),                   \
    optimize("peel-loops"),                     \
    optimize("variable-expansion-in-unroller")

#define _GL_HOT_FUNCTION_FLAGS _GL_GCC_HOT_FUNCTION_FLAGS

/**
 * \ingroup util
 * Forces always inline
 */
#define GL_HOT_INLINE                           \
  GL_OPT_ATTR(always_inline,                    \
              _GL_HOT_FUNCTION_FLAGS            \
              )

/**
 * \ingroup util
 * Forces always inline and flatten
 */
#define GL_HOT_INLINE_FLATTEN                   \
  GL_OPT_ATTR(always_inline,                    \
              flatten,                          \
              _GL_HOT_FUNCTION_FLAGS            \
              )

/**
 * \ingroup util
 * Forces flatten
 */
#define GL_HOT_FLATTEN                          \
  GL_OPT_ATTR(flatten,                          \
              _GL_HOT_FUNCTION_FLAGS            \
              )

#define GL_HOT                                  \
  GL_OPT_ATTR(_GL_HOT_FUNCTION_FLAGS)

#define GL_HOT_NOINLINE                         \
  GL_OPT_ATTR(noinline,                         \
              _GL_HOT_FUNCTION_FLAGS            \
              )

#define GL_HOT_NOINLINE_FLATTEN                 \
  GL_OPT_ATTR(noinline,                         \
              flatten,                          \
              _GL_HOT_FUNCTION_FLAGS            \
              )

#define GL_COLD_NOINLINE                        \
  __attribute__((cold, noinline))


/**
 * \ingroup util
 * This is used with various assertion routines.  It never returns.
 * The noreturn suppresses the "warning: control reaches end of
 * non-void function" messages you get by using it this way.
 */
#define GL_COLD_NOINLINE_ERROR                 \
  __attribute__((cold, noinline, noreturn))

#ifdef __SSE2__
#include <xmmintrin.h>
#endif

// Set the floating point register to flush denormal numbers to zero.
// This gives improved performance on most sgd things.
static inline void set_denormal_are_zero() {
#ifdef __SSE2__
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
}

// Set the floating point register to flush denormal numbers to zero.
// This gives improved performance on most sgd things.
static inline void unset_denormal_are_zero() {
#ifdef __SSE2__
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
  _mm_setcsr(_mm_getcsr() & ~(decltype(_mm_getcsr())(0x8040)));
#endif
}

#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))


/** A macro to make sure that a vector is sized sufficiently to hold
 *  the index idx.  If not, resize.
 *
 *  One issue with using the standard vector resize is that
 */
#define FAST_CHECK_VECTOR_BUFFER_SIZE(v, idx)                           \
  do {                                                                  \
    if(UNLIKELY((v).size() <= (idx) )) {                                \
      auto resize = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {               \
        (v).resize( (5*((idx) + 4)) / 4);                               \
      };                                                                \
      resize();                                                         \
    }                                                                   \
  } while(false)



#endif /* TURI_CODE_OPTIMIZATION_H_ */
