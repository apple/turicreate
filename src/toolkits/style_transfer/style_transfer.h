#ifndef TURI_STYLE_TRANSFER_H_
#define TURI_STYLE_TRANSFER_H_

#import "utils.h"

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

API_AVAILABLE(macos(10.14))
@interface StyleTransferModel : NSObject
{}

- (id) initWithParameters:(NSString *)name
                inputNode:(MPSNNImageNode *)inputNode
                   device:(id<MTLDevice> _Nonnull)dev
                cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q
              initWeights:(struct weights*)w;

- (MPSNNImageNode *) forwardPass;
- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode;
- (MPSCNNNeuronSigmoid *) finalNode;

@end

#endif