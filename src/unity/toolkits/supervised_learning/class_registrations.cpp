#include <unity/toolkits/supervised_learning/class_registrations.hpp>

#include <unity/toolkits/supervised_learning/boosted_trees.hpp>
#include <unity/toolkits/supervised_learning/decision_tree.hpp>
#include <unity/toolkits/supervised_learning/random_forest.hpp>
#include <unity/toolkits/supervised_learning/linear_regression.hpp>
#include <unity/toolkits/supervised_learning/linear_svm.hpp>
#include <unity/toolkits/supervised_learning/logistic_regression.hpp>

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


}  // namespace supervised
}  // namespace turi
