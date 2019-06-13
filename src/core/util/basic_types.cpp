/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <util/basic_types.hpp>

#include <logger/assertions.hpp>

#include <vector>
using std::vector;

int64_t check(const char* desc, int64_t ret) {
  if (ret < 0) {
    perror(desc);
    AU();
  }
  return ret;
}

void* check_ptr(const char* desc, void* ptr) {
  if (!ptr) {
    perror(desc);
    AU();
  }
  return ptr;
}

vector<int64_t> contiguous_strides(const vector<int64_t>& shape) {
  if (shape.size() == 0) {
    return vector<int64_t>();
  }
  int64_t curr = 1;
  vector<int64_t> ret;
  for (int64_t i = shape.size() - 1; i != 0; i--) {
    ret.push_back(curr);
    curr *= shape[i];
  }
  reverse(ret.begin(), ret.end());
  return ret;
}
