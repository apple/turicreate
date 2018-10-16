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
            if (@available(macOS 10.13, *)) {
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
                                    MPSNNImageNode *layer_images = [[mps_layer_dictionary objectForKey:layer_key] resultImage];
                                    [output_layer_images addObject:layer_images];
                                }
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                [mps_layer_dictionary setObject:output_layer_images  forKey:key];
                            }
                            break;

                        case layer_type::addition:
                            {
                                std::shared_ptr<AdditionNode> node = std::dynamic_pointer_cast<AdditionNode>(l);
                            }
                            break;

                        case layer_type::convolution:
                            {
                                std::shared_ptr<ConvolutionNode> node = std::dynamic_pointer_cast<ConvolutionNode>(l); 
                            }
                            break;

                        case layer_type::instance_norm:
                            {
                                std::shared_ptr<InstanceNormNode> node = std::dynamic_pointer_cast<InstanceNormNode>(l);
                            }
                            break;

                        case layer_type::pooling:
                            {
                                std::shared_ptr<PoolingNode> node = std::dynamic_pointer_cast<PoolingNode>(l);
                            }
                            break;

                        case layer_type::relu:
                            {
                                std::shared_ptr<ReluNode> node = std::dynamic_pointer_cast<ReluNode>(l);
                            }
                            break;

                        case layer_type::sigmoid:
                            {
                                std::shared_ptr<SigmoidNode> node = std::dynamic_pointer_cast<SigmoidNode>(l);
                            }
                            break;

                        case layer_type::upsampling:
                            {
                                std::shared_ptr<UpsamplingNode> node = std::dynamic_pointer_cast<UpsamplingNode>(l);
                            }
                            break;
                        default:
                            {
                                std::cout << "TMPS - Layer Type not supported. name: " << l->m_name;
                            }
                            break;
                    }
                }
            } else {
                // Print Warning, MPSGraph isn't supported
            }

            // TODO: populate graph
            return std::make_shared<Graph>();
        }

        int test() {
          return 3;
        }
    }
}