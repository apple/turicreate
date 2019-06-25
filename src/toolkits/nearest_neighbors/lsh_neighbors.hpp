/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LSH_NEIGHBORS_H_
#define TURI_LSH_NEIGHBORS_H_


// Toolkits
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/lsh_family.hpp>

namespace turi {
namespace nearest_neighbors {

/**
 *  LSH nearest neighbor class.
 *
 *  The intuition behind LSH-based indexes is to hash data points into
 *  buckets, such that similar points are more likely to be hashed to the same
 *  bucket than dissimilar ones. We could then find the approximate nearest
 *  neighbors of any point, simply by finding the bucket that it is hashed to.
 *
 *  It works as follows:
 *
 *  1. Choose k hash functions h_1, h_2, ..., h_k from a uniform of some
 *  family of LSH functions. For any data point v, place v in the bucket with
 *  key g(v) = (h_1(v), h_2(v), ..., h_k(v)).
 *  2. Independently perform step 1 l times to construct l separate hash
 *  tables, with hash functions g_1, g_2, ..., g_l
 *
 *  You can set k and l by setting num_projections_per_table and
 *  num_tables respectively.
 *
 *
 */
class EXPORT lsh_neighbors: public nearest_neighbors_model {

 public:

  static constexpr size_t LSH_NEIGHBORS_VERSION = 1;

  /**
   * Destructor. Make sure bad things don't happen
   */
  ~lsh_neighbors();

  /**
   * Set the model options. Use the option manager to set these options. The
   * option manager should throw errors if the options do not satisfy the option
   * manager's conditions.
   *
   * \param[in] opts Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _opts) override;

  /**
   * Create a LSH nearest neighbors model.
   *
   * \param[in] X sframe input feature data
   * \param[in] ref_labels row labels for the reference dataset
   * \param[in] composite_distance_params
   * \param[in] y sframe input labels
   */
  void train(const sframe& X, const std::vector<flexible_type>& ref_labels,
             const std::vector<dist_component_type>& composite_distance_params,
             const std::map<std::string, flexible_type>& opts) override;

  /**
   * Find neighbors of queries in a created LSH model.
   *
   * For each query, the method keeps track of the current k-nearest neighbors
   * in the LSH. At each node, the closest possible point in each child
   * node to the query  is computed, and if this distance is further than the
   * current k'th nearest neighbor, that child node (and its descendants) is
   * skpped in the traversal.
   *
   * \param[in] mld_queries query data
   * \param[in] query_labels sframe query labels
   * \param[in] k size_t max number of neighbors to return for each query
   * \param[in] radius double max distance for returned neighbors to each query
   *
   * \param[out] ret sframe SFrame with four columns: query label, reference
   * label, distance, and rank.
   *
   * \note Assumes that data is already in the right shape.
   */
  sframe query(const v2::ml_data& mld_queries,
               const std::vector<flexible_type>& query_labels,
               const size_t k, const double radius,
               const bool include_self_edges) const override;


  /**
   * Gets the model version number
   */
  inline size_t get_version() const override {
    return LSH_NEIGHBORS_VERSION;
  }

  /**
   * Turi serialization save
   */
  void save_impl(turi::oarchive& oarc) const override;

  /**
   * Turi serialization save
   */
  void load_version(turi::iarchive& iarc, size_t version) override;

  // TODO: convert interface above to use the extensions methods here
  BEGIN_CLASS_MEMBER_REGISTRATION("nearest_neighbors_lsh")
  REGISTER_CLASS_MEMBER_FUNCTION(lsh_neighbors::list_fields)
  END_CLASS_MEMBER_REGISTRATION

 private:
  std::shared_ptr<lsh_family> lsh_model;
};


}  // namespace nearest_neighbors
}  // namespace turi

#endif
