/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import "TCVegaHTMLElement.h"
#import "TCVegaJSCanvas.h"

@implementation TCVegaCSSStyleDeclaration

// TODO: modifications to the cursor attribute should actually be applied
@synthesize cursor;

- (instancetype) init {
    self = [super init];
    if(self) {
        self.cursor = [[NSString alloc] init];
    }
    return self;
}

@end

@implementation TCVegaHTMLElement

@synthesize style;
@synthesize childNodes;
@synthesize tagName;
@synthesize events;

- (instancetype)init {
    return [self initWithTagName:nil];
}

- (instancetype)initWithTagName:(NSString*)tag {
    self = [super init];
    if(self) {
        self.style = [[TCVegaCSSStyleDeclaration alloc] init];
        self.childNodes = [[NSMutableArray alloc] init];
        self.events = [[NSMutableDictionary alloc] init];
        self.tagName = tag;
    }
    return self; 
}

- (TCVegaHTMLElement*) parentNode {
    return [[TCVegaHTMLElement alloc] initWithTagName:@"div"];
}

- (NSObject<TCVegaHTMLElementInterface>*)removeWithChild:(NSObject<TCVegaHTMLElementInterface>*)child {
    // NOTE: removeChild should technically return the object it removed, however, since the
    // return value is never used in Vega we can afford to always return nil.
   [self.childNodes removeObject:child];
   return nil;
}

- (NSObject<TCVegaHTMLElementInterface>*)appendWithChild:(NSObject<TCVegaHTMLElementInterface>*)child {
    // NOTE: appendChild should technically remove the child node from its current parent (if one exists)
    // since we currently don't keep track of parent nodes this can't be implemented.
    [self.childNodes addObject:child];
    return child;
}

- (void)setAttributeWithName:(NSString*)name value:(NSString*)value {
    // NOTE: we currently don't do anything. Might need to eventually.
    (void)name;
    (void)value;
}

- (void)addEventListenerWithType:(NSString*)type listener:(JSValue*)listener {
    JSManagedValue *value = [JSManagedValue managedValueWithValue:listener andOwner:self];
    [self.events setValue:value forKey:type];
}

- (NSDictionary*)getBoundingClientRect {
    // NOTE: Vega seems to use this to adjust the coordinates it recieves to match up with its internal
    // coordinate system. Returning zero seems to be working.
    return @{
        @"left": @0,
        @"top": @0
    };
}

@end
