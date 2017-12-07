/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <zookeeper_util/key_value.hpp>
namespace libfault {
std::string get_zk_objectkey_name(std::string objectkey, size_t nrep) {
  if (nrep == 0) return objectkey;
  else {
    char c[32];
    sprintf(c, "%ld", nrep);
    return objectkey + "." + c;
  }
}

std::string get_publish_key(std::string objectkey) {
  return objectkey + ".PUB";
}


bool master_election(turi::zookeeper_util::key_value* zk_keyval,
                     std::string objectkey) {
  // the actual election process. Simply try to insert the object
  std::cout << "Joining master election: " << objectkey << ":0\n";
  // If we fail to insert. we lose the election
  return zk_keyval->insert(objectkey, "");
}

bool replica_election(turi::zookeeper_util::key_value* zk_keyval,
                     std::string objectkey,
                     size_t replicaid) {
  // the actual election process. Simply try to insert the object
  std::cout << "Joining replica election: " << objectkey << ":" << replicaid << "\n";
  // If we fail to insert. we lose the election
  return zk_keyval->insert(get_zk_objectkey_name(objectkey, replicaid), "");
}



} // namespace

