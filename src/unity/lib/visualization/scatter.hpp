/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/lib/gl_sarray.hpp>

namespace turi {
namespace visualization {

  void show_scatter(const std::string& path_to_client,
                    const gl_sarray& x,
                    const gl_sarray& y,
                    const std::string& xlabel,
                    const std::string& ylabel,
                    const std::string& title);

}}
