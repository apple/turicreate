/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/coreml_export/coreml_export_utils.hpp>
#include <memory>
#include <map>
#pragma push_macro("CHECK")
#undef CHECK
#include "MLModel/build/format/Model.pb.h"
#pragma pop_macro("CHECK")

namespace turi {

void add_metadata(std::shared_ptr<CoreML::Specification::Model> model_spec,
                  const std::map<std::string, flexible_type>& context) {
  DASSERT_TRUE(context.count("short_description"));
  DASSERT_TRUE(context.count("version"));
  DASSERT_TRUE(context.count("class"));
  DASSERT_TRUE(!context.count("model_fields") || context.at("model_fields").get_type() == flex_type_enum::DICT);

  CoreML::Specification::Metadata* metadata = model_spec->mutable_description()->mutable_metadata();
  metadata->set_shortdescription(context.at("short_description").to<flex_string>());
  auto user_defined = metadata->mutable_userdefined();
  if (context.count("model_fields")) {
    for (const auto& kv: context.at("model_fields").get<flex_dict>()) {
      (*user_defined)[kv.first] = kv.second.to<flex_string>();
    }
  }
  (*user_defined)["version"] = context.at("version").to<flex_string>();
  (*user_defined)["class"] = context.at("class").to<flex_string>();
}

}
