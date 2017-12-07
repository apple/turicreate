/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_MEMORY_MAPPED_SFRAME_HPP
#define TURI_MEMORY_MAPPED_SFRAME_HPP
#include <memory>
#include <cstddef>
#include <flexible_type/flexible_type.hpp>
#include <sframe/sframe.hpp>
namespace turi {

namespace user_pagefault {
struct userpf_page_set;
}

class sframe;

namespace gl_numpy {

/**
 * Defines a memory mapped Sframe. 
 * i.e. an SFrame mapped into a single pointer in memory
 * via the pagefault handler.
 *
 * Only FLOAT, INTEGER, VECTOR columns are supported.
 * For Integer columns , missing values are mapped to 0.
 * For float columns , missing values are mapped to NaN.
 */
class memory_mapped_sframe {
 public:
  /// Default constructor. Noop
  memory_mapped_sframe();
  memory_mapped_sframe(const memory_mapped_sframe&) = delete;
  memory_mapped_sframe(memory_mapped_sframe&&) = delete;
  memory_mapped_sframe& operator=(const memory_mapped_sframe&) = delete;
  memory_mapped_sframe& operator=(memory_mapped_sframe&&) = delete;

  /**
   * Loads an SFrame . Returns false if the SFrame 
   * is not of a type we can map in. The SFrame must contain
   * only columns of integer, float, or vector.
   *
   * The frame must not have 0 rows, or 0 columns.
   */
  bool load(sframe frame);

  /**
   * Adds a path to be recursively deleted on destruction of the memory
   * mapped sframe
   */
  void recursive_delete_on_close(std::string path);

  /**
   * Releases the memory mapped in by load.
   * No-op if nothing loaded.
   */
  void unload();

  /**
   * Returns the pointer to the base adddress. Returns a null pointer
   * if nothing is mapped in.
   */
  void* get_pointer();

  /**
   * Returns the type of the output (all integers or all floats)
   * Only return INTEGER or FLOAT.
   */
  flex_type_enum get_type();

  /**
   * Returns the length of the pointer range in bytes.
   */
  size_t length_in_bytes();

  /**
   * Returns the number of elements.
   */
  size_t length();

  /// destructor
  ~memory_mapped_sframe();

 private:
  user_pagefault::userpf_page_set* m_ps = nullptr;
  sframe m_frame;
  std::unique_ptr<sframe::reader_type> m_reader;
  std::vector<std::string> m_delete_paths;

  /// number of elements
  size_t m_length = 0; 
  /// number of bytes in mapped region
  size_t m_length_in_bytes = 0; 

  /// This is really a constant: double nan value in integral form
  static flex_int M_NAN_VALUE; 

  /// output type. Integer if all columns are integral. Float otherwise
  flex_type_enum m_type = flex_type_enum::INTEGER; 

  /// The original frame column types
  std::vector<flex_type_enum> m_column_types;
  /// number of values produced by each SFrame column
  std::vector<size_t> m_values_per_column;
  /// Total number of values generated per row (sum of m_values_per_column)
  size_t m_values_per_row = 0;
  

  static const size_t m_element_length;
  /**
   * Activates memory mapping of an SFrame
   */
  bool activate();


  size_t handler_callback(user_pagefault::userpf_page_set* ps,
                          char* page_address,
                          size_t minimum_fill_length);

  /**
   * Stores a row to a pointer respecting the schema
   */
  void store_row_to_pointer(const sframe_rows::row& row,
                            flex_int* store);

  /**
   * Interprets a flexible_type value as an integer. flexible_type must be
   * an integer or UNDEFINED. If UNDEFINED, it is 0.
   */
  static void integer_value_to_integer(const flexible_type& f,
                                       flex_int* store);

  /**
   * Interprets a flexible_type value as an float. flexible_type must be
   * an integer or UNDEFINED. If UNDEFINED, it is 0.
   *
   * \note the output type is always (flex_int*). even though it may store 
   * really store a floating point value.
   */
  static void integer_value_to_float(const flexible_type& f,
                                     flex_int* store);
  /**
   * Interprets a flexible_type value as an float. flexible_type must be
   * an float or UNDEFINED. If UNDEFINED, it is NaN.
   *
   * \note the output type is always (flex_int*). even though it may store 
   * really store a floating point value. Mainly, this prevents us from having
   * to deal at all with floating point NaN values.
   */
  static void float_value_to_float(const flexible_type& f,
                                   flex_int* store);

  
};
} // namespace gl_numpy
} // namespace turi
#endif
