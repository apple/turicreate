/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LAMBDA_PYLAMBDA_WORKER_H_
#define TURI_LAMBDA_PYLAMBDA_WORKER_H_

#include <string>

namespace turi { namespace lambda {

/** The main function to be called from the python ctypes library to
 *  create a pylambda worker process.
 */
int pylambda_worker_main(const std::string& root_path,
                         const std::string& server_address, int loglevel);

}}

#endif /* _PYLAMBDA_WORKER_H_ */
