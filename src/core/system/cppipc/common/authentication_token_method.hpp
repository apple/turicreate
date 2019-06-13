/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_COMMON_AUTHENTICATION_TOKEN_HPP
#define CPPIPC_COMMON_AUTHENTICATION_TOKEN_HPP
#include <string>
#include <core/system/cppipc/common/message_types.hpp>
#include <core/system/cppipc/common/authentication_base.hpp>
namespace cppipc {
/**
 *
 * The authentication token method is the simplest mode of authentication.
 * Both client and server knows a secret token value, then every message
 * between client and server must contain the token. Without additional
 * safeguards, this authentication method does not provide any real security
 * aside from protecting against accidental connections, since any packet
 * sniffer will be able to obtain the token value.
 */
class authentication_token_method : public authentication_base {
 private:
  std::string token_value;
 public:
  authentication_token_method(std::string token_value) : token_value(token_value) { }
  inline ~authentication_token_method(){}
  inline void apply_auth(call_message& msg) {
    msg.properties["authtoken"] = token_value;
  }

  inline void apply_auth(reply_message& msg) {
    msg.properties["authtoken"] = token_value;
  }
  inline bool validate_auth(call_message& msg) {
    return msg.properties.count("authtoken") > 0 &&
        msg.properties["authtoken"] == token_value;
  }
  inline bool validate_auth(reply_message& msg) {
    return msg.properties.count("authtoken") > 0 &&
        msg.properties["authtoken"] == token_value;
  }

};



} // cppipc
#endif
