/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <fiber/fiber_group.hpp>
#include <timer/timer.hpp>
using namespace turi;
int numticks = 0;
void threadfn() {

  timer ti; ti.start();
  while(1) {
    if (ti.current_time() >= 1) break;
    fiber_control::yield();
    __sync_fetch_and_add(&numticks, 1);
  }
}


void threadfn2() {

  timer ti; ti.start();
  while(1) {
    if (ti.current_time() >= 2) break;
    fiber_control::yield();
    __sync_fetch_and_add(&numticks, 2);
  }
}

int main(int argc, char** argv) {
  timer ti; ti.start();
  fiber_group group;
  fiber_group group2;
  for (int i = 0;i < 100000; ++i) {
    group.launch(threadfn);
    group2.launch(threadfn2);
  }
  group.join();
  std::cout << "Completion in " << ti.current_time() << "s\n";
  std::cout << "Context Switches: " << numticks << "\n";
  group2.join();
  std::cout << "Completion in " << ti.current_time() << "s\n";
  std::cout << "Context Switches: " << numticks << "\n";
}
