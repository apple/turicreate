/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_QUERY_OBJECT_SERVER_PROCESS_HPP
#define FAULT_QUERY_OBJECT_SERVER_PROCESS_HPP
#include <string>
#include <boost/function.hpp>
#include <fault/query_object.hpp>
#include <fault/query_object_create_flags.hpp>
namespace libfault {
typedef boost::function<query_object*(std::string objectkey,
                                      std::vector<std::string> zk_hosts,
                                      std::string zk_prefix,
                                      uint64_t create_flags)>  query_object_factory_type;

int query_main(int argc, char** argv, const query_object_factory_type& factory);

} // namespace;
#endif

