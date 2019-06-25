/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// ML Data
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>

// Toolkits
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/brute_force_neighbors.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

// Miscellaneous
#include <timer/timer.hpp>
#include <algorithm>
#include <string>
#include <Eigen/Core>
#include <model_server/lib/toolkit_util.hpp>
#include <core/logging/table_printer/table_printer.hpp>


namespace turi {
namespace nearest_neighbors {

#define NONE_FLAG (size_t) -1
#ifdef NDEBUG
#define BRUTE_FORCE_NEAREST_NEIGHBORS_BIG_DATA 10000000
#else
// Small enough that the test datasets force multiple blocks to be tested
#define BRUTE_FORCE_NEAREST_NEIGHBORS_BIG_DATA 23
#endif

#ifdef NDEBUG
#define BLOCKWISE_BRUTE_FORCE_MAX_THREAD_MEMORY 1024 * 1024 * 512 // 512MB
#else
#define BLOCKWISE_BRUTE_FORCE_MAX_THREAD_MEMORY 1024 * 128 // 125KB - small enough to test multiple blocks
#endif


/**
 * Destructor. Make sure bad things don't happen
 */
brute_force_neighbors::~brute_force_neighbors(){

}



/**
* Set options
*/
void brute_force_neighbors::init_options(const std::map<std::string,
                                         flexible_type>& _options) {
  options.create_string_option("label",
                               "Name of the reference dataset column with row labels.",
                               "",
                               false);

  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}


/**
 * Train a brute force nearest neighbors model.
 */
void brute_force_neighbors::train(const sframe& X,
                                  const std::vector<flexible_type>& ref_labels,
                                  const std::vector<dist_component_type>& composite_distance_params,
                                  const std::map<std::string, flexible_type>& opts) {

  logprogress_stream << "Starting brute force nearest neighbors model training." << std::endl;

  timer t;
  double start_time = t.current_time();

  // Initialize the table printer. It's not used here, but this saves about 1
  // second on the first call to 'query'.
  table_printer table({ {"Query points", 0}, {"# Pairs", 0}, {"% Complete.", 0},
                        {"Elapsed Time", 0}});

  // Validate the inputs.
  init_options(opts);
  validate_distance_components(composite_distance_params, X);

  // Create the ml_data object for the reference data.
  initialize_model_data(X, ref_labels);

  // Initialize the distance components. NOTE: this needs data to be initialized
  // first because the row slicers need the column indices to be sorted.
  initialize_distances();

  add_or_update_state({ {"method", "brute_force"},
                        {"training_time", t.current_time() - start_time} });
}


sframe brute_force_neighbors::query(const v2::ml_data& mld_queries,
                                 const std::vector<flexible_type>& query_labels,
                                 const size_t k, const double radius,
                                 const bool include_self_edges) const {

  size_t num_query_examples = mld_queries.size();

  ASSERT_FALSE(composite_distances.empty());
  size_t num_components = composite_distances.size();

  function_closure_info distance_fn = std::get<1>(composite_params[0]);
  auto dist_name = extract_distance_function_name(distance_fn);


  // Adjust the value for the max neighbors constraint.
  size_t kstar;

  if (k == NONE_FLAG) {
    kstar = NONE_FLAG;
  } else {
    kstar = std::min(k, mld_ref.size());
  }

  size_t dimension = metadata->num_dimensions();
  std::vector<neighbor_candidates> neighbors;

  // Blockwise queries
  // -----------------
  if (num_query_examples > 20 &&
      dimension > 0 &&                        // must be some numeric data
      dimension <= 10000 &&                   // but not too much (this should really be relative to BLOCKWISE_BRUTE_FORCE_MAX_THREAD_MEMORY)
      dimension == mld_ref.max_row_size() &&  // dense data only for block-wise queries
      num_components == 1 &&                  // must only be a single distance component
      (dist_name == "euclidean"
       || dist_name == "squared_euclidean"
       || dist_name == "gaussian_kernel"
       || dist_name == "cosine"
       || dist_name == "dot_product"
       || dist_name == "transformed_dot_product")) {


    // adjust the radius value for "transformed" distances.
    double rstar;

    if (radius >= 0 && dist_name == "euclidean") {
      rstar = radius * radius;
    } else {
      rstar = radius;
    }

    neighbors.assign(num_query_examples,
                     neighbor_candidates(-1, kstar, rstar, include_self_edges));

    parallel_for(0, num_query_examples, [&](size_t i) {
      neighbors[i].set_label(i);
    });

    blockwise_query(mld_queries, neighbors, dist_name);
  }


  // Pairwise queries
  // ----------------
  else {

    neighbors.assign(num_query_examples,
                    neighbor_candidates(-1, kstar, radius, include_self_edges));

    parallel_for(0, num_query_examples, [&](size_t i) {
      neighbors[i].set_label(i);
    });

    pairwise_query(mld_queries, neighbors);
  }

  // Print the results to an SFrame, sorting each set of neighbors in the
  // process.
  sframe result = write_neighbors_to_sframe(neighbors, reference_labels,
                                            query_labels);
  return result;
}


/**
 * Search a nearest neighbors reference object for the neighbors of every
 * point.
 */
sframe brute_force_neighbors::similarity_graph(const size_t k,
                                               const double radius,
                                               const bool include_self_edges) const {

  // Get the number of distance function components and the name of the first
  // one.
  ASSERT_FALSE(composite_distances.empty());
  size_t num_components = composite_distances.size();
  function_closure_info distance_fn = std::get<1>(composite_params[0]);
  auto dist_name = extract_distance_function_name(distance_fn);


  // Adjust the value for the max_neighbors constraint.
  size_t kstar;

  if (k == NONE_FLAG) {
    kstar = NONE_FLAG;
  } else {
    kstar = std::min(k, mld_ref.size());
  }


  // Compute results and print to an SFrame, sorting the neighbors for each
  // point in the process.
  std::vector<neighbor_candidates> neighbors;
  size_t dimension = metadata->num_dimensions();

  if (dimension > 0 && dimension <= 10000 &&  // reasonable dimension
      dimension == mld_ref.max_row_size() &&  // dense data
      num_components == 1 &&                  // single component
      (dist_name == "euclidean"
       || dist_name == "squared_euclidean" // appropriate distance function
       || dist_name == "gaussian_kernel"
       || dist_name == "cosine"
       || dist_name == "dot_product"
       || dist_name == "transformed_dot_product")) {

    // Adjust the radius value for "transformed" distances.
    double rstar;

    if (radius >= 0 && dist_name == "euclidean") {
      rstar = radius * radius;
    } else {
      rstar = radius;
    }

    neighbors.assign(num_examples, neighbor_candidates(-1, kstar, rstar,
                                                       include_self_edges));

    parallel_for(0, num_examples, [&](size_t i) {
      neighbors[i].set_label(i);
    });


    blockwise_similarity_graph(neighbors, dist_name);
  }

  else {

    neighbors.assign(num_examples, neighbor_candidates(-1, kstar, radius,
                                                       include_self_edges));

    parallel_for(0, num_examples, [&](size_t i) {
      neighbors[i].set_label(i);
    });

    pairwise_query(mld_ref, neighbors);
  }


  sframe result = write_neighbors_to_sframe(neighbors, reference_labels,
                                            reference_labels);
  return result;
}


/**
 * Construct the similarity graph for the reference data, using blockwise matrix
 * multiplication for distance computations.
 */
void brute_force_neighbors::blockwise_similarity_graph(
                                    std::vector<neighbor_candidates>& neighbors,
                                    const std::string& dist_name) const {

  logprogress_stream << "Starting blockwise similarity graph construction."
                     << std::endl;

  // Figure out how many blocks to cut the data into.
  size_t dimension = metadata->num_dimensions();
  atomic<size_t> num_pairs_so_far = 0;
  size_t max_num_threads = thread::cpu_count();

  std::pair<size_t, size_t> num_blocks =
    calculate_num_blocks(num_examples, num_examples, dimension,
                         BLOCKWISE_BRUTE_FORCE_MAX_THREAD_MEMORY,  // max memory to use per thread
                         max_num_threads,             // min reference blocks
                         1);                          // min query blocks - n/a for this usage

  size_t num_ref_blocks = num_blocks.first;

  logprogress_stream << "number of reference data blocks: " << num_ref_blocks << std::endl;


  // Figure out how many blocks and how many total pairs
  size_t num_dist_blocks = num_ref_blocks * (num_ref_blocks + 1) / 2;

  size_t rows_per_block = std::ceil(num_examples / (double) num_ref_blocks);
  size_t pairs_per_block = rows_per_block * rows_per_block;
  size_t num_pairs_total = num_dist_blocks * pairs_per_block;

  table_printer table({ {"# Pairs", 0},
                        {"% Complete.", 0},
                        {"Elapsed Time", 0} });
  table.print_header();


  // Loop over compute blocks
  parallel_for(size_t(0), num_dist_blocks, [&](size_t r) {

    if (cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit cancelled by user."));
    }

    // Figure out which rows of data to use for the current block.
    size_t a, b; std::tie(a, b) = upper_triangular_indices(r, num_ref_blocks);


    // Read block rows into memory
    size_t row_start = (a * num_examples) / num_ref_blocks;
    size_t row_end = ((a + 1) * num_examples) / num_ref_blocks;
    size_t num_rows = row_end - row_start;

    DenseMatrix A(num_rows, dimension);
    parallel_read_data_into_matrix(mld_ref, A, row_start, row_end);


    // Block is on the diagonal of the block matrix.
    if (a == b) {
      find_block_neighbors(A, A, neighbors, dist_name, row_start, row_start);
      num_pairs_so_far += num_rows * num_rows;
    }

    // Block is off the diagonal of the block matrix.
    else {
      size_t col_start = (b * num_examples) / num_ref_blocks;
      size_t col_end = ((b + 1) * num_examples) / num_ref_blocks;
      size_t num_cols = col_end - col_start;
      num_pairs_so_far += (num_rows * num_cols);

      DenseMatrix B(num_cols, dimension);
      parallel_read_data_into_matrix(mld_ref, B, col_start, col_end);
      off_diag_block_similarity_graph(A, B, neighbors, dist_name, row_start,
                                      col_start);
    }

    table.print_timed_progress_row((size_t) num_pairs_so_far,
                                   (4 * 100.0 * num_pairs_so_far) / (num_pairs_total * 4.0),
                                   progress_time());
  });


  // Convert squared euclidean distances to euclidean distances and clean up
  // numerical foibles.
  parallel_for(size_t(0), num_examples, [&](size_t i) {
    for (size_t j = 0; j < neighbors[i].candidates.size(); ++j) {
      if (neighbors[i].candidates[j].first < 1e-15) {
        neighbors[i].candidates[j].first = 0;
      }
    }
  });

  if (dist_name == "euclidean") {
    parallel_for(size_t(0), num_examples, [&](size_t i) {
      for (size_t j = 0; j < neighbors[i].candidates.size(); ++j) {
        neighbors[i].candidates[j].first = std::sqrt(neighbors[i].candidates[j].first);
      }
    });
  } else if (dist_name == "gaussian_kernel") {
    parallel_for(size_t(0), num_examples, [&](size_t i) {
      for (size_t j = 0; j < neighbors[i].candidates.size(); ++j) {
        neighbors[i].candidates[j].first = std::sqrt(neighbors[i].candidates[j].first);
      }
    });
  }


  table.print_row(num_pairs_total, 100, progress_time());
  table.print_footer();
}


/**
 * Find neighbors of queries in a created brute_force model.
 */
void brute_force_neighbors::blockwise_query(const v2::ml_data& mld_queries,
                                    std::vector<neighbor_candidates>& neighbors,
                                    const std::string& dist_name) const {

  logprogress_stream << "Starting blockwise querying." << std::endl;

  // Figure out how many reference and query blocks.
  size_t dimension = metadata->num_dimensions();
  size_t num_ref_examples = mld_ref.size();
  size_t num_query_examples = mld_queries.size();
  size_t num_pairs_total = num_query_examples * num_ref_examples;
  atomic<size_t> num_pairs_so_far = 0;
  size_t max_num_threads = thread::cpu_count();

  std::pair<size_t, size_t> num_blocks =
    calculate_num_blocks(num_ref_examples, num_query_examples, dimension,
                         BLOCKWISE_BRUTE_FORCE_MAX_THREAD_MEMORY,  // max memory to use per thread
                         max_num_threads,             // min reference blocks
                         1);                          // min query blocks

  size_t num_ref_blocks = num_blocks.first;
  size_t num_query_blocks = num_blocks.second;

  logprogress_stream << "number of reference data blocks: " << num_ref_blocks << std::endl;
  logprogress_stream << "number of query data blocks: " << num_query_blocks << std::endl;

  table_printer table({ {"Query points", 0},
                        {"# Pairs", 0},
                        {"% Complete.", 0},
                        {"Elapsed Time", 0} });
  table.print_header();


  // Outer loop over query blocks
  for (size_t q = 0; q < num_query_blocks; ++q) {

    // read query data into memory.
    size_t query_start = (q * num_query_examples) / num_query_blocks;
    size_t query_end = ((q + 1) * num_query_examples) / num_query_blocks;
    size_t num_block_queries = query_end - query_start;

    DenseMatrix Q(num_block_queries, dimension);
    parallel_read_data_into_matrix(mld_queries, Q, query_start, query_end);

    // Inner loop over reference query blocks
    parallel_for(size_t(0), num_ref_blocks, [&](size_t r) {

      // read reference data into memory
      size_t ref_start = (r * num_ref_examples) / num_ref_blocks;
      size_t ref_end = ((r + 1) * num_ref_examples) / num_ref_blocks;
      size_t num_block_refs = ref_end - ref_start;

      DenseMatrix R(num_block_refs, dimension);
      read_data_into_matrix(mld_ref, R, ref_start, ref_end);

      // find nearest neighbors and print progress
      find_block_neighbors(R, Q, neighbors, dist_name, ref_start, query_start);

      if (cppipc::must_cancel()) {
        log_and_throw(std::string("Toolkit cancelled by user."));
      }

      num_pairs_so_far += (num_block_refs * num_block_queries);
      table.print_timed_progress_row(query_end,
                                     (size_t) num_pairs_so_far,
                                     (4 * 100.0 * num_pairs_so_far) / (num_pairs_total * 4.0),
                                     progress_time());
    });
  }


  // Convert squared euclidean distances to euclidean distances and clean up
  // numerical foibles.
  parallel_for(size_t(0), num_query_examples, [&](size_t i) {
    for (size_t j = 0; j < neighbors[i].candidates.size(); ++j) {
      if (neighbors[i].candidates[j].first < 1e-15) {
        neighbors[i].candidates[j].first = 0;
      }
    }
  });

  if (dist_name == "euclidean") {
    parallel_for(size_t(0), num_query_examples, [&](size_t i) {
      for (size_t j = 0; j < neighbors[i].candidates.size(); ++j) {
        neighbors[i].candidates[j].first = std::sqrt(neighbors[i].candidates[j].first);
      }
    });
  } else if (dist_name == "gaussian_kernel") {
    parallel_for(size_t(0), num_query_examples, [&](size_t i) {
      for (size_t j = 0; j < neighbors[i].candidates.size(); ++j) {
        neighbors[i].candidates[j].first = 1.0 - std::exp(-neighbors[i].candidates[j].first);
      }
    });
  }



  table.print_row("Done", num_pairs_total, 100, progress_time());
  table.print_footer();
}


/**
 * Find neighbors of queries in a created brute_force model.
 */
void brute_force_neighbors::pairwise_query(const v2::ml_data& mld_queries,
                            std::vector<neighbor_candidates>& neighbors) const {

  logprogress_stream << "Starting pairwise querying." << std::endl;

  // Extract key sizes and dimensions
  size_t num_queries = mld_queries.size();
  size_t num_pairs_total = num_queries * mld_ref.size();
  atomic<size_t> n_pairs = 0;
  size_t dimension = metadata->num_dimensions();
  size_t num_components = composite_distances.size();

  // Query observations
  SparseVector q_sp(dimension);
  DenseVector q(dimension);
  std::vector<flexible_type> q_flex;

  // Query caches
  // - each of these objects holds a block of query observations, with features
  //   sliced out for each distance component.
  // - This is inefficient because we are reserving memory for each row type and
  //   each distance component, but each distance component can only be a single
  //   row type. The problem is that we don't know it ahead of time.
  // TODO: think about templating to make this simpler.
  std::vector<std::vector<DenseVector>> mld_queries_memory(num_components);
  std::vector<std::vector<SparseVector>> mld_queries_memory_sp(num_components);
  std::vector<std::vector<std::vector<flexible_type>>> mld_queries_memory_flex(num_components);

  table_printer table({ {"Query points", 0}, {"# Pairs", 0}, {"% Complete.", 0},
                        {"Elapsed Time", 0}});

  table.print_header();


  // Figure out a good number of blocks so each can be stored in memory
  size_t num_blocks = 1;
  size_t target_block_size = BRUTE_FORCE_NEAREST_NEIGHBORS_BIG_DATA / (mld_queries.max_row_size() + 1);

  while(mld_queries.size() / num_blocks > target_block_size) {
    num_blocks *= 2;
  }

  flexible_type empty_string = std::string("");

  // Outermost loop is over blocks of queries read into memory
  for(size_t block_index = 0; block_index < num_blocks; ++block_index) {

    // Resize the memory block
    size_t block_start = (block_index * mld_queries.size()) / num_blocks;
    size_t block_end = ((block_index + 1) * mld_queries.size()) / num_blocks;
    size_t block_size = block_end - block_start;

    for (size_t i = 0; i < composite_distances.size(); ++i) {
      if (composite_distances[i].row_sparsity == row_type::dense) {
        mld_queries_memory[i].resize(block_size);
      } else if (composite_distances[i].row_sparsity == row_type::flex_type) {
        mld_queries_memory_flex[i].resize(block_size);
      } else {
        mld_queries_memory_sp[i].resize(block_size);
      }
    }

    // Read a chunk of queries into the memory block
    {
      auto mld_queries_in_block = mld_queries.slice(block_start, block_end);

      in_parallel([&](size_t thread_idx, size_t num_threads) {
        std::vector<v2::ml_data_entry> q_t;
        std::vector<flexible_type> q_u;

        for (auto it_query = mld_queries_in_block.get_iterator(thread_idx, num_threads);
             !it_query.done(); ++it_query) {

          // fill the translated and untranslated vectors
          it_query.fill_observation(q_t);
          it_query.fill_untranslated_values(q_u);

          // Replace missing untranslated values with empty strings.
          for (size_t i = 0; i < q_u.size(); ++i) {
            if (q_u[i].get_type() == flex_type_enum::UNDEFINED) {
              q_u[i] = empty_string;
            }
          }

          // slice out the appropriate features for each distance component
          for (size_t i = 0; i < composite_distances.size(); ++i) {

            auto& c = composite_distances[i];

            if (c.row_sparsity == row_type::dense) {
              c.slicer.slice(mld_queries_memory[i][it_query.row_index()], q_t, q_u);
            } else if (c.row_sparsity == row_type::flex_type) {
              c.slicer.slice(mld_queries_memory_flex[i][it_query.row_index()], q_t, q_u);
            } else {
              c.slicer.slice(mld_queries_memory_sp[i][it_query.row_index()], q_t, q_u);
            }
          }

        }
      });
    }

    // Parallelize over the reference observations
    in_parallel([&](size_t thread_idx, size_t num_threads) {

        std::vector<v2::ml_data_entry> x_t;
        std::vector<flexible_type> x_u;

        // declare the reference observation for all distance components
        std::vector<DenseVector> x(composite_distances.size());
        std::vector<SparseVector> x_sp(composite_distances.size());
        std::vector<std::vector<flexible_type>> x_flex(composite_distances.size());

        // resize the reference observation for each distance component
        for (size_t i = 0; i < composite_distances.size(); ++i) {
          size_t num_vars = metadata->num_dimensions();
          auto& c = composite_distances[i];

          if (c.row_sparsity == row_type::dense) {
            x[i] = DenseVector(num_vars);
            x_sp[i] = SparseVector(0);
            x_flex[i].resize(0);

          } else if(c.row_sparsity == row_type::flex_type) {
            x[i] = DenseVector(1);
            x_sp[i] = SparseVector(0);
            x_flex[i].resize(1); // there should only be a single column for string distances

          } else {
            x[i] = DenseVector(1); // TODO: Why doesn't 0 work?
            x_sp[i] = SparseVector(num_vars);
            x_flex[i].resize(0);
          }
        }

        // Loop over reference points (within each thread)
        for (auto it_ref = mld_ref.get_iterator(thread_idx, num_threads); !it_ref.done(); ++it_ref) {

          it_ref.fill_observation(x_t);
          it_ref.fill_untranslated_values(x_u);
          std::vector<double> block_dists(block_size, 0);

          // Loop over distance components
          for (size_t i = 0; i < composite_distances.size(); ++i) {
            const auto& c = composite_distances[i];

            // Compute the current distance component for each query point in
            // the memory block, conditional on data type
            if (c.row_sparsity == row_type::dense) {
              c.slicer.slice(x[i], x_t, x_u);

              for (size_t idx_query = 0; idx_query < block_size; ++idx_query) {
                block_dists[idx_query] += c.weight * c.distance->distance(
                  x[i], mld_queries_memory[i][idx_query]);
              }

            } else if (c.row_sparsity == row_type::flex_type) {
              c.slicer.slice(x_flex[i], x_t, x_u);

              for (size_t idx_query = 0; idx_query < block_size; ++idx_query) {
                block_dists[idx_query] += c.weight * c.distance->distance(
                  x_flex[i][0].get<std::string>(),
                  mld_queries_memory_flex[i][idx_query][0].get<std::string>());
              }

            } else {
              DASSERT_LT(i, x_sp.size());
              c.slicer.slice(x_sp[i], x_t, x_u);

              for (size_t idx_query = 0; idx_query < block_size; ++idx_query) {
                block_dists[idx_query] += c.weight * c.distance->distance(
                  x_sp[i], mld_queries_memory_sp[i][idx_query]);
              }
            }
          }


          // For each query in the block, evaluate it as a candidate neighbor
          // for the current reference point.
          for (size_t idx_query = 0; idx_query < block_size; ++idx_query) {
            neighbors[block_start + idx_query].evaluate_point(
                  std::pair<double, size_t>(block_dists[idx_query], it_ref.row_index()));
          }

          size_t n_pairs_so_far = (n_pairs += block_size);
          size_t n_query_points_so_far = n_pairs_so_far / mld_ref.size();
          table.print_timed_progress_row( n_query_points_so_far,
                                          n_pairs_so_far,
                                          (4 * 100.0 * n_pairs_so_far) / (num_pairs_total * 4.0),
                                          progress_time());

          if (cppipc::must_cancel()) {
             log_and_throw(std::string("Toolkit cancelled by user."));
          }
        }
      });// End of refs-loop for a single query block
  } // Query block loop

  table.print_row("Done", " ", 100.0, progress_time());
  table.print_footer();
}


/**
* Turi Serialization Save
*/
void brute_force_neighbors::save_impl(turi::oarchive& oarc) const {

  variant_deep_save(state, oarc);

  std::map<std::string, variant_type> data;

  data["is_dense"] = to_variant(is_dense);

  variant_deep_save(data, oarc);

  oarc << options
       << mld_ref
       << composite_params
       << untranslated_cols
       << reference_labels;
}


/**
 * Turi Serialization Load
 */
void brute_force_neighbors::load_version(turi::iarchive& iarc, size_t version) {

  ASSERT_MSG((version == 0) || (version == 1) || (version == 2),
             "This model version cannot be loaded. Please re-save your model.");


  variant_deep_load(state, iarc);
  std::map<std::string, variant_type> data;
  variant_deep_load(data, iarc);

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));

  __EXTRACT(is_dense);

#undef __EXTRACT

  iarc >> options;

  iarc >> mld_ref;
  metadata = mld_ref.metadata();


  if (version == 0) {
    // manually construct a single component that assumes a single distance across
    // all features.
    auto fn = function_closure_info();
    fn.native_fn_name = "_distances.";
    fn.native_fn_name += std::string(options.value("distance"));
    std::vector<std::string> features = variant_get_value<std::vector<std::string>>(state["features"]);
    dist_component_type p = std::make_tuple(features, fn, 1.0);
    composite_params = {p};

    // set empty untranslated columns for string features.
    untranslated_cols = {};
  }

  else {
    iarc >> composite_params;
    iarc >> untranslated_cols;
  }


  // construct the reference labels from the target column of the reference
  // ml_data.
  if (version < 2) {
    reference_labels.resize(mld_ref.size());

    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = mld_ref.get_iterator(); !it.done(); ++it) {
        reference_labels[it.row_index()] = metadata->target_indexer()->map_index_to_value(it.target_index());
      }
    });

    add_or_update_state({ {"num_distance_components", 1} });
  }

  else {
    iarc >> reference_labels;
  }

  num_examples = variant_get_value<size_t>(state["num_examples"]);
  initialize_distances();
}


}  // namespace nearest_neighbors
}  // namespace turi
