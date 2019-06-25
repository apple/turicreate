/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <core/parallel/atomic.hpp>
#include <boost/bind.hpp>
#include <core/logging/logger.hpp>
#include <core/system/nanosockets/socket_errors.hpp>
#include <core/system/nanosockets/socket_config.hpp>
#include <core/system/nanosockets/async_reply_socket.hpp>
#include <core/system/nanosockets/print_zmq_error.hpp>
#include <core/system/nanosockets/get_next_port_number.hpp>
#include <network/net_util.hpp>
extern "C" {
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
}
namespace turi {
namespace nanosockets {


async_reply_socket::async_reply_socket(callback_type callback,
                           size_t nthreads,
                           std::string bind_address) {
  this->callback = callback;
  z_socket = nn_socket(AF_SP_RAW, NN_REP);
  set_conservative_socket_parameters(z_socket);
  if (bind_address.length() > 0) {
    local_address = normalize_address(bind_address);
    int rc = nn_bind(z_socket, local_address.c_str());
    if (rc < 0) {
      print_zmq_error("async_reply_socket construction: ");
      assert(rc >= 0);
    }
  } else {
    std::string localip = turi::get_local_ip_as_str(true);
    bool ok = false;
    while (!ok) {
      size_t port = get_next_port_number();
      char port_as_string[128];
      sprintf(port_as_string, "%ld", port);
      local_address = "tcp://" + localip + ":" + port_as_string;
      // try to bind
      int rc = nn_bind(z_socket, local_address.c_str());
      ok = (rc >= 0);
      /*if (rc == EADDRINUSE) {
        std::cout << local_address << " in use. Trying another port.\n";
        continue;
      } else if (rc != 0) {
        std::cout << "Unable to bind to " << local_address << ". "
                  << "Error(" << rc << ") = " << zmq_strerror(rc) << "\n";
      }*/
    }
  }

  for (size_t i = 0;i < nthreads; ++i) {
    threads.launch(std::bind(&async_reply_socket::thread_function,
                             this));
  }
}
void async_reply_socket::start_polling() {
  poll_thread.launch(std::bind(&async_reply_socket::poll_function,
                           this));
}
void async_reply_socket::stop_polling() {
  queuelock.lock();
  queue_terminate = true;
  queuecond.notify_all();
  queuelock.unlock();
  poll_thread.join();
}

void async_reply_socket::close() {
  if (z_socket != -1) {
    // kill all threads
    stop_polling();
    nn_close(z_socket);
    threads.join();
    z_socket = -1;
  }
}

async_reply_socket::~async_reply_socket() {
  close();
}



void async_reply_socket::poll_function() {

  while(1) {
    struct nn_pollfd pfd[1];
    pfd[0].fd = z_socket;
    pfd[0].events = NN_POLLIN;
    pfd[0].revents = 0;
    nn_poll (pfd, 1, 1000);
    if (queue_terminate) return;
    if ((pfd[0].revents & NN_POLLIN) == 0) {
        continue;
    }
    job j;
    // receive a message
    struct nn_iovec iov;
    struct nn_msghdr hdr;

    iov.iov_base = reinterpret_cast<void*>(&(j.data));
    iov.iov_len = NN_MSG;
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
    hdr.msg_control = reinterpret_cast<void*>(&(j.control));
    hdr.msg_controllen = NN_MSG;
    {
      std::unique_lock<mutex> socklock(socketlock);
      int rc = nn_recvmsg(z_socket, &hdr, 0);
      if (rc == -1) {
        print_zmq_error("async_reply_socket poll: ");
        continue;
      }
      j.datalen = rc;
    }
    {
      std::unique_lock<mutex> lock(queuelock);
      jobqueue.push(j);
      queuecond.signal();
    }
  }
}


void async_reply_socket::process_job(job j) {
  zmq_msg_vector query, reply;
  // deserialize query and perform the call
  iarchive iarc(j.data, j.datalen);
  iarc >> query;
  callback(query, reply);

  // release some memory
  query.clear();

  // serialize the reply
  oarchive oarc;
  oarc << reply;

  struct nn_iovec iov;
  struct nn_msghdr hdr;
  iov.iov_base = oarc.buf;
  iov.iov_len = oarc.off;
  hdr.msg_iov = &iov;
  hdr.msg_iovlen = 1;
  hdr.msg_control = reinterpret_cast<void*>(&(j.control));
  hdr.msg_controllen = NN_MSG;

  std::lock_guard<mutex> socklock(socketlock);
  int rc = nn_sendmsg(z_socket, &hdr, 0);
  free(oarc.buf);
  nn_freemsg(j.data);
  if (rc == -1) print_zmq_error("send failure : ");
}

void async_reply_socket::thread_function() {
  std::unique_lock<mutex> lock(queuelock);
  while(1) {
    while(jobqueue.empty() && !queue_terminate) {
      queuecond.wait(lock);
    }
    // at this point, we have the lock, either
    // jobqueue has something, or queue_terminate is set
    if (queue_terminate) break;
    assert(!jobqueue.empty());
    job j = jobqueue.front();
    jobqueue.pop();
    // we process the job outside of the lock
    lock.unlock();

    // this function also frees msg
    process_job(j);

    lock.lock();
  }
}




std::string async_reply_socket::get_bound_address() {
  return local_address;
}

} // namespace nanosockets
}
