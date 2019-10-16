/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import "TCVegaJSDocument.h"

@implementation TCVegaJSDocument 

@synthesize body;
@synthesize canvas;

- (instancetype)initWithCanvas:(TCVegaCGCanvas*)canvasElement {
    self = [super init];
    if(self) {
        self.body = [[TCVegaHTMLElement alloc] init];
        self.canvas = canvasElement;
    }
    return self;
};

- (TCVegaHTMLElement*)createElementWithString:(NSString*)element {
    if([element isEqualToString:@"canvas"]){
        return self.canvas;
    }
    return [[TCVegaHTMLElement alloc] initWithTagName:element];
}

@end
