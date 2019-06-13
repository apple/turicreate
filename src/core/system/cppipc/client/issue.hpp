/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_CLIENT_ISSUE_HPP
#define CPPIPC_CLIENT_ISSUE_HPP
#include <tuple>
#include <boost/function.hpp>
#include <boost/type_traits.hpp>
#include <boost/function_types/function_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/generics/remove_member_pointer.hpp>
#include <core/system/cppipc/util/generics/member_function_return_type.hpp>
#include <core/system/cppipc/util/generics/tuple.hpp>
#include <core/system/cppipc/ipc_object_base.hpp>
#include <type_traits>

namespace cppipc {

namespace detail{
/**
 * \internal
 * \ingroup cppipc
 * Overload of issue_disect when the tuple list is not empty.
 * Here we serialize the left most argument, shift the tuple and recursively
 * forward the call.
*/
template <typename ArgumentTuple, typename... Args>
struct issue_disect { };

/**
 * Overload of issue_disect when the tuple list is not empty.
 * Here we serialize the left most argument, shift the tuple and recursively
 * forward the call.
 */
template <typename ArgumentTuple, typename Arg, typename... Args>
struct issue_disect<ArgumentTuple, Arg, Args...> {
  static void exec(turi::oarchive& msg,
                   const Arg& a, const Args&... args) {
    static_assert(1 + sizeof...(Args) == std::tuple_size<ArgumentTuple>::value, "Argument Count Mismatch");
    typedef typename std::tuple_element<0, ArgumentTuple>::type  arg_type;

    typedef typename std::decay<arg_type>::type decayed_arg_type;
    if (std::is_same<decayed_arg_type, Arg>::value) {
      msg << a;
    } else {
      msg << (decayed_arg_type)(a);
    }
    typedef typename left_shift_tuple<ArgumentTuple>::type shifted_tuple;
    issue_disect<shifted_tuple, Args...>::exec(msg, args...);
  }
};

/**
 * \internal
 * \ingroup cppipc
 * Overload of execute_disect when the tuple list is empty,
 * and there are no arguments. There is nothing to do here.
 */
template <typename... Args>
struct issue_disect<std::tuple<>, Args...> {
  static void exec(turi::oarchive& msg, const Args&... args) {
    static_assert(sizeof...(Args) == 0, "Too Many Arguments");
  }
};
} // namespace detail


/**
 * \internal
 * \ingroup cppipc
 * Casts the arguments into the required types for the member function
 * and serializes it into the output archive
 */
template <typename MemFn, typename... Args>
void issue(turi::oarchive& msg,
           MemFn fn,
           const Args&... args) {
  typedef typename boost::remove_member_pointer<MemFn>::type fntype;
  typedef typename function_args_to_tuple<fntype>::type tuple;

  detail::issue_disect<tuple, Args...>::exec(msg, args...);
}

} // cppipc

#endif
