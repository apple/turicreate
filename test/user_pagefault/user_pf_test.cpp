/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <user_pagefault/user_pagefault.hpp>
#include <random/random.hpp>
#include <timer/timer.hpp>
#include <parallel/lambda_omp.hpp>
using namespace turi;
using namespace user_pagefault;

// we are just making an array of size_t integers
size_t handler_callback(userpf_page_set* ps,
                        char* page_address,
                        size_t minimum_fill_length) {
  size_t* root = (size_t*) ps->begin;
  size_t* s_addr  = (size_t*) page_address;

  size_t begin_value = s_addr - root;
  size_t num_to_fill = minimum_fill_length / sizeof(size_t);

  for (size_t i = 0; i < num_to_fill; ++i) {
    s_addr[i] = i + begin_value;
  }
  return minimum_fill_length;
}

const size_t MB = 1024*1024;
const size_t MAX_RESIDENT = 128* MB;
const size_t NUM_VIRTUAL_VALUES = 1024 * MB;


void test_reads() {
  size_t nlen = NUM_VIRTUAL_VALUES;
  auto ps = allocate(nlen * sizeof(size_t), handler_callback);
  size_t* begin = (size_t*)ps->begin;


  timer ti;
  for (size_t i = 0;i < 1024; ++i) {
    size_t r = turi::random::fast_uniform<size_t>(0, nlen - 1);
    ASSERT_EQ(begin[r], r);
  }
  std::cout << ti.current_time() << "s for 1024 random accesses\n";

  ti.start();

  for (size_t i = 0 ;i < 1024; ++i) {
    size_t start = turi::random::fast_uniform<size_t>(0, nlen  - (1024*1024) - 1);
    for (size_t j = 0; j < 1024*1024; ++j) {
      size_t r = start + j;
      ASSERT_EQ(begin[r], r);
    }
  }
  std::cout << ti.current_time() << "s for 1024 random accesses of 1M each\n";
  ti.start();

  parallel_for ((size_t)0 ,1024, [&](size_t i) {
    size_t start = turi::random::fast_uniform<size_t>(0, nlen  - (1024 * 1024) - 1);
    // size_t start = 1024 * 1024 * i;
    // std::cout << "Starting at : " << start << std::endl;
    for (size_t j = 0; j < 1024*1024; ++j) {
      size_t r = start + j;
      if (begin[r] != r) {
        std::cerr << "begin[" << r << "] == " << begin[r] << std::endl;
        ASSERT_TRUE(false);
      }
    }
  });
  std::cout << ti.current_time() << "s for 1024 parallel random accesses of 1M each\n";

  release(ps);
}



void test_writes() {
  size_t nlen = NUM_VIRTUAL_VALUES;
  auto ps = allocate(nlen * sizeof(size_t), handler_callback, 
                     nullptr, true /*write enable*/);
  size_t* begin = (size_t*)ps->begin;


  timer ti;
  // read a bunch
  for (size_t i = 0;i < 1024; ++i) {
    size_t r = turi::random::fast_uniform<size_t>(0, nlen - 1);
    ASSERT_EQ(begin[r], r);
  }
  std::cout << ti.current_time() << "s for 1024 random accesses\n";

  ti.start();
  for (size_t i = 0 ;i < nlen; ++i) {
    begin[i] = 1;
  }
  std::cout << ti.current_time() << "s sequential rewrite of " << nlen << " elements to 0\n";
  ti.start();

  // validate
  ti.start();
  for (size_t i = 0 ;i < nlen; ++i) {
    ASSERT_EQ(begin[i], (size_t)(1));
    begin[i] = 2;
  }
  std::cout << ti.current_time() << "s sequential re-read of " << nlen << " elements and rewrite to 1\n";


  ti.start();
  for (size_t i = 0 ;i < nlen; ++i) {
    ASSERT_EQ(begin[i], (size_t)(2));
  }
  std::cout << ti.current_time() << "s sequential re-read of " << nlen << " elements\n";

  ti.start();
  for (size_t i = 0 ;i < nlen; ++i) {
    begin[i] = turi::random::fast_uniform<size_t>(0, nlen - 1);
  }
  std::cout << ti.current_time() << "s sequential re-write of " << nlen << " elements to random values\n";
  release(ps);
}

int main(int argc, char** argv) {
  setup_pagefault_handler(MAX_RESIDENT);
  test_reads();
  test_writes();
  revert_pagefault_handler();
}
