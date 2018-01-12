#ifndef TURI_CAPI_FLEXIBLE_TYPE_INTERNAL
#define TURI_CAPI_FLEXIBLE_TYPE_INTERNAL

#include <flexible_type/flexible_type.hpp> 

extern "C" { 

struct tc_flexible_type_struct {
  turi::flexible_type value; 
};

}

/****************************************************/

static inline tc_flexible_type* new_tc_flexible_type() { 
  return new tc_flexible_type(); 
}

template <typename T>
static inline tc_flexible_type* new_tc_flexible_type(T&& value) {
  tc_flexible_type* ret = new tc_flexible_type(); 
  ret->value = std::move(value);
  return ret; 
}

#endif
