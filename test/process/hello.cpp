/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

int main(int argc, char **argv) {
  std::stringstream ss;
  ss << "Hello world! ";
  for(int i = 0; i < argc; ++i) {
    ss << argv[i];
    ss << " ";
  }
  
  std::cout << ss.str();

  //std::cerr << "ERRORS AND STUFF!!!";

  return 0;
}
