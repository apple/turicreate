#ifndef turi_mps_layer_helpers_instance_norm_h
#define turi_mps_layer_helpers_instance_norm_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
	namespace mps {
		struct EXPORT InstanceNormNode: public Layer {
			public:
				InstanceNormNode(){};
				InstanceNormNode(std::string name, 
								 std::shared_ptr<Layer> input,
								 size_t channels,
								 size_t styles,
								 std::vector<std::vector<float>> gamma,
								 std::vector<std::vector<float>> beta):
					Layer(name, layer_type::instance_norm),
					m_input(input),
					m_channels(channels),
					m_styles(styles),
					m_gamma(gamma),
					m_beta(beta) {};

				std::shared_ptr<Layer> m_input;
				int m_channels;
				int m_styles;
				std::vector<std::vector<float>> m_gamma;
				std::vector<std::vector<float>> m_beta;

				BEGIN_CLASS_MEMBER_REGISTRATION("_InstanceNormNode")
				END_CLASS_MEMBER_REGISTRATION
		};
	}
}

#endif