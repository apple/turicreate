/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_COMMON_AUTHENTICATION_BASE_HPP
#define CPPIPC_COMMON_AUTHENTICATION_BASE_HPP
#include <string>
#include <core/system/cppipc/common/message_types.hpp>
namespace cppipc {

/**
 * Base class for all authentication method implementations.
 *
 * The class implements a few basic functions to attach and validate messages
 * sent between the client and the server. Messages sent from the client to
 * the server are \ref call_message objects. Messages sent from the server
 *  to the client (in response to a call message) are \ref reply_message
 *  objects.
 *
 * \ref authentication_base::apply_auth(call_message& msg) is called on the
 * client side to attach authentication information to the message. When the
 * server receives the message,
 * \ref authentication_base::validate_auth(call_message& msg) is called on the
 * server side to validate the message. If this function returns false, the
 * server discards the message.
 * The server then replies with a \ref reply_message and the function
 * \ref authentication_base::apply_auth(reply_message& msg) is called on the
 * server side to attach authentication information to the message. When the
 * client receives the message,
 * \ref authentication_base::validate_auth(reply_message& msg) is called on the
 * client side to validate the message. If this function returns false, the
 * function call is marked as failed.
 *
 * All of the implemented functions must be reentrant, and must not assume
 * synchronicity. (i.e. apply_auth can be called on the client side many
 * times in succession).
 *
 * Finally, authentication methods should be designed to be "stackable" with
 * other authentication methods. i.e. I should be able to apply two different
 * types of authentication methods on top of each other.
 */
class authentication_base {
 public:
  virtual inline ~authentication_base(){}

  /**
   * Attaches the authentication information to a message sent
   * from the client to the server. This function must be reentrant.
   */
  virtual void apply_auth(call_message& msg) = 0;

  /**
   * Attaches the authentication information to a message sent
   * from the server to the client. This function must be reentrant.
   */
  virtual void apply_auth(reply_message& msg) = 0;

  /**
   * Validates a message received on the server from a client. This function
   * must be reentrant. If the function returns true, the message is good.
   * Otherwise, the message is bad.
   */
  virtual bool validate_auth(call_message& msg) = 0;

  /**
   * Validates a message received on the client from a server. This function
   * must be reentrant. If the function returns true, the message is good.
   * Otherwise, the message is bad.
   */
  virtual bool validate_auth(reply_message& msg) = 0;
};

} // cppipc

#endif
