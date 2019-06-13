/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_NEAREST_NEIGHBORS_H_
#define TURI_NEAREST_NEIGHBORS_H_

// Types
#include <core/storage/sframe_data/sframe.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

// Data structure utils
#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/row_slicing_utilities.hpp>

// Interfaces
#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/unity_base_types.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <toolkits/util/algorithmic_utils.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>

#include <toolkits/nearest_neighbors/distance_functions.hpp>

#include <core/export.hpp>
#include <Eigen/Core>
#include <Eigen/SparseCore>

namespace turi {
namespace nearest_neighbors {

typedef std::tuple<std::vector<std::string>, function_closure_info, double> dist_component_type;

}
}

BEGIN_OUT_OF_PLACE_SAVE(arc, turi::nearest_neighbors::dist_component_type, d) {
  std::map<std::string, turi::variant_type> data;
  data["column_names"] = turi::to_variant(std::get<0>(d));
  data["weight"] = turi::to_variant(std::get<2>(d));
  variant_deep_save(data, arc);
  arc << std::get<1>(d);
} END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(arc, turi::nearest_neighbors::dist_component_type, d) {
  std::map<std::string, turi::variant_type> data;
  variant_deep_load(data, arc);
  std::vector<std::string> column_names;
  function_closure_info distance_info;
  double weight;
#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));
  __EXTRACT(column_names);
  __EXTRACT(weight);
#undef __EXTRACT
  arc >> distance_info;
  d = std::make_tuple(column_names, distance_info, weight);

} END_OUT_OF_PLACE_LOAD()

namespace turi {
namespace nearest_neighbors {

static constexpr size_t NONE_FLAG = (size_t) -1;

enum class row_type {dense, sparse, flex_type};

struct dist_component {
  std::vector<std::string> column_names;
  std::shared_ptr<distance_metric> distance;
  double weight;
  v2::row_slicer slicer;
  row_type row_sparsity;
};

class neighbor_candidates;


// -----------------------------------------------------------------------------
// NEAREST NEIGHBORS HELPER FUNCTIONS
// -----------------------------------------------------------------------------


/**
 * Convert the index of a flat array into row and column indices for an upper
 * triangular matrix. The general idea for the algorithm is from this
 * StackOverflow thread:
 * http://stackoverflow.com/questions/242711/algorithm-for-index-numbers-of-triangular-matrix-coefficients
 *
 * \param i size_t Index in the flat array.
 * \param n size_t Number of rows and columns in the upper triangular matrix.
 *
 * \return std::pair<size_t, size_t> Row and column index in the upper
 * triangular matrix.
 */
 std::pair<size_t, size_t> upper_triangular_indices(const size_t i,
                                                    const size_t n);

/**
 * Extract a distance function's name.
 */
std::string extract_distance_function_name(
  const function_closure_info distance_fn);

/**
 * Figure out how many memory blocks to break the reference and query datasets
 * into, based on the number of data points and the maximum number of points in
 * a memory block.
 *
 * Assume that each block has the same number of query and reference rows (r).
 * Each thread loads into memory a reference block with 8 * dimension * r bytes
 * and a distance matrix of 8 * r^2 bytes. This function simply uses to
 * quadratic formula to figure out the upper bound on r.
 *
 * One copy of each query block is also loaded into memory sequentially, but
 * this is ignored.
 *
 * \param num_ref_examples size_t Number of total reference data points.
 * \param num_query_examples size_t Number of total reference query points.
 * \param dimension size_t Number of (unpacked) features in each data point.
 * \param max_thread_memory Max memory to use in each thread (in bytes).
 * \param min_ref_blocks Lower bound on the number of reference blocks to use.
 * \param min_query_blocks Lower bound on the number of reference blocks to use.
 *
 * \return num_blocks A pair of block sizes, (num_ref_blocks, num_query_blocks)
 */
std::pair<size_t, size_t> calculate_num_blocks(const size_t num_ref_examples,
                                               const size_t num_query_examples,
                                               const size_t dimension,
                                               const size_t max_thread_memory,
                                               const size_t min_ref_blocks,
                                               const size_t min_query_blocks);

/**
 * Read data from an ml_data object into a dense matrix, in parallel.
 */
void parallel_read_data_into_matrix(const v2::ml_data& dataset, DenseMatrix& A,
                                    const size_t block_start,
                                    const size_t block_end);

/**
 * Read data from an ml_data object into a dense matrix, single threaded.
 */
void read_data_into_matrix(const v2::ml_data& dataset, DenseMatrix& A,
                           const size_t block_start, const size_t block_end);

/**
 * Find the query nearest neighbors for a block of queries and a block of
 * reference data.
 *
 * \param R DenseMatrix reference data. Each row is a reference example.
 * \param Q DenseMatrix query data. Each row is a query example.
 * \param neighbors std::vector<neighbor_candidates> output container for the
 * results.
 * \param ref_offset size_t The index value where the reference data starts,
 * assuming the reference data is one block from a larger set.
 * \param query_offset size_t The index value where the query data starts,
 * assuming the query data is one block from a larger set.
 */
void find_block_neighbors(const DenseMatrix& R, const DenseMatrix& Q,
                          std::vector<neighbor_candidates>& neighbors,
                          const std::string& dist_name,
                          const size_t ref_offset, const size_t query_offset);

/**
 * Find the nearest neighbors for each point in a block of reference data.
 * Update the nearest neighbor heaps for both the rows and columns in the
 * resulting distance matrix (unlike the blockwise query, which only worries
 * about the rows).
 *
 * \param R DenseMatrix Block of reference data corresponding to distance matrix
 * rows.
 * \param C DenseMatrix Block of reference data corresponding to distance matrix
 * columns.
 * \param neighbors std::vector<neighbor_candidates> output container for the
 * results.
 * \param row_offset size_t The index value where the row data starts,
 * assuming the data is one block from a larger set.
 * \param col_offset size_t  The index value where the column data starts,
 * assuming the data is one block from a larger set.
 */
void off_diag_block_similarity_graph(const DenseMatrix& R, const DenseMatrix& C,
                                     std::vector<neighbor_candidates>& neighbors,
                                     const std::string& dist_name,
                                     const size_t row_offset,
                                     const size_t col_offset);

/**
 * Write nearest neighbors results stored in a vector of heaps to a stacked
 * SFrame.
 */
sframe write_neighbors_to_sframe(
    std::vector<nearest_neighbors::neighbor_candidates>& neighbors,
    const std::vector<flexible_type>& reference_labels,
    const std::vector<flexible_type>& query_labels);

/**
 * Append nearest neighbors results stored in a vector of heaps to a sframe.
 */
void append_neighbors_to_sframe(
    sframe& result,
    std::vector<nearest_neighbors::neighbor_candidates>& neighbors,
    const std::vector<flexible_type>& reference_labels,
    const std::vector<flexible_type>& query_labels);



// -----------------------------------------------------------------------------
// NEAREST NEIGHBORS MODEL CLASS
// -----------------------------------------------------------------------------

/**
 * Nearest neighbors model base class
 * -----------------------------------------------------------------------------
 *
 * Base class for computing k-nearest neighbors queries, inherited by both the
 * ball tree and LSH structure. Each nearest neighbors model contains the
 * following;
 *
 * - metadata:
 *     A globally consistent object with column wise metadata. This metadata
 *     changes with time (even after training). If you want to freeze the
 *     metadata after training, you have to do so yourself.
 *
 * - label_metadata:
 *     A globally consistent object with target metadata. For nearest neighbors
 *     this is merely used to index reference and query rows; it is not
 *     predicted as in supervised learning.
 *
 * - num_examples:
 *     Number of rows in the reference data.
 *
 * - num_features:
 *     Number of features in the reference and query  data. This counts dense
 *     and sparse vectors as single features.
 *
 * - num_variables:
 *     Number of variables in the reference and query data. Dense and sparse
 *     vectors are counted according to their lengths.
 *
 *
 * The following functions should always be implemented in a
 * nearest_neighbors_model.
 *
 * - supervised_learning_clone:
 *     Clone objects to this base class type.
 *
 * - name:
 *     Get the name of this model. The unity_server can construct
 *     model_base objects and they can be cast to a model of this type. The name
 *     determine how the casting happens. The init_models() function in
 *     unity_server.cpp will give you an idea of how this interface happens.
 *
 * - train:
 *     A train function for the model. The result of this function should be a
 *     model object that is trained and has state updated so that a
 *     caller can use get_training_stats() to get  back the stats that were
 *     collected during training.
 *
 * - predict:
 *     A predict function for the model for batch predictions. The result of
 *     this function can be an SArray of predictions. One for each value of the
 *     input SFrame.
 *
 * - evaluate:
 *     An evaluattion function for the model for evaluations. The result of this
 *     function must be an updated evaluation_stats map which can be queried
 *     with the get_evaluation_stats().
 *
 * - save:
 *     Save the model with the turicreate iarc. Turi is a server-client
 *     module. DO NOT SAVE ANYTHING in the client side. Make sure that
 *     everything is in the server side. For example: You might be tempted do
 *     keep options that the user provides into the server side but DO NOT do
 *     that because save and load will break things for you!
 *
 * - load:
 *     Load the model with the turicreate oarc.
 *
 * - init_options:
 *     Initialize the options with the option manager.
 *
*/
class EXPORT nearest_neighbors_model : public ml_model_base {

 protected:

  std::map<std::string, flexible_type> train_stats;
  std::shared_ptr<v2::ml_metadata> metadata;
  v2::ml_data mld_ref;
  bool is_dense;
  size_t num_examples = 0;               // Number of records in the reference set
  std::vector<dist_component> composite_distances = {};
  std::vector<dist_component_type> composite_params = {};
  std::map<std::string, v2::ml_column_mode> untranslated_cols;  // Map of columns that should not be translated by ml_data
  std::vector<flexible_type> reference_labels;

  /**
   * Methods that may be overriden in derived classes.
   * ---------------------------------------------------------------------------
   */
 public:

  nearest_neighbors_model();

  virtual ~nearest_neighbors_model(){}

  /**
   * Create a nearest neighbors reference object without input reference labels.
   */
  virtual void train(const sframe& X,
                     const std::vector<dist_component_type>& composite_distance_params,
                     const std::map<std::string, flexible_type>& opts);

  /**
   * Create a nearest neighbors reference object.
   */
  virtual void train(const sframe& X, const sframe& ref_labels,
                     const std::vector<dist_component_type>& composite_distance_params,
                     const std::map<std::string, flexible_type>& opts);

  /**
   * Create a nearest neighbors reference object.
   */
  virtual void train(const sframe& X, const std::vector<flexible_type>& ref_labels,
                     const std::vector<dist_component_type>& composite_distance_params,
                     const std::map<std::string, flexible_type>& opts) = 0;

  /**
   * Search a nearest neighbors reference object for neighbors to a set of query
   * points, without input query row labels.
   *
   * \param[in] X query data (features only)
   * \param[in] k number of neighbors to return for each query
   * \param[in] radius distance threshold to call a reference point a neighbor
   *
   * \returns ret   Shared pointer to an SFrame containing query results.
   *
   * \note Already assumes that data is of the right shape.
   */
  virtual sframe query(const sframe& X, const size_t k,
                       const double radius) const;

  /**
   * Search a nearest neighbors reference object for neighbors to a set of query
   * points.
   *
   * \param[in] X query data (features only)
   * \param[in] query labels row labels for the query dataset
   * \param[in] k number of neighbors to return for each query
   * \param[in] radius distance threshold to call a reference point a neighbor
   *
   * \returns ret   Shared pointer to an SFrame containing query results.
   *
   * \note Already assumes that data is of the right shape.
   */
  virtual sframe query(const sframe& X, const sframe& query_labels,
                       const size_t k, const double radius) const;

  /**
   * Search a nearest neighbors reference object for neighbors to a set of query
   * points.
   *
   * \param[in] X query data (features only)
   * \param[in] query labels row labels for the query dataset
   * \param[in] k number of neighbors to return for each query
   * \param[in] radius distance threshold to call a reference point a neighbor
   *
   * \returns ret   Shared pointer to an SFrame containing query results.
   *
   * \note Already assumes that data is of the right shape.
   */
  virtual sframe query(const sframe& X,
                       const std::vector<flexible_type>& query_labels,
                       const size_t k, const double radius) const;

  /**
   * Search a nearest neighbors reference object for neighbors to a set of query
   * points (in ml_data format).
   *
   * \param[in] mld_queries query data in ml_data format
   * \param[in] query labels row labels for the query dataset
   * \param[in] k number of neighbors to return for each query
   * \param[in] radius distance threshold to call a reference point a neighbor
   * \param[in] include_self_edges if false, don't include results where the
   * query index and the reference index are the same.
   *
   * \returns ret   Shared pointer to an SFrame containing query results.
   *
   * \note Already assumes that data is of the right shape.
   */
  virtual sframe query(const v2::ml_data& mld_queries,
                       const std::vector<flexible_type>& query_labels,
                       const size_t k, const double radius,
                       const bool include_self_edges) const = 0;


  /**
   * Search a nearest neighbors reference object for the neighbors of every
   * point.
   *
   * \param[in] k number of neighbors to return for each query.
   * \param[in] radius distance threshold to call a reference point a neighbor.
   * \param[in] include_self_edges if false, don't include results where the
   * query index and the reference index are the same.
   *
   * \returns ret   Shared pointer to an SFrame containing query results.
   */
  virtual sframe similarity_graph(const size_t k, const double radius,
                                  const bool include_self_edges) const;

  /**
   * Set the model options. Use the option manager to set these options. The
   * option manager should throw errors if the options do not satisfy the option
   * manager's conditions.
   *
   * \param[in] opts Options to set
   */
  virtual void init_options(const std::map<std::string,flexible_type>& _opts) = 0;


  /**
   * Gets the model version number
   */
  virtual size_t get_version() const = 0;

  /**
   * Serialize the model object.
   */
  virtual void save_impl(turi::oarchive& oarc) const = 0;

  /**
   * Load the model object.
   */
  virtual void load_version(turi::iarchive& iarc, size_t version) = 0;



  /**
   * Methods that should not be overriden in derived classes.
   * -------------------------------------------------------------------------
   */
 public:

  /**
   * Get training stats.
   *
   * \returns The training stat map.
   */
  std::map<std::string, flexible_type> get_training_stats() const;

  /**
   * Get names of predictor variables.
   *
   * \returns Names of predictors (Vector of string names).
   */
  std::vector<std::string> get_feature_names() const;

  /**
   * Get metadata object.
   *
   * \returns Metadata object.
   */
  std::shared_ptr<v2::ml_metadata> get_metadata() const;

  /**
   * Check the query schema against the create schema.
   * \param[in] X SFrame
   *
   */
  void check_schema_for_query(const sframe& X) const;

  /**
   * Check if input data is empty.
   * \param[in] X SFrame
   */
  void check_empty_data(const sframe& X) const;

  /*
   * Check for missing values in the untranslated columns, aka string features.
   * Assumes the training data is already set in the model, as 'mld_ref'.
   * \param[in] X SFrame
   */
  void check_missing_strings(const sframe& X) const;

  /**
   * Initialize the reference ml_data object in the model, and set metadata in
   * the model's state for visibility to Python.
   */
  void initialize_model_data(const sframe& X,
                             const std::vector<flexible_type>& ref_labels);

  /**
   * Initialize each distance function in the set of distance
   * components.
   */
  void initialize_distances();

  /**
   *
   * Validates feature types for each distance function in the set of distance
   * components.

   * \param[in] composite_params std::vector<dist_component_type> Specifications
   * for each component of a composite distance.
   *
   * \param[in] X SFrame All input features, i.e. the union over features
   * specified in composite params.
   *
   * \param[in] y SFrame Label data.
   */
  void validate_distance_components(const std::vector<dist_component_type>& composite_params,
                                    const sframe& X);

  /**
   * Check that the feature types are valid for a particular distance component.
   * Return the row data type.
   *
   * \param[in] column_names std::vector<std::string>
   * \param[in] X SFrame
   * \param[in] distance_name std::string
   * \param[in] weight double
   *
   */
  void validate_distance_component(const std::vector<std::string> column_names,
                                   const sframe& X,
                                   const function_closure_info distance_name,
                                   const double weight);

  /**
   * Get reference data as a vector of vectors
   * \returns Reference data as a vector of vectors (in ml-data form)
   */
  flexible_type get_reference_data() const;

};

// -----------------------------------------------------------------------------
// CANDIDATE NEIGHBORS CLASS
// -----------------------------------------------------------------------------
/**
 * Class that holds nearest neighbors candidates
 * -----------------------------------------------------------------------------
 * Users may specify a maximum number of neighbors to return (i.e. k) or a
 * maximum radius within which all neighbors should be returned (i.e. radius),
 * or neigther, or both. Each of these four cases has slightly different
 * behavior, which this class encapsulates to make the nearest neighbor models
 * and methods easier to write and use.
 *
 * The model contains the following attributes:
 * - k:
 *      Maximum number of neighbors to return.
 *
 * - radius:
 *      Max distance for a query point to be considered a neighbor of the
 *      reference point.
 *
 * - include_self_edges:
 *      If set to 'false', neighbors with the same index as the object's label
 *      are excluded from the results.
 *
 * - candidates:
 *      Data structure that holds candidate neighbors. The baseline structure is
 *      a vector of pairs. Each pair contains a double distance to the query
 *      point, and an int index of the candidate neighbor. If 'k' is specified,
 *      a heap is constructed on top of this vector.
 *
 * The model contains the following methods:
 * - evaluate_point:
 *      Evaluate a new point as a neighbor candidate. Each of the four settings
 *      for 'k' and 'radius' yield different decisions on when to add a point as
 *      a candidate. If 'k' is specified and the heap is full, this also pops
 *      off the furthest point in the candidates vector.
 *
 * - print_candidates:
 *      Print all of the candidates with logprogress_stream.
 *
 * - sort_candidates:
 *      Sort the candidates, from smallest to largest distances (the first
 *      element of each pair in the candidates vector/heap).
 *
 * - get_max_dist:
 *      Return the maximum distance in the current set of candidates.
 */
class neighbor_candidates {

 protected:

  size_t label = (size_t) -1;
  bool include_self_edges = true;
  size_t k = (size_t) -1;
  double radius = -1.0;
  simple_spinlock heap_lock;

 public:

  // each candidate is both an index and distance
  std::vector<std::pair<double, size_t>> candidates;

  neighbor_candidates(size_t lbl, size_t a, double b, bool c);

  ~neighbor_candidates();

  /**
   * Set the label.
   */
  void set_label(size_t label);

  /**
   * Get the label
   */
  size_t get_label() const;

  /**
   * Accessor for the max number of neighbors (i.e. k).
   */
  size_t get_max_neighbors() const;

  /**
  * Accessor for the radius.
  */
  double get_radius() const;

  /**
   * Evaluate a specified reference point as a nearest neighbor candidate.
   *
   * \param[in] point std::pair<double, int> Reference point to consider.
   * Consists of the distance to the query point and the index of the reference
   * point.
   */
  void evaluate_point(const std::pair<double, size_t>& point) GL_HOT_FLATTEN;

  /**
   * Print all of the current candidates.
   */
  void print_candidates() const;

  /**
   * Sort candidates.
   */
  void sort_candidates();

  /**
   * Return the max distance of the current candidates. Note: returns -1.0 if the
   * candidates vector/heap is empty.
   */
  double get_max_dist() const;
};

/**
 * Function to get the reference data from the NN model
 *
 * \param[in] model Nearest neighbour model.
 */
flexible_type _nn_get_reference_data(std::shared_ptr<nearest_neighbors_model> model);

}  // namespace nearest_neighbors
}  // namespace turi

#endif
