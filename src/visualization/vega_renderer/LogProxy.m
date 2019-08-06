/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "LogProxy.h"
#import "LogProxyHandler.h"

@implementation LogProxy

+ (os_log_t)logger {
    static os_log_t log;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      log = os_log_create("com.apple.turi", "vega_renderer");
    });
    return log;
}

+ (JSValue *)wrap:(JSValue *)instance {
  return [LogProxy wrap:instance withHandler:[[LogProxyHandler alloc] init]];
}

+ (JSValue *)wrapObject:(NSObject *)object {
  // Assumes we have a current JSC context
  return [LogProxy wrap:[JSValue valueWithObject:object inContext:[JSContext currentContext]]];
}

+ (JSValue *)wrap:(JSValue *)instance
   withHandler:(id<LogProxyHandling>)handler {
  instance.context[@"__tmp_instance"] = instance;
  instance.context[@"__tmp_handler"] = handler;
  instance.context[@"__tmp_proxy"] = [instance.context evaluateScript:@"new Proxy(__tmp_instance, __tmp_handler);"];
  [instance.context evaluateScript:@""
	    "Object.defineProperty(__tmp_proxy, '__LogProxy_wrapped', {"
	    "  enumerable: true,"
      "  value: __tmp_instance,"
      "});"];
  return instance.context[@"__tmp_proxy"];
}

+ (id)tryUnwrap:(id)object {
  if ([object isKindOfClass:[JSValue class]]) {
    if ([object hasProperty:@"__LogProxy_wrapped"]) {
      return object[@"__LogProxy_wrapped"];
    }
  }

  // It doesn't appear to be a Proxy object, or at least not
  // one wrapped by LogProxy.
  return object;
}

+ (JSValue *)unwrap:(JSValue *)instance {
  assert([instance isKindOfClass:[JSValue class]]);

  if ([instance hasProperty:@"__LogProxy_wrapped"]) {
    return instance[@"__LogProxy_wrapped"];
  }

  // It doesn't appear to be a Proxy object, or at least not
  // one wrapped by LogProxy.
  assert(false);
  return nil;
}

@end