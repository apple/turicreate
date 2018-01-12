#ifndef TURI_CAPI_DATETIME_INTERNAL
#define TURI_CAPI_DATETIME_INTERNAL

#include <capi/TuriCore.h>
#include <flexible_type/flexible_type.hpp> 

extern "C" { 

struct tc_datetime_struct {
 turi::flex_date_time value;
}; 

}

/****************************************************/

static inline tc_datetime_struct* new_tc_datetime() { 
  return new tc_datetime_struct(); 
}

static inline tc_datetime_struct* new_tc_datetime(const turi::flex_date_time& t) { 
  tc_datetime_struct* ret = new_tc_datetime();
  ret->value = t;
  return ret;
}

#endif
