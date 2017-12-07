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
#include <boost/mpl/contains.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/count.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <flexible_type/flexible_type.hpp>
#include <flexible_type/flexible_type_mpl.hpp>
namespace turi {

/**
 * \ingroup group_gl_flexible_type
 * is_flexible_type_member<T>::value is true if T is a type contained by the
 * flexible_type, and is false otherwise.
 *
 * \code
 * is_flexible_type_member<size_t>::value // false
 * is_flexible_type_member<flex_int>::value // true  (flex_int == int64_t)
 * is_flexible_type_member<std::vector<flexible_type>>::value // true
 * \endcode
 */
template <typename T>
using is_flexible_type_member =  boost::mpl::contains<flexible_type_types, T>;

// forward declaration
template <typename T>
struct is_flexible_type_convertible;

// forward declaration
template <typename... Args>
struct all_flexible_type_convertible;

/**
 * all_arithmetic<Args...>::value is true if every type in 
 * Args are arithmetic types (numeric).
 */
template <typename... Args>
struct all_arithmetic {
  typedef boost::mpl::vector<Args...> type_sequence;
  // makes an mpl::vector<true_, false_ ....> where each element transforms
  // each arg to whether it is flexible_type convertible
  typedef typename 
      boost::mpl::transform<type_sequence, 
      std::is_arithmetic<boost::mpl::_1>>::type transformed_sequence; 

  // the number of good types (number which are true)
  typedef typename 
      boost::mpl::count<transformed_sequence, 
                        std::integral_constant<bool, true>>::type num_good;
 
  // true if all args are convertible.  
  static constexpr bool value = (num_good::value == sizeof...(Args));
};


/**
 * \ingroup group_gl_flexible_type
 * The flexible_type_converter<T> is a class which exposes two member functions.
 * \code
 *   T flexible_type_converter<T>::get(const flexible_type&)
 *   flexible_type flexible_type<T>::set(const T&)
 * \endcode
 *
 * Essentially flexible_type_converter::get converts from a flexible_type to an
 * arbitrary type T, and flexible_type_converter::set converts from an
 * arbitrary type T to a flexible_type.
 *
 * The key is to support as interesting types for T as possible.
 * The following are currently supported:
 *  - flexible_type itself
 *  - flexible_type castable cases:
 *     - Any direct member of flexible_type:
 *        - flex_int (int64_t)
 *        - flex_float (double)
 *        - flex_string (std::string)
 *        - flex_vec (std::vector<double>)
 *        - flex_list (std::vector<flexible_type>)
 *        - flex_dict (std::vector<std::pair<flexible_type, flexible_type> >)
 *        - flex_date_time (boost::ptime)
 *  - Any scalar type (any boolean, integer, or floating point type)
 *  - Recursive cases
 *     - std::vector<T> where T is of any type in this list including the
 *     recursive cases.  
 *     - std::map<S, T> where S and T are of any type in this list including
 *     the recursive cases.  (though note that complicated S types may not be
 *     representable in Python.)
 *     - std::unordered_map<S, T> where S and T are of any type in this list
 *     including the recursive cases.  (though note that complicated S types
 *     may not be representable in Python.)
 *     - std::pair<S, T> where S, T are of any type in this list including the
 *     recursive cases.
 *     - std::tuple<T...> where T... are of any type in this list including the
 *     recursive cases.
 * 
 * flexible_type_converter_implementation Details
 * ----------------------
 * The key difficulty is to make sure that every T matches *exactly* one case.
 * Each case below is numbered, and we document here how each situation maps
 * to each case.
 *
 * Base case. 
 * All failed templatizations will come here where compilation will fail.
 */
template <typename T, class Enable=void>
struct flexible_type_converter {
  // every successful specialization has a value = True. 
  // This one has a value = False.
  // This value is used in is_flexible_type_convertible<T> and 
  // all_flexible_type_convertible<Args...> to test if stuff are convertible 
  // to flexible_type via flexible_type_converter<T>
  static constexpr bool value = false;
};

/**
 * Case 1a: Cover all the direct members of flexible_type, 
 *         EXCEPT for numeric types (flex_int, flex_float), and flex_list.
 *        - flex_string (std::string)
 *        - flex_vec (std::vector<double>)
 *        - flex_dict (std::vector<std::pair<flexible_type, flexible_type> >)
 *        - flex_date_time (boost::ptime)
 */
template <typename T>
struct flexible_type_converter<T,
    typename std::enable_if<is_flexible_type_member<T>::value &&
                            !std::is_arithmetic<T>::value &&
                            !std::is_same<T, flex_list>::value>::type> {
  static constexpr bool value = true;
  T get(const flexible_type& val) {
    if (type_to_enum<T>::value != val.get_type()) {
      std::string errormsg = std::string("Expecting ")
          + flex_type_enum_to_name(type_to_enum<T>::value) 
          + ". But we got a " + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }
    return val.get<T>();
  }
  flexible_type set(const T& val) {
    return flexible_type(val);
  }
};


/**
 * Case 1b: flex_list: std::vector<flexible_type> 
 */
template <>
struct flexible_type_converter<flex_list, void> {
  static constexpr bool value = true;
  flex_list get(const flexible_type& val) {
    if (val.get_type() == flex_type_enum::LIST) {
      return val;
    }
    else if (val.get_type() == flex_type_enum::VECTOR) {
      flex_vec f = val.to<flex_vec>();
      flex_list ret;
      ret.resize(f.size());
      for (size_t i = 0;i < f.size(); ++i) ret[i] = f[i];
      return ret;
    } else {
      std::string errormsg = 
          std::string("Expecting a list or array, but we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }
  }
  flexible_type set(const flex_list& val) {
    return flexible_type(val);
  }
};


/**
 * Case 2: Cover's flexible_type itself
 */
template <>
struct flexible_type_converter<flexible_type, void> {
  static constexpr bool value = true;
  flexible_type get(const flexible_type& val) {
    return val;
  }
  flexible_type set(const flexible_type& val) {
    return val;
  }
};


/**
 * Case 3: All numeric types.
 */
template <typename T>
struct flexible_type_converter<T, 
    typename std::enable_if<std::is_arithmetic<T>::value>::type> {
  static constexpr bool value = true;
  T get(const flexible_type& val) {
    if (val.get_type() != flex_type_enum::INTEGER && 
        val.get_type() != flex_type_enum::FLOAT) {
      std::string errormsg = 
          std::string("Expecting a numeric type, But we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }
    return T(val);
  }
  flexible_type set(const T& val) {
    return flexible_type(val);
  }
};

/**
 * Case 4: std::vector<T> where T is a numeric type. Converts to flex_vec.
 * Must exclude the types already handled by case 1.
 */
template <typename T>
struct flexible_type_converter<std::vector<T>, 
    typename std::enable_if<std::is_arithmetic<T>::value &&
                            !is_flexible_type_member<std::vector<T>>::value
                            >::type> {
  static constexpr bool value = true;
  std::vector<T> get(const flexible_type& val) {

    if (val.get_type() != flex_type_enum::VECTOR) {
      std::string errormsg = 
          std::string("Expecting an array of numbers, But we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }

    flex_vec f = val.to<flex_vec>();
    std::vector<T> ret;
    ret.resize(f.size());
    for (size_t i = 0;i < f.size(); ++i) ret[i] = f[i];
    return ret;
  }
  flexible_type set(const std::vector<T>& val) {
    flex_vec f;
    f.resize(val.size());
    for (size_t i = 0;i < val.size(); ++i) f[i] = val[i];
    return f;
  }
};


/**
 * Case 5: std::vector<T> where T is is of any type convertible to flexible_type.
 * Must exclude the types already handled by case 1, and the numeric types
 * handled by case 3. Converts to flex_list.
 */
template <typename T>
struct flexible_type_converter<std::vector<T>, 
    typename std::enable_if<is_flexible_type_convertible<T>::value &&
                            !is_flexible_type_member<std::vector<T>>::value &&
                            !std::is_arithmetic<T>::value>::type> {
  static constexpr bool value = true;
  std::vector<T> get(const flexible_type& val) {

    if (val.get_type() != flex_type_enum::LIST) {
      std::string errormsg = std::string("Expecting a list, But we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }

    const flex_list& d = val.get<flex_list>();
    std::vector<T> ret(d.size());
    for (size_t i = 0;i < d.size(); ++i) {
      ret[i] = flexible_type_converter<T>().get(d[i]);
    }
    return ret;
  }
  flexible_type set(const std::vector<T>& val) {
    flex_list ret(val.size());
    for (size_t i = 0;i < val.size(); ++i) {
      ret[i] = flexible_type_converter<T>().set(val[i]);
    }
    return ret;
  }
};

/**
 * Case 6: Covers the case std::map<S, T> where S, T are convertible to 
 * flexible_type.
 */
template <typename S, typename T>
struct flexible_type_converter<std::map<S, T>,
    typename std::enable_if<(is_flexible_type_convertible<S>::value && 
                             is_flexible_type_convertible<T>::value)>::type> {
  static constexpr bool value = true;
  std::map<S, T> get(const flexible_type& val) {

    if (val.get_type() != flex_type_enum::DICT) {
      std::string errormsg = 
          std::string("Expecting a dictionary, But we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }

    const flex_dict& d = val.get<flex_dict>();
    std::map<S, T> ret;
    for (const auto& elem: d) {
      ret.insert({flexible_type_converter<S>().get(elem.first), 
                  flexible_type_converter<T>().get(elem.second)});
    }
    return ret;
  }
  flexible_type set(const std::map<S, T>& val) {
    flex_dict ret;
    for (const auto& elem: val) {
      ret.push_back({flexible_type_converter<S>().set(elem.first), 
                     flexible_type_converter<T>().set(elem.second)});
    }
    return flexible_type(ret);
  }
};

/**
 * Case 7: Covers the case std::unordered_map<S, T> where S, T are convertible
 * to flexible_type.
 */
template <typename S, typename T>
struct flexible_type_converter<std::unordered_map<S, T>,
    typename std::enable_if<(is_flexible_type_convertible<S>::value && 
                             is_flexible_type_convertible<T>::value)>::type> {
  static constexpr bool value = true;
  std::unordered_map<S, T> get(const flexible_type& val) {

    if (val.get_type() != flex_type_enum::DICT) {
      std::string errormsg = 
          std::string("Expecting a dictionary, But we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }

    const flex_dict& d = val.get<flex_dict>();
    std::unordered_map<S, T> ret;
    for (const auto& elem: d) {
      ret.insert({flexible_type_converter<S>().get(elem.first), 
                  flexible_type_converter<T>().get(elem.second)});
    }
    return ret;
  }
  flexible_type set(const std::unordered_map<S, T>& val) {
    flex_dict ret;
    for (const auto& elem: val) {
      ret.push_back({flexible_type_converter<S>().set(elem.first), 
                     flexible_type_converter<T>().set(elem.second)});
    }
    return ret;
  }
};


/**
 * Case 8: Covers the case std::pair<S, T> where S, T are convertible
 * to flexible_type.
 */
template <typename S, typename T>
struct flexible_type_converter<std::pair<S, T>,
    typename std::enable_if<is_flexible_type_convertible<S>::value && 
                            is_flexible_type_convertible<T>::value &&
                            !(std::is_arithmetic<S>::value &&
                              std::is_arithmetic<T>::value)>::type> {
  static constexpr bool value = true;
  std::pair<S, T> get(const flexible_type& val) {
    if (val.get_type() != flex_type_enum::LIST) {
      std::string errormsg = 
          std::string("Expecting a list of length 2, But we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }
    const flex_list& d = val.get<flex_list>();
    if (d.size() != 2) {
      std::string errormsg = 
          std::string("Expecting a list of length 2, But got a list of length ") 
          + std::to_string(d.size());
      throw(errormsg);
    }
    return {flexible_type_converter<S>().get(d[0]), 
            flexible_type_converter<T>().get(d[1])};
  }
  flexible_type set(const std::pair<S, T>& val) {
    flex_list ret;
    ret.push_back(flexible_type_converter<S>().set(val.first)); 
    ret.push_back(flexible_type_converter<T>().set(val.second));
    return ret;
  }
};


/**
 * Case 9: Covers the case std::pair<S, T> where S, T are numeric.
 */
template <typename S, typename T>
struct flexible_type_converter<std::pair<S, T>,
    typename std::enable_if<std::is_arithmetic<S>::value &&
                            std::is_arithmetic<T>::value>::type> {
  static constexpr bool value = true;
  std::pair<S, T> get(const flexible_type& val) {
    if (val.get_type() != flex_type_enum::VECTOR) {
      std::string errormsg = 
          std::string("Expecting a numeric array of length 2, But we got a ") 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }
    const flex_vec& d = val.get<flex_vec>();
    if (d.size() != 2) {
      std::string errormsg = 
          std::string("Expecting a numeric array of length 2, "
                      "But we got an array of length ") 
          + std::to_string(d.size());
      throw(errormsg);
    }
    return {d[0],d[1]};
  }
  flexible_type set(const std::pair<S, T>& val) {
    flex_vec ret;
    ret.push_back(val.first);
    ret.push_back(val.second);
    return ret;
  }
};


namespace flexible_type_converter_impl {

/**
 * Fills in a tuple<T ...> with values from a flex_list, using
 * the flexible_type_converter to convert each value.
 * 
 * operator()<int i> essentially performs
 *    tuple[i] = (convert to tuple_i type)input[i]
 * where input[i] is a vector of flexible_type
 */
template <typename TupleType>
struct fill_tuple_from_flex_list {
  const flex_list* input;
  mutable TupleType tuple;
  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const { 
    typedef typename std::decay<decltype(std::get<n>(tuple))>::type TargetType;
    std::get<n>(tuple) = 
        flexible_type_converter<TargetType>().get(input->at(n));
  }
};


/**
 * Fills in a flex_list from tuple<T ...> 
 * the flexible_type_converter to convert each value.
 * 
 * operator()<int i> essentially performs
 *    output[i] = (convert to flexible_type type)tuple[i]
 * where output[i] is a vector of flexible_type
 */
template <typename TupleType>
struct fill_flex_list_from_tuple{
  const TupleType* input;
  mutable flex_list output;
  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const { 
    typedef typename std::decay<decltype(std::get<n>(*input))>::type TargetType;
    output.at(n) = 
        flexible_type_converter<TargetType>().set(std::get<n>(*input));
  }
};


/**
 * Fills in a tuple<T ...> with values from a flex_vec. 
 * T... must be all arithmetic.
 * 
 * operator()<int i> essentially performs
 *    tuple[i] = input[i]
 * where input[i] is a double vector 
 */
template <typename TupleType>
struct fill_tuple_from_flex_vec {
  const flex_vec* input;
  mutable TupleType tuple;
  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const { 
    typedef typename std::decay<decltype(std::get<n>(tuple))>::type TargetType;
    std::get<n>(tuple) = input->at(n);
  }
};


/**
 * Fills in a vector<flexible_type> from tuple<T ...> 
 * the flexible_type_converter to convert each value.
 * 
 * operator()<int i> essentially performs
 *    output[i] = tuple[i]
 * where output[i] is a double vector 
 */
template <typename TupleType>
struct fill_flex_vec_from_tuple{
  const TupleType* input;
  mutable flex_vec output;
  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const { 
    typedef typename std::decay<decltype(std::get<n>(*input))>::type TargetType;
    output.at(n) = std::get<n>(*input);
  }
};

} // flexible_type_converter_impl

/**
 * Case 10: std::tuple<T...> where T... are all convertible to flexible_type
 */
template <typename... Args>
struct flexible_type_converter<std::tuple<Args...>, 
      typename std::enable_if<all_flexible_type_convertible<Args...>::value &&
                             !all_arithmetic<Args...>::value>::type > {
  static constexpr bool value = true;
  std::tuple<Args...> get(const flexible_type& val) {

    if (val.get_type() != flex_type_enum::LIST) {
      std::string errormsg = 
          std::string("Expecting a list of length ") 
          + std::to_string(sizeof...(Args)) + ", But we got a " 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }
    const flex_list& d = val.get<flex_list>();
    if (d.size() != sizeof...(Args)) {
      std::string errormsg = 
          std::string("Expecting a list of length ") 
          + std::to_string(sizeof...(Args)) + ", But we got a list of length " 
          + std::to_string(d.size());
      throw(errormsg);
    }
    typename boost::mpl::range_c<int, 0, sizeof...(Args)>::type tuple_range;
    flexible_type_converter_impl::
        fill_tuple_from_flex_list<std::tuple<Args...>> filler;
    filler.input = &d;
    boost::fusion::for_each(tuple_range, filler);
    return filler.tuple;
  }
  flexible_type set(const std::tuple<Args...> & val) {
    typename boost::mpl::range_c<int, 0, sizeof...(Args)>::type tuple_range;
    flexible_type_converter_impl::
        fill_flex_list_from_tuple<std::tuple<Args...>> filler;
    filler.input = &val;
    filler.output.resize(sizeof...(Args));
    boost::fusion::for_each(tuple_range, filler);
    return filler.output;
  }
};


/**
 * Case 11: std::tuple<T...> where T... are all arithmetic (numeric) types
 */
template <typename... Args>
struct flexible_type_converter<std::tuple<Args...>, 
      typename std::enable_if<all_arithmetic<Args...>::value>::type > {
  static constexpr bool value = true;
  std::tuple<Args...> get(const flexible_type& val) {

    if (val.get_type() != flex_type_enum::VECTOR) {
      std::string errormsg = 
          std::string("Expecting a numeric array of length ") 
          + std::to_string(sizeof...(Args)) + ", But we got a " 
          + flex_type_enum_to_name(val.get_type());
      throw(errormsg);
    }
    const flex_vec& d = val.get<flex_vec>();
    if (d.size() != sizeof...(Args)) {
      std::string errormsg = 
          std::string("Expecting a numeric array of length ") 
          + std::to_string(sizeof...(Args)) 
          + ", But we got a numeric array of length " 
          + std::to_string(d.size());
      throw(errormsg);
    }
    typename boost::mpl::range_c<int, 0, sizeof...(Args)>::type tuple_range;
    flexible_type_converter_impl::
        fill_tuple_from_flex_vec<std::tuple<Args...>> filler;
    filler.input = &d;
    boost::fusion::for_each(tuple_range, filler);
    return filler.tuple;
  }
  flexible_type set(const std::tuple<Args...> & val) {
    typename boost::mpl::range_c<int, 0, sizeof...(Args)>::type tuple_range;
    flexible_type_converter_impl::
        fill_flex_vec_from_tuple<std::tuple<Args...>> filler;
    filler.input = &val;
    filler.output.resize(sizeof...(Args));
    boost::fusion::for_each(tuple_range, filler);
    return filler.output;
  }
};

/**
 * Case 12: All enums.
 */
template <typename T>
struct flexible_type_converter<T, 
    typename std::enable_if<std::is_enum<T>::value>::type> {
  static constexpr bool value = true;
  T get(const flexible_type& val) {
    if (val.get_type() != flex_type_enum::INTEGER) {
      std::string errormsg = 
          (std::string("Expecting a integer type convertable to enum type '")
           + typeid(T).name() + "', but we got a "
           + flex_type_enum_to_name(val.get_type()));
      throw(errormsg);
    }
    return static_cast<T>(val.get<flex_int>());
  }
  flexible_type set(const T& val) {
    return flexible_type(static_cast<flex_int>(val));
  }
};


/**
 * is_flexible_type_convertible<T>::value is true if T can be converted
 * to and from a flexible_type via flexible_type_converter<T>.
 */
template <typename T>
struct is_flexible_type_convertible {
  static constexpr bool value = flexible_type_converter<T>::value;
  typedef boost::mpl::bool_<value> type;
};

/**
 * all_flexible_type_convertible<Args...>::value is true if every type in 
 * Args can be converted to and from a flexible_type via 
 * flexible_type_converter<T>.
 */
template <typename... Args>
struct all_flexible_type_convertible {
  typedef boost::mpl::vector<Args...> type_sequence;
  // makes an mpl::vector<true_, false_ ....> where each element transforms
  // each arg to whether it is flexible_type convertible
  typedef typename 
      boost::mpl::transform<type_sequence, 
      is_flexible_type_convertible<boost::mpl::_1>>::type transformed_sequence; 

  // the number of good types (number which are true)
  typedef 
      typename boost::mpl::count<transformed_sequence, 
                                 boost::mpl::bool_<true>>::type num_good;
 
  // true if all args are convertible.  
  static constexpr bool value = (num_good::value == sizeof...(Args));
};


} // namespace turi
#endif
