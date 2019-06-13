/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_LIB_VARIANT_DETAIL_HPP
#define TURI_UNITY_LIB_VARIANT_DETAIL_HPP
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
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/unity_global_singleton.hpp>
#include <core/data/flexible_type/flexible_type_converter.hpp>
#include <model_server/lib/api/function_closure_info.hpp>
namespace turi {

// forward declarations
class unity_sarray_base;
class unity_sframe_base;
class unity_sgraph_base;
class model_base;
class unity_sarray;
class unity_sframe;
class unity_sgraph;
class gl_sframe;
class gl_sarray;
class gl_sgraph;
class gl_gframe;

extern int64_t USE_GL_DATATYPE;

/**************************************************************************/
/*                                                                        */
/*                         Some Helper Templates                          */
/*                                                                        */
/**************************************************************************/
/**
 * is_variant_member<T>::value is true if T is a type contained by the
 * variant_type, and is false otherwise.
 *
 * \code
 * is_variant_member<unity_sarray_base*>::value // true
 * is_variant_member<size_t>::value // false
 * is_variant_member<flexible_type>::value // true
 * \endcode
 */
template <typename T>
using is_variant_member = boost::mpl::contains<variant_type::types, T>;

/**
 * is_model_descendent<T>::value is true if *T  is a descendent of
 * model_base, or is model_base itself.
 *
 * \code
 * is_model_descendent<flexible_type>::value // false
 * is_model_descendent<svariant_converter_imple_model*>::value // true
 * is_model_descendent<svariant_converter_imple_model>::value // false
 * is_model_descendent<model_base*>::value // true
 * \endcode
 */
template <typename T>
using is_model_descendent =  std::is_convertible<T*, model_base*>;

template <typename T>
struct is_toolkit_builtin {
  static constexpr bool value =
      std::is_same<typename std::decay<T>::type, gl_sarray>::value ||
      std::is_same<typename std::decay<T>::type, gl_sframe>::value ||
      std::is_same<typename std::decay<T>::type, gl_sarray>::value ||
      std::is_same<typename std::decay<T>::type, gl_sgraph>::value;
};



// forward declaration
template <typename T>
struct is_variant_convertible;

// forward declaration
template <typename... Args>
struct all_variant_convertible;

/**************************************************************************/
/*                                                                        */
/*                           variant_converter                            */
/*                                                                        */
/**************************************************************************/

/**
 * The variant_converter<T> is a class which exposes two member functions.
 * \code
 *   T variant_converter<T>::get(const variant_type&)
 *   variant_type variant_converter<T>::set(const T&)
 * \endcode
 *
 * Essentially variant_converter::get converts from a variant type to an
 * arbitrary type T, and variant_converter::set converts from an arbitrary type
 * T to a variant_type.
 *
 * The key is to support as interesting types for T as possible.
 * The following are currently supported:
 *  - variant_type
 *  - any direct member of variant_type:
 *     - std::shared_ptr<unity_sarray_base>
 *     - std::shared_ptr<unity_sframe_base>
 *     - std::shared_ptr<unity_sgraph_base>
 *     - std::shared_ptr<model_base>
 *     - std::vector<variant_type>
 *     - std::map<std::string, variant_type>
 *  - everything convertible to flexible_type (See flexible_type_conveter.hpp)
 *  - std::shared_ptr<unity_sarray>
 *  - std::shared_ptr<unity_sframe>
 *  - std::shared_ptr<unity_sgraph>
 *  - gl_sarray
 *  - gl_sframe
 *  - gl_sgraph
 *  - gl_gframe
 *  - std::shared_ptr<T> where T is a descendent of model_base
 *  - variant_type
 *  - Recursive cases
 *     - std::vector<T> where T is of any type in this list including the
 *     recursive cases.
 *     - std::map<std::string, T> where T is of any type in this list including
 *     the recursive cases.
 *     - std::unordered_map<std::string, T> where T are of any type in this list
 *     including the recurrsive cases.
 *     - std::pair<S, T> where S, T are of any type in this list including the
 *     recursive cases.
 *     - std::tuple<T...> where T... are of any type in this list including the
 *     recursive cases.
 *
 * variant_converter_implementation Details
 * ----------------------
 * The key difficulty is to make sure that every T matches *exactly* one case.
 * Each case below is numbered, and we document here how each situation maps
 * to each case.
 *
 * Base case.
 * All failed templatizations will come here where compilation will fail.
 */
template <typename T, class Enable = void>
struct variant_converter {
  static constexpr bool value = false;
};


/**
 * Case 1: Cover everything convertible to flexible_type.
 *
 * All remaining cases *must* exclude this case.
 */
template <typename T>
struct variant_converter<T,
    typename std::enable_if<is_flexible_type_convertible<T>::value>::type> {
  static constexpr bool value = true;

  T get(const variant_type& val) {
    flexible_type f;
    try {
      f = variant_get_ref<flexible_type>(val);
    } catch(...) {
      std::string errormsg =
          std::string("Expecting a flexible_type. Got a ") +
          get_variant_which_name(val.which());
      std_log_and_throw(std::invalid_argument, errormsg);
    }
    return flexible_type_converter<T>().get(f);
  }

  variant_type set(const T& val) {
    return flexible_type_converter<T>().set(val);
  }
};

/**
 * Case 2: Cover all direct members of variant_type.
 * But not flexible_type since that is covered by case 1.
 *     - unity_sarray_base*
 *     - unity_sframe_base*
 *     - unity_sgraph_base*
 *     - model_base*
 *     - std::vector<variant_type>
 *     - std::map<std::string, variant_type>
 */
template <typename T>
struct variant_converter<T,
    typename std::enable_if<(is_variant_member<T>::value &&
                             !std::is_same<T, flexible_type>::value) ||
                             std::is_same<T, variant_type>::value>::type> {
  static constexpr bool value = true;
  T get(const variant_type& val) {
    return variant_get_ref<T>(val);
  }
  variant_type set(const T& val) {
    return variant_type(val);
  }
};

/**
 * Case 3: Cover variant_type itself.
 */
template <>
struct variant_converter<variant_type, void> {
  static constexpr bool value = true;
  variant_type get(const variant_type& val) {
    return val;
  }
  variant_type set(const variant_type& val) {
    return val;
  }
};

/**
 * Case 4: Cover unity_sarray*.
 * (note that this case is not covered by Case 2 since variant_type
 * stores unity_sarray_base* and not unity_sarray*
 */
template <>
struct variant_converter<std::shared_ptr<unity_sarray>, void> {
  static constexpr bool value = true;
  std::shared_ptr<unity_sarray> get(const variant_type& val);
  variant_type set(std::shared_ptr<unity_sarray> val);
};

/**
 * Case 5: Cover unity_sframe*.
 * (note that this case is not covered by Case 2 since variant_type
 * stores unity_sframe_base* and not unity_sframe*
 */
template <>
struct variant_converter<std::shared_ptr<unity_sframe>, void> {
  static constexpr bool value = true;
  std::shared_ptr<unity_sframe> get(const variant_type& val);
  variant_type set(std::shared_ptr<unity_sframe> val);
};

/**
 * Case 6: Cover unity_sgraph*.
 * (note that this case is not covered by Case 2 since variant_type
 * stores unity_sgraph_base* and not unity_sgraph*
 */
template <>
struct variant_converter<std::shared_ptr<unity_sgraph>, void> {
  static constexpr bool value = true;
  std::shared_ptr<unity_sgraph> get(const variant_type& val);
  variant_type set(std::shared_ptr<unity_sgraph> val);
};


/**
 * Case 7a: Cover pointer descendents of model_base.
 *
 * This case must not capture the case where T is itself model_base*
 * since that case is already covered by case 2 (direct members of variant_type)
 */
template <typename T>
struct variant_converter<std::shared_ptr<T>,
    typename std::enable_if<is_model_descendent<T>::value &&
                            !std::is_same<T, model_base>::value &&
                            !is_toolkit_builtin<T>::value>::type> {
  static constexpr bool value = true;
  std::shared_ptr<T> get(const variant_type& val) {
    return std::dynamic_pointer_cast<T>(variant_get_ref<std::shared_ptr<model_base>>(val));
  }
  variant_type set(const std::shared_ptr<T>& val) {
    return variant_type(std::static_pointer_cast<model_base>(val));
  }
};


/**
 * Case 7b: Cover descendents of model_base.
 *
 * This case must not capture the case where T is itself model_base*
 * since that case is already covered by case 2 (direct members of variant_type)
 */
template <typename T>
struct variant_converter<T,
    typename std::enable_if<is_model_descendent<T>::value &&
                            !std::is_same<T, model_base>::value &&
                            !is_toolkit_builtin<T>::value>::type> {
  static constexpr bool value = true;
  T get(const variant_type& val) {
    return *std::dynamic_pointer_cast<T>(variant_get_ref<std::shared_ptr<model_base>>(val));
  }
  variant_type set(const T& val) {
    return variant_type(std::static_pointer_cast<model_base>(std::make_shared<T>(val)));
  }
};
/**
 * Case 8: std::vector<T> where T can be contained by a variant.
 *
 * This covers all the remaining std::vector<T> cases.
 * It must be careful to exclude Case 2. This must be disabled for
 * std::vector<variant_type>
 *       std::vector<variant_type>
 */
template <typename T>
struct variant_converter<std::vector<T>,
    typename std::enable_if<!is_flexible_type_convertible<std::vector<T>>::value &&
                            is_variant_convertible<T>::value &&
                            !is_variant_member<std::vector<T>>::value>::type> {
  static constexpr bool value = true;
  std::vector<T> get(const variant_type& val_) {
    const variant_vector_type& val  = variant_get_ref<variant_vector_type>(val_);
    std::vector<T> ret(val.size());
    for (size_t i = 0;i < val.size(); ++i) {
      ret[i] = variant_converter<T>().get(val[i]);
    }
    return ret;
  }
  variant_type set(const std::vector<T>& val) {
    variant_vector_type ret(val.size());
    for (size_t i = 0;i < val.size(); ++i) {
      ret[i] = variant_converter<T>().set(val[i]);
    }
    return variant_type(ret);
  }
};

/**
 * Case 9: Covers the case std::map<std::string, T> for any T which
 * is convertible to variant_type.
 * It must be careful to exclude Case 2. This must be disabled for
 * std::map<std::string, variant_type>.
 */
template <typename T>
struct variant_converter<std::map<std::string, T>,
    typename std::enable_if<!is_flexible_type_convertible<T>::value &&
                            is_variant_convertible<T>::value &&
                            !is_variant_member<std::map<std::string, T>>::value>::type> {
  static constexpr bool value = true;
  std::map<std::string, T> get(const variant_type& val_) {
    const variant_map_type& val  = variant_get_ref<variant_map_type>(val_);
    std::map<std::string, T> ret;
    for (const auto& elem: val) {
      ret[elem.first] = variant_converter<T>().get(elem.second);
    }
    return ret;
  }
  variant_type set(const std::map<std::string, T>& val) {
    variant_map_type ret;
    for (const auto& elem: val) {
      ret[elem.first] = variant_converter<T>().set(elem.second);
    }
    return variant_type(ret);
  }
};


/**
 * Case 10: Covers the case std::unordered_map<std::string, T> for any T which
 * is convertible to variant_type.
 */
template <typename T>
struct variant_converter<std::unordered_map<std::string, T>,
    typename std::enable_if<!is_flexible_type_convertible<T>::value &&
                            is_variant_convertible<T>::value>::type> {
  static constexpr bool value = true;
  std::unordered_map<std::string, T> get(const variant_type& val_) {
    const variant_map_type& val  = variant_get_ref<variant_map_type>(val_);
    std::unordered_map<std::string, T> ret;
    for (const auto& elem: val) {
      ret[elem.first] = variant_converter<T>().get(elem.second);
    }
    return ret;
  }
  variant_type set(const std::unordered_map<std::string, T>& val) {
    variant_map_type ret;
    for (const auto& elem: val) {
      ret[elem.first] = variant_converter<T>().set(elem.second);
    }
    return variant_type(ret);
  }
};

/**
 * Case 11: Covers the case std::pair<S, T> where S, T are convertible
 * to variant_type.
 */
template <typename S, typename T>
struct variant_converter<std::pair<S, T>,
    typename std::enable_if<!is_flexible_type_convertible<T>::value &&
                            is_variant_convertible<T>::value>::type> {
  static constexpr bool value = true;
  std::pair<S, T> get(const variant_type& val) {
    std::vector<variant_type> ret =
        variant_converter<std::vector<variant_type>>().get(val);
    if (ret.size() != 2) {
      std_log_and_throw(std::invalid_argument,
               "Expecting an array of length 2");
    }
    return {variant_converter<S>().get(ret[0]),
            variant_converter<T>().get(ret[1])};
  }
  variant_type set(const std::pair<S, T>& val) {
    variant_vector_type ret;
    ret.push_back(variant_converter<S>().set(val.first));
    ret.push_back(variant_converter<T>().set(val.second));
    return ret;
  }
};


#ifndef DISABLE_SDK_TYPES
/**
 * Case 12: gl datatypes
 */
template <>
struct variant_converter<gl_sarray, void> {
  static constexpr bool value = true;
  gl_sarray get(const variant_type& val);
  variant_type set(gl_sarray val);
};

template <>
struct variant_converter<gl_sframe, void> {
  static constexpr bool value = true;
  gl_sframe get(const variant_type& val);
  variant_type set(gl_sframe val);
};

template <>
struct variant_converter<gl_sgraph, void> {
  static constexpr bool value = true;
  gl_sgraph get(const variant_type& val);
  variant_type set(gl_sgraph val);
};

template <>
struct variant_converter<gl_gframe, void> {
  static constexpr bool value = true;
  gl_gframe get(const variant_type& val);
  variant_type set(gl_gframe val);
};
#endif

namespace variant_converter_impl {

template <typename TupleType>
struct fill_tuple {
  const std::vector<variant_type>* input;
  mutable TupleType* tuple;
  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const {
    typedef typename std::decay<decltype(std::get<n>(*tuple))>::type TargetType;
    std::get<n>(*tuple) = variant_converter<TargetType>().get(input->at(n));
  }
};


template <typename TupleType>
struct fill_variant{
  const TupleType* input;
  mutable std::vector<variant_type>* output;
  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const {
    typedef typename std::decay<decltype(std::get<n>(*input))>::type TargetType;
    output->at(n) = variant_converter<TargetType>().set(std::get<n>(*input));
  }
};


/**
 * Gets a callable toolkit function from a closure specification obtaining
 * the function from the unity_global singleton's instance of the toolkit
 * function registry.
 */
std::function<variant_type(const std::vector<variant_type>&)>
    get_toolkit_function_from_closure(const function_closure_info& closure);

} // variant_converter_impl

/**
 * Case 12: std::tuple<T...> where T... are all convertible to variant_type
 */
template <typename... Args>
struct variant_converter<std::tuple<Args...>,
    typename std::enable_if<!all_flexible_type_convertible<Args...>::value &&
                            all_variant_convertible<Args...>::value>::type> {
  static constexpr bool value = true;
  std::tuple<Args...> get(const variant_type& val) {
    std::vector<variant_type> cv =
        variant_converter<std::vector<variant_type>>().get(val);
    if (cv.size() != sizeof...(Args)) {
      std::string error_msg =
          "Expecting an array of length " + std::to_string(sizeof...(Args));
      std_log_and_throw(std::invalid_argument, error_msg);
    }

    typename boost::mpl::range_c<int, 0, sizeof...(Args)>::type tuple_range;
    variant_converter_impl::fill_tuple<std::tuple<Args...>> filler;
    std::tuple<Args...> output;
    filler.input = &cv;
    filler.tuple = &output;
    boost::fusion::for_each(tuple_range, filler);
    return output;
  }
  variant_type set(const std::tuple<Args...> & val) {
    typename boost::mpl::range_c<int, 0, sizeof...(Args)>::type tuple_range;
    variant_converter_impl::fill_variant<std::tuple<Args...>> filler;
    std::vector<variant_type> output;
    filler.input = &val;
    filler.output = &output;
    output.resize(sizeof...(Args));
    boost::fusion::for_each(tuple_range, filler);
    return output;
  }
};

/**
 * Case 13: Covers the case std::function<S(T...)> where S, T... are convertible
 * to variant_type.
 */
template <typename S, typename... Args>
struct variant_converter<std::function<S(Args...)>,
    typename std::enable_if<is_variant_convertible<S>::value &&
                            all_variant_convertible<Args...>::value>::type> {
  static constexpr bool value = true;
  std::function<S(Args...)> get(const variant_type& val) {
    function_closure_info closure;
    closure = variant_get_ref<function_closure_info>(val);
    auto native_execute_function =
        variant_converter_impl::get_toolkit_function_from_closure(closure);
    // ok. now we need to wrap the native function up into a function of the
    // appropriate type. How do we do that?
    return [=](Args... args)->S {
      std::tuple<Args...> val{args...};
      typename boost::mpl::range_c<int, 0, sizeof...(Args)>::type tuple_range;
      variant_converter_impl::fill_variant<std::tuple<Args...>> filler;
      filler.input = &val;
      filler.output->resize(sizeof...(Args));
      boost::fusion::for_each(tuple_range, filler);
      return variant_converter<S>().get(native_execute_function(*(filler.output)));
    };
  }
  variant_type set(const std::function<S(Args...)>& val) {
    std_log_and_throw(std::invalid_argument,
                      "Cannot convert function to variant");
    ASSERT_UNREACHABLE();
  }
};



/**
 * is_variant_type_convertible<T>::value is true if T can be converted
 * to and from a variant_type via variant_converter<T>.
 */
template <typename T>
struct is_variant_convertible{
  static constexpr bool value = variant_converter<T>::value;
  typedef boost::mpl::bool_<value> type;
};

/**
 * all_flexible_type_convertible<Args...>::value is true if every type in
 * Args can be converted to and from a flexible_type via
 * flexible_type_convert<T>.
 */
template <typename... Args>
struct all_variant_convertible {
  typedef boost::mpl::vector<Args...> type_sequence;
  // makes an mpl::vector<true_, false_ ....> where each element transforms
  // each arg to whether it is flexible_type convertible
  typedef typename boost::mpl::transform<type_sequence,
                                         is_variant_convertible<boost::mpl::_1>>::type transformed_sequence;

  // the number of good types (number which are true)
  typedef typename boost::mpl::count<transformed_sequence, boost::mpl::bool_<true>>::type num_good;

  // true if all args are convertible.
  static constexpr bool value = (num_good::value == sizeof...(Args));
};

}; // turicreate

#endif
