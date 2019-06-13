/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKITS_CONTENT_INTERPRETATION_H
#define TURI_UNITY_TOOLKITS_CONTENT_INTERPRETATION_H

#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

namespace turi { namespace feature_engineering {

bool content_interpretation_valid(gl_sarray data, const flex_string& interpretation);
flex_string infer_content_interpretation(gl_sarray data);
flex_string infer_string_content_interpretation(gl_sarray data);

}}

#endif /* CONTENT_INTERPRETATION_H */
