/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef COREML_LINEAR_MODELS_EXPORTER
#define COREML_LINEAR_MODELS_EXPORTER

#include <map>
#include <memory>
#include <string>

#include <ml_data/metadata.hpp>
#include <Eigen/Core>
#include <unity/toolkits/coreml_export/mlmodel_wrapper.hpp>

namespace turi {

/**
 * Export as model asset
 */
std::shared_ptr<coreml::MLModelWrapper> export_linear_regression_as_model_asset(
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context);

/**
 * Export linear SVM as model asset.
 */
std::shared_ptr<coreml::MLModelWrapper> export_linear_svm_as_model_asset(
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context);


/**
 * Export logistic regression as model asset.
 */
std::shared_ptr<coreml::MLModelWrapper> export_logistic_model_as_model_asset(
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context);

/**
 * Simplified versions of the above, with exported symbol visibility so that
 * extensions can link to them.
 */

void EXPORT export_linear_regression_as_model_asset(
    const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context);

void EXPORT export_linear_svm_as_model_asset(const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context);

void EXPORT export_logistic_model_as_model_asset(
    const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context);

}

#endif
