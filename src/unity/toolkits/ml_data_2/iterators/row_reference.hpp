/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML_DATA_ROW_REFERENCE_H_
#define TURI_ML_DATA_ROW_REFERENCE_H_

#include <logger/assertions.hpp>
#include <unity/toolkits/ml_data_2/data_storage/ml_data_row_translation.hpp>
#include <unity/toolkits/ml_data_2/data_storage/ml_data_block_manager.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/side_features.hpp>
#include <util/code_optimization.hpp>

#include <Eigen/SparseCore>
#include <Eigen/Core>

#include <array>

namespace turi { namespace v2 {

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

/**
 *  A class containing a reference to the row of an ml_data instance.
 *  The row can then be used to fill any sort of data row that an
 *  iterator can be used to fill.
 *
 *  In other words,
 *
 *     it.fill_observation(x);
 *
 *  Can be replaced with
 *
 *     auto row_ref = it.get_reference();
 *
 *     // do stuff ...
 *     row_ref.fill(x); 
 *
 *  The data block pointed to by this reference is kept alive as long
 *  as this reference class exists. 
 *
 *
 *  Another example of how it is used is below:
 *
 *     sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} } );
 * 
 *     v2::ml_data data;
 * 
 *     data.fill(X);
 * 
 *     // Get row references
 * 
 *     std::vector<v2::ml_data_row_reference> rows(data.num_rows()); 
 * 
 *     for(auto it = data.get_iterator(); !it.done(); ++it) {
 *       rows[it.row_index()] = it.get_reference();
 *     }
 * 
 *     // Now go through and make sure that each of these hold the
 *     // correct answers.
 *
 *     std::vector<v2::ml_data_entry> x; 
 * 
 *     for(size_t i = 0; i < rows.size(); ++i) {
 *
 *       // The metadata for the row is the same as that in the data. 
 *       ASSERT_TRUE(rows[i].metadata().get() == data.metadata().get());
 *       
 *       rows[i].fill(x);
 * 
 *       ASSERT_EQ(x.size(), 2);
 *
 *       ASSERT_EQ(x[0].column_index, 0); 
 *       ASSERT_EQ(x[0].index, 0); 
 *       ASSERT_EQ(x[0].value, i);
 *
 *       ASSERT_EQ(x[1].column_index, 1); 
 *       ASSERT_EQ(x[1].index, 0); 
 *       ASSERT_EQ(x[1].value, i);
 *     }
 *   }
 *
 */
class ml_data_row_reference {
 public:

  /**
   * Fill an observation vector, represented as an ml_data_entry
   * struct.  (column_index, index, value) pairs, from this row
   * reference.  For each column:
   *
   * Categotical: Returns (col_id, v, 1)
   * Numeric    : Returns (col_id, 0, v)
   * Vector     : Returns (col_id, i, v) for each (i,v) in vector.
   *
   * Example use is given by the following code:
   *
   *   std::vector<ml_data_entry> x;
   *
   *   row_ref.fill(x);
   *   double y = row_ref.target_value();
   *   ...
   */
  template <typename Entry>
  GL_HOT_INLINE
  inline void fill(std::vector<Entry>& x) const {

    x.clear();

    if(!data_block->metadata->has_translated_columns())
      return;

    ml_data_internal::copy_raw_into_ml_data_entry_row(
        x, data_block->rm, current_data_iter(),
        side_features);
  }
  
  /**
   * Fill an observation vector with the untranslated columns, if any
   * have been specified at setup time.  These columns are simply
   * mapped back to their sarray counterparts.
   */
  inline void fill_untranslated_values(std::vector<flexible_type>& x) const GL_HOT_INLINE_FLATTEN {

    if(!data_block->metadata->has_untranslated_columns()) {
      x.clear();
      return;
    }

    x.resize(data_block->untranslated_columns.size());

    for(size_t i = 0; i < data_block->untranslated_columns.size(); ++i) {
      x[i] = data_block->untranslated_columns[i][current_in_block_index];
    }

    DASSERT_TRUE(x.size() >= 1);
  }


  /**
   * Fill an observation vector, represented as an Eigen Sparse Vector, from
   * the current location in the iteration.
   *
   * \note A reference category is used in this version of the function.
   * \note For performance reasons, this function does not check for new
   * categories during predict time.  That must be checked externally.
   *
   * This function returns a flattened version of the vector provided by the
   * std::pair version of fill.
   *
   * Example
   * ---------------------------------------------
   *
   * \warning This only works when the SFrame is "mapped" to integer keys.
   *
   * For a dataset with a 3 column SFrame
   *
   * Row 1: 1.0  0(categorical) <9.1, 2.4>
   * Row 2: 2.0  1(categorical) <1.0, 4.5>
   *
   * with index = {1,2,2}
   *
   * the SparseVector format would return
   *
   * Row 1: < (0, 1.0), (1, 1) ,(3, 9.1) ,(4, 2.4)>
   * Row 2: < (0, 2.0), (2, 1) ,(3, 1.0) ,(4, 4.5)>
   *
   * \note The '0'th category is used as reference.
   *
   * \param[in,out] x   Data containing everything!
   *
   */
  inline void fill(SparseVector& x) const GL_HOT_INLINE_FLATTEN {
    
    x.setZero();

    if(!data_block->metadata->has_translated_columns())
      return;

    ml_data_internal::copy_raw_into_eigen_array(
        x,
        data_block->rm, current_data_iter(),
        side_features,
        use_reference_encoding);
  }

  /**
   * Fill an observation vector, represented as an Eigen Dense Vector, from
   * the current location in the iteration.
   *
   * \note The 0th category is used as a reference category.
   *
   * \note For performance reasons, this function does not check for new
   * categories during predict time.  That must be checked externally.
   *
   * This function returns a flattened version of the vector provided by the
   * std::pair version of fill.
   *
   * Example
   * ---------------------------------------------
   *
   * \warning This only works when the SFrame is "mapped" to intger keys.
   *
   * For a dataset with a 3 column SFrame
   *
   * Row 1: 1.0  0(categorical) <9.1, 2.4>
   * Row 2: 2.0  1(categorical) <1.0, 4.5>
   *
   * with index = {1,2,2}
   *
   * the DenseVector format would return
   *
   * Row 1: <1.0, 0, 1, 9.1, 2.4>
   * Row 2: <2.0, 1, 0, 1.0, 4.5>
   *
   * \param[in,out] x   Data containing everything!
   *
   */
  inline void fill(DenseVector& x) const  GL_HOT_INLINE_FLATTEN {

    x.setZero();

    if(!data_block->metadata->has_translated_columns())
      return;

    ml_data_internal::copy_raw_into_eigen_array(
        x,
        data_block->rm, current_data_iter(),
        side_features,
        use_reference_encoding);
  }

  /**
   * Fill a row of an Eigen Dense Vector, from
   * the current location in the iteration.
   *
   * \note The 0th category is used as a reference category.
   *  
   *
   * Example:
   *
   *   Eigen::MatrixXd X;
   *
   *   ...
   *
   *   it.fill_eigen_row(X.row(row_idx));
   *  
   * ---------------------------------------------
   *
   * \param[in,out] x   An eigen row expression.
   *
   */
  template <typename DenseRowXpr>
  GL_HOT_INLINE_FLATTEN
  inline void fill_eigen_row(DenseRowXpr&& x) const {

    x.setZero();
    
    ml_data_internal::copy_raw_into_eigen_array(
        x,
        data_block->rm, current_data_iter(),
        side_features,
        use_reference_encoding);
  }
  
  /** Returns the current target value, if present, or 1 if not
   *  present.  If the target column is supposed to be a categorical
   *  value, then use categorical_target_index().
   */
  double target_value() const GL_HOT_INLINE_FLATTEN {
    return get_target_value(data_block->rm, current_data_iter());
  }

  /** Returns the current categorical target index, if present, or 0
   *  if not present.
   */
  size_t target_index() const GL_HOT_INLINE_FLATTEN {
    return get_target_index(data_block->rm, current_data_iter());
  }

  /** Returns a pointer to the metadata class that describes the data
   *  that this row reference refers to.
   */
  const std::shared_ptr<ml_metadata>& metadata() const {
    return data_block->metadata;
  }

  
 private:
  friend class ml_data_iterator_base;

  std::shared_ptr<ml_data_internal::ml_data_block> data_block;
  std::shared_ptr<ml_data_side_features> side_features;
  size_t current_in_block_index = size_t(-1);
  bool use_reference_encoding = false;

  /** Return a pointer to the current location in the data.
   */
  inline ml_data_internal::entry_value_iterator current_data_iter() const GL_HOT_INLINE_FLATTEN {

    DASSERT_LT(current_in_block_index, data_block->translated_rows.entry_data.size());

    return &(data_block->translated_rows.entry_data[current_in_block_index]);
  }

};


}}

#endif /* TURI_ML_DATA_ROW_REFERENCE_H_ */
