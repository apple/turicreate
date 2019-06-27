/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SERVER_OPTIONS_HPP
#define TURI_UNITY_SERVER_OPTIONS_HPP

#include <string>

namespace turi {

struct unity_server_options {
  std::string log_file;
  std::string root_path;
  bool daemon = false;
  size_t log_rotation_interval = 0;
  size_t log_rotation_truncate = 0;
};


} // end of namespace turi
#endif
