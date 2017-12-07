#ifndef HALIDE_IR_OPERATOR_H
#define HALIDE_IR_OPERATOR_H

/** \file
 *
 * Defines various operator overloads and utility functions that make
 * it more pleasant to work with Halide expressions.
 */

#include <atomic>

#include "IR.h"
#include "base/Util.h"

namespace Halide {

namespace Internal {
/** Is the expression either an IntImm, a FloatImm, a StringImm, or a
 * Cast of the same, or a Ramp or Broadcast of the same. Doesn't do
 * any constant folding. */
EXPORT bool is_const(const Expr &e);

/** Is the expression an IntImm, FloatImm of a particular value, or a
 * Cast, or Broadcast of the same. */
EXPORT bool is_const(const Expr &e, int64_t v);

/** If an expression is an IntImm or a Broadcast of an IntImm, return
 * a pointer to its value. Otherwise returns nullptr. */
EXPORT const int64_t *as_const_int(const Expr &e);

/** If an expression is a UIntImm or a Broadcast of a UIntImm, return
 * a pointer to its value. Otherwise returns nullptr. */
EXPORT const uint64_t *as_const_uint(const Expr &e);

/** If an expression is a FloatImm or a Broadcast of a FloatImm,
 * return a pointer to its value. Otherwise returns nullptr. */
EXPORT const double *as_const_float(const Expr &e);

/** Is the expression a constant integer power of two. Also returns
 * log base two of the expression if it is. Only returns true for
 * integer types. */
EXPORT bool is_const_power_of_two_integer(const Expr &e, int *bits);

/** Is the expression a const (as defined by is_const), and also
 * strictly greater than zero (in all lanes, if a vector expression) */
EXPORT bool is_positive_const(const Expr &e);

/** Is the expression a const (as defined by is_const), and also
 * strictly less than zero (in all lanes, if a vector expression) */
EXPORT bool is_negative_const(const Expr &e);

/** Is the expression a const (as defined by is_const), and also
 * strictly less than zero (in all lanes, if a vector expression) and
 * is its negative value representable. (This excludes the most
 * negative value of the Expr's type from inclusion. Intended to be
 * used when the value will be negated as part of simplification.)
 */
EXPORT bool is_negative_negatable_const(const Expr &e);

/** Is the expression an undef */
EXPORT bool is_undef(const Expr &e);

/** Is the expression a const (as defined by is_const), and also equal
 * to zero (in all lanes, if a vector expression) */
EXPORT bool is_zero(const Expr &e);

/** Is the expression a const (as defined by is_const), and also equal
 * to one (in all lanes, if a vector expression) */
EXPORT bool is_one(const Expr &e);

/** Is the expression a const (as defined by is_const), and also equal
 * to two (in all lanes, if a vector expression) */
EXPORT bool is_two(const Expr &e);

/** Is the statement a no-op (which we represent as either an
 * undefined Stmt, or as an Evaluate node of a constant) */
EXPORT bool is_no_op(const Stmt &s);

/** Construct an immediate of the given type from any numeric C++ type. */
// @{
EXPORT Expr make_const(Type t, int64_t val);
EXPORT Expr make_const(Type t, uint64_t val);
EXPORT Expr make_const(Type t, double val);
inline Expr make_const(Type t, int32_t val)   {return make_const(t, (int64_t)val);}
inline Expr make_const(Type t, uint32_t val)  {return make_const(t, (uint64_t)val);}
inline Expr make_const(Type t, int16_t val)   {return make_const(t, (int64_t)val);}
inline Expr make_const(Type t, uint16_t val)  {return make_const(t, (uint64_t)val);}
inline Expr make_const(Type t, int8_t val)    {return make_const(t, (int64_t)val);}
inline Expr make_const(Type t, uint8_t val)   {return make_const(t, (uint64_t)val);}
inline Expr make_const(Type t, bool val)      {return make_const(t, (uint64_t)val);}
inline Expr make_const(Type t, float val)     {return make_const(t, (double)val);}
inline Expr make_const(Type t, float16_t val) {return make_const(t, (double)val);}
// @}

/** Check if a constant value can be correctly represented as the given type. */
EXPORT void check_representable(Type t, int64_t val);

/** Construct a boolean constant from a C++ boolean value.
 * May also be a vector if width is given.
 * It is not possible to coerce a C++ boolean to Expr because
 * if we provide such a path then char objects can ambiguously
 * be converted to Halide Expr or to std::string.  The problem
 * is that C++ does not have a real bool type - it is in fact
 * close enough to char that C++ does not know how to distinguish them.
 * make_bool is the explicit coercion. */
EXPORT Expr make_bool(bool val, int lanes = 1);

/** Construct the representation of zero in the given type */
EXPORT Expr make_zero(Type t);

/** Construct the representation of one in the given type */
EXPORT Expr make_one(Type t);

/** Construct the representation of two in the given type */
EXPORT Expr make_two(Type t);

/** Construct the constant boolean true. May also be a vector of
 * trues, if a lanes argument is given. */
EXPORT Expr const_true(int lanes = 1);

/** Construct the constant boolean false. May also be a vector of
 * falses, if a lanes argument is given. */
EXPORT Expr const_false(int lanes = 1);

/** Attempt to cast an expression to a smaller type while provably not
 * losing information. If it can't be done, return an undefined
 * Expr. */
EXPORT Expr lossless_cast(Type t, const Expr &e);

/** Coerce the two expressions to have the same type, using C-style
 * casting rules. For the purposes of casting, a boolean type is
 * UInt(1). We use the following procedure:
 *
 * If the types already match, do nothing.
 *
 * Then, if one type is a vector and the other is a scalar, the scalar
 * is broadcast to match the vector width, and we continue.
 *
 * Then, if one type is floating-point and the other is not, the
 * non-float is cast to the floating-point type, and we're done.
 *
 * Then, if both types are unsigned ints, the one with fewer bits is
 * cast to match the one with more bits and we're done.
 *
 * Then, if both types are signed ints, the one with fewer bits is
 * cast to match the one with more bits and we're done.
 *
 * Finally, if one type is an unsigned int and the other type is a signed
 * int, both are cast to a signed int with the greater of the two
 * bit-widths. For example, matching an Int(8) with a UInt(16) results
 * in an Int(16).
 *
 */
EXPORT void match_types(Expr &a, Expr &b);

/** Halide's vectorizable transcendentals. */
// @{
EXPORT Expr halide_log(const Expr &a);
EXPORT Expr halide_exp(const Expr &a);
EXPORT Expr halide_erf(const Expr &a);
// @}

/** Raise an expression to an integer power by repeatedly multiplying
 * it by itself. */
EXPORT Expr raise_to_integer_power(const Expr &a, int64_t b);

/** Split a boolean condition into vector of ANDs. If 'cond' is undefined,
 * return an empty vector. */
EXPORT void split_into_ands(const Expr &cond, std::vector<Expr> &result);

} // namespace Internal

/** Cast an expression to the halide type corresponding to the C++ type T. */
template<typename T>
inline Expr cast(const Expr &a) {
    return cast(type_of<T>(), a);
}

/** Cast an expression to a new type. */
inline Expr cast(Type t, const Expr &a) {
    user_assert(a.defined()) << "cast of undefined Expr\n";
    if (a.type() == t) return a;

    if (t.is_handle() && !a.type().is_handle()) {
        user_error << "Can't cast \"" << a << "\" to a handle. "
                   << "The only legal cast from scalar types to a handle is: "
                   << "reinterpret(Handle(), cast<uint64_t>(" << a << "));\n";
    } else if (a.type().is_handle() && !t.is_handle()) {
        user_error << "Can't cast handle \"" << a << "\" to type " << t << ". "
                   << "The only legal cast from handles to scalar types is: "
                   << "reinterpret(UInt(64), " << a << ");\n";
    }

    // Fold constants early
    if (const int64_t *i = as_const_int(a)) {
        return Internal::make_const(t, *i);
    }
    if (const uint64_t *u = as_const_uint(a)) {
        return Internal::make_const(t, *u);
    }
    if (const double *f = as_const_float(a)) {
        return Internal::make_const(t, *f);
    }

    if (t.is_vector()) {
        if (a.type().is_scalar()) {
            return Internal::Broadcast::make(cast(t.element_of(), a), t.lanes());
        } else if (const Internal::Broadcast *b = a.as<Internal::Broadcast>()) {
            internal_assert(b->lanes == t.lanes());
            return Internal::Broadcast::make(cast(t.element_of(), b->value), t.lanes());
        }
    }
    return Internal::Cast::make(t, a);
}

/** Return the sum of two expressions, doing any necessary type
 * coercion using \ref Internal::match_types */
inline Expr operator+(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator+ of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::Add::make(a, b);
}

/** Add an expression and a constant integer. Coerces the type of the
 * integer to match the type of the expression. Errors if the integer
 * cannot be represented in the type of the expression. */
// @{
inline Expr operator+(const Expr &a, int b) {
    user_assert(a.defined()) << "operator+ of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::Add::make(a, Internal::make_const(a.type(), b));
}

/** Add a constant integer and an expression. Coerces the type of the
 * integer to match the type of the expression. Errors if the integer
 * cannot be represented in the type of the expression. */
inline Expr operator+(int a, const Expr &b) {
    user_assert(b.defined()) << "operator+ of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::Add::make(Internal::make_const(b.type(), a), b);
}

/** Modify the first expression to be the sum of two expressions,
 * without changing its type. This casts the second argument to match
 * the type of the first. */
inline Expr &operator+=(Expr &a, const Expr &b) {
    user_assert(a.defined() && b.defined()) << "operator+= of undefined Expr\n";
    a = Internal::Add::make(a, cast(a.type(), b));
    return a;
}

/** Return the difference of two expressions, doing any necessary type
 * coercion using \ref Internal::match_types */
inline Expr operator-(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator- of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::Sub::make(a, b);
}

/** Subtracts a constant integer from an expression. Coerces the type of the
 * integer to match the type of the expression. Errors if the integer
 * cannot be represented in the type of the expression. */
inline Expr operator-(const Expr &a, int b) {
    user_assert(a.defined()) << "operator- of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::Sub::make(a, Internal::make_const(a.type(), b));
}

/** Subtracts an expression from a constant integer. Coerces the type
 * of the integer to match the type of the expression. Errors if the
 * integer cannot be represented in the type of the expression. */
inline Expr operator-(int a, const Expr &b) {
    user_assert(b.defined()) << "operator- of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::Sub::make(Internal::make_const(b.type(), a), b);
}

/** Return the negative of the argument. Does no type casting, so more
 * formally: return that number which when added to the original,
 * yields zero of the same type. For unsigned integers the negative is
 * still an unsigned integer. E.g. in UInt(8), the negative of 56 is
 * 200, because 56 + 200 == 0 */
inline Expr operator-(const Expr &a) {
    user_assert(a.defined()) << "operator- of undefined Expr\n";
    return Internal::Sub::make(Internal::make_zero(a.type()), a);
}

/** Modify the first expression to be the difference of two expressions,
 * without changing its type. This casts the second argument to match
 * the type of the first. */
inline Expr &operator-=(Expr &a, const Expr &b) {
    user_assert(a.defined() && b.defined()) << "operator-= of undefined Expr\n";
    a = Internal::Sub::make(a, cast(a.type(), b));
    return a;
}

/** Return the product of two expressions, doing any necessary type
 * coercion using \ref Internal::match_types */
inline Expr operator*(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator* of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::Mul::make(a, b);
}

/** Multiply an expression and a constant integer. Coerces the type of the
 * integer to match the type of the expression. Errors if the integer
 * cannot be represented in the type of the expression. */
inline Expr operator*(const Expr &a, int b) {
    user_assert(a.defined()) << "operator* of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::Mul::make(a, Internal::make_const(a.type(), b));
}

/** Multiply a constant integer and an expression. Coerces the type of
 * the integer to match the type of the expression. Errors if the
 * integer cannot be represented in the type of the expression. */
inline Expr operator*(int a, const Expr &b) {
    user_assert(b.defined()) << "operator* of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::Mul::make(Internal::make_const(b.type(), a), b);
}

/** Modify the first expression to be the product of two expressions,
 * without changing its type. This casts the second argument to match
 * the type of the first. */
inline Expr &operator*=(Expr &a, const Expr &b) {
    user_assert(a.defined() && b.defined()) << "operator*= of undefined Expr\n";
    a = Internal::Mul::make(a, cast(a.type(), b));
    return a;
}

/** Return the ratio of two expressions, doing any necessary type
 * coercion using \ref Internal::match_types. Note that signed integer
 * division in Halide rounds towards minus infinity, unlike C, which
 * rounds towards zero. */
inline Expr operator/(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator/ of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::Div::make(a, b);
}

/** Modify the first expression to be the ratio of two expressions,
 * without changing its type. This casts the second argument to match
 * the type of the first. Note that signed integer division in Halide
 * rounds towards minus infinity, unlike C, which rounds towards
 * zero. */
inline Expr &operator/=(Expr &a, const Expr &b) {
    user_assert(a.defined() && b.defined()) << "operator/= of undefined Expr\n";
    a = Internal::Div::make(a, cast(a.type(), b));
    return a;
}



/** Divides an expression by a constant integer. Coerces the type
 * of the integer to match the type of the expression. Errors if the
 * integer cannot be represented in the type of the expression. */
inline Expr operator/(const Expr &a, int b) {
    user_assert(a.defined()) << "operator/ of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::Div::make(a, Internal::make_const(a.type(), b));
}

/** Divides a constant integer by an expression. Coerces the type
 * of the integer to match the type of the expression. Errors if the
 * integer cannot be represented in the type of the expression. */
inline Expr operator/(int a, const Expr &b) {
    user_assert(b.defined()) << "operator- of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::Div::make(Internal::make_const(b.type(), a), b);
}

/** Return the first argument reduced modulo the second, doing any
 * necessary type coercion using \ref Internal::match_types. For
 * signed integers, the sign of the result matches the sign of the
 * second argument (unlike in C, where it matches the sign of the
 * first argument). For example, this means that x%2 is always either
 * zero or one, even if x is negative.*/
inline Expr operator%(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator% of undefined Expr\n";
    user_assert(!Internal::is_zero(b)) << "operator% with constant 0 modulus\n";
    Internal::match_types(a, b);
    return Internal::Mod::make(a, b);
}

/** Mods an expression by a constant integer. Coerces the type
 * of the integer to match the type of the expression. Errors if the
 * integer cannot be represented in the type of the expression. */
inline Expr operator%(const Expr &a, int b) {
    user_assert(a.defined()) << "operator% of undefined Expr\n";
    user_assert(b != 0) << "operator% with constant 0 modulus\n";
    Internal::check_representable(a.type(), b);
    return Internal::Mod::make(a, Internal::make_const(a.type(), b));
}
/** Mods a constant integer by an expression. Coerces the type
 * of the integer to match the type of the expression. Errors if the
 * integer cannot be represented in the type of the expression. */
inline Expr operator%(int a, const Expr &b) {
    user_assert(b.defined()) << "operator% of undefined Expr\n";
    user_assert(!Internal::is_zero(b)) << "operator% with constant 0 modulus\n";
    Internal::check_representable(b.type(), a);
    return Internal::Mod::make(Internal::make_const(b.type(), a), b);
}

/** Return a boolean expression that tests whether the first argument
 * is greater than the second, after doing any necessary type coercion
 * using \ref Internal::match_types */
inline Expr operator>(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator> of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::GT::make(a, b);
}

/** Return a boolean expression that tests whether an expression is
 * greater than a constant integer. Coerces the integer to the type of
 * the expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator>(const Expr &a, int b) {
    user_assert(a.defined()) << "operator> of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::GT::make(a, Internal::make_const(a.type(), b));
}

/** Return a boolean expression that tests whether a constant integer is
 * greater than an expression. Coerces the integer to the type of
 * the expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator>(int a, const Expr &b) {
    user_assert(b.defined()) << "operator> of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::GT::make(Internal::make_const(b.type(), a), b);
}

/** Return a boolean expression that tests whether the first argument
 * is less than the second, after doing any necessary type coercion
 * using \ref Internal::match_types */
inline Expr operator<(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator< of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::LT::make(a, b);
}

/** Return a boolean expression that tests whether an expression is
 * less than a constant integer. Coerces the integer to the type of
 * the expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator<(const Expr &a, int b) {
    user_assert(a.defined()) << "operator< of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::LT::make(a, Internal::make_const(a.type(), b));
}

/** Return a boolean expression that tests whether a constant integer is
 * less than an expression. Coerces the integer to the type of
 * the expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator<(int a, const Expr &b) {
    user_assert(b.defined()) << "operator< of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::LT::make(Internal::make_const(b.type(), a), b);
}

/** Return a boolean expression that tests whether the first argument
 * is less than or equal to the second, after doing any necessary type
 * coercion using \ref Internal::match_types */
inline Expr operator<=(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator<= of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::LE::make(a, b);
}

/** Return a boolean expression that tests whether an expression is
 * less than or equal to a constant integer. Coerces the integer to
 * the type of the expression. Errors if the integer is not
 * representable in that type. */
inline Expr operator<=(const Expr &a, int b) {
    user_assert(a.defined()) << "operator<= of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::LE::make(a, Internal::make_const(a.type(), b));
}

/** Return a boolean expression that tests whether a constant integer
 * is less than or equal to an expression. Coerces the integer to the
 * type of the expression. Errors if the integer is not representable
 * in that type. */
inline Expr operator<=(int a, const Expr &b) {
    user_assert(b.defined()) << "operator<= of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::LE::make(Internal::make_const(b.type(), a), b);
}

/** Return a boolean expression that tests whether the first argument
 * is greater than or equal to the second, after doing any necessary
 * type coercion using \ref Internal::match_types */
inline Expr operator>=(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator>= of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::GE::make(a, b);
}

/** Return a boolean expression that tests whether an expression is
 * greater than or equal to a constant integer. Coerces the integer to
 * the type of the expression. Errors if the integer is not
 * representable in that type. */
inline Expr operator>=(const Expr &a, int b) {
    user_assert(a.defined()) << "operator>= of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::GE::make(a, Internal::make_const(a.type(), b));
}

/** Return a boolean expression that tests whether a constant integer
 * is greater than or equal to an expression. Coerces the integer to the
 * type of the expression. Errors if the integer is not representable
 * in that type. */
inline Expr operator>=(int a, const Expr &b) {
    user_assert(b.defined()) << "operator>= of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::GE::make(Internal::make_const(b.type(), a), b);
}

/** Return a boolean expression that tests whether the first argument
 * is equal to the second, after doing any necessary type coercion
 * using \ref Internal::match_types */
inline Expr operator==(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator== of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::EQ::make(a, b);
}

/** Return a boolean expression that tests whether an expression is
 * equal to a constant integer. Coerces the integer to the type of the
 * expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator==(const Expr &a, int b) {
    user_assert(a.defined()) << "operator== of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::EQ::make(a, Internal::make_const(a.type(), b));
}

/** Return a boolean expression that tests whether a constant integer
 * is equal to an expression. Coerces the integer to the type of the
 * expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator==(int a, const Expr &b) {
    user_assert(b.defined()) << "operator== of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::EQ::make(Internal::make_const(b.type(), a), b);
}

/** Return a boolean expression that tests whether the first argument
 * is not equal to the second, after doing any necessary type coercion
 * using \ref Internal::match_types */
inline Expr operator!=(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "operator!= of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::NE::make(a, b);
}

/** Return a boolean expression that tests whether an expression is
 * not equal to a constant integer. Coerces the integer to the type of
 * the expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator!=(const Expr &a, int b) {
    user_assert(a.defined()) << "operator!= of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::NE::make(a, Internal::make_const(a.type(), b));
}

/** Return a boolean expression that tests whether a constant integer
 * is not equal to an expression. Coerces the integer to the type of
 * the expression. Errors if the integer is not representable in that
 * type. */
inline Expr operator!=(int a, const Expr &b) {
    user_assert(b.defined()) << "operator!= of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::NE::make(Internal::make_const(b.type(), a), b);
}

/** Returns the logical and of the two arguments */
inline Expr operator&&(Expr a, Expr b) {
    Internal::match_types(a, b);
    return Internal::And::make(a, b);
}

/** Logical and of an Expr and a bool. Either returns the Expr or an
 * Expr representing false, depending on the bool. */
// @{
inline Expr operator&&(const Expr &a, bool b) {
    internal_assert(a.defined()) << "operator&& of undefined Expr\n";
    internal_assert(a.type().is_bool()) << "operator&& of Expr of type " << a.type() << "\n";
    if (b) {
        return a;
    } else {
        return Internal::make_zero(a.type());
    }
}
inline Expr operator&&(bool a, const Expr &b) {
    return b && a;
}
// @}

/** Returns the logical or of the two arguments */
inline Expr operator||(Expr a, Expr b) {
    Internal::match_types(a, b);
    return Internal::Or::make(a, b);
}

/** Logical or of an Expr and a bool. Either returns the Expr or an
 * Expr representing true, depending on the bool. */
// @{
inline Expr operator||(const Expr &a, bool b) {
    internal_assert(a.defined()) << "operator|| of undefined Expr\n";
    internal_assert(a.type().is_bool()) << "operator|| of Expr of type " << a.type() << "\n";
    if (b) {
        return Internal::make_one(a.type());
    } else {
        return a;
    }
}
inline Expr operator||(bool a, const Expr &b) {
    return b || a;
}
// @}


/** Returns the logical not the argument */
inline Expr operator!(const Expr &a) {
    return Internal::Not::make(a);
}

/** Returns an expression representing the greater of the two
 * arguments, after doing any necessary type coercion using
 * \ref Internal::match_types. Vectorizes cleanly on most platforms
 * (with the exception of integer types on x86 without SSE4). */
inline Expr max(Expr a, Expr b) {
    user_assert(a.defined() && b.defined())
        << "max of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::Max::make(a, b);
}

/** Returns an expression representing the greater of an expression
 * and a constant integer.  The integer is coerced to the type of the
 * expression. Errors if the integer is not representable as that
 * type. Vectorizes cleanly on most platforms (with the exception of
 * integer types on x86 without SSE4). */
inline Expr max(const Expr &a, int b) {
    user_assert(a.defined()) << "max of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::Max::make(a, Internal::make_const(a.type(), b));
}


/** Returns an expression representing the greater of a constant
 * integer and an expression. The integer is coerced to the type of
 * the expression. Errors if the integer is not representable as that
 * type. Vectorizes cleanly on most platforms (with the exception of
 * integer types on x86 without SSE4). */
inline Expr max(int a, const Expr &b) {
    user_assert(b.defined()) << "max of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::Max::make(Internal::make_const(b.type(), a), b);
}

inline Expr max(float a, const Expr &b) {return max(Expr(a), b);}
inline Expr max(const Expr &a, float b) {return max(a, Expr(b));}

/** Returns an expression representing the greater of an expressions
 * vector, after doing any necessary type coersion using
 * \ref Internal::match_types. Vectorizes cleanly on most platforms
 * (with the exception of integer types on x86 without SSE4).
 * The expressions are folded from right ie. max(.., max(.., ..)).
 * The arguments can be any mix of types but must all be convertible to Expr. */
template<typename A, typename B, typename C, typename... Rest,
         typename std::enable_if<Halide::Internal::all_are_convertible<Expr, Rest...>::value>::type* = nullptr>
inline Expr max(const A &a, const B &b, const C &c, Rest&&... rest) {
    return max(a, max(b, c, std::forward<Rest>(rest)...));
}

inline Expr min(Expr a, Expr b) {
    user_assert(a.defined() && b.defined())
        << "min of undefined Expr\n";
    Internal::match_types(a, b);
    return Internal::Min::make(a, b);
}

/** Returns an expression representing the lesser of an expression
 * and a constant integer.  The integer is coerced to the type of the
 * expression. Errors if the integer is not representable as that
 * type. Vectorizes cleanly on most platforms (with the exception of
 * integer types on x86 without SSE4). */
inline Expr min(const Expr &a, int b) {
    user_assert(a.defined()) << "max of undefined Expr\n";
    Internal::check_representable(a.type(), b);
    return Internal::Min::make(a, Internal::make_const(a.type(), b));
}

/** Returns an expression representing the lesser of a constant
 * integer and an expression. The integer is coerced to the type of
 * the expression. Errors if the integer is not representable as that
 * type. Vectorizes cleanly on most platforms (with the exception of
 * integer types on x86 without SSE4). */
inline Expr min(int a, const Expr &b) {
    user_assert(b.defined()) << "max of undefined Expr\n";
    Internal::check_representable(b.type(), a);
    return Internal::Min::make(Internal::make_const(b.type(), a), b);
}

inline Expr min(float a, const Expr &b) {return min(Expr(a), b);}
inline Expr min(const Expr &a, float b) {return min(a, Expr(b));}

/** Returns an expression representing the lesser of an expressions
 * vector, after doing any necessary type coersion using
 * \ref Internal::match_types. Vectorizes cleanly on most platforms
 * (with the exception of integer types on x86 without SSE4).
 * The expressions are folded from right ie. min(.., min(.., ..)).
 * The arguments can be any mix of types but must all be convertible to Expr. */
template<typename A, typename B, typename C, typename... Rest,
         typename std::enable_if<Halide::Internal::all_are_convertible<Expr, Rest...>::value>::type* = nullptr>
inline Expr min(const A &a, const B &b, const C &c, Rest&&... rest) {
    return min(a, min(b, c, std::forward<Rest>(rest)...));
}

/** Operators on floats treats those floats as Exprs. Making these
 * explicit prevents implicit float->int casts that might otherwise
 * occur. */
// @{
inline Expr operator+(const Expr &a, float b) {return a + Expr(b);}
inline Expr operator+(float a, const Expr &b) {return Expr(a) + b;}
inline Expr operator-(const Expr &a, float b) {return a - Expr(b);}
inline Expr operator-(float a, const Expr &b) {return Expr(a) - b;}
inline Expr operator*(const Expr &a, float b) {return a * Expr(b);}
inline Expr operator*(float a, const Expr &b) {return Expr(a) * b;}
inline Expr operator/(const Expr &a, float b) {return a / Expr(b);}
inline Expr operator/(float a, const Expr &b) {return Expr(a) / b;}
inline Expr operator%(const Expr &a, float b) {return a % Expr(b);}
inline Expr operator%(float a, const Expr &b) {return Expr(a) % b;}
inline Expr operator>(const Expr &a, float b) {return a > Expr(b);}
inline Expr operator>(float a, const Expr &b) {return Expr(a) > b;}
inline Expr operator<(const Expr &a, float b) {return a < Expr(b);}
inline Expr operator<(float a, const Expr &b) {return Expr(a) < b;}
inline Expr operator>=(const Expr &a, float b) {return a >= Expr(b);}
inline Expr operator>=(float a, const Expr &b) {return Expr(a) >= b;}
inline Expr operator<=(const Expr &a, float b) {return a <= Expr(b);}
inline Expr operator<=(float a, const Expr &b) {return Expr(a) <= b;}
inline Expr operator==(const Expr &a, float b) {return a == Expr(b);}
inline Expr operator==(float a, const Expr &b) {return Expr(a) == b;}
inline Expr operator!=(const Expr &a, float b) {return a != Expr(b);}
inline Expr operator!=(float a, const Expr &b) {return Expr(a) != b;}
// @}

/** Clamps an expression to lie within the given bounds. The bounds
 * are type-cast to match the expression. Vectorizes as well as min/max. */
inline Expr clamp(const Expr &a, const Expr &min_val, const Expr &max_val) {
    user_assert(a.defined() && min_val.defined() && max_val.defined())
        << "clamp of undefined Expr\n";
    Expr n_min_val = lossless_cast(a.type(), min_val);
    user_assert(n_min_val.defined())
        << "clamp with possibly out of range minimum bound: " << min_val << "\n";
    Expr n_max_val = lossless_cast(a.type(), max_val);
    user_assert(n_max_val.defined())
        << "clamp with possibly out of range maximum bound: " << max_val << "\n";
    return Internal::Max::make(Internal::Min::make(a, n_max_val), n_min_val);
}

/** Returns the absolute value of a signed integer or floating-point
 * expression. Vectorizes cleanly. Unlike in C, abs of a signed
 * integer returns an unsigned integer of the same bit width. This
 * means that abs of the most negative integer doesn't overflow. */
inline Expr abs(const Expr &a) {
    user_assert(a.defined())
        << "abs of undefined Expr\n";
    Type t = a.type();
    if (t.is_uint()) {
        user_warning << "Warning: abs of an unsigned type is a no-op\n";
        return a;
    }
    return Internal::Call::make(t.with_code(t.is_int() ? Type::UInt : t.code()),
                                Internal::Call::abs, {a}, Internal::Call::PureIntrinsic);
}

/** Return the absolute difference between two values. Vectorizes
 * cleanly. Returns an unsigned value of the same bit width. There are
 * various ways to write this yourself, but they contain numerous
 * gotchas and don't always compile to good code, so use this
 * instead. */
inline Expr absd(Expr a, Expr b) {
    user_assert(a.defined() && b.defined()) << "absd of undefined Expr\n";
    Internal::match_types(a, b);
    Type t = a.type();

    if (t.is_float()) {
        // Floats can just use abs.
        return abs(a - b);
    }

    // The argument may be signed, but the return type is unsigned.
    return Internal::Call::make(t.with_code(t.is_int() ? Type::UInt : t.code()),
                                Internal::Call::absd, {a, b},
                                Internal::Call::PureIntrinsic);
}

/** Returns an expression similar to the ternary operator in C, except
 * that it always evaluates all arguments. If the first argument is
 * true, then return the second, else return the third. Typically
 * vectorizes cleanly, but benefits from SSE41 or newer on x86. */
inline Expr select(Expr condition, Expr true_value, Expr false_value) {

    if (as_const_int(condition)) {
        // Why are you doing this? We'll preserve the select node until constant folding for you.
        condition = cast(Bool(), condition);
    }

    // Coerce int literals to the type of the other argument
    if (as_const_int(true_value)) {
        true_value = cast(false_value.type(), true_value);
    }
    if (as_const_int(false_value)) {
        false_value = cast(true_value.type(), false_value);
    }

    user_assert(condition.type().is_bool())
        << "The first argument to a select must be a boolean:\n"
        << "  " << condition << " has type " << condition.type() << "\n";

    user_assert(true_value.type() == false_value.type())
        << "The second and third arguments to a select do not have a matching type:\n"
        << "  " << true_value << " has type " << true_value.type() << "\n"
        << "  " << false_value << " has type " << false_value.type() << "\n";

    return Internal::Select::make(condition, true_value, false_value);
}

/** A multi-way variant of select similar to a switch statement in C,
 * which can accept multiple conditions and values in pairs. Evaluates
 * to the first value for which the condition is true. Returns the
 * final value if all conditions are false. */
template<typename... Args,
         typename std::enable_if<Halide::Internal::all_are_convertible<Expr, Args...>::value>::type* = nullptr>
inline Expr select(const Expr &c0, const Expr &v0, const Expr &c1, const Expr &v1, Args&&... args) {
    return select(c0, v0, select(c1, v1, std::forward<Args>(args)...));
}

// TODO: Implement support for *_f16 external functions in various backends.
// No backend supports these yet.

/** Return the sine of a floating-point expression. If the argument is
 * not floating-point, it is cast to Float(32). Does not vectorize
 * well. */
inline Expr sin(const Expr &x) {
    user_assert(x.defined()) << "sin of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "sin_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "sin_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "sin_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the arcsine of a floating-point expression. If the argument
 * is not floating-point, it is cast to Float(32). Does not vectorize
 * well. */
inline Expr asin(const Expr &x) {
    user_assert(x.defined()) << "asin of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "asin_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "asin_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "asin_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the cosine of a floating-point expression. If the argument
 * is not floating-point, it is cast to Float(32). Does not vectorize
 * well. */
inline Expr cos(const Expr &x) {
    user_assert(x.defined()) << "cos of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "cos_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "cos_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "cos_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the arccosine of a floating-point expression. If the
 * argument is not floating-point, it is cast to Float(32). Does not
 * vectorize well. */
inline Expr acos(const Expr &x) {
    user_assert(x.defined()) << "acos of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "acos_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "acos_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "acos_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the tangent of a floating-point expression. If the argument
 * is not floating-point, it is cast to Float(32). Does not vectorize
 * well. */
inline Expr tan(const Expr &x) {
    user_assert(x.defined()) << "tan of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "tan_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "tan_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "tan_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the arctangent of a floating-point expression. If the
 * argument is not floating-point, it is cast to Float(32). Does not
 * vectorize well. */
inline Expr atan(const Expr &x) {
    user_assert(x.defined()) << "atan of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "atan_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "atan_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "atan_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the angle of a floating-point gradient. If the argument is
 * not floating-point, it is cast to Float(32). Does not vectorize
 * well. */
inline Expr atan2(Expr y, Expr x) {
    user_assert(x.defined() && y.defined()) << "atan2 of undefined Expr\n";

    if (y.type() == Float(64)) {
        x = cast<double>(x);
        return Internal::Call::make(Float(64), "atan2_f64", {y, x}, Internal::Call::PureExtern);
    }
    else if (y.type() == Float(16)) {
        x = cast<float16_t>(x);
        return Internal::Call::make(Float(16), "atan2_f16", {y, x}, Internal::Call::PureExtern);
    }
    else {
        y = cast<float>(y);
        x = cast<float>(x);
        return Internal::Call::make(Float(32), "atan2_f32", {y, x}, Internal::Call::PureExtern);
    }
}

/** Return the hyperbolic sine of a floating-point expression.  If the
 *  argument is not floating-point, it is cast to Float(32). Does not
 *  vectorize well. */
inline Expr sinh(const Expr &x) {
    user_assert(x.defined()) << "sinh of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "sinh_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "sinh_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "sinh_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the hyperbolic arcsinhe of a floating-point expression.  If
 * the argument is not floating-point, it is cast to Float(32). Does
 * not vectorize well. */
inline Expr asinh(const Expr &x) {
    user_assert(x.defined()) << "asinh of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "asinh_f64", {x}, Internal::Call::PureExtern);
    }
    else if(x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "asinh_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "asinh_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the hyperbolic cosine of a floating-point expression.  If
 * the argument is not floating-point, it is cast to Float(32). Does
 * not vectorize well. */
inline Expr cosh(const Expr &x) {
    user_assert(x.defined()) << "cosh of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "cosh_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "cosh_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "cosh_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the hyperbolic arccosine of a floating-point expression.
 * If the argument is not floating-point, it is cast to
 * Float(32). Does not vectorize well. */
inline Expr acosh(const Expr &x) {
    user_assert(x.defined()) << "acosh of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "acosh_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "acosh_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "acosh_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the hyperbolic tangent of a floating-point expression.  If
 * the argument is not floating-point, it is cast to Float(32). Does
 * not vectorize well. */
inline Expr tanh(const Expr &x) {
    user_assert(x.defined()) << "tanh of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "tanh_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "tanh_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "tanh_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the hyperbolic arctangent of a floating-point expression.
 * If the argument is not floating-point, it is cast to
 * Float(32). Does not vectorize well. */
inline Expr atanh(const Expr &x) {
    user_assert(x.defined()) << "atanh of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "atanh_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "atanh_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "atanh_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the square root of a floating-point expression. If the
 * argument is not floating-point, it is cast to Float(32). Typically
 * vectorizes cleanly. */
inline Expr sqrt(const Expr &x) {
    user_assert(x.defined()) << "sqrt of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "sqrt_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "sqrt_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "sqrt_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the square root of the sum of the squares of two
 * floating-point expressions. If the argument is not floating-point,
 * it is cast to Float(32). Vectorizes cleanly. */
inline Expr hypot(const Expr &x, const Expr &y) {
    return sqrt(x*x + y*y);
}

/** Return the exponential of a floating-point expression. If the
 * argument is not floating-point, it is cast to Float(32). For
 * Float(64) arguments, this calls the system exp function, and does
 * not vectorize well. For Float(32) arguments, this function is
 * vectorizable, does the right thing for extremely small or extremely
 * large inputs, and is accurate up to the last bit of the
 * mantissa. Vectorizes cleanly. */
inline Expr exp(const Expr &x) {
    user_assert(x.defined()) << "exp of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "exp_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "exp_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "exp_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return the logarithm of a floating-point expression. If the
 * argument is not floating-point, it is cast to Float(32). For
 * Float(64) arguments, this calls the system log function, and does
 * not vectorize well. For Float(32) arguments, this function is
 * vectorizable, does the right thing for inputs <= 0 (returns -inf or
 * nan), and is accurate up to the last bit of the
 * mantissa. Vectorizes cleanly. */
inline Expr log(const Expr &x) {
    user_assert(x.defined()) << "log of undefined Expr\n";
    if (x.type() == Float(64)) {
        return Internal::Call::make(Float(64), "log_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
        return Internal::Call::make(Float(16), "log_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        return Internal::Call::make(Float(32), "log_f32", {cast<float>(x)}, Internal::Call::PureExtern);
    }
}

/** Return one floating point expression raised to the power of
 * another. The type of the result is given by the type of the first
 * argument. If the first argument is not a floating-point type, it is
 * cast to Float(32). For Float(32), cleanly vectorizable, and
 * accurate up to the last few bits of the mantissa. Gets worse when
 * approaching overflow. Vectorizes cleanly. */
inline Expr pow(Expr x, Expr y) {
    user_assert(x.defined() && y.defined()) << "pow of undefined Expr\n";

    if (const int64_t *i = as_const_int(y)) {
        return raise_to_integer_power(x, *i);
    }

    if (x.type() == Float(64)) {
        y = cast<double>(y);
        return Internal::Call::make(Float(64), "pow_f64", {x, y}, Internal::Call::PureExtern);
    }
    else if (x.type() == Float(16)) {
         y = cast<float16_t>(y);
        return Internal::Call::make(Float(16), "pow_f16", {x, y}, Internal::Call::PureExtern);
    }
    else {
        x = cast<float>(x);
        y = cast<float>(y);
        return Internal::Call::make(Float(32), "pow_f32", {x, y}, Internal::Call::PureExtern);
    }
}

/** Evaluate the error function erf. Only available for
 * Float(32). Accurate up to the last three bits of the
 * mantissa. Vectorizes cleanly. */
inline Expr erf(const Expr &x) {
    user_assert(x.defined()) << "erf of undefined Expr\n";
    user_assert(x.type() == Float(32)) << "erf only takes float arguments\n";
    return Internal::halide_erf(x);
}

/** Fast approximate cleanly vectorizable log for Float(32). Returns
 * nonsense for x <= 0.0f. Accurate up to the last 5 bits of the
 * mantissa. Vectorizes cleanly. */
EXPORT Expr fast_log(const Expr &x);

/** Fast approximate cleanly vectorizable exp for Float(32). Returns
 * nonsense for inputs that would overflow or underflow. Typically
 * accurate up to the last 5 bits of the mantissa. Gets worse when
 * approaching overflow. Vectorizes cleanly. */
EXPORT Expr fast_exp(const Expr &x);

/** Fast approximate cleanly vectorizable pow for Float(32). Returns
 * nonsense for x < 0.0f. Accurate up to the last 5 bits of the
 * mantissa for typical exponents. Gets worse when approaching
 * overflow. Vectorizes cleanly. */
inline Expr fast_pow(Expr x, Expr y) {
    if (const int64_t *i = as_const_int(y)) {
        return raise_to_integer_power(x, *i);
    }

    x = cast<float>(x);
    y = cast<float>(y);
    return select(x == 0.0f, 0.0f, fast_exp(fast_log(x) * y));
}

/** Fast approximate inverse for Float(32). Corresponds to the rcpps
 * instruction on x86, and the vrecpe instruction on ARM. Vectorizes
 * cleanly. */
inline Expr fast_inverse(const Expr &x) {
    user_assert(x.type() == Float(32)) << "fast_inverse only takes float arguments\n";
    return Internal::Call::make(x.type(), "fast_inverse_f32", {x}, Internal::Call::PureExtern);
}

/** Fast approximate inverse square root for Float(32). Corresponds to
 * the rsqrtps instruction on x86, and the vrsqrte instruction on
 * ARM. Vectorizes cleanly. */
inline Expr fast_inverse_sqrt(const Expr &x) {
    user_assert(x.type() == Float(32)) << "fast_inverse_sqrt only takes float arguments\n";
    return Internal::Call::make(x.type(), "fast_inverse_sqrt_f32", {x}, Internal::Call::PureExtern);
}

/** Return the greatest whole number less than or equal to a
 * floating-point expression. If the argument is not floating-point,
 * it is cast to Float(32). The return value is still in floating
 * point, despite being a whole number. Vectorizes cleanly. */
inline Expr floor(const Expr &x) {
    user_assert(x.defined()) << "floor of undefined Expr\n";
    if (x.type().element_of() == Float(64)) {
        return Internal::Call::make(x.type(), "floor_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type().element_of() == Float(16)) {
        return Internal::Call::make(Float(16), "floor_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        Type t = Float(32, x.type().lanes());
        return Internal::Call::make(t, "floor_f32", {cast(t, x)}, Internal::Call::PureExtern);
    }
}

/** Return the least whole number greater than or equal to a
 * floating-point expression. If the argument is not floating-point,
 * it is cast to Float(32). The return value is still in floating
 * point, despite being a whole number. Vectorizes cleanly. */
inline Expr ceil(const Expr &x) {
    user_assert(x.defined()) << "ceil of undefined Expr\n";
    if (x.type().element_of() == Float(64)) {
        return Internal::Call::make(x.type(), "ceil_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type().element_of() == Float(16)) {
        return Internal::Call::make(Float(16), "ceil_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        Type t = Float(32, x.type().lanes());
        return Internal::Call::make(t, "ceil_f32", {cast(t, x)}, Internal::Call::PureExtern);
    }
}

/** Return the whole number closest to a floating-point expression. If the
 * argument is not floating-point, it is cast to Float(32). The return value
 * is still in floating point, despite being a whole number. On ties, we
 * follow IEEE754 conventions and round to the nearest even number. Vectorizes
 * cleanly. */
inline Expr round(const Expr &x) {
    user_assert(x.defined()) << "round of undefined Expr\n";
    if (x.type().element_of() == Float(64)) {
        return Internal::Call::make(Float(64), "round_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type().element_of() == Float(16)) {
        return Internal::Call::make(Float(16), "round_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        Type t = Float(32, x.type().lanes());
        return Internal::Call::make(t, "round_f32", {cast(t, x)}, Internal::Call::PureExtern);
    }
}

/** Return the integer part of a floating-point expression. If the argument is
 * not floating-point, it is cast to Float(32). The return value is still in
 * floating point, despite being a whole number. Vectorizes cleanly. */
inline Expr trunc(const Expr &x) {
    user_assert(x.defined()) << "trunc of undefined Expr\n";
    if (x.type().element_of() == Float(64)) {
        return Internal::Call::make(Float(64), "trunc_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type().element_of() == Float(16)) {
        return Internal::Call::make(Float(16), "trunc_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        Type t = Float(32, x.type().lanes());
        return Internal::Call::make(t, "trunc_f32", {cast(t, x)}, Internal::Call::PureExtern);
    }
}

/** Returns true if the argument is a Not a Number (NaN). Requires a
  * floating point argument.  Vectorizes cleanly. */
inline Expr is_nan(const Expr &x) {
    user_assert(x.defined()) << "is_nan of undefined Expr\n";
    user_assert(x.type().is_float()) << "is_nan only works for float";
    Type t = Bool(x.type().lanes());
    if (x.type().element_of() == Float(64)) {
        return Internal::Call::make(t, "is_nan_f64", {x}, Internal::Call::PureExtern);
    }
    else if (x.type().element_of() == Float(64)) {
        return Internal::Call::make(t, "is_nan_f16", {x}, Internal::Call::PureExtern);
    }
    else {
        Type ft = Float(32, x.type().lanes());
        return Internal::Call::make(t, "is_nan_f32", {cast(ft, x)}, Internal::Call::PureExtern);
    }
}

/** Return the fractional part of a floating-point expression. If the argument
 *  is not floating-point, it is cast to Float(32). The return value has the
 *  same sign as the original expression. Vectorizes cleanly. */
inline Expr fract(const Expr &x) {
    user_assert(x.defined()) << "fract of undefined Expr\n";
    return x - trunc(x);
}

/** Reinterpret the bits of one value as another type. */
inline Expr reinterpret(Type t, const Expr &e) {
    user_assert(e.defined()) << "reinterpret of undefined Expr\n";
    int from_bits = e.type().bits() * e.type().lanes();
    int to_bits = t.bits() * t.lanes();
    user_assert(from_bits == to_bits)
        << "Reinterpret cast from type " << e.type()
        << " which has " << from_bits
        << " bits, to type " << t
        << " which has " << to_bits << " bits\n";
    return Internal::Call::make(t, Internal::Call::reinterpret, {e}, Internal::Call::PureIntrinsic);
}

template<typename T>
inline Expr reinterpret(const Expr &e) {
    return reinterpret(type_of<T>(), e);
}

/** Return the bitwise and of two expressions (which need not have the
 * same type). The type of the result is the type of the first
 * argument. */
inline Expr operator&(Expr x, Expr y) {
    user_assert(x.defined() && y.defined()) << "bitwise and of undefined Expr\n";
    user_assert(x.type().is_int() || x.type().is_uint())
        << "The first argument to bitwise and must be an integer or unsigned integer";
    user_assert(y.type().is_int() || y.type().is_uint())
        << "The second argument to bitwise and must be an integer or unsigned integer";
    // First widen or narrow, then bitcast.
    if (y.type().bits() != x.type().bits()) {
        y = cast(y.type().with_bits(x.type().bits()), y);
    }
    if (y.type() != x.type()) {
        y = reinterpret(x.type(), y);
    }
    return Internal::Call::make(x.type(), Internal::Call::bitwise_and, {x, y}, Internal::Call::PureIntrinsic);
}

/** Return the bitwise or of two expressions (which need not have the
 * same type). The type of the result is the type of the first
 * argument. */
inline Expr operator|(Expr x, Expr y) {
    user_assert(x.defined() && y.defined()) << "bitwise or of undefined Expr\n";
    user_assert(x.type().is_int() || x.type().is_uint())
        << "The first argument to bitwise or must be an integer or unsigned integer";
    user_assert(y.type().is_int() || y.type().is_uint())
        << "The second argument to bitwise or must be an integer or unsigned integer";
    // First widen or narrow, then bitcast.
    if (y.type().bits() != x.type().bits()) {
        y = cast(y.type().with_bits(x.type().bits()), y);
    }
    if (y.type() != x.type()) {
        y = reinterpret(x.type(), y);
    }
    return Internal::Call::make(x.type(), Internal::Call::bitwise_or, {x, y}, Internal::Call::PureIntrinsic);
}

/** Return the bitwise exclusive or of two expressions (which need not
 * have the same type). The type of the result is the type of the
 * first argument. */
inline Expr operator^(Expr x, Expr y) {
    user_assert(x.defined() && y.defined()) << "bitwise xor of undefined Expr\n";
    user_assert(x.type().is_int() || x.type().is_uint())
        << "The first argument to bitwise xor must be an integer or unsigned integer";
    user_assert(y.type().is_int() || y.type().is_uint())
        << "The second argument to bitwise xor must be an integer or unsigned integer";
    // First widen or narrow, then bitcast.
    if (y.type().bits() != x.type().bits()) {
        y = cast(y.type().with_bits(x.type().bits()), y);
    }
    if (y.type() != x.type()) {
        y = reinterpret(x.type(), y);
    }
    return Internal::Call::make(x.type(), Internal::Call::bitwise_xor, {x, y}, Internal::Call::PureIntrinsic);
}

/** Return the bitwise not of an expression. */
inline Expr operator~(const Expr &x) {
    user_assert(x.defined()) << "bitwise not of undefined Expr\n";
    user_assert(x.type().is_int() || x.type().is_uint())
        << "Argument to bitwise not must be an integer or unsigned integer";
    return Internal::Call::make(x.type(), Internal::Call::bitwise_not, {x}, Internal::Call::PureIntrinsic);
}

/** Shift the bits of an integer value left. This is actually less
 * efficient than multiplying by 2^n, because Halide's optimization
 * passes understand multiplication, and will compile it to
 * shifting. This operator is only for if you really really need bit
 * shifting (e.g. because the exponent is a run-time parameter). The
 * type of the result is equal to the type of the first argument. Both
 * arguments must have integer type. */
// @{
inline Expr operator<<(Expr x, Expr y) {
    user_assert(x.defined() && y.defined()) << "shift left of undefined Expr\n";
    user_assert(!x.type().is_float()) << "First argument to shift left is a float: " << x << "\n";
    user_assert(!y.type().is_float()) << "Second argument to shift left is a float: " << y << "\n";
    Internal::match_types(x, y);
    return Internal::Call::make(x.type(), Internal::Call::shift_left, {x, y}, Internal::Call::PureIntrinsic);
}
inline Expr operator<<(const Expr &x, int y) {
    Internal::check_representable(x.type(), y);
    return x << Internal::make_const(x.type(), y);
}
inline Expr operator<<(int x, const Expr &y) {
    Internal::check_representable(y.type(), x);
    return Internal::make_const(y.type(), x) << y;
}
// @}

/** Shift the bits of an integer value right. Does sign extension for
 * signed integers. This is less efficient than dividing by a power of
 * two. Halide's definition of division (always round to negative
 * infinity) means that all divisions by powers of two get compiled to
 * bit-shifting, and Halide's optimization routines understand
 * division and can work with it. The type of the result is equal to
 * the type of the first argument. Both arguments must have integer
 * type. */
// @{
inline Expr operator>>(Expr x, Expr y) {
    user_assert(x.defined() && y.defined()) << "shift right of undefined Expr\n";
    user_assert(!x.type().is_float()) << "First argument to shift right is a float: " << x << "\n";
    user_assert(!y.type().is_float()) << "Second argument to shift right is a float: " << y << "\n";
    Internal::match_types(x, y);
    return Internal::Call::make(x.type(), Internal::Call::shift_right, {x, y}, Internal::Call::PureIntrinsic);
}
inline Expr operator>>(const Expr &x, int y) {
    Internal::check_representable(x.type(), y);
    return x >> Internal::make_const(x.type(), y);
}
inline Expr operator>>(int x, const Expr &y) {
    Internal::check_representable(y.type(), x);
    return Internal::make_const(y.type(), x) >> y;
}
// @}

/** Linear interpolate between the two values according to a weight.
 * \param zero_val The result when weight is 0
 * \param one_val The result when weight is 1
 * \param weight The interpolation amount
 *
 * Both zero_val and one_val must have the same type. All types are
 * supported, including bool.
 *
 * The weight is treated as its own type and must be float or an
 * unsigned integer type. It is scaled to the bit-size of the type of
 * x and y if they are integer, or converted to float if they are
 * float. Integer weights are converted to float via division by the
 * full-range value of the weight's type. Floating-point weights used
 * to interpolate between integer values must be between 0.0f and
 * 1.0f, and an error may be signaled if it is not provably so. (clamp
 * operators can be added to provide proof. Currently an error is only
 * signalled for constant weights.)
 *
 * For integer linear interpolation, out of range values cannot be
 * represented. In particular, weights that are conceptually less than
 * 0 or greater than 1.0 are not representable. As such the result is
 * always between x and y (inclusive of course). For lerp with
 * floating-point values and floating-point weight, the full range of
 * a float is valid, however underflow and overflow can still occur.
 *
 * Ordering is not required between zero_val and one_val:
 *     lerp(42, 69, .5f) == lerp(69, 42, .5f) == 56
 *
 * Results for integer types are for exactly rounded arithmetic. As
 * such, there are cases where 16-bit and float differ because 32-bit
 * floating-point (float) does not have enough precision to produce
 * the exact result. (Likely true for 32-bit integer
 * vs. double-precision floating-point as well.)
 *
 * At present, double precision and 64-bit integers are not supported.
 *
 * Generally, lerp will vectorize as if it were an operation on a type
 * twice the bit size of the inferred type for x and y.
 *
 * Some examples:
 * \code
 *
 *     // Since Halide does not have direct type delcarations, casts
 *     // below are used to indicate the types of the parameters.
 *     // Such casts not required or expected in actual code where types
 *     // are inferred.
 *
 *     lerp(cast<float>(x), cast<float>(y), cast<float>(w)) ->
 *       x * (1.0f - w) + y * w
 *
 *     lerp(cast<uint8_t>(x), cast<uint8_t>(y), cast<uint8_t>(w)) ->
 *       cast<uint8_t>(cast<uint8_t>(x) * (1.0f - cast<uint8_t>(w) / 255.0f) +
 *                     cast<uint8_t>(y) * cast<uint8_t>(w) / 255.0f + .5f)
 *
 *     // Note addition in Halide promoted uint8_t + int8_t to int16_t already,
 *     // the outer cast is added for clarity.
 *     lerp(cast<uint8_t>(x), cast<int8_t>(y), cast<uint8_t>(w)) ->
 *       cast<int16_t>(cast<uint8_t>(x) * (1.0f - cast<uint8_t>(w) / 255.0f) +
 *                     cast<int8_t>(y) * cast<uint8_t>(w) / 255.0f + .5f)
 *
 *     lerp(cast<int8_t>(x), cast<int8_t>(y), cast<float>(w)) ->
 *       cast<int8_t>(cast<int8_t>(x) * (1.0f - cast<float>(w)) +
 *                    cast<int8_t>(y) * cast<uint8_t>(w))
 *
 * \endcode
 * */
inline Expr lerp(Expr zero_val, Expr one_val, Expr weight) {
    user_assert(zero_val.defined()) << "lerp with undefined zero value";
    user_assert(one_val.defined()) << "lerp with undefined one value";
    user_assert(weight.defined()) << "lerp with undefined weight";

    // We allow integer constants through, so that you can say things
    // like lerp(0, cast<uint8_t>(x), alpha) and produce an 8-bit
    // result. Note that lerp(0.0f, cast<uint8_t>(x), alpha) will
    // produce an error, as will lerp(0.0f, cast<double>(x),
    // alpha). lerp(0, cast<float>(x), alpha) is also allowed and will
    // produce a float result.
    if (as_const_int(zero_val)) {
        zero_val = cast(one_val.type(), zero_val);
    }
    if (as_const_int(one_val)) {
        one_val = cast(zero_val.type(), one_val);
    }

    user_assert(zero_val.type() == one_val.type())
        << "Can't lerp between " << zero_val << " of type " << zero_val.type()
        << " and " << one_val << " of different type " << one_val.type() << "\n";
    user_assert((weight.type().is_uint() || weight.type().is_float()))
        << "A lerp weight must be an unsigned integer or a float, but "
        << "lerp weight " << weight << " has type " << weight.type() << ".\n";
    user_assert((zero_val.type().is_float() || zero_val.type().lanes() <= 32))
        << "Lerping between 64-bit integers is not supported\n";
    // Compilation error for constant weight that is out of range for integer use
    // as this seems like an easy to catch gotcha.
    if (!zero_val.type().is_float()) {
        const double *const_weight = as_const_float(weight);
        if (const_weight) {
            user_assert(*const_weight >= 0.0 && *const_weight <= 1.0)
                << "Floating-point weight for lerp with integer arguments is "
                << *const_weight << ", which is not in the range [0.0, 1.0].\n";
        }
    }
    return Internal::Call::make(zero_val.type(), Internal::Call::lerp,
                                {zero_val, one_val, weight},
                                Internal::Call::PureIntrinsic);
}

/** Count the number of set bits in an expression. */
inline Expr popcount(const Expr &x) {
    user_assert(x.defined()) << "popcount of undefined Expr\n";
    return Internal::Call::make(x.type(), Internal::Call::popcount,
                                {x}, Internal::Call::PureIntrinsic);
}

/** Count the number of leading zero bits in an expression. The result is
 *  undefined if the value of the expression is zero. */
inline Expr count_leading_zeros(const Expr &x) {
    user_assert(x.defined()) << "count leading zeros of undefined Expr\n";
    return Internal::Call::make(x.type(), Internal::Call::count_leading_zeros,
                                {x}, Internal::Call::PureIntrinsic);
}

/** Count the number of trailing zero bits in an expression. The result is
 *  undefined if the value of the expression is zero. */
inline Expr count_trailing_zeros(const Expr &x) {
    user_assert(x.defined()) << "count trailing zeros of undefined Expr\n";
    return Internal::Call::make(x.type(), Internal::Call::count_trailing_zeros,
                                {x}, Internal::Call::PureIntrinsic);
}

/** Divide two integers, rounding towards zero. This is the typical
 * behavior of most hardware architectures, which differs from
 * Halide's division operator, which is Euclidean (rounds towards
 * -infinity). */
inline Expr div_round_to_zero(Expr x, Expr y) {
    user_assert(x.defined()) << "div_round_to_zero of undefined dividend\n";
    user_assert(y.defined()) << "div_round_to_zero of undefined divisor\n";
    Internal::match_types(x, y);
    if (x.type().is_uint()) {
        return x / y;
    }
    user_assert(x.type().is_int()) << "First argument to div_round_to_zero is not an integer: " << x << "\n";
    user_assert(y.type().is_int()) << "Second argument to div_round_to_zero is not an integer: " << y << "\n";
    return Internal::Call::make(x.type(), Internal::Call::div_round_to_zero,
                                {x, y},
                                Internal::Call::PureIntrinsic);
}

/** Compute the remainder of dividing two integers, when division is
 * rounding toward zero. This is the typical behavior of most hardware
 * architectures, which differs from Halide's mod operator, which is
 * Euclidean (produces the remainder when division rounds towards
 * -infinity). */
inline Expr mod_round_to_zero(Expr x, Expr y) {
    user_assert(x.defined()) << "mod_round_to_zero of undefined dividend\n";
    user_assert(y.defined()) << "mod_round_to_zero of undefined divisor\n";
    Internal::match_types(x, y);
    if (x.type().is_uint()) {
        return x % y;
    }
    user_assert(x.type().is_int()) << "First argument to mod_round_to_zero is not an integer: " << x << "\n";
    user_assert(y.type().is_int()) << "Second argument to mod_round_to_zero is not an integer: " << y << "\n";
    return Internal::Call::make(x.type(), Internal::Call::mod_round_to_zero,
                                {x, y},
                                Internal::Call::PureIntrinsic);
}

/** Return a random variable representing a uniformly distributed
 * float in the half-open interval [0.0f, 1.0f). For random numbers of
 * other types, use lerp with a random float as the last parameter.
 *
 * Optionally takes a seed.
 *
 * Note that:
 \code
 Expr x = random_float();
 Expr y = x + x;
 \endcode
 *
 * is very different to
 *
 \code
 Expr y = random_float() + random_float();
 \endcode
 *
 * The first doubles a random variable, and the second adds two
 * independent random variables.
 *
 * A given random variable takes on a unique value that depends
 * deterministically on the pure variables of the function they belong
 * to, the identity of the function itself, and which definition of
 * the function it is used in. They are, however, shared across tuple
 * elements.
 *
 * This function vectorizes cleanly.
 */
inline Expr random_float(const Expr &seed = Expr()) {
    // Random floats get even IDs
    static std::atomic<int> counter;
    int id = (counter++)*2;

    std::vector<Expr> args;
    if (seed.defined()) {
        user_assert(seed.type() == Int(32))
            << "The seed passed to random_float must have type Int(32), but instead is "
            << seed << " of type " << seed.type() << "\n";
        args.push_back(seed);
    }
    args.push_back(id);

    // This is (surprisingly) pure - it's a fixed psuedo-random
    // function of its inputs.
    return Internal::Call::make(Float(32), Internal::Call::random,
                                args, Internal::Call::PureIntrinsic);
}

/** Return a random variable representing a uniformly distributed
 * unsigned 32-bit integer. See \ref random_float. Vectorizes cleanly. */
inline Expr random_uint(const Expr &seed = Expr()) {
    // Random ints get odd IDs
    static std::atomic<int> counter;
    int id = (counter++)*2 + 1;

    std::vector<Expr> args;
    if (seed.defined()) {
        user_assert(seed.type() == Int(32) || seed.type() == UInt(32))
            << "The seed passed to random_int must have type Int(32) or UInt(32), but instead is "
            << seed << " of type " << seed.type() << "\n";
        args.push_back(seed);
    }
    args.push_back(id);

    return Internal::Call::make(UInt(32), Internal::Call::random,
                                args, Internal::Call::PureIntrinsic);
}

/** Return a random variable representing a uniformly distributed
 * 32-bit integer. See \ref random_float. Vectorizes cleanly. */
inline Expr random_int(const Expr &seed = Expr()) {
    return cast<int32_t>(random_uint(seed));
}

// Secondary args to print can be Exprs or const char *
namespace Internal {
inline NO_INLINE void collect_print_args(std::vector<Expr> &args) {
}

template<typename ...Args>
inline NO_INLINE void collect_print_args(std::vector<Expr> &args, const char *arg, Args&&... more_args) {
    args.push_back(Expr(std::string(arg)));
    collect_print_args(args, std::forward<Args>(more_args)...);
}

template<typename ...Args>
inline NO_INLINE void collect_print_args(std::vector<Expr> &args, const Expr &arg, Args&&... more_args) {
    args.push_back(arg);
    collect_print_args(args, std::forward<Args>(more_args)...);
}
}


/** Create an Expr that prints out its value whenever it is
 * evaluated. It also prints out everything else in the arguments
 * list, separated by spaces. This can include string literals. */
//@{
EXPORT Expr print(const std::vector<Expr> &values);

template <typename... Args>
inline NO_INLINE Expr print(const Expr &a, Args&&... args) {
    std::vector<Expr> collected_args = {a};
    Internal::collect_print_args(collected_args, std::forward<Args>(args)...);
    return print(collected_args);
}
//@}

/** Create an Expr that prints whenever it is evaluated, provided that
 * the condition is true. */
// @{
EXPORT Expr print_when(const Expr &condition, const std::vector<Expr> &values);

template<typename ...Args>
inline NO_INLINE Expr print_when(const Expr &condition, const Expr &a, Args&&... args) {
    std::vector<Expr> collected_args = {a};
    Internal::collect_print_args(collected_args, std::forward<Args>(args)...);
    return print_when(condition, collected_args);
}

// @}

/** Create an Expr that that guarantees a precondition.
 * If 'condition' is true, the return value is equal to the first Expr.
 * If 'condition' is false, halide_error() is called, and the return value
 * is arbitrary. Any additional arguments after the first Expr are stringified
 * and passed as a user-facing message to halide_error(), similar to print().
 *
 * Note that this essentially *always* inserts a runtime check into the
 * generated code (except when the condition can be proven at compile time);
 * as such, it should be avoided inside inner loops, except for debugging
 * or testing purposes.
 *
 * However, using this to make assertions about (say) input values
 * can be useful, both in terms of correctness and (potentially) in terms
 * of code generation, e.g.
 \code
 Param<int> p;
 Expr y = require(p > 0, p);
 \endcode
 * will allow the optimizer to assume positive, nonzero values for y.
 */
// @{
EXPORT Expr require(const Expr &condition, const std::vector<Expr> &values);

template<typename ...Args>
inline NO_INLINE Expr require(const Expr &condition, const Expr &value, Args&&... args) {
    std::vector<Expr> collected_args = {value};
    Internal::collect_print_args(collected_args, std::forward<Args>(args)...);
    return require(condition, collected_args);
}

// @}


/** Return an undef value of the given type. Halide skips stores that
 * depend on undef values, so you can use this to mean "do not modify
 * this memory location". This is an escape hatch that can be used for
 * several things:
 *
 * You can define a reduction with no pure step, by setting the pure
 * step to undef. Do this only if you're confident that the update
 * steps are sufficient to correctly fill in the domain.
 *
 * For a tuple-valued reduction, you can write an update step that
 * only updates some tuple elements.
 *
 * You can define single-stage pipeline that only has update steps,
 * and depends on the values already in the output buffer.
 *
 * Use this feature with great caution, as you can use it to load from
 * uninitialized memory.
 */
inline Expr undef(Type t) {
    return Internal::Call::make(t, Internal::Call::undef,
                                std::vector<Expr>(),
                                Internal::Call::PureIntrinsic);
}

template<typename T>
inline Expr undef() {
    return undef(type_of<T>());
}

namespace Internal {
EXPORT Expr memoize_tag_helper(const Expr &result, const std::vector<Expr> &cache_key_values);
}  // namespace Internal

/** Control the values used in the memoization cache key for memoize.
 * Normally parameters and other external dependencies are
 * automatically inferred and added to the cache key. The memoize_tag
 * operator allows computing one expression and using either the
 * computed value, or one or more other expressions in the cache key
 * instead of the parameter dependencies of the computation. The
 * single argument version is completely safe in that the cache key
 * will use the actual computed value -- it is difficult or imposible
 * to produce erroneous caching this way. The more-than-one argument
 * version allows generating cache keys that do not uniquely identify
 * the computation and thus can result in caching errors.
 *
 * A potential use for the single argument version is to handle a
 * floating-point parameter that is quantized to a small
 * integer. Mutliple values of the float will produce the same integer
 * and moving the caching to using the integer for the key is more
 * efficient.
 *
 * The main use for the more-than-one argument version is to provide
 * cache key information for Handles and ImageParams, which otherwise
 * are not allowed inside compute_cached operations. E.g. when passing
 * a group of parameters to an external array function via a Handle,
 * memoize_tag can be used to isolate the actual values used by that
 * computation. If an ImageParam is a constant image with a persistent
 * digest, memoize_tag can be used to key computations using that image
 * on the digest. */
// @{
template<typename ...Args>
inline NO_INLINE Expr memoize_tag(const Expr &result, Args&&... args) {
    std::vector<Expr> collected_args{std::forward<Args>(args)...};
    return Internal::memoize_tag_helper(result, collected_args);
}
// @}

/** Expressions tagged with this intrinsic are considered to be part
 * of the steady state of some loop with a nasty beginning and end
 * (e.g. a boundary condition). When Halide encounters likely
 * intrinsics, it splits the containing loop body into three, and
 * tries to simplify down all conditions that lead to the likely. For
 * example, given the expression: select(x < 1, bar, x > 10, bar,
 * likely(foo)), Halide will split the loop over x into portions where
 * x < 1, 1 <= x <= 10, and x > 10.
 *
 * You're unlikely to want to call this directly. You probably want to
 * use the boundary condition helpers in the BoundaryConditions
 * namespace instead.
 */
inline Expr likely(const Expr &e) {
    return Internal::Call::make(e.type(), Internal::Call::likely,
                                {e}, Internal::Call::PureIntrinsic);
}

/** Equivalent to likely, but only triggers a loop partitioning if
 * found in an innermost loop. */
inline Expr likely_if_innermost(const Expr &e) {
    return Internal::Call::make(e.type(), Internal::Call::likely_if_innermost,
                                {e}, Internal::Call::PureIntrinsic);
}


/** Cast an expression to the halide type corresponding to the C++
 * type T clamping to the minimum and maximum values of the result
 * type. */
template <typename T>
Expr saturating_cast(const Expr &e) {
    return saturating_cast(type_of<T>(), e);
}

/** Cast an expression to a new type, clamping to the minimum and
 * maximum values of the result type. */
EXPORT Expr saturating_cast(Type t, Expr e);

}

#endif
