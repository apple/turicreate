/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CAPI_ERROR_HANDLING_INTERNAL
#define TURI_CAPI_ERROR_HANDLING_INTERNAL

#include <capi/impl/capi_wrapper_structs.hpp>
#include <exception>
#include <string>


/**  Define the start of the exception handling block.
 *
 */
#define ERROR_HANDLE_START() \
  try { do {} while(false)


/** Defines the end of the exception handling block.
 *
 *  Takes one required and one option argument.  First argument is the
 *  name of the tc_error** variable.  Second argument is the return value
 *  on error.
 */
#define ERROR_HANDLE_END(error_var, ...)                            \
  } catch(...) { \
    fill_error_from_exception(std::current_exception(), error_var); \
    return __VA_ARGS__;                                             \
  } do{} while(false)

#define CHECK_NOT_NULL(error_var, var, name, ...) \
  do {                                            \
    if (var == NULL) {                            \
      set_error(error_var, #name " is null.");    \
      return __VA_ARGS__;                         \
    }                                             \
  } while (false)



// Fill error from a thrown exception
void fill_error_from_exception(std::exception_ptr eptr, tc_error** error);


// Fill the error from an error string
void set_error(tc_error** error, const std::string& message);


#endif
