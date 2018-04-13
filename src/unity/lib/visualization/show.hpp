/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __SHOW_HPP
#define __SHOW_HPP

#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/visualization/plot.hpp>

namespace turi {
  namespace visualization {

    std::vector<toolkit_function_specification>
      get_toolkit_function_registration();

    std::shared_ptr<Plot> plot(const std::string& path_to_client,
              gl_sarray& x,
              gl_sarray& y,
              const std::string& xlabel,
              const std::string& ylabel,
              const std::string& title);

  }
}

#endif
