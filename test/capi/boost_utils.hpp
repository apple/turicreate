/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef CAPI_BOOST_TEST_UTILS
#define CAPI_BOOST_TEST_UTILS

// TODO - not sure why this is necessary, but we get symbol undefined errors
// unless we define this explicitly. The errors are of the form:
//
// print_helper.hpp:216: undefined reference to `boost::test_tools::tt_detail::print_log_value<decltype(nullptr)>::operator()(std::ostream&, decltype(nullptr))'
//
void boost::test_tools::tt_detail::print_log_value<decltype(nullptr)>::operator()(std::ostream& os, decltype(nullptr)) {
  os << "nullptr";
}

#endif
