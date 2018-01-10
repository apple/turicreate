#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <string>
#include <export.hpp> 

// These 
extern "C" { 


/** Retrieves the error message on an active error.
 *
 *  Return object is a null-terminated c-style message string. 
 *
 *  The char buffer returned is invalidated by calling tc_error_destroy. 
 */
EXPORT const char* tc_error_message(const tc_error* error) {
  if(error == NULL) { 
    return "No Error"; 
  } else { 
    return error->message.c_str();
  }
} 


/** Destroys an error structure, deallocating error content data. 
 *
 *  Only needs to be called if an error occured.
 *
 *  Sets the pointer to the error struct to NULL. 
 */
EXPORT void tc_error_destroy(tc_error** error_ptr) {
  if(*error_ptr != NULL) {
    delete (*error_ptr);
  }

  *error_ptr = NULL; 
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
  } catch (...) { 
    set_error(error, "Unknown internal error occurred.");
  }
}

void set_error(tc_error** error, const std::string& message) { 
  if(*error != NULL) { 
    tc_error_destroy(error);
  }

  *error = new tc_error;

  (*error)->message = message;
}


