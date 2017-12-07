/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ios>
#include <memory>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <export.hpp>
#include <parallel/pthread_tools.hpp>
#include <serialization/dir_archive.hpp>
#include <user_pagefault/user_pagefault.hpp>
#include <gl_numpy/memory_mapped_sframe.hpp>
#include <sframe/sframe.hpp>

using namespace turi;

struct alloc_metadata {
  bool system_malloc_managed = true;
  size_t allocated_size = 0;
  // if this is not system malloc managed, there are 2 cases
  //  - it is either something acquired from a large call to malloc,
  //    in which case pageset is set
  //  - otherwise it is acquired from a memory mapped sframe
  //
  //  why are these regular pointers?
  //  - The pageset is managed by the pagefault handler. it should never be 
  //  deleted directly.
  //  - mm_sframe its "ok" to leak. essentially if the user never calls free
  // on us, this will leak and we will then leak. If we try to clean this
  // up on process destruction it may end up causing more complicated 
  // issues due to termination ordering.
  user_pagefault::userpf_page_set* pageset = nullptr; 
  gl_numpy::memory_mapped_sframe* mm_sframe = nullptr;
};
static turi::mutex lock;
static std::unordered_map<void*, std::shared_ptr<alloc_metadata> > addresses;
static bool malloc_called = false;

/**
 * Everything below this limit is going to go to regular malloc
 */
const size_t LOWER_LIMIT = 16*1024*1024; /*16 MB*/

/*
 * Does nothing. For mallocs and stuff like that zero filled pages are fine.
 */
size_t noop_callback(user_pagefault::userpf_page_set* pageset,
                     char* address, 
                     size_t fill_length) { return 0;}


extern "C" {

void* my_malloc(size_t size) {
  user_pagefault::setup_pagefault_handler();
  malloc_called = true;
  if (size < LOWER_LIMIT) {
    void* retptr = malloc(size);
    if (retptr == nullptr) return nullptr;
    else {
      // add to the registry
      alloc_metadata metadata;
      metadata.system_malloc_managed = true;
      metadata.allocated_size = size;
      std::lock_guard<turi::mutex> guard(lock);
      addresses[retptr] = std::make_shared<alloc_metadata>(metadata);
      return retptr;
    }
  } else {
    auto pageset = user_pagefault::allocate(size,
                                          noop_callback, // noop data fill
                                          nullptr, // no release callback
                                          true); // writable
    // allocation failed
    if (pageset == nullptr) {
      // std::cout << "Malloc of size " << size << " Failed " << std::endl;
      return nullptr;
    }
    alloc_metadata metadata;
    metadata.system_malloc_managed = false;
    metadata.allocated_size = size;
    metadata.pageset = pageset;
    std::lock_guard<turi::mutex> guard(lock);
    addresses[pageset->begin] = std::make_shared<alloc_metadata>(metadata);
    /*
     * std::cout << "Malloc of size " << size << " at " 
     *           << (void*)(pageset->begin) << std::endl;
     */
    return pageset->begin;
  }
}

void my_free(void *ptr) {
  std::shared_ptr<alloc_metadata> metadata;
  // look in the address map and remove from it
  {
    std::lock_guard<turi::mutex> guard(lock);
    auto iter = addresses.find(ptr);
    if (iter == addresses.end()) {
      // we don't know this address!
      // redirect to system free
      free(ptr);
      return;
    }
    metadata = iter->second;
    addresses.erase(iter);
  }
  // ok now to actually release the contents of the memory
  if (metadata->system_malloc_managed) {
    free(ptr);
  } else if (metadata->pageset) {
    // std::cout << "Free of managed pointer " << ptr << std::endl;
    user_pagefault::release(metadata->pageset);
  } else if (metadata->mm_sframe) {
    // std::cout << "Free of SFrame mapping " << ptr << std::endl;
    delete metadata->mm_sframe;
  }
}


void* my_calloc(size_t nmemb, size_t size) {
  malloc_called = true;
  if (nmemb * size < LOWER_LIMIT) {
    return calloc(nmemb, size);
  } else {
    return my_malloc(nmemb * size);
  }
}

void* my_realloc(void *ptr, size_t size) {
  std::shared_ptr<alloc_metadata> metadata;
  {
    std::lock_guard<turi::mutex> guard(lock);
    auto iter = addresses.find(ptr);
    if (iter == addresses.end()) {
      // we don't know this address!
      // redirect to system realloc
      return realloc(ptr, size);
    }
    metadata = iter->second;
  }

  // if new size is still small and the original pointer is system
  // managed, we just call the system malloc
  if (size < LOWER_LIMIT && metadata->system_malloc_managed) {
    void* retptr = realloc(ptr, size);
    if (retptr != nullptr) {
      // did the returned pointer move?
      metadata->allocated_size = size;
      if (retptr != ptr) {
        // yup. we need to update the address map
        std::lock_guard<turi::mutex> guard(lock);
        addresses.erase(ptr);
        addresses[retptr] = metadata;
      }
    } 
    return retptr;
  } else {
    // in all other cases things are slightly more annoying
    // malloc the new size
    void* newptr = my_malloc(size);
    if (newptr == nullptr) return nullptr;
    // copy everything over
    memcpy(newptr, ptr, metadata->allocated_size);
    // release old pointer
    my_free(ptr);
    return newptr;
  }
}


EXPORT size_t pointer_length(void* ptr) {
  std::lock_guard<turi::mutex> guard(lock);
  auto iter = addresses.find(ptr);
  if (iter != addresses.end()) {
    return iter->second->allocated_size;
  } else {
    // we don't know this address!
    return 0;
  }
}

EXPORT void* pointer_from_sframe(const char* directory, bool delete_on_close) {
  try {
    gl_numpy::memory_mapped_sframe* msf = new gl_numpy::memory_mapped_sframe();
    // load failed for whatever reason return failure

    turi::dir_archive dir;
    dir.open_directory_for_read(directory);
    std::string content_value;
    if (dir.get_metadata("contents", content_value) && 
        content_value == "sframe") {

      std::string prefix = dir.get_next_read_prefix();
      turi::sframe sf(prefix + ".frame_idx");
      if (msf->load(sf) == false) {
        delete msf;
        return nullptr;
      }
      if (delete_on_close) {
        msf->recursive_delete_on_close(directory);
      }
      auto metadata = std::make_shared<alloc_metadata>();
      metadata->system_malloc_managed = false;
      metadata->allocated_size = msf->length_in_bytes();
      metadata->mm_sframe = msf;

      std::lock_guard<turi::mutex> guard(lock);
      addresses[msf->get_pointer()] = metadata;
      return msf->get_pointer();
    } else {
      std::cout << "Invalid object. Expecting an SFrame in " << directory << "\n";
    }
  } catch (const std::string& err) {
    std::cout << err << "\n";
  } catch (const char* err) {
    std::cout << err << "\n";
  } catch (std::exception& err){ 
    std::cout << err.what() << "\n";
  } catch (...) { 
    std::cout << "Unknown error\n";
  }
  return nullptr;
}


EXPORT void set_memory_limit(size_t n) {
  user_pagefault::set_max_resident(n);
}

EXPORT size_t get_memory_limit() {
  return user_pagefault::get_max_resident();
}

EXPORT size_t pagefile_total_allocated_bytes() {
  return user_pagefault::pagefile_total_allocated_bytes();
}

EXPORT size_t pagefile_total_stored_bytes() {
  return user_pagefault::pagefile_total_stored_bytes();
}

EXPORT size_t pagefile_compression_ratio() {
  return user_pagefault::pagefile_compression_ratio();
}


EXPORT bool malloc_injection_successful() {
  return malloc_called;
}
}
