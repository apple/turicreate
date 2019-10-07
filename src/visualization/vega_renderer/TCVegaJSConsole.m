/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "TCVegaJSConsole.h"
#import "TCVegaLogger.h"

@implementation TCVegaJSConsole

+ (NSString *) arguments {
    NSMutableString *result = [[NSMutableString alloc] init];
    NSArray *arguments = JSContext.currentArguments;
    for (id obj in arguments) {
        [result appendString:@" "];
        if ([obj isKindOfClass:NSString.class]) {
            [result appendString:obj];
        } else if ([obj isKindOfClass:NSObject.class]) {
            [result appendString:((NSObject *)obj).debugDescription];
        } else {
            [result appendString:@"Unknown argument type given to console.log"];
            assert(false);
        }
    }
    return result;
}

+ (void)attachToJavaScriptContext:(JSContext *)context {
    [context evaluateScript:@"var console = {};"];

    context[@"console"][@"log"] = ^() {
        os_log_info(TCVegaLogger.instance, "JS console log: %s", TCVegaJSConsole.arguments.UTF8String);
    };
    context[@"console"][@"warn"] = ^() {
        os_log_info(TCVegaLogger.instance, "JS console warning: %s", TCVegaJSConsole.arguments.UTF8String);
    };
    context[@"console"][@"error"] = ^() {
        os_log_info(TCVegaLogger.instance, "JS console error: %s", TCVegaJSConsole.arguments.UTF8String);
    };

    // set up error handling
    context.exceptionHandler = ^(JSContext *excCtx, JSValue *exception) {
        os_log_info(TCVegaLogger.instance, "Unhandled exception: %s", exception.toString.UTF8String);
        os_log_info(TCVegaLogger.instance, "In context: %s", excCtx.debugDescription.UTF8String);
        os_log_info(TCVegaLogger.instance, "Line %s, column %s", [exception objectForKeyedSubscript:@"line"].toString.UTF8String, [exception objectForKeyedSubscript:@"column"].toString.UTF8String);
        os_log_info(TCVegaLogger.instance, "Stacktrace: %s", [exception objectForKeyedSubscript:@"stack"].toString.UTF8String);
        assert(false);
    };
}

@end
