/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/log_level_setter.hpp>

log_level_setter::log_level_setter(int loglevel) {
  prev_level =  global_logger().get_log_level();
  global_logger().set_log_level(loglevel);
}

log_level_setter::~log_level_setter() {
  global_logger().set_log_level(prev_level);
}
