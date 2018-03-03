#include <unity/toolkits/supervised_learning/automatic_model_creation.hpp>
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>
#include <unity/toolkits/supervised_learning/boosted_trees.hpp>


namespace turi {
namespace supervised {

using namespace xgboost; 


std::shared_ptr<supervised_learning_model_base> create_automatic_classifier_model(
    gl_sframe data, const std::string target, gl_sframe validation_data,
    const std::map<std::string, flexible_type>& options) {

  // TODO: If validation set is empty, create using random sample.

  // TODO: try logisitic regression + SVM + boosted trees.
  
  // TODO: Choose best based on validation set accuracy.
 
  auto m = std::make_shared<boosted_trees_classifier>(); 

  m->api_train(data, target, validation_data, options); 

  return m; 
}

std::shared_ptr<supervised_learning_model_base> create_automatic_regression_model(
    gl_sframe data, const std::string target, gl_sframe validation_data,
    const std::map<std::string, flexible_type>& options) {

  // TODO: If validation set is empty, create using random sample.

  // TODO: try linear regression + SVM + boosted trees.
  
  // TODO: Choose best based on validation set accuracy.
 
  auto m = std::make_shared<boosted_trees_regression>(); 

  m->api_train(data, target, validation_data, options); 

  return m; 
}


}
}
