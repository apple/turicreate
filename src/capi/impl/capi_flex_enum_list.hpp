#ifndef TURI_CAPI_FLEX_ENUM_LIST_INTERNAL
#define TURI_CAPI_FLEX_ENUM_LIST_INTERNAL

#include <flexible_type/flexible_type.hpp>
#include <vector>

extern "C" {
  struct tc_flex_enum_list_struct {
    std::vector<turi::flex_type_enum> value;
  };
}

static inline tc_flex_enum_list* new_tc_flex_enum_list() {
  return new tc_flex_enum_list();
}

#endif
