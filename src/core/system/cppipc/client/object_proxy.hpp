/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_CLIENT_CLIENT_HPP
#define CPPIPC_CLIENT_CLIENT_HPP
#include <string>
#include <map>
#include <core/system/cppipc/client/comm_client.hpp>
namespace cppipc {


/**
 * \ingroup cppipc
 * Refers to a remote object and allows function calls to be issued
 * across a communication link.
 *
 * The object_proxy object internally maintains an object ID which identifies
 * an object on the other side of a communication link established by the
 * comm_client. The object_proxy is templatized over a type T which must have
 * an identical object interface as the remote object. T must also provide the
 * registration callbacks which match the registration callbacks on the remote
 * end. For that reason, it is easiest if T defines an interface and implements
 * the register callbacks. And the remote object then inherits from T.
 *
 * The recommended pattern is as follows:
 * Given the following base class with an implementation on the server side
 *
 * \code
 * class object_base {
 *  public:
 *   virtual ~object_base() { }
 *   virtual int add(int, int) = 0;
 * };
 * \endcode
 *
 * We implement a proxy object which has the object_proxy as a member, and
 * inherits from the object_base. Each function call is then forwarded to
 * the remote machine.
 *
 * \code
 * class object_proxy: public object_base {
 *  public:
 *   cppipc::object_proxy<object_base> proxy;
 *
 *   // proxy must be provided with the comm_client object on construction
 *   test_object_proxy(cppipc::comm_client& comm):proxy(comm){ }
 *
 *   int add(int a, int b) {
 *     // forward the call to the remote machine
 *     return proxy.call(&object_base::add,a, b);
 *   }
 * };
 * \endcode
 */
template <typename T>
class object_proxy {
 public:
   /**
    * Creates an object_proxy object using the communication client provided
    * for communication. If auto_create is set to true, a new remote object
    * is created on the remote machine. Otherwise, no remote object ID is
    * specified, and set_object_id() must be called to set the remote object ID.
    *
    * \param comm The comm client object to use for communication
    * \param auto_create If true, will create the remote object on
    *                    construction of the object proxy.
    * \param object_id   The object ID to use in the proxy. Only valid when
    *                    auto_create is false.
    */
  object_proxy(comm_client& comm,
               bool auto_create = true,
               size_t object_id = (size_t)(-1)):
      comm(comm),
      remote_object_id(object_id) {
    T::__register__(comm);
    if (auto_create) remote_object_id = comm.make_object(T::__get_type_name__());

    // Increase reference count of this object
    size_t ref_cnt = comm.incr_ref_count(remote_object_id);
    if(ref_cnt == 0) {
      // Shouldn't ever happen
      throw ipcexception(reply_status::EXCEPTION,
                        0,
                        "Object not tracked after increasing ref count!");
    }
  }

  ~object_proxy() {
    remote_delete();
  }

  /**
   * Deletes the remote object referred to by this proxy.
   * The object ID in the proxy will be cleared.
   */
  void remote_delete() {
    if (remote_object_id != (size_t)(-1)) {
      comm.decr_ref_count(remote_object_id);
    }
    remote_object_id = (size_t)(-1);
  }

  /**
   * Assigns the object ID managed by this proxy.
   */
  void set_object_id(size_t object_id) {
    comm.decr_ref_count(remote_object_id);
    comm.incr_ref_count(object_id);
    remote_object_id = object_id;
  }
  /**
   * Gets the object ID managed by this proxy.
   */
  size_t get_object_id() const {
    return remote_object_id;
  }

  /**
   * Returns a reference to the communication object used.
   */
  inline comm_client& get_comm() {
    return comm;
  }

  /// \internal do not use
  template <typename S>
  void register_object(S* object) {
    // do nothing
  }

  /**
   * \internal do not use
   * Registers a member function of T, associating it with a string name
   */
  template <typename MemFn>
  void register_function(MemFn f, std::string function_string) {
    comm.register_function(f, function_string);
  }

  /**
   * Calls a remote function returning the result.
   * The remote function's return is forwarded and returned here.
   * An exception is raised of type reply_status if there is an error.
   */
  template <typename MemFn, typename... Args>
  typename detail::member_function_return_type<MemFn>::type
  call(MemFn f, const Args&... args) {
    return comm.call(remote_object_id, f, args...);
  }

 private:
  comm_client& comm;
  size_t remote_object_id;
};


} // cppipc
#endif
