/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import "TCVegaJSMouseEvent.h"

#import <TargetConditionals.h>

@implementation TCVegaJSMouseEvent

@synthesize clientX;
@synthesize clientY;
@synthesize movementX;
@synthesize movementY;

-(instancetype)initWithEvent:(TCVegaRendererNativeEvent*)event view:(TCVegaRendererNativeView*)view height:(double)height {
    self = [super init];
    if(self) {
#if TARGET_OS_OSX
        NSPoint location = [view convertPoint:event.locationInWindow fromView:nil];
        self.clientX = location.x;
        self.clientY = height - location.y; // translate between coordinate systems
        self.movementX = event.deltaX;
        self.movementY = event.deltaY;
#else
        (void)event;
        (void)view;
        (void)height;
#endif
    }
    return self;
}

-(void)preventDefault {
    // we don't need to do anything
}
-(void)stopPropagation {
    // NOTE: this method should prevent the event from propagating further up the
    // DOM, however, since we don't implement the hierarchical structure of
    // the DOM there isn't an obvious way to implement this. Currently, our lack
    // of a proper implementation doesn't seem to break anything.
}

@end
