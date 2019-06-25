/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <process/process.hpp>
#include <core/logging/logger.hpp>

namespace turi {

std::string process::read_from_child() {
  const size_t BUF_SIZE = 4096;
  char buf[BUF_SIZE];
  ssize_t bytes_read;
  std::stringstream msg;
  while( (bytes_read = read_from_child(buf, BUF_SIZE)) > 0 ) {
    msg << std::string(buf, buf + bytes_read);
  }

  std::string ret = msg.str();
  if(bytes_read == -1) {
    logstream(LOG_WARNING) <<
      "Error reading from child, message may be partial " << "(" <<
      ret.size() << " bytes received)." << std::endl;
  }

  return ret;
}

} // namespace turi
