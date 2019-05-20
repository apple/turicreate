#import <unity/toolkits/drawing_classifier/mps_drawing_classifier.hpp>

@implementation DrawingClassifierModel

- (id _Nullable)initWithParameters:(NSString *_Nonnull)name
                       initWeights:(turi::neural_net::float_array_map&)weights {

    // TODO: Replace with TCMPS Device Allocator
    mDev = MTLCreateSystemDefaultDevice();
    mCq = [mDev newCommandQueue];
    @autoreleasepool {
        self = [super init];

        MPSNNImageNode *inputNode = [MPSNNImageNode nodeWithHandle: nil];

        // d_tempInput = (float *) malloc(sizeof(d_inputArray));
        // weights["drawing_classifier_conv0_weight"].data(), weights["drawing_classifier_conv0_weight"].size()

        NSString* conv1handle = [name stringByAppendingString:@"drawing_classifier_conv0"];
        conv1 = [[ConvolutionalLayer alloc] initWithParameters:conv1handle
                                                                       kernelWidth:3
                                                                      kernelHeight:3
                                                              inputFeatureChannels:1
                                                             outputFeatureChannels:16
                                                                       strideWidth:1
                                                                      strideHeight:1
                                                                      paddingWidth:1
                                                                     paddingHeight:1
                                                                           weights:weights["drawing_classifier_conv0_weight"].data()
                                                                            biases:weights["drawing_classifier_conv0_bias"].data()
                                                                         inputNode:inputNode
                                                                            device:mDev
                                                                         cmd_queue:mCq];
        /*
        NSString* relu1handle = [name stringByAppendingString:@"relu1"];
        ReluLayer *relu1 = [[ReluLayer alloc] initWithParams:relu1handle
                                                   inputNode:[conv1 resultImage]];

        NSString* pooling1handle = [name stringByAppendingString:@"pooling1"];
        PoolingLayer* pooling1 = [[PoolingLayer alloc] initWithParams:pooling1handle
                                              inputNode:[relu1 resultImage]
                                            kernelWidth:2 
                                           kernelHeight:2 
                                            strideWidth:2 
                                           strideHeight:2];

        NSString* conv2handle = [name stringByAppendingString:@"drawing_classifier_conv1"];
        ConvolutionalLayer* conv2 = [[ConvolutionalLayer alloc] initWithParameters:conv2handle
                                                   kernelWidth:3
                                                  kernelHeight:3
                                          inputFeatureChannels:16
                                         outputFeatureChannels:32
                                                   strideWidth:1
                                                  strideHeight:1
                                                  paddingWidth:1
                                                 paddingHeight:1
                                                       weights:&weights["drawing_classifier_conv1_weight"][0]
                                                        biases:&weights["drawing_classifier_conv1_bias"][0]
                                                     inputNode:[pooling1 resultImage]
                                                        device:mDev
                                                     cmd_queue:mCq];


        NSString* relu2handle = [name stringByAppendingString:@"relu2"];
        ReluLayer *relu2 = [[ReluLayer alloc] initWithParams:relu2handle
                                        inputNode:[conv2 resultImage]];

        NSString* pooling2handle = [name stringByAppendingString:@"pooling2"];
        PoolingLayer* pooling2 = [[PoolingLayer alloc] initWithParams:pooling2handle
                                              inputNode:[relu2 resultImage]
                                            kernelWidth:2 
                                           kernelHeight:2 
                                            strideWidth:2 
                                           strideHeight:2];

        NSString* conv3handle = [name stringByAppendingString:@"drawing_classifier_conv2"];
        ConvolutionalLayer* conv3 = [[ConvolutionalLayer alloc] initWithParameters:conv3handle
                                                   kernelWidth:3
                                                  kernelHeight:3
                                          inputFeatureChannels:32
                                         outputFeatureChannels:64
                                                   strideWidth:1
                                                  strideHeight:1
                                                  paddingWidth:1
                                                 paddingHeight:1
                                                       weights:&weights["drawing_classifier_conv2_weight"][0]
                                                        biases:&weights["drawing_classifier_conv2_bias"][0]
                                                     inputNode:[pooling2 resultImage]
                                                        device:mDev
                                                     cmd_queue:mCq];

        NSString* relu3handle = [name stringByAppendingString:@"relu3"];
        ReluLayer *relu3 = [[ReluLayer alloc] initWithParams:relu3handle
                                        inputNode:[conv3 resultImage]];

        NSString* pooling3handle = [name stringByAppendingString:@"pooling3"];
        PoolingLayer* pooling3 = [[PoolingLayer alloc] initWithParams:pooling3handle
                                              inputNode:[relu3 resultImage]
                                            kernelWidth:2 
                                           kernelHeight:2 
                                            strideWidth:2 
                                           strideHeight:2];

        NSString* fc1handle = [name stringByAppendingString:@"drawing_classifier_dense0"];
        FullyConnected* fully_connected_1 = [[FullyConnected alloc] initWithParams:fc1handle
                                              inputFeatureChannels:64
                                             outputFeatureChannels:128
                                                       inputHeight:3
                                                        inputWidth:3
                                                           weights:&weights["drawing_classifier_dense0_weight"][0]
                                                            biases:&weights["drawing_classifier_dense0_bias"][0]
                                                         inputNode:[pooling3 resultImage]
                                                            device:mDev
                                                         cmd_queue:mCq];
        
        NSString* relu_fc_handle = [name stringByAppendingString:@"relu_fc"];
        ReluLayer *relu_fc_1 = [[ReluLayer alloc] initWithParams:relu_fc_handle
                                        inputNode:[fully_connected_1 resultImage]];
        
        
        NSString* fc2handle = [name stringByAppendingString:@"drawing_classifier_dense1"];
        FullyConnected* fully_connected_2 = [[FullyConnected alloc] initWithParams:fc2handle
                                              inputFeatureChannels:128
                                             outputFeatureChannels:10
                                                       inputHeight:1
                                                        inputWidth:1
                                                           weights:&weights["drawing_classifier_dense1_weight"][0]
                                                            biases:&weights["drawing_classifier_dense1_bias"][0]
                                                         inputNode:[relu_fc_1 resultImage]
                                                            device:mDev
                                                         cmd_queue:mCq];
        */
        return self;
    }
}

- (turi::neural_net::float_array_map) export_weights {

    // TODO: export weights
    return std::map<std::string, turi::neural_net::shared_float_array>();
}

- (turi::neural_net::float_array_map) predict:(turi::neural_net::float_array_map&)inputs {
    // TODO: predict
    return std::map<std::string, turi::neural_net::shared_float_array>();
}

- (turi::neural_net::float_array_map) train:(turi::neural_net::float_array_map&)inputs {
    // TODO: train
    return std::map<std::string, turi::neural_net::shared_float_array>();
}

@end

/*
@implementation DrawingClassifierModel

- (id _Nullable)initWithParameters:(NSString *_Nonnull)name
                         inputNode:(MPSNNImageNode *_Nonnull)inputNode
                            device:(id<MTLDevice> _Nonnull)dev
                         cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q
                       initWeights:(std::map<std::string, std::vector<float>>)weights {
    @autoreleasepool {
        self = [super init];

        NSString* conv1handle = [name stringByAppendingString:@"drawing_classifier_conv0"];
        conv1 = [[ConvolutionalLayer alloc] initWithParameters:conv1handle
                                                   kernelWidth:3
                                                  kernelHeight:3
                                          inputFeatureChannels:1
                                         outputFeatureChannels:16
                                                   strideWidth:1
                                                  strideHeight:1
                                                  paddingWidth:1
                                                 paddingHeight:1
                                                       weights:&weights["drawing_classifier_conv0_weight"][0]
                                                        biases:&weights["drawing_classifier_conv0_bias"][0]
                                                     inputNode:inputNode
                                                        device:mDev
                                                     cmd_queue:mCq];

        NSString* relu1handle = [name stringByAppendingString:@"relu1"];
        relu1 = [[ReluLayer alloc] initWithParams:relu1handle
                                        inputNode:[conv1 resultImage]];

        NSString* pooling1handle = [name stringByAppendingString:@"pooling1"];
        pooling1 = [[PoolingLayer alloc] initWithParams:pooling1handle
                                              inputNode:[relu1 resultImage]
                                            kernelWidth:2 
                                           kernelHeight:2 
                                            strideWidth:2 
                                           strideHeight:2];

        NSString* conv2handle = [name stringByAppendingString:@"drawing_classifier_conv1"];
        conv2 = [[ConvolutionalLayer alloc] initWithParameters:conv2handle
                                                   kernelWidth:3
                                                  kernelHeight:3
                                          inputFeatureChannels:16
                                         outputFeatureChannels:32
                                                   strideWidth:1
                                                  strideHeight:1
                                                  paddingWidth:1
                                                 paddingHeight:1
                                                       weights:&weights["drawing_classifier_conv1_weight"][0]
                                                        biases:&weights["drawing_classifier_conv1_bias"][0]
                                                     inputNode:[pooling1 resultImage]
                                                        device:mDev
                                                     cmd_queue:mCq];

        NSString* relu2handle = [name stringByAppendingString:@"relu2"];
        relu2 = [[ReluLayer alloc] initWithParams:relu2handle
                                        inputNode:[conv2 resultImage]];

        NSString* pooling2handle = [name stringByAppendingString:@"pooling2"];
        pooling2 = [[PoolingLayer alloc] initWithParams:pooling2handle
                                              inputNode:[relu2 resultImage]
                                            kernelWidth:2 
                                           kernelHeight:2 
                                            strideWidth:2 
                                           strideHeight:2];

        NSString* conv3handle = [name stringByAppendingString:@"drawing_classifier_conv2"];
        conv3 = [[ConvolutionalLayer alloc] initWithParameters:conv3handle
                                                   kernelWidth:3
                                                  kernelHeight:3
                                          inputFeatureChannels:32
                                         outputFeatureChannels:64
                                                   strideWidth:1
                                                  strideHeight:1
                                                  paddingWidth:1
                                                 paddingHeight:1
                                                       weights:&weights["drawing_classifier_conv2_weight"][0]
                                                        biases:&weights["drawing_classifier_conv2_bias"][0]
                                                     inputNode:[pooling2 resultImage]
                                                        device:mDev
                                                     cmd_queue:mCq];

        NSString* relu3handle = [name stringByAppendingString:@"relu3"];
        relu3 = [[ReluLayer alloc] initWithParams:relu3handle
                                        inputNode:[conv3 resultImage]];

        NSString* pooling3handle = [name stringByAppendingString:@"pooling3"];
        pooling3 = [[PoolingLayer alloc] initWithParams:pooling3handle
                                              inputNode:[relu3 resultImage]
                                            kernelWidth:2 
                                           kernelHeight:2 
                                            strideWidth:2 
                                           strideHeight:2];

        NSString* fc1handle = [name stringByAppendingString:@"drawing_classifier_dense0"];
        fully_connected_1 = [[FullyConnected alloc] initWithParams:fc1handle
                                              inputFeatureChannels:64
                                             outputFeatureChannels:128
                                                       inputHeight:3
                                                        inputWidth:3
                                                           weights:&weights["drawing_classifier_dense0_weight"][0]
                                                            biases:&weights["drawing_classifier_dense0_bias"][0]
                                                         inputNode:[pooling3 resultImage]
                                                            device:mDev
                                                         cmd_queue:mCq];
        
        NSString* relu_fc_handle = [name stringByAppendingString:@"relu_fc"];
        relu_fc_1 = [[ReluLayer alloc] initWithParams:relu_fc_handle
                                        inputNode:[fully_connected_1 resultImage]];
        
        
        NSString* fc2handle = [name stringByAppendingString:@"drawing_classifier_dense1"];
        FullyConnected* fully_connected_2 = [[FullyConnected alloc] initWithParams:fc2handle
                                              inputFeatureChannels:128
                                             outputFeatureChannels:10
                                                       inputHeight:1
                                                        inputWidth:1
                                                           weights:&weights["drawing_classifier_dense1_weight"][0]
                                                            biases:&weights["drawing_classifier_dense1_bias"][0]
                                                         inputNode:[relu_fc_1 resultImage]
                                                            device:mDev
                                                         cmd_queue:mCq];

        if (training_graph) {
            endNode = [fully_connected_2 resultImage];
        } else {
            NSString* softmax_handle = [name stringByAppendingString:@"relu_fc"];
            softmax_1 = [[SoftmaxLayer alloc] initWithParams:softmax_handle
                                               inputNode:[fully_connected_2 resultImage]];

            endNode = [softmax_1 resultImage];
        }

        return self;
    }
}

- (SoftmaxLayer * _Nonnull) finalLayer {
    return softmax_1;
}

- (MPSNNImageNode * _Nonnull) forwardPass {
    return endNode;
}

- (MPSNNImageNode * _Nonnull) backwardPass:(MPSNNImageNode *) inputNode {
    MPSNNGradientFilterNode *fcGrad2 = [[fully_connected_2 underlyingNode] gradientFilterWithSource:inputNode];
    MPSNNGradientFilterNode *reluFCGrad1 = [[relu_fc_1 underlyingNode] gradientFilterWithSource: [fcGrad2 resultImage]];
    MPSNNGradientFilterNode *fcGrad1 = [[relu_fc_1 underlyingNode] gradientFilterWithSource: [reluFCGrad1 resultImage]];

    MPSNNGradientFilterNode *poolingGrad3 = [[pooling3 underlyingNode] gradientFilterWithSource: [fcGrad1 resultImage]];
    MPSNNGradientFilterNode *reluGrad3 = [[relu3 underlyingNode] gradientFilterWithSource: [poolingGrad3 resultImage]];
    MPSNNGradientFilterNode *convGrad3 = [[conv3 underlyingNode] gradientFilterWithSource: [reluGrad3 resultImage]];
    
    MPSNNGradientFilterNode *poolingGrad2 = [[pooling2 underlyingNode] gradientFilterWithSource: [convGrad3 resultImage]];
    MPSNNGradientFilterNode *reluGrad2 = [[relu2 underlyingNode] gradientFilterWithSource: [poolingGrad2 resultImage]];
    MPSNNGradientFilterNode *convGrad2 = [[conv2 underlyingNode] gradientFilterWithSource: [reluGrad2 resultImage]];
    
    MPSNNGradientFilterNode *poolingGrad1 = [[pooling1 underlyingNode] gradientFilterWithSource: [convGrad2 resultImage]];
    MPSNNGradientFilterNode *reluGrad1 = [[relu1 underlyingNode] gradientFilterWithSource: [poolingGrad1 resultImage]];
    MPSNNGradientFilterNode *convGrad1 = [[conv1 underlyingNode] gradientFilterWithSource: [reluGrad1 resultImage]];
    
    return [convGrad1 resultImage];
}

@end
*/