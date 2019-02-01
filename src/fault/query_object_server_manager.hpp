/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef QUERY_OBJECT_SERVER_MANAGER_HPP
#define QUERY_OBJECT_SERVER_MANAGER_HPP
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <boost/function.hpp>
#include <zookeeper_util/key_value.hpp>
#include <fault/query_object.hpp>
#include <fault/query_object_create_flags.hpp>
#include <fault/sockets/reply_socket.hpp>
#include <fault/sockets/subscribe_socket.hpp>
#include <fault/sockets/publish_socket.hpp>
#include <boost/thread/mutex.hpp>
#include <fault/sockets/socket_receive_pollset.hpp>
namespace libfault {

/**
 * \ingroup fault
 * Manages a collection of fault tolerant replicated query object.
 */
class query_object_server_manager {
 public:
  /**
   * Constructor. Creates a query object server,
   * but does not activate it, or register it with zookeeper (yet).
   * After construction, the initialization process is
   *  - register_zookeeper()
   *  - set_all_master_object_keys()
   *  - start()
   *
   * The query_object_server will automatically infer master nodes to construct
   * and replicas to construct.
   * \param zmq_ctx ZeroMQ Context
   * \param replica_count The number of relicas per object. Defaults to 2
   * \param object_cap The maximum number of objects managed by this server.
   *                   Defaults to 32
   */
  query_object_server_manager(std::string program,
                              size_t replica_count = 2, size_t objectcap = 32);

  ~query_object_server_manager();

  bool register_zookeeper(std::vector<std::string> zkhosts,
                          std::string prefix);

  /**
   * Provides the class a list of all possible object keys.
   * The object keys must be a printable ASCII string. See the
   * zookeeper documentation for further details. In addition, the
   * characters '.' and '-' are not permitted.
   */
  void set_all_object_keys(const std::vector<std::string>& master_space);
  /**
   * Constructs all available objects up to capacity
   *
   * \param max_masters The maximum number of master objects to auto-create.
   *                    This is a "soft" limit, in that it only affects
   *                    initialization. If it is necessary to create more masters
   *                    due to machine failures, it will do so. Defaults to
   *                    infinity.
   */
  void start(size_t max_masters = (size_t)(-1));

  /**
   * destroys all instantiated objects
   */
  void stop();

  void print_all_object_names();

  bool some_replica_exists(std::string objectkey);

  /**
   * Stop managing a particular object.
   * Returns true on success, false on failure
   */
  bool stop_managing_object(std::string objectkey);

  struct managed_object {
    std::string objectkey; // the object key associated with this object
    size_t replicaid;  // the replica ID of this object. If 0, this is a master.
                       // Otherwise it is a replica
    pid_t pid;
    int outfd;
  };

  std::vector<std::string> build_arguments();

  // do not call directly
  void cleanup(pid_t pid);
private:
  std::string program;
  size_t objectcap;
  size_t replica_count;

  size_t initial_max_masters;
  size_t auto_created_masters_count;

  std::vector<std::string> masterspace; // master object names
  std::set<std::string> managed_keys;
  std::set<std::string> managed_zkkeys;
  turi::zookeeper_util::key_value* zk_keyval;
  std::vector<std::string> zk_hosts;
  std::string zk_prefix;
  size_t zk_kv_callback_id;

  std::vector<managed_object> objects;

  boost::mutex lock;

  sigset_t sigchldset;
  sig_t prev_sighandler;

  void spawn_object(std::string objectkey, size_t replicaid);

  void keyval_change(turi::zookeeper_util::key_value* unused,
                     const std::vector<std::string>& newkeys,
                     const std::vector<std::string>& deletedkeys,
                     const std::vector<std::string>& modifiedkeys);

  void check_managed_objects(size_t max_masters = (size_t)(-1));

  void lock_and_block_signal();
  void unlock_and_unblock_signal();
};

} // namespace libfault

#endif
