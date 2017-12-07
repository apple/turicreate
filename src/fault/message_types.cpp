/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cassert>
#include <vector>
#include <zmq.h>
#include <fault/message_types.hpp>
#include <fault/zmq/zmq_msg_standard_free.hpp>
namespace libfault {



/**************************************************************************/
/*                                                                        */
/*                             Query Message                              */
/*                                                                        */
/**************************************************************************/


void query_object_message::parse(zmq_msg_vector& data) {
  // there should be 2 parts. A header 
  // then the actual data
  assert(data.num_unread_msgs() >= 2);
  zmq_msg_t* zhead = data.read_next();
  zmq_msg_t* zmsg = data.read_next();
  assert(zmq_msg_size(zhead) == sizeof(header));

  header= *(header_type*)zmq_msg_data(zhead);
  msg = (char*)zmq_msg_data(zmsg);
  msglen = zmq_msg_size(zmsg);
}

void query_object_message::write(zmq_msg_vector& outdata) {
  // create 2 message part. One with the header 
  // then the actual data.
  zmq_msg_t* zhead = outdata.insert_back();
  zmq_msg_init_size(zhead, sizeof(header_type));
  (*(header_type*)zmq_msg_data(zhead)) = header;

  zmq_msg_t* zmsg = outdata.insert_back();
  zmq_msg_init_data(zmsg, msg, msglen, zmq_msg_standard_free, NULL);
}



/**************************************************************************/
/*                                                                        */
/*                              Query Reply                               */
/*                                                                        */
/**************************************************************************/

void query_object_reply::parse(zmq_msg_vector& data) {
  // there should be 2 parts. A header 
  // then the actual data
  assert(data.num_unread_msgs() >= 2);
  zmq_msg_t* zhead = data.read_next();
  zmq_msg_t* zmsg = data.read_next();
  assert(zmq_msg_size(zhead) == sizeof(header));

  header= *(header_type*)zmq_msg_data(zhead);
  msg = (char*)zmq_msg_data(zmsg);
  msglen = zmq_msg_size(zmsg);
}


void query_object_reply::write(zmq_msg_vector& outdata) {
  // create 2 message part. One with the header 
  // then the actual data.
  zmq_msg_t* zhead = outdata.insert_back();
  zmq_msg_init_size(zhead, sizeof(header_type));
  (*(header_type*)zmq_msg_data(zhead)) = header;

  zmq_msg_t* zmsg = outdata.insert_back();
  zmq_msg_init_data(zmsg, msg, msglen, zmq_msg_standard_free, NULL);
}


} // namespace libfault
