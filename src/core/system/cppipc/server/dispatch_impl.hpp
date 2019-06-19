/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_SERVER_DISPATCH_IMPL_HPP
#define CPPIPC_SERVER_DISPATCH_IMPL_HPP

#include <tuple>
#include <type_traits>
#include <boost/function.hpp>
#include <boost/type_traits.hpp>
#include <boost/function_types/function_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/generics/remove_member_pointer.hpp>
#include <core/system/cppipc/ipc_object_base.hpp>
#include <core/system/cppipc/util/generics/member_function_return_type.hpp>
#include <core/system/cppipc/util/generics/tuple.hpp>
#include <core/system/cppipc/server/dispatch.hpp>
#include <core/system/cppipc/server/comm_server.hpp>
#include <core/system/cppipc/common/ipc_deserializer.hpp>

namespace cppipc {

namespace detail{
/**
 * \internal
 * \ingroup cppipc
 * Internal utility function.
 * Calls the function, and serializes the result.
 * Works correctly for void types.
 */
template <typename RetType, typename T, typename MemFn, typename... Args>
struct exec_and_serialize_response {
  static void exec(T* objectptr,
                   comm_server* server,
                   MemFn fn,
                   turi::oarchive& response,
                   Args&... args) {
    auto ret = (objectptr->*fn)(std::forward<Args>(args)...);
    detail::set_deserializer_to_server(server);
    response << ret;
  }
};


/**
 * \internal
 * \ingroup cppipc
 * Specialization of exec_and_serialize_response to handle void return types.
 */
template <typename T, typename MemFn, typename... Args>
struct exec_and_serialize_response<void, T, MemFn, Args...> {
  static void exec(T* objectptr,
                   comm_server* server,
                   MemFn fn,
                   turi::oarchive& response,
                   Args&... args) {
    (objectptr->*fn)(std::forward<Args>(args)...);
  }
};


/**:
 * \internal
 * \ingroup cppipc
 * Recursively extracts one argument at a time from the left, and calling
 * execute_disect again with one less element in the tuple. When the tuple is
 * empty the second overload is called.
 */
template <typename T, typename Memfn, typename ArgumentTuple, typename... Args>
struct execute_disect {
  static void exec(T* objectptr,
                   comm_server* server,
                   Memfn fn,
                   turi::iarchive& msg,
                   turi::oarchive& response,
                   Args&... args) {
    typedef typename std::tuple_element<0, ArgumentTuple>::type  arg_type;
    typedef typename std::decay<arg_type>::type decayed_type;
    decayed_type arg = decayed_type();
    msg >> arg;
    typedef typename left_shift_tuple<ArgumentTuple>::type shifted_tuple;
    execute_disect<T, Memfn, shifted_tuple, Args..., arg_type>::exec(objectptr,
                                                                     server,
                                                                     fn,
                                                                     msg,
                                                                     response,
                                                                     args...,
                                                                     arg);
  }
};


/**
 * \internal
 * \ingroup cppipc
 * Overload of execute_disect when the tuple list is empty.
 * Here we just forward the call to the function.
 */
template <typename T, typename Memfn, typename... Args>
struct execute_disect<T, Memfn, std::tuple<>, Args...> {
  static void exec(T* objectptr,
                   comm_server* server,
                   Memfn fn,
                   turi::iarchive& msg,
                   turi::oarchive& response,
                   Args&... args) {
    typedef typename member_function_return_type<Memfn>::type return_type;
    exec_and_serialize_response<return_type,
        T,
        Memfn,
        Args...>::exec(objectptr, server, fn, response, args...);
  }
};



/**
 * \internal
 * \ingroup cppipc
 * A wrapper around the execute_disect call structs.
 * Achieves the effect of call the member function pointer using the
 * objectptr as "this", and extracting the remaining arguments from the input
 * archive. The result will be written to the output archive.
 *
 */
template <typename T, typename Memfn>
struct execute_disect_call {
  static void exec(T* objectptr,
                   comm_server* server,
                   Memfn fn,
                   turi::iarchive& msg,
                   turi::oarchive& response) {
    typedef typename boost::remove_member_pointer<Memfn>::type fntype;
    typedef typename function_args_to_tuple<fntype>::type tuple;

    execute_disect<T, Memfn, tuple>::exec(objectptr, server, fn, msg, response);
  }
};
} // namespace detail



/**
 * \internal
 * \ingroup cppipc
 * An subclass of the dispatch function which specializes around an object type
 * and a member function type. To create an instance of the dispatch_impl
 * use the create_dispatch function.
 */
template <typename T, typename MemFn>
struct dispatch_impl: public dispatch {
  MemFn fn;
  dispatch_impl(MemFn fn): fn(fn) { }

  void execute(void* objectptr,
               comm_server* server,
               turi::iarchive& msg,
               turi::oarchive& response) {
    detail::set_deserializer_to_server(server);
    detail::execute_disect_call<T, MemFn>::exec((T*)objectptr, server, fn, msg, response);
  }
};

/**
 * \internal
 * \ingroup cppipc
 * Creates a dispatch object which wraps a given member function.
 */
template <typename MemFn>
dispatch* create_dispatch(MemFn  memfn) {
  typedef typename boost::mpl::at_c<boost::function_types::parameter_types<MemFn>,0>::type Tref;
  typedef typename std::decay<Tref>::type T;
  return new dispatch_impl<T, MemFn>(memfn);
}
} // cppipc
#endif
