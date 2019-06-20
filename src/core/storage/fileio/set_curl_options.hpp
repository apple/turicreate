/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FILEIO_SET_CURL_OPTIONS_HPP
#define TURI_FILEIO_SET_CURL_OPTIONS_HPP
namespace turi {
namespace fileio {

/**
 * \internal
 * \ingroup fileio
 * Sets curl options for everywhere curl is used.
 */
void set_curl_options(void* ecurl);
}
}
#endif
