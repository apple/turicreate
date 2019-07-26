/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <functional>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sarray_v2_block_types.hpp>
#include <core/storage/sframe_data/sarray_v2_type_encoding.hpp>
#include <core/util/dense_bitset.hpp>
#include <core/storage/sframe_data/integer_pack.hpp>
#include <core/util/coro.hpp>

namespace turi {
namespace v2_block_impl {
using namespace turi::integer_pack;

void encode_number(block_info& info,
                   oarchive& oarc,
                   const std::vector<flexible_type>& data) {
  for (size_t i = 0;i < data.size(); ) {
    uint64_t encode_buf[MAX_INTEGERS_PER_BLOCK];
    size_t encode_buflen = 0;
    // collect a block of 128 integers
    while(i < data.size() && encode_buflen < MAX_INTEGERS_PER_BLOCK) {
      if (data[i].get_type() != flex_type_enum::UNDEFINED) {
        encode_buf[encode_buflen] = data[i].get<flex_int>();
        ++encode_buflen;
      }
      ++i;
    }
    if (encode_buflen == 0) break;
    //     logstream(LOG_INFO) << "Encoding ";
    //     for (size_t i = 0;i < encode_buflen; ++i) {
    //       logstream(LOG_INFO) << " " << encode_buf[i];
    //     }
    //     logstream(LOG_INFO) << std::endl;
    frame_of_reference_encode_128(encode_buf, encode_buflen, oarc);
  }
}


void decode_number(iarchive& iarc,
                   std::vector<flexible_type>& ret,
                   size_t num_undefined) {
  uint64_t buf[MAX_INTEGERS_PER_BLOCK];
  size_t bufstart = 0;
  size_t buflen = 0;
  size_t num_values_to_read = ret.size() - num_undefined;
  for (size_t i = 0;i < ret.size(); ++i) {
    if (ret[i].get_type() != flex_type_enum::UNDEFINED) {
      if (bufstart < buflen) {
        ret[i].reinterpret_mutable_get<flex_int>() = buf[bufstart];
        ++bufstart;
        --num_values_to_read;
      } else {
        buflen = std::min<size_t>(num_values_to_read, MAX_INTEGERS_PER_BLOCK);
        // decode a new block of 128 integers
        frame_of_reference_decode_128(iarc, buflen, buf);
//         logstream(LOG_INFO) << "Decoding ";
//         for (size_t i = 0;i < buflen; ++i) {
//           logstream(LOG_INFO) << " " << buf[i];
//         }
//         logstream(LOG_INFO) << std::endl;
        bufstart = 0;
        ret[i].reinterpret_mutable_get<flex_int>() = buf[bufstart];
        ++bufstart;
        --num_values_to_read;
      }
    }
  }
}


/**
 * Encodes a collection of doubles in data, skipping all UNDEFINED values.
 * It simply loops through the data, collecting a block of up to
 * MAX_INTEGERS_PER_BLOCK numbers and calls frame_of_reference_encode_128()
 * on it.
 *
 * \note The coding does not store the number of values stored. The decoder
 * \ref decode_number() requires the number of values to decode correctly.
 */
void encode_double_legacy(block_info& info,
                          oarchive& oarc,
                          const std::vector<flexible_type>& data) {
  for (size_t i = 0;i < data.size(); ) {
    uint64_t encode_buf[MAX_INTEGERS_PER_BLOCK];
    size_t encode_buflen = 0;
    // collect a block of 128 integers
    while(i < data.size() && encode_buflen < MAX_INTEGERS_PER_BLOCK) {
      if (data[i].get_type() != flex_type_enum::UNDEFINED) {
        encode_buf[encode_buflen] = data[i].reinterpret_get<flex_int>();
        ++encode_buflen;
      }
      ++i;
    }
    if (encode_buflen == 0) break;
    // perform a left rotate on all the numbers.
    // Basically, doubles are stored as sign-and-magnitude. This means
    // that -1.0 looks *very* different from -1.
    for (size_t j = 0;j < encode_buflen; ++j) {
      encode_buf[j] = (encode_buf[j] << 1) | (encode_buf[j] >> 63);
    }
    frame_of_reference_encode_128(encode_buf, encode_buflen, oarc);
  }
}


void encode_double(block_info& info,
                   oarchive& oarc,
                   const std::vector<flexible_type>& data) {
  // we reserve one character so we can add new encoders as needed in the future
  char reserved = 0;
  bool safe_for_integer_code = true;
  for (size_t i = 0;i < data.size(); ++i) {
    if (data[i].get_type() == flex_type_enum::FLOAT) {
      flex_float val = data[i].get<flex_float>();
      flex_int int_val = (flex_int)val;
      flex_float new_float_val = int_val;
      if (new_float_val != val) {
        safe_for_integer_code = false;
        break;
      }
    }
  }
  if (safe_for_integer_code) {
    reserved = DOUBLE_RESERVED_FLAGS::INTEGER_ENCODING;
  } else {
    reserved = DOUBLE_RESERVED_FLAGS::LEGACY_ENCODING;
  }
  oarc.write(&(reserved), sizeof(reserved));
  if (reserved == DOUBLE_RESERVED_FLAGS::LEGACY_ENCODING) {
    encode_double_legacy(info, oarc, data);
    return;
  } else if (reserved == DOUBLE_RESERVED_FLAGS::INTEGER_ENCODING) {
    std::vector<flexible_type> copy = data;
    for (auto& i : copy) {
      if (i.get_type() == flex_type_enum::FLOAT) {
        i = (flex_int)(i.get<flex_float>());
      }
    }
    encode_number(info, oarc, copy);
    return;
  }
}


void decode_double_legacy(iarchive& iarc,
                   std::vector<flexible_type>& ret,
                   size_t num_undefined) {
  uint64_t buf[MAX_INTEGERS_PER_BLOCK];
  size_t bufstart = 0;
  size_t buflen = 0;
  size_t num_values_to_read = ret.size() - num_undefined;

  for (size_t i = 0;i < ret.size(); ++i) {
    if (ret[i].get_type() != flex_type_enum::UNDEFINED) {
      if (bufstart < buflen) {
        ret[i].reinterpret_mutable_get<flex_int>() = buf[bufstart];
        ++bufstart;
        --num_values_to_read;
      } else {
        buflen = std::min<size_t>(num_values_to_read, MAX_INTEGERS_PER_BLOCK);
        // decode a new block of 128 integers
        frame_of_reference_decode_128(iarc, buflen, buf);
        // right rotate
        for (size_t j = 0;j < buflen ; ++j) {
          buf[j] = (buf[j] >> 1) | (buf[j] << 63);
        }
        bufstart = 0;
        ret[i].reinterpret_mutable_get<flex_int>() = buf[bufstart];
        ++bufstart;
        --num_values_to_read;
      }
    }
  }
}

void decode_double(iarchive& iarc,
                   std::vector<flexible_type>& ret,
                   size_t num_undefined) {
  // we reserve one character so we can add new encoders as needed in the future
  char reserved = 0;
  iarc.read(&(reserved), sizeof(reserved));
  ASSERT_LT(reserved, 3);
  if (reserved == DOUBLE_RESERVED_FLAGS::LEGACY_ENCODING) {
    decode_double_legacy(iarc, ret, num_undefined);
    return;
  } else if (reserved == DOUBLE_RESERVED_FLAGS::INTEGER_ENCODING) {
    for (auto& i : ret) {
      if (i.get_type() == flex_type_enum::FLOAT) i.reset(flex_type_enum::INTEGER);
    }
    decode_number(iarc, ret, num_undefined);
    for (auto& i : ret) {
      if (i.get_type() == flex_type_enum::INTEGER) {
        i = (flex_float)(i.get<flex_int>());
      }
    }
    return;
  }
}

/**
 * Encodes a collection of strings in data, skipping all UNDEFINED values.
 *
 * Two encoding strategies are used.
 * Strategy 1:
 * Dictionary encode:
 *  - A dictionary of unique strings are built, and an array of numbers
 *    mapping to the string values are constructed.
 *     - variable_encode(dictionary length)
 *     - for each entry in dictionary:
 *         - variable_encode entry length
 *         - write bytes contents for each entry
 *     - encode_number(dictionary mapping)
 * Strategy 2:
 * Direct encode:
 *  - encode_number(lengths of all the strings)
 *  - for each entry:
 *     - write byte contents for each entry
 *
 * \note The coding does not store the number of values stored. The decoder
 * \ref decode_string() requires the number of values to decode correctly.
 */
static void encode_string(block_info& info,
                          oarchive& oarc,
                          const std::vector<flexible_type>& data) {
  bool use_dictionary_encoding = true;
  std::unordered_map<std::string, size_t> unique_values;
  std::vector<flexible_type> idx_values;
  std::vector<std::string> str_values;
  idx_values.resize(data.size(), flexible_type(flex_type_enum::INTEGER));
  size_t idxctr = 0;
  for (size_t i = 0;i < data.size(); ++i) {
    if (data[i].get_type() != flex_type_enum::UNDEFINED) {
      auto iter = unique_values.find(data[i].get<std::string>());
      if (iter != unique_values.end()) {
        idx_values[idxctr++].mutable_get<flex_int>() = iter->second;
      } else {
        // if we have too many unique values, fail.
        if (unique_values.size() >= 64) {
          use_dictionary_encoding = false;
          break;
        }
        size_t newidx = unique_values.size();
        unique_values[data[i].get<std::string>()] = newidx;
        str_values.push_back(data[i].get<std::string>());
        idx_values[idxctr++].mutable_get<flex_int>() = newidx;
      }
    }
  }
  oarc << use_dictionary_encoding;
  if (use_dictionary_encoding) {
    idx_values.resize(idxctr);

    variable_encode(oarc, str_values.size());
    for (auto& str: str_values) {
      variable_encode(oarc, str.length());
      oarc.write(str.c_str(), str.length());
    }
    encode_number(info, oarc, idx_values);
  } else {
    // encode all the lengths
    idxctr = 0;
    for (auto& f: data) {
      if (f.get_type() != flex_type_enum::UNDEFINED) {
        idx_values[idxctr++].mutable_get<flex_int>() = f.get<std::string>().length();
      }
    }
    idx_values.resize(idxctr);
    encode_number(info, oarc, idx_values);
    for (auto& f: data) {
      if (f.get_type() != flex_type_enum::UNDEFINED) {
        oarc.write(f.get<std::string>().c_str(), f.get<flex_string>().length());
      }
    }
  }
}

/**
 * Decodes a collection of strings into 'data'. Entries in data which are
 * of type flex_type_enum::UNDEFINED will be skipped, and there must be exactly
 * num_undefined number of them. It simply decodes a block using
 * frame_of_reference_decode_128() and fills in data with it.
 */
static void decode_string(iarchive& iarc,
                          std::vector<flexible_type>& ret,
                          size_t num_undefined) {
  decode_string_stream strm;
  strm.read(ret.size() - num_undefined, iarc,
            {ret.data(), ret.size()}, 0);
}

/**
 * Encodes a collection of vectors in data, skipping all UNDEFINED values.
 *
 *  - encode a list of integers with all the vector lengths
 *  - encode a list of all the doubles
 *
 * \note The coding does not store the number of values stored. The decoder
 * \ref decode_vector() requires the number of values to decode correctly.
 */
static void encode_vector(block_info& info,
                          oarchive& oarc,
                          const std::vector<flexible_type>& data) {
  char reserved = VECTOR_RESERVED_FLAGS::NEW_ENCODING;
  oarc.write(&(reserved), sizeof(reserved));
  // length of each vector
  std::vector<flexible_type> lengths;
  // values of all the vectors concatted together
  std::vector<flexible_type> values;

  for (size_t i = 0;i < data.size(); ++i) {
    if (data[i].get_type() != flex_type_enum::UNDEFINED) {
      const flex_vec& vals = data[i].get<flex_vec>();
      lengths.push_back(vals.size());
      for (const double d: vals) {
        values.push_back(d);
      }
    }
  }
  encode_number(info, oarc, lengths);
  encode_double(info, oarc, values);
}

/**
 * Decodes a collection of vectors in data, skipping all UNDEFINED values.
 * Wrapper around decode_number_stream
 */
static void decode_vector(iarchive& iarc,
                          std::vector<flexible_type>& ret,
                          size_t num_undefined,
                          bool new_format) {
  decode_vector_stream strm;
  strm.read(ret.size() - num_undefined, iarc,
            {ret.data(), ret.size()},
            0,
            new_format);
}


/**
 * Encodes a collection of ndvectors in data, skipping all UNDEFINED values.
 *
 *  - encode a list of integers with all the shape lengths (or equivalently stride lengths)
 *  - encode a list of integers with the number of elements in each ndarray
 *  - encode a flattened list of integers with all the shapes
 *  - encode a flattened list of integers with all the strides
 *  - encode a flattened list of all the doubles
 *
 * \note The coding does not store the number of values stored. The decoder
 * \ref decode_vector() requires the number of values to decode correctly.
 */
static void encode_nd_vector(block_info& info,
                          oarchive& oarc,
                          const std::vector<flexible_type>& data) {
  char reserved = VECTOR_RESERVED_FLAGS::NEW_ENCODING;
  // length of each vector
  std::vector<flexible_type> shape_lengths;
  std::vector<flexible_type> numel;
  std::vector<flexible_type> shapes;
  std::vector<flexible_type> strides;
  std::vector<flexible_type> values;

  // a temporary value to hold compacted ndarrays if necessary.
  flex_nd_vec tempval;

  for (size_t i = 0;i < data.size(); ++i) {
    if (data[i].get_type() != flex_type_enum::UNDEFINED) {
      const flex_nd_vec* val = &(data[i].get<flex_nd_vec>());
      ASSERT_TRUE(val->is_valid());
      // if it is not full, compact it.
      // and move the val pointer to point to the tempval
      if (!val->is_full()) {
        tempval = val->compact();
        val = &tempval;
      }

      shape_lengths.push_back(val->shape().size());
      numel.push_back(val->elements().size());
      for (auto d: val->shape()) shapes.push_back(d);
      for (auto d: val->stride()) strides.push_back(d);
      for (auto d: val->elements()) values.push_back(d);
    }
  }
  oarc.write(&(reserved), sizeof(reserved));
  encode_number(info, oarc, shape_lengths);
  encode_number(info, oarc, numel);
  encode_number(info, oarc, shapes);
  encode_number(info, oarc, strides);
  encode_double(info, oarc, values);
}

/**
 * Decodes a collection of ndvectors in data, skipping all UNDEFINED values.
 * Wrapper around decode_number_stream
 */
static void decode_nd_vector(iarchive& iarc,
                          std::vector<flexible_type>& ret,
                          size_t num_undefined,
                          bool new_format) {
  decode_ndvector_stream strm;
  strm.read(ret.size() - num_undefined, iarc,
            {ret.data(), ret.size()},
            0,
            new_format);
}

void typed_encode(const std::vector<flexible_type>& data,
                  block_info& block,
                  oarchive& oarc) {
  block.flags |= IS_FLEXIBLE_TYPE;
  block.num_elem = data.size();

  // figure out how many types there are in the array
  turi::fixed_dense_bitset<16> types_appeared;
  types_appeared.clear();
  for (size_t i = 0;i < data.size(); ++i) {
    types_appeared.set_bit_unsync((char)data[i].get_type());
  }

  // write one byte for the number of types in the block
  char num_types = types_appeared.popcount();
  oarc << num_types;
  bool perform_type_encoding = true;
  if (num_types == 0) {
    // no types. no array. nothing. quit
    block.block_size = oarc.off;
    return;
  } if (num_types == 1) {
    // One type. array has only one type
    oarc << (char)data[0].get_type();
    // entire block is of UNDEFINED values. quit.
    if (data[0].get_type() == flex_type_enum::UNDEFINED) {
      block.block_size = oarc.off;
      return;
    }
  } else if (num_types == 2 && types_appeared.get((char)flex_type_enum::UNDEFINED)) {
    // two types, one of them must be UNDEFINED
    // write that type.
    for(auto t: types_appeared) {
      if ((flex_type_enum)t != flex_type_enum::UNDEFINED) {
        oarc << (char)(t);
        break;
      }
    }
    // then write a bit field containing the positions of the UNDEFINED bits.
    turi::dense_bitset d(data.size());
    d.clear();
    for (size_t i = 0;i < data.size(); ++i) {
      if (data[i].get_type() == flex_type_enum::UNDEFINED) d.set_bit_unsync(i);
    }
    oarc.write((char*)d.array, sizeof(size_t) * d.arrlen);
  } else {
    oarc << data;
    perform_type_encoding = false;
    block.flags |= MULTIPLE_TYPE_BLOCK;
  }
  if (perform_type_encoding) {
    if (types_appeared.get((char)flex_type_enum::INTEGER)) {
      encode_number(block, oarc, data);
    } else if(types_appeared.get((char)flex_type_enum::FLOAT)) {
      block.flags |=  BLOCK_ENCODING_EXTENSION;
      encode_double(block, oarc, data);
    } else if (types_appeared.get((char)flex_type_enum::STRING)) {
      encode_string(block, oarc, data);
    } else if (types_appeared.get((char)flex_type_enum::VECTOR)) {
      block.flags |=  BLOCK_ENCODING_EXTENSION;
      encode_vector(block, oarc, data);
    } else if (types_appeared.get((char)flex_type_enum::ND_VECTOR)) {
      block.flags |=  BLOCK_ENCODING_EXTENSION;
      encode_nd_vector(block, oarc, data);
    } else {
      flexible_type_impl::serializer s{oarc};
      for (size_t i = 0;i < data.size(); ++i) {
        if (data[i].get_type() != flex_type_enum::UNDEFINED) {
          data[i].apply_visitor(s);
        }
      }
    }
  }
  block.block_size = oarc.off;
}

bool typed_decode(const block_info& info,
                  char* start, size_t len,
                  std::vector<flexible_type>& ret) {
  if (!(info.flags & IS_FLEXIBLE_TYPE)) {
    logstream(LOG_ERROR) << "Attempting to decode a non-typed block"
                         << std::endl;
    return false;
  }
  turi::iarchive iarc(start, len);

  size_t dsize = info.num_elem;
  ret.resize(dsize);
  char num_types; iarc >> num_types;
  flex_type_enum column_type;
  size_t num_undefined = 0;
  // if it is a multiple type block, we don't perform a type decode
  bool perform_type_decoding = !(info.flags & MULTIPLE_TYPE_BLOCK);
  if (perform_type_decoding) {
    if (num_types == 0) {
      // empty block
      return true;
    } else if (num_types == 1) {
      //  one block of contiguous type. quit
      char c;
      iarc >> c;
      column_type = (flex_type_enum)c;
      for (size_t i = 0;i < dsize; ++i) {
        ret[i].reset(column_type);
      }
      // all undefined. quit
      if (column_type == flex_type_enum::UNDEFINED) return true;
    } else if (num_types == 2) {
      // two types, but with undefined entries.
      char c;
      iarc >> c;
      column_type = (flex_type_enum)c;
      for (size_t i = 0;i < dsize; ++i) {
        ret[i].reset(column_type);
      }
      // read the bitset and undefine all the flagged entries
      turi::dense_bitset d(info.num_elem);
      d.clear();
      iarc.read((char*)d.array, sizeof(size_t)*d.arrlen);
      for (auto t: d) {
        ret[t].reset(flex_type_enum::UNDEFINED);
      }
      num_undefined = d.popcount();
    } else {
      logstream(LOG_ERROR) << "Unexpected value for num_types: "
                           << static_cast<int>(num_types)
                           << " (expected 0, 1, or 2)" << std::endl;
      return false;
    }
  } else {
    iarc >> ret;
  }
  if (perform_type_decoding) {
    // type decode
    if (column_type == flex_type_enum::INTEGER) {
      decode_number(iarc, ret, num_undefined);
    } else if (column_type == flex_type_enum::FLOAT) {
      if (info.flags & BLOCK_ENCODING_EXTENSION) {
        decode_double(iarc, ret, num_undefined);
      } else {
        decode_double_legacy(iarc, ret, num_undefined);
      }
    } else if (column_type == flex_type_enum::STRING) {
      decode_string(iarc, ret, num_undefined);
    } else if (column_type == flex_type_enum::VECTOR) {
      decode_vector(iarc, ret, num_undefined,
                    info.flags & BLOCK_ENCODING_EXTENSION);
    } else if (column_type == flex_type_enum::ND_VECTOR) {
      decode_nd_vector(iarc, ret, num_undefined,
                    info.flags & BLOCK_ENCODING_EXTENSION);
    } else {
      flexible_type_impl::deserializer s{iarc};
      for (size_t i = 0;i < dsize; ++i) {
        if (ret[i].get_type() != flex_type_enum::UNDEFINED) {
          ret[i].apply_mutating_visitor(s);
        }
      }
    }
  }

  if (ret.size() != info.num_elem) {
    logstream(LOG_ERROR) << "Unexpected number of elements read. "
                         << "Read " << ret.size()
                         << ". Expecting " << info.num_elem << std::endl;
    return false;
  }
  return true;
}


/**************************************************************************/
/*                                                                        */
/*                             Stream Classes                             */
/*                                                                        */
/**************************************************************************/
/*
*/

/**
 * Puts a value into decodebuffer, or skips a value.
 * These macros here are extremely subtle and should not be messed with.
 *
 * Abstractly, this does something very simple.
 *  - put a value into the decodebuffer, skipping undefined and advancing decode_bufpos
 *  - OR decrement skip
 *  - yield when we run out of buffer or run out of skip.
 *
 * However, what took a while to get right is that on resumption of
 * the coroutine, the state of decode_bufpos, decode_buffer and skip can be
 * arbitrary. We may also not find a place in our buffer to put the new value,
 * before we have to yield.
 *
 * To make this work, the 2 key invariants to keep in mind is that:
 *  - We yield we when we run out of buffer space
 *    decode_bufpos >= decodebuffer.second, skip == 0
 *  - The loop keeps running until we consumed the value.
 *
 * Note about the buffer space invariant. Technically, the full form is that
 * buffer is full if
 * \code
 * (decode_buffer.first != nullptr && decode_bufpos == decodebuffer.second) ||
 * (decode_buffer.first == nullptr && skip == 0)
 * \endcode
 *
 * However, enforcing the following 2 constraints:
 *   1) IF decode_buffer.first == nullptr THEN decode_buffer.second == 0
 *   2) IF decode_buffer.first != nullptr THEN skip == 0 on entry
 *
 * We can first generalize the constraint as
 * \code
 * (decode_buffer.first != nullptr && decode_bufpos == decodebuffer.second) ||
 * (decode_buffer.first == nullptr && skip == 0)
 *
 * <==
 * // no harm expanding the bounds check
 * (decode_buffer.first != nullptr && decode_bufpos >= decodebuffer.second) ||
 * (decode_buffer.first == nullptr && skip == 0)
 *
 * <==>
 *
 * // adding decode_bufpos >= 0 does nothing here since it is size_t anyway
 * (decode_buffer.first != nullptr && decode_bufpos >= decodebuffer.second) ||
 * (decode_buffer.first == nullptr && decode_bufpos >= 0 && skip == 0)
 *
 * <==>
 *
 * // Constraint 1, decodebuffer.second == 0 if I am skipping. Substitute.
 * (decode_buffer.first != nullptr && decode_bufpos >= decodebuffer.second) ||
 * (decode_buffer.first == nullptr && decode_bufpos >= decodebuffer.second && skip == 0)
 *
 * <==>
 *
 * // Constraint 2, skip == 0 if I am not skipping. So adding useless constraint
 * (decode_buffer.first != nullptr && decode_bufpos >= decodebuffer.second && skip == 0) ||
 * (decode_buffer.first == nullptr && decode_bufpos >= decodebuffer.second && skip == 0)
 *
 * <==>
 * // simplify
 * decode_bufpos >= decodebuffer.second && skip == 0
 * \endcode
 *
 * Funnily it took me a little time to formalize this, but when I wrote it
 * in code the first time, it was quite intuitive...
 */
#define PUT_BUFFER(x) \
   while(1) { \
     __brk = false; \
     if (skip == 0) { \
       if (decodebuffer.first[decode_bufpos].get_type() != flex_type_enum::UNDEFINED) {  \
         decodebuffer.first[decode_bufpos] = (x);  \
         __brk = true; \
       } \
       ++decode_bufpos; \
     } else { \
       --skip; \
       ++decode_bufpos;  \
       __brk = true; \
     } \
     if (decode_bufpos >= decodebuffer.second && skip == 0) { CORO_YIELD(decode_bufpos); } \
     if (__brk) break; \
   }

/**
 * Puts a value into decodebuffer and skips multiple values.
 * These macros here are extremely subtle and should not be messed with.
 * Also see the documentation in \ref PUT_BUFFER for details.
 *
 * This is like \ref PUT_BUFFER, but takes an additional IDX and LIMIT argument
 * which allows for value skipping. IDX must be a simple variable, as this
 * may work to increment it.
 *
 * Once again the key invariants are
 *  - We yield we when we run out of buffer space
 *    decode_bufpos >= decodebuffer.second, skip == 0
 *  - The loop keeps running until we consumed the value.
 *
 *  Note that in the skipping part, we see IDX += elem_to_skip - 1
 *  The "- 1" is because we expect there to be a normal IDX increment since
 *  we "consumed" one value. For instance, in the non-skip case, we do not
 *  advance IDX since we expect the caller to be advancing IDX in a separate
 *  place. Hence similarly here, we advance IDX by 1 less to account for the
 *  current element we are supposed to be consuming.
 *
 *  An alternative way to write this which makes it clearer is:
 *  ```
 *  else {
 *     // skip one element. Just like in PUT_BUFFER
 *     skip--;
 *     decode_bufpos++;
 *     __brk = true;
 *
 *     // calculate how many more we might be able to skip
 *     size_t more_skip = std::min<size_t>(LIMIT - IDX, skip);
 *     if (more_skip) {
 *       IDX += more_skip;
 *       skip -= more_skip;
 *       decode_bufpos += more_skip;
 *     }
 *  }
 *  ```
 */
#define PUT_BUFFER_SKIP(x, IDX, LIMIT) \
   while(1) { \
     __brk = false; \
     if (skip == 0) { \
       if (decodebuffer.first[decode_bufpos].get_type() != flex_type_enum::UNDEFINED) {  \
         decodebuffer.first[decode_bufpos] = (x);  \
         __brk = true; \
       } \
       ++decode_bufpos; \
     } else { \
       size_t elem_to_skip = std::min<size_t>(LIMIT - IDX, skip); \
       if (elem_to_skip > 0) { \
         IDX += elem_to_skip - 1; \
         skip -= elem_to_skip; \
         decode_bufpos += elem_to_skip; \
         __brk = true; \
       } \
     } \
     if (decode_bufpos >= decodebuffer.second && skip == 0) { CORO_YIELD(decode_bufpos); } \
     if (__brk) break; \
   }


size_t decode_number_stream::read(size_t _num_elements,
                                  iarchive& iarc,
                                  const std::pair<flexible_type*, size_t>& decodebuffer,
                                  size_t skip) {
  size_t decode_bufpos = 0;
  CORO_BEGIN(read)
  num_elements = _num_elements;
  while(num_elements > 0) {
    buflen = std::min<size_t>(num_elements, MAX_INTEGERS_PER_BLOCK);
    frame_of_reference_decode_128(iarc, buflen, buf);
    for (i = 0;i < buflen; ++i) {
      PUT_BUFFER_SKIP(flexible_type(buf[i]), i, buflen);
    }
    num_elements -= buflen;
  }
  CORO_END;
  return decode_bufpos;
}


bool decode_double_stream_legacy::read(size_t _num_elements,
          iarchive& iarc,
          const std::pair<flexible_type*, size_t>& decodebuffer,
          size_t skip) {
  size_t decode_bufpos = 0;
  CORO_BEGIN(read)
  num_elements = _num_elements;
  while(num_elements > 0) {
    buflen = std::min<size_t>(num_elements, MAX_INTEGERS_PER_BLOCK);
    frame_of_reference_decode_128(iarc, buflen, buf);
    for (i = 0;i < buflen; ++i) {
      {
        size_t intval = (buf[i] >> 1) | (buf[i] << 63);
        // make a double flexible_type
        ret.reinterpret_mutable_get<flex_int>() = intval;
      }
      PUT_BUFFER_SKIP(ret,i,buflen);
    }
    num_elements -= buflen;
  }
  CORO_END;
  return decode_bufpos;
}



bool decode_double_stream::read(size_t _num_elements,
          iarchive& iarc,
          const std::pair<flexible_type*, size_t>& decodebuffer,
          size_t skip) {
  size_t decode_bufpos = 0;
  CORO_BEGIN(read)
  num_elements = _num_elements;
  // we reserve one character so we can add new encoders as needed in the future
  reserved = 0;
  iarc.read(&(reserved), sizeof(reserved));
  ASSERT_LT(reserved, 3);
  if (reserved == DOUBLE_RESERVED_FLAGS::LEGACY_ENCODING) {
    do {
      decode_bufpos = legacy.read(num_elements, iarc, decodebuffer, skip);
      CORO_YIELD(decode_bufpos);
    } while(CLASS_CORO_RUNNING(legacy, read));
  } else if (reserved == DOUBLE_RESERVED_FLAGS::INTEGER_ENCODING) {
    while(num_elements > 0) {
      buflen = std::min<size_t>(num_elements, MAX_INTEGERS_PER_BLOCK);
      frame_of_reference_decode_128(iarc, buflen, buf);
      for (i = 0;i < buflen; ++i) {
        PUT_BUFFER_SKIP(flexible_type((flex_float)(int64_t)(buf[i])), i, buflen);
      }
      num_elements -= buflen;
    }
  }
  CORO_END;
  return decode_bufpos;
}


bool decode_string_stream::read(size_t _num_elements,
          iarchive& iarc,
          const std::pair<flexible_type*, size_t>& decodebuffer,
          size_t skip) {
  size_t decode_bufpos = 0;
  CORO_BEGIN(read)
  num_elements = _num_elements;
  idx_values.resize(num_elements, flexible_type(flex_type_enum::INTEGER));
  iarc >> use_dictionary_encoding;
  if (use_dictionary_encoding) {
    variable_decode(iarc, num_values);
    str_values.resize(num_values);
    for (auto& str: str_values) {
      std::string new_str;
      uint64_t str_len;
      variable_decode(iarc, str_len);
      new_str.resize(str_len);
      iarc.read(&(new_str[0]), str_len);
      str = std::move(new_str);
    }
    decode_number(iarc, idx_values, 0);
    for (i = 0;i < num_elements; ++i) {
      PUT_BUFFER_SKIP(str_values[idx_values[i].get<flex_int>()], i, num_elements);
    }
  } else {
    // get all the lengths
    decode_number(iarc, idx_values, 0);
    for (i = 0;i < num_elements; ++i) {
      {
        size_t str_len = idx_values[i].get<flex_int>();
        ret.mutable_get<std::string>().resize(str_len);
        iarc.read(&(ret.mutable_get<std::string>()[0]), str_len);
      }
      PUT_BUFFER(ret);
    }
  }
  CORO_END;
  return decode_bufpos;
}



bool decode_vector_stream::read(size_t _num_elements,
          iarchive& iarc,
          const std::pair<flexible_type*, size_t>& decodebuffer,
          size_t skip,
          bool new_format) {
  size_t decode_bufpos = 0;
  CORO_BEGIN(read)
  num_elements = _num_elements;
  // we reserve one character so we can add new encoders as needed in the future
  if (new_format) {
    iarc.read(&(reserved), sizeof(reserved));
  }
  // decode the length of each vector
  lengths.resize(num_elements);
  decode_number(iarc, lengths, 0);
  total_num_values = 0;
  for (const flexible_type& length : lengths) {
    total_num_values += length.get<flex_int>();
  }

  // decode the values
  values.resize(total_num_values);
  if (new_format) {
    decode_double(iarc, values, 0);
  } else {
    decode_double_legacy(iarc, values, 0);
  }

  length_ctr = 0;
  value_ctr = 0;
  for (i = 0 ;i < num_elements; ++i) {
    {
      flex_vec& output_vec = ret.mutable_get<flex_vec>();
      // resize this to the appropriate length
      output_vec.resize(lengths[length_ctr].get<flex_int>());
      ++length_ctr;
      // fill in the value
      for(j = 0; j < output_vec.size(); ++j) {
        output_vec[j] = values[value_ctr].reinterpret_get<flex_float>();
        ++value_ctr;
      }
    }
    PUT_BUFFER(ret);
  }
  CORO_END
  return decode_bufpos;
}


bool decode_ndvector_stream::read(size_t _num_elements,
          iarchive& iarc,
          const std::pair<flexible_type*, size_t>& decodebuffer,
          size_t skip,
          bool new_format) {
  size_t decode_bufpos = 0;
  CORO_BEGIN(read)
  num_elements = _num_elements;
  // new_format is ignored. it should always be true.
  // one character is reserved so we can add new encoders as needed in the future
  iarc.read(&(reserved), sizeof(reserved));

  shape_lengths.resize(num_elements);
  numel.resize(num_elements);

  // decode shape lengths and numel
  decode_number(iarc, shape_lengths, 0);
  decode_number(iarc, numel, 0);

  // compute the length of shapes and strides
  sum_shape_len = 0;
  for (auto s : shape_lengths) sum_shape_len += s.get<flex_int>();
  // decode shape and strides
  shapes.resize(sum_shape_len);
  strides.resize(sum_shape_len);
  decode_number(iarc, shapes, 0);
  decode_number(iarc, strides, 0);

  // compute the length of values
  sum_values_len = 0;
  for (auto s : numel) sum_values_len += s.get<flex_int>();
  values.resize(sum_values_len);
  decode_double(iarc, values, 0);

  // emit
  shape_stride_ctr = 0;
  value_ctr = 0;

  for (i = 0 ;i < num_elements; ++i) {
    // construct the shape and stride
    ret_shape.resize(shape_lengths[i]);
    ret_stride.resize(shape_lengths[i]);
    for (j = 0;j < shape_lengths[i]; ++j) {
      ret_shape[j] = shapes[shape_stride_ctr].get<flex_int>();
      ret_stride[j] = strides[shape_stride_ctr].get<flex_int>();
      ++shape_stride_ctr;
    }

    // construct the values
    ret_numel = numel[i].get<flex_int>();
    ret_values = std::make_shared<flex_nd_vec::container_type>(ret_numel);
    for (j = 0;j < ret_numel; ++j) {
      (*ret_values)[j] = values[j + value_ctr].reinterpret_get<flex_float>();
    }
    value_ctr += ret_numel;
    PUT_BUFFER(flexible_type(flex_nd_vec(ret_values, ret_shape, ret_stride)));
  }
  CORO_END
  return decode_bufpos;
}

typed_decode_stream::typed_decode_stream(const block_info& info,
                                         char* start, size_t len)
  :info(info),start(start),len(len), iarc(start, len), generic_deserializer{iarc} {
    // some basic block properties which will be filled in
    // number of elements
    dsize = info.num_elem;
    num_undefined = 0;
    iarc >> num_types;
    // if it is a multiple type block, we don't perform a type decode
    perform_type_decoding = !(info.flags & MULTIPLE_TYPE_BLOCK);


    if (!(info.flags & IS_FLEXIBLE_TYPE)) {
      logstream(LOG_ERROR) << "Attempting to decode a non-typed block"
                           << std::endl;
      ASSERT_TRUE(info.flags & IS_FLEXIBLE_TYPE);
    }

    if (perform_type_decoding) {
      if (num_types == 1) {
        //  one block of contiguous type.
        char c;
        iarc >> c;
        column_type = (flex_type_enum)c;
      } else if (num_types == 2) {
        // two types, but with undefined entries.
        char c;
        iarc >> c;
        column_type = (flex_type_enum)c;
        // read the bitset and undefine all the flagged entries
        undefined_bitmap.resize(info.num_elem);
        undefined_bitmap.clear();
        iarc.read((char*)undefined_bitmap.array, sizeof(size_t)*undefined_bitmap.arrlen);
        num_undefined = undefined_bitmap.popcount();
      } else {
        logstream(LOG_ERROR) << "Unexpected value for num_types: "
                             << static_cast<int>(num_types)
                             << " (expected 0, 1, or 2)" << std::endl;
        ASSERT_TRUE(false);
      }
    }
    elements_to_decode = dsize - num_undefined;
  }

typed_decode_stream::~typed_decode_stream() {
  if (number_decoder) delete number_decoder;
  if (double_decoder) delete double_decoder;
  if (double_legacy_decoder) delete double_legacy_decoder;
  if (string_decoder) delete string_decoder;
  if (vector_decoder) delete vector_decoder;
  if (ndvector_decoder) delete ndvector_decoder;
}

/**
 * Decodes a collection of flexible_type values. The array must be of
 * contiguous type, but permitting undefined values.
 *
 * See \ref typed_encode() for details.
 *
 * \note The coding does not store the number of values stored. This is
 * stored in the block_info (block.num_elem)
 */
size_t typed_decode_stream::read(const std::pair<flexible_type*, size_t>& decodebuffer,
                                 size_t skip) {
  if (skip == 0) {
    ASSERT_TRUE(decodebuffer.first != nullptr && decodebuffer.second > 0);
  } else {
    ASSERT_TRUE(decodebuffer.first == nullptr && decodebuffer.second == 0);
  }
  size_t decode_bufpos = 0;
  if (perform_type_decoding) {
    if (num_types == 1) {
      if (decodebuffer.first && column_type == flex_type_enum::UNDEFINED) {
        for (size_t idx = 0;idx < decodebuffer.second; ++idx) {
          decodebuffer.first[idx] = FLEX_UNDEFINED;
        }
      } else {
        for (size_t idx = 0;idx < decodebuffer.second; ++idx) {
          decodebuffer.first[idx] = flexible_type();
        }
      }
    } else if (num_types == 2) {
      if (skip) {
        size_t effective_skip = 0;
        // compute the effective skip
        for (size_t idx = 0;idx < skip; ++idx) {
          effective_skip += !undefined_bitmap.get(last_id);
          ++last_id;
        }
        skip = effective_skip;
        if (skip == 0) return 0;
      } else {
        size_t nvals = pad_retbuf_with_undefined_positions(decodebuffer);
        if (nvals == 0) return 0;
      }
    }
  }

  CORO_BEGIN(read)

  if (perform_type_decoding) {
    if (num_types == 0) {
      // empty block
      return 0;
    } else if (num_types == 1 && column_type == flex_type_enum::UNDEFINED) {
      // the CORO_BEGIN(read) prefix has already filled this with
      // undefined values. So all is good and we just count how
      // many we should actually be returning and return.
      // We still use the CORO mechanic here so that CLASS_CORO_RUNNING
      // can be used to detect if there is still stuff to read.
      while(dsize > 0) {
        if (skip) undefined_elem_consumed = std::min(dsize, skip);
        else undefined_elem_consumed = std::min(dsize, decodebuffer.second);
        dsize -= undefined_elem_consumed;
        CORO_YIELD(undefined_elem_consumed);
      }
    }
  } else {
    iarc >> values;
    for (i = 0; i < values.size(); ++i) {
      PUT_BUFFER_SKIP(std::move(values[i]), i, values.size());
    }
    return decode_bufpos;
  }


  // the interesting decoder
  if (perform_type_decoding) {

    if (column_type == flex_type_enum::INTEGER) {
      number_decoder = new decode_number_stream;
      do {
        decode_bufpos = number_decoder->read(elements_to_decode, iarc, decodebuffer, skip);
        CORO_YIELD(decode_bufpos);
      } while(CLASS_CORO_RUNNING(*number_decoder, read));
      delete number_decoder;
      number_decoder = nullptr;

    } else if (column_type == flex_type_enum::FLOAT) {
      if (info.flags & BLOCK_ENCODING_EXTENSION) {
        double_decoder = new decode_double_stream;
        do {
          decode_bufpos = double_decoder->read(elements_to_decode, iarc, decodebuffer, skip);
          CORO_YIELD(decode_bufpos);
        } while(CLASS_CORO_RUNNING(*double_decoder, read));
      delete double_decoder;
      double_decoder = nullptr;
      } else {
        double_legacy_decoder = new decode_double_stream_legacy;
        do {
          decode_bufpos = double_legacy_decoder->read(elements_to_decode, iarc, decodebuffer, skip);
          CORO_YIELD(decode_bufpos);
        } while(CLASS_CORO_RUNNING(*double_legacy_decoder, read));
      delete double_legacy_decoder;
      double_legacy_decoder = nullptr;
      }
    } else if (column_type == flex_type_enum::STRING) {
      string_decoder = new decode_string_stream;
      do {
        decode_bufpos = string_decoder->read(elements_to_decode, iarc, decodebuffer, skip);
        CORO_YIELD(decode_bufpos);
      } while(CLASS_CORO_RUNNING(*string_decoder, read));
      delete string_decoder;
      string_decoder = nullptr;
    } else if (column_type == flex_type_enum::VECTOR) {
      vector_decoder = new decode_vector_stream;
      do {
        decode_bufpos = vector_decoder->read(elements_to_decode, iarc, decodebuffer, skip,
                                             info.flags & BLOCK_ENCODING_EXTENSION);
        CORO_YIELD(decode_bufpos);
      } while(CLASS_CORO_RUNNING(*vector_decoder, read));
      delete vector_decoder;
      vector_decoder = nullptr;
    } else if (column_type == flex_type_enum::ND_VECTOR) {
      ndvector_decoder = new decode_ndvector_stream;
      do {
        decode_bufpos = ndvector_decoder->read(elements_to_decode, iarc, decodebuffer, skip,
                                               info.flags & BLOCK_ENCODING_EXTENSION);
        CORO_YIELD(decode_bufpos);
      } while(CLASS_CORO_RUNNING(*ndvector_decoder, read));
      delete ndvector_decoder;
      ndvector_decoder = nullptr;
    } else {
      for (i = 0;i < elements_to_decode; ++i) {
        generic_ret = flexible_type(column_type);
        generic_ret.apply_mutating_visitor(generic_deserializer);
        PUT_BUFFER(std::move(generic_ret));
      }
    }
  }
  CORO_END
  return decode_bufpos;
}


size_t typed_decode_stream::pad_retbuf_with_undefined_positions(const std::pair<flexible_type*, size_t>& decodebuffer) {
  size_t nvals = 0;
  // fill the ret buffer with the appropriate undefined values
  // in the right positions
  if (num_undefined && decodebuffer.first) {
    for (size_t idx = 0;idx < decodebuffer.second; ++idx) {
      if (undefined_bitmap.get(last_id)) {
        decodebuffer.first[idx] = FLEX_UNDEFINED;
      } else {
        decodebuffer.first[idx] = flexible_type();
        ++nvals;
      }
      ++last_id;
    }
  }
  return nvals;
}

} // namespace v2_block_impl
} // namespace turi
