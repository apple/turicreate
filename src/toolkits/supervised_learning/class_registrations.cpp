#include <toolkits/supervised_learning/class_registrations.hpp>

#include <toolkits/supervised_learning/boosted_trees.hpp>
#include <toolkits/supervised_learning/decision_tree.hpp>
#include <toolkits/supervised_learning/random_forest.hpp>
#include <toolkits/supervised_learning/linear_regression.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <toolkits/supervised_learning/classifier_evaluations.hpp>
#include <toolkits/supervised_learning/automatic_model_creation.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>

namespace turi { namespace supervised {

using namespace xgboost;

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(boosted_trees_classifier)
REGISTER_CLASS(boosted_trees_regression)
REGISTER_CLASS(decision_tree_classifier)
REGISTER_CLASS(decision_tree_regression)
REGISTER_CLASS(random_forest_regression)
REGISTER_CLASS(random_forest_classifier)
REGISTER_CLASS(linear_regression)
REGISTER_CLASS(linear_svm)
REGISTER_CLASS(logistic_regression)
END_CLASS_REGISTRATION


/**
 * Defines get_toolkit_function_registration for the supervised_learning toolkit
 */
BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_classifier_model_selector, "_X");
REGISTER_FUNCTION(_classifier_available_models, "num_classes", "_X");
REGISTER_FUNCTION(_get_metadata_mapping, "model");

/*  Register the methods for automatic model creation.
 *
 *
 */
REGISTER_FUNCTION(classifier_report_by_class, "data", "target", "predicted");
REGISTER_FUNCTION(confusion_matrix, "data", "target", "predicted");

REGISTER_FUNCTION(create_automatic_classifier_model, "data", "target", "validation_data", "options");
REGISTER_FUNCTION(create_automatic_regression_model, "data", "target", "validation_data", "options");
END_FUNCTION_REGISTRATION

}  // namespace supervised
}  // namespace turi
