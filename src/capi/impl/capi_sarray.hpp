#ifndef TURI_CAPI_SARRAY_H
#define TURI_CAPI_SARRAY_H

#include <unity/lib/gl_sarray.hpp>

extern "C" {

struct tc_sarray_struct {
  turi::gl_sarray value;
};

}

static inline tc_sarray* new_tc_sarray() {
  return new tc_sarray;
}

template <typename T>
static inline tc_sarray* new_tc_sarray(T&& value) {
  tc_sarray* t = new tc_sarray();
  t->value = turi::gl_sarray(value);
  return t;
}

#endif
