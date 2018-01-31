/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_VEGA_SPEC
#define __TC_VEGA_SPEC

#include <string>
#include <sstream>
#include <export.hpp>

namespace turi {
  namespace visualization {

    std::string histogram_spec(std::string title, std::string xlabel, std::string ylabel, double sizeMultiplier = 1.0);
    std::string categorical_spec(size_t length_list, std::string title, std::string xlabel, std::string ylabel, double sizeMultiplier = 1.0);
    std::string summary_view_spec(size_t length_elements, double sizeMultiplier = 1.0);
    std::string scatter_spec(std::string xlabel = "", std::string ylabel = "", std::string title = "");
    std::string heatmap_spec(std::string xlabel = "", std::string ylabel = "", std::string title = "");
    std::string categorical_heatmap_spec(std::string xlabel = "", std::string ylabel = "", std::string title = "");
    std::string boxes_and_whiskers_spec(std::string xlabel = "", std::string ylabel = "", std::string title = "");

    // Utility for escaping JSON string literals. Not concerned with Vega implications of the contents of those strings.
    std::string escape_string(const std::string& str);

  }
}

#endif // __TC_VEGA_SPEC
