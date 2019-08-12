/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "JSConsole.h"

@implementation JSConsole

+ (void)attachToJavaScriptContext:(JSContext *)context {
    [context evaluateScript:@"var console = {};"];

    context[@"console"][@"log"] = ^() {
        NSArray *message = [JSContext currentArguments];
        NSLog(@"JS console log: %@", message);
    };
    context[@"console"][@"warn"] = ^() {
        NSArray *message = [JSContext currentArguments];
        NSLog(@"JS console warning: %@", message);
    };
    context[@"console"][@"error"] = ^() {
        NSArray *message = [JSContext currentArguments];
        NSLog(@"JS console error: %@", message);
    };

    // set up error handling
    context.exceptionHandler = ^(JSContext *context, JSValue *exception) {
        NSLog(@"Unhandled exception: %@", [exception toString]);
        NSLog(@"In context: %@", [context debugDescription]);
        NSLog(@"Line %@, column %@", [exception objectForKeyedSubscript:@"line"], [exception objectForKeyedSubscript:@"column"]);
        NSLog(@"Stacktrace: %@", [exception objectForKeyedSubscript:@"stack"]);
        assert(false);
    };
}

@end