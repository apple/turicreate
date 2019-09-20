/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "TCVegaLogger.h"

@implementation TCVegaLogger

+ (os_log_t)instance {
    static os_log_t log;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      log = os_log_create("com.apple.turi", "vega_renderer");
    });
    return log;
}

@end
