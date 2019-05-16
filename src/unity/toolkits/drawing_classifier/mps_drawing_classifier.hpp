#ifndef TURI_MPS_DRAWING_CLASSIGFIER_H_
#define TURI_MPS_DRAWING_CLASSIGFIER_H_

#include <map>
#include <string>
#include <vector>

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import <unity/toolkits/neural_net/layers/convolution_layer.h>
#import <unity/toolkits/neural_net/layers/instance_norm_layer.h>
#import <unity/toolkits/neural_net/layers/fully_connected.h>
#import <unity/toolkits/neural_net/layers/relu_layer.h>
#import <unity/toolkits/neural_net/layers/sigmoid_layer.h>
#import <unity/toolkits/neural_net/layers/softmax_layer.h>
#import <unity/toolkits/neural_net/layers/upsampling_layer.h>
#import <unity/toolkits/neural_net/layers/addition_layer.h>
#import <unity/toolkits/neural_net/layers/multiplication_layer.h>
#import <unity/toolkits/neural_net/layers/subtraction_layer.h>
#import <unity/toolkits/neural_net/layers/pooling_layer.h>

API_AVAILABLE(macos(10.14))
@interface DrawingClassifierModel : NSObject {
	ConvolutionalLayer *conv1;
	ReluLayer* relu1;
	PoolingLayer* pooling1;

	ConvolutionalLayer *conv2;
	ReluLayer* relu2;
	PoolingLayer* pooling2;

	ConvolutionalLayer *conv3;
	ReluLayer* relu3;
	PoolingLayer* pooling3;

	FullyConnected* fully_connected_1;
	ReluLayer* relu_fc_1;
	FullyConnected* fully_connected_2;

	SoftmaxLayer* softmax_1;

	MPSNNImageNode *endNode;

	id<MTLDevice> mDev;
	id<MTLCommandQueue> mCq;
}

- (id _Nullable)initWithParameters:(NSString *_Nonnull)name
                         inputNode:(MPSNNImageNode *_Nonnull)inputNode
                            device:(id<MTLDevice> _Nonnull)dev
                         cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q
                       initWeights:(std::map<std::string, std::vector<float>>)weights;


- (MPSNNImageNode * _Nonnull) forwardPass;
- (SoftmaxLayer * _Nonnull) finalLayer;
- (MPSNNImageNode * _Nonnull) backwardPass:(MPSNNImageNode *)inputNode;
@end

#endif