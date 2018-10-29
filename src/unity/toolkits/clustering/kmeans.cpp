/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/clustering/kmeans.hpp>

namespace turi {
namespace kmeans {


// **************************
// *** Distance functions ***
// **************************

/**
 * Compute euclidean distance between two dense_vectors.
 * \param a dense_vector
 * \param b dense_vector
 * \returns double
 */
inline double euclidean(const dense_vector& a, const dense_vector& b) {
  DASSERT_TRUE(a.size() > 0);
  DASSERT_TRUE(a.size() == b.size());
  return arma::norm(a - b, 2);
}

/**
 * Compute squared euclidean distance between two dense_vectors. Used only in
 * the Kmeans++ initialization step.
 * \param a dense_vector
 * \param b dense_vector
 * \returns double
 */
inline double squared_euclidean(const dense_vector& a, const dense_vector& b) {
  DASSERT_TRUE(a.size() > 0);
  DASSERT_TRUE(a.size() == b.size());
  return squared_norm(a - b);
}



// ************************
// *** Helper functions ***
// ************************

/**
 * Make sure the dataset is not empty.
 */
void check_empty_data(const sframe& X) {
  if (X.num_rows() == 0) {
    log_and_throw("Input SFrame does not contain any rows.");
  }

  if (X.num_columns() == 0) {
    log_and_throw("Input SFrame does not contain any columns.");
  }
}

/**
 * Check that the feature types are valid for the kmeans model.
 */
void check_column_types(const sframe& X) {
  std::stringstream ss;

  for (size_t i = 0; i < X.num_columns(); i++) {
    flex_type_enum ctype = X.column_type(i);

    // Check if feature types are allowable
    if (ctype != flex_type_enum::INTEGER &&
        ctype != flex_type_enum::FLOAT &&
        ctype != flex_type_enum::VECTOR &&
        ctype != flex_type_enum::DICT) {

      ss.str("");
      ss << "Feature '" << X.column_name(i) << "' not of type"
         << " integer, float, dict, or array." << std::endl;

      log_and_throw(ss.str());
    }
  }
}

/**
 * Convert integer columns in the input dataset into floats.
 */
sframe convert_ints_to_floats(const sframe& X) {
  sframe X_clean = X;

  for (size_t j = 0; j < X.num_columns(); ++j) {
    if (X.column_type(j) == flex_type_enum::INTEGER) {
      gl_sarray new_col(X.select_column(j));
      new_col = new_col.astype(flex_type_enum::FLOAT);
      X_clean = X_clean.replace_column(new_col.materialize_to_sarray(),
                                       X.column_name(j));
    }
  }

  return X_clean;
}

/**
 * Check the table scheme for evaluation.
 */
void check_schema_for_predict(const sframe& X,
                              const std::shared_ptr<v2::ml_metadata> metadata) {

  std::stringstream ss;

  // Make sure feature names and schema's match
  for (size_t i = 0; i < metadata->num_columns(); ++i) {
    std::string name = metadata->column_name(i);

    if (not X.contains_column(name)) {
      ss << "Schema mismatch. Feature '" << name << "' was present during "
         << "training, but is missing during prediction." << std::endl;
      log_and_throw(ss.str());
    }

    flex_type_enum create_type = metadata->column_type(i);
    flex_type_enum query_type = X.column_type(name);

    if (create_type != query_type) {
      ss << "Schema mismatch. Feature '" << name << "' was of type "
         << flex_type_enum_to_name(create_type)
         << " during training but is of type "
         << flex_type_enum_to_name(query_type) << " during prediction."
         << std::endl;
      log_and_throw(ss.str());
    }
  }
}

/**
 * Write cluster assigments to an SFrame and return. Also records the row
 * index of the point and the distance from the point to its assigned cluster.
 *
 * NOTE: it's critical that this function write the results in the correct
 * order. Users expect this output to be sorted in the same order as the input
 * data.
 */
sframe write_cluster_assignments(const std::vector<size_t>& cluster_labels,
                                 const std::vector<float>& distances,
                                 const std::vector<flexible_type>& row_labels,
                                 const std::string& row_label_name) {

  // Determine type of the row label column.
  flex_type_enum row_label_type;

  if (row_labels.size() > 0) {
    row_label_type = row_labels[0].get_type();
  } else {
    row_label_type = flex_type_enum::INTEGER;
  }

  // Construct and open the output SFrame.
  sframe out;
  size_t num_segments = thread::cpu_count();

  std::vector<std::string> col_names ({row_label_name,
                                       "cluster_id",
                                       "distance"});

  std::vector<flex_type_enum> col_types ({row_label_type,
                                          flex_type_enum::INTEGER,
                                          flex_type_enum::FLOAT});

  out.open_for_write(col_names, col_types, "", num_segments);

  // Write the cluster assignments to the output SFrame.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    size_t start_idx = (thread_idx * cluster_labels.size()) / num_threads;
    size_t end_idx   = ((thread_idx + 1) * cluster_labels.size()) / num_threads;
    auto it_out = out.get_output_iterator(thread_idx);
    std::vector<flexible_type> row(3);

    for (size_t i = start_idx; i < end_idx; ++i) {
      row[0] = row_labels[i];
      row[1] = cluster_labels[i];
      row[2] = distances[i];
      *it_out = row;
    }
  });

  out.close();
  return out;
}



// ***********************
// *** Cluster methods ***
// ***********************

/**
 * Copy constructor
 */
cluster& cluster::operator=(const cluster& other) {
  if (this != &other) {
    std::lock_guard<turi::mutex> guard(this->m);
    center = other.center;
    count = other.count;
  }
  return *this;
}

/**
 * Safe mean update that avoids overflow.
 */
void cluster::safe_update_center(const dense_vector& u) {
  std::lock_guard<turi::mutex> guard(this->m);
  count++;
  center += (u - center) / count;
}



// **************************************
// *** Public kmeans training methods ***
// **************************************

/**
 * Empty constructor
 */
kmeans_model::kmeans_model() {}

/**
 * Destructor.
 */
kmeans_model::~kmeans_model() {}


/**
 * Set the model options. The option manager should throw errors if the options
 * do not satisfy the option manager's conditions.
 */
void kmeans_model::init_options(const std::map<std::string,
                                flexible_type>& _options) {

  options.create_integer_option("num_clusters", "Number of clusters to use",
                                5, 1, 1e5, false);
  options.create_integer_option("max_iterations",
                                "Maximum number of iterations to perform", 10,
                                0, std::numeric_limits<int>::max(), false);

  options.create_string_option("method", "Method for training the model",
                               "elkan", false);

    // Note that the batch size can be modified by `train`. In particular, if it's
  // too large, but the chosen method is 'minibatch', then 'batch_size' is
  // reduced.
  options.create_integer_option("batch_size", "Number of data points per iteration",
                                1000, 1, std::numeric_limits<int>::max(), true);

  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Train the kmeans model, without row labels. Just creates the row labels, and
 * calls the other train method.
 */
void kmeans_model::train(const sframe& X, const sframe& init_centers,
                         std::string method, bool allow_categorical) {

  std::vector<flexible_type> row_labels (X.num_rows(), flexible_type(0));
  for (size_t i = 0; i < X.num_rows(); ++i) {
    row_labels[i] = i;
  }

  std::string row_label_name;
  row_label_name = "row_id";

  train(X, init_centers, method, row_labels, row_label_name, allow_categorical);
}

/**
 * Train the kmeans model, with row labels.
 */
void kmeans_model::train(const sframe& X,
                         const sframe& init_centers, std::string method,
                         const std::vector<flexible_type>& row_labels,
                         const std::string row_label_name,
                         bool allow_categorical) {

  timer t;
  size_t start_time = t.current_time();


  // Validate the input data, cast int columns to floats, and convert to
  // ml_data, then set model 'state'.
  check_empty_data(X);
  if (!allow_categorical) {
    check_column_types(X);
  }
  sframe X_clean = convert_ints_to_floats(X);
  initialize_model_data(X_clean, row_labels, row_label_name);


  // Process the combination of batch size and method.
  if (batch_size >= num_examples) {
    logprogress_stream << "Batch size is larger than the input dataset. "
                       << "Switching to an exact Kmeans method." << std::endl;
    batch_size = num_examples;
    method = "elkan";
    add_or_update_state({ {"batch_size", batch_size},
                          {"method", method} });
  }


  // Choose or process initial cluster centers and set in the model.
  if (init_centers.num_rows() > 0) {
    process_custom_centers(init_centers);
  } else {
    choose_random_centers();
  }


  // Update model state after initialization. This can't be done earlier custom
  // centers cause the metadata to change.
  std::vector<std::string> unpacked_feature_names = metadata->feature_names();

  add_or_update_state ({ {"num_unpacked_features", metadata->num_dimensions()},
                         {"unpacked_features", to_variant(unpacked_feature_names)} });


  // Main training iterations, depending on the 'low-mem' option.
  if (max_iterations > 0) {
    logprogress_stream << "Starting kmeans model training." << std::endl;
  }

  size_t iter = 0;

  if (method == "lloyd")
    iter = compute_clusters_lloyd();
  else if (method == "elkan")
    iter = compute_clusters_elkan();
  else if (method == "minibatch")
    iter = compute_clusters_minibatch();
  else
    log_and_throw("Unable to understand which method to use for Kmeans training.");


  // Finalize the model
  add_or_update_state({ {"training_time", t.current_time() - start_time},
                        {"training_iterations", iter} });
}


/**
 * Predict the cluster assignment for new data, according to a trained Kmeans
 * model.
 */
sframe kmeans_model::predict(const sframe& X) {

  // Validate and clean the prediction data.
  check_empty_data(X);
  sframe X_clean = convert_ints_to_floats(X);
  check_schema_for_predict(X_clean, metadata);
  sframe X_predict = X.select_columns(metadata->column_names());

  // Convert X into an ml_data object (with the same metadata as at training).
  v2::ml_data mld_predict(metadata, false);
  mld_predict.set_data(X_predict, "");
  mld_predict.fill();

  // Initialize prediction state.
  std::vector<size_t> new_assignments(mld_predict.size(), 0);
  std::vector<float> new_upper_bounds(mld_predict.size(),
                                      std::numeric_limits<float>::infinity());

  // Find the assignment for each point (naively for now).
  in_parallel([&](size_t thread_idx, size_t num_threads) {
    dense_vector x(metadata->num_dimensions());

    for (auto it = mld_predict.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      size_t i = it.row_index();
      it.fill_row_expr(x);

      for (size_t k = 0; k < num_clusters; ++k) {
        float d = squared_euclidean(x, clusters[k].center);

        if (d < new_upper_bounds[i]) {
          new_assignments[i] = k;
          new_upper_bounds[i] = d;
        }
      }

      // Once the assignment for a point is known, compute the exact distance.
      new_upper_bounds[i] = std::sqrt(new_upper_bounds[i]);
    }
  });

  // Write the results to an SFrame.
  std::vector<flexible_type> row_ids (mld_predict.size(), flexible_type(0));
  for (size_t i = 0; i < mld_predict.size(); ++i) {
    row_ids[i] = i;
  }

  sframe result = write_cluster_assignments(new_assignments, new_upper_bounds,
                                            row_ids, "row_id");
  return result;
}

/**
 * Write cluster assigments to an SFrame and return. Also records the row
 * index of the point and the distance from the point to its assigned cluster.
 */
sframe kmeans_model::get_cluster_assignments() {
  return write_cluster_assignments(assignments, upper_bounds, row_labels,
                                   row_label_name);
}

/**
 * Write cluster metadata to an SFrame and return. Records the features for
 * each cluster, the count of assigned points, and the within-cluster sum of
 * squared distances.
 */
sframe kmeans_model::get_cluster_info() {

  // Get final cluster counts.
  for (size_t k = 0; k < num_clusters; k++) {
    clusters[k].count = 0;
  }

   parallel_for(size_t(0), num_examples, [&](size_t i) {
    clusters[assignments[i]].count++;
  });


  // Sum the squared distances of each point to its assigned center.
  std::vector<double> cluster_squared_error(num_clusters, 0);

  parallel_for(size_t(0), num_examples, [&](size_t i) {
    DASSERT_TRUE(assignments[i] >= 0 && assignments[i] < num_clusters);
    cluster_squared_error[assignments[i]] += (upper_bounds[i] * upper_bounds[i]);
  });


  // Construct the output SFrame
  std::vector<std::string> col_names = metadata->column_names();
  std::vector<flex_type_enum> col_types;

  // REMEMBER: metadata is based on int columns cast to floats
  for (std::string& colname : col_names) {
    col_types.push_back(metadata->column_type(colname));
  }

  col_names.push_back("cluster_id");
  col_names.push_back("size");
  col_names.push_back("sum_squared_distance");
  col_types.push_back(flex_type_enum::INTEGER);
  col_types.push_back(flex_type_enum::INTEGER);
  col_types.push_back(flex_type_enum::FLOAT);

  sframe out;
  out.open_for_write(col_names, col_types, "", 1);
  auto it_out = out.get_output_iterator(0);

  // Write the cluster metadata to the output SFrame.
  for(size_t k = 0; k < num_clusters; ++k) {
    std::vector<flexible_type> row = mldata.translate_row_to_original(clusters[k].center);
    row.push_back(k);
    row.push_back(clusters[k].count.value);
    row.push_back(cluster_squared_error[k]);
    *it_out = row;
  }

  out.close();
  return out;
}




// *******************************************
// *** Public kmeans generic model methods ***
// *******************************************

/**
 * Serialize the model
 */
void kmeans_model::save_impl(turi::oarchive& oarc) const {

  // Extract the cluster centers into a clean vector of dense_vectors.
  std::vector<dense_vector> centers(num_clusters);
  for (size_t k = 0; k < num_clusters; ++k) {
    centers[k] = clusters[k].center;
  }

  // Serialize all the things.
  variant_deep_save(state, oarc);  // 'state' is in ml_model_base
  oarc << metadata
       << options
       << centers
       << row_labels;
}


/**
 * De-serialize the model
 */
void kmeans_model::load_version(turi::iarchive& iarc, size_t version) {

  ASSERT_MSG((version == 1) || (version == 2) || (version == 3) || (version == 4),
             "This model version cannot be loaded. Please re-save your model.");

  variant_deep_load(state, iarc);


  // Standard loading procedure
  if (version >= 2) {

    iarc >> metadata
         >> options;

    num_clusters = options.value("num_clusters");
    std::vector<dense_vector> centers(num_clusters);
    iarc >> centers;

    clusters.assign(num_clusters, cluster(metadata->num_dimensions()));

    for (size_t k = 0; k < num_clusters; ++k) {
      clusters[k].center = centers[k];
    }
  }

  // Load row label information.
  if (version >= 4) {
    iarc >> row_labels;

  } else {
    row_label_name = "row_id";
    add_or_update_state({ {"row_label_name", row_label_name} });

    row_labels.resize(num_examples);
    for (size_t i = 0; i < num_examples; ++i) {
      row_labels[i] = i;
    }
  }

  // Special case for version 1.
  if (version == 1) {
    logprogress_stream << "WARNING: Loading Kmeans model from GrapLab Create 1.3 or "
                       << "earlier. Please note: in Turi Create 1.4 and "
                       << "later, the 'cluster_info' output reports the sum of "
                       << "*squared* distances for each cluster, rather than the "
                       << "sum of distances. The loaded model still reports the "
                       << "sum of distances, using the original column names. "
                       << "Also note that the number of distance computations "
                       << "is set artifically to 0 for models saved in versions "
                       << "before Turi Create 1.4."
                       <<  std::endl;

    std::map<std::string, variant_type> data;
    variant_deep_load(data, iarc);

#define __EXTRACT(n) n = variant_get_value<decltype(n)>(data.at(#n))

      __EXTRACT(num_examples);

#undef __EXTRACT

    add_or_update_state({ {"method", "naive"},
                          {"batch_size", num_examples} });
  }


  // Special case for version 2.
  if (version == 2) {
    add_or_update_state({ {"method", "elkan"},
                          {"batch_size", state["num_examples"]} });
  }
}



// ***************************************
// *** Private kmeans training methods ***
// ***************************************


/**
 * Initialize the kmeans model's data. Setup ML data, and initialize data
 * members of the model.
 */
void kmeans_model::initialize_model_data(const sframe& X,
                                 const std::vector<flexible_type>& row_labels,
                                 const std::string row_label_name) {

  // Initialize the ml_data object and its associated metadata.
  mldata.set_data(X, "");  // no target column
  mldata.fill();
  metadata = mldata.metadata();


  // Compute or retrieve, then set model metadata
  num_examples = mldata.size();
  num_clusters = options.value("num_clusters");
  max_iterations = options.value("max_iterations");
  batch_size = options.value("batch_size");

  this->row_label_name = row_label_name;
  this->row_labels = row_labels;


  // Initialize cluster assignments, center_distances, and distance bounds.
  assignments.assign(num_examples, 0);
  upper_bounds.assign(num_examples, std::numeric_limits<float>::max());


  // Set model data in the model's 'state' field, visible to the Python API.
  std::vector<std::string> feature_names = metadata->column_names();

  add_or_update_state({ {"num_examples", num_examples},
                        {"batch_size", batch_size},
                        {"num_features", metadata->num_columns()},
                        {"features", to_variant(feature_names)},
                        {"row_label_name", row_label_name} });
}


/**
 * Set the user's custom initial cluster centers in the model.
 */
void kmeans_model::process_custom_centers(const sframe& init_centers) {

  logprogress_stream << "Initializing user-provided cluster centers."
                     << std::endl;

  // Convert the initial centers SFrame to an ml_data object for faster
  // iteration and simpler handling of complex feature types.
  v2::ml_data mld_centers(metadata);
  mld_centers.set_data(init_centers, "");
  mld_centers.fill();

  // Reset the index for ml_data using both the training data and the custom
  // centers.
  metadata->set_training_index_sizes_to_current_column_sizes();

  // Initialize and fill clusters.
  clusters.assign(num_clusters, cluster(metadata->num_dimensions()));

  in_parallel([&](size_t thread_idx, size_t num_threads) {
    for (auto it = mld_centers.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      it.fill_row_expr(clusters[it.row_index()].center);
    }
  });
}


/*
 * Choose random initial cluster centers, with a modified version of the
 * probabilisitic k-means++ method.
 */
void kmeans_model::choose_random_centers() {

  logprogress_stream << "Choosing initial cluster centers with Kmeans++."
                     << std::endl;

  clusters.assign(num_clusters, cluster(metadata->num_dimensions()));


  // Figure out the maximum number of rows we can sample, based on the
  // assumption that 1GB of memory is available, and the actual number of center
  // seed points.
  size_t row_bytes = 8 * metadata->num_dimensions();
  size_t max_bytes = 1024 * 1024 * 1024;  // 1 GB
  size_t max_mem_rows = max_bytes / row_bytes;


  // Determine the right number of seed points.
  if (num_clusters > num_examples) {
    std::stringstream ss;
    ss << "For randomly initialized clusters, the number of clusters "
       << "must be no larger than the number of data points.";
    log_and_throw(ss.str());
  }


  // If the number of clusters is larger than the maximum number of rows that
  // can be held in memory, just use the random sample as the initial centers.
  // Remember that if num_clusters is larger than num_examples, we've already
  // errored out, so the sample here is never larger than num_examples.
  if (num_clusters > max_mem_rows) {
    logprogress_stream << "WARNING: Too many clusters to initialize with "
                       << "Kmeans++ (relative to the number of unpacked features). "
                       << "Using uniformly randomly selected initial "
                       << "centers instead. Because cluster centers are in "
                       << "in memory, this may take a long time." << std::endl;

    // Draw a uniformly random sample of data and fill the cluster centers.
    v2::ml_data seed_data = mldata.create_subsampled_copy(num_clusters, 0);

    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = seed_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        it.fill_row_expr(clusters[it.row_index()].center);
      }
    });
  }


  // If the number of clusters is sufficiently small, read as many rows of data
  // into memory as possible (uniformly sampled), and do the usual kmeans++
  // initialization procedure from the sample.
  else {

    // Draw a uniformly random sample of data and read it into memory.
    size_t num_seeds = std::min(max_mem_rows, num_examples);
    v2::ml_data seed_data = mldata.create_subsampled_copy(num_seeds, 0);
    std::vector<decltype(clusters[0].center)> seeds(seed_data.num_rows());

    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = seed_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        seeds[it.row_index()].resize(metadata->num_dimensions());
        it.fill_row_expr(seeds[it.row_index()]);
      }
    });

    table_printer progress_table({{"Center number", 0}, {"Row index", 0}});
    progress_table.print_header();

    // Choose the first center and set in the model.
    size_t idx_center = turi::random::fast_uniform<size_t>(0, seeds.size() - 1);
    progress_table.print_progress_row(0, 0, idx_center);
    clusters[0].center = seeds[idx_center];

    // Choose 2nd through Kth centers
    std::vector<float> min_squared_dists(seeds.size(),
                                         std::numeric_limits<float>::max());

    for (size_t k = 1; k < num_clusters; ++k) {

      // Compute the squared distance from each point to the previously chosen
      // center and update if it's smaller than the existing smallest distance.
      parallel_for(0, seeds.size(), [&](size_t idx) {
        float d = squared_euclidean(clusters[k-1].center, seeds[idx]);
        if (d < min_squared_dists[idx]) {
          min_squared_dists[idx] = d + 1e-16;
        }
      });

      // Sample a point proportional to the squared distance to the closest
      // existing center.
      size_t idx_center = turi::random::multinomial(min_squared_dists);
      clusters[k].center = seeds[idx_center];
      progress_table.print_progress_row(k, k, idx_center);

      // Break if Ctrl-C has been pressed.
      if (cppipc::must_cancel()) {
         log_and_throw(std::string("Toolkit canceled by user."));
      }
    }

    progress_table.print_footer();
  }
}


/**
 * Low-memory version of main Kmeans iterations, using Lloyd's algorithm.
 */
size_t kmeans_model::compute_clusters_lloyd() {

  size_t iter = 0;
  size_t num_changed = update_assignments_lloyd();

  // Initialize the progress table
  table_printer progress_table({{"Iteration", 0},
                               {"Number of changed assignments", 0}});
  if (max_iterations > 0) {
    progress_table.print_header();
  }

  // Main training loop to update cluster centers and point assignments.
  while (num_changed > 0 && iter < max_iterations) {
    ++iter;

    update_cluster_centers();
    num_changed = update_assignments_lloyd();
    progress_table.print_row(iter, num_changed);
  }

  // Finalize the progress table and print a convergence warning, if necessary.
  if (max_iterations > 0) {
    progress_table.print_footer();
  }

  if (num_changed > 0 && iter == max_iterations) {
    logprogress("WARNING: Clustering did not converge within max_iterations.");
  }

  // Compute exact distance between every point and its assigned cluster.
  set_exact_point_distances();

  return iter;
}


/**
 * High-memory version of main Kmeans iterations, using Elkan's algorithm.
 */
size_t kmeans_model::compute_clusters_elkan() {

  // First iteration. Compute initial center distances and initial cluster
  // assignments.
  logprogress_stream << "Assigning points to initial cluster centers." << std::endl;

  this->center_dists = turi::symmetric_2d_array<float> (num_clusters, 0);
  compute_center_distances();
  assign_initial_clusters_elkan();  // also updates the distance upper bounds


  // Initialize the progress table
  table_printer progress_table({{"Iteration", 0},
                               {"Number of changed assignments", 0}});
  if (max_iterations > 0) {
    progress_table.print_header();
  }


  // Main loop of training iterations
  size_t iter = 0;
  size_t num_changed = num_examples;

  while (num_changed > 0 && iter < max_iterations) {
    ++iter;

    // Copy the current cluster centers into a temporary variable then compute
    // the new cluster centers.
    std::vector<cluster> previous_clusters = clusters;
    update_cluster_centers();

    // Compute the distance between each center and its previous location and
    // adjust the upper bounds based on the displacements.
    adjust_distance_bounds(previous_clusters);

    // Compute all pairwise distances between cluster centers.
    compute_center_distances();

    // Update cluster assignment for each point, if necessary.
    num_changed = update_assignments_elkan();
    progress_table.print_row(iter, num_changed);
  }


  // Finalize the progress table and print a convergence warning, if necessary.
  if (max_iterations > 0) {
    progress_table.print_footer();
  }

  if (num_changed > 0 && iter == max_iterations) {
    logprogress("WARNING: Clustering did not converge within max_iterations.");
  }


  // Compute exact distance between every point and its assigned cluster.
  set_exact_point_distances();

  return iter;
}


/**
 * Minibatch Kmeans iterations.
 */
size_t kmeans_model::compute_clusters_minibatch() {

  table_printer progress_table({{"Iteration", 0}});
  if (max_iterations > 0) {
    progress_table.print_header();
  }

  // Main training iterations
  std::vector<float> batch_distances(batch_size, std::numeric_limits<float>::max());
  std::vector<size_t> batch_assignments(batch_size, 0);

  for (size_t iter = 0; iter < max_iterations; ++iter) {

    // Randomly select a batch of data. This is probably expensive, so think
    // about doing all of the batches at once outside of the main loop.
    v2::ml_data batch_data = mldata.create_subsampled_copy(batch_size, iter);


    // 1st pass - assign the current batch of points to a cluster. This could
    // potentially be faster with the triangle inequality on center distances,
    // but for the first draft it's not worth the code complexity.
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      dense_vector x(metadata->num_dimensions());

      for (auto it = batch_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        size_t i = it.row_index();
        it.fill_row_expr(x);
        batch_distances[i] = std::numeric_limits<float>::max();

        for (size_t k = 0; k < num_clusters; ++k) {
          float d = squared_euclidean(x, clusters[k].center);

          if (d < batch_distances[i]) {
            batch_assignments[i] = k;
            batch_distances[i] = d;
          }
        }
      }
    });


    // 2nd pass - update cluster centers. Note: the `safe_update_center` method
    // looks different than what's in the paper, but it is arithmetically
    // identical.
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      dense_vector x(metadata->num_dimensions());

      for (auto it = batch_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        size_t i = it.row_index();
        it.fill_row_expr(x);
        clusters[batch_assignments[i]].safe_update_center(x);
      }
    });

    progress_table.print_row(iter);
  }

  if (max_iterations > 0) {
    progress_table.print_footer();
  }


  // Assign every point in the original dataset to a learned cluster
  // ('assignments') and record the distance from the point to its assigned
  // cluster ('upper_bounds'). The Lloyd assignment returns squared euclidean
  // distance, so the second step is to get the square roots.
  update_assignments_lloyd();

  parallel_for(size_t(0), num_examples, [&](size_t i) {
    upper_bounds[i] = std::sqrt(upper_bounds[i]);
  });


  return max_iterations;
}


/**
 * Compute distances between all pairs of cluster centers. The nested vector
 * containing these distances is a model attribute.
 */
void kmeans_model::compute_center_distances() {

  parallel_for(size_t(0), num_clusters, [&](size_t k) {

    // Break under Ctrl-C. Necessary because no SArray reads here.
    if (cppipc::must_cancel()) {
       log_and_throw(std::string("Toolkit canceled by user."));
    }

    for (size_t j = 0; j < k; ++j) {
      center_dists(j, k) = euclidean(clusters[j].center, clusters[k].center);
    }
  });
}


void kmeans_model::adjust_distance_bounds(
  const std::vector<cluster>& previous_clusters) {

  std::vector<float> displacements(num_clusters, 0);

  parallel_for(size_t(0), num_clusters, [&](size_t k) {
    displacements[k] = euclidean(clusters[k].center, previous_clusters[k].center);
  });

  parallel_for(size_t(0), num_examples, [&](size_t i) {
    upper_bounds[i] = upper_bounds[i] + displacements[assignments[i]];
  });
}


/**
 * Initialize the point assignments and the bounds on distances between points
 * and cluster centers.
 */
void kmeans_model::assign_initial_clusters_elkan() {

  // Assign each point to its closest cluster. Requires looping over all points
  // and all clusters, but only compute the exact distance if the candidate
  // center is sufficiently close to the currently assigned center.
  in_parallel([&](size_t thread_idx, size_t num_threads) {
    dense_vector x(metadata->num_dimensions());

    for(auto it = mldata.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      size_t i = it.row_index();
      it.fill_row_expr(x);

      for (size_t k = 0; k < num_clusters; ++k) {

        // NOTE: upper_bounds are initialized to infinity, so for the first
        // center this condition is always true.
        if (center_dists(assignments[i], k) < 2 * upper_bounds[i]) {
          float d = euclidean(x, clusters[k].center);

          if (d < upper_bounds[i]) {
            upper_bounds[i] = d;
            assignments[i] = k;
          }
        }
      }
    }
  });
}


/**
 * Update cluster centers to be the means of the currently assigned points.
 */
void kmeans_model::update_cluster_centers() {

  // Reset the clusters to be empty.
  clusters.assign(num_clusters, cluster(metadata->num_dimensions()));

  // Add each point to its assigned cluster's center, proportional to the
  // cluster's count.
  in_parallel([&](size_t thread_idx, size_t num_threads) {
    dense_vector x(metadata->num_dimensions());

    for (auto it = mldata.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      size_t i = it.row_index();
      it.fill_row_expr(x);
      clusters[assignments[i]].safe_update_center(x);
    }
  });
}


/**
 * Update the cluster assignments based on the current cluster means and return
 * the number of assignments that changed. Uses cluster center distances to
 * eliminate many exact point-to-center distance computations.
 */
size_t kmeans_model::update_assignments_elkan() {

  atomic<size_t> num_changed = 0;  // changes to false if any point switches clusters

  // Compute distances between points and centers, but only if it's possible for
  // the candidate cluster to be closer to the point, based on the triangle
  // inequality.
  in_parallel([&](size_t thread_idx, size_t num_threads) {
    dense_vector x(metadata->num_dimensions());

    for (auto it = mldata.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      size_t i = it.row_index();
      it.fill_row_expr(x);
      size_t prev_assignment = assignments[i];

      for (size_t k = 0; k < num_clusters; ++k) {
        if (k != assignments[i] &&
            2 * upper_bounds[i] > center_dists(assignments[i], k)) {

          float d_assigned = squared_euclidean(x, clusters[assignments[i]].center);
          float d_candidate = squared_euclidean(x, clusters[k].center);

          if (d_candidate < d_assigned) {
            assignments[i] = k;
            upper_bounds[i] = std::sqrt(d_candidate);

          } else {
            upper_bounds[i] = std::sqrt(d_assigned);
          }
        }
      }

      if(assignments[i] != prev_assignment) {
        ++num_changed;
      }
    }
  });

  return num_changed.value;
}


/**
 * Update cluster assignments based on current cluster means and return the
 * number of assignments that changed. Computes all point-to-center distances.
 */
size_t kmeans_model::update_assignments_lloyd() {

  atomic<size_t> num_changed = 0;

  in_parallel([&](size_t thread_idx, size_t num_threads) {
    dense_vector x(metadata->num_dimensions());

    for (auto it = mldata.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      size_t i = it.row_index();
      it.fill_row_expr(x);
      size_t prev_assignment = assignments[i];
      upper_bounds[i] = std::numeric_limits<float>::infinity();

      for (size_t k = 0; k < num_clusters; ++k) {
        float d = squared_euclidean(x, clusters[k].center);

        if (d < upper_bounds[i]) {
          assignments[i] = k;
          upper_bounds[i] = d;
        }
      }

      if (assignments[i] != prev_assignment) {
        num_changed++;
      }
    }
  });

  return num_changed.value;
}

/**
 *  Compute the exact distance between each point and its assigned cluster.
 *  Store in the 'upper_bounds' member, which for low-memory clustering is the
 *  exact distance anyway.
 */
void kmeans_model::set_exact_point_distances() {
  in_parallel([&](size_t thread_idx, size_t num_threads) {
    dense_vector x(metadata->num_dimensions());

    for (auto it = mldata.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      size_t i = it.row_index();
      it.fill_row_expr(x);

      upper_bounds[i] = euclidean(x, clusters[assignments[i]].center);
    }
  });
}

} // end of namespace kmeans
} // end of namespace turi
