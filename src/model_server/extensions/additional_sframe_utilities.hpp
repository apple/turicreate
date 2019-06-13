/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_EXTENSIONS_ADDITIONAL_SFRAME_UTILITIES_HPP
#define TURI_UNITY_EXTENSIONS_ADDITIONAL_SFRAME_UTILITIES_HPP
#include <core/data/sframe/gl_sarray.hpp>

void sframe_load_to_numpy(turi::gl_sframe input, size_t outptr_addr,
                     std::vector<size_t> outstrides,
                     std::vector<size_t> field_length,
                     size_t begin, size_t end);

void image_load_to_numpy(const turi::image_type& img, size_t outptr_addr,
                         const std::vector<size_t>& outstrides);

#endif
