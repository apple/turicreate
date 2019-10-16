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

#import "TCVegaHTMLElement.h"

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <JavaScriptCore/JavaScriptCore.h>

@interface TCVegaCGFontProperties : NSObject
@property (nonatomic, strong) NSString *cssFontString;
@property (nonatomic, strong) NSString *fontFamily;
@property (nonatomic, strong) NSString *fontSize;
@property (nonatomic, strong) NSString *fontStyle;
@property (nonatomic, strong) NSString *fontWeight;
@property (nonatomic, strong) NSString *fontVariant;
@property (nonatomic, strong) NSString *lineHeight;

- (instancetype)initWithString:(NSString*)font;
@end

@protocol TCVegaCGGradientInterface<JSExport>
JSExportAs(addColorStop,
           - (void)addColorStopWithOffset:(double)offset
           color:(NSString *)color
           );
@end

@interface TCVegaCGLinearGradient : NSObject<TCVegaCGGradientInterface>
- (instancetype)initWithX0:(double)x0
                        y0:(double)y0
                        x1:(double)x1
                        y1:(double)y1;
- (void)fillWithContext:(CGContextRef)context;
@end

@protocol TCVegaCGImageInterface<JSExport>
@end

@interface TCVegaCGImage : NSObject<TCVegaCGImageInterface>
@end

@protocol TCVegaCGTextMetricsInterface<JSExport>
@property (nonatomic) double width;
@end

@interface TCVegaCGTextMetrics : NSObject<TCVegaCGTextMetricsInterface>
@end

@protocol TCVegaCGContextInterface <JSExport>

// properties
@property (strong) JSValue * fillStyle;
@property double globalAlpha;
@property (strong) NSString * lineCap;
@property (strong) NSString * lineJoin;
@property double lineWidth;
@property double miterLimit;
@property (nonatomic) double pixelRatio;
@property (strong) NSString * strokeStyle;
@property (strong) NSString * textAlign;
@property (strong) NSString * font;
@property double lineDashOffset;

// utilities
- (JSValue *)measureText:(NSString *)text;

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
           - (JSValue *)createLinearGradientWithX0:(double)x0
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
JSExportAs(strokeText,
           - (void)strokeTextWithString:(NSString*)string
           x:(double)x
           y:(double)y);
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

JSExportAs(isPointInPath,
           - (BOOL)isPointInPathWithX:(double)x
           y:(double)y
           );

@end

@interface TCVegaCGContext : NSObject<TCVegaCGContextInterface>
@property (readonly) CGContextRef context;
@property double width;
@property double height;
- (instancetype)init;
- (void)dealloc;
- (NSDictionary<NSAttributedStringKey, id> *)textAttributes;
+ (CGAffineTransform)flipYAxisWithHeight:(double)height;
+ (CGColorRef)newColorFromString:(NSString *)string;
@end

@protocol TCVegaCGCanvasInterface <JSExport>
- (TCVegaCGContext *)getContext:(NSString *)type;
@property double width;
@property double height;
@end

@interface TCVegaCGCanvas : TCVegaHTMLElement<TCVegaCGCanvasInterface>
@property (nonatomic, strong) TCVegaCGContext *context;
- (instancetype)init;
@end


#endif /* Canvas_h */
