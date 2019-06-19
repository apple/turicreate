/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DISTRIBUTED_SO_UTILS
#define TURI_DISTRIBUTED_SO_UTILS

#include<string>
#include<core/logging/logger.hpp>
#include<core/logging/assertions.hpp>

namespace turi {
/**
 * \defgroup soutils Shared Library Utilities
 * Mac/Linux Shared Library manipulation utilities
 */

/**
 * \ingroup soutils
 * Wrapping of dynamic library syscalls
 */
namespace so_util {

struct so_handle {
  // absolute path on the local filesystem to the shared library file
  std::string path;
  // the handle pointer returned by dlopen
  void* handle_ptr;
  // the base address at which the shared library is loaded.
  void* base_ptr;
};

/**
 * Try dlopen a shared library and return a so_handle.
 *
 * The path must be absolute path on the local filesystem.
 *
 * Raise std::string exception if it cannot be loaded.
 */
so_handle open_shared_library(const std::string& path);

/**
 * Try dlclose a shared library.
 *
 * Raise std::string exception if it cannot be closed.
 */
void close_shared_library(const so_handle& so);

/**
 * Returns the offset from the function_symbol to the base address of the shared library.
 *
 * function_symbol must be the mangled version.
 *
 * Raise std::string exception if symbol cannot be found or library does not contain
 * the reference function.
 */
size_t get_function_offset(const so_handle& so, const char* function_symbol);


/**
 * Returns the function pointer from given so_handle and offset.
 *
 * The so.base_ptr + offset must point to a valid symbol address.
 *
 * Raise std::string exception on error.
 */
void* get_function_from_offset(const so_handle& so, size_t offset);

}
}
#endif
