/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef VegaCSSCursorMap_h
#define VegaCSSCursorMap_h

#import <TargetConditionals.h>

#ifdef TARGET_OS_OSX

#import <AppKit/AppKit.h>

@interface TCVegaCSSCursorMap: NSObject

+ (NSCursor *) cursorWithCSSName:(NSString *)cssName;

@end

#endif

#endif /* VegaCSSCursorMap_h */
