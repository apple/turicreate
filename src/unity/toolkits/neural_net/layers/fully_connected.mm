#import "fully_connected.h"

@implementation FullyConnected: NSObject

- (id _Nonnull) initWithParams:(NSString * _Nonnull)name
          inputFeatureChannels:(int)inputFeatureChannels
         outputFeatureChannels:(int)outputFeatureChannels
                   inputHeight:(int)inputHeight
                    inputWidth:(int)inputWidth
                       weights:(float * _Nonnull)weights
                        biases:(float * _Nonnull)biases
                     inputNode:(MPSNNImageNode * _Nonnull)inputNode
                        device:(id<MTLDevice> _Nonnull)dev
                     cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q {
    @autoreleasepool {
        self = [self init];
        mName = name;
        mInputNode = inputNode;
        
        mInputFeatureChannels = inputFeatureChannels;
        mOutputFeatureChannels = outputFeatureChannels;
        
        mWeight = weights;
        mBiases = biases;
        
        mKernelWidth = inputWidth;
        mKernelHeight = inputHeight;
        
        mDevice = dev;
        
        mFullyConnectedDataLoad = [[FullyConnectedDataLoader alloc] initWithParams:mName
                                                              inputFeatureChannels:mInputFeatureChannels
                                                             outputFeatureChannels:mOutputFeatureChannels
                                                                       inputHeight:mKernelHeight
                                                                        inputWidth:mKernelWidth
                                                                           weights:mWeight
                                                                            biases:mBiases
                                                                            device:dev
                                                                         cmd_queue:cmd_q];
        
        mFullyConnectedNode = [MPSCNNFullyConnectedNode nodeWithSource:mInputNode weights:mFullyConnectedDataLoad];
        return self;
    }
}

- (MPSNNImageNode * _Nonnull) resultImage{
    return mFullyConnectedNode.resultImage;
}
    
- (MPSCNNFullyConnectedNode * _Nonnull) underlyingNode{
    return mFullyConnectedNode;
}

@end
