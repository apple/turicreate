/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/lambda/python_callbacks.hpp>
#include <core/logging/logger.hpp>

namespace turi { namespace python {

/// Global variables indicating the current exception state.
EXPORT bool _python_exception_occured = false;
static python_exception_info last_exception;

void EXPORT register_python_exception(const python_exception_info* pei) {
  // We shouldn't have a call to this already in flight.
  if(_python_exception_occured) {
    logstream(LOG_ERROR) << "Exception already present when exception is being registered." << std::endl;
    logstream(LOG_ERROR) << "  Exception: " << last_exception.exception_string << std::endl;
  }

  last_exception = *pei;
  _python_exception_occured = true;
}

/** Processes an exception previously registered through a call to
 *  register exception.
 */
void EXPORT _process_registered_exception() {

  // Reset for the next time.
  _python_exception_occured = false;

  // TODO: fill this out.  This should get much more sophisticated.
  throw std::string(last_exception.exception_string);
}

}}
