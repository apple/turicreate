#ifndef TURI_AUTOMATIC_SUPERVISED_LEARNING_H_
#define TURI_AUTOMATIC_SUPERVISED_LEARNING_H_

#include <toolkits/supervised_learning/supervised_learning.hpp>

namespace turi {
namespace supervised {

/**
 * Rule based better than stupid model selector.
 */
std::string _classifier_model_selector(std::shared_ptr<unity_sframe> _X);

/**
 * Rule-based method for getting a list of potential models.
 */
std::vector<std::string> _classifier_available_models(
    size_t num_classes, std::shared_ptr<unity_sframe> _X);

std::shared_ptr<supervised_learning_model_base> create_automatic_classifier_model(
    gl_sframe data, const std::string target, const variant_type& validation_data,
    const std::map<std::string, flexible_type>& options);

std::shared_ptr<supervised_learning_model_base> create_automatic_regression_model(
    gl_sframe data, const std::string target, const variant_type& validation_data,
    const std::map<std::string, flexible_type>& options);

std::pair<gl_sframe, gl_sframe> create_validation_data(
    gl_sframe data, const variant_type& validation_data, int random_seed);

std::pair<gl_sframe, gl_sframe> create_validation_data(
    gl_sframe data, const variant_type& validation_data);

}  // namespace supervised
}  // namespace turi

#endif
