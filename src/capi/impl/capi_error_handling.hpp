#ifndef TURI_CAPI_ERROR_HANDLING_INTERNAL
#define TURI_CAPI_ERROR_HANDLING_INTERNAL

#include <capi/impl/capi_wrapper_structs.hpp>
#include <exception>
#include <string>
#include <util/code_optimization.hpp>


/**  Define the start of the exception handling block.
 *
 */
#define ERROR_HANDLE_START() \
  try {                      \
    do {                     \
  } while (false)

/** Defines the end of the exception handling block.
 *
 *  Takes one required and one option argument.  First argument is the
 *  name of the tc_error** variable.  Second argument is the return value
 *  on error.
 */
#define ERROR_HANDLE_END(error_var, ...)                            \
  }                                                                 \
  catch (...) {                                                     \
    fill_error_from_exception(std::current_exception(), error_var); \
    return __VA_ARGS__;                                             \
  }                                                                 \
  do {                                                              \
  } while (false)

#define CHECK_NOT_NULL(error_var, var, name, ...) \
  do {                                            \
    if (var == NULL) {                            \
      set_error(error_var, #name " is null.");    \
      return __VA_ARGS__;                         \
    }                                             \
  } while (false)

void __set_null_error(tc_error** error, int pos, const char* struct_type,
                      const char* function);
void __set_type_error(tc_error** error, int pos, const char* struct_name,
                      const char* function, const char* actual_type);

#define CHECK_INPUT_STRUCT(pos, var, struct_name, error, ...)      \
  do {                                                             \
    if (UNLIKELY(var == NULL)) {                                   \
      __set_null_error(error, pos, #struct_name, __FUNCTION__);    \
      return __VA_ARGS__;                                          \
    }                                                              \
    /* Now do the rest of the type checking.   */                  \
    if (UNLIKELY(var->type_info !=                                 \
                 &(capi_struct_type_info_##struct_name##_inst))) { \
      __set_type_error(error, pos, #struct_name, __FUNCTION__,     \
                       var->type_info->name());                    \
      return __VA_ARGS__;                                          \
    }                                                              \
  } while (false)

// Fill error from a thrown exception
void fill_error_from_exception(std::exception_ptr eptr, tc_error** error);


// Fill the error from an error string
void set_error(tc_error** error, const std::string& message);


#endif
