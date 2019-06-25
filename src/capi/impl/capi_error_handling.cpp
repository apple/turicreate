/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <string>
#include <core/export.hpp>

// These
extern "C" {


/** Retrieves the error message on an active error.
 *
 *  Return object is a null-terminated c-style message string.
 *
 *  The char buffer returned is invalidated by calling tc_release.
 */
EXPORT const char* tc_error_message(const tc_error* error) {
  if(error == NULL) {
    return "No Error";
  } else {
    return error->value.c_str();
  }
}

} // End the extern "C" block



/********************************************************/

// The primary error handling code.
void fill_error_from_exception(std::exception_ptr eptr, tc_error** error) {

  try {
    if(eptr) {
      std::rethrow_exception(eptr);
    }
  } catch (const std::exception& e) {
    set_error(error, std::string("Error: ") + e.what());
  } catch (const std::string& s) {
    set_error(error, "Error: " + s);
  } catch (const char* c) {
    set_error(error, std::string("Error: ") + c);
  } catch (...) {
    set_error(error, "Unknown internal error occurred.");
  }
}

void set_error(tc_error** error, const std::string& message) {
  if(*error != NULL) {
    tc_release(error);
  }

  *error = new_tc_error();

  (*error)->value = message;
}
