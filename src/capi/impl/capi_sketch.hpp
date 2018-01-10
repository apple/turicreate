#ifndef TURI_CAPI_SKETCH_H
#define TURI_CAPI_SKETCH_H

#include <capi/impl/capi_sarray.hpp>
#include <unity/lib/unity_sketch.hpp>

extern "C" { 

struct tc_sketch_struct { 
  std::shared_ptr<turi::unity_sketch_base> value;
};

}

static inline tc_sketch* new_tc_sketch(const std::shared_ptr<turi::unity_sketch_base>& other) {
  tc_sketch* ret = new tc_sketch;
  ret->value = other;
  return ret;
}

static inline tc_sketch* new_tc_sketch() { 
  return new_tc_sketch(std::make_shared<turi::unity_sketch>());
}

#endif // TURI_CAPI_SKETCH_H
