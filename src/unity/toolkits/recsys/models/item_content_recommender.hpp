/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_MODEL_ITEM_CONTENT_RECOMMENDER_H_
#define TURI_RECSYS_MODEL_ITEM_CONTENT_RECOMMENDER_H_

#include <vector>
#include <string>
#include <unity/toolkits/recsys/models/itemcf.hpp>

namespace turi {

namespace v2 {
class ml_data;
}

class sframe;
class sgraph;
class flexible_type;
class sparse_similarity_lookup;

namespace recsys {

class EXPORT recsys_item_content_recommender : public recsys_itemcf {
 public:
  BEGIN_CLASS_MEMBER_REGISTRATION("item_content_recommender")
  IMPORT_BASE_CLASS_REGISTRATION(recsys_model_base)
  END_CLASS_MEMBER_REGISTRATION
};

}}

#endif
