#include <unity/toolkits/mps/graph.h>

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

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
#include <unity/toolkits/mps/layer_helpers/types.h>

#include <unity/toolkits/mps/layers/input_layer.h>
#include <unity/toolkits/mps/layers/addition_layer.h>
#include <unity/toolkits/mps/layers/convolution_layer.h>
#include <unity/toolkits/mps/layers/instance_norm_layer.h>
#include <unity/toolkits/mps/layers/average_pooling_layer.h>
#include <unity/toolkits/mps/layers/relu_layer.h>
#include <unity/toolkits/mps/layers/sigmoid_layer.h>
#include <unity/toolkits/mps/layers/nearest_upsampling_layer.h>

namespace turi{
    namespace mps {

        Graph::Graph() {
            //base = std::make_shared<GraphBase>();
        }

        void Graph::add_node(std::shared_ptr<Layer> layer) {
            //m_layers.push_back(layer);
        }

        /*
        void Graph::create_from_metadata(std::vector<std::string> names,
                                         std::vector<std::map<std::string, std::float>> parameters,
                                         std::vector<int> type,
                                         std::vector<std::map<std::string, std::vector<float>>> data) {

        }
        */

        void Graph::compile(std::vector<std::shared_ptr<Layer>> &m_layers) {
            if (@available(macOS 10.13.4, *)) {

                id<MTLDevice> m_dev;
                NSMutableDictionary *m_mps_layer_dictionary;
                MPSNNGraph *m_graph API_AVAILABLE(macos(10.13));
                //BOOL *m_results_needed = YES;

                m_mps_layer_dictionary = [[NSMutableDictionary alloc] init];
                m_dev = MTLCreateSystemDefaultDevice();

                for(auto const& l: m_layers) {
                    switch (l->m_type) {
                        case layer_type::input:
                            {
                                std::shared_ptr<InputNode> node = std::dynamic_pointer_cast<InputNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                [m_mps_layer_dictionary setObject:[[InputLayer alloc] initInput]  forKey:key];
                            }
                            break;

                        case layer_type::output:
                            {  
                                std::shared_ptr<OutputNode> node = std::dynamic_pointer_cast<OutputNode>(l);
                                NSMutableArray *output_layer_images = [[NSMutableArray alloc]init];
                                for(auto const& o: node->m_layers){
                                    NSString *layer_key = [NSString stringWithUTF8String:o.m_name.c_str()];
                                    MPSNNImageNode *layer_image = [[m_mps_layer_dictionary objectForKey:layer_key] resultImage];
                                    [output_layer_images addObject:layer_image];
                                }

                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                [m_mps_layer_dictionary setObject:output_layer_images  forKey:key];

                                if([output_layer_images count] == 1) {
                                    m_graph = [[MPSNNGraph alloc] initWithDevice:m_dev
                                                                     resultImage:output_layer_images[0]
                                                             resultImageIsNeeded:YES];
                                }else{
                                    if (@available(macOS 10.15, *)) {
                                        /*
                                        m_graph = [[MPSNNGraph alloc] initWithDevice:m_dev
                                                                        resultImages:output_layer_images
                                                                    resultsAreNeeded:m_results_needed];
                                        */                            
                                    } else {
                                        std::cout << "Can't initate the graph with multiple outputs. MacOS 10.15 needed." << std::endl;
                                    }
                                }
                            }
                            break;

                        case layer_type::addition:
                            {
                                std::shared_ptr<AdditionNode> node = std::dynamic_pointer_cast<AdditionNode>(l);
                                NSString *left_key = [NSString stringWithUTF8String:((node->m_left)->m_name).c_str()];
                                NSString *right_key = [NSString stringWithUTF8String:((node->m_right)->m_name).c_str()];
                                MPSNNImageNode *left_image = [[m_mps_layer_dictionary objectForKey:left_key] resultImage];
                                MPSNNImageNode *right_image = [[m_mps_layer_dictionary objectForKey:right_key] resultImage];
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                AdditionLayer* addition_layer = [[AdditionLayer alloc] initWithParams:key
                                                                                             leftNode:left_image
                                                                                            rightNode:right_image];
                                [m_mps_layer_dictionary setObject:addition_layer forKey:key];
                            }
                            break;

                        case layer_type::convolution:
                            {
                                std::shared_ptr<ConvolutionNode> node = std::dynamic_pointer_cast<ConvolutionNode>(l);
                                /* TODO: Take care of the bias being null */
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                //NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                //MPSNNImageNode *input_image = [[m_mps_layer_dictionary objectForKey:input_key] resultImage];

                                // TODO: HACK, WAS GETTING SEGFAULTS BECAUE OF THIS

                                std::vector<float> biases_vector(node->m_output_feature_channels, 0.0);
                                float* weights = (float *)malloc(node->m_weights.size()*sizeof(float));

                                for(int i = 0; i < node->m_weights.size(); i++){
                                    weights[i] = node->m_weights[i];
                                }

                                float* biases = (float *)malloc(biases_vector.size()*sizeof(float));

                                for(int i = 0; i < node->m_weights.size(); i++){
                                    biases[i] = biases_vector[i];
                                }

                                
                                ConvolutionalLayer *convolution_layer = [[ConvolutionalLayer alloc] initWithParameters:key
                                                                                                           kernelWidth:node->m_kernel_width
                                                                                                          kernelHeight:node->m_kernel_height
                                                                                                  inputFeatureChannels:node->m_input_feature_channels
                                                                                                 outputFeatureChannels:node->m_output_feature_channels
                                                                                                           strideWidth:node->m_stride_width
                                                                                                          strideHeight:node->m_stride_height
                                                                                                          paddingWidth:node->m_padding_width
                                                                                                         paddingHeight:node->m_padding_height
                                                                                                               weights:weights
                                                                                                                biases:biases
                                                                                                             inputNode:[MPSNNImageNode nodeWithHandle: nil]
                                                                                                                device:m_dev];
                                [m_mps_layer_dictionary setObject:convolution_layer forKey:key];
                            }
                            break;

                        case layer_type::instance_norm:
                            {
                                std::shared_ptr<InstanceNormNode> node = std::dynamic_pointer_cast<InstanceNormNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                //NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                //MPSNNImageNode *input_image = [[m_mps_layer_dictionary objectForKey:input_key] resultImage];
                                
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
                                                                                                             inputNode:[MPSNNImageNode nodeWithHandle: nil]
                                                                                                                device:m_dev];
                                [m_mps_layer_dictionary setObject:instance_norm_layer forKey:key];
                            }
                            break;

                        case layer_type::pooling:
                            {
                                std::shared_ptr<PoolingNode> node = std::dynamic_pointer_cast<PoolingNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                //NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                //MPSNNImageNode *input_image = [[m_mps_layer_dictionary objectForKey:input_key] resultImage];
                                AveragePoolingLayer* pooling_layer = [[AveragePoolingLayer alloc] initWithParams:key
                                                                                                       inputNode:[MPSNNImageNode nodeWithHandle: nil]
                                                                                                     kernelWidth:node->m_kernel_width 
                                                                                                    kernelHeight:node->m_kernel_height 
                                                                                                     strideWidth:node->m_stride_in_pixels_x 
                                                                                                    strideHeight:node->m_stride_in_pixels_y];

                                [m_mps_layer_dictionary setObject:pooling_layer forKey:key];
                            }
                            break;

                        case layer_type::relu:
                            {
                                std::shared_ptr<ReluNode> node = std::dynamic_pointer_cast<ReluNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                //NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                //MPSNNImageNode *input_image = [[m_mps_layer_dictionary objectForKey:input_key] resultImage];
                                ReluLayer* relu_layer = [[ReluLayer alloc] initWithParams:key
                                                                                inputNode:[MPSNNImageNode nodeWithHandle: nil]];
                                [m_mps_layer_dictionary setObject:relu_layer forKey:key];
                            }
                            break;

                        case layer_type::sigmoid:
                            {
                                std::shared_ptr<SigmoidNode> node = std::dynamic_pointer_cast<SigmoidNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                //NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                //MPSNNImageNode *input_image = [[m_mps_layer_dictionary objectForKey:input_key] resultImage];
                                SigmoidLayer* sigmoid_layer = [[SigmoidLayer alloc] initWithParams:key
                                                                                         inputNode:[MPSNNImageNode nodeWithHandle: nil]];
                                [m_mps_layer_dictionary setObject:sigmoid_layer forKey:key];
                            }
                            break;

                        case layer_type::upsampling:
                            {
                                std::shared_ptr<UpsamplingNode> node = std::dynamic_pointer_cast<UpsamplingNode>(l);
                                NSString *key = [NSString stringWithUTF8String:node->m_name.c_str()];
                                //NSString *input_key = [NSString stringWithUTF8String:((node->m_input)->m_name).c_str()];
                                //MPSNNImageNode *input_image = [[m_mps_layer_dictionary objectForKey:input_key] resultImage];
                                
                                NearestUpsamplingLayer* upsample_layer = [[NearestUpsamplingLayer alloc] initWithParams:key
                                                                                                                 scaleX:node->m_scale_x
                                                                                                                 scaleY:node->m_scale_y
                                                                                                              inputNode:[MPSNNImageNode nodeWithHandle: nil]];
                                [m_mps_layer_dictionary setObject:upsample_layer forKey:key];
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
        }

        void Graph::clear(){
            //m_layers.clear();
            
            /*
            if (@available(macOS 10.13.4, *)) {
                m_graph = NULL;
            }
            */
        }

        void Graph::update_weights(){
            printf("TODO: update graph weights");
        }

        void Graph::set_trainable_layers(std::map<std::string, bool> trainable){
            printf("TODO: set trainable layers");
        }

        void Graph::forward(){
            printf("TODO: forward pass");
        }

        void Graph::backward(){
            printf("TODO: backward pass");
        }
    }
}