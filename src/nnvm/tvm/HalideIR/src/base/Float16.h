#ifndef HALIDE_FLOAT16_H
#define HALIDE_FLOAT16_H

#include <stdint.h>
#include <string>
#include "./TypeBase.h"
#include "./RoundingMode.h"
#include "./Util.h"

namespace Halide {

/** Class that provides a type that implements half precision
 *  floating point (IEEE754 2008 binary16) in software.
 *
 *  This type is enforced to be 16-bits wide and maintains no state
 *  other than the raw IEEE754 binary16 bits so that it can passed
 *  to code that checks a type's size and used for buffer_t allocation.
 * */
struct float16_t {
    // NOTE: Do not use virtual methods here
    // it will change the size of this data type.

    /// \name Constructors
    /// @{

    /** Construct from a float using a particular rounding mode.
     *  A warning will be emitted if the result cannot be represented exactly
     *  and error will be raised if the conversion results in overflow.
     *
     *  \param value the input float
     *  \param roundingMode The rounding mode to use
     *
     */
    EXPORT explicit float16_t(float value, RoundingMode roundingMode=RoundingMode::ToNearestTiesToEven);

    /** Construct from a double using a particular rounding mode.
     *  A warning will be emitted if the result cannot be represented exactly
     *  and error will be raised if the conversion results in overflow.
     *
     *  \param value the input double
     *  \param roundingMode The rounding mode to use
     *
     */
    EXPORT explicit float16_t(double value, RoundingMode roundingMode=RoundingMode::ToNearestTiesToEven);

    /** Construct by parsing a string using a particular rounding mode.
     *  A warning will be emitted if the result cannot be represented exactly
     *  and error will be raised if the conversion results in overflow.
     *
     *  \param stringRepr the input string. The string maybe in C99 hex format
     *         (e.g. ``-0x1.000p-1``) or in a decimal (e.g.``-0.5``) format.
     *
     *  \param roundingMode The rounding mode to use
     *
     */
    EXPORT explicit float16_t(const char *stringRepr, RoundingMode roundingMode=RoundingMode::ToNearestTiesToEven);

    /** Construct a float16_t with the bits initialised to 0. This represents
     * positive zero.*/
    EXPORT float16_t();

    /// @}

    // Use explicit to avoid accidently raising the precision
    /** Cast to float */
    EXPORT explicit operator float() const;
    /** Cast to double */
    EXPORT explicit operator double() const;

    // Be explicit about how the copy constructor is expected to behave
    EXPORT float16_t(const float16_t&) = default;

    // Be explicit about how assignment is expected to behave
    EXPORT float16_t& operator=(const float16_t&) = default;

    /** \name Convenience "constructors"
     */
    /**@{*/

    /** Get a new float16_t that represents zero
     * \param positive if true then returns positive zero otherwise returns
     *        negative zero.
     */
    EXPORT static float16_t make_zero(bool positive);

    /** Get a new float16_t that represents infinity
     * \param positive if true then returns positive infinity otherwise returns
     *        negative infinity.
     */
    EXPORT static float16_t make_infinity(bool positive);

    /** Get a new float16_t that represents NaN (not a number) */
    EXPORT static float16_t make_nan();

    /** Get a new float16_t with the given raw bits
     *
     * \param bits The bits conformant to IEEE754 binary16
     */
    EXPORT static float16_t make_from_bits(uint16_t bits);

    /** Get a new float16_t from a signed integer.
     *  It is not provided as a constructor to avoid call ambiguity
     * */
    EXPORT static float16_t make_from_signed_int(int64_t value, RoundingMode roundingMode=RoundingMode::ToNearestTiesToEven);
    /**@}*/

    /**\name Arithmetic operators
     * These compute the result of an arithmetic operation
     * using a particular ``roundingMode`` and return a new float16_t
     * representing the result.
     *
     * Exceptions are ignored.
     */
    /**@{*/
    /** add */
    EXPORT float16_t add(float16_t rhs, RoundingMode roundingMode) const;
    /** subtract */
    EXPORT float16_t subtract(float16_t rhs, RoundingMode roundingMode) const;
    /** multiply */
    EXPORT float16_t multiply(float16_t rhs, RoundingMode roundingMode) const;
    /** divide */
    EXPORT float16_t divide(float16_t denominator, RoundingMode roundingMode) const;
    /** IEEE-754 2008 5.3.1 General operations - remainder **/
    EXPORT float16_t remainder(float16_t denominator) const;
    /** C fmod() */
    EXPORT float16_t mod(float16_t denominator, RoundingMode roudingMode) const;
    /**@}*/


    /** Return a new float16_t with a negated sign bit*/
    EXPORT float16_t operator-() const;

    /** \name Overloaded arithmetic operators for convenience
     * These operators assume RoundingMode::ToNearestTiesToEven rounding
     */
    /**@{*/
    EXPORT float16_t operator+(float16_t rhs) const;
    EXPORT float16_t operator-(float16_t rhs) const;
    EXPORT float16_t operator*(float16_t rhs) const;
    EXPORT float16_t operator/(float16_t rhs) const;
    /**@}*/

    /** \name Comparison operators */
    /**@{*/
    /** Equality */
    EXPORT bool operator==(float16_t rhs) const;
    /** Not equal */
    EXPORT bool operator!=(float16_t rhs) const { return !(*this == rhs); }
    /** Greater than */
    EXPORT bool operator>(float16_t rhs) const;
    /** Less than */
    EXPORT bool operator<(float16_t rhs) const;
    /** Greater than or equal to*/
    EXPORT bool operator>=(float16_t rhs) const { return (*this > rhs) || (*this == rhs); }
    /** Less than or equal to*/
    EXPORT bool operator<=(float16_t rhs) const { return (*this < rhs) || (*this == rhs); }
    /** \return true if and only if the float16_t and ``rhs`` are not ordered. E.g.
     * NaN and a normalised number
     */
    EXPORT bool are_unordered(float16_t rhs) const;
    /**@}*/

    /** \name String output methods */
    /**@{*/
    /** Return a string in the C99 hex format (e.g.\ ``-0x1.000p-1``) that
     * represents this float16_t precisely.
     */
    EXPORT std::string to_hex_string() const;
    /** Returns a string in a decimal scientific notation (e.g.\ ``-5.0E-1``)
     * that represents the closest decimal value to this float16_t precise to
     * the number of significant digits requested.
     *
     * \param significantDigits The number of significant digits to use. If
     *        set to ``0`` then string returned will have enough precision to
     *        construct the same float16_t when using
     *        RoundingMode::ToNearestTiesToEven
     */
    EXPORT std::string to_decimal_string(unsigned int significantDigits = 0) const;
    /**@}*/

    /** \name Properties */
    /*@{*/
    EXPORT bool is_nan() const;
    EXPORT bool is_infinity() const;
    EXPORT bool is_negative() const;
    EXPORT bool is_zero() const;
    /*@}*/

    /** Returns the bits that represent this float16_t.
     *
     *  An alternative method to access the bits is to cast a pointer
     *  to this instance as a pointer to a uint16_t.
     **/
    EXPORT uint16_t to_bits() const;

private:
    // The raw bits.
    // This must be the **ONLY** data member so that
    // this data type is 16-bits wide.
    uint16_t data;
};
}  // namespace Halide

template<>
HALIDE_ALWAYS_INLINE halide_type_t halide_type_of<Halide::float16_t>() {
    return halide_type_t(halide_type_float, 16);
}

#endif
