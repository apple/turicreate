/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "TCVegaLogger.h"
#import "TCVegaRenderer.h"
#import "TCVegaJSCanvas.h"
#import "TCVegaJSConsole.h"
#import "TCVegaJSDocument.h"

#import "TCVegaPortabilityTypes.h"
#import <JavaScriptCore/JavaScriptCore.h>

#ifdef NDEBUG

// release mode, use minified JS
#include "vega-5.4.0.min.js.h"
#define vega_js vega_5_4_0_min_js
#define vega_js_size vega_5_4_0_min_js_len

#import "vega-lite-3.3.0.min.js.h"
#define vegalite_js vega_lite_3_3_0_min_js
#define vegalite_js_size vega_lite_3_3_0_min_js_len

#else

// debug mode, use unminified JS
#import "vega-5.4.0.js.h"
#define vega_js vega_5_4_0_js
#define vega_js_size vega_5_4_0_js_len

#import "vega-lite-3.3.0.js.h"
#define vegalite_js vega_lite_3_3_0_js
#define vegalite_js_size vega_lite_3_3_0_js_len

#endif

@interface TCVegaRenderer ()

@property (nonatomic, strong) NSString *spec;
@property (nonatomic, strong) TCVegaCGCanvas *vegaCanvas;
@property (nonatomic, strong) TCVegaJSDocument *vegaJSDocument;
@property (nonatomic) double scaleFactor;
-(TCVegaCGContext *)vegaContext;

@end

@implementation TCVegaRenderer

@synthesize context;
@synthesize spec;
@synthesize vegaCanvas;
@synthesize vegaJSDocument;
@synthesize scaleFactor;

-(instancetype) init {
    return [self initWithSpec:nil config:nil scaleFactor:1];
}

- (instancetype)initWithSpec:(NSString*)specStr {
    // use scaleFactor 2 to keep existing behavior for initWithSpec: method
    // see https://github.com/apple/turicreate/blob/5.7.1/src/visualization/vega_renderer/VegaRenderer.m#L145
    return [self initWithSpec:specStr config:nil scaleFactor:2];
}

-(instancetype) initWithSpec:(NSString *)specStr config:(NSString *)config scaleFactor:(double)providedScaleFactor {
    self = [super init];
    self.spec = specStr;
    self.scaleFactor = providedScaleFactor;

    // Initialize the JSContext first, so we can populate
    // the scene graph and get the width & height from spec
    self.context = [[JSContext alloc] init];

    @autoreleasepool {
        self.context[@"window"] = self.context.globalObject;
        self.context[@"window"][@"devicePixelRatio"] = [NSNumber numberWithDouble:self.scaleFactor];
        self.context[@"HTMLElement"] = TCVegaHTMLElement.class;
        
        // set up logging
        [TCVegaJSConsole attachToJavaScriptContext:self.context];

        self.vegaCanvas = [[TCVegaCGCanvas alloc] init];

        TCVegaJSDocument* document = [[TCVegaJSDocument alloc] initWithCanvas:self.vegaCanvas];
        self.vegaJSDocument = document;
        self.context[@"document"] = document;
        
        // set up an element to contain Vega's canvas, referenced in vg2canvasJS
        self.context[@"container"] = [[TCVegaHTMLElement alloc] initWithTagName:@"div"];

        // set up Image type
        self.context[@"Image"] = [JSValue valueWithObject:^() {
            assert([[JSContext currentArguments] count] == 0);
            return [[TCVegaCGImage alloc] init];
        } inContext:self.context];

        [self.context evaluateScript:[TCVegaRenderer vegaJS]];
        [self.context evaluateScript:[TCVegaRenderer vegaliteJS]];
        [self.context evaluateScript:[TCVegaRenderer vg2canvasJS]];
        JSValue* render_fn = self.context[@"viewRender"];

        if(spec != nil) {
            config = config == nil ? @"" : config;
            [render_fn callWithArguments:@[self.spec, config]];
        }
        assert(self.vegaCanvas!= nil);
    }
    return self;
}

- (TCVegaCGContext *)vegaContext {
    // Make sure we don't have any extra properties on the dictionary
    // backing the JSValue. If we do, it means we missed some property
    // implementations on the JSExport protocol for this type.
    if (self.vegaCanvas == nil) {
        return nil;
    }
    JSValue *jsVegaCanvas = [JSValue valueWithObject:self.vegaCanvas inContext:self.context];
    NSDictionary *jsVegaCanvasExtraProps = jsVegaCanvas.toDictionary;
    if (jsVegaCanvasExtraProps.count > 0) {
        os_log_error(TCVegaLogger.instance, "Encountered extra properties on canvas: %s", jsVegaCanvasExtraProps.debugDescription.UTF8String);
        assert(false);
    }

    JSValue *jsVegaContext = [jsVegaCanvas invokeMethod:@"getContext" withArguments:@[@"2d"]];
    NSDictionary *jsVegaContextExtraProps = jsVegaContext.toDictionary;
    if (jsVegaContextExtraProps.count > 0) {
        os_log_error(TCVegaLogger.instance, "Encountered extra properties on canvas context: %s", jsVegaContextExtraProps.debugDescription.UTF8String);
        assert(false);
    }

    TCVegaCGContext *vegaContext = [self.vegaCanvas getContext:@"2d"];
    assert(vegaContext == jsVegaContext.toObject);
    return vegaContext;
}

-(NSUInteger)width {
    return (NSUInteger)(self.vegaCanvas.context.width / self.scaleFactor);
}

-(NSUInteger)height {
    return (NSUInteger)(self.vegaCanvas.context.height / self.scaleFactor);
}

-(NSString*)cursor {
    return self.vegaJSDocument.body.style.cursor;
}

-(CGImageRef)CGImage {
    return CGBitmapContextCreateImage(self.vegaContext.context);
}

- (void)triggerEventWithType:(NSString *)type event:(TCVegaJSMouseEvent *)event {
    if(self.vegaCanvas.events[type] != nil) {
        JSValue* callback = self.vegaCanvas.events[type].value;
        [callback callWithArguments:@[event]];
    }
}

+ (NSString*)vg2canvasJS {
    return @
    "function viewRender(spec, config) {"
    "  config = config || {};"
    "  if (typeof config === 'string') {"
    "    config = JSON.parse(config);"
    "  }"
    "  if (typeof spec === 'string') {"
    "    spec = JSON.parse(spec);"
    "  }"
    "  const vlPrefix = 'https://vega.github.io/schema/vega-lite/';"
    "  const mode = spec['$schema'].startsWith(vlPrefix) ? 'vega-lite' : 'vega';"
    "  if (mode === 'vega-lite') {"
    "    spec = vl.compile(spec, { config: config }).spec;"
    "  }"
    "  const runtime = vega.parse(spec, mode === 'vega-lite' ? {} : config);"
    "  window.vegaView = new vega.View(runtime, {"
    "    logLevel: vega.Warn,"
    "    renderer: 'canvas',"
    "    hover: true"
    "  })"
    "  .initialize(container)"
    "  .runAsync()"
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
