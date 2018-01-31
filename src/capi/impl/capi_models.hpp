#ifndef TURI_CAPI_MODELS_H_
#define TURI_CAPI_MODELS_H_ 

#include <capi/TuriCore.h>
#include <unity/lib/toolkit_class_base.hpp>


extern "C" { 

struct tc_model_struct {
  std::shared_ptr<turi::toolkit_class_base> value; 
};

}

static inline tc_model* new_tc_model() { 
  return new tc_model;
}

static inline tc_model* new_tc_model(std::shared_ptr<turi::toolkit_class_base> m) { 
  tc_model* model = new_tc_model(); 
  model->value = std::move(m);
  return model; 
}

// TODO: Merge this function with the one above when merging toolkit_class_base
// with model_base.
static inline tc_model* new_tc_model(std::shared_ptr<turi::model_base> m) {
  return new_tc_model(std::dynamic_pointer_cast<turi::toolkit_class_base>(m));
}

#endif:
