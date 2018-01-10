#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_flex_dict.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <export.hpp>
#include <flexible_type/flexible_type.hpp>

extern "C" {

EXPORT tc_flex_dict* tc_flex_dict_create(tc_error** error) {
  ERROR_HANDLE_START();
  return new_tc_flex_dict();
  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_flex_dict_destroy(tc_flex_dict* fd) {
  if (fd) {
    delete fd;
  }
}

// Adds a key to the dictionary, returning the entry index..
EXPORT uint64_t tc_flex_dict_add_element(tc_flex_dict* ft,
                                         const tc_flexible_type* first,
                                         const tc_flexible_type* second,
                                         tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, ft, "flex dict", uint64_t(-1)); 
  CHECK_NOT_NULL(error, first, "key flexible_type", uint64_t(-1)); 
  CHECK_NOT_NULL(error, second, "value flexible_type", uint64_t(-1)); 

  uint64_t ret = ft->value.size(); 
  ft->value.push_back({first->value, second->value}); 
  return ret; 

  ERROR_HANDLE_END(error, uint64_t(-1));
}

// Extract the (key, value) pair corresponding to the entry at entry_index.
EXPORT void tc_flex_dict_extract_entry(const tc_flex_dict* ft,
                                       uint64_t entry_index,
                                       tc_flexible_type* key_dest,
                                       tc_flexible_type* value_dest,
                                       tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, ft, "flex dict"); 
  CHECK_NOT_NULL(error, key_dest, "key dest flexible_type"); 
  CHECK_NOT_NULL(error, value_dest, "value dest flexible_type"); 
  
  if(entry_index >= ft->value.size()) {
    set_error(error, "Index out of range.");
    return; 
  } 

  const std::pair<turi::flexible_type, turi::flexible_type>& p = ft->value[entry_index]; 

  key_dest->value = p.first; 
  value_dest->value = p.second; 

  ERROR_HANDLE_END(error);
}

}
