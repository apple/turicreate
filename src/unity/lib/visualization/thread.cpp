/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "thread.hpp"

#include <iostream>
#include <thread>

using namespace turi;

void visualization::run_thread(std::function<void()> func) {
  std::thread([func]() {

      // Because we're going to detach, make sure we swallow errors.
      // Throw an exception from the code in `func`, and we'll prevent
      // the throwing part while making sure the logging part happens.
      // Otherwise, throwing from a detached thread will take down
      // the entire process (which we don't want to do).
      try {
        func();
      } catch (const std::exception& e) {
        // print exception message
        std::cerr << "Error in visualization background thread: " << e.what() << std::endl;
      } catch (const std::string& s) {
        // print string that was thrown
        std::cerr << "Error in visualization background thread: " << s << std::endl;
      } catch (const char *c) {
        // print char array that was thrown
        // let's hope it's null-terminated!
        std::cerr << "Error in visualization background thread: " << c << std::endl;
      } catch (...) {
        // print generic message
        std::cerr << "Unknown error in visualization background thread." << std::endl;
      }

  }).detach(); // always detach
}
