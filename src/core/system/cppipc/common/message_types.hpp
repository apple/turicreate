/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_COMMON_MESSAGE_TYPES_HPP
#define CPPIPC_COMMON_MESSAGE_TYPES_HPP
#include <string>
#include <exception>
#include <sstream>
#include <map>
#include <core/system/nanosockets/zmq_msg_vector.hpp>
#include <typeinfo>
#include <core/system/exceptions/error_types.hpp>
#include <core/export.hpp>
namespace cppipc {
namespace nanosockets = turi::nanosockets;
/**
 * \ingroup cppipc
 * The contents of the message used to call a function from
 * the client to the server.
 */
struct EXPORT call_message {
  /// Default constructor
  call_message(): objectid(0), body(NULL), zmqbodyused(false) {}
  /**
   * constructs a call_message from a zmq_msg_vector.
   * Will also, as a side effect, clear the zmq_msg_vector.
   * Any existing contents in the call_message will be cleared.
   * Returns true on success, false if the format is incorrect.
   */
  bool construct(nanosockets::zmq_msg_vector& vec);

  /**
   * appends the message a zmq_msg_vector from a call message.
   * Will also as a side effect, clear the contents of this call message.
   */
  void emit(nanosockets::zmq_msg_vector& vec);

  /// Empties the message, freeing all contents
  void clear();

  ~call_message(){clear();};

  size_t objectid; /// the object to call
  std::string function_name; /// the function to call on the object
  std::map<std::string, std::string> properties; /// Other properties

  nanosockets::nn_msg_t bodybuf; /** When receiving, this will contain the actual contents
                       of the body */
  const char* body; /// The serialized arguments of the call. May point into bodybuf
  size_t bodylen; /// The length of the body.

  /*
   * The need to have the seperate zmqbodybuf is that zeromq messages are
   * "intelligent". If they are small, they are managed in place inside
   * the nn_msg_t directly. Only if they are big, then a malloc is used.
   * However, zeromq does not provide me an easy way to figure out when this is
   * the case, and I would like to maintain the zero-copy behavior of the
   * messages as much as possible. The solution is therefore to use zmq_msg_move
   * to move the the body contents into a new nn_msg_t, then take pointers
   * into that.
   *
   * zmqbodyused should only be set on receiving. i.e. by the construct
   * call.
   */
  bool zmqbodyused; /// True if bodybuf is used and body is from bodybuf

 private:
  call_message(const call_message&);
  call_message& operator=(const call_message&);
};


/**
 * \ingroup cppipc
 * The reply status.
 */
enum class reply_status:size_t {
  OK, ///< Call was successful
  BAD_MESSAGE, ///< The object requested did not exist
  NO_OBJECT, ///< The object requested did not exist
  NO_FUNCTION, ///< The function requested did not exist
  COMM_FAILURE, ///< Communication error
  AUTH_FAILURE, ///< Authentication failure
  IO_ERROR, ///< IO Error
  MEMORY_ERROR, ///< Memory Error
  INDEX_ERROR, ///< Index Error
  TYPE_ERROR, ///< Type Error
  EXCEPTION, ///< Other general exception. Body will contain the exception message.
};


/**
 * \ingroup cppipc
 * The contents of the message when replying from the server to the client.
 */
struct EXPORT reply_message {
  /// Default constructor
  reply_message(): body(NULL), bodylen(0),zmqbodyused(false) {}
  /**
   * Constructs a reply_message from a zmq_msg_vector.
   * Will also, as a side effect, clear the zmq_msg_vector
   * Any existing contents in the reply_message will be cleared;
   * Returns true on success, false if the format is incorrect.
   */
  bool construct(nanosockets::zmq_msg_vector& vec);

  /**
   * appends the message a zmq_msg_vector from a call message.
   * Will also as a side effect, clear the contents of this call message.
   */
  void emit(nanosockets::zmq_msg_vector& vec);

  /// Empties the message, freeing all contents
  void clear();

  ~reply_message(){clear();};

  reply_status status; /// The status of the call.
  std::map<std::string, std::string> properties; /// Other properties
  nanosockets::nn_msg_t bodybuf; /** When receiving, this will contain the actual contents
                       of the body */
  char* body; /// The serialized contents of the reply. May point into bodybuf
  size_t bodylen; /// The length of the body.

  /*
   * The need to have the seperate zmqbodybuf is that zeromq messages are
   * "intelligent". If they are small, they are managed in place inside
   * the nn_msg_t directly. Only if they are big, then a malloc is used.
   * However, zeromq does not provide me an easy way to figure out when this is
   * the case, and I would like to maintain the zero-copy behavior of the
   * messages as much as possible. The solution is therefore to use zmq_msg_move
   * to move the the body contents into a new nn_msg_t, then take pointers
   * into that.
   *
   * zmqbodyused should only be set on receiving. i.e. by the construct
   * call.
   */
  bool zmqbodyused; /// True if bodybuf is used and body is from bodybuf

  inline void copy_body_from(const std::string& s) {
    assert(zmqbodyused == false);
    if (!body) free(body);
    body = (char*)malloc(s.length());
    memcpy(body, s.c_str(), s.length());
    bodylen = s.length();
  }

  inline void copy_body_from(const char* c, size_t len) {
    assert(zmqbodyused == false);
    if (!body) free(body);
    body = (char*)malloc(len);
    memcpy(body, c, len);
    bodylen = len;
  }
 private:
  reply_message(const reply_message&);
  reply_message& operator=(const reply_message&);
};


/**
 * \ingroup cppipc
 * Generates a printable string version of the a reply_status
 */
std::string reply_status_to_string(reply_status);

class ipcexception : public std::exception {
  public:
    ipcexception(reply_status s,
                 std::string custom_errstring = "")
        :status(s), errorcode(0), custom_errstring(custom_errstring) {
      make_error_string();
    }
    ipcexception(reply_status s,
                 int errorcode,
                 std::string custom_errstring = "")
        :status(s), errorcode(errorcode), custom_errstring(custom_errstring){
      make_error_string();
    }

    virtual ~ipcexception() throw() { }

    const char* what() const throw() {
      return errstring.c_str();
    }

    /**
     * Returns the reply_status value
     */
    reply_status get_reply_status() {
      return status;
    }

    /**
     * Returns the zeroMQ error code which caused the error. If any.
     */
    int get_zeromq_errorcode() {
      return errorcode;
    }

  private:
    reply_status status;
    int errorcode;
    std::string errstring;
    std::string custom_errstring;

    void make_error_string() {
      std::stringstream strm;
      if (errorcode == 0) {
        strm << reply_status_to_string(status)
             << ". " << custom_errstring;
      } else {
        strm << reply_status_to_string(status) + ": " << errorcode
             << ". " << custom_errstring;
      }
      strm.flush();
      errstring = strm.str();
    }
};

} // cppipc
#endif
