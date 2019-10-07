/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TOOLKITS_STYLE_TRANSFER_H_
#define __TOOLKITS_STYLE_TRANSFER_H_

#include <functional>
#include <map>
#include <memory>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>

namespace turi {
namespace style_transfer {

struct st_image {
  image_type style;
  image_type content;
  size_t index;
};

class data_iterator {
 public:
  struct parameters {
    /** The Style SFrame to traverse */
    gl_sframe style;

    /** The Content SFrame to traverse */
    gl_sframe content;

    /**
     * The name of the column containing the style images.
     */
    std::string style_column_name;

    /**
     * The name of the column containing the content images.
     */
    std::string content_column_name;

    /**
     * Whether to traverse the data more than once.
     */
    bool repeat = true;

    /** Whether to shuffle the data on subsequent traversals. */
    bool shuffle = true;

    /** Determines results of shuffle operations if enabled. */
    int random_seed = 0;
  };

  virtual ~data_iterator() = default;

  virtual std::vector<st_image> next_batch(size_t batch_size) = 0;
};

class style_transfer_data_iterator : public data_iterator {
  style_transfer_data_iterator(const parameters& params);

  style_transfer_data_iterator(const style_transfer_data_iterator&) = delete;
  style_transfer_data_iterator& operator=(const style_transfer_data_iterator&) =
      delete;

  std::vector<st_image> next_batch(size_t batch_size) override;

 private:
  std::vector<flexible_type> m_style_vector;
  gl_sframe m_content;

  std::string m_style_column_name;
  std::string m_content_column_name;

  const bool m_repeat;
  const bool m_shuffle;

  gl_sframe_range m_content_range_iterator;
  gl_sframe_range::iterator m_content_next_row;

  std::default_random_engine m_random_engine;
};

}  // namespace style_transfer
}  // namespace turi

#endif  // __TOOLKITS_STYLE_TRANSFER_H_