/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <zookeeper_util/key_value.hpp>
namespace libfault {

extern std::string get_zk_objectkey_name(std::string objectkey, size_t nrep);
extern std::string get_publish_key(std::string objectkey);

extern bool master_election(turi::zookeeper_util::key_value* zk_keyval,
                     std::string objectkey);

extern bool replica_election(turi::zookeeper_util::key_value* zk_keyval,
                     std::string objectkey,
                     size_t replicaid);

} // namespace
