#ifndef __TURI_MPS_GRAPH
#define __TURI_MPS_GRAPH

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

/*
#include <unity/toolkits/util/mps_layers/addition_layer.h>
#include <unity/toolkits/util/mps_layers/convolution_layer.h>
#include <unity/toolkits/util/mps_layers/instance_norm_layer.h>
#include <unity/toolkits/util/mps_layers/pooling_layer.h>
#include <unity/toolkits/util/mps_layers/relu_layer.h>
#include <unity/toolkits/util/mps_layers/sigmoid_layer.h>
#include <unity/toolkits/util/mps_layers/upsampling_layer.h>
*/

namespace turi {
	namespace mps {
		class EXPORT Graph : public model_base {
			public:
				Graph(){};

				void testing();

				BEGIN_CLASS_MEMBER_REGISTRATION("_Graph")
				REGISTER_CLASS_MEMBER_FUNCTION(mps::Graph::testing)
				END_CLASS_MEMBER_REGISTRATION
		};
	}
}

#endif