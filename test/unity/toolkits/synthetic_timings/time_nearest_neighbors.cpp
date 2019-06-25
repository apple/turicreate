/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <core/random/random.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/testing_utils.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/ball_tree_neighbors.hpp>
#include <toolkits/nearest_neighbors/brute_force_neighbors.hpp>
#include <timer/timer.hpp>
#include <core/storage/sframe_data/sframe.hpp>

using namespace turi;
using namespace turi::v2;

void run_benchmark(size_t n_obs, std::string column_type_info) {

  sframe data[3] = {make_random_sframe(n_obs, column_type_info),
                    make_random_sframe(n_obs / 2, column_type_info),
                    make_random_sframe(100, column_type_info)};

  sframe y[3];

  for(size_t i = 0; i < 3; ++i) {
    std::vector<std::vector<flexible_type> > labels(data[i].size());

    for(size_t j = 0; j < data[i].size(); ++j) {
      labels[j] = {std::to_string(hash64(i, j))};
    }
    y[i] = make_testing_sframe({"label"}, labels);
  }


  std::cout << "SFrame Built, beginning timings." << std::endl;
  std::cout << "Columns: " << column_type_info << "; num observations = " << n_obs << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1:  Time the data indexing.

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3:  Time parallel iterations with the ml_data_entry vector.

  std::shared_ptr<nearest_neighbors::nearest_neighbors_model> nn[2];

  nn[0].reset(new nearest_neighbors::ball_tree_neighbors);
  nn[1].reset(new nearest_neighbors::brute_force_neighbors);

  /// STUPID CLANG PARSING BUG
  typedef std::map<std::string, flexible_type> flex_map;
  flex_map nn_options;

  auto fn = function_closure_info();
  fn.native_fn_name = "_distances.euclidean";
  nearest_neighbors::dist_component_type p = std::make_tuple(data[0].column_names(), fn, 1.0);
  std::vector<nearest_neighbors::dist_component_type> composite_params = {p};

  std::ostringstream ss;


  for(size_t model : {0, 1} ) {

    ss << "###############################" << '\n';

    if(model == 0) {
      ss << "Ball Tree Neighbors" << '\n';
    } else {
      ss << "Brute Force Neighbors" << '\n';
    }

    {
      timer tt;
      tt.start();
      nn[model]->train(data[0], y[0], composite_params, nn_options);

      ss << "Training time, " << data[0].size() << " observations: "
                << tt.current_time_millis()
                << "ms." << '\n';
    }
    for(size_t query_idx : {2, 0, 1} ) {

      for(size_t k : {size_t(1), size_t(10), size_t(100) }) {

        timer tt;
        tt.start();
        nn[model]->query(data[query_idx], y[query_idx], k, -1);

        ss << "Query time, n="
                  << data[query_idx].size()
                  << ", k=" << k << ": "
                  << tt.current_time_millis()
                  << "ms." << '\n';
      }
    }
  }

  std::cerr << ss.str() << std::endl;
}

int main(int argc, char **argv) {

  if(argc == 1) {
    std::cerr << "Call format: " << argv[0] << " <n_observations> [type_string: [ncCsSvVuUdD]+] \n"
              << "n:  numeric column.\n"
              << "c:  categorical column with 100 categories.\n"
              << "C:  categorical column with 1000000 categories.\n"
              << "s:  categorical column with short string keys and 1000 categories.\n"
              << "S:  categorical column with short string keys and 100000 categories.\n"
              << "v:  numeric vector with 10 elements.\n"
              << "V:  numeric vector with 1000 elements.\n"
              << "u:  categorical set with 10 elements.\n"
              << "U:  categorical set with 1000 elements.\n"
              << "d:  dictionary with 10 entries.\n"
              << "D:  dictionary with 100 entries.\n"
              << "\n Example: " << argv[0] << " 100000 ccn -- benchmarks 100000 row sframe with 3 columns, 2 categorical and 1 numeric."
              << std::endl;

    exit(1);
  } else if (argc == 2) {
    size_t n_obs   = std::atoi(argv[1]);

    run_benchmark(n_obs, "cc");
    run_benchmark(n_obs, "ncsvd");
  } else if (argc == 3) {
    size_t n_obs   = std::atoi(argv[1]);

    std::string column_type_info(argv[2]);
    run_benchmark(n_obs, column_type_info);
  }

  return 0;
}
