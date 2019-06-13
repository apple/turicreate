/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_BRUTE_FORCE_NEIGHBORS_H_
#define TURI_BRUTE_FORCE_NEIGHBORS_H_

// Types
#include <core/parallel/atomic.hpp>

// ML-Data Utils

// Toolkits
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>

namespace turi {
namespace nearest_neighbors {



class EXPORT brute_force_neighbors: public nearest_neighbors_model {

 protected:

  // bool is_dense = true;                  // Indicates if SparseVector is needed


 public:

  static constexpr size_t BRUTE_FORCE_NEIGHBORS_VERSION = 2;

  /**
   * Destructor. Make sure bad things don't happen
   */
  ~brute_force_neighbors();

  /**
   * Set the model options. Use the option manager to set these options. The
   * option manager should throw errors if the options do not satisfy the option
   * manager's conditions.
   *
   * \param[in] opts Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _opts) override;

  /**
   * Create a brute force nearest neighbors model.
   *
   * \param[in] X sframe input feature data
   * \param[in] ref_labels row labels for the reference dataset
   * \param[in] composite_distance
   * \param[in] opts
   */
  void train(const sframe& X, const std::vector<flexible_type>& ref_labels,
             const std::vector<dist_component_type>& composite_distance_params,
             const std::map<std::string, flexible_type>& opts) override;

  /**
   * Find neighbors of queries in a created brute_force model. Depending on data
   * attributes, calls either blockwise query or pairwise query.
   *
   * \param[in] mld_queries query data
   * \param[in] query_labels vector query labels
   * \param[in] k size_t max number of neighbors to return for each query
   * \param[in] radius flexible_type max distance for returned neighbors to each query
   * \param[in] include_self_edges if false, don't include results where the
   * query index and the reference index are the same.
   *
   * \returns ret SFrame with four columns: query label, reference
   * label, distance, and rank.
   *
   */
  sframe query(const v2::ml_data& mld_queries,
               const std::vector<flexible_type>& query_labels,
               const size_t k, const double radius,
               const bool include_self_edges) const override;

  /**
   * Search a nearest neighbors reference object for the neighbors of every
   * point.
   *
   * \param[in] k number of neighbors to return for each query
   * \param[in] radius distance threshold to call a reference point a neighbor
   * \param[in] include_self_edges if false, don't include results where the
   * query index and the reference index are the same.
   *
   * \returns ret   Shared pointer to an SFrame containing query results.
   */
  sframe similarity_graph(const size_t k, const double radius,
                          const bool include_self_edges) const override;

  /**
   * Find neighbors of queries in a created brute_force model. Break the
   * reference and query data into blocks small enough to be read into memory,
   * then use matrix multiplication to compute distances in bulk. Only
   * appropriate for dense, numeric data with standard distance functions.
   *
   * \param mld_queries v2::ml_data query data
   * \param neighbors std::vector<neighbor_candidates> container for results
   * \param dist_name std::string name of the distance function.
   */
  void blockwise_query(const v2::ml_data& mld_queries,
                       std::vector<neighbor_candidates>& neighbors,
                       const std::string& dist_name) const;

  /**
   * Find neighbors of queries in a created brute_force model, by explicitly
   * computing the distance function for each pair of query and reference
   * points. This is the default strategy because it works with any distance
   * function (including composite distances).
   *
   * Pseudo code
   * ++++++++++++++++++++++++++++++++++++++++++++++++++
   *  for query_block in query_data {
   *    load query_block in memory
   *    parallel_for ref_row in ref_data {
   *      for query_row in query_block {
   *        evaluate_point(query_row, ref_row, row_id)
   *      }
   *    }
   *  }
   *
   * \param mld_queries v2::ml_data query data
   * \param neighbors std::vector<neighbor_candidates> container for results
   */
  void pairwise_query(const v2::ml_data& mld_queries,
                      std::vector<neighbor_candidates>& neighbors) const;

/**
 * Construct the similarity graph for the reference data, using blockwise matrix
 * multiplication for distance computations.
 *
 * \param neighbors std::vector<neighbor_candidates> container for results
 * \param dist_name std::string name of the distance function.
 */
  void blockwise_similarity_graph(std::vector<neighbor_candidates>& neighbors,
                                  const std::string& dist_name) const;


  inline size_t get_version() const override {
    return BRUTE_FORCE_NEIGHBORS_VERSION;
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
  BEGIN_CLASS_MEMBER_REGISTRATION("nearest_neighbors_brute_force")
  REGISTER_CLASS_MEMBER_FUNCTION(brute_force_neighbors::list_fields)
  END_CLASS_MEMBER_REGISTRATION

};


}  // namespace nearest_neighbors
}  // namespace turi

#endif
