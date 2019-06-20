/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ALS_H_
#define TURI_ALS_H_

#include <Eigen/Cholesky>
// ML-Data & options manager
#include <toolkits/ml_data_2/ml_data.hpp>
#include <model_server/lib/extensions/option_manager.hpp>
#include <core/logging/table_printer/table_printer.hpp>

// Factorization model impl
#include <toolkits/factorization/factorization_model_impl.hpp>
#include <algorithm>


// TODO: List of todo's for this file
//------------------------------------------------------------------------------

namespace turi {
namespace recsys {
namespace als {

// Typedefs
typedef factorization::factorization_model_impl
   <factorization::model_factor_mode::matrix_factorization, Eigen::Dynamic>
                                                                  model_type;
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic,
                                              Eigen::RowMajor> DenseMatrix;
typedef Eigen::Matrix<float, Eigen::Dynamic, 1> DenseVector;


/**
* Make sure that the two meta-data's have the same mappings.
* --------------------------------------------------------------------------
* user_mapping creates a mapping from the user-id of the second metadata
* to the user-id of the first metadata. Likewise, the item-mapping does
* the same.
*
* This is required because there are 2 ml_data objects needed for this method.
* The first is sorted by user, while the second is sorted by item. However,
* they do not share the same metadata. We need to make sure that the index
* for each user and item are the same when iterating over both the
* ml_datas.
*
* The master metadata is the one saved in ml_data_sorted_by_user, the
* item_mapping and user_mapping map the indices from ml_data_sorted_by_index
* to the ml_data_sorted_by_user
*
*
* \param[in] training_data_by_user  ml-data sorted by user
* \param[in] training_data_by_item  ml-data sorted by item
* \param[out] item_mapping          Index in item metadata -> User index
* \param[out] user_mapping          Index in user metadata -> Item index
*
*/
void get_common_user_item_local_index_mapping(
                                  const v2::ml_data& training_data_by_user,
                                  const v2::ml_data& training_data_by_item,
                                  std::vector<size_t>& user_mapping,
                                  std::vector<size_t>& item_mapping) {

  size_t num_users = training_data_by_user.metadata()->column_size(0);
  size_t num_items = training_data_by_user.metadata()->column_size(1);
  std::shared_ptr<v2::ml_data_internal::column_indexer> user_index_sorted_by_user =
                             training_data_by_user.metadata()->indexer(0);
  std::shared_ptr<v2::ml_data_internal::column_indexer> item_index_sorted_by_user =
                             training_data_by_user.metadata()->indexer(1);
  std::shared_ptr<v2::ml_data_internal::column_indexer> user_index_sorted_by_item =
                             training_data_by_item.metadata()->indexer(1);
  std::shared_ptr<v2::ml_data_internal::column_indexer> item_index_sorted_by_item =
                             training_data_by_item.metadata()->indexer(0);

  user_mapping.resize(num_users);
  item_mapping.resize(num_items);
  for(size_t u = 0; u < num_users; u++){
    user_mapping[u] = user_index_sorted_by_user->immutable_map_value_to_index(
                       user_index_sorted_by_item->map_index_to_value(u));
  }
  for(size_t i = 0; i < num_items; i++){
    item_mapping[i] = item_index_sorted_by_user->immutable_map_value_to_index(
                       item_index_sorted_by_item->map_index_to_value(i));
  }

}

/**
 *
 * Solve a recommender problem with ALS.
 *
 * \param[in] training_data_by_user  ml-data sorted by user
 * \param[in] training_data_by_item  ml-data sorted by item
 * \param[in] options                List of options provided by the user
 *
 *
 * Python Pseudo code for this method is:
 * ---------------------------------------------------------------------------
 * lambda_ = 10
 * n_factors = 8
 * m, n = Q.shape
 * n_iterations = 20
 *
 * Q = rating # For only those users and items that had rating
 * W = Q>0.5
 * W[W == True] = 1
 * W[W == False] = 0
 *
 * X = 5 * np.random.rand(m, n_factors)
 * Y = 5 * np.random.rand(n_factors, n)
 *
 * def get_error(Q, X, Y):
 *     return np.sum((Q - np.dot(X, Y))**2)
 *
 * weighted_errors = []
 * for ii in range(n_iterations):
 *
 *  X = np.linalg.solve(np.dot(Y, Y.T) + lambda_ * np.eye(n_factors),
 *                      np.dot(Y, Q.T)).T
 *  Y = np.linalg.solve(np.dot(X.T, X) + lambda_ * np.eye(n_factors),
 *                      np.dot(X.T, Q))
 *
*/
inline std::shared_ptr<factorization::factorization_model> als(
                const v2::ml_data& training_data_by_user,
                const v2::ml_data& training_data_by_item,
                const std::map<std::string, flexible_type>& options){



  // Setup the model
  std::shared_ptr<model_type> model(new model_type);
  model->setup("squared_error", training_data_by_user, options);

  // Problem definition
  size_t num_users = training_data_by_user.metadata()->column_size(0);
  size_t num_factors = model->num_factors();
  size_t num_ratings = training_data_by_user.num_rows();
  double lambda = std::max<double>(1e-6,
                num_ratings * (double) options.at("regularization"));
  size_t max_iters = (size_t) options.at("max_iterations");
  size_t seed = (size_t) options.at("random_seed");
  double init_rand_sigma = (double) options.at("init_random_sigma");

  // Make sure that the two meta-data's have the same mappings.
  std::vector<size_t> user_mapping;
  std::vector<size_t> item_mapping;
  get_common_user_item_local_index_mapping(training_data_by_user, training_data_by_item,
                               user_mapping, item_mapping);

  // Global variables needed
  DenseMatrix eye = DenseMatrix::Identity(num_factors, num_factors);
  double rmse, best_rmse = 1e20;

  // Setup the table printer
  table_printer table({
      {"Iter.", 7},
      {"Elapsed time", 12},
      {"RMSE", 22}});
  table.print_header();
  table.print_row("Initial", progress_time(), "NA");
  table.print_line_break();

  // Init the model
  model->reset_state(seed, init_rand_sigma);
  model->w.setZero();

  double reset_fraction = 1;
  double reset_fraction_reduction_rate = 1e-2;

  // Each iteration of ALS
  // --------------------------------------------------------------------------
  size_t iter = 0;
  for (iter = 0; iter < max_iters; iter++){

    // Step 1: User step
    // ------------------------------------------------------------------------
    in_parallel([&](size_t thread_idx, size_t num_threads) {
    DenseMatrix A(num_factors, num_factors);
    DenseVector b(num_factors);
    std::vector<v2::ml_data_entry> x;
    size_t user_id, item_id = 0;
    double rating = 0;
    A = lambda * eye;
    b.setZero();
    for(auto it =
        training_data_by_user.get_block_iterator(thread_idx, num_threads);
                                                                !it.done();) {

      it.fill_observation(x);
      user_id = x[0].index;
      item_id = num_users + x[1].index;
      rating = it.target_value() - model->w0;

      A += model->V.row(item_id).transpose() * model->V.row(item_id);
      b += rating * model->V.row(item_id);
      ++it;

      // Solve the system
      if(it.is_start_of_new_block() || it.done()){
        model->V.row(user_id) = (A.ldlt().solve(b)).transpose();
        if (it.done()) break;

        // Reset for the next user
        A = lambda * eye;
        b.setZero();
      }
    }});

    // Step 2: Item Step
    // ------------------------------------------------------------------------
    // Compute Yt C Y using equation (4) of [1]
    in_parallel([&](size_t thread_idx, size_t num_threads) {
    DenseMatrix A(num_factors, num_factors);
    DenseVector b(num_factors);
    std::vector<v2::ml_data_entry> x;
    size_t user_id, item_id = 0;
    double rating = 0.0;
    A = lambda * eye;
    b.setZero();

    for(auto it =
        training_data_by_item.get_block_iterator(thread_idx, num_threads);
                                                                !it.done();) {

      // Update equations
      it.fill_observation(x);
      user_id = user_mapping[x[1].index];
      item_id = num_users + item_mapping[x[0].index];
      rating = it.target_value() - model->w0;

      A += model->V.row(user_id).transpose() * model->V.row(user_id);
      b += rating * model->V.row(user_id);
      ++it;

      // Solve the system
      if(it.is_start_of_new_block() || it.done()){
        model->V.row(item_id) =
          (A.selfadjointView<Eigen::Upper>().ldlt().solve(b)).transpose();
        if (it.done()) break;

        // Reset for the next item
        A = lambda * eye;
        b.setZero();
      }
    }});

    // Step 3: Calculate RMSE
    // ------------------------------------------------------------------------
    if (best_rmse < rmse) best_rmse = rmse;
    rmse = model->loss_model->reported_loss_value(
        model->calculate_loss(training_data_by_user));
    table.print_row(iter, progress_time(), rmse);

    // Step 4: Termination checking
    // ------------------------------------------------------------------------
    // The iteration count is still ticking, so the reseted model does not get
    // the full max_iterations. This ensures that the model does not reset
    // infinitely.
    if (rmse > 10 * best_rmse || std::isnan(rmse) || std::isinf(rmse)) {
      logprogress_stream << "Resetting model." << std::endl;
      reset_fraction *= reset_fraction_reduction_rate;
      model->reset_state(rand(), reset_fraction);
      model->w.setZero();

      continue;
    }
  }

  std::string termination_criterion = "";
  table.print_row("FINAL", progress_time(), rmse);
  table.print_footer();
  if (iter == max_iters){
    termination_criterion = "Optimization Complete: Iteration limit reached.";
  }
  logprogress_stream << termination_criterion << std::endl;

  // Return the training stats
  std::map<std::string, variant_type> training_stats;
  training_stats["training_time"] = progress_time().elapsed_seconds;
  training_stats["num_iterations"]        = to_variant(iter);
  training_stats["final_objective_value"] = to_variant(rmse);
  model->_training_stats = training_stats;
  return model;
}



/**
 *
 * Solve a recommender problem with Implcit ALS [1]
 *
 * \param[in] training_data_by_user  ml-data sorted by user
 * \param[in] training_data_by_item  ml-data sorted by item
 * \param[in] options                List of options provided by the user
 *
 * References:
 *
 * [1] Collaborative Filtering for Implicit Feedback Datasets (Yifan Hu et. al)
 *
 *
 * Python Pseudo code for this method is:
 * ---------------------------------------------------------------------------
 * lambda_ = 10
 * n_factors = 8
 * m, n = Q.shape
 * n_iterations = 20
 *
 * X = 5 * np.random.rand(m, n_factors)
 * Y = 5 * np.random.rand(n_factors, n)
 *
 * def get_error(Q, X, Y, W):
 *     return np.sum((W * (Q - np.dot(X, Y)))**2)
 *
 * weighted_errors = []
 * for ii in range(n_iterations):
 *     for u, Wu in enumerate(W):
 *         X[u] = np.linalg.solve(np.dot(Y, np.dot(np.diag(Wu), Y.T)) +
 *                                lambda_ * np.eye(n_factors),
 *                                np.dot(Y, np.dot(np.diag(Wu), Q[u].T))).T
 *     for i, Wi in enumerate(W.T):
 *         Y[:,i] = np.linalg.solve(np.dot(X.T, np.dot(np.diag(Wi), X)) +
 *                                  lambda_ * np.eye(n_factors),
 *                                  np.dot(X.T, np.dot(np.diag(Wi), Q[:, i])))
 *
*/
inline std::shared_ptr<factorization::factorization_model> implicit_als(
                const v2::ml_data& training_data_by_user,
                const v2::ml_data& training_data_by_item,
                const std::map<std::string, flexible_type>& options){


  // Setup the model
  std::shared_ptr<model_type> model(new model_type);
  model->setup("squared_error", training_data_by_user, options);

  // Problem definition
  size_t num_users = training_data_by_user.metadata()->column_size(0);
  size_t num_items = training_data_by_user.metadata()->column_size(1);
  size_t num_factors = model->num_factors();
  size_t num_ratings = training_data_by_user.num_rows();
  double lambda = std::max<double>(1e-6,
                num_ratings * (double) options.at("regularization"));
  size_t max_iters = (size_t) options.at("max_iterations");
  size_t seed = (size_t) options.at("random_seed");
  double init_rand_sigma = (double) options.at("init_random_sigma");

  // Make sure that the two meta-data's have the same mappings.
  std::vector<size_t> user_mapping;
  std::vector<size_t> item_mapping;
  get_common_user_item_local_index_mapping(training_data_by_user, training_data_by_item,
                               user_mapping, item_mapping);


  // Global variables needed
  DenseMatrix eye = DenseMatrix::Identity(num_factors, num_factors);
  double rmse, best_rmse = 1e20;
  std::vector<double>rmse_per_thread
          (turi::thread_pool::get_instance().size(), 0.0);

  // Setup the table printer
  table_printer table({
      {"Iter.", 7},
      {"Elapsed time", 12},
      {"Estimated Objective Value", 22}});
  table.print_header();
  table.print_row("Initial", progress_time(), "NA");
  table.print_line_break();

  // Init the model
  model->reset_state(seed, init_rand_sigma);
  model->w.setZero();
  model->w0 = 0;

  // Two random constants used in the ials paper [1]
  double alpha = options.at("ials_confidence_scaling_factor");
  size_t is_log_scaling = (options.at("ials_confidence_scaling_type") == "log");
  double eps = 1e-8;
	double reset_fraction = 1;
	double reset_fraction_reduction_rate = 1e-2;

  // Each iteration of ALS
  // --------------------------------------------------------------------------
  size_t iter = 0;
  for (iter = 0; iter < max_iters; iter++){

    // Calcuate the common base matrix to use.
    DenseMatrix A_cached(num_factors, num_factors);
    A_cached.triangularView<Eigen::Upper>() = lambda * eye +
          model->V.bottomRows(num_items).transpose() * model->V.bottomRows(num_items);

    // Step 1: User step
    // ------------------------------------------------------------------------
    in_parallel([&](size_t thread_idx, size_t num_threads) {
    DenseMatrix A(num_factors, num_factors);
    DenseVector b(num_factors);

    std::vector<v2::ml_data_entry> x;
    double scaling = 0.0;
    size_t user_id, item_id = 0;
    A.triangularView<Eigen::Upper>() = A_cached;

    b.setZero();

    for(auto it =
        training_data_by_user.get_block_iterator(thread_idx, num_threads);
                                                                !it.done();) {
      it.fill_observation(x);
      user_id = x[0].index;
      item_id = num_users + x[1].index;
      if (is_log_scaling) {
        scaling = alpha * log(1 + it.target_value() / eps);
      } else {
        scaling = alpha * it.target_value();
      }
      A.triangularView<Eigen::Upper>() += scaling *
                    model->V.row(item_id).transpose() * model->V.row(item_id);
      b += (1 + scaling) * model->V.row(item_id);
      ++it;

      // Update user factor
      if(it.is_start_of_new_block() || it.done()){
        model->V.row(user_id) =
            (A.selfadjointView<Eigen::Upper>().ldlt().solve(b)).transpose();
        if (it.done()) break;

        // Reset for the next user
        A.triangularView<Eigen::Upper>() = A_cached;
        b.setZero();
      }
    }});

    // Step 2: Item Step
    // ------------------------------------------------------------------------
    // Compute Yt C Y using equation (4) of [1]
    in_parallel([&](size_t thread_idx, size_t num_threads) {
    DenseMatrix A(num_factors, num_factors);
    DenseMatrix A_cached(num_factors, num_factors);
    DenseVector b(num_factors);

    std::vector<v2::ml_data_entry> x;
    size_t user_id, item_id = 0;
    double scaling = 0.0;
    A_cached.triangularView<Eigen::Upper>() = lambda * eye +
          model->V.topRows(num_users).transpose() * model->V.topRows(num_users);
    A.triangularView<Eigen::Upper>() = A_cached;
    b.setZero();

    for(auto it =
        training_data_by_item.get_block_iterator(thread_idx, num_threads);
                                                                !it.done();) {
      // Update equations
      it.fill_observation(x);
      user_id = user_mapping[x[1].index];
      item_id = num_users + item_mapping[x[0].index];
      if (is_log_scaling) {
        scaling = alpha * log(1 + std::max(it.target_value(), 0.0) / eps);
      } else {
        scaling = alpha * std::max(it.target_value(), 0.0);
      }
      A.triangularView<Eigen::Upper>() += scaling *
                    model->V.row(user_id).transpose() * model->V.row(user_id);
      b += (1 + scaling) * model->V.row(user_id);
      ++it;

      // Solve the system
      if(it.is_start_of_new_block() || it.done()){
        model->V.row(item_id) =
          (A.selfadjointView<Eigen::Upper>().ldlt().solve(b)).transpose();
        if (it.done()) break;

        // Reset for the next item
        A.triangularView<Eigen::Upper>() = A_cached;
        b.setZero();
      }
    }});

    // Step 3: Calculate RMSE
    // ------------------------------------------------------------------------
    if (best_rmse < rmse) best_rmse = rmse;
    rmse = model->loss_model->reported_loss_value(
        model->calculate_loss(training_data_by_user));
    table.print_row(iter, progress_time(), rmse);

    // Step 4: Termination checking
    // ------------------------------------------------------------------------
    // The iteration count is still ticking, so the reseted model does not get
    // the full max_iterations. This ensures that the model does not reset
    // infinitely.
    if (rmse > 10 * best_rmse || std::isnan(rmse) || std::isinf(rmse)) {
      logprogress_stream << "Resetting model." << std::endl;
      reset_fraction *= reset_fraction_reduction_rate;
      model->reset_state(rand(), reset_fraction);
      model->w.setZero();
      model->w0 = 0;
      continue;
    }
  }
  table.print_row("FINAL", progress_time(), rmse);
  table.print_footer();

  std::string termination_criterion = "";
  if (iter == max_iters){
    termination_criterion = "Optimization Complete: Iteration limit reached.";
  }
  logprogress_stream << termination_criterion << std::endl;

  // Return the training stats
  std::map<std::string, variant_type> training_stats;
  training_stats["training_time"] = progress_time().elapsed_seconds;
  training_stats["num_iterations"]        = to_variant(iter);
  training_stats["final_objective_value"] = to_variant(rmse);
  model->_training_stats = training_stats;
  return model;
}


} // namespace als
} // namespace recsys
} // namespace turi


#endif
