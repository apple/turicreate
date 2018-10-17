#include <iostream>
#include <vector>
#include <map>

#import <Foundation/Foundation.h>

#import <unity/toolkits/mps/utils.h>
#import <unity/toolkits/mps/layer_helpers/types.h>

#include <unity/toolkits/mps/layer_helpers/addition.h>
#include <unity/toolkits/mps/layer_helpers/base.h>
#include <unity/toolkits/mps/layer_helpers/convolution.h>
#include <unity/toolkits/mps/layer_helpers/input.h>
#include <unity/toolkits/mps/layer_helpers/instance_norm.h>
#include <unity/toolkits/mps/layer_helpers/output.h>
#include <unity/toolkits/mps/layer_helpers/pooling.h>
#include <unity/toolkits/mps/layer_helpers/relu.h>
#include <unity/toolkits/mps/layer_helpers/sigmoid.h>
#include <unity/toolkits/mps/layer_helpers/upsampling.h>

#include <unity/toolkits/mps/layers/addition_layer.h>
#include <unity/toolkits/mps/layers/convolution_layer.h>
#include <unity/toolkits/mps/layers/instance_norm_layer.h>
#include <unity/toolkits/mps/layers/average_pooling_layer.h>
#include <unity/toolkits/mps/layers/relu_layer.h>
#include <unity/toolkits/mps/layers/sigmoid_layer.h>
#include <unity/toolkits/mps/layers/nearest_upsampling_layer.h>

namespace turi {
    namespace mps {

        std::shared_ptr<Graph> create_graph(std::map<std::string, std::shared_ptr<Layer>>& layer_dict, std::vector<std::shared_ptr<Layer>> layer_arr) {
            /* TODO: MOVE THIS INTO COMPILE IN THE GRAPH OBJECT */

            if (@available(macOS 10.13.4, *)) {
                /* TODO: MOVE THIS OUT */
                id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
                /* TODO: MOVE THIS OUT */

                NSMutableDictionary *mps_layer_dictionary = [[NSMutableDictionary alloc] init];
                for(auto const& l: layer_arr) {
                    switch (l->m_type) {
                        case layer_type::input:
                            {
                                std::shared_ptr<InputNode> node = std::dynamic_pointer_cast<InputNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                [mps_layer_dictionary setObject:[MPSNNImageNode nodeWithHandle: nil]  forKey:key];
                            }
                            break;

                        case layer_type::output:
                            {  
                                std::shared_ptr<OutputNode> node = std::dynamic_pointer_cast<OutputNode>(l);
                                NSMutableArray *output_layer_images = [[NSMutableArray alloc]init];
                                for(auto const& o: node->m_layers){
                                    NSString *layer_key = [NSString stringWithUTF8String:o.m_name.c_str()];
                                    MPSNNImageNode *layer_image = [[mps_layer_dictionary objectForKey:layer_key] resultImage];
                                    [output_layer_images addObject:layer_image];
                                }
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                [mps_layer_dictionary setObject:output_layer_images  forKey:key];
                            }
                            break;

                        case layer_type::addition:
                            {
                                std::shared_ptr<AdditionNode> node = std::dynamic_pointer_cast<AdditionNode>(l);
                                NSString *left_key = [NSString stringWithUTF8String:((node->m_left)->m_name).c_str()];
                                NSString *right_key = [NSString stringWithUTF8String:((node->m_right)->m_name).c_str()];
                                MPSNNImageNode *left_image = [[mps_layer_dictionary objectForKey:left_key] resultImage];
                                MPSNNImageNode *right_image = [[mps_layer_dictionary objectForKey:right_key] resultImage];
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                AdditionLayer* addition_layer = [[AdditionLayer alloc] initWithParams:key
                                                                                             leftNode:left_image
                                                                                            rightNode:right_image];
                                [mps_layer_dictionary setObject:addition_layer forKey:key];
                            }
                            break;

                        case layer_type::convolution:
                            {
                                std::shared_ptr<ConvolutionNode> node = std::dynamic_pointer_cast<ConvolutionNode>(l);
                                /* TODO: Take care of the bias being null */
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                MPSNNImageNode *input_image = [[mps_layer_dictionary objectForKey:input_key] resultImage];
                                ConvolutionalLayer *convolution_layer = [[ConvolutionalLayer alloc] initWithParameters:key
                                                                                                           kernelWidth:node->m_kernel_width
                                                                                                          kernelHeight:node->m_kernel_height
                                                                                                  inputFeatureChannels:node->m_input_feature_channels
                                                                                                 outputFeatureChannels:node->m_output_feature_channels
                                                                                                           strideWidth:node->m_stride_width
                                                                                                          strideHeight:node->m_stride_height
                                                                                                          paddingWidth:node->m_padding_width
                                                                                                         paddingHeight:node->m_padding_height
                                                                                                               weights:&(node->m_weights[0])
                                                                                                                biases:NULL
                                                                                                             inputNode:input_image
                                                                                                                device:dev];
                                [mps_layer_dictionary setObject:convolution_layer forKey:key];
                            }
                            break;

                        case layer_type::instance_norm:
                            {
                                std::shared_ptr<InstanceNormNode> node = std::dynamic_pointer_cast<InstanceNormNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                MPSNNImageNode *input_image = [[mps_layer_dictionary objectForKey:input_key] resultImage];
                                
                                std::vector<float*> gamma(node->m_gamma.size());
                                for (int i = 0; i < node->m_gamma.size(); ++i)
                                    gamma[i] = &*(node->m_gamma[i]).begin();

                                std::vector<float*> beta(node->m_beta.size());
                                for (int i = 0; i < node->m_beta.size(); ++i)
                                    beta[i] = &*(node->m_beta[i]).begin();

                                InstanceNormLayer *instance_norm_layer = [[InstanceNormLayer alloc] initWithParameters:key
                                                                                                              channels:node->m_channels
                                                                                                                styles:node->m_styles
                                                                                                                 gamma:&(gamma[0])
                                                                                                                  beta:&(beta[0])
                                                                                                             inputNode:input_image
                                                                                                                device:dev];
                                [mps_layer_dictionary setObject:instance_norm_layer forKey:key];
                            }
                            break;

                        case layer_type::pooling:
                            {
                                std::shared_ptr<PoolingNode> node = std::dynamic_pointer_cast<PoolingNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                MPSNNImageNode *input_image = [[mps_layer_dictionary objectForKey:input_key] resultImage];
                                AveragePoolingLayer* pooling_layer = [[AveragePoolingLayer alloc] initWithParams:key
                                                                                                       inputNode:input_image
                                                                                                     kernelWidth:node->m_kernel_width 
                                                                                                    kernelHeight:node->m_kernel_height 
                                                                                                     strideWidth:node->m_stride_in_pixels_x 
                                                                                                    strideHeight:node->m_stride_in_pixels_y];

                                [mps_layer_dictionary setObject:pooling_layer forKey:key];
                            }
                            break;

                        case layer_type::relu:
                            {
                                std::shared_ptr<ReluNode> node = std::dynamic_pointer_cast<ReluNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                MPSNNImageNode *input_image = [[mps_layer_dictionary objectForKey:input_key] resultImage];
                                ReluLayer* relu_layer = [[ReluLayer alloc] initWithParams:key
                                                                                inputNode:input_image];
                                [mps_layer_dictionary setObject:relu_layer forKey:key];
                            }
                            break;

                        case layer_type::sigmoid:
                            {
                                std::shared_ptr<SigmoidNode> node = std::dynamic_pointer_cast<SigmoidNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                MPSNNImageNode *input_image = [[mps_layer_dictionary objectForKey:input_key] resultImage];
                                SigmoidLayer* sigmoid_layer = [[SigmoidLayer alloc] initWithParams:key
                                                                                         inputNode:input_image];
                                [mps_layer_dictionary setObject:sigmoid_layer forKey:key];
                            }
                            break;

                        case layer_type::upsampling:
                            {
                                std::shared_ptr<UpsamplingNode> node = std::dynamic_pointer_cast<UpsamplingNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                MPSNNImageNode *input_image = [[mps_layer_dictionary objectForKey:input_key] resultImage];
                                
                                NearestUpsamplingLayer* upsample_layer = [[NearestUpsamplingLayer alloc] initWithParams:key
                                                                                                                 scaleX:node->m_scale_x
                                                                                                                 scaleY:node->m_scale_y
                                                                                                              inputNode:input_image];
                                [mps_layer_dictionary setObject:upsample_layer forKey:key];
                            }
                            break;
                        default:
                            {
                                std::cout << "TMPS - Layer Type not supported. name: " << l->m_name << std::endl;
                            }
                            break;
                    }
                }
            } else {
                std::cout << "TCMPS not supported on this machine." << std::endl;
            }

            // TODO: populate graph
            return std::make_shared<Graph>();
        }

        int test() {
          return 3;
        }
    }
}