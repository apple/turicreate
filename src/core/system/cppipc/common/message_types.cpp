/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/common/message_types.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
namespace cppipc {

void call_message::clear() {
  if (!zmqbodyused) {
    if (body) free((void*)body);
  }
  body = NULL;
  objectid = 0;
  zmqbodyused = false;
}

bool call_message::construct(nanosockets::zmq_msg_vector& msg) {
  clear();
  if(msg.size() != 4) return false;
  // first block is the object ID
  if (msg.front()->length() != sizeof(size_t)) return false;
  objectid = *reinterpret_cast<const size_t*>(msg.front()->data());
  msg.pop_front_and_free();

  // second block is the property bag
  const char* propertybuf = msg.front()->data();
  size_t propertybuflen = msg.front()->length();
  turi::iarchive iarc(propertybuf, propertybuflen);
  iarc >> properties;
  msg.pop_front_and_free();

  // third block is the function name
  function_name = *(msg.front());
  msg.pop_front_and_free();
  bodybuf = std::move(*msg.front());
  body = bodybuf.data();
  bodylen = bodybuf.size();
  zmqbodyused = true;
  msg.pop_front_and_free(); // no free this time since we are keeping a pointer
  return true;
}

void call_message::emit(nanosockets::zmq_msg_vector& msg) {
  assert(zmqbodyused == false);
  // first block is the object ID
  nanosockets::nn_msg_t* z_objectid = msg.insert_back();
  z_objectid->assign(sizeof(size_t), 8);
  (*reinterpret_cast<size_t*>(&((*z_objectid)[0]))) = objectid;

  // second block is the property bag
  turi::oarchive oarc;
  oarc << properties;
  nanosockets::nn_msg_t* z_propertybag = msg.insert_back();
  z_propertybag->assign(oarc.buf, oarc.off);

  // third block is the function name
  nanosockets::nn_msg_t* z_function_name = msg.insert_back();
  z_function_name->assign(function_name);

  // fourth block is the serialization body
  nanosockets::nn_msg_t* z_body = msg.insert_back();

  if (body != NULL) {
    z_body->assign(body, bodylen);
  }

  clear();
}



void reply_message::clear() {
  if (!zmqbodyused) {
    if (body) free((void*)body);
  }
  body = NULL;
  bodylen = 0;
  zmqbodyused = false;
}

bool reply_message::construct(nanosockets::zmq_msg_vector& msg) {
  clear();
  if(msg.size() != 3) return false;
  // first block is the reply status
  if (msg.front()->length() != sizeof(reply_status)) return false;
  status = *reinterpret_cast<const reply_status*>(msg.front()->data());
  msg.pop_front_and_free();

  // second block is the property bag
  const char* propertybuf = msg.front()->data();
  size_t propertybuflen = msg.front()->length();
  turi::iarchive iarc(propertybuf, propertybuflen);
  iarc >> properties;
  msg.pop_front_and_free();

  // third block is the serialization body
  bodybuf = std::move(*msg.front());
  body = &(bodybuf[0]);
  bodylen = bodybuf.length();
  zmqbodyused = true;
  msg.pop_front_and_free(); // no free this time since we are keeping a pointer
  return true;
}

void reply_message::emit(nanosockets::zmq_msg_vector& msg) {
  assert(zmqbodyused == false);
  // first block is the reply status
  nanosockets::nn_msg_t* z_status = msg.insert_back();
  z_status->assign(sizeof(reply_status), 0);
  (*reinterpret_cast<reply_status*>(&((*z_status)[0]))) = status;

  // second block is the property bag
  turi::oarchive oarc;
  oarc << properties;
  nanosockets::nn_msg_t* z_propertybag = msg.insert_back();
  z_propertybag->assign(oarc.buf, oarc.off);

  // third block is the serialization body
  nanosockets::nn_msg_t* z_body= msg.insert_back();

  if (body != NULL) {
    z_body->assign(body, bodylen);
  }

  clear();
}


EXPORT std::string reply_status_to_string(reply_status status) {
  switch(status) {
   case reply_status::OK:
     return "OK";
   case reply_status::BAD_MESSAGE:
     return "Bad message";
   case reply_status::NO_OBJECT:
     return "No such object ID";
   case reply_status::NO_FUNCTION:
     return "No such function";
   case reply_status::COMM_FAILURE:
     return "Communication Failure";
   case reply_status::AUTH_FAILURE:
     return "Authorization Failure";
   case reply_status::EXCEPTION:
     return "Runtime Exception";
   case reply_status::IO_ERROR:
     return "IO Exception";
   case reply_status::TYPE_ERROR:
     return "Type Exception";
   case reply_status::MEMORY_ERROR:
     return "Memory Exception";
   case reply_status::INDEX_ERROR:
     return "Index Exception";
   default:
     return "";
  }
}

} // cppipc
