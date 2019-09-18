//
//  VegaPortabilityTypes.h
//
//  Created by Zachary Nation on 8/20/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#ifndef VegaPortabilityTypes_h
#define VegaPortabilityTypes_h

#import <TargetConditionals.h>

#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
typedef NSColor TCVegaRendererNativeColor;
typedef NSEvent TCVegaRendererNativeEvent;
typedef NSFont TCVegaRendererNativeFont;
typedef NSView TCVegaRendererNativeView;
#else
#import <UIKit/UIKit.h>
typedef UIColor TCVegaRendererNativeColor;
typedef UIEvent TCVegaRendererNativeEvent;
typedef UIFont TCVegaRendererNativeFont;
typedef UIView TCVegaRendererNativeView;
#endif

#endif /* VegaPortabilityTypes_h */
