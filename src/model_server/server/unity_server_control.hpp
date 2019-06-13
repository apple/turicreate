/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SERVER_CONTROL_H
#define TURI_UNITY_SERVER_CONTROL_H

#include <model_server/server/unity_server.hpp>
#include <model_server/server/unity_server_options.hpp>

namespace turi {
  void start_server(const unity_server_options& server_options,
      const unity_server_initializer& server_initializer = unity_server_initializer());
  void stop_server();
  void set_log_progress_callback( void (*callback)(const std::string&) );
  void set_log_progress(bool enable);
} // end of turicreate


#endif
