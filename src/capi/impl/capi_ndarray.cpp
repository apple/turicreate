/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <core/export.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

extern "C" {

EXPORT tc_ndarray* tc_ndarray_create_empty(tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_ndarray();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_ndarray* tc_ndarray_create_from_data(uint64_t n_dim, const uint64_t* shape,
                                        const int64_t* strides,
                                        const double* in_data, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if(shape == nullptr) {
    return new_tc_ndarray();
  }

  // Get the shape information
  typename turi::flex_nd_vec::index_range_type _shape(shape, shape + n_dim);

  size_t total_size = 1;
  for (size_t d : _shape) {
    total_size *= d;
  }

  // Allocate the array shared with the ndarray
  auto data =
      std::make_shared<typename turi::flex_nd_vec::container_type>(total_size);

  // This is slow and could definitely be optimized.
  std::vector<size_t> iv(n_dim, 0);

  if (strides != nullptr) {
    for (size_t dest_index = 0;; ++dest_index) {
      // Get the source index
      size_t src_index = 0;
      for (size_t i = 0; i < n_dim; ++i) {
        src_index += strides[i] * iv[i];
      }

      (*data)[dest_index] = in_data[src_index];

      bool done = false;
      // Increment the indexes
      for (size_t i = n_dim;;) {
        if (i == 0) {
          done = true;
          break;
        }
        --i;
        ++iv[i];

        if (iv[i] < shape[i]) {
          break;
        } else {
          iv[i] = 0;
        }
      }

      if (done) {
        break;
      }
    }
  } else {
    data->assign(in_data, in_data + total_size);
  }

  // Now, give the data to the ndarray and exit.
  return new_tc_ndarray(data, _shape);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_ndarray_num_dimensions(const tc_ndarray* ndv, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ndv, "tc_ndarray", NULL);

  return ndv->value.shape().size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT const uint64_t* tc_ndarray_shape(const tc_ndarray* ndv, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ndv, "tc_ndarray", NULL);

  static_assert(sizeof(size_t) == sizeof(uint64_t), "Size mismatch; size_t is not 64 bits.");

  return reinterpret_cast<const uint64_t*>(ndv->value.shape().data());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT const int64_t* tc_ndarray_strides(const tc_ndarray* ndv, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ndv, "tc_ndarray", NULL);

  return reinterpret_cast<const int64_t*>(ndv->value.stride().data());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT const double* tc_ndarray_data(const tc_ndarray* ndv, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ndv, "tc_ndarray", NULL);

  if(ndv->value.empty()) {
    return nullptr;
  } else {
    return &(ndv->value.at(0));
  }

  ERROR_HANDLE_END(error, NULL);
}

EXPORT double* tc_ndarray_writable_data(tc_ndarray* ndv, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ndv, "tc_ndarray", NULL);

  if(ndv->value.empty()) {
    return nullptr;
  } else {
    ndv->value.ensure_unique();
    return &(ndv->value.at(0));
  }

  ERROR_HANDLE_END(error, NULL);
}

} // End Extern "C"
