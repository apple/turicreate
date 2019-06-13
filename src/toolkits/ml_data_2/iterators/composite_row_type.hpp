/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML_DATA_ROW_TYPES_H_
#define TURI_ML_DATA_ROW_TYPES_H_

#include <vector>
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <core/data/flexible_type/flexible_type.hpp>

namespace turi { namespace v2 {

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  dense_vector;
typedef Eigen::SparseVector<double> sparse_vector;

class composite_row_specification;

/** A collection of subrows to put into a composite row container.
 *  Requires a composite_row_specification to first be defined; this
 *  specification determines how the container is going to be filled
 *  by the iterator.
 *
 *  The following code segment illustrates how this is to be used:
 *
 *     // Create a simple sframe
 *     sframe X = make_integer_testing_sframe( {"C0", "C1", "C2"}, { {1,2,3}, {4,5,6} } );
 *
 *     v2::ml_data data;
 *
 *     // Set column "C0" to be untranslated.
 *     data.set_data(X, "", {}, { {"C0", v2::ml_column_mode::UNTRANSLATED} });
 *
 *     data.fill();
 *
 *     auto row_spec = std::make_shared<v2::composite_row_specification>(data.metadata());
 *
 *     // Add one dense subrow formed from columns 1 and 2
 *     size_t dense_row_index_1  = row_spec->add_dense_subrow( {1, 2} );
 *
 *     // Add a sparse subrow formed from column 2
 *     size_t sparse_row_index   = row_spec->add_sparse_subrow( {2} );
 *
 *     // Add an untranslated row formed from column 0
 *     size_t flex_row_index     = row_spec->add_flex_type_subrow( {0} );
 *
 *     // Add another dense subrow formed from column 1
 *     size_t dense_row_index_2  = row_spec->add_dense_subrow( {1} );
 *
 *     v2::composite_row_container crc(row_spec);
 *
 *     ////////////////////////////////////////
 *
 *     auto it = data.get_iterator();
 *
 *     {
 *       it.fill_observation(crc);
 *
 *       // The 1st dense component; two numerical columns.
 *       const auto& vd = crc.dense_subrows[dense_row_index_1];
 *
 *       ASSERT_EQ(vd.size(), 2);
 *       ASSERT_EQ(size_t(vd[0]), 2);  // First row, 2nd column
 *       ASSERT_EQ(size_t(vd[1]), 3);  // First row, 3nd column
 *
 *       // The 2nd dense component; one numerical columns.
 *       const auto& vd2 = crc.dense_subrows[dense_row_index_2];
 *
 *       ASSERT_EQ(vd2.size(), 1);
 *       ASSERT_EQ(size_t(vd2[0]), 2);  // First row, 2nd column
 *
 *       // The sparse component: one numerical column
 *       const auto& vs = crc.sparse_subrows[sparse_row_index];
 *
 *       ASSERT_EQ(vs.size(), 1);
 *       ASSERT_EQ(size_t(vs.coeff(0)), 3);  // First row, 3nd column
 *
 *       // The untranslated column.
 *       const auto& vf = crc.flex_subrows[flex_row_index];
 *
 *       ASSERT_EQ(vf.size(), 1);
 *       ASSERT_TRUE(vf[0] == 1);   // First row, 1st column
 *     }
 *
 */
struct composite_row_container {

  composite_row_container(const std::shared_ptr<composite_row_specification>& _subrow_specs)
      : subrow_spec(_subrow_specs)
  {}

  std::vector<dense_vector> dense_subrows;
  std::vector<sparse_vector> sparse_subrows;
  std::vector<std::vector<flexible_type> > flex_subrows;

 private:

  friend class ml_data_iterator_base;
  friend class composite_row_specification;

  // Used to fill the row above.
  std::shared_ptr<composite_row_specification> subrow_spec;

  // Used by the composite_row_specification filling function
  std::vector<size_t> buffer;
  std::vector<flexible_type> flextype_buffer;
};


/** The specification for a composite row container.  See
 *  documentation on composite_row_container for use.
 */
class composite_row_specification {
 public:

  /** Constructor; requires a metadata object.
   */
  composite_row_specification(const std::shared_ptr<ml_metadata>& _metadata);

  /** Add in a sparse subrow.  Returns the index in the sparse_subrows
   *  attribute of the composite_row_container where this particular
   *  row will go.
   */
  size_t add_sparse_subrow(const std::vector<size_t>& column_indices);

  /** Add in a dense subrow.  Returns the index in the dense_subrows
   *  attribute of the composite_row_container where this particular
   *  row will go upon filling from the iterator.
   */
  size_t add_dense_subrow(const std::vector<size_t>& column_indices);

  /** Add in a flexible type subrow.  Returns the index in the
   *  flex_subrows attribute of the composite_row_container where this
   *  particular row will go upon filling from the iterator.
   */
  size_t add_flex_type_subrow(const std::vector<size_t>& column_indices);

 private:

  friend class ml_data_iterator_base;

  std::shared_ptr<ml_metadata> metadata;

  size_t n_dense_subrows = 0;
  size_t n_sparse_subrows = 0;
  size_t n_flex_subrows = 0;

  // These are indexed by columns; each entry contains the subrow
  // indices that use that particular column.
  std::vector<std::vector<size_t> > dense_spec;
  std::vector<std::vector<size_t> > sparse_spec;

  // These are indexed by subrow; each contains the column indices
  // used by that particular subrow.
  std::vector<std::vector<size_t> > flex_subrow_spec_by_subrow;

  // Sizes for the dense and sparse rows
  std::vector<size_t> dense_spec_sizes;
  std::vector<size_t> sparse_spec_sizes;

  /** Fill the composite container; called by the iterator.
   */
  void fill(composite_row_container& crc,
            const ml_data_internal::row_metadata& rm,
            ml_data_internal::entry_value_iterator row_block_ptr,
            std::vector<flexible_type> flexible_type_row) GL_HOT_FLATTEN;

};



}}

#endif
