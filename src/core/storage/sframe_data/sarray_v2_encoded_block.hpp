/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_ENCODED_BLOCK_HPP
#define TURI_SFRAME_ENCODED_BLOCK_HPP

#include <boost/circular_buffer.hpp>
#include <vector>
#include <memory>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sarray_v2_block_types.hpp>
namespace turi {


/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * SFrame v2 Format Implementation Detail
 */
namespace v2_block_impl {

class encoded_block_range;
struct typed_decode_stream;
/**
 * This class provides accessors into a typed v2
 * sarray<flexible_type> encoded column block. It maintains the
 * block in a compressed state, and stream decodes it.
 *
 * The encoded block object is copyable, move constructable,
 * copy assignable, move assignable. Copies are cheap and free
 * as it only needs to copy a single shared pointer.
 */
class encoded_block {
 public:
  /// Default constructor. Does nothing.
  encoded_block();

  /// Default Copy constructor
  encoded_block(const encoded_block&) = default;
  /// Default Move constructor
  encoded_block(encoded_block&&) = default;
  /// Default Copy assignment
  encoded_block& operator=(const encoded_block&) = default;
  /// Default Move assignment
  encoded_block& operator=(encoded_block&&) = default;

  /// block constructor from data contents; simply calls init().
  encoded_block(block_info info, std::vector<char>&& data) {
    init(info, std::move(data));
  }


  /// block constructor from data contents; simply calls init().
  encoded_block(block_info info, std::shared_ptr<std::vector<char> > data) {
    init(info, data);
  }

  /**
   * Initializes this block to point to new data.
   *
   * Existing ranges are NOT invalidated.
   * They will continue to point to what they used to point to.
   * \param info The block information structure
   * \param data The binary data
   */
  void init(block_info info, std::vector<char>&& data);


  /**
   * Initializes this block to point to new data.
   *
   * Existing ranges are NOT invalidated.
   * They will continue to point to what they used to point to.
   * \param info The block information structure
   * \param data The binary data
   */
  void init(block_info info, std::shared_ptr<std::vector<char> > data);

  /**
   * Returns an accessor to the contents of the block.
   *
   * The range is *not* concurrent. But independent ranges can be accessed
   * in parallel safely.
   */
  encoded_block_range get_range();

  /**
   * Release the block object. All acquired ranges are stil valid.
   */
  void release();

  inline size_t size() const {
    return m_size;
  }

  const block_info get_block_info() const {
    return m_block.m_block_info;
  }

  std::shared_ptr<std::vector<char> > get_block_data() const {
    return m_block.m_data;
  }

  friend class encoded_block_range;

 private:


  struct block {
    /// The block information. Needed for the decode.
    block_info m_block_info;
    /// The actual block data.
    std::shared_ptr<std::vector<char> > m_data;
  };

  block m_block;
  size_t m_size = 0;
}; // class encoded_block


/**
 * The range returned by \ref encoded_block::get_range().
 * It provides 2 basic methods. \ref decode(target, size) and \ref skip(n)
 *
 * The encoded_block_range provides a one pass reader to the data.
 *
 * The encoded_block_range holds its own pointers to the data and hence
 * is not invalidated by destruction or reassignment of the originating
 * encoded_block object.
 *
 * The range is *not* concurrent.
 */
class encoded_block_range {
 private:
 public:
  encoded_block_range() = default;
  explicit encoded_block_range(const encoded_block& block);

  encoded_block_range(const encoded_block_range&) = delete;
  encoded_block_range(encoded_block_range&&) = default;
  encoded_block_range& operator=(const encoded_block_range&) = delete;
  encoded_block_range& operator=(encoded_block_range&&) = default;

  /**
   * Decodes the next num_elem elements into the decode_target.
   * Returns the number of elements read, i.e.
   * while(num_elem) {
   *   (*decode_target) = next_value;
   *   ++decode_target;
   *   --num_elem;
   * }
   */
  size_t decode_to(flexible_type* decode_target, size_t num_elem);
  void skip(size_t n);
  void release();

  ~encoded_block_range();

 private:

  encoded_block::block m_block;
  std::unique_ptr<typed_decode_stream> decoder;

}; // encoded_block_range



} // v2_block_impl

/// \}
} // namespace turi
#endif
