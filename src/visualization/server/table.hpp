/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_VISUALIZATION_TABLE
#define __TC_VISUALIZATION_TABLE

#include <string>
#include <sstream>

namespace turi {

  class sframe_reader;
  class unity_sframe;

  namespace visualization {
    std::string table_spec(const std::shared_ptr<unity_sframe>& table, const std::string& title, std::string table_id = "");
    std::string table_data(const std::shared_ptr<unity_sframe>& table, sframe_reader* reader, size_t start, size_t end);
    std::string table_accordion(const std::shared_ptr<unity_sframe>& table, const std::string& column_name, size_t row_idx);
    std::string image_png_data(const flex_image& image, size_t resized_height);
  }

}

#endif // __TC_VISUALIZATION_TABLE
