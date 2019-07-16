#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

#import <ml/neural_net/mps_descriptor_utils.h>

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(10.14))
@interface TCMPSVgg16Block2 : NSObject 

@property (nonatomic) MPSNNImageNode *features;
@property (nonatomic) MPSNNImageNode *output;

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSVgg16Block2Descriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights;

- (MPSNNImageNode * _Nullable) backwardPass:(MPSNNImageNode *) inputNode;

@end

NS_ASSUME_NONNULL_END