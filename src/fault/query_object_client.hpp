/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef QUERY_OBJECT_CLIENT_HPP
#define QUERY_OBJECT_CLIENT_HPP

#include <stdint.h>
#include <boost/function.hpp>
#include <zookeeper_util/key_value.hpp>
#include <fault/sockets/async_request_socket.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/future.hpp>
#include <fault/sockets/socket_receive_pollset.hpp>
#include <fault/message_types.hpp>
namespace libfault {

/**
 * \ingroup fault
 */
class query_object_client {
 public:

  /**
   * Creates a client for query objects.
   */
  query_object_client(void* zmq_ctx,
                      std::vector<std::string> zkhosts,
                      std::string prefix,
                      size_t replica_count = 3);


 /**
   * Creates a client for query objects.
   */
  query_object_client(void* zmq_ctx,
                      turi::zookeeper_util::key_value* keyval,
                      size_t replica_count = 3);


  ~query_object_client();
  /**
   * A negative status denotes an error
   */
  struct query_result {
    query_result():content_ptr(new contents) {
      content_ptr->status = 0;
      content_ptr->ready = false;
    }

    query_result(const query_result& other):content_ptr(other.content_ptr) { }
    query_result& operator=(const query_result& other) {
      content_ptr = other.content_ptr;
      return *this;
    }

    std::string& get_reply() {
      if (!content_ptr->ready) wait();
      return content_ptr->parsed_reply;
    }
    int get_status() {
      if (!content_ptr->ready) wait();
      return content_ptr->status;
    }
    bool is_ready() const { return content_ptr->ready; }

    uint64_t reply_header_flags() const { return content_ptr->header.flags ; }
    uint64_t reply_header_msgid() const { return content_ptr->header.msgid; }
    uint64_t reply_header_version() const { return content_ptr->header.version; }

    void wait() {
      // only 1 thread should parse the future
      content_ptr->lock.lock();
      if (!content_ptr->ready) {
        // read the status
        content_ptr->status = content_ptr->future.get()->status;
        if (content_ptr->status == 0) {
          // we have messages. parse the reply
          query_object_reply qrep;
          qrep.parse(content_ptr->future.get()->msgvec);
          content_ptr->parsed_reply.assign(qrep.msg, qrep.msglen);
          content_ptr->header = qrep.header;
        } else {
          // error. clear the reply
          content_ptr->parsed_reply.clear();
        }
        //clear the msgvec in the future
        content_ptr->future.get()->msgvec.clear();
        // set the ready flag
        content_ptr->ready = true;
      }
      content_ptr->lock.unlock();
    }
   private:
    struct contents {
      boost::shared_future<message_reply*> future;
      boost::mutex lock;
      bool ready;
      int status;
      query_object_reply::header_type header;
      std::string parsed_reply;
    };
    boost::shared_ptr<contents> content_ptr;
    friend class query_object_client;
  };

  /// Get a handle which can be used for more efficient repeated calls
  void* get_object_handle(const std::string& key);
  /// Closes the connection to a particular object key
  void close_connection(const std::string& key);
  /// Closes the connection to a particular handle. The handle can no
  /// longer be used.
  void close_connection(void* objecthandle);

  /**
   * Sends an update message to the object identified by the objectkey.
   * This function takes over the msg pointer and will free it when done.
   */
  inline query_result update(const std::string& objectkey,
                             char* msg, size_t msglen, bool noreply = false) {
    socket_data* sock = get_socket(objectkey);
    return update(sock, msg, msglen, noreply);
  }


  /**
   * Sends an query message to the object identified by the objectkey.
   * This function takes over the msg pointer and will free it when done.
   */
  inline query_result query(const std::string& objectkey,
                            char* msg, size_t msglen, bool noreply = false) {
    socket_data* sock = get_socket(objectkey);
    return query(sock, msg, msglen, noreply);
  }


/**
   * Sends an query message to the object identified by the objectkey,
   * sending it to any master/slave object.
   * This function takes over the msg pointer and will free it when done.
   */
  inline query_result query_any(const std::string& objectkey,
                                char* msg, size_t msglen, bool noreply = false) {
    socket_data* sock = get_socket(objectkey);
    return query_any(sock, msg, msglen, noreply);
  }
  /**
   * Sends an update message to the object identified by the the objecthandle.
   * This function takes over the msg pointer and will free it when done.
   */
  query_result update(void* objecthandle,
                      char* msg, size_t msglen, bool noreply = false) {
    uint64_t flags = QO_MESSAGE_FLAG_UPDATE;
    if (noreply) flags |= QO_MESSAGE_FLAG_NOREPLY;
    return query_update_general(objecthandle, msg, msglen, flags);
  }

  /**
   * Sends an query message to the object identified by the objecthandle.
   * This function takes over the msg pointer and will free it when done.
   */
  query_result query(void* objecthandle,
                     char* msg, size_t msglen, bool noreply = false) {
    uint64_t flags = QO_MESSAGE_FLAG_QUERY;
    if (noreply) flags |= QO_MESSAGE_FLAG_NOREPLY;
    return query_update_general(objecthandle, msg, msglen, flags);
  }

  /**
   * Sends an query message to the object identified by the objecthandle,
   * sending it to any master/slave object.
   * This function takes over the msg pointer and will free it when done.
   */
  query_result query_any(void* objecthandle,
                         char* msg, size_t msglen, bool noreply = false) {
    uint64_t flags = QO_MESSAGE_FLAG_QUERY | QO_MESSAGE_FLAG_ANY_TARGET;
    if (noreply) flags |= QO_MESSAGE_FLAG_NOREPLY;
    return query_update_general(objecthandle, msg, msglen, flags);
  }

  query_result query_update_general(void* objecthandle,
                                    char* msg, size_t msglen,
                                    uint64_t flags);


  /**************************************************************************/
  /*                                                                        */
  /*                 Future versions. Not yet implemented.                  */
  /*            Requires some asynchronous stuff to exist first.            */
  /*                                                                        */
  /**************************************************************************/

  /*
  boost::unique_future<query_result>
      update(const std::string& objectkey,
              char* msg, size_t msglen);


  boost::unique_future<query_result>
      query(const std::string& objectkey,
            char* msg, size_t msglen);

  boost::unique_future<query_result>
      update(void* objecthandle,
              char* msg, size_t msglen);

  boost::unique_future<query_result>
      query(void* objecthandle,
            char* msg, size_t msglen);

  */

 private:
  void* z_ctx;
  turi::zookeeper_util::key_value* zk_keyval;
  bool my_keyval; // whether I allocated the keyval
  size_t replica_count;

  struct socket_data {
    std::string key;
    async_request_socket* sock;
    uint64_t randid;
    size_t creation_time;
  };

  socket_data* get_socket(const std::string& objectkey);

  boost::mutex lock;
  std::map<std::string, socket_data*> sockets;

  socket_receive_pollset pollset;

 };

} // libfault
#endif

