/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/coreml_export/coreml_export_utils.hpp>
#include <memory>
#include <map>

#include "toolkits/coreml_export/mlmodel_include.hpp"

namespace turi {

void add_metadata(CoreML::Specification::Model& model_spec,
                  const std::map<std::string, flexible_type>& context) {

    CoreML::Specification::Metadata* metadata = model_spec.mutable_description()->mutable_metadata();

    if (context.count("author")) {
      metadata->set_author(context.at("author").to<flex_string>());
    }

    if (context.count("short_description")) {
      metadata->set_shortdescription(context.at("short_description").to<flex_string>());
    }

    if (context.count("version_string")) {
      metadata->set_versionstring(context.at("version_string").to<flex_string>());
    }

    if (context.count("license")) {
      metadata->set_license(context.at("license").to<flex_string>());
    }

    if (context.count("user_defined")) {
      DASSERT_TRUE(context.at("user_defined").get_type() == flex_type_enum::DICT);

      auto user_defined = metadata->mutable_userdefined();
      for (const auto& kv: context.at("user_defined").get<flex_dict>()) {
        (*user_defined)[kv.first] = kv.second.to<flex_string>();
      }
    }
  }
}
