/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GL_NUMPY_ALT_MALLOC_HPP
#define TURI_GL_NUMPY_ALT_MALLOC_HPP
#include <cstddef>
extern "C" {
void* my_malloc(size_t size);
void my_free(void *ptr);
void* my_calloc(size_t nmemb, size_t size);
void* my_realloc(void *ptr, size_t size);



/**
 * Sets the maximum resident memory limit for the user pagefault handler
 */
void set_memory_limit(size_t n);

/**
 * Gets the maximum resident memory limit for the user pagefault handler
 */
size_t get_memory_limit();

/**
 * Returns the total number of allocated bytes (from calls to malloc)
 * managed by us.
 */
size_t pagefile_total_allocated_bytes();

/**
 * Returns the total number of compressed bytes we are currently managing in 
 * the pagefile
 */
size_t pagefile_total_stored_bytes();

/**
 * Returns the total number of compressed bytes we are currently managing in 
 * the pagefile
 */
size_t pagefile_compression_ratio();


/**
 * Returns true if malloc injection was successful: i.e. my_malloc
 * was called at least once. For this test to be correct,
 * malloc injection has to be attempted, and a numpy array created 
 * (to try to trigger our malloc implementation). Then 
 * malloc_injection_successful() can be tested.
 */
bool malloc_injection_successful();
}
#endif
