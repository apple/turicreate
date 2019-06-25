/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_CLASS_WRAPPER_IMPL_HPP
#define TURI_UNITY_TOOLKIT_CLASS_WRAPPER_IMPL_HPP
#include <model_server/lib/toolkit_function_wrapper_impl.hpp>

namespace turi {
class model_base;
namespace toolkit_class_wrapper_impl {
using turi::toolkit_function_wrapper_impl::generate_member_function_wrapper;
using turi::toolkit_function_wrapper_impl::generate_const_member_function_wrapper;

/**
 * Wraps a member function T::f(...) with a function that takes a
 * variant_map_type and returns a variant_type.
 * Essentially, given a function f of type ret(in1, in2, in3 ...),
 * returns a function g of type variant_type(variant_map_type) where g
 * performs the equivalent of:
 *
 * \code
 * variant_type g(T* t, variant_map_type input) {
 *   return variant_encode( t->f (variant_decode(input[inargnames[0]),
 *                             variant_decode(input[inargnames[1]),
 *                             variant_decode(input[inargnames[2]),
 *                             ...));
 * }
 *
 * \endcode
 * Performs the equivalent to
 * toolkit_function_wrapper_impl::generate_member_function_wrapper but takes
 * the inargnames argument as a varargs.
 */
template <typename T, typename Ret, typename... Args, typename... VarArgs>
std::function<variant_type(model_base*, variant_map_type)>
generate_member_function_wrapper_indirect(Ret (T::* fn)(Args...), VarArgs... args) {
  auto newfn = generate_member_function_wrapper<sizeof...(Args), T, Ret, Args...>(fn, {args...});
  return [newfn](model_base* curthis, variant_map_type in)->variant_type {
    return to_variant(newfn(dynamic_cast<T*>(curthis), in));
  };
}


/**
 * Wraps a member function T::f(...) const with a function that takes a
 * variant_map_type and returns a variant_type.
 * Essentially, given a function f of type ret(in1, in2, in3 ...),
 * returns a function g of type variant_type(variant_map_type) where g
 * performs the equivalent of:
 *
 * \code
 * variant_type g(T* t, variant_map_type input) {
 *   return variant_encode( t->f (variant_decode(input[inargnames[0]),
 *                             variant_decode(input[inargnames[1]),
 *                             variant_decode(input[inargnames[2]),
 *                             ...));
 * }
 *
 * \endcode
 *
 * Performs the equivalent to toolkit_function_wrapper_impl::generate_const
 * member_function_wrapper but takes the inargnames argument as a varargs.
 *
 * \note Overload of generate_member_function_wrapper_indirect to handle
 * the const case even though the code is... *identical*. I can't figure out
 * how to templatize around the const.
 */
template <typename T, typename Ret, typename... Args, typename... VarArgs>
std::function<variant_type(model_base*, variant_map_type)>
generate_member_function_wrapper_indirect(Ret (T::* fn)(Args...) const, VarArgs... args) {
  auto newfn = generate_const_member_function_wrapper<sizeof...(Args), T, Ret, Args...>(fn, {args...});
  return [newfn](model_base* curthis, variant_map_type in)->variant_type {
    return to_variant(newfn(dynamic_cast<T*>(curthis), in));
  };
}

/**
 * Given a member function of type Ret T::f(), wraps it with a function
 * that takes a variant_map_type and returns a variant type.
 *
 * Essentially, given Ret T::f(), wraps it with the following:
 *
 * \code
 * variant_type g(T* t, variant_map_type input) {
 *   return variant_encode(t->f());
 * }
 * \endcode
 */
template <typename T, typename Ret>
std::function<variant_type(model_base*, variant_map_type)>
generate_getter(Ret (T::* fn)()) {
  return [fn](model_base* curthis, variant_map_type in)->variant_type {
    T* t = dynamic_cast<T*>(curthis);
    return to_variant((t->*fn)());
  };
}



/**
 * Given a member function of type Ret T::f() const, wraps it with a function
 * that takes a variant_map_type and returns a variant type.
 *
 * Essentially, given Ret T::f(),
 * returns a function that performs the following:
 * \code
 * variant_type g(T* t, variant_map_type input) {
 *   return variant_encode(t->f());
 * }
 * \endcode
 *
 * \note Overload of generate_getter to handle
 * the const case even though the code is... *identical*. I can't figure out
 * how to templatize around the const.
 */
template <typename T, typename Ret>
std::function<variant_type(model_base*, variant_map_type)>
generate_getter(Ret (T::* fn)() const) {
  return [fn](model_base* curthis, variant_map_type in)->variant_type {
    T* t = dynamic_cast<T*>(curthis);
    return to_variant((t->*fn)());
  };
}


/**
 * Given a member function of type void T::f(S) const, wraps it with a function
 * that takes a variant_map_type and returns a variant type.
 *
 * Essentially, given void T::f(S), and the input element name input_map_elem,
 * returns a function that performs the following:
 * \code
 * variant_type g(T* t, variant_map_type input) {
 *   t->f(input[input_map_elem]);
 *   return variant_type();
 * }
 * \endcode
 *
 * \note Overload of generate_getter to handle
 * the const case even though the code is... *identical*. I can't figure out
 * how to templatize around the const.
 */
template <typename T, typename SetValType>
std::function<variant_type(model_base*, variant_map_type)>
generate_setter(void (T::* fn)(SetValType), std::string input_map_elem) {
  return [fn, input_map_elem](model_base* curthis, variant_map_type in)->variant_type {
    SetValType val = variant_get_value<SetValType>(in[input_map_elem]);
    T* t = dynamic_cast<T*>(curthis);
    (t->* fn)(val);
    return variant_type();
  };
}


}
}

#endif
