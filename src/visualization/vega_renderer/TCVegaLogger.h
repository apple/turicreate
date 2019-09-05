/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <os/log.h>

@interface TCVegaLogger : NSObject

/*
 * A preconfigured log object for use with os_log methods.
 */
+ (os_log_t)instance;

@end
