/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef COREML_EXPORT_UTILS_HPP
#define COREML_EXPORT_UTILS_HPP
#include <memory>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

namespace turi {

/**
 * Add short description, metadata to the model.
 *
 * \param[in] model CoreML specification model
 * \param[in] context Dictionary of context passed from python.
 */
void add_metadata(CoreML::Specification::Model& model_spec,
                  const std::map<std::string, flexible_type>& context);


}

#endif
