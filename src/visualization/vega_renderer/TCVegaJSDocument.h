/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "TCVegaHTMLElement.h"
#import "TCVegaJSCanvas.h"

@protocol TCVegaJSDocumentInterface<JSExport>

@property (nonatomic, strong) TCVegaHTMLElement* body;

JSExportAs(createElement,
           -(TCVegaHTMLElement*)createElementWithString:(NSString*)element
           );

@end

@interface TCVegaJSDocument : NSObject<TCVegaJSDocumentInterface>

-(instancetype)initWithCanvas:(TCVegaCGCanvas*)vegaCanvas;
@property (atomic, strong) TCVegaCGCanvas* canvas;

@end
