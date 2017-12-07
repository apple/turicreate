/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <queue>
#include <cstdint>
#include <cstddef>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <cstdlib>
#include <user_pagefault/pagefile.hpp>
#include <minipsutil/minipsutil.h>
#include <random/random.hpp>
#include "user_pagefault.hpp"
namespace turi {
namespace user_pagefault {


static void handle_eviction();
bool pagefault_handler(void* addr);

static turi::mutex lock;
/**
 * All page sets allocated so far
 */
static std::vector<userpf_page_set*> all_page_sets;

/**
 * A queue of all on-demand allocated pages. used for selective decommit.
 * Each element in the queue is about one LARGE_PAGE
 */
static std::deque<std::tuple<userpf_page_set*, size_t, size_t> > access_queue;

/**
 * The maximum number of elements in the access queue before we start 
 * decommiting stuff.
 */
static size_t MAX_QUEUE_LENGTH = 128;

/**
 * We want to avoid working at the granularity of single pages.
 * This is the granularity we are going to operate at.
 */
const size_t TURI_PAGE_SIZE = 2 * 1024 * 1024;

static bool pagefault_handler_installed = false;

/**
 * External memory storage
 */
pagefile disk_pagefile;


/**
 * Finds the page set containing the address, or failing which,
 * returns the page set immediately after the address.
 * Note that the returned pageset can be all_page_sets.size() if the address
 * is after all the pagesets.
 *
 * Returns a pair of (page_set id, returned page set contains address)
 *
 * lock must be acquired prior to entry to this function.
 */
std::pair<int, bool> find_page_set(void* address, 
                                   int start = 0,
                                   int end = -1) {
  if (end == -1) end = all_page_sets.size();
  // yeah binary search.
  if (start >= end) return {start, false};

  int mid = (start + end) / 2;
  if (address < all_page_sets[mid]->begin) {
    // page is rght recurse left
    return find_page_set(address, start, mid);
  } else if (all_page_sets[mid]->begin <= address) {
    // page is right recurse right.
    if ((char*)address < 
        (char*)all_page_sets[mid]->begin + all_page_sets[mid]->length) {
      return {mid, true};
    } else {
      return find_page_set(address, mid + 1, end);
    }
  } else {
    return {mid, false};
  }
}

/**
 * Inserts the pageset into all pagesets.
 */
void insert_page_set(userpf_page_set* ps) {
  std::lock_guard<turi::mutex> guard(lock);
  auto iter = std::upper_bound(all_page_sets.begin(), 
                   all_page_sets.end(),
                   ps,
                   [](const userpf_page_set* left, const userpf_page_set* right) {
                    return left->begin < right->begin;
                   });
  all_page_sets.insert(iter, ps);
}

/**
 * Removes the pageset from all pagesets.
 */
bool remove_page_set(userpf_page_set* ps) {
  std::lock_guard<turi::mutex> guard(lock);
  auto location = find_page_set(ps->begin);
  if (location.second) {
    // page found
    all_page_sets.erase(all_page_sets.begin() + location.first);
  }
  // printf("Removing pageset: %p %p\n", ps, ps->begin);
  access_queue.erase(
      std::remove_if(access_queue.begin(),
                 access_queue.end(),
                 [&](const std::tuple<userpf_page_set*, size_t, size_t>& s)->bool {
                   return std::get<0>(s) == ps;
                 }), access_queue.end());
  return location.second;
}

/**
 * Finds a pageset containing the queried address.
 * If not found, returns nullptr.
 */
userpf_page_set* get_page_set(void* address) {
  std::lock_guard<turi::mutex> guard(lock);
  auto location = find_page_set(address);
  if (location.second == false) return nullptr;
  else return all_page_sets[location.first];
}

userpf_page_set* allocate(size_t length, 
                          userpf_handler_callback fill_callback,
                          userpf_release_callback release_callback,
                          bool writable) {
  // try to map a set of pages.
#ifdef __APPLE__
  void* mem = mmap(NULL, length, PROT_NONE,
                   MAP_ANON | MAP_PRIVATE , -1, 0);
  if (mem == nullptr) return nullptr;
#else
  void* mem = mmap(NULL, length, PROT_NONE,
                   MAP_ANONYMOUS | MAP_PRIVATE , -1, 0);
  if (mem == nullptr) return nullptr;
#endif
  // map successful. full in the userpf_page_set datastructure
  // and register the page in all_mapped_pages and return
  userpf_page_set* ps = new userpf_page_set;
  // pointer arithmetics
  ps->begin = (char*)mem;
  ps->end = ps->begin + length;
  ps->length = length;
  ps->num_large_pages = (length / TURI_PAGE_SIZE) + (length % TURI_PAGE_SIZE > 0);
  // callbacks
  ps->fill_callback = fill_callback;
  ps->release_callback = release_callback;
  // flags
  ps->writable = writable;
  ps->resident.resize(ps->num_large_pages);
  ps->resident.clear();
  ps->dirty.resize(ps->num_large_pages);
  ps->dirty.clear();
  ps->pagefile_maintained.resize(ps->num_large_pages);
  ps->pagefile_maintained.clear();
  // locks
  ps->locks.resize(ps->num_large_pages);
  // pagefile
  ps->pagefile_allocations = 
      std::vector<size_t>(ps->num_large_pages, (size_t)(-1));
  insert_page_set(ps);
  return ps;
}

void release(userpf_page_set* pageset) {
  if (pageset->release_callback) pageset->release_callback(pageset);
  remove_page_set(pageset);
  munmap(pageset->begin, pageset->length);
  for (size_t i = 0;i < pageset->pagefile_allocations.size(); ++i) {
    if (pageset->pagefile_allocations[i] != (size_t)(-1)) {
      disk_pagefile.release(pageset->pagefile_allocations[i]);
    }
  }
  delete pageset;
}



bool is_pagefault_handler_installed() {
  return pagefault_handler_installed;
}

size_t get_max_resident() {
  return MAX_QUEUE_LENGTH * TURI_PAGE_SIZE;
}

void set_max_resident(size_t max_resident_memory) {
  size_t max_queue = max_resident_memory / TURI_PAGE_SIZE;
  if (max_queue < 2) max_queue = 2;
  MAX_QUEUE_LENGTH = max_queue;
  handle_eviction();
}

/**
 * Evicts a page to the pagefile. page must be locked.
 */
static bool evict_to_pagefile(userpf_page_set* ps, size_t index) {
  if (ps->pagefile_allocations[index] == (size_t)(-1)) {
    // printf("Allocating a new page\n");
    ps->pagefile_allocations[index] = disk_pagefile.allocate();
  } 
  // printf("Evicting page: %ld to %ld\n", index, ps->pagefile_allocations[index]);
  char* aligned_address =  ps->begin + TURI_PAGE_SIZE * index;
  char* last_address = aligned_address + TURI_PAGE_SIZE;
  if (last_address > ps->end) last_address = ps->end;
  bool ret = disk_pagefile.write(ps->pagefile_allocations[index], aligned_address, 
                             last_address - aligned_address);
  // printf("Evicting page: %ld to %ld Done with %d \n", index, ps->pagefile_allocations[index], ret);
  return ret;
}

/**
 * Evicts stuff from the queue until the queue is smaller than MAX_QUEUE_LENGTH
 */
static void handle_eviction() {
  while(access_queue.size() > MAX_QUEUE_LENGTH) {
    // prefer to evict from the first half of the access queue
    auto evict_index = random::fast_uniform<size_t>(0, access_queue.size() / 2);
    auto evict_iter = access_queue.begin() + evict_index;

    // prefer things I don't need to swap to pagefile on
    // sample one additional random location
    {
      auto evict_index2 = random::fast_uniform<size_t>(0, access_queue.size() - 1);
      auto evict_iter2 = access_queue.begin() + evict_index2;
      userpf_page_set* ps;
      size_t index;
      size_t length;
      std::tie(ps, index, length) = (*evict_iter2);
      if (!ps->dirty.get(index)) evict_iter = evict_iter2;
    }

    {
      auto evict_index2 = random::fast_uniform<size_t>(0, access_queue.size() - 1);
      auto evict_iter2 = access_queue.begin() + evict_index2;
      userpf_page_set* ps;
      size_t index;
      size_t length;
      std::tie(ps, index, length) = (*evict_iter2);
      if (!ps->dirty.get(index)) evict_iter = evict_iter2;
    }

    
    auto evict = *evict_iter;
    access_queue.erase(evict_iter);
    userpf_page_set* ps;
    size_t index;
    size_t length;
    std::tie(ps, index, length) = evict;
    // printf("Evicting: %p Base: %p, index: %d\n", ps, ps->begin, index);

    void* address = ps->begin + TURI_PAGE_SIZE * index;
    std::lock_guard<simple_spinlock> protect_lock(ps->locks[index]);
    // if dirty..
    if (ps->writable && ps->dirty.get(index)) {
      // printf("Evicting to pagefile\n");
      evict_to_pagefile(ps, index);
      ps->dirty.clear_bit(index);
      ps->pagefile_maintained.set_bit(index);
    }
    ps->resident.clear_bit(index);
    mprotect(address, length,  PROT_NONE);
    // this releases the pages
#if __APPLE__
    madvise(address, length, MADV_FREE);
#else
    madvise(address, length, MADV_DONTNEED);
#endif
  }
}

/**
 * Inserts a page into the access queue.
 * (The actual page address is ps->begin + index * TURI_PAGE_SIZE to
 *  ps->begin + index * TURI_PAGE_SIZE + length)
 *
 *  The reason for "length" is because the last set of pages in a pageset
 *  may not encompass the entire TURI_PAGE_SIZE (see \ref fill_pages)
 */
static void add_to_access_queue(userpf_page_set* ps, size_t index, size_t length) {
  std::lock_guard<turi::mutex> guard(lock);
  access_queue.push_back(std::tuple<userpf_page_set*, size_t, size_t>{ps, index, length});
}


/**
 * fill in pages for a particular page from the pagefile.
 * page must be locked.
 */
static bool fill_pages_from_pagefile(userpf_page_set* ps, size_t index) {
  ASSERT_LT(index, ps->pagefile_allocations.size());
  char* aligned_address =  ps->begin + TURI_PAGE_SIZE * index;
  char* last_address = aligned_address + TURI_PAGE_SIZE;
  // printf("Filling page at %p from %ld\n", aligned_address, ps->pagefile_allocations[index]);
  if (last_address > ps->end) last_address = ps->end;
  if (ps->pagefile_allocations[index] == (size_t)(-1)) {
    return false;
  } else {
    disk_pagefile.read(ps->pagefile_allocations[index],
                       aligned_address,
                       last_address - aligned_address);
  }
  return true;
}

/**
 * fill in pages for a particular page using the callback handler
 */
static bool fill_pages(userpf_page_set* ps, size_t index) {

    char* aligned_address =  ps->begin + TURI_PAGE_SIZE * index;
    char* last_address = aligned_address + TURI_PAGE_SIZE;
    // shift back the last address if we will exceed the end
    if (last_address > ps->end) last_address = ps->end;
    size_t fill_length = last_address - aligned_address;
    // printf("Filling page at %p\n", aligned_address);
    handle_eviction();

    // ok call the callback to fill pages.
    {
      std::lock_guard<simple_spinlock> protect_lock(ps->locks[index]);
      // if we think it is already resident, quit
      if (ps->resident.get(index)) {
        // printf("Already resident\n");
        return true;
      }
      // mark the pages as readable + writable
      mprotect(aligned_address, fill_length,  PROT_READ | PROT_WRITE);
      // trigger callback to fill the pages
      if (ps->pagefile_maintained.get(index)) {
        // printf("Filling page: %ld from pagefile at %p\n", index, aligned_address);
        fill_pages_from_pagefile(ps, index);
      } else {
        // printf("Filling page: %ld from callback at %p\n", index, aligned_address);
        ps->fill_callback(ps, aligned_address, fill_length);
      }
      // mark it again as readable
      mprotect(aligned_address, fill_length,  PROT_READ);
      ps->resident.set_bit(index);
    }
    // put it into the queue so we can decommit it later.
    add_to_access_queue(ps, index, fill_length);
    return true;
}

/**
 * Make the page writable and dity
 */
static bool dirty_pages(userpf_page_set* ps, size_t index) {

    char* aligned_address =  ps->begin + TURI_PAGE_SIZE * index;
    char* last_address = aligned_address + TURI_PAGE_SIZE;
    // shift back the last address if we will exceed the end
    if (last_address > ps->end) last_address = ps->end;
    // printf("Dirty page: %p\n", aligned_address);
    mprotect(aligned_address, last_address - aligned_address,  
             PROT_READ | PROT_WRITE);
    ps->dirty.set_bit(index);
    return true;
}

/**
 * The actual handler called for the page. Handles a pagefault within the page 
 * "index" within the pageset ps.
 *
 * Requests for the TURI_PAGE_SIZE bytes beginning at 
 * ps->begin + index * TURI_PAGE_SIZE (or up to the allocation limit) 
 * be filled by the callback.
 */
static bool page_handler(userpf_page_set* ps, size_t index) {
  /*
   * There are a few cases.
   * Case 1: It is not resident. I need to bring it into memory.
   *    1a: It is maintained by pagefile
   *    1b: I need to ask the callback handelr for it
   * Case 2: It is resident. Then this must be an access violation
   *         If the pageset is writable, that's fine. remove the page
   *         protection and mark as dirty.
   *
   */
  if (!ps->resident.get(index)) {
    return fill_pages(ps, index);
  } else if (ps->writable) {
    return dirty_pages(ps, index);
  } 
  // all other cases are bad
  return false;
}

bool pagefault_handler(char* addr) {
  userpf_page_set* ps = get_page_set((void*)addr);
  bool page_handler_ok = true;
  if (ps != nullptr) {
    // printf("Pagefault handling at %p\n", addr);
    // okie. this points to one of our pages
    // round down to page boundaries
    // we are going to fill starting from the aligned address
    size_t index = (addr - ps->begin) / TURI_PAGE_SIZE;
    page_handler_ok &= page_handler(ps, index);

    if (page_handler_ok == false) {
      std::cerr << "Pagefault handler failed at " << (void*)(addr) << std::endl;
    }

    // it may be an unaligned read. so check the index of
    // address + (system word size - 1)
    // and fill the next pages too if the index does not match up
    size_t index2 = (addr + (sizeof(ptrdiff_t) - 1) - ps->begin) / TURI_PAGE_SIZE;
    if (index2 != index && 
        index2 < ps->num_large_pages) {
      auto success = page_handler(ps, index2);
      if (success == false) {
        std::cerr << "Pagefault handler failed at " << (void*)(addr) << std::endl;
      }
      page_handler_ok &= success;
    }
    // otherwise. fall through to the regular segfault handlers
    // and quit.
    return page_handler_ok;
  }  else {
    return false;
  }
}

size_t get_num_allocations() {
  return all_page_sets.size();
}

size_t pagefile_total_allocated_bytes() {
  return disk_pagefile.total_allocated_bytes();
}

size_t pagefile_total_stored_bytes() {
  auto sizes = disk_pagefile.get_arena_sizes();
  auto counts = disk_pagefile.get_allocation_counts();
  size_t ret = 0;
  for (size_t i = 0;i < sizes.size(); ++i) {
    ret += sizes[i] * counts[i];
  }
  return ret;
}


double pagefile_compression_ratio() {
  return disk_pagefile.get_compression_ratio();
}

#ifndef _WIN32
/**
 * The previous segfault action we fall back onto if the address
 * is not within the expected ranges.
 */
static struct sigaction prev_action;

// Mac and Linux segfault handler
static void segv_handler(int sig_num, siginfo_t* info, void* ucontext) {
  // this is the pointer which segfaulted
  char* addr = (char*)(info->si_addr);
  bool handler_ok = pagefault_handler(addr); 
  if (handler_ok) return;

  std::cerr << "Access to invalid address: " << (void*)(addr) << std::endl;
  if (prev_action.sa_handler) {
    prev_action.sa_handler(sig_num);
  } else if (prev_action.sa_sigaction) {
    prev_action.sa_sigaction(sig_num, info, ucontext);
  } else {
    abort();
  }
}
bool setup_pagefault_handler(size_t max_resident_memory) {
  // already installed...
  if (pagefault_handler_installed == true) return false;

  if (max_resident_memory == (size_t)(-1))  {
    char* ret = getenv("TURI_DEFAULT_PAGEFAULT_RESIDENT_LIMIT");
    if (ret != nullptr) {
      size_t limit = atoll(ret);
      if (limit != 0) max_resident_memory = limit;
    } 
  }
  if (max_resident_memory == (size_t)(-1)) {
    size_t default_memory_size = total_mem() / 2;
    if (default_memory_size == 0) default_memory_size = 128*1024*1024;
    max_resident_memory = default_memory_size;
  }

  //TODO: This functionality can be mirrored in Windows. Potentially with SEH,
  MAX_QUEUE_LENGTH = max_resident_memory / TURI_PAGE_SIZE;
  if (MAX_QUEUE_LENGTH < 2) MAX_QUEUE_LENGTH = 2;
  struct sigaction sigact;
  sigact.sa_sigaction = segv_handler;
  sigact.sa_flags = SA_RESTART | SA_SIGINFO;
#if __APPLE__
  if (sigaction(SIGBUS, &sigact, &prev_action) != 0) {
    fprintf(stderr, "error registering page fault handler");
    return false;
  } 
#else
  if (sigaction(SIGSEGV, &sigact, &prev_action) != 0) {
    fprintf(stderr, "error registering page fault handler");
    return false;
  } 
#endif
  pagefault_handler_installed = true;
  // set up disk pagefile
  std::vector<size_t> sizes;
  size_t num_4k_pages = TURI_PAGE_SIZE / 4096;
  size_t csize = num_4k_pages;
  for (size_t i = 0;i < pagefile::NUM_ARENAS; ++i) {
    sizes.push_back(csize * 4096);
    if (csize * 4096 < 64*1024 /*64 K minimum */) break;
    auto newcsize = csize / 4 * 3;
    if (newcsize == csize) break;
    csize = newcsize;
  }
  disk_pagefile.init(sizes);
  return true;
}



bool revert_pagefault_handler() {
  // not installed...
  if (pagefault_handler_installed == false) return false;
  // reset back to the previous action
  struct sigaction unused;
  sigaction(SIGSEGV, &prev_action, &unused);
  disk_pagefile.reset();
  pagefault_handler_installed = false;
  return true;
}

#else
#endif
} // user_pagefault
} // turicreate
