#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_flex_list.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <export.hpp>
#include <flexible_type/flexible_type.hpp> 

extern "C" { 

/******************************************************************************/
/*                                                                            */
/*    flex_list                                                               */
/*                                                                            */
/******************************************************************************/


EXPORT tc_flex_list* tc_flex_list_create(tc_error** error) { 
ERROR_HANDLE_START();

return new tc_flex_list; 

ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_list* tc_flex_list_create_with_capacity(uint64_t capacity, tc_error** error) {
  ERROR_HANDLE_START();

  tc_flex_list* ret = new tc_flex_list; 
  ret->value.reserve(capacity);
  return ret; 

  ERROR_HANDLE_END(error, NULL);
}

/**
 */
EXPORT uint64_t tc_flex_list_add_element(tc_flex_list* fl, const tc_flexible_type* ft,
   tc_error** error) {

  ERROR_HANDLE_START();

  if(fl == NULL) {
    set_error(error, "tc_flex_list instance null.");
    return uint64_t(-1);
  }

  if(ft == NULL) {
    set_error(error, "tc_flexible_type instance null.");
    return uint64_t(-1);
  }

  uint64_t pos = fl->value.size();
  fl->value.push_back(ft->value);
  return pos; 
    
  ERROR_HANDLE_END(error, uint64_t(-1));
}

/** Extract an element at a specific index.
 *
 */
EXPORT tc_flexible_type* tc_flex_list_extract_element(
    const tc_flex_list* fl, uint64_t index, tc_error **error) {

  ERROR_HANDLE_START();
  
  if(fl == NULL) {
    set_error(error, "tc_flex_list instance null.");
    return NULL;
  }

  tc_flexible_type* ft = tc_ft_create_empty(error);
  
  if(*error) { return NULL; }
  
  if(index >= fl->value.size()) {
    set_error(error, "tc_flex_list index out of bounds.");
    return NULL;
  }

  ft->value = fl->value[index];

  return ft; 
  
  ERROR_HANDLE_END(error, NULL);
}


EXPORT uint64_t tc_flex_list_size(const tc_flex_list* fl) { 
  if(fl == NULL) { return 0; }
  return fl->value.size(); 
}

EXPORT void tc_flex_list_destroy(tc_flex_list* fl) { 
  delete fl; 
}


}

