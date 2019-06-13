/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_ROW_TRANSATION_H_
#define TURI_DML_DATA_ROW_TRANSATION_H_

#include <ml_data/data_storage/ml_data_row_format.hpp>
#include <ml_data/ml_data_entry.hpp>
#include <ml_data/ml_data_column_modes.hpp>
#include <ml_data/data_storage/internal_metadata.hpp>
#include <flexible_type/flexible_type.hpp>
#include <util/code_optimization.hpp>
#include <type_traits>

#include <Eigen/SparseCore>
#include <Eigen/Core>

#include <array>

namespace turi {

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

////////////////////////////////////////////////////////////////////////////////
// Translation routines to the basic ml_data_entry type

/** Translation from one row type to another
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry_global_index>& row);

/** Translation routines.
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const DenseVector& row);

/** Translates the original sparse row format to the ml_data_entry
 *  vector.
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const SparseVector& v);


////////////////////////////////////////////////////////////////////////////////
// translation routines to the original row type

std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry>& row);

/** Translates the original sparse row format to the original flexible
 *  types.
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const DenseVector& v);

/** Translates the original sparse row format to the original flexible
 *  types.
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const SparseVector& v);

/** Translate a vector of global indices to the next
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry_global_index>& row);

}

////////////////////////////////////////////////////////////////////////////////

#endif /* _ML_DATA_ROW_FORMAT_H_ */
