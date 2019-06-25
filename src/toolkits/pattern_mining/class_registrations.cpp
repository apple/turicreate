/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/pattern_mining/fp_growth.hpp>

namespace turi {
namespace pattern_mining {


BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_pattern_mining_create, "data", "event", "features",
          "min_support", "max_patterns", "min_length");
END_FUNCTION_REGISTRATION

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(fp_growth)
END_CLASS_REGISTRATION

}// pattern_mining
}// turicreate
