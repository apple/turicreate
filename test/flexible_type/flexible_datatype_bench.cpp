/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/flexible_type/flexible_type.hpp>
#include <timer/timer.hpp>
using namespace turi;
const int PI_ITERATIONS = 100000000;
const int BUBBLE_SORT_SIZE = 30000;

double pi_apx() {
  double val = 0.0;
  for (size_t i = 0;i < PI_ITERATIONS; ++i) {
    if (i % 2 == 0) {
      val += 4.0 / (2 * i + 1);
    } else {
      val -= 4.0 / (2 * i + 1);
    }
  }
  return val;
}



void pi_apx_flex(flexible_type& val) {
  val = 0.0;
  for (size_t i= 0;i < PI_ITERATIONS; ++i) {
    if (i % 2 == 0) {
      val += 4.0 / double(2 * i + 1);
    } else {
      val -= 4.0 / double(2 * i + 1);
    }
  }
}


void pi_apx_flex2(flexible_type& val) {
  val = 0.0;
  for (flexible_type i = 0;i < PI_ITERATIONS; ++i) {
    if (i % 2 == 0) {
      val += 4.0 / double(2 * i + 1);
    } else {
      val -= 4.0 / double(2 * i + 1);
    }
  }
}


void pi_apx_flex3(flexible_type& val) {
  for (flexible_type i = 0;i < PI_ITERATIONS; ++i) {
    if (i % 2 == 0) {
      val += 4.0 / double(2 * i + 1);
    } else {
      val -= 4.0 / double(2 * i + 1);
    }
  }
}



void sort_vec(std::vector<double>& s) {
  for (size_t i = 0;i < s.size(); ++i) {
    auto& a = s[i];
    for (size_t j = i + 1;j < s.size(); ++j) {
      auto& b = s[j];
      if (a < b) std::swap(a, b);
    }
  }
}

void sort_flexvec(flexible_type& s) {
  assert(s.get_type() == flex_type_enum::VECTOR);
  size_t len = s.size();
  for (size_t i = 0;i < len; ++i) {
    const auto& a = s[i];
    for (size_t j = i + 1;j < len; ++j) {
      const auto& b = s[j];
      if (a < b) std::swap(s[i], s[j]);
    }
  }
}


void sort_flexrecursive(flexible_type& __restrict__ s) {
  size_t len = s.size();
  for (size_t i = 0;i < len; ++i) {
    auto& a = s(i);
    for (size_t j = i + 1;j < len; ++j) {
      auto& b = s(j);
      //if (a.get<flex_float>() < b.get<flex_float>()) std::swap(a,b);
      if (a.get<flex_float>() < b.get<flex_float>()) std::swap(a,b);
      //if (s.array_at(i).get<flex_float>() < s.array_at(j).get<flex_float>()) std::swap(s.array_at(i), s.array_at(j));
    }
  }
}


int main(int argc, char** argv) {
  flexible_type f;

  std::cout << "size of flexible_type = " << sizeof(flexible_type) << "\n";

  timer ti;
  std::cout << "Gregory-Liebniz Pi Approximation. " << PI_ITERATIONS << " iterations\n";
  ti.start();
  std::cout << pi_apx() << "\n";
  std::cout << "Double: " << ti.current_time() << "\n";


  f = 0.0;
  ti.start();
  pi_apx_flex(f);
  std::cout << (double)f << "\n";
  std::cout << "Flexible_type summand: " << ti.current_time() << "\n";


  f = 0.0;
  ti.start();
  pi_apx_flex2(f);
  std::cout << (double)f << "\n";
  std::cout << "Flexible_type with flexible loop index: " << ti.current_time() << "\n";


  f = 0.0;
  ti.start();
  pi_apx_flex3(f);
  std::cout << double(f) << "\n";
  std::cout << "Flexible_type with flexible_loop index and no type forcing: " << ti.current_time() << "\n";

  std::cout << "\n\n\n"
            << "Double Array Sort (really... a bad insertion sort)\n";

  srand(144);
  std::vector<double> s;
  s.resize(BUBBLE_SORT_SIZE);
  for (size_t i = 0;i < BUBBLE_SORT_SIZE; ++i) s[i] =rand();
  f = s;
  assert(s.size() == f.size());
  
 flexible_type f2(flex_type_enum::LIST);
  for (size_t i = 0;i < BUBBLE_SORT_SIZE; ++i) {
    f2.push_back(flexible_type(s[i]));
  }


  ti.start();
  sort_vec(s);
  std::cout << "Vec Sort in " << ti.current_time() << "\n";


  ti.start();
  sort_flexvec(f);
  std::cout << "flex vec Sort in " << ti.current_time() << "\n";


  ti.start();
  sort_flexrecursive(f2);
  std::cout << "Recursive flex vec Sort in " << ti.current_time() << "\n";


  // equality test
  for (size_t i = 0; i < s.size(); ++i) {
    assert(s[i] == f[i]);
    assert(s[i] == f2(i));
  }
}
