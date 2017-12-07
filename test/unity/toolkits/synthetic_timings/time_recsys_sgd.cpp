/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include "time_recsys_model.hpp"

using namespace turi;
using namespace turi::recsys;

int main(int argc, char **argv) {

  if(argc < 5 || argc > 6) {
    std::cerr << "Call format: " << argv[0] << " {lm/mf/fm} <n_users> <n_items> <n_observations> [num_factors]" << std::endl;
    exit(1);
  }

  std::string model(argv[1]);

  if(model != "fm" && model != "rfm") {
    std::cerr << "Call format: " << argv[0] << " {lm/mf/fm} <n_users> <n_items> <n_observations> [num_factors]" << std::endl;
    std::cerr << model << " not fm or mf" << std::endl;

    exit(1);
  }

  flexible_type num_factors = 8;

  size_t n_users = flexible_type(argv[2]);
  size_t n_items = flexible_type(argv[3]);
  size_t n_observations = flexible_type(argv[4]);

  // Set up the data
  std::map<std::string, flexible_type> data_gen_options = {
    {"random_seed", 0},
    {"num_factors", num_factors},
    {"y_mode", "squared_error"} };

  // Set up the data
  std::map<std::string, flexible_type> model_options;

  if(model != "lm")
    model_options["num_factors"] = num_factors;

  if(model == "fm")
    do_timing_run<recsys::recsys_factorization_model>(
        n_users, n_items, n_observations, data_gen_options, model_options);
  else if(model == "rfm")
    do_timing_run<recsys::recsys_ranking_factorization_model>(
        n_users, n_items, n_observations, data_gen_options, model_options);


  return 0;
}
