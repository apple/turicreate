/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

/* VegaCGCanvas is a partial implementation of JavaScript Canvas as described in:
   https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API

   VegaCGContext is a partial implementation of JavaScript CanvasRenderingContext2D as described in:
   https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D

   These are intended to support Vega rendering through a <canvas> element in JavaScriptCore.
   They are not intended to be a full, or standards-compliant, web canvas implementation.
*/

#ifndef Canvas_h
#define Canvas_h

#import "VegaHTMLElement.h"

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <JavaScriptCore/JavaScriptCore.h>

@interface VegaCGFontProperties : NSObject
@property NSString *cssFontString;
@property NSString *fontFamily;
@property NSString *fontSize;
@property NSString *fontStyle;
@property NSString *fontWeight;
@property NSString *fontVariant;
@property NSString *lineHeight;

- (instancetype)initWithString:(NSString*)font;
@end

@protocol VegaCGGradientInterface<JSExport>
JSExportAs(addColorStop,
           - (void)addColorStopWithOffset:(double)offset
           color:(NSString *)color
           );
@end

@interface VegaCGLinearGradient : NSObject<VegaCGGradientInterface>
- (instancetype)initWithX0:(double)x0
                        y0:(double)y0
                        x1:(double)x1
                        y1:(double)y1;
- (void)fillWithContext:(CGContextRef)context;
@end

@protocol VegaCGImageInterface<JSExport>
@end

@interface VegaCGImage : NSObject<VegaCGImageInterface>
@end

@protocol VegaCGTextMetricsInterface<JSExport>
@property double width;
@end

@interface VegaCGTextMetrics : NSObject<VegaCGTextMetricsInterface>
@end

@protocol VegaCGContextInterface <JSExport>

// properties
@property id fillStyle;
@property double globalAlpha;
@property NSString * lineCap;
@property NSString * lineJoin;
@property double lineWidth;
@property double miterLimit;
@property double pixelRatio;
@property NSString * strokeStyle;
@property NSString * textAlign;
@property NSString * font;
@property double lineDashOffset;

// utilities
- (VegaCGTextMetrics *)measureText:(NSString *)text;

// save/restore context state
- (void)save;
- (void)restore;

// drawing APIs
JSExportAs(arc,
           -(void)arcWithX:(double)x
           y:(double)y
           radius:(double)radius
           startAngle:(double)startAngle
           endAngle:(double)endAngle
           anticlockwise:(BOOL)anticlockwise
           );
- (void)beginPath;
JSExportAs(bezierCurveTo,
           - (void)bezierCurveToCP1x:(double)cp1x
           cp1y:(double)cp1y
           cp2x:(double)cp2x
           cp2y:(double)cp2y
           x:(double)x
           y:(double)y
           );
JSExportAs(clearRect,
           - (void)clearRectWithX:(double)x
           y:(double)y
           w:(double)w
           h:(double)h
           );
- (void)clip;
- (void)closePath;
JSExportAs(createLinearGradient,
           - (VegaCGLinearGradient *)createLinearGradientWithX0:(double)x0
           y0:(double)y0
           x1:(double)x1
           y1:(double)y1
           );
- (void)fill;
JSExportAs(fillText,
           - (void)fillTextWithString:(NSString *)string
           x:(double)x
           y:(double)y
           );
JSExportAs(fillRect,
            - (void)fillRectWithX:(double)x
            y:(double)y
            width:(double)width
            height:(double)height
            );
JSExportAs(lineTo,
           - (void)lineToX:(double)x
           y:(double)y
           );
JSExportAs(moveTo,
           - (void)moveToX:(double)x
           y:(double)y
           );
- (void)stroke;
JSExportAs(rect,
           - (void)rectWithX:(double)x
           y:(double)y
           width:(double)width
           height:(double)height
           );
JSExportAs(setLineDash,
           - (void)setLineDashWithSegments:(NSArray<NSNumber *> *)segments
           );

// translation matrix
JSExportAs(rotate,
           - (void)rotateWithAngle:(double)angle
           );

JSExportAs(setTransform,
           - (void)setTransformWithA:(double)a
           b:(double)b
           c:(double)c
           d:(double)d
           e:(double)e
           f:(double)f
           );

JSExportAs(translate,
           - (void)translateWithX:(double)x
           y:(double)y
           );

@end

@interface VegaCGContext : NSObject<VegaCGContextInterface>
@property (readonly) CGContextRef context;
@property CGLayerRef layer;
@property double width;
@property double height;
- (instancetype)initWithContext:(CGContextRef)parentContext;
- (void)dealloc;
- (NSDictionary<NSAttributedStringKey, id> *)textAttributes;
+ (CGAffineTransform)flipYAxisWithHeight:(double)height;
+ (CGColorRef)newColorFromString:(NSString *)string;
@end

@protocol VegaCGCanvasInterface <JSExport, VegaHTMLElement>
- (VegaCGContext *)getContext:(NSString *)type;
@property double width;
@property double height;
@end

@interface VegaCGCanvas : NSObject<VegaCGCanvasInterface>
@property VegaCGContext *context;
- (instancetype)initWithContext:(CGContextRef)parentContext;
@end


#endif /* Canvas_h */
