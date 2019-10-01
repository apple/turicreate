/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "TCVegaJSMouseEvent.h"

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@class JSContext;

@interface TCVegaRenderer : NSObject
@property (atomic, strong) JSContext *context;

- (instancetype)init;
- (instancetype)initWithSpec:(NSString*)spec;
- (instancetype)initWithSpec:(NSString*)spec
                      config:(NSString*)config
                 scaleFactor:(double)scaleFactor;

- (void)triggerEventWithType:(NSString*)type
                       event:(TCVegaJSMouseEvent*)event;

-(NSUInteger)width;
-(NSUInteger)height;
-(NSString*)cursor;
-(CGImageRef)CGImage;

// JS dependencies
+(NSString *)vegaJS;
+(NSString *)vegaliteJS;

@end
