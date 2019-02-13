/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_ROW_REFERENCE_H_
#define TURI_DML_DATA_ROW_REFERENCE_H_

#include <logger/assertions.hpp>
#include <ml_data/data_storage/ml_data_row_format.hpp>
#include <ml_data/data_storage/ml_data_block_manager.hpp>
#include <ml_data/ml_data.hpp>
#include <util/code_optimization.hpp>

#include <Eigen/SparseCore>
#include <Eigen/Core>

#include <array>
#include <type_traits>

namespace turi {

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

/**
 * \ingroup mldata
 *  A class containing a reference to the row of an ml_data instance,
 *  providing access to the underlying data.
 *
 *  In other words, you can do
 *
 *     it->fill(x);
 *
 *  or
 *
 *     auto row_ref = *it;
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
 *     ml_data data;
 *
 *     data.fill(X);
 *
 *     // Get row references
 *
 *     std::vector<ml_data_row_reference> rows(data.num_rows());
 *
 *     for(auto it = data.get_iterator(); !it.done(); ++it) {
 *       rows[it.row_index()] = *it;
 *     }
 *
 *     // Now go through and make sure that each of these hold the
 *     // correct answers.
 *
 *     std::vector<ml_data_entry> x;
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

  /** Create an ml_data_row_reference from a single sframe row reference.
   *
   *  Row must be in the format {column_name, value} and columns
   *  correspond to the columns in metadata.  Missing columns are
   *  treated as missing values.
   *
   *  Returns a single row reference. 
   */
  static GL_HOT ml_data_row_reference from_row(
      const std::shared_ptr<ml_metadata>& metadata, const flex_dict& row,
      ml_missing_value_action none_action = ml_missing_value_action::USE_NAN);
  
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
  inline void fill(std::vector<Entry>& x) const;

  /**
   * Fill a row of an Eigen expression in the current location in the
   * iteration.
   *
   * Example:
   *
   *   Eigen::MatrixXd X;
   *
   *   ...
   *
   *   it.fill(X.row(row_idx));
   *
   * ---------------------------------------------
   *
   * \param[in,out] x   An eigen row expression.
   *
   */
  template <typename EigenXpr>
  GL_HOT_INLINE_FLATTEN
  inline void fill(
      EigenXpr&& x,
      typename std::enable_if<std::is_convertible<EigenXpr, DenseVector>::value >::type* = 0) const {

    fill_eigen(x);
  }

  /**
   * Fill an observation vector with the untranslated columns, if any
   * have been specified at setup time.  These columns are simply
   * mapped back to their sarray counterparts.
   */
  void fill_untranslated_values(std::vector<flexible_type>& x) const;

  /**  The explicit version to fill an eigen expression.
   */
  template <typename EigenXpr>
  GL_HOT_INLINE_FLATTEN
  inline void fill_eigen(EigenXpr&& x) const;

  /**  A generic function to unpack the values into a particular
   *   format.  This allows, e.g. custom distance functions and stuff
   *   to work out well.
   *
   *   // Called for every element.
   *   // mode: What type of column it is.
   *   // feature_index: index within the column
   *   // index_size: number of features in this column.
   *   // index_offset: The global index would be index_offset + feature_index
   *   // value: the value of the feature.
   *
   *   auto unpack_function =
   *   [&](ml_column_mode mode, size_t column_index,
   *       size_t feature_index, double value,
   *       size_t index_size, size_t index_offset) {
   *     ...
   *   };
   *
   *   // Called after every column is done unpacking.
   *   auto column_end_function =
   *     [&](ml_column_mode mode, size_t column_index, size_t index_size) {
   *     ...
   *    };
   */
  template <typename ElementWriteFunction,
            typename ColumnEndFunction>
  GL_HOT_INLINE_FLATTEN
  void unpack(ElementWriteFunction&& ewf, ColumnEndFunction&& cef) const {

    if(UNLIKELY(!has_translated_columns))
      return;

    DASSERT_TRUE(data_block != nullptr);
    DASSERT_TRUE(data_block->metadata != nullptr);
    DASSERT_TRUE(data_block->translated_rows.entry_data.size() != 0);

    const ml_data_internal::row_metadata& rm = data_block->rm;
    ml_data_internal::entry_value_iterator row_block_ptr = current_data_iter();

    read_ml_data_row(rm, row_block_ptr, ewf, cef);
  }

  /** Returns the current target value, if present, or 1 if not
   *  present.  If the target column is supposed to be a categorical
   *  value, then use target_index().
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
  friend class turi::ml_data_iterator;

  std::shared_ptr<ml_data_internal::ml_data_block> data_block;
  size_t current_in_block_index = size_t(-1);
  size_t current_in_block_row_index = size_t(-1);
  bool has_translated_columns = false;
  bool has_untranslated_columns = false;

  /** Return a pointer to the current location in the data.
   */
  inline ml_data_internal::entry_value_iterator current_data_iter() const GL_HOT_INLINE_FLATTEN {
    
#ifndef NDEBUG
    if(data_block->translated_rows.entry_data.empty()) {
      ASSERT_EQ(current_in_block_index, 0);
    } else {
      ASSERT_LT(current_in_block_index, data_block->translated_rows.entry_data.size());
    }
#endif
    
    // Note, this may be nullptr in the case of only untranslated columns and no targets.
    return data_block->translated_rows.entry_data.data() + current_in_block_index;
  }


};

////////////////////////////////////////////////////////////////////////////////
// Implementations of the above

template <typename Entry>
GL_HOT_INLINE
void ml_data_row_reference::fill(std::vector<Entry>& x) const {

  x.clear();

  unpack(

      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

        size_t global_index = (LIKELY(feature_index < index_size)
                               ? index_offset + feature_index
                               : size_t(-1));

        Entry e;
        e = ml_data_full_entry{column_index, feature_index, global_index, value};
        x.push_back(e);
      },

      // Nothing that we need to do at the end of each column.
      [&](ml_column_mode, size_t, size_t) {});
}

////////////////////////////////////////////////////////////////////////////////
// fill eigen stuff

template <typename EigenXpr>
GL_HOT_INLINE_FLATTEN
inline void ml_data_row_reference::fill_eigen(EigenXpr&& x) const {

  x.setZero();

  unpack(

      /** The function to write out the data to x.
       */
      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) {

        if(UNLIKELY(feature_index >= index_size))
          return;

        size_t idx = index_offset + feature_index;

        DASSERT_GE(idx,  0);
        DASSERT_LT(idx, size_t(x.size()));
        x.coeffRef(idx) = value;
      },

      /** The function to advance the offset, called after each column
       *  is finished.
       */
      [&](ml_column_mode mode, size_t column_index, size_t index_size) {});
}

}

#endif /* TURI_ML_DATA_ROW_REFERENCE_H_ */
