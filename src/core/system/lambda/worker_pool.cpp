/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/globals/globals.hpp>
#include <core/export.hpp>

namespace turi {

/** The timeout for connecting to a lambda worker, in seconds.
 *
 *  Set to 0 to attempt one try and exit immediately on failure.
 *
 *  Set to -1 to disable timeout completely.
 */
EXPORT double LAMBDA_WORKER_CONNECTION_TIMEOUT = 60;

REGISTER_GLOBAL(double, LAMBDA_WORKER_CONNECTION_TIMEOUT, true)

}
