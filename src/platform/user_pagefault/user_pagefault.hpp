/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef TURI_USER_PAGEFAULT_H_
#define TURI_USER_PAGEFAULT_H_

#include <util/dense_bitset.hpp>
#include <parallel/pthread_tools.hpp>

namespace turi {
/**
 * \defgroup pagefault User Mode Page Fault Handler
 */

/**
 * \ingroup pagefault
 * \{
 */

/**
 * This implements a user mode page fault handler.
 *
 * The basic mechanics of operation are not very complicated.
 * We first install a segfault_handler.
 *
 * When you ask for some memory, we use mmap to allocate a region, but set
 * memory protection on it to PROT_NONE (disable both read and writes to the 
 * region). This way, every memory access to the region will trigger a segfault.
 *
 * When the memory is accessed, the segfault handler (segv_handler()) is
 * triggered, and try to fill in the data in the page.
 * To do so, (fill_pages()) we set the protection on the page to 
 * PROT_READ | PROT_WRITE (enable read + write), call a callback function to
 * fill in the data, then sets the protection on the page to PROT_READ. Then we
 * return from the segfault handler which allows the program to resume
 * execution correctly (but cannot write to the memory, only read).
 *
 * We keep a queue of pages we have committed (add_to_access_queue()). and when
 * the queue size becomes too large, we start evicting/decommitting pages
 * (handle_eviction()). Essentially we are doing FIFO caching.  To "evict"
 * a set of pages simply involves calling madvise(MADV_DONTNEED) on the pages.
 *
 * A bunch of additional maintenance stuff are needed:
 *  - a queue of committed pages (used to manage eviction)
 *  - The set of all regions (page_sets) allocated, sorted by address
 *    to permit fast binary searches).  this is used to figure out the mapping
 *    between address and callback.
 *
 * One key design consideration is the careful use of mmap. The way the 
 * procedure is defined above only calls mmap once for the entire region. 
 * decommitting and committing memory is done by use of memory protection
 * and madvise. Alternate implementation built entirely around mmap is possible
 * where we use completely unallocated memory addresses, and use mmap to map
 * the pages in as required. But this requires a large number of calls to mmap,
 * which uses up a lot of kernel datastructures, which will cause mmap to fail
 * with ENOMEM after a while.
 *
 * Also, with regards to parallelism, the kernel API does not quite provide
 * sufficient capability to handle parallel accesses correctly. What is missing
 * is an atomic "fill and enable page read/write" function. Essentially, while 
 * the callback function is filling in the pages for a particular memory address,
 * some other thread can read from the same memory addresses and get erroneous 
 * values. mremap could in theory be used to enable this by having the callback
 * fill to alternate pages, and using mremap to atomically swap those pages in.
 * However, see the design consideration above regarding excessive use of mmap.
 * Also, mremap is not available on OS X.
 */
namespace user_pagefault {

struct userpf_page_set;

/**
 *  Page filling callback
 */
typedef std::function<size_t (userpf_page_set* pageset,
                              char* address, 
                              size_t fill_length)> userpf_handler_callback;


/**
 *  Page release callback
 */
struct userpf_page_set;
typedef std::function<size_t (userpf_page_set* pageset)> userpf_release_callback;

/**
 * The structure used to maintain all the metadata about the pagefault handler.
 */
struct userpf_page_set {
  char* begin = nullptr; ///< The start of the managed address
  char* end = nullptr;   ///< The end of the manged address
  size_t length = 0;     ///< end - start
  /**
   * The number of "pages". Each pagefault will trigger the fill of one
   * "page". The size of the page is predefined at compile time.
   * \ref PAGE_SIZE.
   */
  size_t num_large_pages = 0; 

  bool writable = false;
                              
  /**
   * Internal datastructures. Should not touch.
   */
  /// one lock for each page to handle at least, a limited amount of parallelism
  std::vector<simple_spinlock> locks;

  /// one bit for each page that is resident (has physical pages associated)
  dense_bitset resident;
  /// one bit for each page that is dirty (written to but not flushed)
  dense_bitset dirty;
  /**
   * one bit for each page that is maintained by the pagefault handler's
   * pagefile instead of using the callback. Basically these pages
   * are pages which have been written to, and then evicted.
   */
  dense_bitset pagefile_maintained;

  /**
   * the callback handler which is called to fill in new page
   */
  userpf_handler_callback fill_callback;
  /**
   * the callback handler which is called when the memory is freed.
   * (this allows an external procedure to potentially persist the data)
   * This must be reentrant with the filling callback handler since the release
   * callback may read the entire memory region (which required filling)
   */
  userpf_release_callback release_callback;

  /**
   * pagefile_allocation[i] is the handle to the pagefile storing the disk 
   * backed memory. (size_t)(-1) if not allocated.
   */
  std::vector<size_t> pagefile_allocations;

};

/**
 * The page size we operate at. see user_pagefault.cpp for the actual value.
 * generally, we want to avoid working at the granularity of single system 
 * pages. While that is going to be faster for random access, that will trigger
 * a very large number of segfaults and will also require quite a lot of kernel
 * datastructures.
 */
extern const size_t TURI_PAGE_SIZE;



/**
 * Initializes the on-demand paging handlers. 
 *
 * \ref max_resident_memory The maximum amount of resident memory to be used
 * before memory is decommited. If not provided (-1), the environment variable
 * TURI_DEFAULT_PAGEFAULT_RESIDENT_LIMIT is read. If not available a
 * default value corresponding to half of total memory size is used.
 *
 * Returns true if the pagefault handler was installed successfully, 
 * and false otherwise.
 */
bool setup_pagefault_handler(size_t max_resident_memory = (size_t)(-1));

/**
 * Returns true if the pagefault handler is installed
 */
bool is_pagefault_handler_installed();

/**
 * Returns the maximum amount of resident memory to be used before memory
 * is decommitted. \see setup_pagefault_handler
 */
size_t get_max_resident();

/**
 * Sets the maximum amount of resident memory to be used before memory
 * is decommitted. \see setup_pagefault_handler
 */
void set_max_resident(size_t max_resident_memory);

/**
 * Allocates a block of memory of a certain length, where the contents of 
 * the memory are to be filled using the specified callback function.
 *
 * Returns a page_set pointer where page_set->begin is the memory address
 * at which on-demand paging will be performed. The callback is triggered 
 * to fill in the pages on demand.
 *
 * The pagefault handler is an std::function object of the form:
 * \code
 * size_t fill_callback(userpf_page_set* ps,
 *                      char* address, 
 *                      size_t fill_length) {
 *   // MUST fill in the contents of address to address + fill_length
 *   // most not write out of the bounds of [address, address+fill_length)
 *   // for instance, to fill in the array such that
 *   // arr = (size_t*)(ps->begin)
 *   // arr[i] == i
 *   // we do the following:
 *
 *    size_t* root = (size_t*) ps->begin;
 *    size_t* s_addr  = (size_t*) page_address;
 *
 *    size_t begin_value = s_addr - root;
 *    size_t num_to_fill = minimum_fill_length / sizeof(size_t);
 *
 *    for (size_t i = 0; i < num_to_fill; ++i) {
 *      s_addr[i] = i + begin_value;
 *    }
 * }
 * \endcode
 *
 * The release_callback is an std::function object of the form:
 * \code
 * size_t release_callback(userpf_page_set* ps) {
 *   // for instance, we may write out the contents of 
 *   // ps->begin to ps->end to disk here.
 * }
 * \endcode
 * The release_callback function *must* be reentrant with the fill_callback
 * function. This is because memory accesses performed by release_callback into
 * to the pointer may trigger the fill_callback to fill in the memory regions.
 *
 * If writable is set (default false), the user_pagefault handler will perform
 * automatic read/write/eviction.
 *
 * \param length The number of bytes to allocate
 * \param fill_callback The callback to be triggered when a page range needs 
 *                      to be filled.
 * \param release_callback The callback to be triggered the the memory is
 *                         released, i.e. release(pf) is called.
 * \param writable If the page range is to be writable. The pagefault handler
 *                 will handle paging.
 * The returned object MUST NOT be freed or deallocated. use \ref release.
 */
userpf_page_set* allocate(size_t length, 
                          userpf_handler_callback fill_callback,
                          userpf_release_callback release_callback = nullptr,
                          bool writable = true);

/**
 * Releases the pageset. The caller must ensure that there are no other 
 * memory accesses for this allocation.
 */
void release(userpf_page_set* pageset);
/**
 * Disables the on-demand paging handlers.
 *
 * Releases the pagefault handler, reverting it to the previous handler if any.
 * Returns true if the pagefault handler was disabled successfully, 
 * false otherwise.
 */
bool revert_pagefault_handler();

/**
 * Returns the number of allocations made 
 */
size_t get_num_allocations();

/**
 * Retursn the total number of bytes allocated to the pagefile
 */
size_t pagefile_total_allocated_bytes();

/**
 * Returns the total number of bytes actually stored in the pagefile
 * (after compression and stuff)
 */
size_t pagefile_total_stored_bytes();


/**
 * Returns the total number of bytes stored compressed.
 */
double pagefile_compression_ratio();
} // user_pagefault
/// \}
} // turicreate
#endif
