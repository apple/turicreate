/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2DATA_SFRAME_TRANSLATION_H_
#define TURI_ML2DATA_SFRAME_TRANSLATION_H_

#include <vector>
#include <memory>
#include <core/storage/sframe_data/sframe.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/indexing/column_indexer.hpp>

////////////////////////////////////////////////////////////////////////////////
//
//  Translation helper functions
//
////////////////////////////////////////////////////////////////////////////////

namespace turi { namespace v2 {

  /**
   * Translates an external SFrame into the corresponding indexed
   * SFrame representation, as dictated by the indexing in
   * column_metadata.  Only the columns specified in metadata are
   * used, and all of these must be present.
   *
   * If allow_new_categorical_values is false, then the metadata is
   * not changed.  New categorical values are mapped to size_t(-1)
   * with a warning.
   *
   * Categorical: If a column is categorical, each unique value is mapped to
   * a unique index in the range 0, ..., n-1, where n is the number of unique
   * values.
   *
   * Numeric: The column type is checked to be INT/FLOAT, then
   * returned as-is.
   *
   * Numeric Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Categorical Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Dictionary : If the dictated column type is DICT, it is checked to make
   * sure the values are numeric. The keys are then translated to 0..n-1
   * where n is the number of unique keys.
   *
   * \param[in] metadata The metadata used for the mapping.
   * \param[in] unindexed_x The SFrame to map to indices.
   * \param[in] allow_new_categorical_values Whether to allow new categories.
   *
   * \returns Indexed SFrame.
   */
std::shared_ptr<sarray<flexible_type> > map_to_indexed_sarray(
    const std::shared_ptr<ml_data_internal::column_indexer>& indexer,
    const std::shared_ptr<sarray<flexible_type> >& src,
    bool allow_new_categorical_values = true);

  /**
   * Translates an external SFrame into the corresponding indexed
   * SFrame representation, as dictated by the indexing in
   * column_indexer.  Only the columns specified in metadata are
   * used, and all of these must be present.
   *
   * If allow_new_categorical_values is false, then the metadata is
   * not changed.  New categorical values are mapped to size_t(-1)
   * with a warning.
   *
   * Categorical: If a column is categorical, each unique value is mapped to
   * a unique index in the range 0, ..., n-1, where n is the number of unique
   * values.
   *
   * Numeric: The column type is checked to be INT/FLOAT, then
   * returned as-is.
   *
   * Numeric Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Categorical Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Dictionary : If the dictated column type is DICT, it is checked to make
   * sure the values are numeric. The keys are then translated to 0..n-1
   * where n is the number of unique keys.
   *
   * \param[in,out] metadata The metadata used for the mapping.
   * \param[in] unindexed_x The SFrame to map to indices.
   * \param[in] allow_new_categorical_values Whether to allow new categories.
   *
   * \returns Indexed SFrame.
   */
sframe map_to_indexed_sframe(
    const std::vector<std::shared_ptr<ml_data_internal::column_indexer> >& indexer,
    sframe unindexed_x,
    bool allow_new_categorical_values = true);

  /**
   * Translates an external SFrame into the corresponding indexed
   * SFrame representation, as dictated by the indexing in
   * column_indexer.  Only the columns specified in metadata are
   * used, and all of these must be present.
   *
   * If allow_new_categorical_values is false, then the metadata is
   * not changed.  New categorical values are mapped to size_t(-1)
   * with a warning.
   *
   * Categorical: If a column is categorical, each unique value is mapped to
   * a unique index in the range 0, ..., n-1, where n is the number of unique
   * values.
   *
   * Numeric: The column type is checked to be INT/FLOAT, then
   * returned as-is.
   *
   * Numeric Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Categorical Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Dictionary : If the dictated column type is DICT, it is checked to make
   * sure the values are numeric. The keys are then translated to 0..n-1
   * where n is the number of unique keys.
   *
   * \overload
   *
   * \param[in,out] metadata The metadata used for the mapping.
   * \param[in] unindexed_x The SFrame to map to indices.
   * \param[in] allow_new_categorical_values Whether to allow new categories.
   *
   * \returns Indexed SFrame.
   */
sframe map_to_indexed_sframe(
    const std::shared_ptr<ml_metadata>& metadata,
    sframe unindexed_x,
    bool allow_new_categorical_values = true);

  /**
   * Translates an indexed SArray into the cooriginal non-indexed
   * representation, as dictated by the indexing in column_indexer.
   *
   * \param[in,out] metadata The metadata used for the mapping.
   * \param[in] indexing_x The indexed SArray to map to external values.
   *
   * \returns Indexed SArray in original format.
   */
std::shared_ptr<sarray<flexible_type> > map_from_indexed_sarray(
    const std::shared_ptr<ml_data_internal::column_indexer>& indexer,
    const std::shared_ptr<sarray<flexible_type> >& indexed_x);


  /**
   * Translates an indexed SFrame into the original non-indexed
   * representation, as dictated by the indexing in column_indexer.
   *
   * \param[in,out] metadata The metadata used for the mapping.
   * \param[in] indexing_x The indexed SArray to map to external values.
   *
   * \returns Indexed SFrame in original format.
   */
sframe map_from_indexed_sframe(
    const std::vector<std::shared_ptr<ml_data_internal::column_indexer> >& indexer,
    sframe indexed_x);

  /**
   * Translates an indexed SFrame into the original non-indexed
   * representation, as dictated by the indexing in column_indexer.
   *
   * \param[in,out] metadata The metadata used for the mapping.
   * \param[in] indexing_x The indexed SArray to map to external values.
   *
   * \returns Indexed SFrame in original format.
   */
sframe map_from_indexed_sframe(
    const std::shared_ptr<ml_metadata>& metadata, sframe indexed_x);

  /** Translates an indexed SFrame into the original non-indexed
   * representation, as dictated by the indexing in column_indexer.
   * In this case, the column metadata is contained in a column name
   * to metadata map.
   *
   * \param metadata The metadata used for the mapping.
   *
   * \param external_x The external SArray to map to indices.
   */
sframe map_from_custom_indexed_sframe(
    const std::map<std::string, std::shared_ptr<ml_data_internal::column_indexer> >& indexer,
    sframe indexed_x);


}}

#endif /* TURI_ML2DATA_SFRAME_TRANSLATION_H_  */
