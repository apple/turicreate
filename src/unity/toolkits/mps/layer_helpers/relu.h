#ifndef turi_mps_layer_helpers_relu_h
#define turi_mps_layer_helpers_relu_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
	namespace mps {
		struct EXPORT ReluNode: public Layer {
			public:
				ReluNode(){};
				ReluNode(std::string name, std::shared_ptr<Layer> input):
					Layer(name, layer_type::relu),
					m_input(input) {};

				std::shared_ptr<Layer> m_input;

				BEGIN_CLASS_MEMBER_REGISTRATION("_ReluNode")
				END_CLASS_MEMBER_REGISTRATION
		};
	}
}

#endif