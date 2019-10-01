/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import <JavaScriptCore/JavaScriptCore.h>

@class TCVegaCGCanvas;

@protocol TCVegaCSSStyleDeclarationInterface<JSExport>

@property (nonatomic, strong) NSString* cursor;

@end

@interface TCVegaCSSStyleDeclaration : NSObject<TCVegaCSSStyleDeclarationInterface>
@end

@protocol TCVegaHTMLElementInterface<JSExport>

@property (nonatomic, strong) TCVegaCSSStyleDeclaration* style;
@property (nonatomic, strong) NSMutableArray<NSObject<TCVegaHTMLElementInterface>*>* childNodes;
@property (nonatomic, strong) NSString* tagName;
@property (nonatomic, strong) NSMutableDictionary<NSString*, JSManagedValue*>* events;

- (instancetype)initWithTagName:(NSString*)tagName;
- (NSDictionary*)getBoundingClientRect;
- (instancetype)parentNode;

JSExportAs(removeChild,
           - (NSObject<TCVegaHTMLElementInterface>*)removeWithChild:(NSObject<TCVegaHTMLElementInterface>*)child
           );
JSExportAs(appendChild,
            - (NSObject<TCVegaHTMLElementInterface>*)appendWithChild:(NSObject<TCVegaHTMLElementInterface>*)child
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

@interface TCVegaHTMLElement : NSObject<TCVegaHTMLElementInterface>
-(instancetype)initWithTagName:(NSString *)tagName NS_DESIGNATED_INITIALIZER;
@end
