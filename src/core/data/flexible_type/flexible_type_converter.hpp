/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FLEXIBLE_TYPE_FLEXIBLE_TYPE_CONVERTER_HPP
#define TURI_FLEXIBLE_TYPE_FLEXIBLE_TYPE_CONVERTER_HPP
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>
#include <type_traits>
#include <core/util/code_optimization.hpp>
#include <core/data/flexible_type/type_traits.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/flexible_type/flexible_type_conversion_utilities.hpp>

namespace turi {

/** The primary functions/structs available for doing the flexible
 *  type conversion.
 *
 */
template <typename T> struct is_flexible_type_convertible;
template <typename T> static void convert_from_flexible_type(T& t, const flexible_type& f);
template <typename T> static void convert_to_flexible_type(flexible_type& f, T&& t);
template <typename T> static flexible_type convert_to_flexible_type(T&& t);

namespace flexible_type_internals {

////////////////////////////////////////////////////////////////////////////////

/** The conversion path is chosen based on a list of possible
 *  conversions ranked by priority, with the lowest numbered converter
 *  that matches the particular type chosen as the conversion path.
 *  The constants below declare the particular converters available,
 *  and the actual definitions -- defined as specialized structs --
 *  are given after that.
 *
 *  The conversion mechanism works by testing the matches() constexpr
 *  condition on each numbered struct, then choosing the struct number
 *  that matches with no lower numbers matching.  A static_assert
 *  failure occurs if there are no matches.
 */

// The obvious case of another flexible type.
static constexpr int CVTR__FLEXIBLE_TYPE_EXACT            = 1;

// Numeric types.
static constexpr int CVTR__FLOATING_POINT                 = 2;
static constexpr int CVTR__INTEGER                        = 3;

// Strings.
static constexpr int CVTR__FLEX_STRING_CONVERTIBLE        = 4;


// Vectors -- sequences of numeric elements that can be exactly
// converted into a flex_vec type.  Prioritized above lists.
static constexpr int CVTR__FLEX_VEC_CONVERTIBLE_SEQUENCE  = 5;
static constexpr int CVTR__FLEX_VEC_CONVERTIBLE_PAIR      = 6;
static constexpr int CVTR__FLEX_VEC_CONVERTIBLE_TUPLE     = 7;

// Dictionaries -- sequences convertible to an exact match.
static constexpr int CVTR__FLEX_DICT_CONVERTIBLE_SEQUENCE = 8;
static constexpr int CVTR__FLEX_DICT_CONVERTIBLE_MAPS     = 9;

// Lists -- any other sequences that cannot be converted into a
// dictionary or vector but can be converted into a list of flexible
// type.
static constexpr int CVTR__FLEX_LIST_CONVERTIBLE_PAIR     = 10;
static constexpr int CVTR__FLEX_LIST_CONVERTIBLE_TUPLE    = 11;
static constexpr int CVTR__FLEX_LIST_CONVERTIBLE_SEQUENCE = 12;

// Enum types.
static constexpr int CVTR__ENUM                           = 13;

// The last value -- this tells the resolver how many options there
// are out there.
static constexpr int NUM_CONVERTER_STRUCTS                = 14;

/**
 * Type conversion priority is done according to the lowest numbered
 * ft_converter for which matches<T>() returns true.  If no converter
 * correctly converts things, then a static assert is raised if
 * convert is attempted.
 *
 * If matches() is not true, then the get and set methods are never
 * instantiated.
 */
template <int> struct ft_converter {
  template <typename T> static constexpr bool matches() { return false; }

  template <typename T> static inline void get(T& t, const flexible_type& v) {
    static_assert(swallow_to_false<T>::value, "get with bad match.");
  }

  template <typename T>
  static inline void set(flexible_type& t, const T& v) {
    static_assert(swallow_to_false<T>::value, "set with bad match.");
  }
};

////////////////////////////////////////////////////////////////////////////////
//
//  flexible_type
//
////////////////////////////////////////////////////////////////////////////////

/** straight flexible_type.  Always given priority.
 */
template <> struct ft_converter<CVTR__FLEXIBLE_TYPE_EXACT> {

  template <typename T> static constexpr bool matches() {
    return (std::is_same<T, flexible_type>::value
        || std::is_same<T, flex_undefined>::value
        || std::is_same<T, flex_int>::value
        || std::is_same<T, flex_float>::value
        || std::is_same<T, flex_string>::value
        || std::is_same<T, flex_vec>::value
        || std::is_same<T, flex_list>::value
        || std::is_same<T, flex_dict>::value
        || std::is_same<T, flex_image>::value
        || std::is_same<T, flex_date_time>::value);

  }

  static void get(flexible_type& dest, const flexible_type& src) {
    dest = src;
  }

  static void get(flex_float& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::FLOAT) {
      dest = src.get<flex_float>();
    } else if(src.get_type() == flex_type_enum::INTEGER) {
      dest = src.to<flex_int>();
    } else {
      throw_type_conversion_error(src, "numeric value");
      ASSERT_UNREACHABLE();
    }
  }

  static void get(flex_int& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::INTEGER) {
      dest = src.get<flex_int>();
    } else if(src.get_type() == flex_type_enum::FLOAT) {
      flex_float v = src.get<flex_float>();
      dest = static_cast<flex_int>(v);
      if(UNLIKELY(v != dest)) {
        throw_type_conversion_error(src, "integer value");
        ASSERT_UNREACHABLE();
      }
    } else {
      throw_type_conversion_error(src, "numeric integer value");
      ASSERT_UNREACHABLE();
    }
  }

  static void get(flex_vec& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::VECTOR) {
      dest = src.get<flex_vec>();
    } else if(src.get_type() == flex_type_enum::LIST) {
      dest = src.to<flex_vec>();
    } else {
      throw_type_conversion_error(src, "vector of floats");
      ASSERT_UNREACHABLE();
    }
  }

  static void get(flex_list& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::LIST) {
      dest = src.get<flex_list>();
    } else if(src.get_type() == flex_type_enum::VECTOR) {
      dest = src.to<flex_list>();
    } else {
      throw_type_conversion_error(src, "vector of floats");
      ASSERT_UNREACHABLE();
    }
  }

  // Finally, a generic, strict method that fails if type is not matched exactly.
  template <typename T>
  static void get(T& dest, const flexible_type& src) {
    if (type_to_enum<T>::value != src.get_type()) {
      std::string errormsg = std::string("Expecting ")
          + flex_type_enum_to_name(type_to_enum<T>::value)
          + ". But we got a " + flex_type_enum_to_name(src.get_type());
      throw(errormsg);
    }
    dest = src.get<T>();
  }


  // This is the only case where setting from a move expression makes sense
  template <typename T>
  static void set(flexible_type& dest, T&& src) {
    dest = std::forward<T>(src);
  }
};

////////////////////////////////////////////////////////////////////////////////
//
//  numeric types.
//
////////////////////////////////////////////////////////////////////////////////

/** Any floating point values.
 */
template <> struct ft_converter<CVTR__FLOATING_POINT> {

  template <typename T> static constexpr bool matches() {
    return std::is_floating_point<T>::value;
  }

  template <typename Float>
  static void get(Float& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::FLOAT) {
      dest = static_cast<Float>(src.get<flex_float>());
    } else if(src.get_type() == flex_type_enum::INTEGER) {
      dest = static_cast<Float>(src.get<flex_int>());
    } else {
      throw_type_conversion_error(src, "numeric");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename Float>
  static void set(Float& dest, const flexible_type& src) {
    dest = flex_float(src);
  }
};

////////////////////////////////////////////////////////////////////////////////

/** Integer values.
 */
template <> struct ft_converter<CVTR__INTEGER> {

  template <typename T> static constexpr bool matches() {
    return std::is_integral<T>::value || std::is_same<T, bool>::value;
  }

  template <typename Integer>
  static void get(Integer& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::FLOAT) {
      flex_float v = src.get<flex_float>();
      if(static_cast<Integer>(v) != v) {
        throw_type_conversion_error(src, "integer / convertable float");
        ASSERT_UNREACHABLE();
      }
      dest = static_cast<Integer>(v);
    } else if(src.get_type() == flex_type_enum::INTEGER) {
      dest = static_cast<Integer>(src.get<flex_int>());
    } else {
      throw_type_conversion_error(src, "integer");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename Integer>
  static void set(Integer& dest, const flexible_type& src) {
    dest = Integer(flex_int(src));
  }
};

////////////////////////////////////////////////////////////////////////////////
//
//  Strings
//
////////////////////////////////////////////////////////////////////////////////

/** Any string types not exactly flex_string.
 */
template <> struct ft_converter<CVTR__FLEX_STRING_CONVERTIBLE> {

  template <typename T> static constexpr bool matches() {
    return is_string<T>::value || std::is_same<T, const char*>::value;
  }

  template <typename String>
  static void get(String& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::STRING) {
      const flex_string& s = src.get<flex_string>();
      dest.assign(s.begin(), s.end());
    } else {
      flex_string s = src.to<flex_string>();
      dest.assign(s.begin(), s.end());
    }
  }

  static void get(const char*& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::STRING) {
      dest = src.get<flex_string>().c_str();
    } else {
      throw_type_conversion_error(src, "const char* (reference to temporary).");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename String>
  static void set(flexible_type& dest, const String& src) {
    dest = flex_string(src.begin(), src.end());
  }

  static void set(flexible_type& dest, const char* src) {
    dest = flex_string(src);
  }
};

////////////////////////////////////////////////////////////////////////////////
//
//  Vectors
//
////////////////////////////////////////////////////////////////////////////////

/** Any sequence containers (vectors, lists, deques, etc.) of floating
 *  point values.
 */
template <> struct ft_converter<CVTR__FLEX_VEC_CONVERTIBLE_SEQUENCE> {

  template <typename V> static constexpr bool matches() {
    typedef typename first_nested_type<V>::type  U;

    return (is_sequence_container<V>::value
            && (std::is_floating_point<U>::value || is_integer_in_4bytes<U>::value));
  }

  template <typename Vector>
  static void get(Vector& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::VECTOR) {
      const flex_vec& v = src.get<flex_vec>();
      dest.assign(v.begin(), v.end());
    } else if(src.get_type() == flex_type_enum::LIST) {
      const flex_list& f = src.get<flex_list>();
      dest.assign(f.begin(), f.end());
    } else {
      throw_type_conversion_error(src, "flex_vec");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename Vector>
  static void set(flexible_type& dest, const Vector& src) {
    dest = flex_vec(src.begin(), src.end());
  }
};


////////////////////////////////////////////////////////////////////////////////

/** Any pairs of numeric values. This is different from the general
 *  pair of flex_type_converible values since this allows the the
 *  std::pair to convert to and from a 2-element flex_vec if the types
 *  in the std::pair are numeric.  Internally, the logic is the same
 *  as the case below if they are not strictly doubles.
 */
template <> struct ft_converter<CVTR__FLEX_VEC_CONVERTIBLE_PAIR> {

  template <typename T> static constexpr bool matches() {
    typedef typename first_nested_type<T>::type  U1;
    typedef typename second_nested_type<T>::type U2;

    return (is_std_pair<T>::value
            && std::is_floating_point<U1>::value
            && std::is_floating_point<U2>::value);
  }

  template <typename T, typename U>
  static void get(std::pair<T, U>& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::LIST) {
      const flex_list& l = src.get<flex_list>();
      if(l.size() != 2) {
        throw_type_conversion_error(src, "2-element flex_list/flex_vec (list size != 2)");
        ASSERT_UNREACHABLE();
      }
      convert_from_flexible_type(dest.first, l[0]);
      convert_from_flexible_type(dest.second, l[1]);
    } else if(src.get_type() == flex_type_enum::VECTOR) {
      const flex_vec& v = src.get<flex_vec>();
      if(v.size() != 2){
        throw_type_conversion_error(src, "2-element flex_list/flex_vec (vector size != 2)");
        ASSERT_UNREACHABLE();
      }
      dest.first = static_cast<T>(v[0]);
      dest.second = static_cast<U>(v[1]);
    } else {
      throw_type_conversion_error(src, "2-element flex_list/flex_vec");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename T, typename U>
  static void set(flexible_type& dest, const std::pair<T,U>& src) {
    if(std::is_floating_point<T>::value && std::is_floating_point<U>::value) {
      dest = flex_vec{flex_float(src.first), flex_float(src.second)};
    } else {
      dest = flex_list{convert_to_flexible_type(src.first), convert_to_flexible_type(src.second)};
    }
  }
};

////////////////////////////////////////////////////////////////////////////////

/** std::tuple of numeric values.
 */
template <> struct ft_converter<CVTR__FLEX_VEC_CONVERTIBLE_TUPLE> {

  template <typename T> static constexpr bool matches() {
    return (is_tuple<T>::value && all_nested_true<std::is_arithmetic, T>::value);
  }

  template <typename... Args>
  static void get(std::tuple<Args...>& dest, const flexible_type& src) {
    switch(src.get_type()) {
      case flex_type_enum::LIST: {
        const flex_list& d = src.get<flex_list>();

        if (d.size() != sizeof...(Args)) {
          std::string errormsg =
              std::string("Expecting a list or vector of length ")
              + std::to_string(sizeof...(Args)) + ", but we got a list of length "
              + std::to_string(d.size());
          throw(errormsg);
        }
        pack_tuple(dest, d);
        break;
      }

      case flex_type_enum::VECTOR: {
        const flex_vec& d = src.get<flex_vec>();
        if (d.size() != sizeof...(Args)) {
          std::string errormsg =
              std::string("Expecting a list or vector of length ")
              + std::to_string(sizeof...(Args)) + ", but we got a vector of length "
              + std::to_string(d.size());
          throw(errormsg);
        }
        pack_tuple(dest, d);
        break;
      }

      default: {
        std::string errormsg =
            std::string("Expecting a list or vector of length ")
            + std::to_string(sizeof...(Args)) + ", but we got a "
            + flex_type_enum_to_name(src.get_type());
        throw(errormsg);
      }
    }
  }

  template <typename... Args>
  static void set(flexible_type& dest, const std::tuple<Args...> & src) {
    flex_vec v(sizeof...(Args));
    unpack_tuple(v, src);
    dest = std::move(v);
  }
};



////////////////////////////////////////////////////////////////////////////////
//
//   Dictionaries
//
////////////////////////////////////////////////////////////////////////////////

/** Any sequence containers of pairs of flexible type convertable
 *  values (may be numeric).  Put into a dictionary.
 */
template <> struct ft_converter<CVTR__FLEX_DICT_CONVERTIBLE_SEQUENCE> {

  template <typename T> static constexpr bool matches() {
    typedef typename first_nested_type<T>::type pair_type;

    return conditional_test<is_sequence_container<T>::value && is_std_pair<pair_type>::value,
                            is_flexible_type_convertible, pair_type>::value;
  }

  template <typename T, typename U, template <typename...> class C>
  static void get(C<std::pair<T, U> >& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::DICT) {
      const flex_dict& fd = src.get<flex_dict>();
      dest.resize(fd.size());
      for(size_t i = 0; i < fd.size(); ++i) {
        convert_from_flexible_type(dest[i].first, fd[i].first);
        convert_from_flexible_type(dest[i].second, fd[i].second);
      }
    } else if(src.get_type() == flex_type_enum::LIST) {
      const flex_list& fl = src.get<flex_list>();
      dest.resize(fl.size());
      for(size_t i = 0; i < fl.size(); ++i) {
        convert_from_flexible_type(dest[i], fl[i]);
      }
    } else {
      throw_type_conversion_error(src, "flex_dict or flex_list of 2-element list/vectors");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename T, typename U, template <typename...> class C>
  static void set(flexible_type& dest, const C<std::pair<T, U> >& src) {
   flex_dict d(src.size());
    for(size_t i = 0; i < src.size();++i) {
      d[i] = std::make_pair(convert_to_flexible_type(src[i].first), convert_to_flexible_type(src[i].second));
    }
    dest = std::move(d);
  }
};

////////////////////////////////////////////////////////////////////////////////

/** std::map<T, U>, std::unordered_map<T, U>, or boost::unordered_map,
 *  with T and U convertable to flexible_type.
 */
template <> struct ft_converter<CVTR__FLEX_DICT_CONVERTIBLE_MAPS> {

  template <typename T> static constexpr bool matches() {

    typedef typename first_nested_type<T>::type U1;
    typedef typename second_nested_type<T>::type U2;

    return (is_map<T>::value
            && conditional_test<is_map<T>::value, is_flexible_type_convertible, U1>::value
            && conditional_test<is_map<T>::value, is_flexible_type_convertible, U2>::value);
  }

  template <typename T>
  static void get(T& dest, const flexible_type& src) {
    std::pair<typename T::key_type, typename T::mapped_type> p;

    if(src.get_type() == flex_type_enum::DICT) {

      const flex_dict& fd = src.get<flex_dict>();

      for(size_t i = 0; i < fd.size(); ++i) {
        convert_from_flexible_type(p.first, fd[i].first);
        convert_from_flexible_type(p.second, fd[i].second);
        dest.insert(std::move(p));
      }
    } else if(src.get_type() == flex_type_enum::LIST) {
      const flex_list& l = src.get<flex_list>();
      for(size_t i = 0; i < l.size(); ++i) {
        convert_from_flexible_type(p, l[i]);
        dest.insert(std::move(p));
      }
    } else {
      throw_type_conversion_error(src, "flex_dict / list of 2-element flex_lists/flex_vec");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename T>
  static void set(flexible_type& dest, const T& src) {
    flex_dict fd;
    fd.reserve(src.size());
    for(const auto& p : src) {
      fd.push_back({convert_to_flexible_type(p.first), convert_to_flexible_type(p.second)});
    }
    dest = std::move(fd);
  }
};


////////////////////////////////////////////////////////////////////////////////
//
//  Lists.
//
////////////////////////////////////////////////////////////////////////////////


/** std::pair of flexible_type convertable stuff.  Note that a pair of
 *  numeric values is taken care of by the numeric case.
 */
template <> struct ft_converter<CVTR__FLEX_LIST_CONVERTIBLE_PAIR> {

  template <typename T> static constexpr bool matches() {
    typedef typename first_nested_type<T>::type  U1;
    typedef typename second_nested_type<T>::type U2;

    return (is_std_pair<T>::value
            && conditional_test<is_std_pair<T>::value, is_flexible_type_convertible, U1>::value
            && conditional_test<is_std_pair<T>::value, is_flexible_type_convertible, U2>::value);
  }

  template <typename T, typename U>
  static void get(std::pair<T, U>& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::LIST) {
      const flex_list& l = src.get<flex_list>();
      if(l.size() != 2) {
        throw_type_conversion_error(src, "2-element flex_list/flex_vec (list size != 2)");
        ASSERT_UNREACHABLE();
      }
      convert_from_flexible_type(dest.first, l[0]);
      convert_from_flexible_type(dest.second, l[1]);
    } else {
      throw_type_conversion_error(src, "2-element flex_list/flex_vec");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename T, typename U>
  static void set(flexible_type& dest, const std::pair<T,U>& src) {
    dest = flex_list{convert_to_flexible_type(src.first), convert_to_flexible_type(src.second)};
  }
};

////////////////////////////////////////////////////////////////////////////////

/** std::tuple of flexible_type-convertable values.  (Note that tuples
 * of arithmetic values are taken care of by the numeric tuple case).
 */
template <> struct ft_converter<CVTR__FLEX_LIST_CONVERTIBLE_TUPLE> {

  template <typename T> static constexpr bool matches() {
    return (is_tuple<T>::value && all_nested_true<is_flexible_type_convertible, T>::value);
  }

  template <typename... Args>
  static void get(std::tuple<Args...>& dest, const flexible_type& src) {
    switch(src.get_type()) {
      case flex_type_enum::LIST: {
        const flex_list& d = src.get<flex_list>();

        if (d.size() != sizeof...(Args)) {
          std::string errormsg =
              std::string("Expecting a list or vector of length ")
              + std::to_string(sizeof...(Args)) + ", but we got a list of length "
              + std::to_string(d.size());
          throw(errormsg);
        }
        pack_tuple(dest, d);
        break;
      }

      default: {
        std::string errormsg =
            std::string("Expecting a list or vector of length ")
            + std::to_string(sizeof...(Args)) + ", but we got a "
            + flex_type_enum_to_name(src.get_type());
        throw(errormsg);
      }
    }
  }

  template <typename... Args>
  static void set(flexible_type& dest, const std::tuple<Args...> & src) {
    flex_list v(sizeof...(Args));
    unpack_tuple(v, src);
    dest = std::move(v);
  }
};

////////////////////////////////////////////////////////////////////////////////

/** Any sequence container of values that are convertable to a
 *  flexible type, but for which the flex_dict and flex_vec converters
 *  do not apply.
 */
template <> struct ft_converter<CVTR__FLEX_LIST_CONVERTIBLE_SEQUENCE> {

  template <typename T> static constexpr bool matches() {
    return (conditional_test<
               is_sequence_container<T>::value,
               is_flexible_type_convertible, typename first_nested_type<T>::type>::value);
  }

  template <typename FlexContainer>
  static void get(FlexContainer& dest, const flexible_type& src) {
    switch(src.get_type()) {
      case flex_type_enum::LIST: {
        const flex_list& fl = src.get<flex_list>();
        dest.resize(fl.size());
        auto it = dest.begin();
        for(size_t i = 0; i < fl.size(); ++i, ++it) {
          typename FlexContainer::value_type t;
          convert_from_flexible_type(t, fl[i]);
          *it = std::move(t);
        }
        break;
      }
      case flex_type_enum::VECTOR: {
        const flex_vec& fv = src.get<flex_vec>();
        dest.resize(fv.size());

        auto it = dest.begin();
        for(size_t i = 0; i < fv.size(); ++i, ++it) {
          typename FlexContainer::value_type t;
          // To prevent difficult compiler-time issues on this
          // conversion path, convert these through a flexible_type
          // double first to get proper dynamic type checking.  The
          // fast path should be compiled in given the flattening
          // here.
          convert_from_flexible_type(t, flexible_type(fv[i]));
          *it = std::move(t);
        }
        break;
      }
      default: {
        throw_type_conversion_error(src, "flex_list");
        ASSERT_UNREACHABLE();
      }
    }
  }

  template <typename FlexContainer>
  static void set(flexible_type& dest, const FlexContainer& src) {
    flex_list fl(src.size());

    auto it = src.begin();
    for(size_t i = 0; i < fl.size(); ++i, ++it) {
      // This explicit clast here is to get around the vector<bool> reference class,
      // when the type is actually bool.
      typename first_nested_type<FlexContainer>::type v = *it;
      fl[i] = convert_to_flexible_type(std::move(v));
    }

    dest = std::move(fl);
  }
};



////////////////////////////////////////////////////////////////////////////////
//
//  Date time types
//
////////////////////////////////////////////////////////////////////////////////

// Handled by the exact case


////////////////////////////////////////////////////////////////////////////////
//
//  Enum types.
//
////////////////////////////////////////////////////////////////////////////////

/**  Enum types.
 */
template <> struct ft_converter<CVTR__ENUM> {

  template <typename T> static constexpr bool matches() {
    return std::is_enum<T>::value;
  }

  template <typename Enum>
  static void get(Enum& dest, const flexible_type& src) {
    if(src.get_type() == flex_type_enum::INTEGER) {
      dest = static_cast<Enum>(src.get<flex_int>());
    } else {
      throw_type_conversion_error(src, "integer / enum.");
      ASSERT_UNREACHABLE();
    }
  }

  template <typename Enum>
  static void set(flexible_type& dest, const Enum& val) {
    dest = static_cast<flex_int>(val);
  }
};


////////////////////////////////////////////////////////////////////////////////
//
//  All the boilerplate code to make the above work well.
//
////////////////////////////////////////////////////////////////////////////////

template <int idx> struct ft_resolver {

  ////////////////////////////////////////////////////////////////////////////////
  // Do any of the converters of this idx or lower match on this one?

  // This match is true
  template <typename T> static constexpr bool any_match(
      typename std::enable_if<ft_converter<idx>::template matches<typename base_type<T>::type>()>::type* = NULL) {
    return true;
  }

  // This match is false -- recurse
  template <typename T> static constexpr bool any_match(
      typename std::enable_if<!ft_converter<idx>::template matches<typename base_type<T>::type>()>::type* = NULL) {
    return ft_resolver<idx - 1>::template any_match<T>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Does this idx, and none before this one, match?

  template <typename T> static constexpr bool matches() {
      return (ft_converter<idx>::template matches<typename base_type<T>::type>()
              && !ft_resolver<idx - 1>::template any_match<T>());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Does this idx, and none before this one, match?

  template <typename T>
  static void get(T& t, const flexible_type& v,
                  typename std::enable_if<matches<T>()>::type* = NULL) {
    ft_converter<idx>::get(t, v);
  }

  template <typename T>
  static void get(T& t, const flexible_type& v,
                  typename std::enable_if<!matches<T>()>::type* = NULL) {
    ft_resolver<idx - 1>::get(t, v);
  }

  template <typename T>
  static void set(flexible_type& t, T&& v,
                  typename std::enable_if<matches<T>()>::type* = NULL) {
    ft_converter<idx>::set(t, std::forward<T>(v));
  }

  template <typename T>
  static void set(flexible_type& t, T&& v,
                  typename std::enable_if<!matches<T>()>::type* = NULL) {
    ft_resolver<idx - 1>::set(t, std::forward<T>(v));
  }
};

// The base case in which any_match finally returns false
template <> struct ft_resolver<0> {
  template <typename T> static constexpr bool any_match() { return false; }
  template <typename T> static constexpr bool matches() { return false; }
  template <typename... T> static void set(T...) {}
  template <typename... T> static void get(T...) {}
};

}

/**
 * is_flexible_type_convertible<T>::value is true if T can be converted
 * to and from a flexible_type via flexible_type_converter<T>.
 */
template <typename T>
struct is_flexible_type_convertible {
  static constexpr bool value = flexible_type_internals::ft_resolver<
    flexible_type_internals::NUM_CONVERTER_STRUCTS-1>::template any_match<typename base_type<T>::type>();
};

template <typename T> GL_HOT_INLINE_FLATTEN
static void convert_from_flexible_type(T& t, const flexible_type& f) {
  static_assert(is_flexible_type_convertible<T>::value, "Type not convertable from flexible_type.");

  flexible_type_internals::ft_resolver<flexible_type_internals::NUM_CONVERTER_STRUCTS-1>::get(t, f);
};

template <typename T> GL_HOT_INLINE_FLATTEN
static void convert_to_flexible_type(flexible_type& f, T&& t) {
  static_assert(is_flexible_type_convertible<T>::value, "Type not convertable to flexible_type.");

  flexible_type_internals::ft_resolver<flexible_type_internals::NUM_CONVERTER_STRUCTS-1>::set(f, std::forward<T>(t));
};

template <typename T> GL_HOT_INLINE_FLATTEN
static flexible_type convert_to_flexible_type(T&& t) {
  static_assert(is_flexible_type_convertible<T>::value, "Type not convertable to flexible_type.");

  flexible_type f;
  flexible_type_internals::ft_resolver<flexible_type_internals::NUM_CONVERTER_STRUCTS-1>::set(f, std::forward<T>(t));
  return f;
};

/** A class that wraps the above functions in a convenient way for testing.
 */
template <typename T>
struct flexible_type_converter {

  static constexpr bool value = is_flexible_type_convertible<T>::value;

  flexible_type set(const T& t) const { return convert_to_flexible_type(t); }
  flexible_type set(T&& t) const { return convert_to_flexible_type(t); }
  T get(const flexible_type& f) const {
    T t;
    convert_from_flexible_type(t, f);
    return t;
  }
};

/** A convenience class that is true if all arguments are flexible type convertable.
 */
template <typename... Args>
struct all_flexible_type_convertible {
  static constexpr bool value = all_true<is_flexible_type_convertible, Args...>::value;
};

} // namespace turi
#endif
