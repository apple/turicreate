/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/assertions.hpp>
#include <core/storage/sframe_data/sframe_rows.hpp>
#include <core/storage/sframe_data/sarray_v2_block_types.hpp>
#include <core/storage/sframe_data/sarray_v2_type_encoding.hpp>

namespace turi {

void sframe_rows::resize(size_t num_cols, ssize_t num_rows) {
  ensure_unique();
  if (m_decoded_columns.size() != num_cols) m_decoded_columns.resize(num_cols);
  for (auto& col: m_decoded_columns) {
    if (col == nullptr) {
      if (num_rows == -1) {
        col = std::make_shared<decoded_column_type>();
      } else {
        col = std::make_shared<decoded_column_type>(num_rows, flex_undefined());
      }
    } else if (num_rows != -1 && col->size() != (size_t)num_rows) {
      col->resize(num_rows, flex_undefined());
    }
  }
}

void sframe_rows::clear() {
  m_decoded_columns.clear();
}

void sframe_rows::save(oarchive& oarc) const {
  oarc << m_decoded_columns.size();
  oarchive temp_inmemory_arc;
  for (auto& i : m_decoded_columns) {
    v2_block_impl::block_info info;
    // write into the in memory archive to fill the block info
    temp_inmemory_arc.off = 0;
    v2_block_impl::typed_encode(*i, info, temp_inmemory_arc);
    info.block_size = temp_inmemory_arc.off;

    // write the block info
    oarc.write(reinterpret_cast<const char*>(&info), sizeof(info));
    // write the data
    oarc.write(temp_inmemory_arc.buf, temp_inmemory_arc.off);
  }
  free(temp_inmemory_arc.buf);
}

void sframe_rows::load(iarchive& iarc) {
  size_t ncols = 0;
  iarc >> ncols;
  resize(ncols);
  char* buf = nullptr;
  for (size_t i = 0; i < ncols; ++i) {
    // read the block info
    v2_block_impl::block_info info;
    iarc.read(reinterpret_cast<char*>(&info), sizeof(info));
    buf = (char*)realloc(buf, info.block_size);
    iarc.read(buf, info.block_size);
    typed_decode(info, buf, info.block_size, *(m_decoded_columns[i]));
  }
  if (buf != nullptr) free(buf);
}

void sframe_rows::add_decoded_column(
    const sframe_rows::ptr_to_decoded_column_type& decoded_column) {
  m_decoded_columns.push_back(decoded_column);
}

void sframe_rows::ensure_unique() {
  if (m_is_unique) return;
  for (auto& col: m_decoded_columns) {
    if (!col.unique()) {
      col = std::make_shared<decoded_column_type>(*col);
    }
  }
  m_is_unique = true;
}

void sframe_rows::type_check_inplace(const std::vector<flex_type_enum>& typelist) {
  ASSERT_EQ(typelist.size(), num_columns());
  // one pass for column type check
  for (size_t c = 0; c < num_columns(); ++c) {
    if (typelist[c] != flex_type_enum::UNDEFINED) {
      // assume no modification required first
      auto& arr = m_decoded_columns[c];
      auto length = arr->size();
      bool current_array_is_unique = arr.unique();
      size_t i = 0;
      // ok what we are going to do is to assume no modifications are required
      // even if we do not have out own copy of the array.
      // then if modifications are required, we break out and resume below.
      if (!current_array_is_unique) {
        for (;i < length; ++i) {
          const auto& val = (*arr)[i];
          if (val.get_type() != typelist[c] &&
              val.get_type() != flex_type_enum::UNDEFINED) {
            // damn. modifications are required
            arr = std::make_shared<decoded_column_type>(*arr);
            current_array_is_unique = true;
            break;
          }
        }
      }
      if (current_array_is_unique) {
        for (;i < length; ++i) {
          auto& val = (*arr)[i];
          if (val.get_type() != typelist[c] &&
              val.get_type() != flex_type_enum::UNDEFINED) {
            flexible_type res(typelist[c]);
            res.soft_assign(val);
            val = std::move(res);
          }
        }
      }
    }
  }
}

sframe_rows sframe_rows::type_check(const std::vector<flex_type_enum>& typelist) const {
  ASSERT_EQ(typelist.size(), num_columns());
  sframe_rows other = *this;
  other.type_check_inplace(typelist);
  return other;
}

} // namespace turi
