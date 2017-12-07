/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <zmq.h>
#include <stdint.h>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <fault/zmq/print_zmq_error.hpp>


namespace libfault {


int zmq_msg_vector::send(void* socket, int timeout) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = send_impl(socket, timeout);
  } while (rc == EINTR);
  return rc;
}

int zmq_msg_vector::send(void* socket) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = send_impl(socket);
  } while (rc == EINTR);
  return rc;
}

int zmq_msg_vector::recv(void* socket, int timeout) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = recv_impl(socket, timeout);
  } while (rc == EINTR);
  return rc;
}

int zmq_msg_vector::recv(void* socket) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = recv_impl(socket);
  } while (rc == EINTR);
  return rc;
}




int zmq_msg_vector::send_impl(void* socket, int timeout) {
  // wait until I can send
  zmq_pollitem_t pollitem;
  pollitem.events = 0;
  pollitem.revents = 0;
  pollitem.fd = 0;
  pollitem.events = ZMQ_POLLOUT;
  pollitem.socket = socket;

  int rc = zmq_poll(&pollitem, 1, timeout);

  if (rc == -1) return zmq_errno();
  else if (rc == 0) return EAGAIN;
  std::deque<zmq_msg_t>::iterator iter = msgs.begin();
  while (iter != msgs.end()){
    zmq_msg_t temp;
    zmq_msg_init(&temp);
    zmq_msg_copy(&temp, &(*iter));
    ++iter;
    // to revert
    int rc = zmq_msg_send(&temp, socket, ZMQ_DONTWAIT |
                         ((iter != msgs.end())?ZMQ_SNDMORE:0));
    zmq_msg_close(&temp);
    if (rc == -1) {
      if (zmq_errno() == EAGAIN) {
        continue;
        // should not be possible. We do have valid poll success
        print_zmq_error("zmq_msg_vector Unexpected EAGAIN in send");
      }
      return zmq_errno();
    }
  }
  return 0;
}


int zmq_msg_vector::send_impl(void* socket) {
  // like send with a timeout of -1. We can however, avoit the POLLOUT
  // and just go to sendmsg directly
  std::deque<zmq_msg_t>::iterator iter = msgs.begin();
  while (iter != msgs.end()){
    zmq_msg_t temp;
    zmq_msg_init(&temp);
    zmq_msg_copy(&temp, &(*iter));
    ++iter;
    // to revert
    int rc = zmq_msg_send(&temp, socket, ((iter != msgs.end())?ZMQ_SNDMORE:0));
    zmq_msg_close(&temp);
    if (rc == -1) return zmq_errno();
  }
  return 0;
}


int zmq_msg_vector::recv_impl(void* socket, int timeout) {
  zmq_pollitem_t pollitem;
  pollitem.events = 0;
  pollitem.revents = 0;
  pollitem.fd = 0;
  pollitem.events = ZMQ_POLLIN;
  pollitem.socket = socket;

  int rc = zmq_poll(&pollitem, 1, timeout);

  if (rc == -1) return zmq_errno();
  else if (rc == 0) return EAGAIN;

  int64_t more = 1;
  size_t morelen = sizeof(more);
  clear();
  while(more) {
    zmq_msg_t* msg = insert_back();
    zmq_msg_init(msg);
    int rc = zmq_msg_recv(msg, socket, ZMQ_DONTWAIT);
    if (rc == -1) {
      if (zmq_errno() == EAGAIN) {
        continue;
        // should not be possible. We do have valid poll success
        print_zmq_error("zmq_msg_vector Unexpected EAGAIN in recv");
      }
      return zmq_errno();
    }
    // any more stuff to receive?
    rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &morelen);
    if (rc == -1) return zmq_errno();
  }
  return 0;
}

int zmq_msg_vector::recv_impl(void* socket) {
  int64_t more = 1;
  size_t morelen = sizeof(more);
  clear();
  while(more) {
    zmq_msg_t* msg = insert_back();
    zmq_msg_init(msg);
    int rc = zmq_msg_recv(msg, socket, 0);
    if (rc == -1) return zmq_errno();
    // any more stuff to receive?
    rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &morelen);
    if (rc == -1) return zmq_errno();
  }
  return 0;
}

};
