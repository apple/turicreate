/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <pch/pch.hpp>

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <zookeeper_util/key_value.hpp>
#include <fault/query_object_server_master.hpp>
#include <fault/query_object_server_replica.hpp>
#include <fault/query_object_server_process.hpp>
#include <fault/query_object_create_flags.hpp>
namespace libfault {


int query_main(int argc, char** argv,
               const query_object_factory_type& factory) {

  void* zmq_ctx = zmq_ctx_new();

  if (argc < 4) {
    std::cout << "Usage: " << argv[0]
              << " [comma-seperated Zookeeper machines] [prefix] [objectkey:replicaid]\n";
    return 0;
  }
  // parse zkhosts
  std::vector<std::string> zkhosts;
  boost::split(zkhosts, argv[1],
               boost::is_any_of(", "), boost::token_compress_on);
  // prefix
  std::string prefix = argv[2];

  // object and key
  std::vector<std::string> object_and_key_split;
  boost::split(object_and_key_split, argv[3],
               boost::is_any_of(": "), boost::token_compress_on);
  if (object_and_key_split.size() != 2) {
    std::cout << "Invalid object key name. Expected objectkey:replicaid\n";
    return 0;
  }
  std::string objectkey = object_and_key_split[0];
  size_t replicaid = atoi(object_and_key_split[1].c_str());

  // create the zookeeper instance
  turi::zookeeper_util::key_value keyval(zkhosts, prefix, "");


  if (replicaid == 0) {
    std::cout << "Creating Master : " << objectkey << "\n";
    // if we fail the master election, fail immediately
    if (master_election(&keyval, objectkey) == false) return false;

    // create object
    uint64_t flags = 0;
    if (replicaid == 0) flags |= QUERY_OBJECT_CREATE_MASTER;
    else flags |= QUERY_OBJECT_CREATE_REPLICA;
    query_object* qobj = factory(objectkey, zkhosts, prefix, flags);

    query_object_server_master master(zmq_ctx, &keyval, objectkey, qobj);
    master.start();
  } else {
    std::cout << "Creating Replica: " << objectkey << ":" << replicaid << "\n";
    // replica
    if (replica_election(&keyval, objectkey, replicaid) == false) return false;

    // create object
    uint64_t flags = 0;
    if (replicaid == 0) flags |= QUERY_OBJECT_CREATE_MASTER;
    else flags |= QUERY_OBJECT_CREATE_REPLICA;
    query_object* qobj = factory(objectkey, zkhosts, prefix, flags);

    int ret = 0;
    {
      query_object_server_replica replica(zmq_ctx, &keyval, objectkey,
                                        qobj, replicaid);
      ret = replica.start();
    }
    if (ret > 0) {
      std::cout << "Master lost. Promoting... ";
      // promote
      query_object_server_master master(zmq_ctx, &keyval, objectkey, qobj);
      master.start();
    }
  }
  return 0;
}


} // namespace
