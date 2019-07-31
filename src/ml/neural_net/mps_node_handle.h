#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

NS_ASSUME_NONNULL_BEGIN

@interface TCMPSGraphNodeHandle : NSObject <MPSHandle>
+ (nullable instancetype)handleWithLabel:(NSString *)label;
- (nullable instancetype)initWithLabel:(NSString *)label;
- (NSString *)label;
- (BOOL)isEqual:(id)what;
- (nullable instancetype)initWithCoder:(NSCoder *)aDecoder;
- (void)encodeWithCoder:(NSCoder *)aCoder;
+ (BOOL)supportsSecureCoding;
@end

NS_ASSUME_NONNULL_END