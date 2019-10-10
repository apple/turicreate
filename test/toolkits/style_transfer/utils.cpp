/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include "utils.hpp"

#include <model_server/lib/image_util.hpp>

namespace turi {
namespace style_transfer {

std::string generate_data(size_t data_size) {
  std::string data_array;
  data_array.reserve(data_size);
  for (size_t x = 0; x < data_size; x++)
    data_array[x] = static_cast<char>((uint8_t)(rand() % 256));

  return data_array;
}

turi::flex_image random_image() {
  size_t height = ((size_t)(rand() % 10) + 15);
  size_t width = ((size_t)(rand() % 10) + 15);
  size_t channels = 3;

  size_t data_size = height * width * channels;

  size_t image_type_version = IMAGE_TYPE_CURRENT_VERSION;
  size_t format = 2;

  std::string img_data = generate_data(data_size);

  return turi::image_type(img_data.c_str(), height, width, channels, data_size,
                          image_type_version, format);
}

turi::gl_sarray random_image_sarray(size_t length) {
  std::vector<turi::flexible_type> image_column_data;
  for (size_t x = 0; x < length; x++)
    image_column_data.push_back(random_image());

  turi::gl_sarray sa;
  sa.construct_from_vector(image_column_data, turi::flex_type_enum::IMAGE);

  return sa.apply(turi::image_util::encode_image, turi::flex_type_enum::IMAGE);;
}

turi::gl_sframe random_sframe(size_t length, std::string image_column_name) {
  turi::gl_sarray image_sa = random_image_sarray(length);
  turi::gl_sframe image_sf;
  image_sf.add_column(image_sa, image_column_name);

  return image_sf;
}

} // namespace style_transfer
} // namespace turi