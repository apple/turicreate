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
#include <toolkits/nearest_neighbors/lsh_neighbors.hpp>
#include <toolkits/nearest_neighbors/hash_map_container.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

// Miscellaneous
#include <timer/timer.hpp>
#include <algorithm>
#include <time.h>
#include <string>
#include <Eigen/Core>
#include <model_server/lib/toolkit_util.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <core/util/cityhash_tc.hpp>

namespace turi {
namespace nearest_neighbors {

// TODO: can it be set by the available RAM??
#ifdef NDEBUG
const size_t LSH_NEAREST_NEIGHBORS_BIG_DATA = 1024 * 1024 * (1024 / 8) * 2; // note overflow
#else
// Small enough that the test datasets force multiple blocks to be tested
const size_t LSH_NEAREST_NEIGHBORS_BIG_DATA = 1000;
#endif


/**
 * Destructor. Make sure bad things don't happen
 */
lsh_neighbors::~lsh_neighbors(){

}

/**
* Set options
*/
void lsh_neighbors::init_options(const std::map<std::string,
                                         flexible_type>& _options) {

  options.create_integer_option("num_tables",
                            "number of hash tables for LSH",
                            20,
                            1,
                            std::numeric_limits<int>::max(),
                            true);

  options.create_integer_option("num_projections_per_table",
                            "number of projections in each hash table",
                            8,
                            1,
                            std::numeric_limits<int>::max(),
                            true);

  options.create_string_option("label",
                             "Name of the reference dataset column with row labels.",
                             "",
                             false);

  // Set options and update model state with final option values
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));

}


/**
 * Train a LSH nearest neighbors model.
 *
 */
void lsh_neighbors::train(const sframe& X, const std::vector<flexible_type>& ref_labels,
                          const std::vector<dist_component_type>& composite_distance_params,
                          const std::map<std::string, flexible_type>& opts) {

  logprogress_stream << "Starting LSH nearest neighbors model training." << std::endl;

  timer t;
  double start_time = t.current_time();

   // Validate the inputs.
  init_options(opts);
  validate_distance_components(composite_distance_params, X);

  // Create the ml_data object for the reference data.
  initialize_model_data(X, ref_labels);

  // Initialize the distance components. NOTE: this needs data to be initialized
  // first because the row slicers need the column indices to be sorted.
  initialize_distances();

  DASSERT_FALSE(composite_distances.empty());
  dist_component c = composite_distances[0];

  bool is_sparse = (mld_ref.max_row_size() < metadata->num_dimensions());

  std::string distance_name = std::get<1>(composite_distance_params[0]).native_fn_name;
  if (distance_name.find_first_of(".") != std::string::npos) {
    distance_name = distance_name.substr(distance_name.find_first_of(".") + 1,
                                         distance_name.size());
  }

  size_t num_tables = (size_t)options.value("num_tables");
  size_t num_projections_per_table = (size_t)options.value("num_projections_per_table");
  size_t num_dimensions = metadata->num_dimensions();
  size_t num_rows = X.num_rows();

  // TODO: better format
  logprogress_stream << "LSH Options: " << std::endl;
  logprogress_stream << "  Number of tables : " << num_tables << std::endl;
  logprogress_stream << "  Number of projections per table : " << num_projections_per_table << std::endl;

  table_printer table({ {"Rows Processed", 0}, {"\% Complete", 0},
                        {"Elapsed Time", 0}});

  table.print_header();

  // Initialize the lsh_model
  lsh_model = lsh_family::create_lsh_family(distance_name);
  lsh_model->init_options(options.current_option_values());
  lsh_model->init_model(num_dimensions);
  lsh_model->pre_lsh(mld_ref, is_sparse);

  turi::atomic<size_t> n_train_points = 0;

  in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {
    size_t ref_idx;

    DenseVector v(num_dimensions);
    SparseVector s(num_dimensions);

    for (auto it = mld_ref.get_iterator(thread_idx, num_threads);
           !it.done(); ++it) {

      ref_idx = it.row_index(); // reference id

      if (cppipc::must_cancel()) {
        log_and_throw("Toolkit canceled by user.");
      }

      if (!is_sparse) { // dense
        it.fill_observation(v);
        lsh_model->add_reference_data<DenseVector>(ref_idx, v);
      } else { //sparse
        it.fill_observation(s);
        lsh_model->add_reference_data<SparseVector>(ref_idx, s);
      }

      ++ n_train_points;
      size_t num_points_so_far = n_train_points;
      if (num_points_so_far > 0 && num_points_so_far % 10000 == 0) {
        table.print_row(num_points_so_far, (num_points_so_far * 100) / num_rows, progress_time());
      }
    }
  });
  table.print_row("Done", "100", progress_time());
  table.print_footer();

  // logprogress_stream << "Successfully trained LSH nearest neighbors model." << std::endl;
  add_or_update_state({ {"method", "lsh"},
                      {"training_time", t.current_time() - start_time} });
}


sframe lsh_neighbors::query(const v2::ml_data& mld_queries,
                            const std::vector<flexible_type>& query_labels,
                            const size_t k, const double radius,
                            const bool include_self_edges) const {

  DASSERT_FALSE(composite_distances.empty());
  dist_component c = composite_distances[0];

  bool is_sparse = (mld_ref.max_row_size() < metadata->num_dimensions());

  // Compute the actual number of nearest neighbors and construct the data
  // structures to hold candidate neighbors while reference points are searched
  size_t kstar;

  if (k == NONE_FLAG) {
    kstar = NONE_FLAG;
  } else {
    kstar = std::min(k, mld_ref.size());
  }

  // output
  sframe result;

  size_t num_dimensions = metadata->num_dimensions();

  //  key: reference_id
  //  value: a set of query ids that have the reference id as a candidate
  //
  // The reasons why building the hashtable by ref_id instead of query id:
  // 1) we read queries into memory.
  // 2) we scan over ref ids and push the real distance to the heaps of queries
  //
  // NOTE: this is optimized for large number of queries.
  hash_map_container<size_t, std::vector<size_t>> ref_to_check_map;

  size_t max_block_size = mld_queries.size();

  /////////////////////////////////////////////////////////////////////////
  // *roughly* calculate max_block_size using a subset
  //
  // estimated_overall_size = max_block_size * (average_non_zero_values_per_data
  // + average_num_candidates_per_data)
  //
  if (mld_queries.size() > 1000) {
    size_t num_samples = std::max(size_t(100), static_cast<size_t>(mld_queries.size() * 0.01));
    turi::atomic<size_t> num_nnz = 0;
    turi::atomic<size_t> num_candidates = 0;

    double average_nnz = static_cast<double>(num_dimensions);
    double average_candidates = 0.;

    v2::ml_data sampled_data = mld_queries.create_subsampled_copy(num_samples, time(NULL));
    if (!is_sparse) { // dense
      in_parallel([&](size_t thread_idx, size_t num_threads) {
        DenseVector vec(num_dimensions);
        std::vector<size_t> candidates;
        for (auto it = sampled_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
          it.fill_eigen_row(vec);
          candidates = lsh_model->query<DenseVector>(vec);
          num_candidates += candidates.size();
        }
      });
    } else { // sparse
      in_parallel([&](size_t thread_idx, size_t num_threads) {
        SparseVector vec(num_dimensions);
        std::vector<size_t> candidates;
        for (auto it = sampled_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
          it.fill_observation(vec);
          num_nnz += vec.nonZeros();
          candidates = lsh_model->query<SparseVector>(vec);
          num_candidates += candidates.size();
        }
      });
      average_nnz = num_nnz / static_cast<double>(num_samples) * 2; // * 2 because sparse_vec is stored as key-value pairs
    }
    average_candidates = num_candidates / static_cast<double>(num_samples);

    //logprogress_stream << "Average NNZ: " << average_nnz << std::endl;
    //logprogress_stream << "Average Candidates: " << average_candidates << std::endl;

    max_block_size = std::min(max_block_size, static_cast<size_t>(
              LSH_NEAREST_NEIGHBORS_BIG_DATA / ((average_candidates + average_nnz + kstar * 2))) + 1);

  }
  /////////////////////////////////////////////////////////////////////////////

  // logprogress_stream << "Max query block size : " << max_block_size << std::endl;

  size_t num_blocks = (mld_queries.size() - 1) / max_block_size + 1;

  logprogress_stream << "Queries are processed into " << num_blocks
      << " blocks." << std::endl;

  table_printer table({ {"Query points", 0}, {"\% Complete", 0},
                      {"Elapsed Time", 0}});
  table.print_header();

  // working on queries
  //
  // Queries are read in blocks

  DenseMatrix query_block_buff_dense;
  std::vector<SparseVector> query_block_buff_sparse;

  if (!is_sparse){
    query_block_buff_dense.setZero(max_block_size, num_dimensions);
  } else {
    query_block_buff_sparse.assign(max_block_size, SparseVector(num_dimensions));
  }

  turi::atomic<size_t> n_query_points = 0;

  for (size_t block_index = 0; block_index < num_blocks; ++block_index) {
    size_t block_start = (block_index * mld_queries.size()) / num_blocks;
    size_t block_end = ((block_index + 1) * mld_queries.size()) / num_blocks;
    size_t block_size = block_end - block_start;

    DASSERT_TRUE(block_size <= max_block_size);

    auto mld_queries_in_block = mld_queries.slice(block_start, block_end);

    // only keep topk nn of queries in the block
    std::vector<neighbor_candidates> topk_neighbors(block_size,
                    neighbor_candidates(-1, kstar, radius, include_self_edges));

    parallel_for (0, block_size, [&](size_t idx) {
                    topk_neighbors[idx].set_label(idx + block_start);
                  });

    if (cppipc::must_cancel()) {
      log_and_throw("Toolkit canceled by user.");
    }

    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {

      size_t idx_query;
      std::vector<size_t> candidates;

      for (auto it_query = mld_queries_in_block.get_iterator(thread_idx, num_threads);
           !it_query.done(); ++it_query) {

        if (cppipc::must_cancel()) {
          log_and_throw("Toolkit canceled by user.");
        }

        ASSERT_TRUE(it_query.target_index() != NONE_FLAG);

        idx_query = it_query.row_index();
        DASSERT_TRUE(idx_query < block_end - block_start);

        if (!is_sparse){
          it_query.fill_eigen_row(query_block_buff_dense.row(idx_query));
          candidates = lsh_model->query<DenseVector>(query_block_buff_dense.row(idx_query));
        } else {
          it_query.fill_observation(query_block_buff_sparse[idx_query]);
          candidates = lsh_model->query<SparseVector>(query_block_buff_sparse[idx_query]);
        }

        for (const auto& ref_id: candidates) {
          ref_to_check_map.update(ref_id, [idx_query](std::vector<size_t>& v) {
                                    v.push_back(idx_query);
                                  });
        }

        ++ n_query_points;
        size_t num_points_so_far = n_query_points;
        if (num_points_so_far > 0 && num_points_so_far % 10000 == 0) {
          table.print_row(num_points_so_far, (num_points_so_far * 100) / mld_queries.size(), progress_time());
        }
      }
    }); // finish candidates check

    // In the next step, scan over the reference sframe to do the real distance
    // check to get top k nn.
    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {

      size_t idx_ref;
      DenseVector ref_v(num_dimensions);
      SparseVector ref_s(num_dimensions);

      for(auto it_ref = mld_ref.get_iterator(thread_idx, num_threads);
        !it_ref.done(); ++it_ref) {

        if (cppipc::must_cancel()) {
          log_and_throw("Toolkit canceled by user.");
        }

        idx_ref = it_ref.row_index();

        const auto& to_check_set = ref_to_check_map.get(idx_ref);
        if (to_check_set.empty()) {
          continue;
        }

        if (!is_sparse) {
          it_ref.fill_observation(ref_v);
        } else {
          it_ref.fill_observation(ref_s);
        }

        for (const auto& idx_query : to_check_set) {
          double dist = 0;
          if (!is_sparse) {
            dist = c.distance->distance(
              ref_v, query_block_buff_dense.row(idx_query));
          } else {
            dist = c.distance->distance(
              ref_s, query_block_buff_sparse[idx_query]);
          }
          topk_neighbors[idx_query].evaluate_point(
              std::pair<double, size_t>(dist, idx_ref));
        }
      }
    });

    append_neighbors_to_sframe(result, topk_neighbors, reference_labels, query_labels);

    // clear the map
    ref_to_check_map.clear();

    table.print_row(block_end,
                    static_cast<double>(block_end) / mld_queries.num_rows() * 100,
                    progress_time());
  } // end block

  table.print_row("Done", 100.0, progress_time());
  table.print_footer();

  result.close();
  return result;
}


/**
* Turi Serialization Save
*/
void lsh_neighbors::save_impl(turi::oarchive& oarc) const {

  variant_deep_save(state, oarc);

  std::map<std::string, variant_type> data;
  data["is_dense"]      = to_variant(is_dense);

  variant_deep_save(data, oarc);
  oarc << lsh_model->distance_type_name()
      << *lsh_model;

  oarc << options
       << mld_ref
       << composite_params
       << untranslated_cols
       << reference_labels;
}


/**
 * Turi Serialization Load
 */
void lsh_neighbors::load_version(turi::iarchive& iarc, size_t version) {

  ASSERT_MSG((version == LSH_NEIGHBORS_VERSION) ||
             (version == LSH_NEIGHBORS_VERSION - 1),
             "This model version cannot be loaded. Please re-save your model.");

  variant_deep_load(state, iarc);

  std::map<std::string, variant_type> data;

  variant_deep_load(data, iarc);

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));
  __EXTRACT(is_dense);
#undef __EXTRACT

  std::string distance_type_name;

  iarc >> distance_type_name;
  lsh_model = lsh_family::create_lsh_family(distance_type_name);

  iarc >> *lsh_model;

  iarc >> options;
  iarc >> mld_ref;
  metadata = mld_ref.metadata();

  // there is no previous version of LSH
  if (version == 0) {
    log_and_throw("There is no available LSH model with version 0!");
  } else {
    iarc >> composite_params;
    iarc >> untranslated_cols;
    iarc >> reference_labels;
  }

  initialize_distances();
}


}  // namespace nearest_neighbors
}  // namespace turi
