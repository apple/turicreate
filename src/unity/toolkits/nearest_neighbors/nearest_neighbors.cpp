/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Toolkits
#include <unity/toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <algorithm>
#include <boost/algorithm/string.hpp>

// SFrame
#include <sframe/sframe_reader.hpp>

// ML Data
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>

namespace turi {
namespace nearest_neighbors {



// -----------------------------------------------------------------------------
// NEAREST NEIGHBORS HELPER FUNCTIONS
// -----------------------------------------------------------------------------


/**
 * Convert the index of a flat array into row and column indices for an upper
 * triangular matrix. The general idea for the algorithm is from this
 * StackOverflow thread:
 * http://stackoverflow.com/questions/242711/algorithm-for-index-numbers-of-triangular-matrix-coefficients
 */
std::pair<size_t, size_t> upper_triangular_indices(const size_t i,
                                                   const size_t n) {

  size_t num_cells = n * (n + 1) / 2;

  DASSERT_TRUE(num_cells > 0);
  DASSERT_TRUE(i <= num_cells);

  size_t reverse_row_idx = std::floor((std::sqrt(8 * (num_cells - i)) - 1) / 2);
  size_t row = n - 1 - reverse_row_idx;
  size_t col = n - num_cells + i + (reverse_row_idx * (reverse_row_idx + 1) / 2);

  return {row, col};
}


/**
 * Get a distance function's name from the function_closure_info.
 */
std::string extract_distance_function_name(
  const function_closure_info distance_fn) {

  std::string dist_name = distance_fn.native_fn_name;

  if (dist_name.find_first_of(".") != std::string::npos) {
    dist_name = dist_name.substr(dist_name.find_first_of(".")+1, dist_name.size());
  }

  return dist_name;
}


/**
 * Figure out how many memory blocks to break the reference and query datasets
 * into.
 *
 * Assume that each block has the same number of query and reference rows (r).
 * Each thread loads into memory a reference block with 8 * dimension * r bytes
 * and a distance matrix of 8 * r^2 bytes. This function simply uses to
 * quadratic formula to figure out the upper bound on r.
 *
 * One copy of each query block is also loaded into memory sequentially, but
 * this is ignored.
 */
std::pair<size_t, size_t> calculate_num_blocks(const size_t num_ref_examples,
                                               const size_t num_query_examples,
                                               const size_t dimension,
                                               const size_t max_thread_memory,
                                               const size_t min_ref_blocks,
                                               const size_t min_query_blocks) {

  size_t max_thread_cells = max_thread_memory / sizeof(double);

  // quadratic formula to find the number of rows of data that will fill the
  // allotted (per-thread) memory. This includes both the reference data on the
  // thread, as well as the distance matrix that will be computed.
  double max_block_rows =
    (-2 * (double)dimension + std::sqrt(dimension * dimension + 4 * max_thread_cells)) / 2;
  max_block_rows = (size_t) std::max((double)1, max_block_rows);

  logprogress_stream << "max rows per data block: " << max_block_rows << std::endl;


  // Allow the client to use *more* blocks than would otherwise be required to
  // fit in memory, to take full advantage of parallelization.
  size_t min_ref_data_blocks = ((num_ref_examples - 1) / max_block_rows) + 1;
  size_t num_ref_blocks = std::max(min_ref_data_blocks, min_ref_blocks);

  size_t min_query_data_blocks = ((num_query_examples - 1) / max_block_rows) + 1;
  size_t num_query_blocks = std::max(min_query_data_blocks, min_query_blocks);

  return {num_ref_blocks, num_query_blocks};
}


/**
 * Read data into a dense matrix, in parallel.
 */
void parallel_read_data_into_matrix(const v2::ml_data& dataset, DenseMatrix& A,
                                    const size_t block_start,
                                    const size_t block_end) {

  DASSERT_EQ(A.n_rows, (block_end - block_start));
  DASSERT_EQ(A.n_cols, dataset.metadata()->num_dimensions());

  v2::ml_data block_data = dataset.slice(block_start, block_end);

  in_parallel([&](size_t thread_idx, size_t num_threads) {
    for (auto it = block_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
      it.fill_row_expr(A.row(it.row_index()));
    }
  });
}


/**
 * Read data into a dense matrix, single threaded.
 */
void read_data_into_matrix(const v2::ml_data& dataset, DenseMatrix& A,
                           const size_t block_start, const size_t block_end) {

  DASSERT_EQ(A.n_rows, (block_end - block_start));
  DASSERT_EQ(A.n_cols, dataset.metadata()->num_dimensions());

  v2::ml_data block_data = dataset.slice(block_start, block_end);

  for (auto it = block_data.get_iterator(0, 1); !it.done(); ++it) {
    it.fill_row_expr(A.row(it.row_index()));
  }
}


/**
 * Find the query nearest neighbors for a block of queries and a block of
 * reference data.
 */
void find_block_neighbors(const DenseMatrix& R, const DenseMatrix& Q,
                          std::vector<neighbor_candidates>& neighbors,
                          const std::string& dist_name,
                          const size_t ref_offset, const size_t query_offset) {

  // Compute euclidean distance by matrix multiplication.
  size_t num_ref_examples = R.n_rows;
  size_t num_query_examples = Q.n_rows;
  DenseMatrix dists(num_ref_examples, num_query_examples);

  if (dist_name == "euclidean" || dist_name == "squared_euclidean") {
    all_pairs_squared_euclidean(R, Q, dists);
  }

  else if (dist_name == "cosine") {
    all_pairs_cosine(R, Q, dists);
  }

  else if (dist_name == "dot_product") {
    all_pairs_dot_product(R, Q, dists);
  }

  else if (dist_name == "transformed_dot_product") {
    all_pairs_transformed_dot_product(R, Q, dists);
  }

  else {
    log_and_throw("Distance name not understood.");
  }

  for (size_t j = 0; j < num_query_examples; ++j) {
    size_t idx_query = j + query_offset;

    // Find the closest reference points
    for (size_t i = 0; i < num_ref_examples; ++i) {
      size_t idx_ref = i + ref_offset;
      neighbors[idx_query].evaluate_point(std::pair<double, size_t>(dists(i, j), idx_ref));
    }
  }
}


/**
 * Find the nearest neighbors for each point in a block of reference data.
 * Update the nearest neighbor heaps for both the rows and columns in the
 * resulting distance matrix.
 */
void off_diag_block_similarity_graph(const DenseMatrix& R, const DenseMatrix& C,
                                     std::vector<neighbor_candidates>& neighbors,
                                     const std::string& dist_name,
                                     const size_t row_offset,
                                     const size_t col_offset) {

  // Compute distances by matrix multiplication.
  size_t num_rows = R.n_rows;
  size_t num_cols = C.n_rows;
  DenseMatrix dists(num_rows, num_cols);

  if (dist_name == "euclidean" || dist_name == "squared_euclidean") {
    all_pairs_squared_euclidean(R, C, dists);
  }

  else if (dist_name == "cosine") {
    all_pairs_cosine(R, C, dists);
  }

  else if (dist_name == "dot_product") {
    all_pairs_dot_product(R, C, dists);
  }

  else if (dist_name == "transformed_dot_product") {
    all_pairs_dot_product(R, C, dists);
  }

  else {
    log_and_throw("Distance name not understood.");
  }

  // Update the nearest neighbors. Assume the matrix is not symmetric.
  for (size_t i = 0; i < num_rows; ++i) {
    size_t idx_row = i + row_offset;

    for (size_t j = 0; j < num_cols; ++j) {
      size_t idx_col = j + col_offset;
      neighbors[idx_row].evaluate_point(std::pair<double, size_t>(dists(i, j), idx_col));
      neighbors[idx_col].evaluate_point(std::pair<double, size_t>(dists(i, j), idx_row));
    }
  }
}


/**
 * Write nearest neighbors results stored in a vector of heaps to a stacked
 * SFrame.
 */
sframe write_neighbors_to_sframe(
    std::vector<nearest_neighbors::neighbor_candidates>& neighbors,
    const std::vector<flexible_type>& reference_labels,
    const std::vector<flexible_type>& query_labels) {

  sframe result;
  append_neighbors_to_sframe(result, neighbors, reference_labels, query_labels);
  result.close();
  return result;
}


/**
 * append nearest neighbors results stored in a vector of heaps to a SFrame.
 */
void append_neighbors_to_sframe(
    sframe& result,
    std::vector<nearest_neighbors::neighbor_candidates>& neighbors,
    const std::vector<flexible_type>& reference_labels,
    const std::vector<flexible_type>& query_labels) {

  size_t num_queries = neighbors.size();
  size_t max_num_threads = thread::cpu_count();

  if (!result.is_opened_for_write()) {
    // Set up the output SFrame
    flex_type_enum ref_label_type = flex_type_enum::INTEGER;
    flex_type_enum query_label_type = flex_type_enum::INTEGER;

    if (reference_labels.size() > 0) {
      ref_label_type = reference_labels[0].get_type();
    }

    if (query_labels.size() > 0) {
      query_label_type = query_labels[0].get_type();
    }

    std::vector<std::string> column_names ({"query_label", "reference_label",
                                           "distance", "rank"});
    std::vector<flex_type_enum> column_types ({query_label_type,
                                              ref_label_type,
                                              flex_type_enum::FLOAT,
                                              flex_type_enum::INTEGER});

    result.open_for_write(column_names, column_types, "", max_num_threads);
  }

  in_parallel([&](size_t thread_idx, size_t num_threads) {
    auto it = result.get_output_iterator(thread_idx);
    std::vector<flexible_type> row(4);

    size_t start_idx = (thread_idx * num_queries) / num_threads;
    size_t end_idx = ((thread_idx + 1) * num_queries) / num_threads;

    for (size_t i = start_idx; i < end_idx; ++i) {
      neighbors[i].sort_candidates();
      row[0] = query_labels[neighbors[i].get_label()];  // query_label
      for (size_t j = 0; j < neighbors[i].candidates.size(); ++j) {
        row[1] = reference_labels[neighbors[i].candidates[j].second]; // reference row label
        row[2] = neighbors[i].candidates[j].first;                    // distance
        row[3] = j + 1;                                               // rank
        *it = row;
      }
    }
  });
}



// -----------------------------------------------------------------------------
// NEAREST NEIGHBORS MODEL METHODS
// -----------------------------------------------------------------------------

void nearest_neighbors_model::train(const sframe& X,
                     const std::vector<dist_component_type>& composite_distance_params,
                     const std::map<std::string, flexible_type>& opts) {
 std::vector<flexible_type> ref_labels (X.num_rows(), flexible_type(0));
  for (size_t i = 0; i < X.num_rows(); ++i) {
    ref_labels[i] = i;
  }

  train(X, ref_labels, composite_distance_params, opts);
}


void nearest_neighbors_model::train(const sframe& X, const sframe& ref_labels,
                     const std::vector<dist_component_type>& composite_distance_params,
                     const std::map<std::string, flexible_type>& opts) {
  if (ref_labels.num_columns() < 1) {
    log_and_throw("No columns present in the reference labels SFrame.");
  }

  std::vector<flexible_type> ref_labels_vec (X.num_rows(), flexible_type(0));
  ref_labels.select_column(0)->get_reader()->read_rows(0, X.num_rows(), ref_labels_vec);
  train(X, ref_labels_vec, composite_distance_params, opts);
}


sframe nearest_neighbors_model::query(const sframe& X, const size_t k,
                                      const double radius) const {

  std::vector<flexible_type> query_labels (X.num_rows(), flexible_type(0));
  for (size_t i = 0; i < X.num_rows(); ++i) {
    query_labels[i] = i;
  }

  return query(X, query_labels, k, radius);
}


sframe nearest_neighbors_model::query(const sframe& X,
                                      const sframe& query_labels,
                                      const size_t k,
                                      const double radius) const {

 if (query_labels.num_columns() < 1) {
    log_and_throw("No columns present in the query labels SFrame.");
  }

  std::vector<flexible_type> query_labels_vec (X.num_rows(), flexible_type(0));
  query_labels.select_column(0)->get_reader()->read_rows(0, X.num_rows(),
                                                         query_labels_vec);
  return query(X, query_labels_vec, k, radius);
}

sframe nearest_neighbors_model::query(const sframe& X,
                                      const std::vector<flexible_type>& query_labels,
                                      const size_t k,
                                      const double radius) const {
  check_schema_for_query(X);
  sframe X_query = X.select_columns(get_feature_names());

  v2::ml_data mld_queries(metadata, false);
  mld_queries.set_data(X_query, "", {}, untranslated_cols);
  mld_queries.fill();

  return query(mld_queries, query_labels, k, radius, true);
}


sframe nearest_neighbors_model::similarity_graph(const size_t k,
                                                 const double radius,
                                                 const bool include_self_edges) const {
  return query(mld_ref, reference_labels, k, radius, include_self_edges);
}


/**
 * Empty constructor
 */
nearest_neighbors_model::nearest_neighbors_model() {}


/**
 * Get training stats.
 */
std::map<std::string, flexible_type> nearest_neighbors_model::get_training_stats() const {

  auto ret = std::map<std::string, flexible_type>();
  auto fields = std::vector<std::string>({"training_time",
                                          "num_examples",
                                          "num_features",
                                          "num_unpacked_features",
                                          "label"});
  for (auto& k : fields) {
    ret[k] = safe_varmap_get<flexible_type>(state, k);
  }

  DASSERT_TRUE(is_trained());
  return ret;
}


/**
 * Get names of features
 */
std::vector<std::string> nearest_neighbors_model::get_feature_names() const {

  size_t num_columns = metadata->num_columns();

  std::vector<std::string> features(num_columns);

  for(size_t c_idx = 0; c_idx < num_columns; ++c_idx)
    features[c_idx] = metadata->column_name(c_idx);

  return features;
}


/**
 * Get metadata
 */
std::shared_ptr<v2::ml_metadata>
nearest_neighbors_model::get_metadata() const {
  return metadata;
}


/**
 * Initialize data inputs for model training.
 */
void nearest_neighbors_model::validate_distance_components(
  const std::vector<dist_component_type>& _composite_params, const sframe& X) {

  std::vector<std::string> column_names;
  function_closure_info distance_fn;
  double weight;

  auto composite_params_for_python
    = std::vector<std::tuple<std::vector<std::string>, std::string, double> >();

  // First loop over distance components
  // - validate inputs
  // - record which columns are untranslated for string distances
  for (auto& d : _composite_params) {

    // Retrieve items from the distance component tuple
    column_names = std::get<0>(d);
    distance_fn = std::get<1>(d);
    weight = std::get<2>(d);

    // Validate distance component arguments and get the row data type
    validate_distance_component(column_names, X, distance_fn, weight);

    // Update 'untranslated_cols' if it's a string component
    // - there should only be a single column if this is the case.  TODO: Why?
    if (boost::algorithm::ends_with(distance_fn.native_fn_name,
                                    "levenshtein")) {
      untranslated_cols[column_names[0]] = v2::ml_column_mode::UNTRANSLATED;
    }

    // Save the composite function for retreival from Python, but replace
    // the function_closure_info with the name of the function.
    auto dist_name = extract_distance_function_name(distance_fn);
    composite_params_for_python.push_back(std::make_tuple(column_names, dist_name, weight));
  }

  this->composite_params = _composite_params;
  state["distance"] = to_variant(composite_params_for_python);
  state["num_distance_components"] = composite_params_for_python.size();
}


void nearest_neighbors_model::initialize_distances() {

  // Second loop over the distance component tuple to construct distance
  // component objects
  std::vector<std::string> column_names;
  function_closure_info distance_fn;
  double weight;

  for (auto& d : composite_params) {

    // Retrieve items from the distance component tuple
    column_names = std::get<0>(d);
    distance_fn = std::get<1>(d);
    weight = std::get<2>(d);

    // Get native function, if it exists
    auto dist = distance_metric::make_distance_metric(distance_fn);

    // Get the column indices for the features in the current distance component
    // - sort the indices for the row slicer
    std::vector<size_t> column_idxs;

    for (auto col : column_names) {
      column_idxs.push_back(metadata->column_index(col));
    }
    std::sort(column_idxs.begin(), column_idxs.end());

    // Determine row data type (i.e. sparsity, if not a string)
    // TODO: remove 'is_dense' once the ball tree is updated to use distance components
    is_dense = (metadata->num_dimensions() <= 4 * metadata->num_columns());

    // Determine row data type and/or sparsity
    row_type data_type_flag;

    if (boost::algorithm::ends_with(distance_fn.native_fn_name,
                                    ".levenshtein")) {
      data_type_flag = row_type::flex_type;
    } else if (is_dense) {
      data_type_flag = row_type::dense;
    } else {
      data_type_flag = row_type::sparse;
    }


    // TODO: Clean the choice of data_type up
    if (boost::algorithm::ends_with(distance_fn.native_fn_name, ".jaccard")||
        boost::algorithm::ends_with(distance_fn.native_fn_name, ".weighted_jaccard")) {
      data_type_flag = row_type::sparse;
    }

    // Create the distance component
    dist_component c = {column_names,
                        dist,
                        weight,
                        v2::row_slicer(metadata, column_idxs),
                        data_type_flag};
    composite_distances.push_back(c);
  }
}


/**
 * Check the table scheme for evaluation.
 */
void nearest_neighbors_model::check_schema_for_query(const sframe& X) const {

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
 * Check if the data is empty!
 */
void nearest_neighbors_model::check_empty_data(const sframe& X) const {
  if (X.num_rows() == 0) {
    log_and_throw("Input SFrame does not contain any rows.");
  }

  if (X.num_columns() == 0) {
    log_and_throw("Input SFrame does not contain any columns.");
  }
}


/*
 * Check for missing values in the untranslated columns, aka string features.
 * - Assumes the training data is already set in the model, as 'mld_ref'.
 */
void nearest_neighbors_model::check_missing_strings(const sframe& X) const {

  // Collect names of string columns and subset the input SFrame to just these
  // columns.
  std::vector<std::string> string_col_names;
  for (size_t i = 0; i < X.num_columns(); ++i) {
    if (X.column_type(i) == flex_type_enum::STRING) {
      string_col_names.push_back(X.column_name(i));
    }
  }

  if (string_col_names.size() == 0)
    return;

  // Parallel iterate through rows of the subsetted SFrame, checking if the
  // values are of type flex_type_enum::UNDEFINED
  sframe X_string = X.select_columns(string_col_names);
  parallel_sframe_iterator_initializer it_init(X_string);

  in_parallel([&](size_t thread_idx, size_t num_threads) {
    for (parallel_sframe_iterator it(it_init, thread_idx, num_threads); !it.done(); ++it) {

      for (size_t j = 0; j < string_col_names.size(); ++j) {

        if (it.value(j).get_type() == flex_type_enum::UNDEFINED) {
          std::stringstream ss;
          ss << "Missing value (None) encountered in column '"
             << string_col_names[j] << "'. "
             << "Use the SFrame's `dropna` function to drop rows with "
             << "'None' values." << std::endl;
          log_and_throw(ss.str());
        }
      }
    }
  });
}


/**
 * Initialize the reference ml_data object in the model, and set metadata in
 * the model's state for visibility to Python.
 */
void nearest_neighbors_model::initialize_model_data(const sframe& X,
  const std::vector<flexible_type>& ref_labels) {

  // Check if there are missing values in the string features. If there are,
  // error out.
  check_missing_strings(X);

  // Initialize the ml_data object and associated metadata.
  mld_ref.set_data(X, "", {}, untranslated_cols);
  mld_ref.fill();
  metadata = mld_ref.metadata();

  reference_labels = ref_labels;

  // Set metadata in model state
  num_examples = mld_ref.size();
  std::vector<std::string> unpacked_feature_names = metadata->feature_names();

  size_t num_unpacked_features = metadata->num_dimensions() +
                                 metadata->num_untranslated_columns();

  add_or_update_state({ {"num_examples", num_examples},
                        {"num_features", metadata->num_columns()},
                        {"num_unpacked_features", num_unpacked_features},
                        {"features", to_variant(metadata->column_names())},
                        {"unpacked_features", to_variant(unpacked_feature_names)} });
}


/**
* Check that the feature types are valid for a particular distance component.
*/
void nearest_neighbors_model::validate_distance_component(
  const std::vector<std::string> column_names, const sframe& X,
  const function_closure_info distance_fn, const double weight) {

  std::stringstream ss;
  flex_type_enum ctype;
  std::string col_name;
  bool string_col_present = false;

  // Get the string form of the distance name
  std::string distance_name = distance_fn.native_fn_name;
  if (distance_name.find_first_of(".") != std::string::npos) {
    distance_name = distance_name.substr(distance_name.find_first_of(".") + 1,
                                         distance_name.size());
  }

  // Validate the relative component weight
  if ((weight < 0) || (weight > 1e9)) {
    ss.str("");
    ss << "Relative distance weights must be between 0 and 1e9."
       << std::endl;
    log_and_throw(ss.str());
  }


  // Loop through column names
  for (size_t i = 0; i < column_names.size(); i++) {
    col_name = column_names[i];
    ctype = X.column_type(col_name);

    // Flag string columns and make sure they only get a string distance
    if (ctype == flex_type_enum::STRING) {
      string_col_present = true;

      if (distance_name != "levenshtein") {
        ss.str("");
        ss << "The only distance allowed for string features is 'levenshtein'. "
           << "Please try this distance, or use 'text_analytics.count_ngrams' to "
           << "convert the strings to dictionaries, which permit more distance "
           << "functions." << std::endl;
        log_and_throw(ss.str());
      }
    }

    // Check if feature types are allowable
    if (ctype != flex_type_enum::INTEGER &&
        ctype != flex_type_enum::FLOAT &&
        ctype != flex_type_enum::VECTOR &&
        ctype != flex_type_enum::DICT &&
        ctype != flex_type_enum::LIST &&
        ctype != flex_type_enum::STRING) {

      ss.str("");
      ss << "Feature '" << col_name << "' not of type"
         << " integer, float, dictionary, list, vector, or string." << std::endl;
      log_and_throw(ss.str());
    }

    // Jaccard and weighted_jaccard distance should only get dictionary features
    if (((distance_name == "jaccard") || (distance_name == "weighted_jaccard")) &&
        ((ctype != flex_type_enum::DICT) && (ctype != flex_type_enum::LIST))) {
      ss.str("");
      ss << "Cannot compute jaccard distances with column '" << X.column_name(i)
         << "'. Jaccard distances currently can only be computed for dictionary "
         << "and list features." << std::endl;
      log_and_throw(ss.str());
    }

    // Levenshtein distance should get only string columns
    if ((distance_name == "levenshtein") && (ctype != flex_type_enum::STRING)) {
      ss.str("");
      ss << "Cannot compute levenshtein distance with column '" << X.column_name(i)
         << "'. levenshtein distance can only computed for string features."
         << std::endl;
      log_and_throw(ss.str());
    }
  }  // end loop over columns

  // String distances should only have a single data feature
  if ((string_col_present) && (column_names.size() > 1)) {
    ss.str("");
    ss << "Cannot compute string distances on multiple columns."
       << " Please select a single column for string distances, or concatenate "
       << "multiple string columns into a single column before creating the "
       << "nearest neighbors model."
       << std::endl;
    log_and_throw(ss.str());
  }
}



// -----------------------------------------------------------------------------
// CANDIDATE NEIGHBORS METHODS
// -----------------------------------------------------------------------------

/**
* Constructors and destructor
*/
neighbor_candidates::neighbor_candidates(size_t lbl, size_t a, double b, bool c) {

  // note: if either 'label' or 'k' is set to -1, they will actually be
  // max(size_t) - 1;
  label = lbl;
  k = a;
  radius = b;
  include_self_edges = c;

  if (k != NONE_FLAG) {
    candidates.reserve(k + 1); // the extra spot is for points pushed onto the heap
    std::make_heap(candidates.begin(), candidates.end());
  }
}


neighbor_candidates::~neighbor_candidates() {
}


/**
 * Setter for the label member.
 */
void neighbor_candidates::set_label(size_t label) {
  this->label = label;
}

/**
 * Accesor for the label
 */
size_t neighbor_candidates::get_label() const {
  return label;
}

/**
 * Accessor for the max number of neighbors (i.e. k).
 */
size_t neighbor_candidates::get_max_neighbors() const {
  return k;
}


/**
 * Accessor for the radius.
 */
double neighbor_candidates::get_radius() const {
  return radius;
}


/**
* Evaluate a point as a nearest neighbors candidate.
*/
void neighbor_candidates::evaluate_point(const std::pair<double, size_t>& point) {

  // If the heaps are set to be length 0, do nothing.
  if(k == 0)
    return;

  // If the candidate label matches the heap label, and self edges are supposed
  // to be excluded, do nothing.
  if ((point.second == label) && (!include_self_edges)) {
    return;
  }

  // First check if the radius constraint is either undefined or defined and
  // satisfied
  if (((radius >= 0) && (point.first <= radius)) || (radius < 0)) {

    // If k is not defined, meeting the radius constraint is sufficient to add
    // the point to the candidates *vector*
    heap_lock.lock();

    if (k == NONE_FLAG) {
      candidates.push_back(point);

    // If k is defined, then need to follow the logic of adding to the fixed
    // length heap.
    } else {

      // If the heap isn't full, push the point but don't pop any old candidates
      if (candidates.size() < k) {
        candidates.push_back(point);
        std::push_heap(candidates.begin(), candidates.end());

      // If the heap is full and the new point's distances is less than the max
      // distance, push the new point and pop the existing candidate with the
      // max distance.
      } else {
        if (point.first < candidates[0].first) {
          candidates.push_back(point);
          std::push_heap(candidates.begin(), candidates.end());
          std::pop_heap(candidates.begin(), candidates.end());
          candidates.pop_back();
        }
      }
    }

    heap_lock.unlock();
  }
}


/**
* Print candidates.
*/
void neighbor_candidates::print_candidates() const {
  logprogress_stream << std::endl << "label: " << label<< std::endl;

  for (size_t i = 0; i < candidates.size(); i++) {
    logprogress_stream << candidates[i].second << ": " << candidates[i].first<< std::endl;
  }
  logprogress_stream << std::endl;
}


/**
* Sort candidates.
*/
void neighbor_candidates::sort_candidates() {
  // std::sort(candidates.begin(), candidates.end());

  if (k != NONE_FLAG) {  // if k is defined, candidates is a heap
    std::sort_heap(candidates.begin(), candidates.end());
  } else {
    std::sort(candidates.begin(), candidates.end());
  }
}


/**
* Return the max distance of the current candidates.
*/
double neighbor_candidates::get_max_dist() const {
  double max_dist = -1.0;

  // if there's nothing in the candidates vector, return -1.0;
  if (candidates.size() > 0) {

    if (k != NONE_FLAG) {         // if k is defined, candidates is a heap
      max_dist = candidates[0].first;

    } else {
      for (size_t i = 0; i < candidates.size(); i++) {
        if (candidates[i].first > max_dist)
          max_dist = candidates[i].first;
      }
    }
  }

  return max_dist;
}


flexible_type nearest_neighbors_model::get_reference_data() const {
    DASSERT_EQ(num_examples, mld_ref.size());

    DenseVector ref_data(metadata->num_dimensions());
    flex_list ret(num_examples);
    for (auto it = mld_ref.get_iterator(0, 1); !it.done(); ++it) {
      it.fill_row_expr(ref_data);
      ret[it.row_index()] = arma::conv_to<std::vector<double>>::from(ref_data);
    }

    return ret;
}

flexible_type _nn_get_reference_data(std::shared_ptr<nearest_neighbors_model> model) {
  return model->get_reference_data();
}

}  // namespace nearest_neighbors
}  // namespace turi
