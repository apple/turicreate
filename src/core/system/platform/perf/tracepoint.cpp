/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <limits>
#include <string>
#include <core/logging/assertions.hpp>
#include <perf/tracepoint.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <boost/unordered_map.hpp>


namespace turi {

void trace_count::print(std::ostream& out, unsigned long long tpersec) const {
  if (tpersec == 0) {
    out << name << ": " << description << "\n";
    out << "Events:\t" << count.value << "\n";
    out << "Total:\t" << total.value << "ticks \n";
    if (count.value > 0) {
      out << "Mean:\t" << (double)total.value / count.value << "ticks \n";
      out << "Min:\t" << minimum << "ticks \n";
      out << "Max:\t" << maximum << "ticks \n";
    }
  }
  else {
    double tperms = (double)tpersec / 1000;
    out << name << ": " << description << "\n";
    out << "Events:\t" << count.value << "\n";
    out << "Total:\t" << (double)total.value / tperms << " ms \n";
    if (count.value > 0) {
      out << "Mean:\t" << (double)total.value / count.value / tperms << " ms \n";
      out << "Min:\t" << (double)minimum / tperms << " ms \n";
      out << "Max:\t" << (double)maximum / tperms << " ms \n";
    }
  }
}


static mutex printlock;

trace_count::~trace_count() {
#ifdef USE_TRACEPOINT
  printlock.lock();
  print(std::cerr, estimate_ticks_per_second());
  std::cerr.flush();
  printlock.unlock();
#endif
}

} // namespace turi
