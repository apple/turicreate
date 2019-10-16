/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import "TCVegaPortabilityTypes.h"
#import <JavaScriptCore/JavaScriptCore.h>

@protocol TCVegaJSMouseEventInterface<JSExport>

@property (nonatomic) double clientX;
@property (nonatomic) double clientY;
@property (nonatomic) double movementX;
@property (nonatomic) double movementY;

- (instancetype) initWithEvent:(TCVegaRendererNativeEvent*)event view:(TCVegaRendererNativeView*)view height:(double)height;
- (void) preventDefault;
- (void) stopPropagation;
@end

@interface TCVegaJSMouseEvent : NSObject<TCVegaJSMouseEventInterface>
@end
