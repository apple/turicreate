#include <model_server_v2/model_base.hpp>

namespace turi {
namespace v2 {

model_base::model_base() 
  : m_registry(new method_registry<model_base>())
{
  register_method("name", &model_base::name);
}
  

model_base::~model_base() { }

}
}

