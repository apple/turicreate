/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sframe/sframe.hpp>
#include <user_pagefault/user_pagefault.hpp>
#include <flexible_type/flexible_type.hpp>
#include <minipsutil/minipsutil.h>
#include <gl_numpy/memory_mapped_sframe.hpp>
#include <logger/assertions.hpp>
#include <util/code_optimization.hpp>
#include <fileio/fs_utils.hpp>
namespace turi {
namespace gl_numpy { 

/*
 * Since we can only hold flex_int and flex_float at the moment,
 * the element length is generally just going to be 8.
 */
const size_t memory_mapped_sframe::m_element_length = 8;
/*
 * Annoyingly, there is no compile time static nan.
 */
flex_int memory_mapped_sframe::M_NAN_VALUE = 0;

memory_mapped_sframe::memory_mapped_sframe() {
  user_pagefault::setup_pagefault_handler();
  double double_nan = std::nan("");
  M_NAN_VALUE = *reinterpret_cast<flex_int*>(&double_nan);
}
memory_mapped_sframe::~memory_mapped_sframe() {
  unload();
}

bool memory_mapped_sframe::load(sframe frame) {
  if (frame.num_rows() == 0 || frame.num_columns() == 0) return false;
  // do a quick schema check
  for (auto coltype: frame.column_types()) {
    if (coltype != flex_type_enum::INTEGER &&
        coltype != flex_type_enum::FLOAT &&
        coltype != flex_type_enum::VECTOR) {
      return false;
    }
  }

  if (m_ps) unload();
  m_frame = frame;
  bool ret = activate();
  if (ret == false) return false;

  /*
   * std::cout << "Creating SFrame mapping at " 
   *           << (void*)(m_ps->begin) << std::endl;
   */
  return true;
}

void memory_mapped_sframe::unload() {
  if (m_ps) {
    user_pagefault::release(m_ps);
    // clear all variables.
    m_ps = nullptr;
    m_reader.reset();
    m_frame = sframe();
    m_length = 0;
    m_length_in_bytes = 0;
    m_type = flex_type_enum::INTEGER;
    m_values_per_column.clear();
    m_values_per_row = 0;
    for (const auto& p: m_delete_paths) {
      fileio::delete_path_recursive(p);
    }
  }
}

bool memory_mapped_sframe::activate() {
  /*
   * The mapping between sframe and a flat memory address is 
   * mildly complicated by the need to support column types which
   * are of vector type. This means that there is not a 1-1 mapping between
   * columns and values: some columns may have more values than other columns.
   *
   * We are going to assume that for vector columns, the 1st element of the
   * column tells us the size of the vector and we will interpret all the 
   * remaining values that way:
   *   i.e. 
   *    - From the 1st value, we say that all vectors are of length N
   *    - if there are vectors later which are shorter than N, 
   *      all remaining values are NaN
   *    - If there are vectors which are longer than N, the vector is truncated.
   *
   * The last annoyance is the mapping to the pagefault handler.
   * The pagefault handler basically requires us to fill in one "page" 
   * and exactly that one page. No more, no less. So we may end up slicing
   * between rows to fill up the page. And that is kinda annoying.
   * We are going to assume the pagesize divides by 8 though, (which really
   * it must anyway), so we don't  have to slice single values.
   */

  m_column_types = m_frame.column_types();
  m_type = flex_type_enum::INTEGER;
  m_values_per_column.resize(m_column_types.size());
  for (size_t i = 0; i < m_column_types.size(); ++i) {
    if (m_column_types[i] == flex_type_enum::INTEGER) {
      m_values_per_column[i] = 1;
    } else if (m_column_types[i] == flex_type_enum::FLOAT) {
      m_values_per_column[i] = 1;
      m_type = flex_type_enum::FLOAT;
    } else if (m_column_types[i] == flex_type_enum::VECTOR) {
      // read the first row
      sframe_rows rows;
      m_frame.select_column(i)
          ->get_reader()
          ->read_rows(0, 1, rows);

      // failure to read rows?
      if (rows.num_rows() == 0) return false;
      m_values_per_column[i] = rows[0][0].size();
      m_type = flex_type_enum::FLOAT;
    }
  }
  
  // fill in the remaining values
  m_values_per_row = 0;
  for (auto i : m_values_per_column) m_values_per_row += i;

  m_reader = std::move(m_frame.get_reader());
  // assume that sizeof(flex_int) is same as sizeof(flex_float)
  m_length = m_values_per_row * m_frame.num_rows();
  m_length_in_bytes = m_length * sizeof(flex_int);
  m_ps = 
      user_pagefault::allocate(m_length_in_bytes, 
               [=](user_pagefault::userpf_page_set* ps,
                   char* page_address,
                   size_t minimum_fill_length)->size_t { 
                     return this->handler_callback(ps,
                                                   page_address,
                                                   minimum_fill_length); 
                   });
  return true;
}

void memory_mapped_sframe::recursive_delete_on_close(std::string path) {
  m_delete_paths.push_back(path);
}

void* memory_mapped_sframe::get_pointer() {
  if (m_ps) return (void*)(m_ps->begin);
  else return nullptr;
}

flex_type_enum memory_mapped_sframe::get_type() {
  return m_type;
}

size_t memory_mapped_sframe::length_in_bytes() {
  return m_length_in_bytes;
}

size_t memory_mapped_sframe::length() {
  return m_length;
}

inline void memory_mapped_sframe::integer_value_to_integer(const flexible_type& f, 
                                                   flex_int* store) {
  (*store) =  f.get_type() != flex_type_enum::UNDEFINED ? f.get<flex_int>() : 0;
}

inline void memory_mapped_sframe::integer_value_to_float(const flexible_type& f, 
                                                         flex_int* store) {
  (*reinterpret_cast<flex_float*>(store)) =  
      f.get_type() != flex_type_enum::UNDEFINED ? f.get<flex_int>() : 0;
}

inline void memory_mapped_sframe::float_value_to_float(const flexible_type& f, 
                                                       flex_int* store) {
  (*store) =  f.get_type() != flex_type_enum::UNDEFINED ? f.get<flex_int>() : M_NAN_VALUE;
}


inline GL_HOT_INLINE_FLATTEN 
void memory_mapped_sframe::store_row_to_pointer(const sframe_rows::row& row,
                                                flex_int* store) {
  if (m_type == flex_type_enum::INTEGER) {
    // integer output type can only have integer column types
    for (size_t i = 0;i < row.size(); ++i) {
      integer_value_to_integer(row[i], store++);
    }
  } else if (m_type == flex_type_enum::FLOAT) {
    // float output type has a bunch of possibilities
    for (size_t i = 0;i < row.size(); ++i) {
      if (m_column_types[i] == flex_type_enum::INTEGER) {
        // integer column type
        integer_value_to_float(row[i], store++);
      } else if (m_column_types[i] == flex_type_enum::FLOAT) {
        // float column type
        float_value_to_float(row[i], store++);
      } else if (m_column_types[i] == flex_type_enum::VECTOR) {
        // vector input type is slightly annoying
        if (row[i].get_type() == flex_type_enum::VECTOR) {
          const auto& vec = row[i].get<flex_vec>();
          // we have at least as many values as we need. read it.
          if (vec.size() >= m_values_per_column[i]) {
            for (size_t j = 0; j < m_values_per_column[i]; ++j) {
              (*reinterpret_cast<flex_float*>(store++)) = vec[j];
            }
          } else {
            // we don't have enough values. read and then pad with NaN
            for (size_t j = 0; j < vec.size(); ++j) {
              (*reinterpret_cast<flex_float*>(store++)) = vec[j];
            }
            for (size_t j = vec.size(); j < m_values_per_column[i]; ++j) {
              (*store++) = M_NAN_VALUE;
            }
          } 
        } else {
          // missing value. all NaNs
          for (size_t j = 0; j < m_values_per_column[i]; ++j) {
            (*store++) = M_NAN_VALUE;
          }
        }
      }
    }
  }
}

size_t memory_mapped_sframe::handler_callback(user_pagefault::userpf_page_set* ps,
                                            char* page_address,
                                            size_t minimum_fill_length) {
  // we take advantage that double and flex_int have the same size
  // and do all work as integers.
  flex_int* root = (flex_int*) ps->begin;
  flex_int* s_addr  = (flex_int*) page_address;
  size_t num_to_fill = minimum_fill_length / m_element_length;
  size_t start = s_addr - root;

  // ok. so we basically need to fill from 
  // s_addr[0] to s_addr[num_to_fill]
  //
  // filling in values 'start' to 'start + num_to_fill' of a 
  // "flattened" sframe (i.e. we think about the values in each row just
  // appearing consecutively one after another).
  // This is ever so slightly annoying.
  //
  // Once again, to reiterate, we are assuming everything is already 
  // 8 bytes == sizeof(flex_int) == sizeof(flex_float) aligned. 

  size_t frame_row_start = start / m_values_per_row;
  size_t frame_row_start_offset = start - frame_row_start * m_values_per_row;
  size_t frame_row_end = (start + num_to_fill) / m_values_per_row;
  size_t frame_row_end_offset = start + num_to_fill - frame_row_end * m_values_per_row;
  if (frame_row_end_offset == 0) {
    // frame-row_end must point to the last row
    --frame_row_end;
    frame_row_end_offset = m_values_per_row;
  }
  /* Where each block is a row of an SFrame
   *    --------------------------------------------
   *    |                                          |
   *    --------------------------------------------
   *        ...         ...           ...
   * 
   *  frame_row_start
   * |
   * |                       frame_row_start_offset
   * |                       |
   * |                       v
   * |  --------------------------------------------
   * -> |                    |                     |
   *    --------------------------------------------
   *    |                                          |
   *    --------------------------------------------
   *    |                                          |
   *    --------------------------------------------
   *    |                                          |
   *    --------------------------------------------
   *    |                                          |
   *    --------------------------------------------
   *    |                                          |
   *    --------------------------------------------
   *    |                                          |
   *    --------------------------------------------
   * -> |                             |            |
   * |  --------------------------------------------
   * |                                ^
   * |                                |
   * |                                frame_row_end_offset
   * |
   * frame_row_end
   */

  sframe_rows rows;
  // remember. read_rows exclude the "end" so we need to read one more row. 
  m_reader->read_rows(frame_row_start, 
                      frame_row_end + 1,
                      rows);

  std::vector<flex_int> buffer_row(m_values_per_row);
  // loop over all the rows
  size_t row_number = frame_row_start;
  for (auto& row: rows) {
    if (row_number == frame_row_start) {
      // first row
      // this row may be cut since we really only need to start from
      // frame_row_start_offset. so we read the row into a buffer
      store_row_to_pointer(row, buffer_row.data()); 

      size_t first_row_last_elem = 
          frame_row_start != frame_row_end ? buffer_row.size() : frame_row_end_offset;
      for (size_t col = frame_row_start_offset; col < first_row_last_elem; ++col) {
        (*s_addr++) = buffer_row[col];
      }
    } else if (row_number == frame_row_end) {
      // last row
      // this row may be cut since we need to end at 
      // frame_row_end_offset. so we read the row into a buffer
      store_row_to_pointer(row, buffer_row.data()); 
      for (size_t col = 0; col < frame_row_end_offset; ++col) {
        (*s_addr++) = buffer_row[col];
      }
    } else {
      // middle rows
      // in the middle we can just store directly
      store_row_to_pointer(row, s_addr);
      s_addr += m_values_per_row;
    }
    ++row_number;
  }
  //std::cerr << "callback returns" << std::endl;
  return minimum_fill_length;
}



} // namespace gl_numpy
} // namespace turi
