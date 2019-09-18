/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "TCVegaJSCanvas.h"
#import "TCVegaLogger.h"
#import "TCVegaLogProxy.h"
#import "TCVegaCGColorMap.h"
#import "TCVegaPortabilityTypes.h"

#import <CoreText/CoreText.h>

@implementation TCVegaCGLinearGradient
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

- (instancetype)initWithDictionary:(NSDictionary*)values {
    double x0 = [(NSNumber*) values[@"x1"] doubleValue];
    double y0 = [(NSNumber*) values[@"y1"] doubleValue];
    double x1 = [(NSNumber*) values[@"x2"] doubleValue];
    double y1 = [(NSNumber*) values[@"y2"] doubleValue];
    
    self = [self initWithX0:x0 y0:y0 x1:x1 y1:y1];
    
    for(NSDictionary* stop in values[@"stops"]) {
        double offset = [(NSNumber*) stop[@"offset"] doubleValue];
        [self addColorStopWithOffset:offset color:stop[@"color"]];
    }
    return self;
}

- (void)addColorStopWithOffset:(double)offset color:(NSString *)color {
    CGColorRef cgcolor = [TCVegaCGContext newColorFromString:color];
    [_colorStops addObject:@[@(offset), (__bridge_transfer id)cgcolor]];
}
- (void)fillWithContext:(CGContextRef)context {
    CGContextSaveGState(context);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CFMutableArrayRef colors = CFArrayCreateMutable(NULL, (CFIndex)_colorStops.count, NULL);
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

@implementation TCVegaCGFontProperties

@synthesize cssFontString;
@synthesize fontFamily;
@synthesize fontSize;
@synthesize fontVariant;
@synthesize fontWeight;
@synthesize fontStyle;
@synthesize lineHeight;

- (instancetype)initWithString:(NSString*)fontStr {
    self = [super init];
    self.cssFontString = fontStr;
    self.fontFamily = nil;
    self.fontSize = nil;
    self.fontVariant = nil;
    self.fontWeight = nil;
    self.fontStyle = nil;
    self.lineHeight = nil;

    NSArray<NSString *> *elements = [fontStr componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    for (size_t i=0; i<elements.count; i++) {
        NSString *element = elements[i];
        if ([element isEqualToString:@"normal"]) {
            continue;
        }
        if ([element isEqualToString:@"italic"] ||
            [element isEqualToString:@"oblique"]) {
            self.fontStyle = element;
            continue;
        }
        if ([element isEqualToString:@"small-caps"]) {
            self.fontVariant = element;
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
            self.fontWeight = element;
            continue;
        }
        if (self.fontSize == nil) {
            NSArray<NSString *> *parts = [element componentsSeparatedByString:@"/"];
            self.fontSize = parts[0];
            if (parts.count > 1) {
                self.lineHeight = parts[1];
                assert(parts.count == 2);
            }
            continue;
        }
        self.fontFamily = element;
        if (i < elements.count - 1) {
            NSArray<NSString *> *remainingElements = [elements subarrayWithRange:NSMakeRange(i+1, elements.count-(i+1))];
            self.fontFamily = [self.fontFamily stringByAppendingString:[@" " stringByAppendingString:[remainingElements componentsJoinedByString:@" "]]];
        }
        break;
    }

    if ([self.fontFamily isEqualToString:@"sans-serif"]) {
        self.fontFamily = @"Helvetica";
    }

    return self;
}
@end

@implementation TCVegaCGImage
@end

@implementation TCVegaCGTextMetrics
@synthesize width;
@end

@implementation TCVegaCGContext
{
    // MUST only use these from property setter/getters
    JSValue * _fillStyle;
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
    TCVegaRendererNativeFont *_nsFont;
    CGContextRef _bitmapContext;
}

- (CGContextRef)context {
    assert(_bitmapContext != nil);
    return _bitmapContext;
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

    // Because we are about to create a new context, set up other relevant
    // instance properties so they are in the right state.
    assert(self != nil);
    _currentTransform = CGAffineTransformIdentity;
    
    const size_t BITS_PER_COMPONENT = 8;
    const size_t BYTES_PER_ROW = 0; // setting to zero results in automatic calculation
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
    
    _bitmapContext = CGBitmapContextCreate(nil, (size_t)width, (size_t)height, BITS_PER_COMPONENT, BYTES_PER_ROW, colorspace, kCGImageAlphaPremultipliedLast);
    
    CGColorSpaceRelease(colorspace);
#if TARGET_OS_OSX
    CGContextConcatCTM(self.context, [self.class flipYAxisWithHeight:height]);
#endif
}

- (double)width {
    if(_bitmapContext == nil) {
        return 1;
    }
    return CGBitmapContextGetWidth(_bitmapContext);
}

- (void)setWidth:(double)width {
    [self resizeWithWidth:width height:self.height];
}

- (double)height {
    if(_bitmapContext == nil) {
        return 1;
    }
    return CGBitmapContextGetHeight(_bitmapContext);
}

- (void)setHeight:(double)height {
    [self resizeWithWidth:self.width height:height];
}

- (instancetype)init {
    self = [super init];
    return self;
}

- (void)dealloc {
    CGContextRelease(_bitmapContext);
}

// properties
- (JSValue *)fillStyle {
    assert([_fillStyle isKindOfClass:[JSValue class]]);
    return _fillStyle;
}
- (void)setFillStyle:(JSValue *)fillStyle {
    _fillStyle = fillStyle;
    if ([_fillStyle isObject] && [_fillStyle.toObject isKindOfClass:NSDictionary.class]) {
        _fillStyle = [JSValue valueWithObject:[[TCVegaCGLinearGradient alloc] initWithDictionary:fillStyle.toObject] inContext:fillStyle.context];
    } else if (![_fillStyle isString]) {
        assert([_fillStyle isObject] && [_fillStyle.toObject isKindOfClass:TCVegaCGLinearGradient.class]);
    }
}

- (NSDictionary<NSAttributedStringKey, id> *)textAttributes {
    assert(_nsFont != nil);
    CGColorRef color = nil;
    if (_fillStyle == nil) {
        color = [self.class newColorFromR:0 G:0 B:0 A:255];
    } else {
        assert(_fillStyle.isString);
        color = [self.class newColorFromString:_fillStyle.toString];
    }
    assert(color != nil);
    TCVegaRendererNativeColor *nsColor = [TCVegaRendererNativeColor colorWithCGColor:color];
    CGColorRelease(color);
    return @{
             NSFontAttributeName: _nsFont,
             NSForegroundColorAttributeName: nsColor,
             };
}

- (void)setFont:(NSString *)fontStr {
    TCVegaCGFontProperties *fontProperties = [[TCVegaCGFontProperties alloc] initWithString:fontStr];

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

    TCVegaRendererNativeFont *newFont;
    if (fontProperties.fontFamily == nil) {
        newFont = [TCVegaRendererNativeFont systemFontOfSize:fontSize];
    } else {
#if TARGET_OS_OSX
        NSFontManager *fontManager = [NSFontManager sharedFontManager];
#endif
        NSArray<NSString *> *possibleFontFamilies = [fontProperties.fontFamily componentsSeparatedByString:@","];
        for (NSString * __strong possibleFontFamily in possibleFontFamilies) {
            possibleFontFamily = [possibleFontFamily stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
            newFont = [TCVegaRendererNativeFont fontWithName:possibleFontFamily size:fontSize];
            if (newFont != nil && fontProperties.fontWeight != nil) {
                if ([fontProperties.fontWeight isEqualToString:@"bold"]) {
#if TARGET_OS_OSX
                    newFont = [fontManager convertFont:newFont toHaveTrait:NSBoldFontMask];
#else
                    UIFontDescriptor *descriptor = [[newFont fontDescriptor] fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitBold];
                    newFont = [TCVegaRendererNativeFont fontWithDescriptor:descriptor size:newFont.pointSize];
#endif
                    assert(newFont != nil);
                } else if (fontProperties.fontWeight.length == 3 &&
                           [[fontProperties.fontWeight substringFromIndex:1] isEqualToString:@"00"]) {
                    NSString *weightString = [fontProperties.fontWeight substringToIndex:1];
                    NSInteger weightInt = weightString.intValue;
                    NSInteger weight = -1;
                    // Convert fonts from a scale from 100 - 900, to
                    // a scale from 0 to 15. We only look at the first digit,
                    // which is why we switch on 1 through 9.
                    switch (weightInt) {
                        case 1:
                            weight = 1;
                            break;
                        case 2:
                            weight = 3;
                            break;
                        case 3:
                            weight = 4;
                            break;
                        case 4:
                            weight = 5;
                            break;
                        case 5:
                            weight = 6;
                            break;
                        case 6:
                            weight = 8;
                            break;
                        case 7:
                            weight = 9;
                            break;
                        case 8:
                            weight = 10;
                            break;
                        case 9:
                            weight = 12;
                            break;
                        default:
                            os_log_debug(TCVegaLogger.instance, "Encountered unexpected font weight %s", fontProperties.fontWeight.UTF8String);
                            assert(false);
                    }
#if TARGET_OS_OSX
                    newFont = [fontManager fontWithFamily:newFont.familyName traits:[fontManager traitsOfFont:newFont] weight:weight size:newFont.pointSize];
#else
                    // TODO apply font weights in non-macOS
#endif
                    assert(newFont != nil);
                } else {
                    // unexpected font weight
                    os_log_debug(TCVegaLogger.instance, "Encountered unexpected font weight %s", fontProperties.fontWeight.UTF8String);
                    assert(false);
                }
            }
            if (newFont != nil) {
                break;
            }
        }
    }

    if(newFont == nil) {
        newFont = [TCVegaRendererNativeFont systemFontOfSize:fontSize];
        // TODO should we be updating _font to reflect the system font we've fallen back to?
        os_log_debug(TCVegaLogger.instance, "The specified font: '%s' is unavailable. Falling back to '%s'.", fontStr.UTF8String, newFont.displayName.UTF8String);
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

- (NSString *)strokeStyle {
    return _strokeStyle;
}

- (BOOL)isPointInPathWithX:(double)x y:(double)y {
#if TARGET_OS_OSX
    CGAffineTransform transform = CGContextGetCTM(self.context);
    CGAffineTransform flipYAxis = [self.class flipYAxisWithHeight:self.height];
    CGContextConcatCTM(self.context, CGAffineTransformInvert(transform));
    CGContextConcatCTM(self.context, flipYAxis);
#endif
    BOOL inPath = CGContextPathContainsPoint(self.context, CGPointMake(x, y), kCGPathFillStroke);
#if TARGET_OS_OSX
    CGContextConcatCTM(self.context, CGAffineTransformInvert(flipYAxis));
    CGContextConcatCTM(self.context, transform);
#endif
    return inPath;
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
    } else if ([str length] >= 4 && [[str substringToIndex:4] isEqualToString:@"rgb("]) {
        // parse as RGB integers like rgb(r,g,b)
        str = [str stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        int r,g,b;
        NSString *commaSeparated = [str substringWithRange:NSMakeRange(4, str.length-5)];
        NSArray<NSString *> *components = [commaSeparated componentsSeparatedByString:@","];
        assert(components.count == 3);
        [[NSScanner scannerWithString:components[0]] scanInt:&r];
        [[NSScanner scannerWithString:components[1]] scanInt:&g];
        [[NSScanner scannerWithString:components[2]] scanInt:&b];
        return [self.class newColorFromR:(unsigned int)r G:(unsigned int)g B:(unsigned int)b A:255];
    } else if ([str length] >= 5 && [[str substringToIndex:5] isEqualToString:@"rgba("]) {
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
        return [self.class newColorFromR:(unsigned int)r G:(unsigned int)g B:(unsigned int)b A:(unsigned int)(a*255.0)];
    } else {
        return [self.class newColorFromString:[[TCVegaCGColorMap map] objectForKey:str]];
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

- (JSValue *)measureText:(NSString *)text {
    NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:text attributes:self.textAttributes];
    TCVegaCGTextMetrics *ret = [[TCVegaCGTextMetrics alloc] init];
    ret.width = attrStr.size.width;
    return [TCVegaLogProxy wrapObject:ret];
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

- (void)strokeTextWithString:(NSString *)string x:(double)x y:(double)y {
    [self textWithString:string x:x y:y drawingMode:kCGTextStroke];
}

- (void)fillTextWithString:(NSString *)string x:(double)x y:(double)y {
    [self textWithString:string x:x y:y drawingMode:kCGTextFill];
}

- (void)textWithString:(NSString *)string x:(double)x y:(double)y drawingMode:(CGTextDrawingMode)mode {
    // Get rid of the transformations on the context
    CGContextSaveGState(self.context);
#if TARGET_OS_OSX
    CGAffineTransform flipYAxis = [self.class flipYAxisWithHeight:self.height];
    CGContextConcatCTM(self.context, CGAffineTransformInvert(flipYAxis));
#endif
    
    // Reapply the transformations, but only on the coordinates,
    // so we don't flip the text upside down
#if TARGET_OS_OSX
    CGPoint coords = CGPointApplyAffineTransform(CGPointMake(x, y), flipYAxis);
#else
    CGPoint coords = CGPointMake(x, y);
#endif
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
    CGContextSetTextDrawingMode(self.context, mode);
    
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
    if (_fillStyle.isString) {
        CGColorRef color = [self.class newColorFromString:_fillStyle.toString];
        CGContextSetFillColorWithColor(self.context, color);
        CGColorRelease(color);
        CGContextFillPath(self.context);
    } else {
        assert(_fillStyle.isObject);
        assert([_fillStyle.toObject isKindOfClass:TCVegaCGLinearGradient.class]);
        TCVegaCGLinearGradient *gradient = _fillStyle.toObject;
        [gradient fillWithContext:self.context];
    }
    CGContextAddPath(self.context, currentPath);
    CGPathRelease(currentPath);
}

- (JSValue *)createLinearGradientWithX0:(double)x0 y0:(double)y0 x1:(double)x1 y1:(double)y1 {
    TCVegaCGLinearGradient *ret = [[TCVegaCGLinearGradient alloc] initWithX0:x0 y0:y0 x1:x1 y1:y1];
    assert(JSContext.currentContext != nil);
    return [JSValue valueWithObject:ret inContext:JSContext.currentContext];
}



@synthesize pixelRatio;

@end

@implementation TCVegaCGCanvas

@synthesize context;

- (instancetype)init {
    self = [super initWithTagName:@"canvas"];
    if(self) {
        self.context = [[TCVegaCGContext alloc] init];
    }
    return self;
}

- (TCVegaCGContext *)getContext:(NSString *)type {
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
