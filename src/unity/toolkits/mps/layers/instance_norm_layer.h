#ifndef turi_mps_instance_norm_layer_h
#define turi_mps_instance_norm_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import <unity/toolkits/mps/layers/instance_norm_data_loader.h>

API_AVAILABLE(macos(10.13.4))
@interface InstanceNormLayer : NSObject
{
    NSString *mName;
    
    int mChannels;
    int mStyles;
    
    float **mGamma;
    float **mBeta;

    MPSNNImageNode *mInputNode; 
    MPSCNNInstanceNormalizationNode *mInstNormNode;
    InstanceNormDataLoader *mInstNormDataLoad;
        
    id<MTLDevice> mDevice;
}

- (id __nonnull) initWithParameters:(NSString *__nonnull)name
                           channels:(int)channels
                             styles:(int)styles
                              gamma:(float *__nonnull*__nonnull)gamma
                               beta:(float *__nonnull*__nonnull)beta
                          inputNode:(MPSNNImageNode *__nonnull)inputNode
                             device:(id<MTLDevice> __nonnull)dev;

- (MPSCNNInstanceNormalizationNode *__nonnull) underlyingNode;
- (InstanceNormDataLoader *__nonnull) underlyingDataLoader;
- (MPSNNImageNode *__nonnull) resultImage;

@end

#endif
