/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@interface VegaRenderer : NSObject

- (instancetype)initWithSpec:(NSString*)spec;
- (instancetype)initWithSpec:(NSString*)spec
                     context:(CGContextRef)parentContext;

-(NSUInteger)width;
-(NSUInteger)height;
-(CGImageRef)CGImage;

// JS dependencies
+(NSString *)vegaJS;
+(NSString *)vegaliteJS;

@end
