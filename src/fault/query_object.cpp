/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cassert>
#include <fault/query_object.hpp>
#include <fault/message_types.hpp>
#include <fault/zmq/zmq_msg_standard_free.hpp>

namespace libfault {

void query_object::parse_message(zmq_msg_vector& message,
                                 query_object_message& qmsg) {
  qmsg.parse(message);
}

/// process the query_object message and returns the reply
bool query_object::process_message(query_object_message& qmsg,
                                   zmq_msg_vector& reply,
                                   bool* hasreply) {
  bool retval = false;
  (*hasreply) = false;
  // parse the query message
  // here is where the reply will go
  query_object_reply qreply;
  qreply.msg = NULL; qreply.msglen = 0;

  // special internal flags
  if (qmsg.header.flags & QO_MESSAGE_FLAG_GET_SERIALIZED_CONTENTS) {
    serialize(&qreply.msg, &qreply.msglen);
  }
  // dispatch among the 4 possible update/query function
  else if (qmsg.header.flags & QO_MESSAGE_FLAG_UPDATE) {
    uint64_t oldversion = version;
    if (qmsg.header.flags & QO_MESSAGE_FLAG_NOREPLY) {
      version += update(qmsg.msg, qmsg.msglen);
    } else {
      version += update(qmsg.msg, qmsg.msglen, &qreply.msg, &qreply.msglen);
    }
    retval = (oldversion != version);
  } else {
    if (qmsg.header.flags & QO_MESSAGE_FLAG_NOREPLY) {
      query(qmsg.msg, qmsg.msglen);
    } else {
      query(qmsg.msg, qmsg.msglen, &qreply.msg, &qreply.msglen);
    }
  }


  // if there is a reply needed. construct it
  if (!(qmsg.header.flags & QO_MESSAGE_FLAG_NOREPLY)) {
    // fill the reply header
    qreply.header.msgid = qmsg.header.msgid;
    qreply.header.flags = 0;
    qreply.header.version = version;
    qreply.write(reply);
    (*hasreply) = true;
  }
  return retval;
}



bool query_object::message_wrapper(zmq_msg_vector& message,
                                   zmq_msg_vector& reply,
                                   bool* hasreply,
                                   uint64_t flags_override) {
  query_object_message qmsg;
  parse_message(message, qmsg);
  qmsg.header.flags |= flags_override;
  return process_message(qmsg, reply, hasreply);
}

void query_object::serialize_wrapper(zmq_msg_vector& output) {
  char* buf = NULL;
  size_t len;
  serialize(&buf, &len);
  zmq_msg_t* qmsg = output.insert_back();
  zmq_msg_init_data(qmsg, buf, len, zmq_msg_standard_free, NULL);
}

void query_object::deserialize_wrapper(zmq_msg_vector& input) {
  assert(input.num_unread_msgs() >= 1);
  zmq_msg_t* zmsg = input.read_next();
  deserialize((const char*)zmq_msg_data(zmsg), zmq_msg_size(zmsg));
}




} // namespace libfault
