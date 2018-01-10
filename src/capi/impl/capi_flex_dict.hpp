#ifndef TURI_CAPI_FLEX_DICT_INTERNAL
#define TURI_CAPI_FLEX_DICT_INTERNAL

#include <flexible_type/flexible_type.hpp> 
extern "C" {

struct tc_flex_dict_struct { 
  turi::flex_dict value;
};

}

static inline tc_flex_dict* new_tc_flex_dict() { 
  return new tc_flex_dict(); 
}

template <typename T>
static inline tc_flex_dict* new_tc_flex_dict(T&& value) {
  tc_flex_dict* ret = new tc_flex_dict(); 
  ret->value = value; 
  return ret; 
}

#endif // TURI_CAPI_FLEX_DICT_INTERNAL
