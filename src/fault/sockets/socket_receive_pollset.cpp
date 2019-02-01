/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <fault/sockets/socket_receive_pollset.hpp>


namespace libfault {
socket_receive_pollset::socket_receive_pollset() {
  poll_thread_started = false;
  contended = false;
  poll_thread = NULL;
  last_trigger_time = time(NULL);
}

socket_receive_pollset::~socket_receive_pollset() {
  stop_poll_thread();
}

void socket_receive_pollset::poll(int timems) {
  boost::lock_guard<boost::recursive_mutex> guard(poll_lock);
  // reset pollset
  int rc = zmq_poll(pollset.data(), pollset.size(), timems);
  if (rc > 0) {
    for (size_t i = 0; i < pollset.size(); ++i) {
      // we have stuff to receive here
      if (pollset[i].revents == ZMQ_POLLIN) {
        callbacks[i](this, pollset[i]);
        // reset revents
        pollset[i].revents = 0;
      }
    }
  }
  time_t t = ::time(NULL);
  if (t > last_trigger_time) {
    //std::cout << "tick\n";
    last_trigger_time = t;
    zmq_pollitem_t empty;
    empty.events = 0; empty.fd = 0; empty.revents = 0; empty.socket = NULL;
    for (size_t i = 0; i < timerset.size(); ++i) {
      timerset[i].second(this, empty);
    }
  }
}

void socket_receive_pollset::add_timer_item(void* tag,
                                            const callback_type& callback) {
  contended = true;
  boost::lock_guard<boost::recursive_mutex> guard(poll_lock);
  contended = false;
  timerset.resize(timerset.size() + 1);
  timerset[timerset.size() - 1].first = tag;
  timerset[timerset.size() - 1].second = callback;
}

void socket_receive_pollset::remove_timer_item(void* tag) {
  contended = true;
  boost::lock_guard<boost::recursive_mutex> guard(poll_lock);
  contended = false;
  for (size_t i = 0;i < timerset.size(); ++i) {
    if (timerset[i].first == tag) {
      // O(1) delete
      if (timerset.size() > 1 && i != timerset.size() - 1) {
        timerset[i] = *timerset.rbegin();
      }
      timerset.erase(timerset.end() - 1);
    }
  }
}


void socket_receive_pollset::add_pollitem(
    const zmq_pollitem_t& item, const callback_type& callback) {
  contended = true;
  boost::lock_guard<boost::recursive_mutex> guard(poll_lock);
  contended = false;
  zmq_pollitem_t pollitem = item;
  pollitem.revents = 0;
  pollitem.events = ZMQ_POLLIN;
  pollset.push_back(pollitem);
  callbacks.push_back(callback);
}

bool socket_receive_pollset::remove_pollitem(const zmq_pollitem_t& item) {
  contended = true;
  boost::lock_guard<boost::recursive_mutex> guard(poll_lock);
  contended = false;
  // loop though all the items and deleting from the pollset the
  // sockets which match.
  // Returns true only if all items are deleted.
  for (size_t j = 0;j < pollset.size(); ++j) {
    if ((item.socket && pollset[j].socket == item.socket) ||
        (item.fd && pollset[j].fd == item.fd)) {
      // move the last element to the front
      // O(1) delete
      if (pollset.size() > 1 && j != pollset.size() - 1) {
        pollset[j] = *pollset.rbegin();
        callbacks[j] = *callbacks.rbegin();
      }
      pollset.erase(pollset.end() - 1);
      callbacks.erase(callbacks.end() - 1);
      return true;
    }
  }
  return false;
}


void socket_receive_pollset::start_poll_thread() {
  boost::lock_guard<boost::recursive_mutex> guard(poll_lock);
  if (poll_thread_started) return;
  poll_thread_started = true;
  boost::thread::attributes attrs;
  attrs.set_stack_size(8*1024*1024); // 8MB
  poll_thread = new boost::thread(attrs, boost::bind(&socket_receive_pollset::poll_loop, this));

}

void socket_receive_pollset::stop_poll_thread() {
  if (poll_thread_started) {
    poll_thread_started = false;
    asm volatile("" ::: "memory");
    poll_thread->join();
    delete poll_thread;
    poll_thread = NULL;
  }
}



void socket_receive_pollset::poll_loop() {
  while(poll_thread_started) {
    poll(200);
    while (contended) {
#if _WIN32
      // The Windows CriticalSection (which boost recursive mutex uses) is
      // unfair. Apparently a 10 microsecond sleep doesn't send the polling
      // thread to the back of the queue as effectively as this does on
      // Windows. This keeps the polling thread from starving stuff that needs
      // to do actual work.
      sched_yield();
#else
      std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif
    }
  }
}
} // namespace libfault
