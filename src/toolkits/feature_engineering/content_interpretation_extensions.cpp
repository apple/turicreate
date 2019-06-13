/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/feature_engineering/content_interpretation.hpp>

using namespace turi;
using namespace turi::feature_engineering;

EXPORT bool _content_interpretation_valid(gl_sarray data, const flex_string& interpretation) {
  return content_interpretation_valid(data, interpretation);
}

EXPORT flex_string _infer_content_interpretation(gl_sarray data) {
  return infer_content_interpretation(data);
}


BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_content_interpretation_valid, "data", "interpretation");
REGISTER_FUNCTION(_infer_content_interpretation, "data");
END_FUNCTION_REGISTRATION
