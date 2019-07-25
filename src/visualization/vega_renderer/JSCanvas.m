/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "JSCanvas.h"
#import "colors.h"

#import <AppKit/AppKit.h>
#import <CoreText/CoreText.h>

@implementation VegaCGLinearGradient
{
    double _x0;
    double _y0;
    double _x1;
    double _y1;
    NSMutableArray *_colorStops;
}
- (instancetype)initWithX0:(double)x0 y0:(double)y0 x1:(double)x1 y1:(double)y1 {
    self = [super init];
    _x0 = x0;
    _y0 = y0;
    _x1 = x1;
    _y1 = y1;
    _colorStops = [[NSMutableArray alloc] init];
    return self;
}
- (void)addColorStopWithOffset:(double)offset color:(NSString *)color {
    CGColorRef cgcolor = [VegaCGContext newColorFromString:color];
    [_colorStops addObject:@[@(offset), (__bridge_transfer id)cgcolor]];
}
- (void)fillWithContext:(CGContextRef)context {
    CGContextSaveGState(context);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CFMutableArrayRef colors = CFArrayCreateMutable(NULL, _colorStops.count, NULL);
    CGFloat offsets[_colorStops.count];
    for (size_t i=0; i<_colorStops.count; i++) {
        NSArray *values = _colorStops[i];
        assert(values != nil);
        assert(values.count == 2);
        NSNumber *offset = values[0];
        assert(offset != nil);
        offsets[i] = offset.doubleValue;
        CGColorRef color = (__bridge CGColorRef)(values[1]);
        assert(color != nil);
        CFArrayAppendValue(colors, color);
    }
    CGGradientRef gradient = CGGradientCreateWithColors(colorSpace, colors, offsets);
    CGPoint startPoint = CGPointMake(_x0, _y0);
    CGPoint endPoint = CGPointMake(_x1, _y1);
    CGGradientDrawingOptions options = 0;
    CGContextClip(context);
    CGContextDrawLinearGradient(context, gradient, startPoint, endPoint, options);
    CGContextRestoreGState(context);
    CGGradientRelease(gradient);
    CGColorSpaceRelease(colorSpace);
    CFRelease(colors);
}
@end

@implementation VegaCGFontProperties
- (instancetype)initWithString:(NSString*)fontStr {
    self = [super init];
    _cssFontString = fontStr;
    _fontFamily = nil;
    _fontSize = nil;
    _fontVariant = nil;
    _fontWeight = nil;
    _fontStyle = nil;
    _lineHeight = nil;

    NSArray<NSString *> *elements = [fontStr componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    for (size_t i=0; i<elements.count; i++) {
        NSString *element = elements[i];
        if ([element isEqualToString:@"normal"]) {
            continue;
        }
        if ([element isEqualToString:@"italic"] ||
            [element isEqualToString:@"oblique"]) {
            _fontStyle = element;
            continue;
        }
        if ([element isEqualToString:@"small-caps"]) {
            _fontVariant = element;
            continue;
        }
        if ([element isEqualToString:@"bold"] ||
            [element isEqualToString:@"bolder"] ||
            [element isEqualToString:@"lighter"] ||
            [element isEqualToString:@"100"] ||
            [element isEqualToString:@"200"] ||
            [element isEqualToString:@"300"] ||
            [element isEqualToString:@"400"] ||
            [element isEqualToString:@"500"] ||
            [element isEqualToString:@"600"] ||
            [element isEqualToString:@"700"] ||
            [element isEqualToString:@"800"] ||
            [element isEqualToString:@"900"]) {
            _fontWeight = element;
            continue;
        }
        if (_fontSize == nil) {
            NSArray<NSString *> *parts = [element componentsSeparatedByString:@"/"];
            _fontSize = parts[0];
            if (parts.count > 1) {
                _lineHeight = parts[1];
                assert(parts.count == 2);
            }
            continue;
        }
        _fontFamily = element;
        if (i < elements.count - 1) {
            NSArray<NSString *> *remainingElements = [elements subarrayWithRange:NSMakeRange(i+1, elements.count-(i+1))];
            _fontFamily = [_fontFamily stringByAppendingString:[@" " stringByAppendingString:[remainingElements componentsJoinedByString:@" "]]];
        }
        break;
    }

    if ([_fontFamily isEqualToString:@"sans-serif"]) {
        _fontFamily = @"Helvetica";
    }

    return self;
}
@end

@implementation VegaCGImage
@end

@implementation VegaCGTextMetrics
@synthesize width;
@end

@implementation VegaCGContext
{
    // MUST only use these from property setter/getters
    CGLayerRef _layer;

    id _fillStyle;
    double _globalAlpha;
    NSString * _lineCap;
    NSString * _lineJoin;
    double _lineWidth;
    double _miterLimit;
    NSString * _strokeStyle;
    NSString * _textAlign;
    CGAffineTransform _currentTransform;
    double _lineDashOffset;
    NSString *_font;
    NSFont *_nsFont;
    CGContextRef _parentContext;
}

- (void)setLayer:(CGLayerRef)layer {
    CFRetain(layer);
    CGLayerRelease(_layer);
    _layer = layer;
}

- (CGLayerRef)layer {
    return _layer;
}

- (CGContextRef)context {
    return CGLayerGetContext(self.layer);
}

+ (CGAffineTransform)flipYAxisWithHeight:(double)height {
    // Flip vertically and move origin to bottom left instead of top left
    return CGAffineTransformScale(CGAffineTransformTranslate(CGAffineTransformIdentity, 0, height), 1.0, -1.0);
}

- (void)resizeWithWidth:(double)width height:(double)height {
    // This is a hack to ensure that neither width or height is ever zero as this causes
    // CGLayerCreateWithContext(...) to crash. This can occur when width and height are
    // set one at a time in subsequent calls. A more faithful rendering context implementation
    // would wait until both are set before creating the layer.
    width = MAX(1, width);
    height = MAX(1, height);

    // Because we are about to create a new layer, set up other relevant
    // instance properties so they are in the right state.
    _currentTransform = CGAffineTransformIdentity;
    assert(self != nil);
    CGLayerRef layer = CGLayerCreateWithContext(_parentContext, CGSizeMake(width, height), nil);
    self.layer = layer;
    CGLayerRelease(layer);
    CGContextConcatCTM(self.context, [self.class flipYAxisWithHeight:height]);
}

- (double)width {
    return CGLayerGetSize(self.layer).width;
}

- (void)setWidth:(double)width {
    [self resizeWithWidth:width height:self.height];
}

- (double)height {
    return CGLayerGetSize(self.layer).height;
}

- (void)setHeight:(double)height {
    [self resizeWithWidth:self.width height:height];
}

- (instancetype)initWithContext:(CGContextRef)parentContext {
    self = [super init];
    _layer = nil;
    _parentContext = parentContext;
    return self;
}

- (void)dealloc {
    CGLayerRelease(_layer);
    _layer = nil;
}

// properties
- (id)fillStyle {
    return _fillStyle;
}
- (void)setFillStyle:(id)fillStyle {
    _fillStyle = fillStyle;
    if (![_fillStyle isKindOfClass:NSString.class]) {
        assert([_fillStyle isKindOfClass:VegaCGLinearGradient.class]);
    }
}

- (NSDictionary<NSAttributedStringKey, id> *)textAttributes {
    assert(_nsFont != nil);
    CGColorRef color = nil;
    if (_fillStyle == nil) {
        color = [self.class newColorFromR:0 G:0 B:0 A:255];
    } else {
        assert([_fillStyle isKindOfClass:NSString.class]);
        color = [self.class newColorFromString:_fillStyle];
    }
    assert(color != nil);
    NSColor *nsColor = [NSColor colorWithCGColor:color];
    CGColorRelease(color);
    return @{
             NSFontAttributeName: _nsFont,
             NSForegroundColorAttributeName: nsColor,
             };
}

- (void)setFont:(NSString *)fontStr {
    VegaCGFontProperties *fontProperties = [[VegaCGFontProperties alloc] initWithString:fontStr];

    // TODO - allow other font properties
    // for now, make sure we don't need to handle them
    assert(fontProperties.fontStyle == nil || [fontProperties.fontStyle isEqualToString:@"normal"]);
    assert(fontProperties.fontVariant == nil || [fontProperties.fontVariant isEqualToString:@"normal"]);
    assert(fontProperties.lineHeight == nil || [fontProperties.lineHeight isEqualToString:@"normal"]);

    double fontSize;
    if (fontProperties.fontSize == nil) {
        fontSize = 10;
    } else {
        fontSize = atof([fontProperties.fontSize UTF8String]);
        fontSize = ceil(fontSize);
    }

    assert(fontSize > 0 && fontSize < 1000);

    NSFont *newFont;
    if (fontProperties.fontFamily == nil) {
        newFont = [NSFont systemFontOfSize:fontSize];
    } else {
        NSFontManager *fontManager = [NSFontManager sharedFontManager];
        NSArray<NSString *> *possibleFontFamilies = [fontProperties.fontFamily componentsSeparatedByString:@","];
        for (NSString * __strong possibleFontFamily in possibleFontFamilies) {
            possibleFontFamily = [possibleFontFamily stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
            newFont = [NSFont fontWithName:possibleFontFamily size:fontSize];
            if (newFont != nil && fontProperties.fontWeight != nil) {
                if ([fontProperties.fontWeight isEqualToString:@"bold"]) {
                    newFont = [fontManager convertFont:newFont toHaveTrait:NSBoldFontMask];
                    assert(newFont != nil);
                } else if (fontProperties.fontWeight.length == 3 &&
                           [[fontProperties.fontWeight substringFromIndex:1] isEqualToString:@"00"]) {
                    NSString *weightString = [fontProperties.fontWeight substringToIndex:1];
                    NSInteger weightInt = weightString.intValue;
                    NSFontWeight weight = NSFontWeightRegular;
                    switch (weightInt) {
                        case 1:
                            weight = NSFontWeightUltraLight;
                            break;
                        case 2:
                            weight = NSFontWeightThin;
                            break;
                        case 3:
                            weight = NSFontWeightLight;
                            break;
                        case 4:
                            weight = NSFontWeightRegular;
                            break;
                        case 5:
                            weight = NSFontWeightMedium;
                            break;
                        case 6:
                            weight = NSFontWeightSemibold;
                            break;
                        case 7:
                            weight = NSFontWeightBold;
                            break;
                        case 8:
                            weight = NSFontWeightHeavy;
                            break;
                        case 9:
                            weight = NSFontWeightBlack;
                            break;
                        default:
                            NSLog(@"Encountered unexpected font weight %@", fontProperties.fontWeight);
                            assert(false);
                    }
                    newFont = [fontManager fontWithFamily:newFont.familyName traits:[fontManager traitsOfFont:newFont] weight:weight size:newFont.pointSize];
                    assert(newFont != nil);
                } else {
                    // unexpected font weight
                    NSLog(@"Encountered unexpected font weight %@", fontProperties.fontWeight);
                    assert(false);
                }
            }
            if (newFont != nil) {
                break;
            }
        }
    }

    if(newFont == nil) {
        newFont = [NSFont systemFontOfSize:fontSize];
        // TODO should we be updating _font to reflect the system font we've fallen back to?
        NSLog(@"The specified font: '%@' is unavailable. Falling back to '%@'.", fontStr, [newFont displayName]);
    } else {
        _font = fontStr;
    }
    assert(newFont != nil);
    _nsFont = newFont;
}

- (NSString *)font {
    return _font;
}

- (double)globalAlpha {
    return _globalAlpha;
}
- (void)setGlobalAlpha:(double)globalAlpha {
    _globalAlpha = globalAlpha;
    CGContextSetAlpha(self.context, _globalAlpha);
}
- (NSString *)lineCap {
    return _lineCap;
}
- (void)setLineCap:(NSString *)lineCap {
    _lineCap = lineCap;
    CGLineCap cap;
    if ([lineCap isEqualToString:@"butt"]) {
        cap = kCGLineCapButt;
    } else if ([lineCap isEqualToString:@"round"]) {
        cap = kCGLineCapRound;
    } else if ([lineCap isEqualToString:@"square"]) {
        cap = kCGLineCapSquare;
    } else {
        cap = kCGLineCapButt;
        assert(false);
    }
    CGContextSetLineCap(self.context, cap);
}
- (NSString *)lineJoin {
    return _lineJoin;
}
- (void)setLineJoin:(NSString *)lineJoin {
    _lineJoin = lineJoin;
    CGLineJoin join;
    if ([lineJoin isEqualToString:@"miter"]) {
        join = kCGLineJoinMiter;
    } else if ([lineJoin isEqualToString:@"round"]) {
        join = kCGLineJoinRound;
    } else if ([lineJoin isEqualToString:@"bevel"]) {
        join = kCGLineJoinBevel;
    } else {
        join = kCGLineJoinMiter;
        assert(false);
    }
    CGContextSetLineJoin(self.context, join);
}
- (double)lineWidth {
    return _lineWidth;
}
- (void)setLineWidth:(double)lineWidth {
    _lineWidth = lineWidth;
    CGContextSetLineWidth(self.context, _lineWidth);
}
- (double)miterLimit {
    return _miterLimit;
}
- (void)setMiterLimit:(double)miterLimit {
    _miterLimit = miterLimit;
    CGContextSetMiterLimit(self.context, _miterLimit);
}

- (double)pixelRatio {
    return 1;
}

- (void)setPixelRatio:(double)pixelRatio {
    (void)pixelRatio;
    assert(pixelRatio == 1);
    // TODO: not sure what to do with this...
    // but let's assume we don't have to do anything if it's 1.0.
}

- (NSString *)strokeStyle {
    return _strokeStyle;
}

+ (CGColorRef) newColorFromR:(unsigned int) r
                        G:(unsigned int) g
                        B:(unsigned int) b
                        A:(unsigned int) a {
    CGFloat rgba[4];
    rgba[0] = (CGFloat)r / 255.0;
    assert(rgba[0] >= 0.0 && rgba[0] <= 1.0);
    rgba[1] = (CGFloat)g / 255.0;
    assert(rgba[1] >= 0.0 && rgba[1] <= 1.0);
    rgba[2] = (CGFloat)b / 255.0;
    assert(rgba[2] >= 0.0 && rgba[2] <= 1.0);
    rgba[3] = (CGFloat)a / 255.0;
    assert(rgba[3] >= 0.0 && rgba[3] <= 1.0);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGColorRef ret = CGColorCreate(colorSpace, &rgba[0]);
    CGColorSpaceRelease(colorSpace);
    return ret;
}

+ (CGColorRef) newColorFromString:(NSString *)str {
    assert(str != nil);
    if ([str characterAtIndex:0] == '#') {
        str = [str substringFromIndex:1];

        // parse as RGB hexadecimal
        NSString *rs;
        NSString *gs;
        NSString *bs;
        NSString *as;
        if ([str length] == 3) {
            rs = [str substringWithRange:NSMakeRange(0, 1)];
            rs = [rs stringByAppendingString:rs];
            gs = [str substringWithRange:NSMakeRange(1, 1)];
            gs = [gs stringByAppendingString:gs];
            bs = [str substringWithRange:NSMakeRange(2, 1)];
            bs = [bs stringByAppendingString:bs];
            as = @"ff";
        } else if ([str length] == 6) {
            rs = [str substringWithRange:NSMakeRange(0, 2)];
            gs = [str substringWithRange:NSMakeRange(2, 2)];
            bs = [str substringWithRange:NSMakeRange(4, 2)];
            as = @"ff";
        } else {
            assert([str length] == 8);
            rs = [str substringWithRange:NSMakeRange(0, 2)];
            gs = [str substringWithRange:NSMakeRange(2, 2)];
            bs = [str substringWithRange:NSMakeRange(4, 2)];
            as = [str substringWithRange:NSMakeRange(6, 2)];
        }
        unsigned int r,g,b,a;
        [[NSScanner scannerWithString:rs] scanHexInt:&r];
        [[NSScanner scannerWithString:gs] scanHexInt:&g];
        [[NSScanner scannerWithString:bs] scanHexInt:&b];
        [[NSScanner scannerWithString:as] scanHexInt:&a];
        return [self.class newColorFromR:r G:g B:b A:a];
    } else if ([[str substringToIndex:4] isEqualToString:@"rgb("]) {
        // parse as RGB integers like rgb(r,g,b)
        str = [str stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        int r,g,b;
        NSString *commaSeparated = [str substringWithRange:NSMakeRange(4, str.length-5)];
        NSArray<NSString *> *components = [commaSeparated componentsSeparatedByString:@","];
        assert(components.count == 3);
        [[NSScanner scannerWithString:components[0]] scanInt:&r];
        [[NSScanner scannerWithString:components[1]] scanInt:&g];
        [[NSScanner scannerWithString:components[2]] scanInt:&b];
        return [self.class newColorFromR:r G:g B:b A:255];
    } else if ([[str substringToIndex:5] isEqualToString:@"rgba("]) {
        // parse as RGBA integers like rgb(r,g,b,a)
        str = [str stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        int r,g,b;
        double a;
        NSString *commaSeparated = [str substringWithRange:NSMakeRange(5, str.length-6)];
        NSArray<NSString *> *components = [commaSeparated componentsSeparatedByString:@","];
        assert(components.count == 4);
        [[NSScanner scannerWithString:components[0]] scanInt:&r];
        [[NSScanner scannerWithString:components[1]] scanInt:&g];
        [[NSScanner scannerWithString:components[2]] scanInt:&b];
        [[NSScanner scannerWithString:components[3]] scanDouble:&a];
        return [self.class newColorFromR:r G:g B:b A:(a*255.0)];
    } else {
        return [self.class newColorFromString:[[VegaCGColorMap map] objectForKey:str]];
    }
}

- (void)setStrokeStyle:(NSString *)strokeStyle {
    NSArray *args = [JSContext currentArguments];
    (void)args;
    assert(args != nil);
    assert(args.count == 1);
    _strokeStyle = strokeStyle;
    CGColorRef color = [self.class newColorFromString:_strokeStyle];
    assert(color != nil);
    CGContextSetStrokeColorWithColor(self.context, color);
    CGColorRelease(color);
}
- (NSString *)textAlign {
    return _textAlign;
}
- (void)setTextAlign:(NSString *)textAlign {
    _textAlign = textAlign;
}

- (VegaCGTextMetrics *)measureText:(NSString *)text {
    NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:text attributes:self.textAttributes];
    VegaCGTextMetrics *ret = [[VegaCGTextMetrics alloc] init];
    ret.width = attrStr.size.width;
    return ret;
}

- (void)rotateWithAngle:(double)angle {
    CGContextRotateCTM(self.context, angle);
}

- (void)setTransformWithA:(double)a b:(double)b c:(double)c d:(double)d e:(double)e f:(double)f {
    // TODO do we need to reset the CTM on each call here? (And reapply our CG->Canvas transformations?)
    // For now, just make sure we don't need to.
    assert(CGAffineTransformIsIdentity(_currentTransform));
    _currentTransform = CGAffineTransformMake(a, b, c, d, e, f);
    CGContextConcatCTM(self.context, _currentTransform);
}

- (void)restore {
    CGContextRestoreGState(self.context);
}


- (void)save {
    CGContextSaveGState(self.context);
}

- (void)clearRectWithX:(double)x y:(double)y w:(double)w h:(double)h {
    CGContextClearRect(self.context, CGRectMake(x, y, w, h));
}

- (void)translateWithX:(double)x y:(double)y {
    CGContextTranslateCTM(self.context, x, y);
}

- (void)arcWithX:(double)x y:(double)y radius:(double)radius startAngle:(double)startAngle endAngle:(double)endAngle anticlockwise:(BOOL)anticlockwise {
    CGContextAddArc(self.context, x, y, radius, startAngle, endAngle, anticlockwise);
}

- (void)beginPath {
    NSArray *args = [JSContext currentArguments];
    (void)args;
    assert(args != nil);
    assert(args.count == 0);
    CGContextBeginPath(self.context);
}

- (void)closePath {
    NSArray *args = [JSContext currentArguments];
    (void)args;
    assert(args != nil);
    assert(args.count == 0);
    CGContextClosePath(self.context);
}

- (void)bezierCurveToCP1x:(double)cp1x cp1y:(double)cp1y cp2x:(double)cp2x cp2y:(double)cp2y x:(double)x y:(double)y {
    CGContextAddCurveToPoint(self.context, cp1x, cp1y, cp2x, cp2y, x, y);
}

- (void)moveToX:(double)x y:(double)y {
    CGContextMoveToPoint(self.context, x, y);
}

- (void)lineToX:(double)x y:(double)y {
    NSArray *args = [JSContext currentArguments];
    (void)args;
    assert(args != nil);
    assert(args.count == 2);
    CGContextAddLineToPoint(self.context, x, y);
}

- (void)stroke {
    NSArray *args = [JSContext currentArguments];
    (void)args;
    assert(args != nil);
    assert(args.count == 0);
    CGPathRef currentPath = CGContextCopyPath(self.context);
    CGContextStrokePath(self.context);
    CGContextAddPath(self.context, currentPath);
    CGPathRelease(currentPath);
}

- (void)fillTextWithString:(NSString *)string x:(double)x y:(double)y {
    // Get rid of the transformations on the context
    CGContextSaveGState(self.context);
    CGAffineTransform flipYAxis = [self.class flipYAxisWithHeight:self.height];
    CGContextConcatCTM(self.context, CGAffineTransformInvert(flipYAxis));

    // Reapply the transformations, but only on the coordinates,
    // so we don't flip the text upside down
    CGPoint coords = CGPointApplyAffineTransform(CGPointMake(x, y), flipYAxis);
    NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:string attributes:self.textAttributes];
    CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attrStr);
    assert(line != nil);
    double width = attrStr.size.width;

    if ([_textAlign isEqualToString:@"right"]) {
        coords.x -= width;
    } else if ([_textAlign isEqualToString:@"center"]) {
        coords.x -= (width / 2.0);
    } else {
        // TODO implement other alignments
        assert(_textAlign == nil || [_textAlign isEqualToString:@"left"]);
    }

    CGContextSetTextPosition(self.context, coords.x, coords.y);
    CGContextSetTextDrawingMode(self.context, kCGTextFill);

    CTLineDraw(line, self.context);
    CGContextRestoreGState(self.context);

    CFRelease(line);
}

- (void)fillRectWithX:(double)x y:(double)y width:(double)width height:(double)height {
    [self rectWithX:x y:y width:width height:height];
    [self fill];
}

- (void)rectWithX:(double)x y:(double)y width:(double)width height:(double)height {
    CGContextAddRect(self.context, CGRectMake(x, y, width, height));
}

- (double)lineDashOffset {
    return _lineDashOffset;
}

- (void)setLineDashOffset:(double)lineDashOffset {
    _lineDashOffset = lineDashOffset;
}

- (void)setLineDashWithSegments:(NSArray<NSNumber *> *)segments {
    size_t count = segments.count;
    CGFloat lengths[count];
    for (size_t i=0; i<count; i++) {
        lengths[i] = segments[i].doubleValue;
    }
    CGContextSetLineDash(self.context, self.lineDashOffset, lengths, count);
}

- (void)clip {
    NSArray *args = [JSContext currentArguments];
    (void)args;
    assert(args != nil);
    assert(args.count == 0);
    CGContextClip(self.context);
}

- (void)fill {
    CGPathRef currentPath = CGContextCopyPath(self.context);
    if ([_fillStyle isKindOfClass:VegaCGLinearGradient.class]) {
        VegaCGLinearGradient *gradient = _fillStyle;
        [gradient fillWithContext:self.context];
    } else {
        assert([_fillStyle isKindOfClass:NSString.class]);
        CGColorRef color = [self.class newColorFromString:_fillStyle];
        CGContextSetFillColorWithColor(self.context, color);
        CGColorRelease(color);
        CGContextFillPath(self.context);
    }
    CGContextAddPath(self.context, currentPath);
    CGPathRelease(currentPath);
}

- (VegaCGLinearGradient *)createLinearGradientWithX0:(double)x0 y0:(double)y0 x1:(double)x1 y1:(double)y1 {
    return [[VegaCGLinearGradient alloc] initWithX0:x0 y0:y0 x1:x1 y1:y1];
}

@end

@implementation VegaCGCanvas

- (instancetype)initWithContext:(CGContextRef)parentContext {
    self = [super initWithTagName:@"canvas"];
    if(self) {
        self.context = [[VegaCGContext alloc] initWithContext:parentContext];
    }
    return self;
}

- (VegaCGContext *)getContext:(NSString *)type {
    assert([type isEqualToString:@"2d"]); // no other types handled
    return self.context;
}

- (double)height {
    return self.context.height;
}

- (void)setHeight:(double)height {
    self.context.height = height;
}

- (double)width {
    return self.context.width;
}

- (void)setWidth:(double)width {
    self.context.width = width;
}

@end
