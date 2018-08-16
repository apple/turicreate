/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_IPC_DESERIALIZER_HPP
#define CPPIPC_IPC_DESERIALIZER_HPP

#ifdef DISABLE_TURI_CPPIPC_PROXY_GENERATION
#include <cppipc/common/ipc_deserializer_minimal.hpp>

#else

#include <type_traits>
#include <serialization/iarchive.hpp>
#include <cppipc/ipc_object_base.hpp>
namespace cppipc {

class comm_server;
class comm_client;

namespace detail {
extern void set_deserializer_to_server(comm_server* server);
extern void set_deserializer_to_client(comm_client* client);
extern void get_deserialization_type(comm_server** server, comm_client** client);
extern std::shared_ptr<void> get_server_object_ptr(comm_server* server, size_t object_id);

} // detail
} // cppipc


// This is using an undocumented feature of the serializer to overwrite the 
// deserializer for when attempting to deserialize T where T inherits from
// ipc_object_base. This will allow me to transport proxied objects
// across the network, even if the proxied object is stored inside another 
// object.
namespace turi {
namespace archive_detail {

// by templating over Server, we detach the cyclic dependency between
// this file and comm_server
// Tries to find an object on the server, and registers it if it is not found.
// Returns objectID
template <typename Server, typename T>
size_t get_server_object_id(Server* server, std::shared_ptr<T> objectptr) {
  return server->register_object(objectptr);
}



template <typename OutArcType, typename T>
struct serialize_impl<OutArcType, std::shared_ptr<T>, false,
    typename std::enable_if<std::is_convertible<T*, cppipc::ipc_object_base*>::value>::type
    > {
  inline static 
      void
      exec(OutArcType& oarc, const std::shared_ptr<T> value) {
    // check that the object has been registered on the server size
    cppipc::comm_server* server;
    cppipc::comm_client* client;
    cppipc::detail::get_deserialization_type(&server, &client);
    if (server) {
      // server to client message is an "object ID"
      oarc << get_server_object_id(server, value);
    } else {
      oarc << (*value);
    }
  }
};


template <typename InArcType, typename T>
struct deserialize_impl<InArcType, std::shared_ptr<T>, false,
    typename std::enable_if<std::is_convertible<T*, cppipc::ipc_object_base*>::value>::type
    > {
  inline static 
      void exec(InArcType& iarc, std::shared_ptr<T>& value) {
    cppipc::comm_server* server;
    cppipc::comm_client* client;
    cppipc::detail::get_deserialization_type(&server, &client);
    if (server) {
      size_t object_id;
      iarc >> object_id;
      std::shared_ptr<void> obj = cppipc::detail::get_server_object_ptr(server, object_id);
      if (obj == NULL) {
        throw std::to_string(object_id) + " Object not found";
      }
      value = std::static_pointer_cast<T>(obj);
    } else if (client) {
      size_t object_id;
      iarc >> object_id;
      value.reset(new typename T::proxy_object_type(*client, false, object_id));
    }
  }
};
} // archive_detail
} // turicreate
#endif
#endif
