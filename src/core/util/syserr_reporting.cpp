/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/util/syserr_reporting.hpp>
#include <iostream>
#ifdef _WIN32
#include <cross_platform/windows_wrapper.hpp>
#else
#include <errno.h>
#include <string.h>
#endif

std::string get_last_err_str(unsigned err_code) {
  std::string ret_str;
#ifdef _WIN32
  LPVOID msg_buf;

  auto rc = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      err_code,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&msg_buf,
      0,
      NULL);

  if(rc) {
    ret_str = std::string((char *)msg_buf);
    LocalFree(msg_buf);
  } else {
    ret_str = std::string("");
  }
#else
  //WARNING: This is untested.  I just added it because I was writing the
  //Windows function.
  ret_str = std::string(strerror((int)err_code));
#endif //_WIN32
  return ret_str;
}
