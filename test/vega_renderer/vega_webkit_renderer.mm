#import "vega_webkit_renderer.hpp"

@implementation VegaWebKitRenderer

- (instancetype)initWithExpectation:(XCTestExpectation*)expectation
                  completionHandler:(VegaRenderingCompletionHandler)completionHandler {
    self = [super init];
    self.expectation = expectation;
    self.completionHandler = completionHandler;
    return self;
}

- (void)userContentController:(nonnull WKUserContentController *)userContentController didReceiveScriptMessage:(nonnull WKScriptMessage *)message {
    
    if ([message.name isEqualToString:@"sendCanvasData"]) {
        NSString *body = message.body;
        assert([[body substringToIndex:22] isEqualToString:@"data:image/png;base64,"]);
        body = [body substringFromIndex:22];
        NSData *data = [[NSData alloc] initWithBase64EncodedString:body options:0];
        NSImage *image = [[NSImage alloc] initWithData:data];
        assert(image != nil);
        CGRect imageRect = CGRectMake(0, 0, image.size.width, image.size.height);
        CGImageRef cgImage = [image CGImageForProposedRect:&imageRect context:nil hints:nil];
        assert(cgImage != nil);
        self.completionHandler(cgImage);
    } else {
        // unexpected message or error
        assert(false);
        self.completionHandler(nil);
    }
    [self.expectation fulfill];
}

@end
