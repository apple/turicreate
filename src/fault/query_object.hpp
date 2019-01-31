/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef FAULT_QUERY_OBJECT_HPP
#define FAULT_QUERY_OBJECT_HPP

#include <stdint.h>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <fault/message_types.hpp>

namespace libfault {

class query_object_server;
struct query_object_server_master;
struct query_object_server_replica;

/**
 * \ingroup fault
 */
class query_object {
 public:

   query_object():version(0) {
   }

   virtual ~query_object() { }

  /**
   * Processes a query message which may not make changes to the object.
   * \param msg A pointer to the query message.
   * \param msglen The length of the query message
   * \param outreply An output parameter which should contain the reply message.
   *                 Reply should be written to (*outreply).
   *                 (*outreply) will be NULL and should be allocated
   *                 using malloc()
   * \param outreplylen An output parameter which should contain the length of
   *                    the reply message in (*outreply)
   */
  virtual void query(char* msg, size_t msglen,
                     char** outreply, size_t *outreplylen) = 0;

  /**
   * Processes an update message which may make changes to the object.
   * Returns true if the object was modified. The return value may be
   * conservative. As in, update() could always return true whether or not
   * the object was modified.
   * \param msg A pointer to the query message.
   * \param msglen The length of the query message
   * \param outreply An output parameter which should contain the reply message.
   *                 Reply should be written to (*outreply).
   *                 (*outreply) will be NULL and should be allocated
   *                 using malloc()
   * \param outreplylen An output parameter which should contain the length of
   *                    the reply message in (*outreply)
   * \retval True If the query modified the object.
   * \retval False If the query did not modify the object.
   */
  virtual bool update(char* msg, size_t msglen,
                     char** outreply, size_t *outreplylen) = 0;


  /**
   * Processes a query message which may not make changes to the object.
   * \param msg A pointer to the query message.
   * \param msglen The length of the query message
   * This need not be implemented. The default implementation simply
   * calls the full update() function, but immediately drops the reply.
   */
  inline virtual void query(char* msg, size_t msglen) {
    char* outreply = NULL;
    size_t outreplylen = 0;
    query(msg, msglen, &outreply, &outreplylen);
    if (outreply) free(outreply);
  }

  /**
   * Processes an update message which may make changes to the object,
   * and does not need a reply message.
   * Returns true if the object was modified. The return value may be
   * conservative. As in, update() could always return true whether or not
   * the object was modified.
   * This need not be implemented. The default implementation simply
   * calls the full update() function, but immediately drops the reply.
   * \param msg A pointer to the query message.
   * \param msglen The length of the query message
   * \retval True If the query modified the object.
   * \retval False If the query did not modify the object.
   */
  inline virtual bool update(char* msg, size_t msglen) {
    char* outreply = NULL;
    size_t outreplylen = 0;
    bool ret = update(msg, msglen, &outreply, &outreplylen);
    if (outreply) free(outreply);
    return ret;
  }


  /**
   * Serializes the object to a string.
   * \param outbuf An output parameter which should contain the serialized
   *               representation of the object.
   *               (*outbuf) will be NULL and should be allocated
   *               using malloc()
   * \param outbuflen An output parameter which should contain the length of
   *                    the serialized representation in outbuf
   */
  virtual void serialize(char** outbuf, size_t *outbuflen) = 0;


  /**
   * Deserializes the object from a string.
   * The deserialized object must not retain any state prior to the call
   * to deserialize. i.e. \ref serialize and \ref deserialize are fully
   * symmetric.
   * \param buf The memory buffer to deserialize from.
   * \param buflen The length of the buffer.
   */
  virtual void deserialize(const char* buf, size_t buflen) = 0;

  /**
   * Optional. Called if the object was upgraded to a master.
   */
  virtual void upgrade_to_master() { }

  inline uint64_t get_version() {
    return version;
  }

 protected:
  uint64_t version;

  /**
   * Wraps the query/update calls, giving it a zeromq interface.
   * Also detaches the additional tracking information attached to the message,
   * and attaches the tracking information required in the reply.
   *
   * Since the actual query, and its reply have additional headers attached,
   * it is important to use the query_client object to communicate with the
   * query_object.
   * Returns true if the object changed. False otherwise.
   *
   * \note
   * Equivalent to
   * parse_message(msg, qmsg);
   * qomsg.header.flags |= flags_override
   * process_message(qmsg, reply, hasreply);
   */
  bool message_wrapper(zmq_msg_vector& message,
                       zmq_msg_vector& reply,
                       bool* hasreply,
                       uint64_t flags_override = 0);

  /// Converts a zeromq message to a query_object_message
  void parse_message(zmq_msg_vector& message,
                     query_object_message& qmsg);


  /// process the query_object message and returns the reply
  bool process_message(query_object_message& qmsg,
                       zmq_msg_vector& reply,
                       bool* hasreply);



  /**
   * A wrapper around serialize giving it a zeromq interface
   */
  void serialize_wrapper(zmq_msg_vector& output);

  /**
   * A wrapper around deserialize giving it a zeromq interface
   */
  void deserialize_wrapper(zmq_msg_vector& input);

  friend class query_object_server;
  friend struct query_object_server_master;
  friend struct query_object_server_replica;
};

} // libfault

#endif
