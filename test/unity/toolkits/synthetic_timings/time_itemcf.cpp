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

  if(argc < 4) {
    std::cerr << "Call format: " << argv[0] << " <n_users> <n_items> <n_observations> [mode]" << std::endl;
    exit(1);
  }

  size_t n_users = flexible_type(argv[1]);
  size_t n_items = flexible_type(argv[2]);
  size_t n_observations = flexible_type(argv[3]);

  std::string method = "jaccard";

  if(argc == 5) {
    method = argv[4];
  }

  // Set up the data
  std::map<std::string, flexible_type> data_gen_options = {
    {"random_seed", 0},
    {"y_mode", "squared_error"} };

  // Set up the data
  std::map<std::string, flexible_type> model_options = {
    {"similarity_type", method} };

  do_timing_run<recsys::recsys_itemcf>(n_users, n_items, n_observations, data_gen_options, model_options);

  return 0;
}
