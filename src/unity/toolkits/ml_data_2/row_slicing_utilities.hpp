/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef ML_DATA_ROW_SLICING_UTILITIES_H_
#define ML_DATA_ROW_SLICING_UTILITIES_H_

#include <vector>
#include <unity/toolkits/ml_data_2/metadata.hpp> 
#include <flexible_type/flexible_type.hpp>
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <unity/lib/variant.hpp> 
#include <unity/lib/variant_deep_serialize.hpp> 

namespace turi { namespace v2 {

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  dense_vector;
typedef Eigen::SparseVector<double> sparse_vector;

/** A slicer class that allows taking a row and splitting it up by
 *  columns.
 */
class row_slicer {

 public:

  row_slicer() {} 
  
  /** Constructor -- provide ml_metadata class and a subset of column
   *  indices to use in this particular row.  the columns_to_pick must
   *  be in sorted order.
   *
   *  If the chosen columns are from untranslated columns, then they
   *  must be all untranslated columns.  In this case, only the
   *  flexible_type slice method below can be used.  Otherwise, none
   *  of the columns must be untranslated, and either the sparse or
   *  dense slicing methods must be used.
   *
   *  Example:
   *
   *    sframe X = make_integer_testing_sframe( {"C0", "C1", "C2"}, { {1,2,3}, {4,5,6} } );
   *
   *    v2::ml_data data;
   *
   *    data.fill(X);
   *
   *    std::vector<v2::ml_data_entry> x_t;
   *    std::vector<flexible_type> x_u;
   *
   *    // Select that we want columns 1 and 2, but drop 0.
   *    v2::row_slicer s_c1_c2(data.metadata(), {1, 2} );
   *
   *    v2::dense_vector vd; 
   *    v2::sparse_vector vs; 
   *    std::vector<flexible_type> vu;
   *    
   *    ////////////////////////////////////////
   *
   *    auto it = data.get_iterator();
   *
   *    it.fill_observation(x_t);
   *    it.fill_untranslated_values(x_u);
   *
   *    s_c1_c2.slice(vd, x_t, x_u);
   *
   *    // There are 2 numerical columns included in this test
   *    ASSERT_EQ(vd.size(), 2);
   *    ASSERT_EQ(size_t(vd[0]), 2);  // First row, 2nd column, by the slicer
   *    ASSERT_EQ(size_t(vd[1]), 3);  // First row, 3nd column, by the slicer
   *
   *    s_c1_c2.slice(vs, x_t, x_u);
   *
   *    ASSERT_EQ(vd.nonZeros(), 2);
   *    ASSERT_EQ(size_t(vd.coeffRef(0)), 2);  // First row, 2nd column, by the slicer
   *    ASSERT_EQ(size_t(vd.coeffRef(1)), 3);  // First row, 2nd column, by the slicer
   *
   *
   *  ================================================================================
   *
   *  Example with untranslated columns:
   *        sframe X = make_integer_testing_sframe( {"C0", "C1", "C2"}, { {1,2,3}, {4,5,6} } );
   *
   *      v2::ml_data data;
   *
   *      // Set column C1 and C2 to be untranslated.
   *      data.set_data(X, "", {},
   *                    { {"C1", v2::ml_column_mode::UNTRANSLATED},
   *                      {"C2", v2::ml_column_mode::UNTRANSLATED} });
   *      
   *      data.fill();
   *      
   *      std::vector<v2::ml_data_entry> x_t;
   *      std::vector<flexible_type> x_u;
   *
   *      // Take the second and third columns
   *      v2::row_slicer s_c1_c2(data.metadata(), {1, 2} );
   *
   *      std::vector<flexible_type> vu;
   *      
   *      auto it = data.get_iterator();
   *
   *      it.fill_observation(x_t);
   *      it.fill_untranslated_values(x_u);
   *
   *      s_c1_c2.slice(vu, x_t, x_u);
   *
   *      // There are 2 numerical columns included in this test
   *      ASSERT_EQ(vu.size(), 2);
   *      ASSERT_EQ(size_t(vu[0]), 2);  // First row, 2nd column, by the slicer
   *      ASSERT_EQ(size_t(vu[1]), 3);  // First row, 3nd column, by the slicer
   *
   *      ++it;
   */
  row_slicer(const std::shared_ptr<ml_metadata>& metadata,
             const std::vector<size_t>& columns_to_pick); 

  /**  Take a row, represented by a pair of translated and
   *   untranslated columns (either of which may be empty), and
   *   use it to fill an eigen sparse vector with the result.
   */
  void slice(sparse_vector& dest,
             const std::vector<ml_data_entry>& x_t, const std::vector<flexible_type>& x_u) const;

  /**  Take a row, represented by a pair of translated and
   *   untranslated columns (either of which may be empty), and
   *   use it to fill an eigen dense vector with the result.
   */
  void slice(dense_vector& dest,
             const std::vector<ml_data_entry>& x_t, const std::vector<flexible_type>& x_u) const;

  /**  Take a row, represented by a pair of translated and
   *   untranslated columns (either of which may be empty), and
   *   use it to fill an untranslated row with the result. 
   */
  void slice(std::vector<flexible_type>& dest,
             const std::vector<ml_data_entry>& x_t, const std::vector<flexible_type>& x_u) const;

  /**  For translated row types, this returns the number of dimensions
   *  present.  The eigen dense vectors will have this size after a
   *  call to slice above.
   */
  size_t num_dimensions() { return _num_dimensions; }

  /** Serialization -- save.
   */
  void save(turi::oarchive& oarc) const;

  /** Serialization -- load.
   */
  void load(turi::iarchive& iarc);

  
 private:
  
  bool pick_from_flexible_type = false; 

  std::vector<size_t> flex_type_columns_to_pick;
  
  std::vector<int> column_pick_mask; 

  std::vector<size_t> index_offsets; 
  std::vector<size_t> index_sizes;
  
  size_t _num_dimensions = 0;
};

}}

#endif
