/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <thread>
#include <iostream>

int main(int argc, char **argv) {
  std::cout << "Sleeping!" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return 0;
}

