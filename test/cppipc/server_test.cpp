/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/cppipc.hpp>
#include "test_object_base.hpp"

int main(int argc, char** argv) {
  //cppipc::comm_server server({"localhost:2181"}, "test");
  cppipc::comm_server server({}, "","tcp://127.0.0.1:19000");
  /*
  cppipc::comm_server server({}, 
                             "",
                             "ipc:///tmp/cppipc_server_test");
                             */

  server.register_type<test_object_base>([](){ 
                                           return new test_object_impl;
                                         });
  server.start();
  getchar();
}
