#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

#import <ml/neural_net/mps_descriptor_utils.h>

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(10.14))
@interface TCMPSVgg16Network : NSObject 

@property (nonatomic) MPSNNImageNode *reluOut1;
@property (nonatomic) MPSNNImageNode *reluOut2;
@property (nonatomic) MPSNNImageNode *reluOut3;
@property (nonatomic) MPSNNImageNode *reluOut4;

@property (nonatomic) MPSNNImageNode *output;

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSVgg16Block2Descriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSDictionary *> *) weights;

- (MPSNNImageNode * _Nullable) backwardPass:(MPSNNImageNode *) inputNode;

@end

NS_ASSUME_NONNULL_END