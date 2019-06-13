/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/variant.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/api/unity_sframe_interface.hpp>
#include <model_server/lib/api/unity_sarray_interface.hpp>
#include <model_server/lib/api/unity_graph_interface.hpp>
#include <model_server/lib/api/function_closure_info.hpp>
#include <model_server/lib/extensions/model_base.hpp>

namespace turi {
namespace archive_detail {

void serialize_impl<oarchive, variant_type, false>::
exec(oarchive& oarc, const variant_type& v) {
  oarc << v.which();
  switch(v.which()) {
   case 0:
     oarc << boost::get<flexible_type>(v);
     break;
   case 1:
     oarc << boost::get<std::shared_ptr<unity_sgraph_base>>(v);
     break;
   case 2:
     oarc << boost::get<dataframe_t>(v);
     break;
   case 3:
     oarc << boost::get<std::shared_ptr<model_base>>(v);
     break;
   case 4:
     oarc << boost::get<std::shared_ptr<unity_sframe_base>>(v);
     break;
   case 5:
     oarc << boost::get<std::shared_ptr<unity_sarray_base>>(v);
     break;
   case 6:
     oarc << boost::get<variant_map_type>(v);
     break;
   case 7:
     oarc << boost::get<variant_vector_type>(v);
     break;
   case 8:
     oarc << boost::get<function_closure_info>(v);
     break;
   default:
     break;
  }
}



void deserialize_impl<iarchive, variant_type, false>::
exec(iarchive& iarc, variant_type& v) {
  int which;
  iarc >> which;
  switch(which) {
   case 0:
     v = flexible_type();
     iarc >> boost::get<flexible_type>(v);
     break;
   case 1:
     v = std::shared_ptr<unity_sgraph_base>();
     iarc >> boost::get<std::shared_ptr<unity_sgraph_base>>(v);
     break;
   case 2:
     v = dataframe_t();
     iarc >> boost::get<dataframe_t>(v);
     break;
   case 3:
     v = std::shared_ptr<model_base>();
     iarc >> boost::get<std::shared_ptr<model_base>>(v);
     break;
   case 4:
     v = std::shared_ptr<unity_sframe_base>();
     iarc >> boost::get<std::shared_ptr<unity_sframe_base>>(v);
     break;
   case 5:
     v = std::shared_ptr<unity_sarray_base>();
     iarc >> boost::get<std::shared_ptr<unity_sarray_base>>(v);
     break;
   case 6:
     v = variant_map_type();
     iarc >> boost::get<variant_map_type>(v);
     break;
   case 7:
     v = variant_vector_type();
     iarc >> boost::get<variant_vector_type>(v);
     break;
   case 8:
     v = function_closure_info();
     iarc >> boost::get<function_closure_info>(v);
     break;
   default:
     break;
  }
}


} // archive_detail
} // turi
