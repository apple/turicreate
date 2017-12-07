/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "tests/MLModelTests.hpp"
#include <iostream>

// TODO -- Make these macros
int main(int argc, char *argv[]) {
    int result = 0;
#define MLMODEL_RUN_TEST(t) result += t();
#include "tests/MLModelTests.hpp"
    if (result == 0) {
        std::cerr << "Passed all tests!" << std::endl;
    } else {
        std::cerr << "Failed " << result << " tests." << std::endl;
    }
    return result;
}
