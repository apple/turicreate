/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_NET_UTIL_HPP
#define TURI_NET_UTIL_HPP
#include <string>
#include <stdint.h>

namespace turi {
  /**
  * \ingroup util
  * Returns the first non-localhost ipv4 address
  */
  uint32_t get_local_ip(bool print = true);

  /**
  * \ingroup util
  * Returns the first non-localhost ipv4 address as a standard dot delimited string
  */
  std::string get_local_ip_as_str(bool print = true);
  /** \ingroup util
   * Find a free tcp port and binds it. Caller must release the port.
   * Returns a pair of [port, socket handle]
   */
  std::pair<size_t, int> get_free_tcp_port();
};

#endif
