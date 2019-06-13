#include <toolkits/coreml_export/mlmodel_wrapper.hpp>

#include <toolkits/coreml_export/coreml_export_utils.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace coreml {
void MLModelWrapper::save(const std::string& path_to_save_file) {
  CoreML::Result r = m_model->save(path_to_save_file);

  if (!r.good()) {
    log_and_throw("Could not export model: " + r.message());
  }
}

void MLModelWrapper::add_metadata(
    const std::map<std::string, flexible_type>& context_metadata) {
  ::turi::add_metadata(m_model->getProto(), context_metadata);
}

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(MLModelWrapper)
END_CLASS_REGISTRATION
}  // namespace coreml
}  // namespace turi
