/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_RANDOM_SFRAME_GENERATION_H_
#define TURI_UNITY_RANDOM_SFRAME_GENERATION_H_

#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <string>

turi::gl_sframe _generate_random_sframe(size_t num_rows, std::string column_types,
                                            size_t random_seed, bool generate_target, double noise_level);

turi::gl_sframe _generate_random_classification_sframe(size_t n_rows, std::string column_types,
                                                           size_t _random_seed, size_t num_classes,
                                                           size_t num_extra_class_bins, double noise_level);

#endif /* TURI_UNITY_RANDOM_SFRAME_GENERATION_H_ */
