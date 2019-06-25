/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_SARRAY_V2_BLOCK_TYPES_HPP
#define TURI_SFRAME_SARRAY_V2_BLOCK_TYPES_HPP
#include <stdint.h>
#include <tuple>
#include <core/storage/serialization/serializable_pod.hpp>
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


/**
 * Types of blocks
 */
enum BLOCK_FLAGS {
  LZ4_COMPRESSION = 1,
  IS_FLEXIBLE_TYPE = 2,
  MULTIPLE_TYPE_BLOCK = 4,
  BLOCK_ENCODING_EXTENSION = 8  // used to flag secondary compression schemes
};

/**
 * Floating point encoding formats
 */
namespace DOUBLE_RESERVED_FLAGS {
enum FLAGS {
  LEGACY_ENCODING = 0,
  INTEGER_ENCODING = 1
};
}

/**
 * Vector encoding formats
 */
namespace VECTOR_RESERVED_FLAGS {
enum FLAGS {
  NEW_ENCODING = 0
};
}
/**
 * A column address is a tuple of segment_id,
 * column number within the segment
 */
typedef std::tuple<size_t, size_t> column_address;

/**
 * A block address is a tuple of segment_id,
 * column number, block number within the segment
 */
typedef std::tuple<size_t, size_t, size_t> block_address;

/**
 * Metadata about each block
 */
struct block_info: public turi::IS_POD_TYPE {
  uint64_t offset = (uint64_t)(-1); /// The file offsets of the block
  uint64_t length = 0; /// The length of the block in bytes on disk
  /**
   * The decompressed length of the block in bytes
   * on disk. Only different from length if the block is LZ4_compressed.
   */
  uint64_t block_size = 0;
  uint64_t num_elem = 0; /// The number of elements in the block
  uint64_t flags = 0;  /// block flags
  /**
   * If flags & IS_FLEXIBLE_TYPE, the type of the contents.
   * This is really of type flex_type_enum
   */
  uint16_t content_type = 0;
};
} // v2_block_impl

/// \}
} // turi

#endif
