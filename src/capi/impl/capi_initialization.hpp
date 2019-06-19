/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CAPI_INITIALIZATION
#define TURI_CAPI_INITIALIZATION

#include <model_server/lib/unity_global.hpp>

namespace turi {

/** The server initializer function.
 *
 *  This function creates the server initializer for the C-API, which is what
 *  determines which models and functions are registered to the unity sever,
 *  which then determines what models are packaged in the framework and
 *  available from the C-API.
 *
 *  In some cases, it may be desirable to have a custom server initializer, for
 *  example if only a subset of the models are needed.  In this case, define the
 *  macro CAPI_DISABLE_DEFAULT_SERVER_INITIALIZER and create a custom
 *  implementation of that function to be compiled in.  In addition, the CMakeLists.txt
 */
std::shared_ptr<turi::unity_server_initializer> capi_server_initializer();

}

#endif
