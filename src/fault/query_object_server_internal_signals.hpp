/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef QUERY_OBJECT_SERVER_INTERNAL_SIGNALS_HPP
#define QUERY_OBJECT_SERVER_INTERNAL_SIGNALS_HPP
// this defines the set of messages that are passed from the manager to the
// server processes.

#define QO_SERVER_FAIL -1
#define QO_SERVER_STOP 0
#define QO_SERVER_PROMOTE 1
#define QO_SERVER_PRINT 2


#define QO_SERVER_FAIL_STR "-1\n"
#define QO_SERVER_STOP_STR "0\n"
#define QO_SERVER_PROMOTE_STR "1\n"
#define QO_SERVER_PRINT_STR "2\n"


#endif
