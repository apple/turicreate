#ifndef TURI_DRAWING_CLASSIFIER_H_
#define TURI_DRAWING_CLASSIFIER_H_

#include <map>
#include <string>
#include <vector>

#include <unity/lib/extensions/ml_model.hpp>

namespace turi {
namespace drawing_classifier {

class EXPORT drawing_classifier : public ml_model_base {
public:
  void init_model(std::map<std::string, std::vector<float>> weights);

  

  BEGIN_CLASS_MEMBER_REGISTRATION("object_detector")
  IMPORT_BASE_CLASS_REGISTRATION(ml_model_base);
  REGISTER_CLASS_MEMBER_FUNCTION(drawing_classifier::init_model, "weights");
  END_CLASS_MEMBER_REGISTRATION
};

} // namespace drawing_classifier
} // namespace turi

#endif