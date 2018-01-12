#ifndef TURI_CAPI_FLEX_IMAGE_INTERNAL
#define TURI_CAPI_FLEX_IMAGE_INTERNAL

#include <flexible_type/flexible_type.hpp> 

extern "C" {

struct tc_flex_image_struct { 
  turi::flex_image value;
};

}

static inline tc_flex_image* new_tc_flex_image() { 
  return new tc_flex_image(); 
}

template <typename... T>
static inline tc_flex_image* new_tc_flex_image(T&&... value) {
  tc_flex_image* ret = new_tc_flex_image(); 
  ret->value = turi::flex_image(std::move(value)...); 
  return ret; 
}

#endif
