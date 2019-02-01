/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef FAULT_MESSAGE_TYPES_HPP
#define FAULT_MESSAGE_TYPES_HPP

#include <stdint.h>
#include <zmq.h>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <fault/message_flags.hpp>
namespace libfault {

/**
 * \internal
 * \ingroup fault
 * The internal structure of a message for an object
 * This struct is filled in using direct pointers to the
 * actual message string. Thus the member pointers should not 
 * be freed.
 */
struct query_object_message {
  struct header_type {
    /// Flags identifying message properties.
    /// for instance, if this is a query message or an update message
    uint64_t flags;
    /**
     * Each update message has a a ID identifying the query.
     * This should be generated randomly by the sender.
     * This ID does not require strong randomness guarantees, but is
     * used to only to identify unprocessed messages in the event
     * of a machine failure.
     */
    uint64_t msgid;
  } header;
  /// The message contents
  char* msg;
  /// The length of the message.
  size_t msglen;

  /// Parses the message header. 
  /// No copies are made, and msg will point directly into the zeromq
  /// message structure.
  void parse(zmq_msg_vector& data);

  /// Generates the zeromq message corresponding to this message
  /// Gives away to zeromq the pointer to the message contents
  /// Only appends are performed to outdata.
  void write(zmq_msg_vector& outdata);
};


/**
 * \internal
 * \ingroup fault
 * The internal structure of a reply to a query message.
 * This struct is filled in using direct pointers to the
 * actual message string. Thus the member pointers should not 
 * be freed.
 */
struct query_object_reply {
  struct header_type {
    /// Any additional flags
    uint64_t flags;
    /// The version of the object which generated the reply.
    uint64_t version;
    /// The message id which generated this reply
    uint64_t msgid;
  } header;
  /// The message contents
  char* msg;
  /// The length of the message.
  size_t msglen;
  
  /// Parses the reply header.
  /// No copies are made, and msg will point directly into the zeromq
  /// message structure.
  void parse(zmq_msg_vector& data);

  /// Generates the zeromq message corresponding to this message
  /// Gives away to zeromq the pointer to the message contents
  /// Only appends are performed to outdata.
  void write(zmq_msg_vector& outdata);

};



} // namespace libfault

#endif
