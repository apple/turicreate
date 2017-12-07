/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstddef>
#include <flexible_type/flexible_type.hpp>
#include <sframe/sarray.hpp>
using namespace turi;

/**
 * We define a little local helper type called "read_row_of_value" which reads
 * a row of a particular type from a pointer, and writes it to a flexible_type
 * either as a VECTOR or as a scalar (FLOAT or INTEGER).
 */
template <typename T>
struct read_row_of_value {
  flexible_type value;
  size_t row_length = 0;

  /**
   * Sets the row length and also determines the output type.
   * 
   * If row_length == 1, based on the properties of T, the output type
   * is either INTEGER or FLOAT.
   * Otherwise it is a VECTOR.
   */
  void set_row_length(size_t n) {
    row_length = n;
    if (row_length == 1) {
      if (std::is_integral<T>::value) value = flexible_type(flex_type_enum::INTEGER);
      else value = flexible_type(flex_type_enum::FLOAT);
    } else {
      value = flexible_type(flex_type_enum::VECTOR); 
      value.mutable_get<flex_vec>().resize(n);
    }
  }

  /**
   * Reads "row_length" elements of T from the pointer,
   * returning pointer the byte after all reads.
   *
   * After the read is complete, "value" will contain the read values.
   */
  T* read_row(T* src) {
    if (row_length == 1) {
      if (std::is_integral<T>::value) {
        value.mutable_get<flex_int>() = (*src++);
      } else {
        value.mutable_get<flex_float>() = (*src++);
      }
    } else {
      flex_vec& value_ref = value.mutable_get<flex_vec>();
      for (size_t i = 0;i < row_length; ++i) value_ref[i] = (*src++);
    }
    return src;
  }
};


template <typename T>
void create_sarray(void* ptr, 
                   size_t num_rows,
                   size_t row_length, 
                   turi::sarray<flexible_type>& out) {

  read_row_of_value<T> row_reader;
  row_reader.set_row_length(row_length);
  out.set_type(row_reader.value.get_type());

  auto output_iter = out.get_output_iterator(0);
  T* src = reinterpret_cast<T*>(ptr);
  for (size_t i = 0;i < num_rows; ++i) {
    src = row_reader.read_row(src);
    (*output_iter) = row_reader.value;
  }
}

extern "C" {

/**
 * Converts a pointer to an SArray. pointer must point to a "C-style" laid
 * out array.
 *
 * Note the distinction between ELEMENTS and BYTES.
 * Number of bytes is #ELEMENTS * size of each element.
 *
 * \param ptr Pointer to the actual data
 * \param ptr_length total number of ELEMENTS pointed to by ptr
 * \param row_length Number of ELEMENTS per row
 * \param is_integer True if it is an integral type. False for float.
 * \param signed_type True if it is a signed type (only valid for integers)
 * \param element_width Number of bytes per element. 
 *                      (ex: if integer this is the number of bytes for the 
 *                      integer representation)
 * \param output_location The output SArray name
 */
EXPORT bool numpy_to_sarray(void* ptr, 
                            size_t ptr_length, 
                            size_t row_length,
                            bool is_integer,
                            bool signed_type,
                            size_t element_width,
                            char* output_location) {
  turi::sarray<flexible_type> out;
  out.open_for_write(output_location, 1);

  // ok! lets do some type selection
  size_t num_rows = ptr_length / row_length;
  if (is_integer) {
    switch(element_width) {
     case 8:
       if (signed_type) create_sarray<int64_t>(ptr, num_rows, row_length, out);
       else             create_sarray<uint64_t>(ptr, num_rows, row_length, out);
       break;
     case 4:
       if (signed_type) create_sarray<int32_t>(ptr, num_rows, row_length, out);
       else             create_sarray<uint32_t>(ptr, num_rows, row_length, out);
       break;
     case 2:
       if (signed_type) create_sarray<int16_t>(ptr, num_rows, row_length, out);
       else             create_sarray<uint16_t>(ptr, num_rows, row_length, out);
       break;
     case 1:
       if (signed_type) create_sarray<int8_t>(ptr, num_rows, row_length, out);
       else             create_sarray<uint8_t>(ptr, num_rows, row_length, out);
       break;
     default:
      std::cout << "Invalid integer element width\n";
      return false;
    }
  } else {
    // floats
    switch(element_width) {
     case 8:
       create_sarray<double>(ptr, num_rows, row_length, out);
       break;
     case 4:
       create_sarray<float>(ptr, num_rows, row_length, out);
       break;
     default:
      std::cout << "Invalid floating point element width\n";
      return false;
    }
  }
  out.close();
  return true;
}
} // export
