#ifndef TURI_SUPERVISED_LEARNING_CLASSIFIER_EVALUATION_H_
#define TURI_SUPERVISED_LEARNING_CLASSIFIER_EVALUATION_H_

#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/boosted_trees.hpp>


namespace turi {
namespace supervised {

gl_sframe classifier_report_by_class(
    gl_sframe input, const std::string& actual,
    const std::string& predicted);

gl_sframe confusion_matrix(
    gl_sframe input, const std::string& actual,
    const std::string& predicted);

}}




#endif
