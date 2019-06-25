/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef UTIL_MD5_HPP
#define UTIL_MD5_HPP
#include <string>
namespace turi {

/**
 * \ingroup util
 * Computes the md5 checksum of a string, returning the md5 checksum in a
 * hexadecimal string
 */
std::string md5(std::string val);

/**
 * \ingroup util
 * Returns a 16 byte (non hexadecimal) string of the raw md5.
 */
std::string md5_raw(std::string val);

} // namespace turi
#endif
