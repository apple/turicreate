/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef COREML_XGBOOST_EXPORTER
#define COREML_XGBOOST_EXPORTER

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <ml/ml_data/metadata.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>

namespace turi {

std::shared_ptr<coreml::MLModelWrapper> export_xgboost_model(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<std::string>& trees_as_json_dump,
    bool is_classifier, bool is_random_forest,
    const std::map<std::string, flexible_type>& context);

// Simplified version of the above, with exported symbol visibility so that
// extensions can link to it.
void EXPORT export_xgboost_model(const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<std::string>& trees_as_json_dump,
    bool is_classifier, bool is_random_forest,
    const std::map<std::string, flexible_type>& context);

}

#endif
