/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_ITERATOR_BASE_H_
#define TURI_ML2_DATA_ITERATOR_BASE_H_

#include <core/logging/assertions.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_row_translation.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_block_manager.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/side_features.hpp>
#include <toolkits/ml_data_2/iterators/composite_row_type.hpp>
#include <toolkits/ml_data_2/iterators/row_reference.hpp>
#include <core/util/code_optimization.hpp>

// SArray and Flex type
#include <core/storage/sframe_data/sarray.hpp>

#include <Eigen/SparseCore>
#include <Eigen/Core>

#include <array>

namespace turi { namespace v2 {

class ml_data;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

/**
 * Just a simple iterator on the ml_data class.  It's just a
 * convenience structure that keeps track of everything relevant for
 * the toolkits.
 */
class ml_data_iterator_base {
 private:

  // To be initialized only from the get_iterator() method of ml_data.
  friend class ml_data;

  /**
   * Default method of constructing the data.
   *
   * \param[in] ml_init      ML Data iterator initializer.
   */
  void setup(const ml_data& _data,
             const ml_data_internal::row_metadata& rm,
             size_t thread_idx, size_t num_threads,
             const std::map<std::string, flexible_type>& options);

 protected:

  virtual void internal_setup(const std::map<std::string, flexible_type>& options) {}

 public:

  /**  Yup, need this.
   */
  virtual ~ml_data_iterator_base(){}

 public:

  ml_data_iterator_base(){}
  ml_data_iterator_base(const ml_data_iterator_base&) = delete;
  ml_data_iterator_base(ml_data_iterator_base&&) = default;

  ml_data_iterator_base& operator=(const ml_data_iterator_base&) = delete;
  ml_data_iterator_base& operator=(ml_data_iterator_base&&) = default;

  ///  Resets the iterator to the start of the sframes in ml_data.
  virtual void reset();

  /// Returns true if the iteration is done, false otherwise.
  virtual inline bool done() const { return current_row_index == iter_row_index_end; }

  /// Returns the current index of the sframe row, respecting all
  /// slicing operations on the original ml_data.
  inline size_t row_index() const { return current_row_index - global_row_start; }

  /// Returns the absolute row index
  inline size_t unsliced_row_index() const { return current_row_index; }

  /**
   * Fill an observation vector, represented as an ml_data_entry struct.
   * (column_index, index, value) pairs, from the current location in the
   * iteration.  For each column:
   *
   * Categotical: Returns (col_id, v, 1)
   * Numeric    : Returns (col_id, 0, v)
   * Vector     : Returns (col_id, i, v) for each (i,v) in vector.
   *
   * Example use is given by the following code:
   *
   *   std::vector<ml_data_entry> x;
   *
   *   for(ml_data_iterator it(data); !it.is_done(); ++it) {
   *      it.fill_observation(x);
   *      double y = it.target_value();
   *      ...
   *   }
   */
  template <typename Entry>
  GL_HOT_INLINE
  inline void fill_observation(std::vector<Entry>& x) const {


    if(UNLIKELY(x.capacity() < max_row_size)) {
      x.reserve(max_row_size);
    }

    x.clear();

    if(!has_translated_columns)
      return;

    ml_data_internal::copy_raw_into_ml_data_entry_row(
        x, rm, current_data_iter(),
        side_features);

    DASSERT_LE(x.size(), data->max_row_size());
  }

  /**
   * Fill an observation vector with the untranslated columns, if any
   * have been specified at setup time.  These columns are simply
   * mapped back to their sarray counterparts.
   *
   * The metadata surrounding the original column indices are
   */
  inline void fill_untranslated_values(std::vector<flexible_type>& x) const GL_HOT_INLINE_FLATTEN {

    if(!has_untranslated_columns) {
      x.clear();
      return;
    }

    x.resize(data_block->untranslated_columns.size());

    size_t row_index = current_block_row_index();

    for(size_t i = 0; i < data_block->untranslated_columns.size(); ++i) {
      x[i] = data_block->untranslated_columns[i][row_index];
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
   * std::pair version of fill_observation.
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
  inline void fill_observation(SparseVector& x) const GL_HOT_INLINE_FLATTEN {


    x.setZero();

    if(!has_translated_columns)
      return;

    ml_data_internal::copy_raw_into_eigen_array(
        x,
        rm, current_data_iter(),
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
   * std::pair version of fill_observation.
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
  inline void fill_observation(DenseVector& x) const  GL_HOT_INLINE_FLATTEN {

    x.setZero();

    if(!has_translated_columns)
      return;

    ml_data_internal::copy_raw_into_eigen_array(
        x,
        rm, current_data_iter(),
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
        rm, current_data_iter(),
        side_features,
        use_reference_encoding);
  }


  /** Fill a composite row container.  The composite row container
   *  must have its specification set; this specification is used to
   *  then fill the observation.
   */
  inline void fill_observation(composite_row_container& crc) GL_HOT_INLINE_FLATTEN {

    DASSERT_TRUE(crc.subrow_spec != nullptr);

    fill_untranslated_values(crc.flextype_buffer);
    crc.subrow_spec->fill(crc, rm, current_data_iter(), crc.flextype_buffer);
  }


  /** Returns the current target value, if present, or 1 if not
   *  present.  If the target column is supposed to be a categorical
   *  value, then use categorical_target_index().
   */
  double target_value() const GL_HOT_INLINE_FLATTEN {

    DASSERT_FALSE(done());
    DASSERT_FALSE(current_in_block_index == data_block->translated_rows.entry_data.size());

    return get_target_value(rm, current_data_iter());
  }

  /** Returns the current categorical target index, if present, or 0
   *  if not present.
   */
  size_t target_index() const GL_HOT_INLINE_FLATTEN {

    DASSERT_FALSE(done());
    DASSERT_FALSE(current_in_block_index == data_block->translated_rows.entry_data.size());

    return get_target_index(rm, current_data_iter());
  }

  ////////////////////////////////////////////////////////////////////////////////

  /**  Return a row reference instead of the actual observation.  The
   *   row reference can be used to fill the observation vectors just
   *   like the iterator can, and can easily be passed around by value.
   */
  ml_data_row_reference get_reference() const {
    ml_data_row_reference ref;

    ref.data_block = data_block;
    ref.side_features = side_features;
    ref.current_in_block_index = current_in_block_index;
    ref.use_reference_encoding = use_reference_encoding;

    return ref;
  }


  ////////////////////////////////////////////////////////////////////////////////

  /** Return the data this iterator is working with.
   */
  inline const ml_data& ml_data_source() const {
    return *data;
  }

  /** Return the raw value of the internal row storage.  Used by some
   * of the internal ml_data processing routines.
   */
  inline ml_data_internal::entry_value _raw_row_entry(size_t raw_index) const GL_HOT_INLINE_FLATTEN {

    if(!rm.data_size_is_constant)
      ++raw_index;

    return *(current_data_iter() + raw_index);
  }

 protected:

  // Internally, ml_data is just a bunch of shared pointers, so it's
  // not expensive to store a copy.
  std::shared_ptr<ml_data> data;

  ml_data_internal::row_metadata rm;

  std::shared_ptr<ml_data_side_features> side_features;

  /** The options used for this iterator.
   */
  bool add_side_information = false;
  bool use_reference_encoding = false;
  bool has_untranslated_columns = false;
  bool has_translated_columns = false;

  size_t row_block_size = -1;
  size_t iter_row_index_start = -1;   /**< Starting row index for this iterator. */
  size_t iter_row_index_end = -1;     /**< Ending row index for this iterator. */
  size_t current_row_index = -1;      /**< Current row index for this iterator. */
  size_t current_block_index = -1;    /**< Index of the currently loaded block. */

  /** The current index pointed to inside the block.
   */
  size_t current_in_block_index;

  /** The absolute values of the global row starting locations.
   */
  size_t global_row_start, global_row_end;

  /** The maximum row size across all rows in the given ml_data object.
   * Each row's size is defined to be the number of unpacked features in that
   * row. For example, this is useful when one needs to preallocate a vector
   * to be the largest size needed for any row that will be given by this
   * iterator.
   */
  size_t max_row_size;

  /** The total sum of column sizes.
   */
  size_t num_dimensions;

 private:

  /** A pointer to the current block.
   */
  std::shared_ptr<ml_data_internal::ml_data_block> data_block;

 protected:

  /** Return a pointer to the current location in the data.
   */
  inline ml_data_internal::entry_value_iterator current_data_iter() const GL_HOT_INLINE_FLATTEN {

    DASSERT_FALSE(done());
    DASSERT_LT(current_in_block_index, data_block->translated_rows.entry_data.size());

    return &(data_block->translated_rows.entry_data[current_in_block_index]);
  }

  /** Return a pointer to the current location in the data.
   */
  inline size_t current_block_row_index() const GL_HOT_INLINE_FLATTEN {

    size_t index = current_row_index - (current_block_index * row_block_size);

    DASSERT_FALSE(done());
    DASSERT_LT(index, row_block_size);

    return index;
  }


  /** Advance to the next row.
   */
  inline void advance_row() GL_HOT_INLINE_FLATTEN {

    if(has_translated_columns)
      current_in_block_index += get_row_data_size(rm, current_data_iter());

    ++current_row_index;

    if(current_row_index == (current_block_index + 1) * row_block_size && !done())
      load_next_block();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Internal reader functions

  /// Loads the block containing the row index row_index
  void setup_block_containing_current_row_index() GL_HOT_NOINLINE;

  /// Loads the next block, resetting all the values so iteration will
  /// be supported over the next row.
  void load_next_block() GL_HOT_NOINLINE;

};

}}

#endif /* TURI_ML2_DATA_ITERATOR_H_ */
