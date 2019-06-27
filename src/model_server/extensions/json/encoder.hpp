/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/variant.hpp>

namespace turi {
  namespace JSON {
    // variant_type -> variant_type
    // where the input is an arbitrary variant_type,
    // and the output is guaranteed to be naively JSON serializable.
    variant_type to_serializable(variant_type input);
  }
}
