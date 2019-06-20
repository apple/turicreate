/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_BRANCH_HINTS_HPP
#define TURI_UTIL_BRANCH_HINTS_HPP

/**
 * \ingroup util
 * Sets a branch as likely.
 *
 * \code
 * if (__likely__(age < 100)) {
 *    ...
 * } else if (__unlikely__(age >= 100)) {
 *    ...
 * }
 * \endcode
 */
#define __likely__(x)       __builtin_expect((x),1)

/**
 * \ingroup util
 * Sets a branch as unlikely.
 *
 * \code
 * if (__likely__(age < 100)) {
 *    ...
 * } else if (__unlikely__(age >= 100)) {
 *    ...
 * }
 * \endcode
 */
#define __unlikely__(x)     __builtin_expect((x),0)

#endif //TURI_UTIL_BRANCH_HINTS_HPP
