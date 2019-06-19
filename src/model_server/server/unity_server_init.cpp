/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/filesystem.hpp>
#include <model_server/lib/unity_global.hpp>
#include <model_server/lib/unity_global_singleton.hpp>
#include <model_server/server/unity_server_init.hpp>
#include <model_server/server/registration.hpp>
#include <core/storage/sframe_interface/unity_sarray.hpp>
#include <core/storage/sframe_interface/unity_sarray_builder.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/sframe_interface/unity_sframe_builder.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <ml/sketches/unity_sketch.hpp>
#include <model_server/lib/simple_model.hpp>
#
namespace turi {
unity_server_initializer::~unity_server_initializer() {}

void unity_server_initializer::init_toolkits(toolkit_function_registry& registry) const {
  register_functions(registry);
};

void unity_server_initializer::init_models(toolkit_class_registry& registry) const {
  register_models(registry);
};

void unity_server_initializer::init_extensions(
    const std::string& root_path_, std::shared_ptr<unity_global> unity_global_ptr) const {

  using namespace fileio;
  namespace fs = boost::filesystem;
  fs::path root_path(root_path_);
  // look for shared libraries I can load
  std::vector<fs::path> candidate_paths {root_path / "*.so",
                                               root_path / "*.dylib",
                                               root_path / "*.dll"};

  // we exclude all of our own libraries
  std::vector<fs::path> exclude_paths {root_path / "*libunity*.so",
                                       root_path / "*libunity*.dylib",
                                       root_path / "*libunity*.dll"};

  std::set<std::string> exclude_files;

  for (auto exclude_candidates: exclude_paths) {
    auto globres = get_glob_files(exclude_candidates.string());
    for (auto file : globres) exclude_files.insert(file.first);
  }

  for (auto candidates: candidate_paths) {
    for (auto file : get_glob_files(candidates.string())) {
      // exclude files in the exclusion list
      if (exclude_files.count(file.first)) {
        logstream(LOG_INFO) << "Excluding load of " << file.first << std::endl;
        continue;
      }
      // exclude libhdfs
      if (boost::ends_with(file.first, "libhdfs.so") ||
          boost::ends_with(file.first, "libhdfs.dylib") ||
          boost::ends_with(file.first, "hdfs.dll")) {
        continue;
}
      if (file.second == file_status::REGULAR_FILE) {
        logstream(LOG_INFO) << "Autoloading of " << file.first << std::endl;
        unity_global_ptr->load_toolkit(file.first, "..");
      }
    }
  }
}

} // end of namespace turi
