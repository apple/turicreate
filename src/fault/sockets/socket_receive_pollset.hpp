/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef FAULT_SOCKETS_SOCKET_RECEIVE_POLLSET_HPP
#define FAULT_SOCKETS_SOCKET_RECEIVE_POLLSET_HPP
#include <time.h>
#include <boost/function.hpp>
#include <zmq.h>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <export.hpp>
namespace libfault {

/**
 * \ingroup fault
 * A receive multiplexor.
 */  
class EXPORT socket_receive_pollset {
  public:  
   typedef boost::function<void(socket_receive_pollset*, const zmq_pollitem_t&)> callback_type;
   socket_receive_pollset();
   ~socket_receive_pollset();

   /**
    * Registers a poll item which when triggered will make the callback
    * Either the "socket", or "fd" field in the item must be filled.
    */
   void add_pollitem(const zmq_pollitem_t& item, const callback_type& callback);


   /**
    * Registers a callback which is triggered approximately every 1 second.
    * The tag is used to uniquely identify the item.
    * The pollitem passed into the callback will have every field zeroed.
    */
   void add_timer_item(void* tag, const callback_type& callback);

   /**
    * Removes a poll item. 
    * Either the "socket", or "fd" field in the item must be filled.
    * It will match based on these two fields, whichever is non-zero.
    */
   bool remove_pollitem(const zmq_pollitem_t& item);

   /**
    * Registers a callback which is triggered approximately every 1 second.
    */
   void remove_timer_item(void* tag);


   /** polls for a certain amount of time.
    * all callbacks will be issued within this thread.
    * Not safe to call if a poll thread is running.
    */
   void poll(int timems);


   /** spawns a background polling thread.
    * Note that a certain amount of care has to be done when
    * this is used. Particularly, all dependent sockets should be locked
    * appropriately since the zeromq socket implementations are not thread safe.
    *
    * It is not safe to have a poll thread and call poll().
    */
   void start_poll_thread();

   /** Stops the polling thread
    */ 
   void stop_poll_thread();
  private:
   std::vector<zmq_pollitem_t> pollset;
   std::vector<std::pair<void*, callback_type> > timerset;
   std::vector<callback_type> callbacks;
    
   boost::thread* poll_thread;
   volatile bool poll_thread_started;
   volatile bool contended;
   boost::recursive_mutex poll_lock;

   void poll_loop();
   time_t last_trigger_time;
};

} // libfault
#endif

