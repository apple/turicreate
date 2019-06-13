/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/server/comm_server.hpp>
#include <unistd.h>

int main(int argc, char** argv) {
  //cppipc::comm_server server({"localhost:2181"}, "pingtest");
  cppipc::comm_server server({}, "", "tcp://127.0.0.1:19000");
  //cppipc::comm_server server({}, "ipc:///tmp/ping_server_test");
  server.start();
  getchar();
  server.stop();
}
