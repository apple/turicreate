/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_USER_ITEM_GRAPH_H_
#define TURI_RECSYS_USER_ITEM_GRAPH_H_

#include <toolkits/ml_data_2/ml_data.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sgraph_data/sgraph_compute.hpp>
#include <memory>
#include <vector>

namespace turi { namespace recsys {

/**
 *  Build a user-item bipartite graph.
 *  user_item_lists are added to user nodes as vertex_data
 *
 */
void make_user_item_graph(const v2::ml_data& data,
                          const std::shared_ptr<sarray<flex_dict> >& user_item_lists,
                          sgraph& g);
}}
#endif /* _USER_ITEM_GRAPH_H_ */
