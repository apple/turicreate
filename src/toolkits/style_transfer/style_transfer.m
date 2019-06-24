#import "style_transfer.h"

@implementation StyleTransferModel

- (id) initWithParameters:(NSString *)name
                inputNode:(MPSNNImageNode *)inputNode
                   device:(id<MTLDevice> _Nonnull)dev
                cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q
              initWeights:(struct weights*)w {
  @autoreleasepool {
    self = [super init];
    return self;
  }
}

- (MPSNNImageNode *) forwardPass {
  return nil;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  return nil;
}

- (MPSCNNNeuronSigmoid *) finalNode {
  return nil;
}

@end