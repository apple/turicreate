/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_ENTRY_H_
#define TURI_DML_DATA_ENTRY_H_

#include <cstdlib>
#include <core/storage/serialization/serialization_includes.hpp>

namespace turi {


/**
 * \ingroup mldata
 * \{
 * Information relevant to a single entry of ml_data.
 */
struct ml_data_full_entry {
  size_t column_index;      /**< Column id .*/
  size_t feature_index;     /**< Local index within the column.*/
  size_t global_index;      /**< Global index, referenced off of the training index sizes. */
  double value;             /**< Value */
};

/**
 * Information relevant to a single entry of ml_data.
 */
struct ml_data_entry {
  size_t column_index;      /**< Column id .*/
  size_t index;             /**< Local index within the column.*/
  double value;             /**< Value */

  /// Simple equality test
  bool operator==(const ml_data_entry& other) const {
    return ((column_index == other.column_index)
            && (index == other.index)
            && (value == other.value));
  }

  const ml_data_entry& operator=(const ml_data_full_entry& fe) {
    column_index = fe.column_index;
    index = fe.feature_index;
    value = fe.value;

    return *this;
  }
};


/**
 * Information relevant to a single entry of ml_data.
 */
struct ml_data_entry_global_index {
  size_t global_index;      /**< Global index based on training indices. */
  double value = 1.0;       /**< Value */

  /// Simple equality test
  bool operator==(const ml_data_entry_global_index& other) const {
    return ((global_index == other.global_index)
            && (value == other.value));
  }

  const ml_data_entry_global_index& operator=(const ml_data_full_entry& fe) {
    global_index = fe.global_index;
    value = fe.value;

    return *this;
  }
};

/// \}
}

SERIALIZABLE_POD(ml_data_entry);


#endif /* TURI_DML_DATA_ENTRY_H_ */
