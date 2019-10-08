/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/style_transfer_data_iterator.hpp>

#include <core/data/image/io.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <ml/neural_net/image_augmentation.hpp>
#include <model_server/lib/image_util.hpp>

namespace turi {
namespace style_transfer {

namespace {

flex_image get_image(const flexible_type& image_feature) {
  if (image_feature.get_type() == flex_type_enum::STRING) {
    return read_image(image_feature, /* format_hint */ "");
  } else {
    return image_feature;
  }
}

gl_sarray ensure_encoded(const gl_sarray& sa) {
  if (sa.dtype() == flex_type_enum::IMAGE)
    return sa.apply(image_util::encode_image, flex_type_enum::IMAGE);
  return sa;
}

}

style_transfer_data_iterator::style_transfer_data_iterator(
    const data_iterator::parameters& params)
    : m_style_images(ensure_encoded(params.style)),
      m_content_images(ensure_encoded(params.content)),
      m_repeat(params.repeat),
      m_shuffle(params.shuffle),
      m_content_range_iterator(m_content_images.range_iterator()),
      m_content_next_row(m_content_range_iterator.begin()),
      m_random_engine(params.random_seed) {}

std::vector<st_example> style_transfer_data_iterator::next_batch(
    size_t batch_size) {

  std::vector<std::tuple<flexible_type, flexible_type, flexible_type>>
      raw_batch;
  raw_batch.reserve(batch_size);

  while (raw_batch.size() < batch_size && m_content_next_row != m_content_range_iterator.end()) {
    const turi::flexible_type& content_image = *m_content_next_row;

    std::uniform_int_distribution<size_t> dist(0, m_style_images.size() - 1);
    size_t random_style_index = dist(m_random_engine);
    const flexible_type& style_image = m_style_images[random_style_index];

    raw_batch.emplace_back(content_image, style_image, random_style_index);

    if (++m_content_next_row == m_content_range_iterator.end() && m_repeat) {
      if (m_shuffle) {
        gl_sarray indices = gl_sarray::from_sequence(0, m_content_images.size());
        std::uniform_int_distribution<uint64_t> dist(0);
        uint64_t random_mask = dist(m_random_engine);
        auto randomize_indices = [random_mask](const flexible_type& x) {
          uint64_t masked_index = random_mask ^ x.to<uint64_t>();
          return flexible_type(hash64(masked_index));
        };

        gl_sframe temp_content({{"content", m_content_images}});
        temp_content.add_column(
            indices.apply(randomize_indices, flex_type_enum::INTEGER, false),
            "_random_order");

        temp_content = temp_content.sort("_random_order");
        m_content_images = temp_content["content"];
      }
    }
  }

  std::vector<st_example> result(raw_batch.size());

  for (size_t i = 0; i < raw_batch.size(); ++i) {
    flexible_type content_image, style_image, style_index;
    std::tie(content_image, style_image, style_index) = raw_batch[i];

    result[i].style_image = get_image(style_image);
    result[i].content_image = get_image(content_image);
    result[i].style_index = style_index.get<flex_int>();
  }

  return result;
}

}  // namespace style_transfer
}  // namespace turi