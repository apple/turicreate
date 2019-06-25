/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef NANOSOCKETS_ZMQ_MSG_VECTOR_HPP
#define NANOSOCKETS_ZMQ_MSG_VECTOR_HPP
#include <cassert>
#include <string>
#include <list>
#include <cstring>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iarchive.hpp>
#include <core/export.hpp>
namespace turi {
namespace nanosockets {
/**
 * \ingroup nanosockets
 *
 * Describes a wrapper around an array of nanomsg messages part.
 * Writes are performed through the insert_back() function.
 * The standard pattern is:
 * \code
 *  // creates a new message part and returning it
 *  nn_msg_t* msg = msgvec.insert_back();
 *  zmq_msg_init_...(msg, ...)
 * \endcode
 *
 * It can also be used for reading. It maintains a read index which
 * is initialized to zero. Each call to read_next() will return the message
 * part at the readindex, and increment the read index. If there are no more
 * parts, the function will return NULL.
 *
 * The contents of the vector are automatically freed on destruction.
 *
 * This class says zmq. It was originally built as a thin wrapper around
 * zeromq but is now built using nanomsg but the interface and all names
 * are maintained for maximal backward compatibility
 */
typedef std::string nn_msg_t;
class EXPORT zmq_msg_vector {
 public:
   zmq_msg_vector():read_index(0) { }
   zmq_msg_vector(const zmq_msg_vector& other) {
     read_index = 0;
     (*this) = other;
   }
   zmq_msg_vector& operator=(const zmq_msg_vector& other) {
     clone_from(const_cast<zmq_msg_vector&>(other));
     return *this;
   }

   inline ~zmq_msg_vector() {
      clear();
   }

   void clone_from(zmq_msg_vector& other) {
     clear();
     msgs = other.msgs;
   }

   /// Returns the number of elements in the msg vector
   inline size_t size() const {
     return msgs.size();
   }

   /// Return the message at index i in the vector. Slow.
   inline const nn_msg_t* operator[](size_t i) const {
     auto iter = msgs.begin();
     while (i > 0) ++iter;
     return &(*iter);
   }

   /// Return the message at index i in the vector. Slow.
   inline nn_msg_t* operator[](size_t i) {
     auto iter = msgs.begin();
     while (i > 0) ++iter;
     return &(*iter);
   }

   /// Returns true if the vector is empty
   inline bool empty() const {
    return msgs.empty();
   }

   /**
    * Allocates a new zeromq message part, inserting it into the
    * end of the vector, and returning the message part.
    */
   inline nn_msg_t* insert_back() {
     msgs.push_back(nn_msg_t());
     return &msgs.back();
   }

   /**
    * Inserts a predefined nn_msg_t to the back
    */
   inline void insert_back(nn_msg_t& msg) {
     msgs.push_back(msg);
   }

    /**
    * Inserts a message with a given length
    */
   inline nn_msg_t* insert_back(const void* c, size_t len) {
     msgs.push_back(nn_msg_t(reinterpret_cast<const char*>(c), len));
     return &(msgs.back());
   }

   /**
    * Inserts a message with a given length
    */
   inline nn_msg_t* insert_back(const std::string& s) {
     msgs.push_back(s);
     return &(msgs.back());
   }


   /**
    * Allocates a new zeromq message part, inserting it into the
    * front of the vector, and returning the message part.
    */
   inline nn_msg_t* insert_front() {
     msgs.push_front(nn_msg_t());
     return &(msgs.front());
   }


   /**
    * Inserts a predefined nn_msg_t to the front
    */
   inline void insert_front(nn_msg_t& msg) {
     msgs.push_back(msg);
   }

   /**
    * Inserts a message with a given length
    */
   inline nn_msg_t* insert_front(const void* c, size_t len) {
     msgs.push_front(nn_msg_t(reinterpret_cast<const char*>(c), len));
     return &(msgs.front());
   }

   /**
    * Inserts a message with a given length
    */
   inline nn_msg_t* insert_front(const std::string& s) {
     msgs.push_front(s);
     return &(msgs.front());
   }



   /**
    * Returns the next unread message.
    * Returns NULL if all message have been read
    */
   inline nn_msg_t* read_next() {
     if (read_index >= size()) return NULL;
     nn_msg_t* ret = (*this)[read_index];
     ++read_index;
     return ret;
   }

   /// Returns the current read index
   inline size_t get_read_index() const {
     return read_index;
   }

   /// Returns the number of unread messages; equal to size() - read_index.
   inline size_t num_unread_msgs() const {
     return size() - get_read_index();
   }

   /// Resets the read index to 0
   inline void reset_read_index() {
     read_index = 0;
   }

   /// clears the vector and frees all the messages.
   /// Also resets the read index.
   inline void clear() {
     msgs.clear();
     read_index = 0;
   }

     /// removes an element from the front.
   inline void pop_front() {
     if (!empty()) {
       msgs.pop_front();
       // shift the read index if it not already at the head
       read_index -= (read_index > 0);
     }
   }

   /// removes an element from the back.
   inline void pop_back() {
     if (!empty()) {
       msgs.pop_back();
     }
   }

   /// removes an element from the back.
   inline nn_msg_t* back() {
     if (!empty()) {
       return &msgs.back();
     }
     return NULL;
   }

/// removes an element from the back.
   inline nn_msg_t* front() {
     if (!empty()) {
       return &msgs.front();
     }
     return NULL;
   }



   /**
    * Pops front and tries to match it against a fixed string
    * asertion failure is it does not match
    */
   inline void assert_pop_front(const void* c, size_t len) {
     assert(!empty());
     assert(msgs.front().length() == len);
     if (len > 0) {
       assert(memcmp(msgs.front().data(), c, len) == 0);
     }
     pop_front_and_free();
   }

   inline void assert_pop_front(const std::string& s) {
     assert_pop_front(s.c_str(), s.length());
   }


   inline std::string extract_front() {
     assert(!empty());
     std::string ret = std::move(msgs.front());
     pop_front_and_free();
     return ret;
   }

   inline void extract_front(void* c, size_t clen) {
     assert(!empty());
     assert(clen == msgs.front().length());
     if (clen > 0) {
       memcpy(c, msgs.front().data(), clen);
     }
     pop_front_and_free();
   }

   /// removes an element from the front and deletes it
   inline void pop_front_and_free() {
     if (!empty()) {
       pop_front();
       // shift the read index if it not already at the head
       read_index -= (read_index > 0);
     }
   }

   /**
    * Tries to send this message through a socket with a timeout
    * Returns 0 on success.
    * Returns EAGAIN if a timeout is reached.
    * Otherwise returns a zeromq error code.
    */
   int send(int socket, int timeout);

   /**
    * Tries to send this message through a ZeroMQ socket.
    * Returns 0 on success.
    * Otherwise returns a zeromq error code.
    */
   int send(int socket);

   /**
    * Tries to receive a multipart tmessage through a ZeroMQ
    * socket with a timeout
    * Returns 0 on success.
    * Returns EAGAIN if a timeout is reached.
    * Otherwise returns a zeromq error code.
    */
   int recv(int socket, int timeout);

   /**
    * Tries to receive a multipart tmessage through a ZeroMQ socket.
    * Returns 0 on success.
    * Otherwise returns a zeromq error code.
    */
   int recv(int socket);

   void save(turi::oarchive& oarc) const {
     oarc << msgs;
   }

   void load(turi::iarchive& iarc) {
     msgs.clear();
     iarc >> msgs;
   }
 private:
   // list so things are not invalidated. A number of
   // things in this class pass pointers around
   std::list<nn_msg_t> msgs;
   size_t read_index;


   /**
    * Tries to send this message through a ZeroMQ socket with a timeout
    * Returns 0 on success.
    * Returns EAGAIN if a timeout is reached.
    * Otherwise returns a zeromq error code.
    * Does not retry on EINTR
    */
   int send_impl(int socket, int timeout);

   /**
    * Tries to send this message through a ZeroMQ socket.
    * Returns 0 on success.
    * Otherwise returns a zeromq error code.
    * Does not retry on EINTR
    */
   int send_impl(int socket);

   /**
    * Tries to receive a multipart tmessage through a ZeroMQ
    * socket with a timeout
    * Returns 0 on success.
    * Returns EAGAIN if a timeout is reached.
    * Otherwise returns a zeromq error code.
    * Does not retry on EINTR
    */
   int recv_impl(int socket, int timeout);

   /**
    * Tries to receive a multipart tmessage through a ZeroMQ socket.
    * Returns 0 on success.
    * Otherwise returns a zeromq error code.
    * Does not retry on EINTR
    */
   int recv_impl(int socket);


 };

} // namespace nanosockets
}
#endif
