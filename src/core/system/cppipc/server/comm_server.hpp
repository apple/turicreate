/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_SERVER_COMM_SERVER_HPP
#define CPPIPC_SERVER_COMM_SERVER_HPP
#include <string>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <algorithm>
#include <iterator>
#include <atomic>
#include <core/parallel/mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <core/system/nanosockets/socket_errors.hpp>
#include <core/system/nanosockets/async_reply_socket.hpp>
#include <core/system/nanosockets/publish_socket.hpp>
#include <core/system/cppipc/common/status_types.hpp>
#include <core/system/cppipc/server/dispatch.hpp>
#include <core/system/cppipc/server/cancel_ops.hpp>


namespace cppipc {
namespace nanosockets = turi::nanosockets;
struct reply_message;
struct call_message;

// some annoying forward declarations I need to get by some circular references
class object_factory_impl;
namespace detail {
  template <typename RetType, typename T, typename MemFn, typename... Args>
  struct exec_and_serialize_response;
}

/**
 * \ingroup cppipc
 * The comm_server manages the server side of the communication interface.
 *
 * The comm_server manages the serving of objects. It listens on a bind address
 * (defaults to an arbitrary TCP port, but an alternate_bind_address can be
 * provided), and registers its existance in zookeeper. Clients can reach the
 * server by associating with the same keys in zookeeper.
 *
 * The comm_server manages a list of member function pointers and strings they
 * map to, as well as a complete list of all served objects.
 *
 * Basic Utilization
 * -----------------
 * To create a object which can be served by remote machines,
 * first create a base interface class which describes the functions to be
 * exported using the registration macros  \ref REGISTRATION_BEGIN
 * \ref REGISTRATION_END \ref REGISTER, or the magic macros
 * \ref GENERATE_INTERFACE and \ref GENERATE_INTERFACE_AND_PROXY.
 * The actual server side implementation of the object then inherits from
 * this interface, implementing all the functions.
 *
 * For instance, I may have a base interface class called "file_write_base", and
 * an implementation called "file_write_impl".
 *
 * \code
 * class file_write_base {
 *  public:
 *   virtual int open(std::string s) = 0;
 *   virtual void write(std::string s) = 0;
 *   virtual void close() = 0;
 *   virtual ~file_write_base() {}
 *
 *   REGISTRATION_BEGIN(file_write_base)
 *   REGISTER(file_write_base::open)
 *   REGISTER(file_write_base::write)
 *   REGISTER(file_write_base::close)
 *   REGISTRATION_END
 * };
 *
 * class file_write_impl: public file_write_base {
 *   file_write_impl(); // regular constructor
 *
 *   explicit file_write_impl(std::string f) {  // open a file on construction
 *     open(f);
 *   }
 *   // ... other implementation details omitted ...
 * };
 * \endcode
 *
 * To make this class available on the server side, we must tell the server
 * how to construct an instance of this object by registering a type with the
 * server, and providing a lambda function returning a pointer to an
 * implementation.
 * \code
 * int main() {
 *   ...
 *   comm_server server(...);
 *   server.register_type<file_write_base>([](){ return new file_write_impl;});
 *   ...
 *   server.start();
 *   ...
 * }
 * \endcode
 * Here we use the trivial constructor, but more generally we can provide
 * arbitrarily interesting constructors in the lambda. For instance, here
 * we use the alternate constructor in file_write_impl.
 * \code
 * int main() {
 *   ...
 *   comm_server server(...);
 *   server.register_type<file_write_base>([](){ return new file_write_impl("log.txt");});
 *   ...
 *   server.start();
 *   ...
 * }
 * \endcode
 *
 * Once the server is started, the client will have the ability to create
 * proxy objects which in turn create matching objects on the server.
 *
 * It is important that each base class only describes exactly one
 * implementation. i.e. register_type<T> should be used only once for any T.
 *
 * To see how this code is used, see the comm_client documentation.
 *
 * Implementation Details
 * ----------------------
 * There is a special "root object" which manages all "special" tasks that
 * operate on the comm_server itself. This root object always has object ID 0
 * and is the object_factory_base. This is implemented on the server side by
 * object_factory_impl, and on the client side as object_factory_proxy.
 *
 * The object_factory_impl manages the construction of new object types.
 *
 * Interface Modification Safety
 * -----------------------------
 * The internal protocol is designed to be robust against changes in interfaces.
 * i.e. if new functions are added, and the server is recompiled, all previous
 * client builds will still work. Similarly, if new functions are added and the
 * client is recompiled, the new client will still work with old servers as long
 * as the new functions are not called.
 *
 */
class EXPORT comm_server {
 private:

  friend class object_factory_impl;

  // true if start was called
  bool started;

  nanosockets::async_reply_socket* object_socket;
  nanosockets::async_reply_socket* control_socket;
  nanosockets::publish_socket* publishsock;

  /// Internal callback for messages received from zeromq
  bool callback(nanosockets::zmq_msg_vector& recv, nanosockets::zmq_msg_vector& reply);

  std::map<std::string, dispatch*> dispatch_map;
  boost::mutex registered_object_lock;
  std::map<size_t, std::shared_ptr<void>> registered_objects;
  std::map<void*, size_t> inv_registered_objects; // A reverse map of the registered objects
  object_factory_impl* object_factory;

  /**
   * Object IDs are generated by using Knuth's LCG
   * s' = s * 6364136223846793005 + 1442695040888963407 % (2^64)
   * This guarantees a complete period of 2^64.
   * We just need a good initialization.
   */
  size_t lcg_seed;

  bool comm_server_debug_mode = false;


  /**
   * Registers a mapping from a type name to a constructor call which returns
   * a pointer to an instance of an object of the specified typename.
   */
  void register_constructor(std::string type_name,
                           std::function<std::shared_ptr<void>()> constructor_call);

  /**
   * Returns the next freely available object ID
   */
  size_t get_next_object_id();

  /**
   * Overload of register_object in the event that the object is already
   * wrapped in a deleting wrapper. This function should not be used directly.
   * If the object already exists, the existing ID is returned
   */
  inline size_t register_object(std::shared_ptr<void> object) {
    boost::lock_guard<boost::mutex> guard(registered_object_lock);
    if (inv_registered_objects.count(object.get())) {
      return inv_registered_objects.at(object.get());
    }
    size_t id = get_next_object_id();
    registered_objects.insert({id, object});
    inv_registered_objects.insert({object.get(), id});
    return id;
  }

  struct object_map_key_cmp {
    bool operator()(size_t i,
        const std::pair<size_t, std::shared_ptr<void>>& map_elem) const {
      return i < map_elem.first;
    }

    bool operator()(const std::pair<size_t,std::shared_ptr<void>>& map_elem,
                    size_t i) const {
      return map_elem.first < i;
    }
  };

 public:

  /**
   * Constructs a comm server which uses remote communication
   * via zookeeper/zeromq.
   * \param zkhosts The zookeeper hosts to connect to. May be empty. If empty,
   *                the "alternate_bind_address" parameter must be a zeromq
   *                endpoint address to bind to.
   * \param name The key name to wait for connections on. All remotes connect
   *             to this server on this name. If zkhosts is empty, this is
   *             ignored.
   * \param alternate_bind_address The communication defaults to using an
   *        arbitrary TCP port. This can be changed to any URI format supported
   *        by zeroMQ.
   * \param alternate_publish_address Only valid if zkhosts is empty.
   *                    The address to publish server statuses on. If zookeeper
   *                    is not used, all remotes should connect to this address
   *                    to get server status. If not provided, one is
   *                    generated automatically.
   */
  comm_server(std::vector<std::string> zkhosts,
              std::string name,
              std::string alternate_bind_address="",
              std::string alternate_control_address="",
              std::string alternate_publish_address="",
              std::string secret_key="");
  /// Destructor. Stops receiving messages, and closes all communication
  ~comm_server();


  /** Start receiving messages. Message processing occurs on a seperate
   * thread, so this function returns immediately..
   */
  void start();

  /**
   * Stops receiving messages. Has no effect if start() was not called
   * before this.
   */
  void stop();

  /**
   * Gets the address we are bound on
   */
  std::string get_bound_address();

  /**
   * Gets the address where we receive control messages
   */
  std::string get_control_address();

  /**
   * Gets the address on which you can subscribe to for status updates.
   */
  std::string get_status_address();

  /**
   * Gets the zeromq context.. Deprecated. Returns NULL.
   */
  void* get_zmq_context();

  /**
   * Publishes a message of the form "[status_type]: [message]".
   * Since the client can filter messages, it is important to have a
   * small set of possible status_type strings. For the purposes of the
   * comm_server, we define the following:
   *
   * \li \b COMM_SERVER_INFO Used for Comm Server informational messages.
   * \li \b COMM_SERVER_ERROR Used for Comm Server error messages.
   *
   * These status strings are defined in \ref core/system/cppipc/common/status_types.hpp
   */
  void report_status(std::string status_type, std::string message);

  /**
   * Deletes an object of object ID objectid.
   */
  inline void delete_object(size_t objectid) {
    boost::lock_guard<boost::mutex> guard(registered_object_lock);
   if(registered_objects.count(objectid) != 1) {
     logstream(LOG_DEBUG) << "Deleting already deleted object " << objectid << std::endl;
   }
   inv_registered_objects.erase(registered_objects[objectid].get());
   logstream(LOG_DEBUG) << "Deleting Object " << objectid << std::endl;
   registered_objects.erase(objectid);
  }

  inline size_t num_registered_objects() {
    boost::lock_guard<boost::mutex> guard(registered_object_lock);
    return registered_objects.size();
  }

  /**
   * Registers a type to be managed by the comm_server. After registration of
   * this type, remote machines will be able to create instances of this object
   * via the comm_client's make_object function call.
   */
  template <typename T>
  void register_type(std::function<T*()> constructor_call) {
   T::__register__(*this);
   register_constructor(T::__get_type_name__(),
                        [=]()->std::shared_ptr<void> {
                          return std::static_pointer_cast<void>(std::shared_ptr<T>(constructor_call()));
                        }
                       );
  }


  /**
   * Registers a type to be managed by the comm_server. After registration of
   * this type, remote machines will be able to create instances of this object
   * via the comm_client's make_object function call.
   */
  template <typename T>
  void register_type(std::function<std::shared_ptr<T>()> constructor_call) {
   T::__register__(*this);
   register_constructor(T::__get_type_name__(),
                        [=]()->std::shared_ptr<void> {
                          return std::static_pointer_cast<void>(constructor_call());
                        }
                       );
  }



  /**
   * Registers an object to be managed by the comm server, returning the new
   * object ID. If the object already exists, the existing ID is returned.
   */
  template <typename T>
  size_t register_object(std::shared_ptr<T> object) {
    boost::lock_guard<boost::mutex> guard(registered_object_lock);
    if (inv_registered_objects.count(object.get())) {
      return inv_registered_objects.at(object.get());
    }
    size_t id = get_next_object_id();
    logstream(LOG_DEBUG) << "Registering Object " << id << std::endl;
    registered_objects.insert({id, std::static_pointer_cast<void>(object)});
    inv_registered_objects.insert({object.get(), id});
    return id;
  }

  /**
   * Returns an object ID of the object has been previously registere.d
   * Returns (size_t)(-1) otherwise.
   */
  inline size_t find_object(void* object) {
    boost::lock_guard<boost::mutex> guard(registered_object_lock);
    if (inv_registered_objects.count((void*)object)) {
      return inv_registered_objects.at((void*)object);
    } else {
      return (size_t)(-1);
    }
  }

  /**
   * Returns a pointer to the object with a given object ID.
   * Returns NULL on failure.
   */
  inline std::shared_ptr<void> get_object(size_t objectid) {
    boost::lock_guard<boost::mutex> guard(registered_object_lock);
    if (registered_objects.count(objectid) == 1) {
      return registered_objects[objectid];
    } else {
      return nullptr;
    }
  }

  void delete_unused_objects(std::vector<size_t> object_ids,
                             bool active_list);

  /**
   * \internal
   * Registers a member function pointer. Do not use directly. Used by the
   * REGISTER macros to allow the comm_server to maintain the mapping of
   * member function pointers to names.
   */
  template <typename MemFn>
  void register_function(MemFn fn, std::string function_name);


  template <typename RetType, typename T, typename MemFn, typename... Args>
  friend struct detail::exec_and_serialize_response;
};

} // cppipc


#include <core/system/cppipc/server/dispatch_impl.hpp>

namespace cppipc {
template <typename MemFn>
void comm_server::register_function(MemFn fn, std::string function_name) {
  if (dispatch_map.count(function_name) == 0) {
    dispatch_map[function_name] = create_dispatch(fn);
    logstream(LOG_EMPH) << "Registering function " << function_name << "\n";
  }
}
};
#endif
