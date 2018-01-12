#ifndef TURI_CAPI_PARAMETERS_INTERNAL
#define TURI_CAPI_PARAMETERS_INTERNAL

#include <capi/TuriCore.h>
#include <flexible_type/flexible_type.hpp> 
#include <map> 
#include <unity/lib/variant.hpp>

extern "C" { 

struct tc_parameters_struct {
 turi::variant_map_type value;
}; 

}

/****************************************************/

static inline tc_parameters* new_tc_parameters() { 
  return new tc_parameters_struct(); 
}

static inline tc_parameters* new_tc_parameters(turi::variant_map_type v) { 
  tc_parameters* ret = new_tc_parameters();
  ret->value = std::move(v);
  return ret;
}

#endif
