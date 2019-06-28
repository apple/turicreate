/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_SARRAY_V2_TYPE_ENCODING_HPP
#define TURI_SFRAME_SARRAY_V2_TYPE_ENCODING_HPP
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sarray_v2_block_types.hpp>
#include <core/util/basic_types.hpp>
#include <core/util/dense_bitset.hpp>
#include <core/storage/sframe_data/integer_pack.hpp>
#include <core/util/coro.hpp>
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
using namespace turi::integer_pack;

static const size_t MAX_INTEGERS_PER_BLOCK = 128;
static const size_t MAX_DOUBLES_PER_BLOCK = 512;

/**
 * Encodes a collection of numbers in data, skipping all UNDEFINED values.
 * It simply loops through the data, collecting a block of up to
 * MAX_INTEGERS_PER_BLOCK numbers and calls frame_of_reference_encode_128()
 * on it.
 *
 * \note The coding does not store the number of values stored. The decoder
 * \ref decode_number() requires the number of values to decode correctly.
 */
void encode_number(block_info& info,
                   oarchive& oarc,
                   const std::vector<flexible_type>& data);

/**
 * Decodes a collection of numbers into 'data'. Entries in data which are
 * of type flex_type_enum::UNDEFINED will be skipped, and there must be exactly
 * num_undefined number of them. It simply decodes a block using
 * frame_of_reference_decode_128() and fills in data with it.
 *
 * \note We have an explicit implementation here that is equivalent to
 * decode_number_stream for performance reasons since this is a *very* commonly
 * encountered function.
 */
void decode_number(iarchive& iarc,
                   std::vector<flexible_type>& ret,
                   size_t num_undefined);

/**
 * Encodes a collection of doubles in data, skipping all UNDEFINED values.
 * It simply loops through the data, collecting a block of up to
 * MAX_INTEGERS_PER_BLOCK numbers and calls frame_of_reference_encode_128()
 * on it.
 *
 * This is the 2nd generation vector decoder. its use is flagged by
 * turning on the block flag BLOCK_ENCODING_EXTENSION.
 *
 * \note The coding does not store the number of values stored. The decoder
 * \ref decode_number() requires the number of values to decode correctly.
 */
void encode_double(block_info& info,
                   oarchive& oarc,
                   const std::vector<flexible_type>& data);

/**
 * Decodes a collection of doubles into 'data'. Entries in data which are
 * of type flex_type_enum::UNDEFINED will be skipped, and there must be exactly
 * num_undefined number of them.
 *
 *
 * This is the 2nd generation floating point encoder. its use is flagged by
 * turning on the block flag BLOCK_ENCODING_EXTENSION.
 * The format is basically:
 * - one byte: encoding format. LEGACY, or INTEGER.
 * If LEGACY:
 *   The old encoder is used
 * If INTEGER:
 *   The floating point values are encoded as integers.
 */
void decode_double(iarchive& iarc,
                   std::vector<flexible_type>& ret,
                   size_t num_undefined);

/**
 * Decodes a collection of doubles into 'data'. Entries in data which are
 * of type flex_type_enum::UNDEFINED will be skipped, and there must be exactly
 * num_undefined number of them. It simply decodes a block using
 * frame_of_reference_decode_128() and fills in data with it.
 *
 * \note We have an explicit implementation here that is equivalent to
 * decode_number_stream for performance reasons since this is a *very* commonly
 * encountered function.
 */
void decode_double_legacy(iarchive& iarc,
                          std::vector<flexible_type>& ret,
                          size_t num_undefined);

/**
 * Decodes a collection of flexible_type values. The array must be of
 * contiguous type, but permitting undefined values.
 *
 * See \ref typed_encode() for details.
 *
 * \note The coding does not store the number of values stored. This is
 * stored in the block_info (block.num_elem)
 */
bool typed_decode(const block_info& info,
                  char* start, size_t len,
                  std::vector<flexible_type>& ret);


/**
 * Encodes a collection of flexible_type values. The array must be of
 * contiguous type, but permitting undefined values.
 *
 * There is a two byte header to the block.
 * - num_types: 1 byte
 *     - if 0, the block is empty.
 *     - if 1, the array is of contiguous type (see next byte)
 *     - if 2, the array is of contiguous type, but has missing values.
 * - type: 1 byte.
 * - [undefined bitfield]: if type is 2, this contains a bitfield of
 *   (round_op(#elem / 8) bytes) listing the positions of all the UNDEFINED
 *   fields)
 * - type specific encoding:
 *     - if integer or float, encode_number() is called
 *     - if string, encode_string() is called
 *     - otherwise, direct serialization is currently used.
 *     - If UNDEFINED (i.e. array is of all UNDEFINED values, nothing is written)
 *
 * \note The coding does not store the number of values stored. This is
 * stored in the block_info (block.num_elem)
 */
void typed_encode(const std::vector<flexible_type>& data,
                  block_info& info,
                  oarchive& oarc);


/**
 * Handle a stream decoding of integers.
 *
 * read acts as a coroutine where it is called multiple times to decode
 * up to num_elements values from the archive. For each call either
 * decodebuffer is set, or skip is set. At least 1 value must be read/skipped
 * or there will be problems. num_elements and iarc should be the same on
 * every call.
 */
struct decode_number_stream {
  DECL_CORO_STATE(read);
  uint64_t buf[MAX_INTEGERS_PER_BLOCK];
  size_t num_elements;
  size_t buflen;
  size_t i;
  bool __brk;

  size_t read(size_t num_elements,
              iarchive& iarc,
              const std::pair<flexible_type*, size_t>& decodebuffer,
              size_t skip);
};


/**
 * Handle a stream decoding of double values (Old format).
 *
 * read acts as a coroutine where it is called multiple times to decode
 * up to num_elements values from the archive. For each call either
 * decodebuffer is set, or skip is set. At least 1 value must be read/skipped
 * or there will be problems. num_elements and iarc should be the same on
 * every call.
 */
struct decode_double_stream_legacy{
  DECL_CORO_STATE(read);
  size_t num_elements;
  uint64_t buf[MAX_INTEGERS_PER_BLOCK];
  size_t buflen;
  size_t i;
  turi::flexible_type ret;
  bool __brk;

  inline decode_double_stream_legacy():ret(0.0) { }

  bool read(size_t num_elements,
            iarchive& iarc,
            const std::pair<flexible_type*, size_t>& decodebuffer,
            size_t skip);
};


/**
 * Handle a stream decoding of double values (new format).
 *
 * read acts as a coroutine where it is called multiple times to decode
 * up to num_elements values from the archive. For each call either
 * decodebuffer is set, or skip is set. At least 1 value must be read/skipped
 * or there will be problems. num_elements and iarc should be the same on
 * every call.
 */
struct decode_double_stream{
  DECL_CORO_STATE(read);
  size_t num_elements;

  char reserved;
  decode_double_stream_legacy legacy;
  uint64_t buf[MAX_INTEGERS_PER_BLOCK];
  size_t buflen;
  size_t i;
  bool __brk;

  bool read(size_t num_elements,
            iarchive& iarc,
            const std::pair<flexible_type*, size_t>& decodebuffer,
            size_t skip);
};

/**
 * Handle a stream decoding of string values (new format).
 *
 * read acts as a coroutine where it is called multiple times to decode
 * up to num_elements values from the archive. For each call either
 * decodebuffer is set, or skip is set. At least 1 value must be read/skipped
 * or there will be problems. num_elements and iarc should be the same on
 * every call.
 */
struct decode_string_stream{
  DECL_CORO_STATE(read);
  size_t num_elements;
  bool use_dictionary_encoding = false;
  std::vector<flexible_type> idx_values;
  uint64_t num_values;
  std::vector<flexible_type> str_values;
  flexible_type ret;
  size_t i;
  bool __brk;

  inline decode_string_stream():ret(flex_type_enum::STRING) { }

/**
 * Decodes num_elements of strings , calling the callback for each string.
 */
  bool read(size_t num_elements,
            iarchive& iarc,
            const std::pair<flexible_type*, size_t>& decodebuffer,
            size_t skip);
};

/**
 * Handle a stream decoding of vector values.
 *
 * read acts as a coroutine where it is called multiple times to decode
 * up to num_elements values from the archive. For each call either
 * decodebuffer is set, or skip is set. At least 1 value must be read/skipped
 * or there will be problems. num_elements and iarc should be the same on
 * every call.
 *
 * This is the 2nd generation vector decoder. its use is flagged by
 * turning on the block flag BLOCK_ENCODING_EXTENSION.
 */
struct decode_vector_stream {
  DECL_CORO_STATE(read);
  size_t num_elements;
  char reserved;
  std::vector<flexible_type> lengths;
  size_t total_num_values = 0;
  std::vector<flexible_type> values;
  size_t length_ctr = 0;
  size_t value_ctr = 0;
  flexible_type ret;
  size_t i,j;
  bool __brk;

  inline decode_vector_stream():ret(flex_type_enum::VECTOR) { }

  bool read(size_t num_elements,
            iarchive& iarc,
            const std::pair<flexible_type*, size_t>& decodebuffer,
            size_t skip,
            bool new_format);
};


/**
 * Handle a stream decoding of ndvector values.
 *
 * read acts as a coroutine where it is called multiple times to decode
 * up to num_elements values from the archive. For each call either
 * decodebuffer is set, or skip is set. At least 1 value must be read/skipped
 * or there will be problems. num_elements and iarc should be the same on
 * every call.
 *
 * This is the 2nd generation vector decoder. its use is flagged by
 * turning on the block flag BLOCK_ENCODING_EXTENSION.
 */
struct decode_ndvector_stream {
  DECL_CORO_STATE(read);
  size_t num_elements;
  char reserved;
  std::vector<flexible_type> shape_lengths;
  std::vector<flexible_type> numel;
  std::vector<flexible_type> shapes;
  std::vector<flexible_type> strides;
  std::vector<flexible_type> values;
  size_t sum_shape_len = 0;
  size_t sum_values_len = 0;
  size_t shape_stride_ctr = 0;
  size_t value_ctr = 0;
  std::vector<size_t> ret_shape;
  std::vector<size_t> ret_stride;
  size_t ret_numel;
  std::shared_ptr<flex_nd_vec::container_type> ret_values;
  size_t i, j;
  bool __brk;
  /**
   * Decodes num_elements of nd_vectors, calling the callback for each string.
   */
  bool read(size_t num_elements,
            iarchive& iarc,
            const std::pair<flexible_type*, size_t>& decodebuffer,
            size_t skip,
            bool new_format);

};


/**
 * Handle a stream decoding of flexible_type values.
 *
 * read acts as a coroutine where it is called multiple times to decode
 * flexible_type values from a block. For each call either
 * decodebuffer is set, or skip is set.
 *
 * This is the 2nd generation vector decoder. its use is flagged by
 * turning on the block flag BLOCK_ENCODING_EXTENSION.
 */
struct typed_decode_stream {
  DECL_CORO_STATE(read);
  block_info info;
  char* start;
  size_t len;
  turi::iarchive iarc;

  size_t dsize;
  flex_type_enum column_type;
  size_t num_undefined;
  turi::dense_bitset undefined_bitmap;
  char num_types;
  bool perform_type_decoding;
  size_t i;
  std::vector<flexible_type> values;
  size_t last_id = 0;
  size_t elements_to_decode;
  size_t undefined_elem_consumed;
  bool __brk;

  decode_number_stream* number_decoder = nullptr;
  decode_double_stream* double_decoder = nullptr;
  decode_double_stream_legacy* double_legacy_decoder = nullptr;
  decode_string_stream* string_decoder = nullptr;
  decode_vector_stream* vector_decoder = nullptr;
  decode_ndvector_stream* ndvector_decoder = nullptr;

  flexible_type_impl::deserializer generic_deserializer;
  flexible_type generic_ret;
  typed_decode_stream(const block_info& info,
                      char* start, size_t len);

  ~typed_decode_stream();

  /**
   * Decodes a collection of flexible_type values.
   *
   * decodebuffer points to a target location and length. skip is the
   * number of elements to skip. Either
   *
   * 1) decodebuffer.first != nullptr, decodebuffer.second > 0, skip == 0
   * OR
   * 2) decodebuffer.first == nullptr, decodebuffer.second == 0, skip > 0
   *
   * This method can be called repeatedly to extract more values from the
   * buffer, but note that the buffer is 1 pass only.  caller must make sure
   * to not read more than the actual number of values in the block, or bad
   * things will happen.
   *
   * \note The coding does not store the number of values stored. This is
   * stored in the block_info (block.num_elem)
   *
   * Returns the number of actual values skipped or decoded
   * (excluding undefined values).
   */
  size_t read(const std::pair<flexible_type*, size_t>& decodebuffer, size_t skip);
 private:
  size_t pad_retbuf_with_undefined_positions(const std::pair<flexible_type*, size_t>& decodebuffer);

};

} // namespace v2_block_impl

/// \}
} // namespace turi

#endif
