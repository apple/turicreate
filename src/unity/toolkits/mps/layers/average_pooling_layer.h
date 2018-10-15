#ifndef turi_mps_average_pooling_layer_h
#define turi_mps_average_pooling_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

API_AVAILABLE(macos(10.13))
@interface AveragePoolingLayer: NSObject {
    NSString *mName;
    MPSCNNPoolingAverageNode *mPoolingNode;
}

- (id __nonnull) initWithParams:(NSString *__nonnull)name
                      inputNode:(MPSNNImageNode *__nonnull)inputNode
                    kernelWidth:(int)kernelWidth
                   kernelHeight:(int)kernelHeight
                    strideWidth:(int)strideWidth
                   strideHeight:(int)strideHeight;

- (MPSNNImageNode *__nonnull) resultImage;

@end
    
#endif
