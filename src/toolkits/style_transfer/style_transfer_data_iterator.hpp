/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TOOLKITS_STYLE_TRANSFER_DATA_ITERATOR_H_
#define __TOOLKITS_STYLE_TRANSFER_DATA_ITERATOR_H_

#include <random>

#include <core/data/sframe/gl_sarray.hpp>

namespace turi {
namespace style_transfer {

struct st_example {
  image_type content_image;
  image_type style_image;
  size_t style_index;
};

enum st_mode {
  TRAIN = 0,
  PREDICT = 1
};

class data_iterator {
 public:
  struct parameters {
    /** The Style SArray to traverse */
    gl_sarray style;

    /** The Content SArray to traverse */
    gl_sarray content;

    /**
     * Whether to traverse the data more than once.
     */
    bool repeat = true;

    /** Whether to shuffle the data on subsequent traversals. */
    bool shuffle = true;

    /** Check whether in train or predict mode */
    enum st_mode mode = st_mode::TRAIN;

    /** Determines results of shuffle operations if enabled. */
    int random_seed = 0;
  };

  virtual ~data_iterator() = default;

  /** Returns true when `next_batch` will return a non-empty value. */
  virtual bool has_next_batch() const = 0;

  virtual std::vector<st_example> next_batch(size_t batch_size) = 0;

  virtual void reset() = 0;
};

class style_transfer_data_iterator : public data_iterator {
 public:
  style_transfer_data_iterator(const data_iterator::parameters& params);

  style_transfer_data_iterator(const style_transfer_data_iterator&) = delete;
  style_transfer_data_iterator& operator=(const style_transfer_data_iterator&) =
      delete;

  bool has_next_batch() const override {
    // TODO: gl_sframe_range::end() should be a const method.
    gl_sarray_range range_iterator(m_content_range_iterator);
    return m_content_next_row != range_iterator.end();
  }

  std::vector<st_example> next_batch(size_t batch_size) override;

  void reset() override;

 private:
  gl_sarray m_style_images;
  gl_sarray m_content_images;

  const bool m_repeat;
  const bool m_shuffle;
  const enum st_mode m_mode;

  gl_sarray_range m_content_range_iterator;
  gl_sarray_range::iterator m_content_next_row;

  std::default_random_engine m_random_engine;
};

}  // namespace style_transfer
}  // namespace turi

#endif  // __TOOLKITS_STYLE_TRANSFER_H_
