/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include "style_transfer_data_iterator.hpp"

#include <core/data/image/io.hpp>
#include <model_server/lib/image_util.hpp>

namespace turi {
namespace style_transfer {

using neural_net::image_annotation;
using neural_net::labeled_image;
using neural_net::shared_float_array;

flex_image get_image(const flexible_type& image_feature) {
  if (image_feature.get_type() == flex_type_enum::STRING) {
    return read_image(image_feature, /* format_hint */ "");
  } else {
    return image_feature;
  }
}

std::vector<flexible_type> get_style(const data_iterator::parameters& params) {
  gl_sarray style_images = params.style[params.style_column_name];

  if (style_images.dtype() == flex_type_enum::IMAGE) {
    style_images =
        style_images.apply(image_util::encode_image, flex_type_enum::IMAGE);
  }

  gl_sframe result({{params.style_column_name, style_images}});

  std::vector<flexible_type> style_batch;
  style_batch.reserve(result.size());

  for (const auto& i : result.range_iterator()) {
    const flexible_type& style_image = i[0];
    style_batch.emplace_back(style_image);
  }

  return style_batch;
}

gl_sframe get_content(const data_iterator::parameters& params) {
  gl_sarray content_images = params.style[params.content_column_name];

  if (content_images.dtype() == flex_type_enum::IMAGE) {
    content_images =
        content_images.apply(image_util::encode_image, flex_type_enum::IMAGE);
  }

  gl_sframe result({{params.content_column_name, content_images}});

  return result;
}

style_transfer_data_iterator::style_transfer_data_iterator(
    const parameters& params)
    : m_style_vector(get_style(params)),
      m_content(get_content(params)),
      m_style_column_name(params.style_column_name),
      m_content_column_name(params.content_column_name),
      m_repeat(params.repeat),
      m_shuffle(params.shuffle),
      m_content_range_iterator(m_content.range_iterator()),
      m_content_next_row(m_content_range_iterator.begin()),
      m_random_engine(params.random_seed) {}

std::vector<st_image> style_transfer_data_iterator::next_batch(
    size_t batch_size) {
  std::vector<std::tuple<flexible_type, flexible_type, flexible_type>>
      raw_batch;
  raw_batch.reserve(batch_size);

  while (raw_batch.size() < batch_size &&
         m_content_next_row != m_content_range_iterator.end()) {
    const sframe_rows::row& content_row = *m_content_next_row;

    std::uniform_int_distribution<size_t> dist(0, m_style_vector.size() - 1);
    size_t random_style_index = dist(m_random_engine);
    const flexible_type& style_image = m_style_vector.at(random_style_index);

    raw_batch.emplace_back(content_row[0], style_image, random_style_index);

    if (++m_content_next_row == m_content_range_iterator.end() && m_repeat) {
      if (m_shuffle) {
        gl_sarray indices = gl_sarray::from_sequence(0, m_content.size());
        std::uniform_int_distribution<uint64_t> dist(0);
        uint64_t random_mask = dist(m_random_engine);
        auto randomize_indices = [random_mask](const flexible_type& x) {
          uint64_t masked_index = random_mask ^ x.to<uint64_t>();
          return flexible_type(hash64(masked_index));
        };
        m_content.add_column(
            indices.apply(randomize_indices, flex_type_enum::INTEGER, false),
            "_random_order");
        m_content = m_content.sort("_random_order");
        m_content.remove_column("_random_order");
      }
    }
  }

  std::vector<st_image> result(raw_batch.size());

  for (size_t i = 0; i < raw_batch.size(); ++i) {
    flexible_type content_image, style_image, style_index;
    std::tie(content_image, style_image, style_index) = raw_batch[i];

    result[i].style = get_image(style_image);
    result[i].content = get_image(content_image);
    result[i].index = style_index.get<flex_int>();
  }

  return result;
}

}  // namespace style_transfer
}  // namespace turi