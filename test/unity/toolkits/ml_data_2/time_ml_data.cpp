/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <random/random.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/testing_utils.hpp>
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <timer/timer.hpp>
#include <sframe/sframe.hpp>

using namespace turi;
using namespace turi::v2;

void run_benchmark(size_t n_obs, std::string column_type_info) {

  sframe data = make_random_sframe(n_obs, column_type_info);

  std::cout << "SFrame Built, beginning timings." << std::endl;
  std::cout << "Columns: " << column_type_info << "; num observations = " << n_obs << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1:  Time the data indexing.

  timer tt1;

  tt1.start();

  v2::ml_data mdata;
  mdata.fill(data);

  std::cerr << "Loading and indexing (" << column_type_info
            << "):                "
            << tt1.current_time_millis()
            << "ms." << std::endl;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2:  Time sequential iteration

  for(size_t attempt : {1, 2})
  {
    size_t common_value = 0;
    timer tt2;
    tt2.start();
    std::vector<v2::ml_data_entry> x;

    for(auto it = mdata.get_iterator(); !it.done(); ++it) {

      it.fill_observation(x);

      for(const auto& v : x)
        common_value += (v.column_index + v.index + (v.value != 0));
    }

    std::cerr << "Non-parallel Iteration, try " << attempt
              << ":            "
              << tt2.current_time_millis()
              << "ms." << std::endl;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3:  Time parallel iterations with the ml_data_entry vector.

  for(size_t attempt : {1, 2})
  {
    size_t common_value = 0;
    timer tt2;
    tt2.start();

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        std::vector<v2::ml_data_entry> x;

        for(auto it = mdata.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
          it.fill_observation(x);

          for(const auto& v : x)
            common_value += (v.column_index + v.index + (v.value != 0));
        }

      });

    std::cerr << "Parallel Iteration, try " << attempt
              << ", n_cpu = " << thread::cpu_count()
              << ":     "
              << tt2.current_time_millis()
              << "ms." << std::endl;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4:  Time parallel iterations with the Eigen sparse vector.

  for(size_t attempt : {1, 2})
  {
    timer tt2;
    tt2.start();

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        double cv = 0;
        Eigen::SparseVector<double> x;

        for(auto it = mdata.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
          it.fill_observation(x);
          cv += x.sum();
        }

      });

    std::cerr << "Parallel, SparseVector, try " << attempt
              << ", n_cpu = " << thread::cpu_count()  << ": "
              << tt2.current_time_millis()
              << "ms." << std::endl;
  }

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
