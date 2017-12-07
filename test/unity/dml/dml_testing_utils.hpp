/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <distributed/distributed_context.hpp>
#include <distributed/cluster_interface.hpp>

namespace turi {
namespace dml_testing_utils {

  using namespace turi::dml;

  /**
   * Setup the distribute context, and return the distributed exec function
   * in the shared library.
   */
  std::function<std::string(std::string)> get_distributed_function(
      std::string function_name, 
      size_t num_workers = 1,
      std::string shared_lib="./dml_toolkits.so") {

    // In-proc cluster.
    auto cluster = make_local_inproc_cluster(num_workers);
    cluster->start();

    // Get distributed context.
    create_distributed_context(cluster);
    auto& ctx = get_distributed_context();

    // Load the shared library.
    ctx.register_shared_library(shared_lib.c_str());
    auto lib_handle = dlopen(shared_lib.c_str(), RTLD_LOCAL | RTLD_NOW);
    auto exec_fun = (std::string (*)(std::string)) dlsym(lib_handle, function_name.c_str());
    std::function<std::string(std::string)> std_exec_fun = [=](std::string args) {
      return exec_fun(args);
    };
    return std_exec_fun;
  }

} // namespace dml_testing_util
} // namespace turi
