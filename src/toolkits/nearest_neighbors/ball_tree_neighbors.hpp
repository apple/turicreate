/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_BALL_TREE_NEIGHBORS_H_
#define TURI_BALL_TREE_NEIGHBORS_H_

// Toolkits
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>

// Miscellaneous
#include <Eigen/SparseCore>


namespace turi {
namespace nearest_neighbors {


/**
 * Ball tree nearest neighbors class
 * -----------------------------------------------------------------------------
 *
 * Implements the ball tree method for k-nearest neighbors search.
 *
 * The ball tree works by partitioning the reference data into into successively
 * smaller balls, and recording the center (i.e. pivot) and radius of each ball.
 * A ball tree query uses the pivots and radii to exclude many of the balls from
 * the k-nearest neighbor search, allowing it to run in sub-linear time.
 *
 * In addition to the objects contained in the nearest_neighbors_model base
 * class, the ball tree contains the following:
 *
 * - membership:
 *     Each element of this vector indicates which node the corresponding
 *     reference data point belongs to. After the tree is constructed, the
 *     elements in this vector correspond to leaf nodes of the tree only.
 *
 * - pivots:
 *     The reference data point at the center of each tree node.
 *
 * - node_radii:
 *     The distance from the pivot of each node to the most distant
 *     reference point belonging to the tree node.
 */
class EXPORT ball_tree_neighbors: public nearest_neighbors_model {

 protected:

  std::vector<size_t> membership;        // leaf node membership
  std::vector<DenseVector> pivots;       // dense pivot obserations
  std::vector<SparseVector> pivots_sp;   // sparse pivot obserations
  std::vector<double> node_radii;        // node radii

  size_t tree_depth;                      // number of levels in the tree

  /**
   * Decide if a node should be activated for a query. Activating a node means
   * it will be traversed in the search for a query's nearest neighbors. For
   * internal nodes, this means the search will in turn check if each child node
   * should be activated. For leaf nodes, it means the distances between the
   * query and all members of the node will be computed (and potentially added
   * to the set of candidate nearest neighbors).
   *
   * \param[in] k size_t Max number of neighbors
   * \param[in] radius double Max distance for a neighbor
   * \param[in] min_poss_dist double Minimum possible distance from the query
   *  point to the node in question.
   * \param[in] num_current_neighbors size_t Current number of neighbors
   * \param[in] max_current_dist double Max distance to the current neighbors
   * set. Note that if the neighbor candidates set is empty, this will be -1.0.
   *
   * \param[out] activate bool If true, the node should be activated.
   */
  bool activate_query_node(size_t k, double radius, double min_poss_dist,
                           size_t num_current_neighbors,
                           double max_current_dist) const;


 public:

  /**
   * version 3 (GLC 1.6/sprint 1509): Add the original_row_index member, to
   * facilitate the 'include_self_edges' flag.
   */
  static constexpr size_t BALL_TREE_NEIGHBORS_VERSION = 2;

  /**
   * Destructor. Make sure bad things don't happen
   */
  ~ball_tree_neighbors();

  /**
   * Set the model options. Use the option manager to set these options. The
   * option manager should throw errors if the options do not satisfy the option
   * manager's conditions.
   *
   * \param[in] opts Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _opts) override;


  /**
   * Create a ball tree nearest neighbors model.
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
   * Find neighbors of queries in a created ball tree model.
   *
   * For each query, the method keeps track of the current k-nearest neighbors
   * in the ball tree. At each node, the closest possible point in each child
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
    return BALL_TREE_NEIGHBORS_VERSION;
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
  BEGIN_CLASS_MEMBER_REGISTRATION("nearest_neighbors_ball_tree")
  REGISTER_CLASS_MEMBER_FUNCTION(ball_tree_neighbors::list_fields)
  END_CLASS_MEMBER_REGISTRATION

};


}  // namespace nearest_neighbors
}  // namespace turi

#endif
