/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/variant.hpp>

namespace turi {
  namespace JSON {
    // flexible_type, flexible_type -> variant_type
    // where the input is a pair (data, schema)
    // as produced by JSON::to_serializable, and the
    // output is the original underlying variant_type.
    variant_type from_serializable(flexible_type data, variant_type schema);
  }
}
