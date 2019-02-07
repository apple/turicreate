/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_KMEANS
#define TURI_KMEANS

// Types
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <sframe/sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <parallel/atomic.hpp>
#include <parallel/pthread_tools.hpp>
#include <parallel/lambda_omp.hpp>
#include <flexible_type/flexible_type.hpp>
#include <generics/symmetric_2d_array.hpp>
#include <globals/globals.hpp>

// ML Data utils
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>

// Interfaces
#include <unity/lib/extensions/ml_model.hpp>
#include <unity/lib/extensions/option_manager.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
#include <globals/globals.hpp>
#include <unity/toolkits/supervised_learning/supervised_learning_utils-inl.hpp>

// Miscellaneous
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <table_printer/table_printer.hpp>
#include <export.hpp>


namespace turi {
namespace kmeans {


typedef Eigen::Matrix<double, Eigen::Dynamic, 1>  dense_vector;
typedef Eigen::SparseVector<double>  sparse_vector;


/**
 * ----------------
 * Helper functions
 * ----------------
 */

/**
 * Make sure the dataset is not empty.
 *
 * \param X Input dataset.
 */
void check_empty_data(const sframe& X);

/**
 * Check that the feature types are valid for the kmeans model.
 *
 * \param X Input dataset.
 */
void check_column_types(const sframe& X);



/**
 * ----------------------
 * Definition of clusters
 * ----------------------
 */
struct cluster {

  // Data
  dense_vector center;
  atomic<size_t> count = 0;
  turi::mutex m;


  // Methods
  cluster(size_t dimension): center(dense_vector(dimension)), count(0) {
    center.setZero();
  };

  cluster() = delete;
  cluster(const cluster& other): center(other.center), count(other.count) {};
  cluster& operator=(const cluster& other);

  /**
   * Safe mean update that avoids overflow. See
   *  http://www.johndcook.com/standard_deviation.html
   */
  void safe_update_center(const dense_vector& u);
};


/**
 * ------------------------
 * Kmeans clustering model
 * ------------------------
 *
 * Kmeans clustering model. By default, the model uses the KMeans++ algorithm to
 * choose initial cluster centers, although users may also pass custom initial
 * centers. This implementation uses the implementation of Elkan (2003), which
 * takes advantage of the triangle inequality to reduce the number of distance
 * computations. In addition to storing the n x 1 vectors of cluster assignments
 * and distances from each point to its assigned cluster center (necessary for
 * any Kmeans implementation), the Elkan algorithm also requires computation and
 * storage of all pairwise distances between cluster centers.
 *
 * \note This implementation does *not* currently use the second lemma from the
 * Elkan (2003) paper, which further reduces the number of exact distance
 * computations by storing the lower bound on the distance between every point
 * and every cluster center. This n x K matrix is generally too big to store in
 * memory and too slow to write to as an SFrame.
 *
 * The Kmeans model contains the following private data objects:
 *
 *  mldata: The data, in ml_data_2 form. Each row is an observation. After the
 *    model is trained, this member is no longer needed, so it is not serialized
 *    when the model is saved.
 *
 *  num_examples: Number of points.
 *
 *  assignments: Cluster assignment for each data point.
 *
 *  clusters: Vector of cluster structs, each of which contains a center vector,
 *    a count of assigned points, and a mutex lock, so the cluster can safely be
 *    updated in parallel.
 *
 *  num_clusters: Number of clusters, set by the user.
 *
 *  max_iterations: Maximum iterations of the main Kmeans loop, excluding
 *    initial selection of the centers.
 *
 *  upper_bounds: For every point, an upper bound on the distance from the point
 *    to its currently assigned center. Whenever the distance between a point
 *    and its assigned center is computed exactly, then this bound is tight, but
 *    the bounds also must be adjusted at the end of each iteration to account
 *    for the movement of the assigned center. Despite this adjustment, the
 *    upper bound often remains small enough to avoid computing the exact
 *    distance to other candidate centers.
 *
 *  center_dists: Exact distance between all pairs of cluster centers. Computing
 *    this K x K matrix allows us to use the triangle inequality to avoid
 *    computing all n x K point-to-center distances in every iteration.
 *
 *
 * The Kmeans model stores the following members in its *state* object, which is
 * exposed to the Python API:
 *
 *  options: Option manager which keeps track of default options, current
 *    options, option ranges, type etc. This must be initialized only once in
 *    the init_options() function.
 *
 *  training_time: Run time of the algorithm, in seconds.
 *
 *  training_iterations: Number of iterations of the main Kmeans loop. If the
 *    algorithm does not converge, this is equal to 'max_iterations'.
 *
 *
 * In addition, the following public methods return information from a trained
 * Kmeans model:
 *
 *  get_cluster_assignments: Returns an SFrame with the fields "row_id",
 *    "cluster_id", and "distance" for input data point. The "cluster_id" field
 *    (integer) contains the cluster assignment of the data point, and the
 *    "distance" field (float) contains the Euclidean distance of the data point
 *    to the assigned cluster's center.
 *
 *  get_cluster_info: Returns an SFrame with metadata about each cluster. For
 *    each cluster, the output SFrame includes the features describing the
 *    center, the number of points assigned to the cluster, and the
 *    within-cluster sum of squared Euclidean distances from points to the
 *    center.
 */
class EXPORT kmeans_model : public ml_model_base {

  // Data objects and attributes
  v2::ml_data mldata;
  std::shared_ptr<v2::ml_metadata> metadata;
  size_t num_examples = 0;

  // Model items
  std::vector<size_t> assignments;
  std::vector<cluster> clusters;
  size_t num_clusters = 0;
  size_t max_iterations = 0;
  size_t batch_size = 1;
  std::vector<flexible_type> row_labels;
  std::string row_label_name;

  // Training objects
  std::vector<float> upper_bounds;
  turi::symmetric_2d_array<float> center_dists;

  /**
   * Initialize the model's members.
   *
   * \param X Input dataset.
   * \param row_labels Row labels, either flex_type string or flex_type int.
   */
  void initialize_model_data(const sframe& X,
                             const std::vector<flexible_type>& row_labels,
                             const std::string row_label_name);

  /**
   * Initialize the point assignments and the bounds on distances between points
   * and cluster centers. Uses the triangle inequality and pairwise cluster
   * center distances to eliminate unnecessary distance computations.
   */
  void assign_initial_clusters_elkan();

  /**
   * Choose random initial cluster centers, with a modified version of the
   * k-means++ method. For the first cluster center, sample uniformly at random
   * from data set. For the remaining clusters, sample proportional to the
   * distance between each point the distance to its nearest existing center.
   * For this toolkit we actually read a sample of the data into memory first,
   * for much faster random access of the chosen centers.
   */
  void choose_random_centers();

  /**
   * High-memory version of main Kmeans iterations, using Elkan's algorithm.
   *
   * \returns iter Number of iterations in the main training loop.
   *
   * \note: Uses only lemma 1 from Elkan (2003), which uses the triangle
   * inequality and distances between cluster centers to avoid distance
   * computations. Does *not* use the lower bounds between all points and all
   * distances.
   */
  size_t compute_clusters_elkan();

  /**
   * Minibatch version of main Kmeans iterations, using the D. Sculley
   * algorithm (http://www.eecs.tufts.edu/~dsculley/papers/fastkmeans.pdf).
   *
   * \returns iter Number of iterations in the main training loop.
   */
  size_t compute_clusters_minibatch();

  /**
   * Low-memory version of main Kmeans iterations, using Lloyd's algorithm.
   *
   * \returns iter Number of iterations in the main training loop.
   */
  size_t compute_clusters_lloyd();

  /**
   * Set custom initial centers in the model.
   *
   * \param init_centers Custom initial centers, in an SFrame with the same
   * metadata as the input dataset.
   */
  void process_custom_centers(const sframe& init_centers);

  /**
   * Compute distances between all pairs of cluster centers. The result is
   * stored in the model's 'center_dists' member.
   */
  void compute_center_distances();

  /**
   * Update cluster centers to be the means of the currently assigned points.
   */
  void update_cluster_centers();

  /**
   * Update distance bounds based on the distance between each cluster center's
   * previous and current locations.
   *
   * \param previous_clusters Previous iteration's clusters.
   */
  void adjust_distance_bounds(const std::vector<cluster>& previous_clusters);

  /**
   * Compute the exact distance between each point and its assigned cluster.
   * Store in the 'upper_bounds' member, which for low-memory clustering is the
   * exact distance anyway.
   */
  void set_exact_point_distances();

  /**
   * Update the cluster assignments based on the current cluster means and
   * return the number of assignments that changed. Uses cluster center
   * distances to eliminate many exact point-to-center distance computations.
   *
   * \return num_chaged Number of points whose cluster assignment changed.
   */
  size_t update_assignments_elkan();

  /**
   * Update cluster assignments based on current cluster means and return the
   * number of assignments that changed. Computes all point-to-center distances.
   *
   * \return num_chaged Number of points whose cluster assignment changed.
   *
   * \note: 'upper_bounds' for the low-memory version (i.e. Lloyd's algorithm) are
   * actually exact distances between each point and its assigned cluster's
   * center.
   */
  size_t update_assignments_lloyd();


public:
  static constexpr size_t KMEANS_VERSION = 4;

  /**
   * Constructor
   */
  kmeans_model();

  /**
   * Destructor.
   */
   ~kmeans_model();

  /**
   * Set the model options. The option manager should throw errors if the
   * options do not satisfy the option manager's conditions.
   */
  void init_options(const std::map<std::string, flexible_type>& _opts) override;

  /**
   * Train the kmeans model, without row labels.

   * \param X Input data. Each row is an observation.
   * \param init_centers Custom initial centers provided by the user.
   * \param method Indicate if Lloyd's algorithm should be used
   * instead of Elkan's. Lloyd's is generally substantially slower, but uses
   * very little memory, while Elkan's requires storage of all pairwise cluster
   * center distances.
   */
  void train(const sframe& X, const sframe& init_centers, std::string method,
             bool allow_categorical = false);

  /**
   * Train the kmeans model, with row labels.

   * \param X Input data. Each row is an observation.
   * \param init_centers Custom initial centers provided by the user.
   * \param use_naive_method Indicate if Lloyd's algorithm should be used
   * instead of Elkan's. Lloyd's is generally substantially slower, but uses
   * very little memory, while Elkan's requires storage of all pairwise cluster
   * center distances.
   * \param row_labels Flexible type row labels.
   * \param row_label_name Name of the row label column.
   */
  void train(const sframe& X,
             const sframe& init_centers,
             std::string method,
            const std::vector<flexible_type>& row_labels,
            const std::string row_label_name,
             bool allow_categorical = false);

  /**
   * Predict the cluster assignment for new data, according to a trained Kmeans
   * model.
   *
   * \param X Input dataset.
   * \return out SFrame with row index, cluster assignment and distance to
   * assigned cluster center (similar to `get_cluster_assignments`).
   */
  sframe predict(const sframe& X);

  /**
   * Write cluster assigments to an SFrame and return. Also records the row
   * index of the point and the distance from the point to its assigned cluster.
   *
   * \returns out SFrame with row index, cluster assignment, and distance to
   * assigned cluster center for each input data point. cluster's center.
   */
  sframe get_cluster_assignments();

  /**
   * Write cluster metadata to an SFrame and return. Records the features for
   * each cluster, the count of assigned points, and the within-cluster sum of
   * squared distances.
   *
   * \returns out SFrame with metadata about each cluster, including the center
   * vector, count of assigned points, and within-cluster sum of squared
   * Euclidean distances.
   */
  sframe get_cluster_info();

  /**
   * Get the model version number.
   *
   * Version map
   * ===========
   * GLC version -> Kmeans version
   * -----------    --------------
   * <= 1.3         1
   * 1.4            2
   * 1.5            3
   * 1.9            4
   */
  inline size_t get_version() const override { return KMEANS_VERSION; }

  /**
   * Serialize the model.
   */
  void save_impl(turi::oarchive& oarc) const override;

  /**
   * De-serialize the model.
   */
  void load_version(turi::iarchive& iarc, size_t version) override;

  // TODO: convert interface above to use the extensions methods here
  BEGIN_CLASS_MEMBER_REGISTRATION("kmeans")
  REGISTER_CLASS_MEMBER_FUNCTION(kmeans_model::list_fields)
  END_CLASS_MEMBER_REGISTRATION

};  // kmeans_model class



} // namespace kmeans
} // namespace turi
#endif
