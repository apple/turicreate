/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <util/time_spec.hpp>

#include <chrono>

int64_t time_spec::val() {
  assert (this->sec < (1LL<<32));
  return (int64_t)this->sign * (this->sec * 1000000000LL + this->nsec);
}

time_spec operator+(time_spec a, time_spec b) {
  time_spec ret;
  if (a.sign == b.sign) {
    ret.sec = a.sec + b.sec;
    ret.nsec = a.nsec + b.nsec;
    if (ret.nsec >= 1000000000) {
      ret.nsec -= 1000000000;
      ++ret.sec;
    }
    ret.sign = a.sign;
  } else {
    if (b.sec > a.sec || (b.sec == a.sec && b.nsec > a.nsec)) {
      return b + a;
    } else {
      ret.sec = a.sec - b.sec;
      ret.nsec = a.nsec - b.nsec;
      if (ret.nsec < 0) {
        ret.nsec += 1000000000;
        --ret.sec;
      }
      ret.sign = a.sign;
    }
  }
  return ret;
}

time_spec operator-(time_spec a) {
  time_spec ap;
  ap.sign = -a.sign;
  ap.sec = a.sec;
  ap.nsec = a.nsec;
  return ap;
}

time_spec operator-(time_spec a, time_spec b) {
  return a + (-b);
}

bool operator==(time_spec a, time_spec b) {
  return a.nsec == b.nsec && a.sec == b.sec && a.sign == b.sign;
}

int32_t cmp(time_spec a, time_spec b) {
  if (a.sign != b.sign) {
    return (a.sign < b.sign ? -1 : 1);
  }
  auto a_sec = a.sign * a.sec;
  auto b_sec = b.sign * b.sec;
  if (a_sec < b_sec) {
    return -1;
  }
  if (b_sec < a_sec) {
    return 1;
  }
  auto a_nsec = a.sign * a.nsec;
  auto b_nsec = b.sign * b.nsec;
  if (a_nsec < b_nsec) {
    return -1;
  }
  if (b_nsec < a_nsec) {
    return 1;
  }
  return 0;
}

bool operator<(time_spec a, time_spec b) {
  return cmp(a, b) == -1;
}

bool operator>(time_spec a, time_spec b) {
  return b < a;
}

bool operator<=(time_spec a, time_spec b) {
  return !(b < a);
}

bool operator>=(time_spec a, time_spec b) {
  return !(a < b);
}

time_spec now() {
  time_spec ret;
  auto t = std::chrono::high_resolution_clock::now().time_since_epoch();
  auto ts = std::chrono::duration_cast<std::chrono::seconds>(t);
  ret.sec = ts.count();
  auto tn = t - ts;
  ret.nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(tn).count();
  ret.sign = +1;
  return ret;
}
