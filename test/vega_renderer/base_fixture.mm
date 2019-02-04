/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "base_fixture.hpp"

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#import <vega_renderer/VegaRenderer.h>
#import "vega_webkit_renderer.hpp"

using namespace vega_renderer::test_utils;

#define Q(x) #x
#define QUOTE(x) Q(x)

// out of 100.0; chosen arbitrarily
double base_fixture::acceptable_diff = 2.02;

void base_fixture::replace_url_with_values_in_data_entry(CFMutableDictionaryRef cfDataEntry) {
    NSMutableDictionary *dataEntry = (__bridge NSMutableDictionary*)cfDataEntry;
    NSError *error = nil;
    if ([dataEntry objectForKey:@"url"]) {
        NSString *dataUrl = dataEntry[@"url"];
        TS_ASSERT_DIFFERS(dataUrl, nil);
        [dataEntry removeObjectForKey:@"url"];
        
        // Assumes a base URL of https://vega.github.io/editor/
        // since that's where the examples all come from
        NSURL *fullUrl = [NSURL URLWithString:dataUrl relativeToURL:[NSURL URLWithString:@"https://vega.github.io/editor/"]];
        
        if ([[dataUrl substringFromIndex:(dataUrl.length - 4)] isEqualToString:@".csv"]) {
            NSString *valuesStr = [NSString stringWithContentsOfURL:fullUrl encoding:NSUTF8StringEncoding error:&error];
            TS_ASSERT_EQUALS(error, nil);
            TS_ASSERT_DIFFERS(valuesStr, nil);
            dataEntry[@"values"] = valuesStr;
            dataEntry[@"format"] = @{@"type": @"csv"};
        } else {
            TS_ASSERT([[dataUrl substringFromIndex:(dataUrl.length - 5)] isEqualToString:@".json"]);
            NSData *valuesData = [NSData dataWithContentsOfURL:fullUrl];
            TS_ASSERT_DIFFERS(valuesData, nil);
            NSArray *values = [NSJSONSerialization JSONObjectWithData:valuesData options:0 error:&error];
            TS_ASSERT_EQUALS(error, nil);
            TS_ASSERT_DIFFERS(values, nil);
            dataEntry[@"values"] = values;
        }
    }
}

std::string base_fixture::replace_urls_with_values_in_spec(const std::string& spec) {
    NSString *specStr = [NSString stringWithUTF8String:spec.c_str()];
    NSData *specData = [specStr dataUsingEncoding:NSUTF8StringEncoding];
    NSError *error = nil;
    NSMutableDictionary *dict = [NSJSONSerialization JSONObjectWithData:specData options:(NSJSONReadingMutableContainers | NSJSONReadingMutableLeaves) error:&error];
    TS_ASSERT_EQUALS(error, nil);
    TS_ASSERT_DIFFERS(dict, nil);
    
    id dataEntries = dict[@"data"];
    TS_ASSERT_DIFFERS(dataEntries, nil);
    if ([dataEntries isKindOfClass:NSMutableArray.class]) {
        for (id dataEntry in dataEntries) {
            TS_ASSERT([dataEntry isKindOfClass:NSMutableDictionary.class]);
            replace_url_with_values_in_data_entry((__bridge CFMutableDictionaryRef)dataEntry);
        }
    } else {
        TS_ASSERT([dataEntries isKindOfClass:NSMutableDictionary.class]);
        replace_url_with_values_in_data_entry((__bridge CFMutableDictionaryRef)dataEntries);
    }
    NSData *replacedSpecData = [NSJSONSerialization dataWithJSONObject:dict options:0 error:&error];
    TS_ASSERT_EQUALS(error, nil);
    TS_ASSERT_DIFFERS(replacedSpecData, nil);
    NSString *ret = [[NSString alloc] initWithData:replacedSpecData encoding:NSUTF8StringEncoding];
    TS_ASSERT_DIFFERS(ret, nil);
    return std::string(ret.UTF8String);
}

CGContextRef base_fixture::create_cgcontext(double width, double height) {
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, // Let CG allocate it for us
                                             width,
                                             height,
                                             8,
                                             0,
                                             colorSpace,
                                             kCGImageAlphaNoneSkipLast); // RGBA
    // draw a white background
    CGContextSaveGState(ctx);
    CGContextSetFillColorWithColor(ctx, NSColor.whiteColor.CGColor);
    CGContextFillRect(ctx, CGRectMake(0, 0, width, height));
    CGContextRestoreGState(ctx);
    CGColorSpaceRelease(colorSpace);
    return ctx;
}

double base_fixture::count_all_pixels(CGImageRef cgimg) {
    // black pixel is 0,0,0; counts as 0
    // white pixel is 1,1,1; counts as 3 (?)
    // TODO test the above assumptions
    NSImage *img = [[NSImage alloc] initWithCGImage:cgimg size:NSZeroSize];
    double total = 0.0;
    NSBitmapImageRep* imageRep = [[NSBitmapImageRep alloc] initWithData:[img TIFFRepresentation]];
    for (NSInteger y=0; y<img.size.height; y++) {
        for (NSInteger x=0; x<img.size.width; x++) {
            NSColor *pixel = [imageRep colorAtX:x y:y];
            total += pixel.redComponent;
            total += pixel.greenComponent;
            total += pixel.blueComponent;
        }
    }
    return total;
}

double base_fixture::compare_expected_with_actual(CGImageRef expected,
                                                  CGImageRef actual,
                                                  double acceptable_diff) {
    
    if (CGImageGetWidth(actual) != CGImageGetWidth(expected)) {
        TS_ASSERT(false);
        return 100.0;
    }
    
    if (CGImageGetHeight(actual) != CGImageGetHeight(expected)) {
        TS_ASSERT(false);
        return 100.0;
    }
    
    
    CGRect imageRect = CGRectMake(0, 0,
                                  CGImageGetWidth(actual),
                                  CGImageGetHeight(actual));
    double width = imageRect.size.width;
    double height = imageRect.size.height;
    
    // Get a new context to draw both onto
    CGContextRef ctx = create_cgcontext(width, height);
    
    // Draw the expected image on white background
    CGContextDrawImage(ctx, imageRect, expected);
    
    // Change the blend mode for the remaining drawing operations
    CGContextSetBlendMode(ctx, kCGBlendModeDifference);
    
    // Draw the actual image on white background
    CGContextRef ctxActual = create_cgcontext(width, height);
    CGContextDrawImage(ctxActual, imageRect, actual);
    CGImageRef actualNoAlpha = CGBitmapContextCreateImage(ctxActual);
    
    // Draw the actual image inverted(ish) on top
    CGContextDrawImage(ctx, imageRect, actualNoAlpha);
    
    // Grab the composed CGImage
    CGImageRef diffed = CGBitmapContextCreateImage(ctx);
    
    double totalCount = imageRect.size.width * imageRect.size.height;
    double diffCount = this->count_all_pixels(diffed);
    double diffPct = (diffCount / totalCount) * 100.0;
    TS_ASSERT_LESS_THAN(diffPct, acceptable_diff);
    
    CGImageRelease(actualNoAlpha);
    CGImageRelease(diffed);
    CGContextRelease(ctx);
    CGContextRelease(ctxActual);

    return diffPct;
}

void base_fixture::expected_rendering(const std::string& spec,
                                      std::function<void(CGImageRef image)> completion_handler) {
    @autoreleasepool {

        NSString *vg2pngJS = @"function viewRender(spec, scale) {"
        "    if (typeof spec === 'string') {"
        "        spec = JSON.parse(spec);"
        "    }"
        "    if (spec['$schema'].startsWith('https://vega.github.io/schema/vega-lite/')) {"
        "        spec = vl.compile(spec).spec;"
        "    }"
        "    new vega.View(vega.parse(spec), {"
        "        logLevel: vega.Warn,"
        "        renderer: 'canvas'"
        "    })"
        "    .initialize()"
        "    .runAsync()"
        "    .then(view => {"
        "        view.toCanvas(scale).then(canvas => {"
        "            window.webkit.messageHandlers.sendCanvasData.postMessage(canvas.toDataURL());"
        "        })"
        "        .catch(err => { window.webkit.messageHandlers.error.postMessage(err); });"
        "    })"
        "    .catch(err => { window.webkit.messageHandlers.error.postMessage(err); });"
        "}";

        NSString *script = @"viewRender(";
        script = [script stringByAppendingString:[NSString stringWithUTF8String:spec.c_str()]];
        script = [script stringByAppendingString:@", 2.0)"];

        XCTestExpectation *finishedRendering = [[XCTestExpectation alloc] initWithDescription:@"Wait for WKWebView to render the Vega spec"];
        XCTestExpectation *finishedRunningScript = [[XCTestExpectation alloc] initWithDescription:@"Wait for WKWebView to run all JavaScript"];

        WKWebViewConfiguration *webConfig = [[WKWebViewConfiguration alloc] init];
        VegaWebKitRenderer *webRenderer = [[VegaWebKitRenderer alloc] initWithExpectation:finishedRendering completionHandler:completion_handler];
        [[webConfig userContentController] addScriptMessageHandler:webRenderer name:@"sendCanvasData"];
        [[webConfig userContentController] addScriptMessageHandler:webRenderer name:@"error"];
        WKWebView *webView = [[WKWebView alloc] initWithFrame:CGRectZero configuration:webConfig];
        [webView loadHTMLString:@"" baseURL:nil];
        [webView evaluateJavaScript:VegaRenderer.vegaJS completionHandler:^(id _Nullable _, NSError * _Nullable error) {
            TS_ASSERT_EQUALS(error, nil);
            BOOST_TEST_MESSAGE("Finished loading vega.js.");
            [webView evaluateJavaScript:VegaRenderer.vegaliteJS completionHandler:^(id _Nullable _, NSError * _Nullable error) {
                TS_ASSERT_EQUALS(error, nil);
                BOOST_TEST_MESSAGE("Finished loading vega-lite.js.");
                [webView evaluateJavaScript:vg2pngJS completionHandler:^(id _Nullable _, NSError * _Nullable error) {
                    TS_ASSERT_EQUALS(error, nil);
                    BOOST_TEST_MESSAGE("Finished loading vg2png js.");
                    [webView evaluateJavaScript:script completionHandler:^(id _Nullable _, NSError * _Nullable error) {
                        if (error != nil) {
                            TS_WARN(error.debugDescription);
                        }
                        TS_ASSERT_EQUALS(error, nil);
                        BOOST_TEST_MESSAGE("Finished running JavaScript.");
                        [finishedRunningScript fulfill];
                    }];
                }];
            }];
        }];

        XCTWaiterResult result = [XCTWaiter waitForExpectations:@[finishedRendering, finishedRunningScript] timeout:60.0];
        TS_ASSERT_EQUALS(result, XCTWaiterResultCompleted);
        BOOST_TEST_MESSAGE("Returned from expected_rendering");
    }
}

void base_fixture::run_test_case_with_path(const std::string& path) {
    this->run_test_case_with_path(path, base_fixture::acceptable_diff);
}

void base_fixture::run_test_case_with_path(const std::string& path, double acceptable_diff) {
    @autoreleasepool {
        BOOST_TEST_MESSAGE("Running test case with path: " + path);
        const static std::string projectDir = QUOTE(PROJECT_DIR);
        std::string fullSpecPath = projectDir + "/examples/" + path;
        std::ifstream specStream(fullSpecPath, std::ios::in | std::ios::binary);
        TS_ASSERT(specStream.good());
        std::string spec = std::string((std::istreambuf_iterator<char>(specStream)), std::istreambuf_iterator<char>());
        spec = this->replace_urls_with_values_in_spec(spec);
        VegaRenderer *view = [[VegaRenderer alloc] initWithSpec:[NSString stringWithUTF8String:spec.c_str()]];
        CGImageRef actual = view.CGImage;
        this->expected_rendering(spec, [this, &actual, &path, acceptable_diff](CGImageRef expected) {
            TS_ASSERT_DIFFERS(expected, nil);
            double diffPct = this->compare_expected_with_actual(expected, actual, acceptable_diff);
            BOOST_TEST_MESSAGE(path + " differed by " + std::to_string(diffPct));
            CGImageRelease(actual);
        });
    }
}