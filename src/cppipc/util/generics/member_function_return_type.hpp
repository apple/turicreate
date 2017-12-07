/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_UTIL_GENERICS_MEMBER_FUNCTION_RETURN_TYPE_HPP
#define CPPIPC_UTIL_GENERICS_MEMBER_FUNCTION_RETURN_TYPE_HPP

namespace cppipc {
namespace detail {


template <typename MemFn>
struct member_function_return_type {
  typedef typename boost::remove_member_pointer<MemFn>::type fntype;
  typedef typename boost::function_traits<fntype>::result_type type;
};

} // detail
} // cppipc

#endif
