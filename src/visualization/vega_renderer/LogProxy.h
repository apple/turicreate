/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <JavaScriptCore/JavaScriptCore.h>
#include <os/log.h>

#import "LogProxyHandler.h"

@interface LogProxy : NSObject

/*
 * Logs all property accesses using os_log_info, and will log missing
 * properties with os_log_error, using subsystem "com.apple.turi" and component
 * "vega_renderer".
 */
+ (JSValue *)wrap:(JSValue *)instance;

/*
 * Logs all property accesses using os_log_info, and will log missing
 * properties with os_log_error, using subsystem "com.apple.turi" and component
 * "vega_renderer".
 */
+ (JSValue *)wrapObject:(NSObject *)object;

/*
 * Takes a handler to wrap the instance with; all property accesses will go
 * through this handler, and the handler should return the property value.
 */
+ (JSValue *)wrap:(JSValue *)instance
      withHandler:(id<LogProxyHandling>)handler;

/*
 * Takes a LogProxy, or any other object type.
 * If object is a LogProxy wrapper, returns the wrapped object.
 * Otherwise, returns the object passed in.
 */
+ (id)tryUnwrap:(id)object;

/*
 * Takes a LogProxy wrapped object.
 * If object is a LogProxy wrapper, returns the wrapped object.
 * Otherwise, returns nil.
 */
+ (JSValue *)unwrap:(JSValue *)object;

/*
 * A preconfigured log object for use with os_log methods.
 */
+ (os_log_t)logger;

@end