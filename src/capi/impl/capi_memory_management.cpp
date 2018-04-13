#include <capi/TuriCreate.h>

#include <memory>

#include <export.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>

extern "C" {

  struct __tc_wrapper_base {
    capi_struct_type_info* type_info;
  };

 // Destructor function -- universal way to call the destructor for any object.
EXPORT void tc_release(void* dt) {

  if(dt) {
    // Not sure how to make sure this won't segfault on bad input.
    reinterpret_cast<const __tc_wrapper_base*>(dt)->type_info->free(dt);
  }
}

}  // extern "C"
