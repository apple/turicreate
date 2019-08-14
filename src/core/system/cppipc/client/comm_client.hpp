/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_SERVER_COMM_CLIENT_HPP
#define CPPIPC_SERVER_COMM_CLIENT_HPP
#include <map>
#include <memory>
#include <chrono>
#include <core/parallel/atomic.hpp>
#include <core/logging/logger.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_types.hpp>
#include <core/system/nanosockets/socket_errors.hpp>
#include <core/system/nanosockets/async_request_socket.hpp>
#include <core/system/nanosockets/subscribe_socket.hpp>
#include <core/system/cppipc/common/message_types.hpp>
#include <core/system/cppipc/common/status_types.hpp>
#include <core/system/cppipc/client/issue.hpp>
#include <core/system/cppipc/common/ipc_deserializer.hpp>
#include <core/system/cppipc/client/console_cancel_handler.hpp>
#include <core/system/exceptions/error_types.hpp>
#include <cctype>
#include <atomic>

namespace cppipc {
namespace nanosockets = turi::nanosockets;

std::atomic<size_t>& get_running_command();
std::atomic<size_t>& get_cancelled_command();

class comm_client;

namespace detail {
/**
 * \ingroup cppipc
 * \internal
 * Internal utility function.
 * Deserialized an object of a specific type from a reply message,
 * and returns the result, clearing the message object.
 * Works correctly for void types.
 */
template <typename RetType, bool is_proxied_object>
struct deserialize_return_and_clear {
  static RetType exec(comm_client& client, reply_message& msg) {
    turi::iarchive iarc(msg.body, msg.bodylen);
    RetType ret = RetType();
    iarc >> ret;
    msg.clear();
    return ret;
  }
};


template <>
struct deserialize_return_and_clear<void, false> {
  static void exec(comm_client& client, reply_message& msg) {
    msg.clear();
    return;
  }
};


} // namespace detail

class object_factory_proxy;

/**
 * \ingroup cppipc
 * The client side of the IPC communication system.
 *
 * The comm_client manages the serialization, and the calling of functions
 * on remote machines. The comm_client and the comm_server reaches each other
 * through the use of zookeeper. If both client and server connect to the same
 * zookeeper host, and on construction are provided the same "name", they are
 * connected.
 *
 * The comm_client provides communication capability for the \ref object_proxy
 * objects. Here, we will go through an example of proxying the file_write class
 * described in the \ref comm_server documentation.
 *
 * Basic Utilization
 * -----------------
 * Given a base class file_write_base, which is implemented on the server side,
 * we can construct on the client side, an \ref object_proxy object which
 * creates and binds to an implementation of file_write_base on the server side,
 * and allows function calls across the network.
 *
 * We first repeat the description for the base class here:
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
 * \endcode
 *
 * We can create an object_proxy by templating over the base class, and
 * providing the comm_client object in the constructor.
 * \code
 * object_proxy<file_write_base> proxy(client);
 * \endcode
 * This will create on the remote machine, an instance of the file_write_impl
 * object, and the object_proxy class provides the capability to call functions
 * on the remote object. For instance, to call the "open" function, followed
 * by some "writes" and "close".
 * \code
 * int ret = proxy.call(&file_write_base::open, "log.txt");
 * proxy.call(&file_write_base::write, "hello");
 * proxy.call(&file_write_base::write, "world");
 * proxy.call(&file_write_base::close);
 * \endcode
 * The return type of the proxy.call function will match the return type of the
 * member function pointer provided. For instance, &file_write_base::open
 * returns an integer, and the result is forwarded across the network and
 * returned.
 *
 * On destruction of the proxy object, the remote object is also deleted.
 *
 * Wrapping the Proxy Object
 * -------------------------
 * It might be convenient in many ways to wrap the object_proxy in a way to
 * make it easy to use. This is a convenient pattern that is very useful.
 *
 * For instance, a proxy wrapper for the file_write_base object might look like:
 * \code
 * class file_write_proxy: public file_write_base {
 *  object_proxy<file_write_base> proxy;
 *  public:
 *   file_write_proxy(comm_client& comm): proxy(comm) { }
 *   int open(std::string s) {
 *     return proxy.call(&file_write_base::open, s);
 *   }
 *   void write(std::string s) {
 *     return proxy.call(&file_write_base::write , s);
 *   }
 *   void close() {
 *     return proxy.call(&file_write_base::close);
 *   }
 * };
 * \endcode
 *
 * Preprocessor Magic
 * ------------------
 * To facilitate the creation of base interfaces, calling REGISTER macros
 * appropriately, and implementing the proxy wrapper, we provide the
 * \ref GENERATE_INTERFACE, \ref GENERATE_PROXY and
 * \ref GENERATE_INTERFACE_AND_PROXY magic macros.
 *
 * For instance, to generate the proxy and the base class for the above
 * file_write_base object, we can write
 *
 * \code
 * GENERATE_INTERFACE_AND_PROXY(file_write_base, file_write_proxy,
 *                              (int, open, (std::string))
 *                              (void, write, (std::string))
 *                              (void, close, )
 *                              )
 * \endcode
 *
 * This is the recommended way to create proxy and base objects since this
 * allows a collection of interesting functionality to be injected.
 * For instance, this will allow functions to take base pointers as arguments
 * and return base pointers (for instance file_write_base*). On the client side,
 * proxy objects are serialized in a way so that the server side uses the
 * matching implementation instance. When an object is returned, the object
 * is registered on the server side and converted to a new proxy object
 * on the client side. a
 *
 * Implementation Details
 * ----------------------
 * Many other details regarding safety when interfaces, or interface argument
 * type modifications are described in the \ref comm_server documentation.
 *
 * The comm client internally maintains the complete mapping of all member
 * function pointers to strings. The object_proxy class then has the simple
 * task of just maintaining the object_ids: i.e. what remote object does it
 * connect to.
 *
 * There is a special "root object" which manages all "special" tasks that
 * operate on the comm_server itself. This root object always has object ID 0
 * and is the object_factory_base. This is implemented on the server side by
 * object_factory_impl, and on the client side as object_factory_proxy.
 * The comm_client exposes the object_factory functionality as member functions
 * in the comm_client itself. These are make_object(), ping() and
 * delete_object()
 */
class EXPORT comm_client {
 private:
  nanosockets::async_request_socket object_socket;
  // This is a pointer because the endpoint address must be received from the
  // server, so it cannot be constructed in the constructor
  nanosockets::async_request_socket *control_socket = NULL;
  nanosockets::subscribe_socket subscribesock;
  turi::atomic<size_t> m_command_id;

  // a map of a string representation of the function pointer
  // to the name. Why a string representation of a function pointer you ask.
  // That is because a member function pointer is not always 8 bytes long.
  // It can be 16 bytes long. Without any other knowledge, it is safer to keep it
  // as a variable length object.
  std::map<std::string, std::string> memfn_pointer_to_string;

  // status callbacks. Pairs of registered prefixes to the callback function
  std::vector<std::pair<std::string,
      std::function<void(std::string)> > > prefix_to_status_callback;

  boost::mutex status_callback_lock;

  boost::mutex ref_count_lock;
  // a map of object IDs to the number of references they hold
  std::map<size_t, size_t> object_ref_count;

  /**
   * Issue a call to the remote machine.
   * As a side effect, the call and reply message structures will be cleared.
   * Returns 0 on success and a system error code on communication failure.
   * Note that the reply_message may contain other cppipc errors.
   */
  int internal_call(call_message& call, reply_message& reply, bool control=false);

  int internal_call_impl(call_message& call,
                         nanosockets::zmq_msg_vector& ret,
                         bool control,
                         size_t timeout = 0);

  /** Checks is the pid set with set_server_alive_watch_pid is running.
   * Sets server_running to false if pid is no longer running.
   */
  void poll_server_pid_is_running();

  /**
   * Convert the auxiliary addresses we get back from the server to a real IP
   * address if needed.
   *
   * This is only used for control and publish addresses.
   */
  std::string convert_generic_address_to_specific(std::string aux_addr);

  /**
   * The root object (always object 0)
   */
  object_factory_proxy* object_factory;

  /**
   * This thread repeatedly pings the server every 3 seconds, setting
   * and clearing the flag server_alive as appropriate.
   */
  boost::thread* ping_thread;

  /**
   * The lock / cv pair around the ping_thread_done value
   */
  boost::mutex ping_mutex;
  boost::condition_variable ping_cond;

  /**
   * Sets to true when the ping thread is done
   */
  volatile bool ping_thread_done = false;

  /**
   * Server alive is true if the server is reachable some time in the
   * last 3 pings. Server_alive is true on startup.
   */
  volatile bool server_alive = true;

  /**
   * True if the socket is closed
   */
  bool socket_closed = false;

  /**
   * The number of pings which have failed consecutively.
   */
  volatile size_t ping_failure_count = 0;

  size_t num_tolerable_ping_failures = 10;

  /**
   * Callback issued when server reports status
   */
  void subscribe_callback(const std::string& msg);

  /**
   * The point in time that must have passed for us to sync our tracked objects
   * with the server.
   */
  std::chrono::steady_clock::time_point object_sync_point;

  /**
   * If set, the control address to use.
   */
  std::string alternate_control_address;

  /**
   * If set, the publish address to use.
   */
  std::string alternate_publish_address;

  /**
   * Set to true when the client is started. False otherwise.
   */
  bool started = false;

  /**
   * The name this client was told to connect to.
   */
  std::string endpoint_name;

  /**
   * The signal handler that was in effect before this client was established
   */
  bool cancel_handling_enabled = false;

  /**
   * The pid to watch to test if the server is alive
   */
  int32_t server_alive_watch_pid = 0;

 public:

  /**
   * Constructs a comm client which uses remote communication
   * via zookeeper/zeromq. The client may find the remote server via either
   * zookeeper (in which case zkhosts must be a list of zookeeper servers, and
   * name must be a unique key value), or you can provide the address
   * explicitly. Note that if a server is listening via zookeeper, the client
   * MUST connect via zookeeper; providing the server's actual tcp bind address
   * will not work. And similarly if the server is listening on an explicit
   * zeromq endpoint address and not using zookeeper, the client must connect
   * directly also without using zookeeper.
   *
   * After construction, authentication methods can be added then \ref start()
   * must be called to initiate the connection.
   *
   * \param zkhosts The zookeeper hosts to connect to. May be empty. If empty,
   *                the "name" parameter must be a zeromq endpoint address to
   *                bind to.
   * \param name The key name to connect to. This must match the "name" argument
   *             on construction of the remote's comm_server. If zkhosts is
   *             empty, this must be a valid zeromq endpoint address.
   * \param num_tolerable_ping_failures The number of allowable consecutive ping
   *        failures before the server is considered dead.
   * \param alternate_publish_address This should match the
   *        "alternate_publish_address" argument on construction of the
   *        remote's comm_server. If zkhosts is empty, this must be a valid
   *        zeromq endpoint address. This can be empty, in which case the client
   *        will ask the server for the appropriate address. It is recommended
   *        that this is not specified.
   */
  comm_client(std::vector<std::string> zkhosts,
              std::string name,
              size_t num_tolerable_ping_failures = (size_t)(-1),
              std::string alternate_control_address="",
              std::string alternate_publish_address="",
              const std::string public_key = "",
              const std::string secret_key = "",
              const std::string server_public_key = "",
              bool ops_interruptible = false);

  /**
   * Constructs an inproc comm_client. The inproc comm_client
   * and comm_server are required to have the same zmq_ctx.
   *
   * \param name The inproc socket address, must start with inproc://
   */
  comm_client(std::string name, void* zmq_ctx);

  /**
   * Sets a pid to watch. If this pid goes away, server is considered dead.
   * This is a more robust way compared to "pings" for local interprocess
   * communication. Set to 0 to disable.
   */
  void set_server_alive_watch_pid(int32_t pid);

  /**
   * Initialize the comm_client, called right inside the constructor.
   */
  void init(bool ops_interruptible = false);

  /**
   * Initializes connections with the servers
   * Must be called prior to creation of any client objects.
   * Returns reply_status::OK on success, and an error code failure.
   * Failure could be caused by an inability to connect to the server, or
   * could also be due to authentication errors.
   */
  reply_status start();

  /**
   * Destructor. Calls stop if not already called
   */
  ~comm_client();

  /**
   * Stops the comm client object. Closes all open sockets
   */
  void stop();

  /**
   * Creates an object of a given type on the remote machine.
   * Returns an object ID. If return value is (-1), this is a failure.
   *
   * \note This call redirects to the object_factory_proxy
   */
  size_t make_object(std::string object_type_name);

  /**
   * Ping test. Sends a string to the remote system,
   * and replies with the same string.
   *
   * \note This call redirects to the object_factory_proxy
   */
  std::string ping(std::string);

  /**
   * Delete object. Deletes the object with ID objectid on the remote machine.
   *
   * \note This call redirects to the object_factory_proxy
   */
  void delete_object(size_t objectid);

  /**
   * Functions for manipulating local reference counting data structure
   */
  size_t incr_ref_count(size_t object_id);

  size_t decr_ref_count(size_t object_id);

  size_t get_ref_count(size_t object_id);

  /**
   * This thread is used to serve the status callbacks.
   * This prevents status callback locks from blocking the server
   */
  boost::thread* status_callback_thread = NULL;
  /**
   * The lock / cv pair around the ping_thread_done value
   */
  boost::mutex status_buffer_mutex;
  boost::condition_variable status_buffer_cond;
  std::vector<std::string> status_buffer;
  bool status_callback_thread_done = false;

  /** The function which implements the thread which
   * issues the messages to
   * the status callback handlers.
   */
  void status_callback_thread_function();

  /**
   * Terminates the thread which calls the callback handlers. Unprocessed
   * messages are dropped.
   */
  void stop_status_callback_thread();

  /**
   * Starts the status callback thread if not already started.
   */
  void start_status_callback_thread();

  /**
   * Adds a callback for server status messages.
   * The callback will receive all messages matching the specified prefix.
   * For instance:
   * \code
   * client.add_status_watch("A", callback);
   * \endcode
   *
   * will match all the following server messages
   * \code
   * server.report_status("A", "hello world"); // emits A: hello world
   * server.report_status("ABC", "hello again"); // emits ABC: hello again
   * \endcode
   *
   * On the other hand
   * \code
   * client.add_status_watch("A:", callback);
   * \endcode
   * will only match the first.
   *
   * Callbacks should be processed relatively quickly and should be thread
   * safe. Multiple callbacks may be processed simultaneously in different
   * threads. Callback function also should not call \ref add_status_watch
   * or \ref remove_status_watch, or a deadlock may result.
   *
   * If multiple callbacks are registered for exactly the same prefix, only the
   * last callback is recorded.
   *
   * \note The current prefix checking implementation is not fast, and is
   * simply linear in the number of callbacks registered.
   */
  void add_status_watch(std::string watch_prefix,
                           std::function<void(std::string)> callback);

  /**
   * Removes a status callback for a given prefix. Note that the function
   * associated with the prefix may still be called even after this function
   * returns. To ensure complete removal of the function,
   * stop_status_callback_thread() and start_status_callback_thread()
   * must be called.
   */
  void remove_status_watch(std::string watch_prefix);

  /**
   * Clears all status callbacks.
   * Note that status callbacks may still be called even after this function
   * returns. To ensure complete removal of the function,
   * stop_status_callback_thread() and start_status_callback_thread()
   * must be called.
   */
  void clear_status_watch();

  /**
   * Stops the ping thread.
   */
  void stop_ping_thread();

  /**
   * Tries to synchronize the list of tracked objects with the server by
   * sending a list of objects to be deleted.
   * Returns 0 on success, -1 on failure.
   */
  int send_deletion_list(const std::vector<size_t>& object_ids);


  /**
   * \internal
   * Registers a member function which then can be used in the \ref call()
   * function
   */
  template <typename MemFn>
  void register_function(MemFn f, std::string function_string) {
    // It seems like the function pointer itself is insufficient to identify
    // the function. Append the type of the function.
    std::string string_f(reinterpret_cast<const char*>(&f), sizeof(MemFn));
    string_f = string_f + typeid(MemFn).name();
    if (memfn_pointer_to_string.count(string_f) == 0) {
      memfn_pointer_to_string[string_f] = function_string;
      //std::cerr << "Registering function " << function_string << "\n";
    }
  }

  template <typename MemFn>
  void prepare_call_message_structure(size_t objectid, MemFn f, call_message& msg) {
    // try to find the function
    // It seems like the function pointer itself is insufficient to identify
    // the function. Append the type of the function.
    std::string string_f(reinterpret_cast<char*>(&f), sizeof(MemFn));
    string_f = string_f + typeid(MemFn).name();
    if (memfn_pointer_to_string.count(string_f) == 0) {
      throw ipcexception(reply_status::NO_FUNCTION);
    }
    msg.objectid = objectid;
    msg.function_name = memfn_pointer_to_string[string_f];
    // trim the function call printing to stop at the first space
//     std::string trimmed_function_name;
//     std::copy(msg.function_name.begin(),
//               std::find(msg.function_name.begin(), msg.function_name.end(), ' '),
//               std::inserter(trimmed_function_name, trimmed_function_name.end()));
    //std::cerr << "Calling object " << objectid << " function " << trimmed_function_name << "\n";
  }

  /**
   * Calls a remote function returning the result.
   * The return type is the actual return value.
   * May throw an exception of type reply_status on failure.
   *
   * NOTE: ONLY the main thread can call this.  If this becomes untrue, some
   * invariants will be violated (only one thread is allowed to change the
   * currently running command).
   */
  template <typename MemFn, typename... Args>
  typename detail::member_function_return_type<MemFn>::type
  call(size_t objectid, MemFn f, const Args&... args) {
    if (!started) {
      throw ipcexception(reply_status::COMM_FAILURE, 0, "Client not started");
    }
    typedef typename detail::member_function_return_type<MemFn>::type return_type;
    call_message msg;
    prepare_call_message_structure(objectid, f, msg);
    // generate the arguments
    turi::oarchive oarc;
    cppipc::issue(oarc, f, args...);
    /*
     * Complete hack.
     * For whatever reason zeromq's zmq_msg_send and zmq_msg_recv function
     * return the size of the mesage sent in an int. Even though the message
     * size can be size_t.
     * Also, zmq_msg_send/zmq_msg_recv use "-1" return for failure, thus
     * bringing up the issue of integer overflow just "coincidentally" hitting
     * -1 and thus failing terribly terribly.
     *  Solution is simple. Pad the buffer to even.
     */
    if (oarc.off & 1) oarc.write(" ", 1);
    msg.body = oarc.buf;
    msg.bodylen = oarc.off;

    // Set the command id
    // 0 and uint64_t(-1) have special meaning, so don't send those
    size_t command_id = m_command_id.inc();
    auto ret = msg.properties.insert(std::make_pair<std::string, std::string>(
          "command_id", std::to_string(command_id)));
    ASSERT_TRUE(ret.second);

    auto &r = get_running_command();
    r.store(command_id);

    // Read and save the current signal handler (e.g. Python's SIGINT handler)
    // Set signal handler to catch CTRL-C during this call
    if(cancel_handling_enabled &&
        !console_cancel_handler::get_instance().set_handler()) {
      logstream(LOG_WARNING) << "Could not read previous signal handler, "
        "thus will not respond to CTRL-C.\n";
      cancel_handling_enabled = false;
    }

    // call
    reply_message reply;
    int retcode = internal_call(msg, reply);

    // Replace the SIGINT signal handler with the original one
    if(cancel_handling_enabled) {
      if(!console_cancel_handler::get_instance().unset_handler()) {
        logstream(LOG_WARNING) <<
          "Could not reset signal handler after server operation. Disabling CTRL-C support.\n";
        cancel_handling_enabled = false;
      }
    }

    // Check if we need to re-raise a SIGINT in Python
    if(cancel_handling_enabled) {
      // Check if CTRL-C was pressed for this command.
      size_t running_command = get_running_command().load();
      if(running_command && running_command == get_cancelled_command().load()) {
        // Check if there was a cancel message, whether it was heeded.
        auto ret = reply.properties.find(std::string("cancel"));

        // If there is no 'cancel' property, then must_cancel was never checked
        // on the server side, showing that this command does not support it.
        if(ret == reply.properties.end()) {
          // Raise this again for Python to make sure you can break out of a for
          // loop with a turicreate call inside that does not throw on ctrl-c
          // NOTE: I don't think I care if raise fails.
          console_cancel_handler::get_instance().raise_cancel();
        }
      }
    }

    // Reset running command
    get_running_command().store(0);

    bool success = (retcode == 0);
    std::string custommsg;
    if (reply.body != NULL && reply.bodylen > 0) {
      custommsg = std::string(reply.body, reply.bodylen);
    }
    if (!success) {
      throw ipcexception(reply_status::COMM_FAILURE, retcode, custommsg);
    } else if (reply.status != reply_status::OK) {
      switch(reply.status) {
        case reply_status::IO_ERROR:
#ifdef COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE
          throw(std::ios_base::failure(custommsg, std::error_code()));
#else
          throw(std::ios_base::failure(custommsg));
#endif
        case reply_status::INDEX_ERROR:
          throw std::out_of_range(custommsg);
        case reply_status::MEMORY_ERROR:
          throw turi::bad_alloc(custommsg);
        case reply_status::TYPE_ERROR:
          throw turi::bad_cast(custommsg);
        default:
          throw ipcexception(reply.status, retcode, custommsg);
      }
    } else {
      detail::set_deserializer_to_client(this);
      return detail::deserialize_return_and_clear<return_type,
             std::is_convertible<return_type, ipc_object_base*>::value>::exec(*this, reply);
    }
  }
};

} // cppipc
#endif
