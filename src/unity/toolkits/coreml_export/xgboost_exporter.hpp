/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef COREML_XGBOOST_EXPORTER
#define COREML_XGBOOST_EXPORTER

#include <string>
#include <ml_data/metadata.hpp>
#include <vector>

namespace turi {

void EXPORT export_xgboost_model(const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<std::string>& trees_as_json_dump,
    bool is_classifier, bool is_random_forest,
    const std::map<std::string, flexible_type>& context);

}

#endif
