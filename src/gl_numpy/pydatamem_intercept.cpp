/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <so_utils/so_utils.hpp>
#include <export.hpp>
#include <logger/assertions.hpp>
#include <gl_numpy/alt_malloc.hpp>
using namespace turi;
// helper function to align a pointer to a particular page size
static char* page_align_address(char* ptr, size_t pagesize) {
  return ptr - (reinterpret_cast<size_t>(ptr) & (pagesize - 1));
}

/*
 * Helper function to set memory protection on a pointer range.
 * Assumes length is no larger than pagesize.
 */
static bool set_memory_protection(char* ptr, size_t len, int protect_flags) {
  size_t pagesize = getpagesize();
  ASSERT_LT(len, pagesize);

  char* aligned_address_start = page_align_address(ptr, pagesize);
  char* aligned_address_end = page_align_address(ptr + len - 1, pagesize);
  size_t npages = 1;
  if (aligned_address_start != aligned_address_end) {
    npages = 2;
  } 
  int ret = mprotect(aligned_address_start, npages * pagesize, protect_flags);
  return ret == 0;
}

/*
 * Gets the base address of a library
 */
static void* get_dl_baseaddr(char* so_name) {
  try {
    auto handle = so_util::open_shared_library(so_name);
    auto ret = handle.base_ptr;
    close_shared_library(handle);
    return ret;
  } catch(...){
    return nullptr;
  }
}

/* 
 * There are basically reimplementations of a few stuff
 * in numpy/core/src/multiarray/alloc.c
 * 
 *  void* PyDataMem_NEW(size_t);            == malloc
 *
 *  void* PyDataMem_ZEROED(size_t, size_t); == calloc
 *
 *  void* PyDataMem_RENEW(void*, size_t);   == realloc
 *
 *  void* PyDataMem_FREE(void*);            == free
 *  
 *  void* npy_alloc_cache(npy_uintp sz);    == malloc
 *
 *  void* npy_alloc_cache_zero(npy_uintp sz); == calloc with elsize = 1
 *
 *  void* npy_free_cache(void * p, npy_uintp sd); == free(p)
 */
                                   
void* AltPyDataMem_NEW(size_t size) {
  return my_malloc(size);
}

void* AltPyDataMem_NEW_ZEROED(size_t size, size_t elsize) {
  return my_calloc(size, elsize);
}

void* AltPyDataMem_RENEW(void* ptr, size_t len) {
  return my_realloc(ptr, len);
}

void AltPyDataMem_FREE(void* ptr) {
  my_free(ptr);
}

void* Altnpy_alloc_cache(size_t size) {
  return AltPyDataMem_NEW(size);
}

void* Altnpy_alloc_cache_zero(size_t size) {
  return AltPyDataMem_NEW_ZEROED(size, 1);
}

void Altnpy_free_cache(void* ptr, size_t unused) {
  AltPyDataMem_FREE(ptr);
}
extern "C" {


EXPORT bool perform_function_override(char* function_to_override,
                                      void* target_function) {
  // basically what we are going to do is to write the instructions
  //
  //     MOV [Address], %rax       48 B8 [8 bytes address]
  //     JMP %rax                 FF D0
  //
  // to the address of the function to override
  const size_t NBYTES_TO_WRITE = 12;
  unsigned char instructions[12] = 
                        {0x48, 0xB8, 
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                           0xFF, 0xE0};
  (*reinterpret_cast<void**>(instructions + 2)) = target_function;

  // ok. unprotect the pages
  bool ok = set_memory_protection(function_to_override,
                                  NBYTES_TO_WRITE,
                                  PROT_READ | PROT_WRITE);
  if (!ok) return false;
  //write
  memcpy(function_to_override, instructions, NBYTES_TO_WRITE);

  // protect them again
  set_memory_protection(function_to_override,
                        NBYTES_TO_WRITE,
                        PROT_READ | PROT_EXEC);
  return true;
}
EXPORT bool perform_numpy_malloc_override(char* library, 
                                          size_t malloc_offset,
                                          size_t calloc_offset,
                                          size_t realloc_offset,
                                          size_t free_offset,
                                          size_t npy_alloc_cache_offset,
                                          size_t npy_alloc_cache_zero_offset,
                                          size_t npy_free_cache_offset) {
  char* base_addr = reinterpret_cast<char*>(get_dl_baseaddr(library));
  if (base_addr == nullptr) return false;

  bool ret = true;
  ret &= perform_function_override(base_addr + malloc_offset,
                                   reinterpret_cast<void*>(AltPyDataMem_NEW));
  ret &= perform_function_override(base_addr + calloc_offset,
                                   reinterpret_cast<void*>(AltPyDataMem_NEW_ZEROED));
  ret &= perform_function_override(base_addr + realloc_offset,
                                   reinterpret_cast<void*>(AltPyDataMem_RENEW));
  ret &= perform_function_override(base_addr + free_offset,
                                   reinterpret_cast<void*>(AltPyDataMem_FREE));
  ret &= perform_function_override(base_addr + npy_alloc_cache_offset,
                                   reinterpret_cast<void*>(Altnpy_alloc_cache));
  ret &= perform_function_override(base_addr + npy_alloc_cache_zero_offset,
                                   reinterpret_cast<void*>(Altnpy_alloc_cache_zero));
  ret &= perform_function_override(base_addr + npy_free_cache_offset,
                                   reinterpret_cast<void*>(Altnpy_free_cache));
  return ret;
}

}
