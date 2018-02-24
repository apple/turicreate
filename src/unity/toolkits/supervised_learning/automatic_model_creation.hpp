#ifndef TURI_AUTOMATIC_SUPERVISED_LEARNING_H_
#define TURI_AUTOMATIC_SUPERVISED_LEARNING_H_

#include <unity/toolkits/supervised_learning/supervised_learning.hpp>
#include <unity/toolkits/supervised_learning/boosted_trees.hpp>


namespace turi {
namespace supervised {

std::shared_ptr<supervised_learning_model_base> create_automatic_classifier_model(
    gl_sframe data, const std::string target, gl_sframe validation_data,
    const std::map<std::string, flexible_type>& options);

std::shared_ptr<supervised_learning_model_base> create_automatic_regression_model(
    gl_sframe data, const std::string target, gl_sframe validation_data,
    const std::map<std::string, flexible_type>& options); 

}}

#endif
