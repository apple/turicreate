
#ifndef TURI_CAPI_SFRAME_H
#define TURI_CAPI_SFRAME_H

#include <unity/lib/gl_sframe.hpp>

extern "C" { 

struct tc_sframe_struct { 
  turi::gl_sframe value;
}; 

}

static inline tc_sframe* new_tc_sframe() { 
  return new tc_sframe;
}

template <typename... Args> 
static inline tc_sframe* new_tc_sframe(Args... args) { 
  tc_sframe* t = new tc_sframe();
  t->value = turi::gl_sframe(args...);
  return t; 
}

#endif 
