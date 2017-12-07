/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef COREML_LINEAR_MODELS_EXPORTER
#define COREML_LINEAR_MODELS_EXPORTER

#include <string>
#include <ml_data/metadata.hpp>
#include <vector>
#include <numerics/armadillo.hpp>

namespace turi {

/**
 * Export as model asset
 */
void EXPORT export_linear_regression_as_model_asset(
    const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const arma::vec& coefs,
    const std::map<std::string, flexible_type>& context);

/**
 * Export linear SVM as model asset.
 */
void EXPORT export_linear_svm_as_model_asset(const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const arma::vec& coefs,
    const std::map<std::string, flexible_type>& context);


/**
 * Export logistic regression as model asset.
 */
void EXPORT export_logistic_model_as_model_asset(
    const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const arma::vec& coefs,
    const std::map<std::string, flexible_type>& context);


}

#endif
