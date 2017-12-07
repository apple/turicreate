#ifndef HALIDE_MODULUS_REMAINDER_H
#define HALIDE_MODULUS_REMAINDER_H

/** \file
 * Routines for statically determining what expressions are divisible by.
 */

#include "Scope.h"

namespace Halide {
namespace Internal {

/** The result of modulus_remainder analysis */
struct ModulusRemainder {
    ModulusRemainder() : modulus(0), remainder(0) {}
    ModulusRemainder(int m, int r) : modulus(m), remainder(r) {}
    int modulus, remainder;
};

/** For things like alignment analysis, often it's helpful to know
 * if an integer expression is some multiple of a constant plus
 * some other constant. For example, it is straight-forward to
 * deduce that ((10*x + 2)*(6*y - 3) - 1) is congruent to five
 * modulo six.
 *
 * We get the most information when the modulus is large. E.g. if
 * something is congruent to 208 modulo 384, then we also know it's
 * congruent to 0 mod 8, and we can possibly use it as an index for an
 * aligned load. If all else fails, we can just say that an integer is
 * congruent to zero modulo one.
 */
EXPORT ModulusRemainder modulus_remainder(Expr e);

/** If we have alignment information about external variables, we can
 * let the analysis know about that using this version of
 * modulus_remainder: */
EXPORT ModulusRemainder modulus_remainder(Expr e, const Scope<ModulusRemainder> &scope);

/** Reduce an expression modulo some integer. Returns true and assigns
 * to remainder if an answer could be found. */
///@{
EXPORT bool reduce_expr_modulo(Expr e, int modulus, int *remainder);
EXPORT bool reduce_expr_modulo(Expr e, int modulus, int *remainder, const Scope<ModulusRemainder> &scope);
///@}

EXPORT void modulus_remainder_test();

/** The greatest common divisor of two integers */
EXPORT int gcd(int, int);

/** The least common multiple of two integers */
EXPORT int lcm(int, int);

}
}

#endif
