#include <functional>

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>
#import <XCTest/XCTest.h>

typedef std::function<void(CGImageRef image)> VegaRenderingCompletionHandler;

@interface VegaWebKitRenderer : NSObject<WKScriptMessageHandler>

@property XCTestExpectation *expectation;
@property VegaRenderingCompletionHandler completionHandler;

- (instancetype)initWithExpectation:(XCTestExpectation*)expectation
                  completionHandler:(VegaRenderingCompletionHandler)completionHandler;

@end