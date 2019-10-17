/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_TRAINING_H_
#define TURI_UTIL_TRAINING_H_

#include <string>
#include <vector>

#include <core/logging/assertions.hpp>

namespace turi {

/**
 * This prints the verbose statements based on GPU names
 * it gets from the respective compute contexts.
 */
void print_training_device(std::vector<std::string> gpu_names);

}  // namespace turi

#endif /* TURI_UTIL_TRAINING_H_ */