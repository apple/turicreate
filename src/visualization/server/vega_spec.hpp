/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_VEGA_SPEC
#define __TC_VEGA_SPEC

#include <flexible_type/flexible_type.hpp>

#include <string>
#include <sstream>

namespace turi {
  namespace visualization {

    std::string histogram_spec(const flexible_type& title, const flexible_type& xlabel, const flexible_type& ylabel, flex_type_enum dtype);
    std::string categorical_spec(const flexible_type& title, const flexible_type& xlabel, const flexible_type& ylabel, flex_type_enum dtype);
    std::string summary_view_spec(size_t length_elements);
    std::string scatter_spec(const flexible_type& xlabel, const flexible_type& ylabel, const flexible_type& title);
    std::string heatmap_spec(const flexible_type& xlabel, const flexible_type& ylabel, const flexible_type& title);
    std::string categorical_heatmap_spec(const flexible_type& xlabel, const flexible_type& ylabel, const flexible_type& title);
    std::string boxes_and_whiskers_spec(const flexible_type& xlabel, const flexible_type& ylabel, const flexible_type& title);

    // Utility for escaping JSON string literals. Not concerned with Vega implications of the contents of those strings.
    std::string make_format_string(unsigned char *raw_format_str_ptr,
                                           size_t raw_format_str_len);
    std::string format(const std::string& format_str, const std::unordered_map<std::string, std::string>& format_params);
  }
}

#endif // __TC_VEGA_SPEC
