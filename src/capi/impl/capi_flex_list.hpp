#ifndef TURI_CAPI_FLEX_LIST_INTERNAL
#define TURI_CAPI_FLEX_LIST_INTERNAL

#include <flexible_type/flexible_type.hpp>
extern "C" {

struct tc_flex_list_struct {
  turi::flex_list value;
};

}

static inline tc_flex_list* new_tc_flex_list() {
  return new tc_flex_list();
}

template <typename T>
static inline tc_flex_list* new_tc_flex_list(T&& value) {
  tc_flex_list* ret = new tc_flex_list();
  ret->value = std::move(value);
  return ret;
}

#endif
