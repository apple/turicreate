/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <so_utils/so_utils.hpp>
#include <dlfcn.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#else
#include <link.h>
#endif


namespace turi {

namespace so_util {


#ifdef __APPLE__
so_handle open_shared_library(const std::string& path) {
  so_handle ret;
  void* handle_ptr = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (handle_ptr == NULL) {
    std::string err (dlerror());
    log_and_throw(std::string("Cannot load shared library. Path: ") + path + " Error: " + err);
  }
  ret = {path, handle_ptr, NULL};
  size_t size = _dyld_image_count();
  for (size_t i = 0; i < size; ++i) {
    if(strcmp(_dyld_get_image_name(i), ret.path.c_str()) == 0) {
      ret.base_ptr = (void*)(_dyld_get_image_header(i));
    }
  }
  if (ret.base_ptr == NULL) {
    log_and_throw(std::string("Cannot get base address of the shared library. Path: ") + path);
  }
  return ret;
}
#else
/**
 * \internal
 *
 * Callback function passed to dl_iterate_phdr.
 *
 * \param data is type so_handle*, pointer to the target so_handle which
 * we want to set its base_address.
 *
 * dl_iterate_phdr will iterate over all opened shared library, find
 * one with dlpi_name equal to the so_handle.path and set the handle's base_ptr
 * to dlpi_addr.
 */
int set_base_addr_callback(struct dl_phdr_info *info, size_t size, void *data) {
  so_handle* handle = (so_handle*)(data);
  if (strcmp(info->dlpi_name, handle->path.c_str()) == 0) {
    handle->base_ptr = (void*)info->dlpi_addr;
  }
  return 0;
}

so_handle open_shared_library(const std::string& path) {
  so_handle ret;
  void* handle_ptr = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (handle_ptr == NULL) {
    std::string err (dlerror());
    log_and_throw(std::string("Cannot load shared library. Path: ") + path + " Error: " + err);
  }
  ret = {path, handle_ptr, NULL};
  dl_iterate_phdr(set_base_addr_callback, &ret);
  if (ret.base_ptr == NULL) {
    log_and_throw(std::string("Cannot get base address of the shared library. Path: ") + path);
  }
  return ret;
}
#endif

void close_shared_library(const so_handle& so) {
  if (dlclose(so.handle_ptr) != 0) {
    std::string err (dlerror());
    log_and_throw(std::string("Cannot close shared library. Path: ") + so.path+ " Error: " + err);
  }
}

size_t get_function_offset(const so_handle& so, const char* function_symbol){
  Dl_info info;
  void* fptr = dlsym(so.handle_ptr, function_symbol);
  if (fptr == NULL) {
    log_and_throw(std::string("Cannot find function ") + function_symbol);
  }

  // check dladdr return. dladdr returns 0 when failed
  int success = dladdr((void*)(fptr), &info);
  if (!success) {
    log_and_throw(std::string("dladdr failed. ") + dlerror());
  }

  ASSERT_TRUE((char*)so.base_ptr == (char*)info.dli_fbase);
  ASSERT_TRUE(info.dli_sname != NULL && info.dli_saddr != NULL);
  ASSERT_TRUE((char*)fptr == (char*)info.dli_saddr);

  return (char*)(info.dli_saddr) - (char*)(info.dli_fbase);
}

void* get_function_from_offset(const so_handle& so, size_t offset) {
  Dl_info info;
  void* fptr = (char*)so.base_ptr + offset;

  // Verify the function pointer is a valid symbol address.
  // We do this by lookup the address of the assumed pointer using dladdr.
  int success = dladdr((void*)(fptr), &info);
  if (!success) {
    log_and_throw(std::string("dladdr failed. ") + dlerror());
  }

  ASSERT_TRUE((char*)so.base_ptr == (char*)info.dli_fbase);

  // Function pointer may not have symbol, e.g. anonymous function (lambda)
  bool has_symbol = info.dli_saddr != NULL;
  // throw if pointer lookup succeed but different from query pointer.
  if (has_symbol && (char*)fptr != (char*)info.dli_saddr) {
    log_and_throw(std::string("Invalid function offset"));
  };
  return fptr;
}

}
}
