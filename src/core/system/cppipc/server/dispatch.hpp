/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_SERVER_DISPATCH_HPP
#define CPPIPC_SERVER_DISPATCH_HPP
#include <core/storage/serialization/serialization_includes.hpp>

namespace cppipc {
class comm_server;
/**
 * \internal
 * \ingroup cppipc
 * The base class for the function dispatch object.
 * The function dispatch object wraps a member function pointer and the
 * execute call then calls the member function pointer using the objectptr as
 * the object, and the remaining arguments are deserialized from the iarchive.
 * The result of the call are serialized in the response archive.
 *
 * The restrictions are the the function call must not take any
 * arguments by reference.
 */
struct dispatch {
  virtual void execute(void* objectptr,
                       comm_server* server,
                       turi::iarchive& msg,
                       turi::oarchive& response) = 0;
  virtual ~dispatch() { }
};





} // cppipc

#endif
