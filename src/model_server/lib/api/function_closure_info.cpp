/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/variant.hpp>
#include <model_server/lib/api/function_closure_info.hpp>

namespace turi {


void function_closure_info::save(oarchive& oarc) const {
  oarc << native_fn_name;
  oarc << arguments.size();
  for (size_t i = 0;i < arguments.size(); ++i) {
    oarc << arguments[i].first << *(arguments[i].second);
  }
}
void function_closure_info::load(iarchive& iarc) {
  arguments.clear();
  size_t nargs = 0;
  iarc >> native_fn_name >> nargs;
  arguments.resize(nargs);
  for (size_t i = 0;i < arguments.size(); ++i) {
    iarc >> arguments[i].first;
    arguments[i].second.reset(new variant_type);
    iarc >> *(arguments[i].second);
  }
}
}
