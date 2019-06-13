/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/util/any.hpp>

namespace turi {


  /**
   * Define the static registry for any
   */
  any::registry_map_type& any::get_global_registry() {
    static any::registry_map_type global_registry;
    return global_registry;
  }



  any::iholder* any::iholder::load(iarchive_soft_fail &arc) {
    registry_map_type& global_registry = get_global_registry();
    uint64_t idload;
    arc >> idload;
    registry_map_type::const_iterator iter = global_registry.find(idload);
    if(iter == global_registry.end()) {
      logstream(LOG_FATAL)
        << "Cannot load object with hashed type [" << idload
        << "] from stream!" << std::endl
        << "\t A possible cause of this problem is that the type"
        << std::endl
        << "\t is never explicity used in this program.\n\n" << std::endl;
      return NULL;
    }
    // Otherwise the iterator points to the deserialization routine
    // for this type
    return iter->second(arc);
  }


} // end of namespace turi


std::ostream& operator<<(std::ostream& out, const turi::any& any) {
  return any.print(out);
} // end of operator << for any
