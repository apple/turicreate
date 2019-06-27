/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TEST_LAMBDA_DUMMY_WORKER_INTERFACE
#define TURI_TEST_LAMBDA_DUMMY_WORKER_INTERFACE

#include <core/system/cppipc/cppipc.hpp>

GENERATE_INTERFACE_AND_PROXY(dummy_worker_interface, dummy_worker_proxy,
      (std::string, echo, (const std::string&))
      (void, throw_error, )
      (void, quit, (int))
    );

#endif
