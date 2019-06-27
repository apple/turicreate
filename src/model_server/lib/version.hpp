/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_VERSION_HPP
#define TURI_UNITY_VERSION_HPP
#include "version_number.hpp"

#ifdef __UNITY_VERSION__
#define UNITY_VERSION __UNITY_VERSION__
#else
#define UNITY_VERSION "0.1.internal"
#endif
#endif
