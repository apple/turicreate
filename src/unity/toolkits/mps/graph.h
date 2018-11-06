#ifndef turi_mps_graph_h
#define turi_mps_graph_h

#include <map>
#include <string>
#include <vector>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <unity/toolkits/mps/layer_helpers/base.h>

namespace turi {
    namespace mps {
        class EXPORT Graph : public model_base {
            public:
                Graph();

                void add_node(std::shared_ptr<Layer> layer);
                /*
                void create_from_metadata(std::vector<std::string> names,
                                          std::vector<std::map<std::string, std::float>> parameters,
                                          std::vector<int> type,
                                          std::vector<std::map<std::string, std::vector<float>>> data);
                */

                void compile(std::vector<std::shared_ptr<Layer>> &m_layers);
                void clear();

                void forward();
                void backward();
                void update_weights();

                void set_trainable_layers(std::map<std::string, bool> trainable);

                std::string debugDescription();

                //std::vector<std::shared_ptr<Layer>> m_layers;

                BEGIN_CLASS_MEMBER_REGISTRATION("_Graph")
                REGISTER_CLASS_MEMBER_FUNCTION(Graph::add_node, "layer")
                //REGISTER_CLASS_MEMBER_FUNCTION(Graph::create_from_metadata, "names", "parameters", "type", "data")
                REGISTER_CLASS_MEMBER_FUNCTION(Graph::compile, "m_layers")
                REGISTER_CLASS_MEMBER_FUNCTION(Graph::clear)
                REGISTER_CLASS_MEMBER_FUNCTION(Graph::forward)
                REGISTER_CLASS_MEMBER_FUNCTION(Graph::backward)
                REGISTER_CLASS_MEMBER_FUNCTION(Graph::update_weights)
                REGISTER_CLASS_MEMBER_FUNCTION(Graph::set_trainable_layers, "trainable")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif