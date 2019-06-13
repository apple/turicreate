/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cassert>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/integer_traits.hpp>
#include <core/util/md5.hpp>
#include <core/util/sys_util.hpp>
#include <core/globals/globals.hpp>
#include <core/logging/logger.hpp>
#include <core/export.hpp>

extern "C" {
#include <nanomsg/nn.h>
}

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#endif
namespace turi {
namespace nanosockets {
int SEND_TIMEOUT = 3000;
int RECV_TIMEOUT = 7000;

void set_send_timeout(int ms) {
  SEND_TIMEOUT = ms;
}
void set_recv_timeout(int ms) {
  RECV_TIMEOUT = ms;
}


void set_conservative_socket_parameters(int z_socket) {
  int timeoutms = 500;
  int rcvmaxsize = -1;
  TURI_ATTRIBUTE_UNUSED_NDEBUG int rc;
  rc = nn_setsockopt(z_socket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeoutms, sizeof(timeoutms));
  assert(rc == 0);
  rc = nn_setsockopt(z_socket, NN_SOL_SOCKET, NN_SNDTIMEO, &timeoutms, sizeof(timeoutms));
  assert(rc == 0);
  rc = nn_setsockopt(z_socket, NN_SOL_SOCKET, NN_RCVMAXSIZE, &(rcvmaxsize), sizeof(rcvmaxsize));
  assert(rc == 0);

}

/**
 * Given a string, returns a zeromq localhost tcp address (ex:
 * tcp://127.15.21.22:11111).
 *
 * On windows we don't get zeromq IPC sockets. Hence, the easiest thing to do
 * is to remap ipc sockets to tcp addresses.
 *
 * When the server is started with address=default mode, on *nix systems we
 * map to ipc://something or order. Instead, on windows systems
 * we must map to tcp://[arbitrary local IP] where there is optimally,
 * a 1-1 correspondence between the local IP and the PID.
 *
 * So, here are the rules:
 * - We cannot generate any port number <= 1024
 * - Lets about 127.0.0.1 because too many stuff like to live there
 * - 127.0.0.0 is invalid. (network address)
 * - 127.255.255.255 is invalid (broadcast address)
 */
std::string hash_string_to_tcp_address(const std::string& s) {
  std::string md5sum = turi::md5_raw(s);
  // we get approximately 5 bytes of entropy (yes really somewhat less)

  unsigned char addr[4];
  addr[0] = 127;
  addr[1] = md5sum[0];
  addr[2] = md5sum[1];
  addr[3] = md5sum[2];
  uint16_t port = (uint16_t)(md5sum[3]) * 256 + md5sum[4];

  // validate
  if ((addr[1] == 0 && addr[2] == 0 && addr[3] == 0) ||  // network address
      (addr[1] == 0 && addr[2] == 0 && addr[3] == 1) ||  // common address
      (addr[1] == 255 && addr[2] == 255 && addr[3] == 255) ||  // broadcast
      (port <= 1024)) { // bad port
    // bad. this is network name
    // rehash
    return hash_string_to_tcp_address(md5sum);
  }

  // ok generate the string
  std::stringstream strm;
  strm << "tcp://" << (int)(addr[0]) << "."
                   << (int)(addr[1]) << "."
                   << (int)(addr[2]) << "."
                   << (int)(addr[3]) << ":"
                   << (int)(port);

  std::string s_out = strm.str();

  logstream(LOG_INFO) << "normalize_address: Hashed ipc address '" << s << "' to '"
                      << s_out << "'." << std::endl;
  return s_out;
}

EXPORT int64_t FORCE_IPC_TO_TCP_FALLBACK = 0;

REGISTER_GLOBAL(int64_t, FORCE_IPC_TO_TCP_FALLBACK, true);

std::string normalize_address(const std::string& address) {

  bool use_tcp_fallback = (FORCE_IPC_TO_TCP_FALLBACK != 0);

  std::string address_out;

#ifdef _WIN32
  use_tcp_fallback = true;
#endif

  if(use_tcp_fallback) {

    logstream(LOG_INFO) << "normalize_address: Using TCP fallback mode." << std::endl;

    if (boost::starts_with(address, "ipc://")) {
      address_out = hash_string_to_tcp_address(address);
    } else {
      address_out = address;
    }
  }
#ifndef _WIN32
  // sockaddr_un not defined on windows.
  else {
    /*
     *
     ipc sockets on Linux and Mac use Unix domain sockets which have a maximum
     length defined by

     #include <iostream>
     #include <sys/socket.h>
     #include <sys/un.h>

     int main() {
     struct sockaddr_un my_addr;
     std::cout << sizeof(my_addr.sun_path) << std::endl;
     }
     This appears to be 104 on Mac OS X 10.11 and 108 on a Ubuntu machine
     (length includes the null terminator).

     See http://man7.org/linux/man-pages/man7/unix.7.html
    */
    struct sockaddr_un un_addr;
    size_t max_length = sizeof(un_addr.sun_path) - 1;  // null terminator
    if (boost::starts_with(address, "ipc://") &&
        address.length() > max_length) { // strictly this leaves a 5 char buffer
      // since we didn't strip the ipc://
      // we hash it to a file we put in /tmp
      // we could use $TMPDIR but that might be a bad idea.
      // since $TMPDIR could bump the length much bigger again.
      // with /tmp the length is bounded to strlen("/tmp") + md5 length.
      std::string md5_hash = turi::md5(address);
      address_out = "ipc:///tmp/" + md5_hash;
    } else {
      address_out = address;
    }
  }
#else
  else {
    address_out = address;
  }
#endif

  if(address_out == address) {
    logstream(LOG_INFO) << "normalize_address: kept '" << address_out << "'." << std::endl;
  } else {
    logstream(LOG_INFO) << "normalize_address: '" << address << "' --> '"
                        << address_out << "'." << std::endl;
  }

  return address_out;
}

}
}
