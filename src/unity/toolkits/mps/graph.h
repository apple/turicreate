#ifndef turi_mps_graph_h
#define turi_mps_graph_h

#include <string>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

namespace turi {
    namespace mps {
        class EXPORT Graph : public model_base {
            public:
                Graph(){};

                void add_node();
                void compile();
                void clear();

                void forward();
                void backward();

                std::string debugDescription();

                BEGIN_CLASS_MEMBER_REGISTRATION("_Graph")
                REGISTER_CLASS_MEMBER_FUNCTION(mps::Graph::add_node)
                REGISTER_CLASS_MEMBER_FUNCTION(mps::Graph::compile)
                REGISTER_CLASS_MEMBER_FUNCTION(mps::Graph::clear)
                REGISTER_CLASS_MEMBER_FUNCTION(mps::Graph::forward)
                REGISTER_CLASS_MEMBER_FUNCTION(mps::Graph::backward)
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif