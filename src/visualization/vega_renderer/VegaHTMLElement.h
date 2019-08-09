/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import <JavaScriptCore/JavaScriptCore.h>

@class VegaCGCanvas;

@protocol VegaCSSStyleDeclarationInterface<JSExport>

@property NSString* cursor;

@end

@interface VegaCSSStyleDeclaration : NSObject<VegaCSSStyleDeclarationInterface>
@end

@protocol VegaHTMLElementInterface<JSExport>

@property VegaCSSStyleDeclaration* style;
@property NSMutableArray<NSObject<VegaHTMLElementInterface>*>* childNodes;
@property NSString* tagName;

- (instancetype) initWithTagName:(NSString*)tagName;

JSExportAs(removeChild,
           - (NSObject<VegaHTMLElementInterface>*)removeWithChild:(NSObject<VegaHTMLElementInterface>*)child
           );
JSExportAs(appendChild,
            - (NSObject<VegaHTMLElementInterface>*)appendWithChild:(NSObject<VegaHTMLElementInterface>*)child
           );
JSExportAs(setAttribute, 
           - (void)setAttributeWithName:(NSString*)name 
           value:(NSString*)value
           );
JSExportAs(addEventListener,
            - (void)addEventListenerWithType:(NSString*)type
            listener:(JSValue*)listener
           );

@end

@interface VegaHTMLElement : NSObject<VegaHTMLElementInterface>
@end
