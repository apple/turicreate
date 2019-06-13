/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_UNITY_FLEXIBLE_TYPE_BASE_TYPES_HPP
#define TURI_UNITY_FLEXIBLE_TYPE_BASE_TYPES_HPP
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/data/flexible_type/ndarray.hpp>
#include <core/data/image/image_type.hpp>

namespace turi {


class flexible_type;
/**
 * \defgroup group_flexible_type_types Flexible Type Content Types
 * \ingroup group_gl_flexible_type
 * \{
 */
/**
 * A 64 bit integer type that can be stored in a flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::INTEGER
 */
typedef int64_t flex_int;
/**
 * A 64 bit double type that can be stored in a flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::FLOAT.
 */
typedef double flex_float;
/**
 * A string that can be stored in a flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::STRING.
 */
typedef std::string flex_string;
/**
 * A vector<double> that can be stored in a flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::VECTOR.
 * \deprecated
 */
typedef std::vector<double> flex_vec;
/**
 * A recursive object vector<flexible_type> that can be stored in a flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::LIST.
 */
typedef std::vector<flexible_type> flex_list;
/**
 * An recursive associate dictionary object vector<std::pair<flexible_type, flexible_type> >
 * Corresponds to the type enum \ref flex_type_enum::DICT.
 */
typedef std::vector<std::pair<flexible_type, flexible_type>> flex_dict;
/**
 * An \ref image_type "image type" that can be stored in a flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::IMAGE.
 */
typedef image_type flex_image;

/**
 * A ND-array double type that can be stored in a flexible_type.
 * This is the successor to flex_vec.  */
typedef flexible_type_impl::ndarray<double> flex_nd_vec;
/**
 * A date_time object that can be stored in a flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::DATETIME
 * flex_date_time.first is utc timestamp and flex_date_time.second is the
 * timezone offset.
 *
 * \note Some of the integer oddities done in this struct is to force it to
 * have size 12 bytes so it fits inside a flexible_type
 */
struct flex_date_time {
  /// Number of microseconds in 1 second
  static constexpr int64_t MICROSECONDS_PER_SECOND = 1000000;
  /** When doing time comparisons against floating point values, this is the
   * epsilon to use. This is 0.5 microseconds in seconds
   */
  static constexpr double MICROSECOND_EPSILON = 1.0 / (2 * MICROSECONDS_PER_SECOND);
  /// lowest value for timezone. 12 hours * 4 (15 min resolution)
  static constexpr int32_t TIMEZONE_LOW = -12 * 4;
  /// highest value for timezone. 12 hours * 4 (15 min resolution)
  static constexpr int32_t TIMEZONE_HIGH = 12 * 4;
  /// Value denoting an invalid timezone
  static constexpr int32_t EMPTY_TIMEZONE = 64;
  // EMPTY_TIMEZONE value is arbitrary, but not close to TIMEZONE_LOW or
  // TIMEZONE_HIGH so that off by one errors does not become the empty timezone
  /// timezone resolution is stored in 15 minute intervals (15*60 seconds)
  static constexpr int32_t TIMEZONE_RESOLUTION_IN_SECONDS = 15 * 60;
  /// timezone resolution is stored in 15 minute intervals (15 minutes)
  static constexpr int32_t TIMEZONE_RESOLUTION_IN_MINUTES = 15;
  /// timezone resolution is stored in 15 minute intervals (1/4 hour)
  static constexpr double TIMEZONE_RESOLUTION_IN_HOURS = 0.25;
  static constexpr int32_t _LEGACY_TIMEZONE_SHIFT = 25;

  inline flex_date_time(): m_posix_timestamp_low(0), m_posix_timestamp_high(0),
    m_tz_15min_offset(EMPTY_TIMEZONE + _LEGACY_TIMEZONE_SHIFT), m_microsecond(0) { }
  flex_date_time(const flex_date_time&) = default;
  flex_date_time(flex_date_time&&) = default;
  flex_date_time& operator=(const flex_date_time&) = default;
  flex_date_time& operator=(flex_date_time&&) = default;
  /**
   * Constructs a flex_date_time object from a posix timestamp, and and time zone offset.
   * \param posix_timestamp Timestamp value since 1st Jan 1970 UTC
   * \param tz_15_min_offset Additional offset for timezone. In integral
   * increments of 15 minutes. If EMPTY_TIMEZONE, or not provided, there is no
   * timezone information
   * \param microsecond Microsecond value
   */
  explicit inline flex_date_time(int64_t posix_timestamp,
                                 int32_t tz_15min_offset = EMPTY_TIMEZONE,
                                 int32_t microsecond = 0) {
    set_posix_timestamp(posix_timestamp);
    set_time_zone_offset(tz_15min_offset);
    set_microsecond(microsecond);
  }

  /**
   * Returns the timestamp value at UTC
   */
  inline int64_t posix_timestamp() const {
    // the << 8 >> 8 looks odd, but it is basically to allow for sign extension.
    // We are truncating a 64 bit signed value to 56 bits manually
    uint64_t unsigned_ts = ((uint64_t(m_posix_timestamp_high) << 32) +
                             uint64_t(m_posix_timestamp_low));
    int64_t signed_ts = unsigned_ts;
    signed_ts = (signed_ts << 8) >> 8;
    return signed_ts;
  }

  /**
   * Returns the timestamp value adding the timezone shift
   */
  inline int64_t shifted_posix_timestamp() const {
    if (time_zone_offset() == EMPTY_TIMEZONE) return posix_timestamp();
    else return posix_timestamp() + time_zone_offset() * TIMEZONE_RESOLUTION_IN_SECONDS;
  }

  /**
   * Sets the posix timestamp value
   */
  inline void set_posix_timestamp(int64_t ts) {
    uint64_t uposix_timestamp = ts;
    m_posix_timestamp_high = uposix_timestamp >> 32;
    m_posix_timestamp_low = uposix_timestamp;
  }

  /**
   * Returns the timestamp in a floating point value including the microsecond
   * interval.
   * \note This function is not generally safe since there are only
   * 52 mantissa bits, We may lose precision for extremely large or
   * extremely small timestamp values
   */
  inline double microsecond_res_timestamp() const {
    return double(posix_timestamp()) + double(microsecond()) / MICROSECONDS_PER_SECOND;
  }


  /**
   * Sets the timestamp from a floating point value including the microsecond
   * interval.
   * \note This function is not generally safe since there are only
   * 52 mantissa bits, We may lose precision for extremely large or
   * extremely small timestamp values
   */
  inline void set_microsecond_res_timestamp(double d) {
    int64_t integral_part = std::floor(d);
    set_posix_timestamp(integral_part);
    int32_t us = ((d - integral_part) * MICROSECONDS_PER_SECOND);
    // avoid some rounding oddities where if the floating point part
    // is sufficiently large, we may get MICROSECONDS_PER_SECOND
    us = std::min<int32_t>(us, MICROSECONDS_PER_SECOND - 1);
    set_microsecond(us);
  }
  /**
   * Returns the timestamp value.
   */
  inline int32_t microsecond() const {
    return m_microsecond;
  }

  /**
   * Returns the timestamp value.
   */
  inline void set_microsecond(int32_t microsecond) {
    if (microsecond < 0 || microsecond >= MICROSECONDS_PER_SECOND) {
      log_and_throw("Invalid microsecond value");
    }
    m_microsecond = microsecond;
  }

  /**
   * Returns the time zone value in integral increments of 15 min
   */
  inline int32_t time_zone_offset() const {
    // see load(iarchive) for explanations of this oddity.
    if (m_tz_15min_offset < 0) {
      return m_tz_15min_offset + _LEGACY_TIMEZONE_SHIFT;
    } else {
      return m_tz_15min_offset - _LEGACY_TIMEZONE_SHIFT;
    }
  }

  /**
   * Sets the time zone value in integral increments of 15 min
   */
  inline void set_time_zone_offset(int32_t tz_15min_offset) {
    if ((tz_15min_offset < TIMEZONE_LOW || tz_15min_offset > TIMEZONE_HIGH) &&
        tz_15min_offset != EMPTY_TIMEZONE) {
      log_and_throw("Invalid timezone value");
    }
    // see load(iarchive) for explanations of this oddity.
    if (tz_15min_offset < 0) {
      m_tz_15min_offset = tz_15min_offset - _LEGACY_TIMEZONE_SHIFT;
    } else {
      m_tz_15min_offset = tz_15min_offset + _LEGACY_TIMEZONE_SHIFT;
    }
  }

  /// Less than comparator
  inline bool operator<(const flex_date_time& other) const{
    return posix_timestamp() < other.posix_timestamp() ||
        (posix_timestamp() == other.posix_timestamp() &&
         microsecond() < other.microsecond());
  }

  /// Greater than comparator
  inline bool operator>(const flex_date_time& other) const{
    return posix_timestamp() > other.posix_timestamp() ||
        (posix_timestamp() == other.posix_timestamp() &&
         microsecond() > other.microsecond());
  }

  /// Equality comparator, timezone is ignored.
  inline bool operator==(const flex_date_time& other) const{
    return
        posix_timestamp() == other.posix_timestamp() &&
        microsecond() == other.microsecond();
  }


  /// Equality comparator, timezone is checked
  inline bool identical(const flex_date_time& other) const{
    return posix_timestamp() == other.posix_timestamp() &&
        time_zone_offset() == other.time_zone_offset() &&
        microsecond() == other.microsecond();
  }

  inline void save(oarchive& oarc) const {
    oarc.write(reinterpret_cast<const char*>(this), sizeof(flex_date_time));
  }

  inline void load(iarchive& iarc) {
    /*
     * The old datetime representation is simply 56 bits for the timestamp
     * and 8 bits for the timezone at 1/2 hour increments, and no microsecond.
     * for a total of 64 bits.
     *
     * To ensure backward compatibility, we have to some tricks.
     *  - We shift the timezone value to EXCLUDE the timezone range the old
     *  format allowed.  (this will be [-24, 24] since at 1/2 hour offsets,
     *  24 is the maximum value possible).
     *
     *  This allows us to simply by inspection of the
     *  timezone value, determine if it was an old format or a new format
     *  timezone.
     *
     * - So to deserialize, we first deserialize 8 bytes, check the timezone
     *   value, to determine if it is old format or new format. If it is new
     *   format, deserialize a further 4 bytes.
     *
     *   If it is old format, scale up the timezone value by 2 (to get it to
     *   15 minute increments, and clear the microsecond value.
     */
    iarc.read(reinterpret_cast<char*>(this), 8);
    if (m_tz_15min_offset > - _LEGACY_TIMEZONE_SHIFT &&
        m_tz_15min_offset < _LEGACY_TIMEZONE_SHIFT
        ) {
      // old format
      set_time_zone_offset(m_tz_15min_offset * 2);
      m_microsecond = 0;
    } else {
      // new format
      iarc.read(reinterpret_cast<char*>(&m_microsecond), sizeof(m_microsecond));
    }
  }

  friend class flexible_type;

 private:
  uint32_t m_posix_timestamp_low: 32;
  uint32_t m_posix_timestamp_high: 24;
  int32_t m_tz_15min_offset: 8;
  int32_t m_microsecond;
};

/**
 * Represents the undefined type in the flexible_type.
 * Corresponds to the type enum \ref flex_type_enum::UNDEFINED.
 *
 * The global static variable \ref FLEX_UNDEFINED can be used when an undefined
 * value is needed. i.e.
 * \code
 * flexible_type f = FLEX_UNDEFINED;
 * \endcode
 * This is equivalent to:
 * \code
 * flexible_type f = flex_undefined();
 * \endcode
 */
struct flex_undefined {
  inline void save(oarchive& oarc) const { }
  inline void load(iarchive& iarc) { }
};

/// \}

/**
 * \ingroup group_gl_flexible_type
 * An enumeration over the possible types the flexible type can contain.
 *
 * \note Renumber this at your own risk.
 *
 * \see flex_type_enum_from_name
 * \see flex_type_enum_to_name
 */
enum class flex_type_enum: char {
  INTEGER = 0,  /**< Integer type. Stored type is the \ref flex_int */
  FLOAT = 1,    /**< 64-bit floating point type. Stored type is the \ref flex_float */
  STRING = 2,   /**< String type. Stored type is the \ref flex_string */
  VECTOR = 3,   /**< Numeric vector type. Stored type is the \ref flex_vec. This is deprecated. */
  LIST = 4,     /**< Recursive List type. Stored type is the \ref flex_list */
  DICT = 5,     /**< Recursive Dictionary type. Stored type is the \ref flex_dict */
  DATETIME = 6, /**< Date-Time type. Stored type is the \ref flex_date_time */
  UNDEFINED = 7,/**< Undefined / Missing Value type. Stored type is the \ref flex_undefined */
  IMAGE = 8,    /**< Image type. Stored type is the \ref flex_image */
  ND_VECTOR = 9,   /**< Numeric vector type. Stored type is the \ref flex_nd_vec */
  // types >= 128 are reserved.
  // (Or rather more accurately, they should not be used without a lot of
  //  thought and care.)
  // See flexible_type::save / load for the reason why.
};

/**
 * \ingroup group_gl_flexible_type
 * Check if one flexible type is convertable to the other.
 */
inline bool flex_type_is_convertible(flex_type_enum from, flex_type_enum to) {
  static constexpr bool castable[10][10] =
        // int flt str vec rec dic dtime undef img ndvec
/*int*/  {{1,  1,  1,  0,  0,  0,  1,  0,  0, 0},  // integer casts to self, float and string
/*flt*/   {1,  1,  1,  0,  0,  0,  1,  0,  0, 0},  // float casts to integer, self, string
/*str*/   {0,  0,  1,  0,  0,  0,  0,  0,  0, 0},  // string casts to string only
/*vec*/   {0,  0,  1,  1,  1,  0,  0,  0,  0, 1},  // vector casts to string and self and recursive, ndvec
/*rec*/   {0,  0,  1,  0,  1,  0,  0,  0,  0, 0},  // recursive casts to string vec, rec, ndvec, and self.
/*dic*/   {0,  0,  1,  0,  0,  1,  0,  0,  0, 0},  // dict casts to self
/*dtime*/ {1,  1,  1,  0,  0,  0,  1,  0,  0, 0},  // dtime casts to string and self
/*undef*/ {0,  0,  1,  0,  0,  0,  0,  1,  0, 0},  //UNDEFINED casts to string and UNDEFINED
/*img*/   {0,  0,  1,  1,  0,  0,  0,  0,  1, 1},  // img casts to string, ndvec, vec, and self.
/*ndvec*/ {0,  0,  1,  1,  0,  0,  0,  0,  1, 1}}; // ndvec casts to str and flattens to vec, img, self
  return castable[static_cast<int>(from)][static_cast<int>(to)];
}


/**
 * \ingroup group_gl_flexible_type
 * Check if numeric operators between these types are supported.
 * if a and b are flexible_types and
 * flex_type_has_binary_op(a.get_type(), b.get_type(), op) is true, this means.
 * that a op= b and a op b is false. op is any of '+','-','*','/','%','<','>','='
 *
 * '<' is the less than operator, and '>' is the greater than operator.
 * '=' is the equality comparison operator.
 * Suceeding on '<' or '>' implies that the less than or equal operator (<=)
 * and the greater than or equal operator (>=) will also succeed
 *
 * For instance given flexible_type a, b
 * \code
 * if (flex_type_has_binary_op(a, b, '+')) {
 *   a += b // will succeed
 *   a + b  // will succeed
 * } else {
 *   a += b // will fail
 *   a + b  // will fail
 * }
 * \endcode
 */
inline bool flex_type_has_binary_op(flex_type_enum left,
                                    flex_type_enum right,
                                    char op) {
  static constexpr bool plus_operator[10][10] =     // '+' operator
        // int flt str vec rec cel dtime undef img ndvec
/*int*/  {{1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*flt*/   {1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*str*/   {0,  0,  1,  0,  0,  0,  0,  0,  0,  0},
/*vec*/   {1,  1,  0,  1,  0,  0,  0,  0,  0,  0},
/*rec*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*cel*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*dtime*/ {1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*undef*/ {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*img*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*ndvec*/ {1,  1,  0,  0,  0,  0,  0,  0,  0,  1}};

  static constexpr bool minus_operator[10][10] =  // '-' operator
        // int flt str vec rec cel dtime undef img ndvec
/*int*/  {{1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*flt*/   {1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*str*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*vec*/   {1,  1,  0,  1,  0,  0,  0,  0,  0,  0},
/*rec*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*cel*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*dtime*/ {1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*undef*/ {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*img*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*ndvec*/ {1,  1,  0,  0,  0,  0,  0,  0,  0,  1}};

  static constexpr bool other_numeric_operators[10][10] =  // '*','/','%' operator
        // int flt str vec rec cel dtime undef img ndvec
/*int*/  {{1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*flt*/   {1,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/*str*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*vec*/   {1,  1,  0,  1,  0,  0,  0,  0,  0,  0},
/*rec*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*cel*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*dtime*/ {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*undef*/ {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*img*/   {0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/*ndvec*/ {1,  1,  0,  0,  0,  0,  0,  0,  0,  1}};

  static constexpr bool comparison_operators[10][10] =  // < or > operator
        // int flt str vec rec cel dtime undef img ndvec
/*int*/  {{1,  1,  0,  0,  0,  0,  1,  0,  0},
/*flt*/   {1,  1,  0,  0,  0,  0,  1,  0,  0},
/*str*/   {0,  0,  1,  0,  0,  0,  0,  0,  0},
/*vec*/   {0,  0,  0,  0,  0,  0,  0,  0,  0},
/*rec*/   {0,  0,  0,  0,  0,  0,  0,  0,  0},
/*cel*/   {0,  0,  0,  0,  0,  0,  0,  0,  0},
/*dtime*/ {1,  1,  0,  0,  0,  0,  1,  0,  0},
/*undef*/ {0,  0,  0,  0,  0,  0,  0,  0,  0},
/*img*/   {0,  0,  0,  0,  0,  0,  0,  0,  0},
/*ndvec*/ {0,  0,  0,  0,  0,  0,  0,  0,  0}};

  static constexpr bool equality_operators[10][10] =  // = operator
        // int flt str vec rec cel dtime undef img ndvec
/*int*/  {{1,  1,  0,  0,  0,  0,  1,  1,  0,  0},
/*flt*/   {1,  1,  0,  0,  0,  0,  1,  1,  0,  0},
/*str*/   {0,  0,  1,  0,  0,  0,  0,  1,  0,  0},
/*vec*/   {0,  0,  0,  1,  0,  0,  0,  1,  0,  0},
/*rec*/   {0,  0,  0,  0,  0,  0,  0,  1,  0,  0},
/*cel*/   {0,  0,  0,  0,  0,  0,  0,  1,  0,  0},
/*dtime*/ {1,  1,  0,  0,  0,  0,  1,  1,  0,  0},
/*undef*/ {1,  1,  1,  1,  1,  1,  1,  1,  1,  0},
/*img*/   {0,  0,  0,  0,  0,  0,  0,  1,  0,  0},
/*ndvec*/ {0,  0,  0,  0,  0,  0,  0,  0,  0,  1}};
  switch(op) {
   case '+':
    return plus_operator[static_cast<int>(left)][static_cast<int>(right)];
   case '-':
    return minus_operator[static_cast<int>(left)][static_cast<int>(right)];
   case '*':
   case '/':
   case '%':
    return other_numeric_operators[static_cast<int>(left)][static_cast<int>(right)];
   case '<':
   case '>':
    return comparison_operators[static_cast<int>(left)][static_cast<int>(right)];
   case '=':
    return equality_operators[static_cast<int>(left)][static_cast<int>(right)];
   default:
    return false;
  }
}


/**
 * \ingroup group_gl_flexible_type
 * Identifies the flex_type_enum representing the type,
 * All other non-matching types will return UNDEFINED.
 *
 * type_to_enum<T>::value is the corresponding enumeration value in \ref
 * flex_type_enum if T matches \b exactly one of the flexible_type content
 * types. type_to_enum<T>::value is flex_type_enum::UNDEFINED otherwise.
 *
 * This is very useful for SFINAE operations.
 *
 * Example:
 * The following lines will both return flex_type_enum::INTEGER
 * \code
 * (flex_type_enum) type_to_enum<flex_int>
 * type_to_enum<flex_int>::value
 * \endcode
 */
template <typename T>
struct type_to_enum {
  static constexpr flex_type_enum value = flex_type_enum::UNDEFINED;
};

template <>
struct type_to_enum<flex_int> {
  static constexpr flex_type_enum value = flex_type_enum::INTEGER;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_date_time> {
  static constexpr flex_type_enum value = flex_type_enum::DATETIME;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_float> {
  static constexpr flex_type_enum value = flex_type_enum::FLOAT;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_string> {
  static constexpr flex_type_enum value = flex_type_enum::STRING;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_vec> {
  static constexpr flex_type_enum value = flex_type_enum::VECTOR;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_nd_vec> {
  static constexpr flex_type_enum value = flex_type_enum::ND_VECTOR;
  constexpr operator flex_type_enum() const { return value; }
};


template <>
struct type_to_enum<flex_list> {
  static constexpr flex_type_enum value = flex_type_enum::LIST;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_dict> {
  static constexpr flex_type_enum value = flex_type_enum::DICT;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_image> {
  static constexpr flex_type_enum value = flex_type_enum::IMAGE;
  constexpr operator flex_type_enum() const { return value; }
};

template <>
struct type_to_enum<flex_undefined> {
  static constexpr flex_type_enum value = flex_type_enum::UNDEFINED;
  constexpr operator flex_type_enum() const { return value; }
};


/**
 * \ingroup group_gl_flexible_type
 *
 * is_valid_flex_type<T>::value is true if T is type which matches \b exactly
 * one of the flexible_type content types. false otherwise.
 *
 * This is very useful for SFINAE operations.
 *
 * Example:
 * The following lines will both return true
 * \code
 * (bool)type_to_enum<flex_int>
 * type_to_enum<flex_int>::value
 * \endcode
 *
 */
template <typename T>
struct is_valid_flex_type{
  static constexpr bool value = false;
  constexpr operator bool() const { return value; }
};

template <>
struct is_valid_flex_type<flex_int> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};


template <>
struct is_valid_flex_type<flex_date_time> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};

template <>
struct is_valid_flex_type<flex_float> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};

template <>
struct is_valid_flex_type<flex_string> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};

template <>
struct is_valid_flex_type<flex_vec> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};

template <>
struct is_valid_flex_type<flex_nd_vec> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};

template <>
struct is_valid_flex_type<flex_dict> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};


template <>
struct is_valid_flex_type<flex_list> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};

template <>
struct is_valid_flex_type<flex_image> {
  static constexpr bool value = true;
  constexpr operator bool() const { return value; }
};


/**
 * \ingroup group_gl_flexible_type
 *
 * enum_to_type<flex_type_enum e>::type is the flexible_type content type
 * corresponding to the enumeration value. This is very useful for SFINAE
 * operations.
 *
 * Example:
 * This will refer to the type flex_int
 * \code
 * type_to_enum<flex_type_enum::INTEGER>::type
 * \endcode
 */
template <flex_type_enum>
struct enum_to_type{
  typedef void type;
};

template <>
struct enum_to_type<flex_type_enum::INTEGER>{
  typedef flex_int type;
};


template <>
struct enum_to_type<flex_type_enum::DATETIME>{
  typedef flex_date_time type;
};

template <>
struct enum_to_type<flex_type_enum::FLOAT>{
  typedef flex_float type;
};

template <>
struct enum_to_type<flex_type_enum::STRING>{
  typedef flex_string type;
};

template <>
struct enum_to_type<flex_type_enum::VECTOR>{
  typedef flex_vec type;
};

template <>
struct enum_to_type<flex_type_enum::ND_VECTOR>{
  typedef flex_nd_vec type;
};

template <>
struct enum_to_type<flex_type_enum::LIST>{
  typedef flex_list type;
};

template <>
struct enum_to_type<flex_type_enum::DICT>{
  typedef flex_dict type;
};

template <>
struct enum_to_type<flex_type_enum::IMAGE>{
  typedef flex_image type;
};


template <>
struct enum_to_type<flex_type_enum::UNDEFINED>{
  typedef flex_undefined type;
};

/**
 * \ingroup group_gl_flexible_type
 * Returns a printable name of a flex_type_enum.
 * Reverse of \ref flex_type_enum_from_name
 */
inline const char* flex_type_enum_to_name(flex_type_enum en) {
  switch(en) {
   case flex_type_enum::INTEGER:
     return "integer";
   case flex_type_enum::DATETIME:
     return "datetime";
   case flex_type_enum::FLOAT:
     return "float";
   case flex_type_enum::STRING:
     return "string";
   case flex_type_enum::VECTOR:
     return "array";
   case flex_type_enum::ND_VECTOR:
     return "ndarray";
   case flex_type_enum::LIST:
     return "list";
   case flex_type_enum::DICT:
     return "dictionary";
   case flex_type_enum::IMAGE:
     return "image";
   default:
     return "undefined";
  }
}

/**
 * \ingroup group_gl_flexible_type
 * Given the printable name of a flexible_type type, returns its corresponding
 * \ref flex_type_enum enumeration value.
 * Reverse of \ref flex_type_enum_to_name.
 */
inline flex_type_enum flex_type_enum_from_name(const std::string& name) {
  static std::map<std::string, flex_type_enum> type_map{
    {"integer", flex_type_enum::INTEGER},
    {"datetime", flex_type_enum::DATETIME},
    {"float", flex_type_enum::FLOAT},
    {"string", flex_type_enum::STRING},
    {"array", flex_type_enum::VECTOR},
    {"ndarray", flex_type_enum::ND_VECTOR},
    {"list", flex_type_enum::LIST},
    {"dictionary", flex_type_enum::DICT},
    {"image", flex_type_enum::IMAGE},
    {"undefined", flex_type_enum::UNDEFINED}
  };

  if (type_map.count(name) == 0) {
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR {
      std::ostringstream ss;

      ss << "Invalid flexible type name '" << name << "'. "
         << "Valid names are: ";

      for(const auto& p : type_map) {
         ss << p.first << ",";
  }

      log_and_throw(ss.str().c_str());
    };
    throw_error();
  }

  return type_map.at(name);
}


/**
 * \ingroup group_gl_flexible_type
 *
 * has_direct_conversion_to_flexible_type<T>::value is is true if:
 * \code
 * flexible_type f = T();
 * \endcode
 * will succeed. This is very useful for SFINAE operations.
 */
template <typename T>
struct has_direct_conversion_to_flexible_type {
  static constexpr bool value =
      !std::is_same<T, flexible_type>::value && (
      std::is_integral<T>::value ||
      std::is_floating_point<T>::value ||
      std::is_convertible<T, flex_string>::value ||
      std::is_convertible<T, flex_list>::value ||
      std::is_convertible<T, flex_vec>::value ||
      std::is_convertible<T, flex_nd_vec>::value ||
      std::is_convertible<T, flex_dict>::value ||
      std::is_convertible<T, flex_date_time>::value ||
      std::is_convertible<T, flex_image>::value ||
      std::is_same<T, flex_undefined>::value);

  static constexpr flex_type_enum desired_type =
      std::is_integral<T>::value ? flex_type_enum::INTEGER :
      std::is_floating_point<T>::value ? flex_type_enum::FLOAT :
      std::is_convertible<T, flex_string>::value ? flex_type_enum::STRING :
      std::is_convertible<T, flex_list>::value ? flex_type_enum::LIST :
      std::is_convertible<T, flex_vec>::value ? flex_type_enum::VECTOR :
      std::is_convertible<T, flex_nd_vec>::value ? flex_type_enum::ND_VECTOR :
      std::is_convertible<T, flex_dict>::value ? flex_type_enum::DICT :
      std::is_convertible<T, flex_date_time>::value ? flex_type_enum::DATETIME :
      std::is_convertible<T, flex_image>::value ? flex_type_enum::IMAGE :
      flex_type_enum::UNDEFINED;
};

/**
 * Given a set of types, choose a common type that all types in the set can be
 * converted to and preserves the most data. Not designed to be passed a set
 * with UNDEFINED in it.
 */
inline flex_type_enum get_common_type(const std::set<flex_type_enum> &types) {
  if (types.size() == 0) return flex_type_enum::FLOAT;
  else if (types.size() == 1) return *(types.begin());
  else if (types.size() == 2) {
    if (types.count(flex_type_enum::INTEGER) && types.count(flex_type_enum::FLOAT)) {
      return flex_type_enum::FLOAT;
    }
    if (types.count(flex_type_enum::LIST) && types.count(flex_type_enum::VECTOR)) {
      return flex_type_enum::LIST;
    }
  } else {
    throw std::string("Could not find a common type to convert all values.");
  }

  return flex_type_enum::UNDEFINED;
}

// Allow printing of the flex_type_enum
static inline std::ostream& operator<<(std::ostream& out, const flex_type_enum e) {
  out << (int)e;
  return out;
}

} // namespace turi

#endif
