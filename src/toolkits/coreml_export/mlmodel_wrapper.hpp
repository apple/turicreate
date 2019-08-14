#ifndef __TC_ML_MODEL_WRAPPER_HPP_
#define __TC_ML_MODEL_WRAPPER_HPP_

#include <memory>

#include <model_server/lib/extensions/model_base.hpp>
#include <model_server/lib/toolkit_class_macros.hpp>

// Forward declare CoreML::Model in lieu of including problematic protocol
// buffer headers.
namespace CoreML {
class Model;
}

namespace turi {
namespace coreml {

class MLModelWrapper : public model_base {
 public:
  MLModelWrapper() {}
  MLModelWrapper(std::shared_ptr<CoreML::Model> model) : m_model(std::move(model)) {}

  void save(const std::string& path_to_save_file);

  void add_metadata(const std::map<std::string, flexible_type>& context_metadata);

  std::shared_ptr<CoreML::Model> coreml_model() const { return m_model; }

 private:
  std::shared_ptr<CoreML::Model> m_model;

  BEGIN_CLASS_MEMBER_REGISTRATION("_MLModelWrapper")
  REGISTER_CLASS_MEMBER_FUNCTION(MLModelWrapper::save, "path")
  REGISTER_CLASS_MEMBER_FUNCTION(MLModelWrapper::add_metadata,
                                 "context_metadata")
  END_CLASS_MEMBER_REGISTRATION
};
}  // namespace coreml
}  // namespace turi

#endif
