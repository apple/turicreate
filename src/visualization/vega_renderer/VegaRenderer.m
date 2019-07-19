/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "VegaRenderer.h"
#import "JSCanvas.h"

#import <AppKit/AppKit.h>
#import <JavaScriptCore/JavaScriptCore.h>

#define Q(x) #x
#define QUOTE(x) Q(x)
#define IDENT(x) x
#define PATH(x,y) QUOTE(IDENT(x)IDENT(y))

#define VEGA_JS_H PATH(OBJROOT,/vega-4.4.0.min.js.h)
#import VEGA_JS_H
#define vega_js vega_4_4_0_min_js
#define vega_js_size vega_4_4_0_min_js_len

#define VEGALITE_JS_H PATH(OBJROOT,/vega-lite-3.0.0-rc10.min.js.h)
#import VEGALITE_JS_H
#define vegalite_js vega_lite_3_0_0_rc10_min_js
#define vegalite_js_size vega_lite_3_0_0_rc10_min_js_len

@interface VegaRenderer ()

@property (strong) NSString *spec;
@property (strong) JSContext *context;
@property (strong) VegaCGCanvas *vegaCanvas;
-(VegaCGContext *)vegaContext;

@end

@implementation VegaRenderer 

-(instancetype) initWithSpec:(NSString *)spec {
    return [self initWithSpec:spec context:[[NSGraphicsContext currentContext] CGContext]];
}

-(instancetype) initWithSpec:(NSString *)spec
                     context:(CGContextRef)parentContext {
    self = [super init];
    self.spec = spec;

    // Initialize the JSContext first, so we can populate
    // the scene graph and get the width & height from spec
    self.context = [[JSContext alloc] init];

    __unsafe_unretained typeof(self) weakSelf = self;

    @autoreleasepool {

        // set up logging
        [self.context evaluateScript:@"var console = {};"];
        self.context[@"console"][@"log"] = ^() {
            NSArray *message = [JSContext currentArguments];
            NSLog(@"JS console log: %@", message);
        };
        self.context[@"console"][@"warn"] = ^() {
            NSArray *message = [JSContext currentArguments];
            NSLog(@"JS console warning: %@", message);
        };
        self.context[@"console"][@"error"] = ^() {
            NSArray *message = [JSContext currentArguments];
            NSLog(@"JS console error: %@", message);
            assert(false);
        };

        // set up error handling
        self.context.exceptionHandler = ^(JSContext *context, JSValue *exception) {
            NSLog(@"Unhandled exception: %@", [exception toString]);
            NSLog(@"In context: %@", [context debugDescription]);
            assert(false);
        };

        JSValue *require = [JSValue valueWithObject:^(NSString *module) {
            if ([module isEqualToString:@"canvas"]) {
                JSValue *canvas2 = [JSValue valueWithObject:^(double width, double height) {
                    weakSelf.vegaCanvas = [[VegaCGCanvas alloc] initWithWidth:width height:height context:parentContext];
                    return weakSelf.vegaCanvas;
                } inContext:weakSelf.context];
                canvas2[@"Image"] = [JSValue valueWithObject:^() {
                    assert([[JSContext currentArguments] count] == 0);
                    return [[VegaCGImage alloc] init];
                } inContext:weakSelf.context];
                return canvas2;
            }

            // fall through if we don't know what module it is
            NSLog(@"Called require with unknown module %@", module);
            return [JSValue valueWithNullInContext:weakSelf.context];
        } inContext:self.context];

        [self.context setObject:require forKeyedSubscript:@"require"];

        [self.context evaluateScript:[VegaRenderer vegaJS]];
        [self.context evaluateScript:[VegaRenderer vegaliteJS]];
        [self.context evaluateScript:[VegaRenderer vg2canvasJS]];
        JSValue* render_fn = self.context[@"viewRender"];
        [render_fn callWithArguments:@[self.spec]];

        assert(self.vegaCanvas!= nil);

    }

    return self;
}

- (VegaCGContext *)vegaContext {
    // Make sure we don't have any extra properties on the dictionary
    // backing the JSValue. If we do, it means we missed some property
    // implementations on the JSExport protocol for this type.
    if (self.vegaCanvas == nil) {
        return nil;
    }
    JSValue *jsVegaCanvas = [JSValue valueWithObject:self.vegaCanvas inContext:self.context];
    NSDictionary *jsVegaCanvasExtraProps = jsVegaCanvas.toDictionary;
    if (jsVegaCanvasExtraProps.count > 0) {
        NSLog(@"Encountered extra properties on canvas: %@", jsVegaCanvasExtraProps);
        assert(false);
    }

    JSValue *jsVegaContext = [jsVegaCanvas invokeMethod:@"getContext" withArguments:@[@"2d"]];
    NSDictionary *jsVegaContextExtraProps = jsVegaContext.toDictionary;
    if (jsVegaContextExtraProps.count > 0) {
        NSLog(@"Encountered extra properties on canvas context: %@", jsVegaContextExtraProps);
        assert(false);
    }

    VegaCGContext *vegaContext = [self.vegaCanvas getContext:@"2d"];
    assert(vegaContext == jsVegaContext.toObject);
    return vegaContext;
}

-(NSUInteger)width {
    return self.vegaCanvas.context.width;
}

-(NSUInteger)height {
    return self.vegaCanvas.context.height;
}

-(CGLayerRef)CGLayer {
    return self.vegaContext.layer;
}

-(CGImageRef)CGImage {
    double scaleFactor = 2.0;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    NSUInteger bitsPerComponent = 8;
    CGContextRef bitmapContext = CGBitmapContextCreate(NULL, self.width * scaleFactor, self.height * scaleFactor, bitsPerComponent, 0, colorSpace, kCGImageAlphaPremultipliedLast);
    CGContextScaleCTM(bitmapContext, scaleFactor, scaleFactor);
    CGLayerRef layer = self.vegaContext.layer;
    CGContextDrawLayerAtPoint(bitmapContext, CGPointMake(0, 0), layer);
    CGImageRef image = CGBitmapContextCreateImage(bitmapContext);
    CGColorSpaceRelease(colorSpace);
    CGContextRelease(bitmapContext);
    return image;
}

+ (NSString*)vg2canvasJS {
    return @
    "function viewRender(spec) {"
    "  if (typeof spec === 'string') {"
    "    spec = JSON.parse(spec);"
    "  }"
    "  if (spec['$schema'].startsWith('https://vega.github.io/schema/vega-lite/')) {"
    "    spec = vl.compile(spec).spec;"
    "  }"
    "  return new vega.View(vega.parse(spec), {"
    "    logLevel: vega.Warn,"
    "    renderer: 'canvas'"
    "  })"
    "  .initialize()"
    "  .runAsync()"
    "  .then(view => {"
    "    view.toCanvas().then(canvas => {"
    "    })"
    "    .catch(err => { console.error(err); });"
    "  })"
    "  .catch(err => { console.error(err); });"
    "}";
}

+ (NSString*)vegaJS {

    NSData *data = [NSData dataWithBytes:vega_js length:vega_js_size];
    NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    return str;
}

+ (NSString*)vegaliteJS {

    NSData *data = [NSData dataWithBytes:vegalite_js length:vegalite_js_size];
    NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    return str;
}

@end
