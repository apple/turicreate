/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FILEIO_SANITIZE_URL_HPP
#define FILEIO_SANITIZE_URL_HPP
#include <string>
namespace turi {
/**
 * \ingroup fileio
 * Sanitizes a general_fstream URL so that it is suitable for printing;
 * right now, all it does is to drop all credential information when
 * the protocol is s3.
 */
std::string sanitize_url(std::string url);
}
#endif
