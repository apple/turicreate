/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef _TURI_TIME_SPEC_H_
#define _TURI_TIME_SPEC_H_

#include <util/basic_types.hpp>

struct time_spec {
  int8_t sign;
  int64_t sec;
  int64_t nsec;

  int64_t val();
};

time_spec operator+(time_spec a, time_spec b);
time_spec operator-(time_spec a);
time_spec operator-(time_spec a, time_spec b);

bool operator<(time_spec a, time_spec b);
bool operator>(time_spec a, time_spec b);
bool operator<=(time_spec a, time_spec b);
bool operator>=(time_spec a, time_spec b);
bool operator==(time_spec a, time_spec b);

time_spec now();

#endif
