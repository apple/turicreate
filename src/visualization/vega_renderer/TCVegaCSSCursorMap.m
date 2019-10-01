/* Copyright © 2019 Apple Inc. All rights reserved.
*
* Use of this source code is governed by a BSD-3-clause license that can
* be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/

#import "TCVegaCSSCursorMap.h"

#ifdef TARGET_OS_OSX

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

@implementation TCVegaCSSCursorMap

+ (NSCursor *) cursorWithCSSName:(NSString *)cssName {
    NSString* cssNameLowerCase = [cssName lowercaseString];
    static NSDictionary* dictionary = nil;
    if(dictionary == nil) {
        dictionary = @{
            // CSS cursors with direct NSCursor correspondence.
            @"default": [NSCursor arrowCursor],
            @"crosshair": [NSCursor crosshairCursor],
            @"pointer": [NSCursor pointingHandCursor],
            @"grab": [NSCursor openHandCursor],
            @"grabbing": [NSCursor closedHandCursor],
            @"text": [NSCursor IBeamCursor],
            @"vertical-text": [NSCursor IBeamCursorForVerticalLayout],
            @"not-allowed": [NSCursor operationNotAllowedCursor],
            @"context-menu": [NSCursor contextualMenuCursor],
            @"alias": [NSCursor dragLinkCursor],
            @"copy": [NSCursor dragCopyCursor],
            // Resize cursors look a bit different between browser and NSCursor
            @"col-resize": [NSCursor resizeLeftRightCursor],
            @"row-resize":[NSCursor resizeUpDownCursor],
            @"n-resize": [NSCursor resizeUpCursor],
            @"s-resize": [NSCursor resizeDownCursor],
            @"e-resize": [NSCursor resizeRightCursor],
            @"w-resize": [NSCursor resizeLeftCursor]
            // TODO: below are not available from NSCursor, images are needed
            // move, help, progress, wait, cell, no-drop, all-scroll,
            // {ne, nw, se, sw}-resize, zoom-in, zoom-out
        };
    }
    NSCursor* cursor = [dictionary valueForKey:cssNameLowerCase];
    if(cursor != nil) {
        return cursor;
    } else {
#ifndef NDEBUG
        NSLog(@"unknown cursor: %@\n", cssName);
#endif
        return [NSCursor arrowCursor];
    }
}

@end

#endif
