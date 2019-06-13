/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LOGIT_MATH_H_
#define TURI_LOGIT_MATH_H_


#include <util/code_optimization.hpp>
#include <logger/assertions.hpp>
#include <cmath>


namespace turi {

/** Numerically stable version of 1 / (1 + exp(-x));
 *
 *  If x < 0, then this is equal to
 *
 *  exp(- abs(x)) / (1 + exp(-abs(x))) = 1 / (1 + exp(x))
 *
 *  If x >=0, then this is equal to 1 / (1 + exp(-x)).
 *
 *  This separation into the positive and negative case is so that the
 *  code never uses the result of exp(x) where x is large and
 *  positive, which could easily result in overflow.
 */
static inline
GL_HOT_INLINE_FLATTEN
    double sigmoid(double x) {
  double m_abs_x = std::copysign(x, -1.0);
  double exp_neg = std::exp(m_abs_x);
  bool is_negative = std::signbit(x);
  double numerator = is_negative ? exp_neg : 1.0;

  return numerator / (1.0 + exp_neg);
}

/** Numerically stable version of log(1 + exp(x) )
 */
static inline GL_HOT_INLINE_FLATTEN double log1pe(double x)
{
  return __builtin_expect((x > 48), 0) ? x : std::log1p(exp(x));
}

/** Numerically stable version of log(1 + exp(-x) )
 */
static inline GL_HOT_INLINE_FLATTEN double log1pen(double x)
{
  return __builtin_expect((x < -48), 0) ? -x : std::log1p(exp(-x));
}

/** Numerically stable version of log(1 - exp(x) )
 */
static inline GL_HOT_INLINE_FLATTEN double log1me(double x)
{
  DASSERT_LT(x, 0);
  return __builtin_expect((x < -48), 0) ? 0 : std::log1p(-exp(x));
}

/** Numerically stable version of log(1 - exp(-x) )
 */
static inline GL_HOT_INLINE_FLATTEN double log1men(double x)
{
  DASSERT_GT(x, 0);
  return __builtin_expect((x > 48), 0) ? 0 : std::log1p(-exp(-x));
}

/** Numerically stable version of log(exp(x) - 1)
 */
static inline GL_HOT_INLINE_FLATTEN double logem1(double x)
{
  return __builtin_expect((x > 48), 0) ? x : std::log(std::expm1(x));
}

/** Numerically stable version of
 *  d/dx (log(1 + exp(x) )) = 1 / (1 + exp(-x)) = sigmoid(x).
 */
static inline GL_HOT_INLINE_FLATTEN double log1pe_deriviative(double x)
{
  return sigmoid(x);
}

/** Numerically stable version of
 *  d/dx (log(1 + exp(-x) )) =  -1 / (1 + exp(x)).
 */
static inline GL_HOT_INLINE_FLATTEN double log1pen_deriviative(double x)
{
  double m_abs_x = std::copysign(x, -1.0);
  double exp_neg = std::exp(m_abs_x);
  bool is_negative = std::signbit(x);
  double numerator = is_negative ? 1.0 : exp_neg;

  return -numerator / (1.0 + exp_neg);
}

template <typename T> static inline T sq(const T& t) { return t*t; }


}


#endif /* _LOGIT_MATH_H_ */
