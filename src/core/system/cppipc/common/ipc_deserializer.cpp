/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/server/comm_server.hpp>
#include <core/system/cppipc/client/comm_client.hpp>
#include <core/system/cppipc/common/ipc_deserializer.hpp>
#include <core/export.hpp>
namespace cppipc {
namespace detail {
static __thread comm_server* thlocal_server = NULL;
static __thread comm_client* thlocal_client = NULL;
EXPORT void set_deserializer_to_server(comm_server* server) {
 thlocal_server = server;
 thlocal_client = NULL;
}
EXPORT void set_deserializer_to_client(comm_client* client) {
  thlocal_client = client;
  thlocal_server = NULL;
}


EXPORT void get_deserialization_type(comm_server** server, comm_client** client) {
  (*server) = thlocal_server;
  (*client) = thlocal_client;
}


EXPORT std::shared_ptr<void> get_server_object_ptr(comm_server* server, size_t object_id) {
  return server->get_object(object_id);
}

} // detail
} // cppipc
