#import "VegaHTMLElement.h"
#import "JSCanvas.h"

@implementation VegaCSSStyleDeclaration

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

@implementation VegaHTMLElement

@synthesize style;
@synthesize childNodes;
@synthesize tagName;

- (instancetype)init {
    self = [super init];
    if(self) {
        self.style = [[VegaCSSStyleDeclaration alloc] init];
        self.childNodes = [[NSMutableArray alloc] init];
    }
    return self;
}

- (instancetype)initWithTagName:(NSString*)tag {
    self = [self init];
    if(self) {
        self.tagName = tag;
    }
    return self; 
}

- (NSObject<VegaHTMLElementInterface>*)removeWithChild:(NSObject<VegaHTMLElementInterface>*)child {
    // TODO: removeChild should technically return the object it removed, however, since the
    // return value is never used in Vega we can afford to always return nil.
   [self.childNodes removeObject:child];
   return nil;
}

- (NSObject<VegaHTMLElementInterface>*)appendWithChild:(NSObject<VegaHTMLElementInterface>*)child {
    // TODO: appendChild should technically remove the child node from its current parent (if one exists)
    // since we currently don't keep track of parent nodes this can't be implemented.
    [self.childNodes addObject:child];
    return child;
}

- (void)setAttributeWithName:(NSString*)name value:(NSString*)value {
    // TODO: we currently just log these calls. Not sure if we need to do anything.
    NSLog(@"call made to setAttribute(%@, %@)", name, value);
}

- (void)addEventListenerWithType:(NSString*)type listener:(JSValue*)listener {
    // TODO: store these listeners and trigger them when appropriate
    // NSLog(@"call made to addEventListener(%@, ...)", type);
}

@end
