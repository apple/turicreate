
#ifndef TURI_CAPI_VARIANT_TYPE_INTERNAL
#define TURI_CAPI_VARIANT_TYPE_INTERNAL

#include <unity/lib/variant.hpp>
#include <unity/lib/variant_converter.hpp>
#include <capi/TuriCore.h>

extern "C" {

struct tc_variant_struct {
  turi::variant_type value;
};

}

/****************************************************/

static inline tc_variant* new_tc_variant() {
  return new tc_variant();
}

template <typename T>
static inline tc_variant* new_tc_variant(T&& value) {
  tc_variant* ret = new tc_variant();
  ret->value = turi::to_variant(value);
  return ret;
}

#endif
