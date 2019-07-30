#import "mps_node_handle.h"

@implementation TCMPSGraphNodeHandle {
  NSString *_label;
}

+ (instancetype)handleWithLabel:(NSString *)label {
  return [[self alloc] initWithLabel:label];
}

- (instancetype)initWithLabel:(NSString *)label {
  self = [super init];
  if (nil == self)
    return self;
  _label = label;
  return self;
}

- (NSString *)label {
  return _label;
}

- (BOOL)isEqual:(id)what {
  return [_label isEqual:((TCMPSGraphNodeHandle *)what).label];
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
  self = [super init];
  if (nil == self)
    return self;

  _label =
      [aDecoder decodeObjectOfClass:NSString.class forKey:@"TCMPSGraphNodeHandleLabel"];
  return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {
  [aCoder encodeObject:_label forKey:@"TCMPSGraphNodeHandleLabel"];
}

+ (BOOL)supportsSecureCoding {
  return YES;
}

@end