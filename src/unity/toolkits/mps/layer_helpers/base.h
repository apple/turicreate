#ifndef turi_mps_layer_helpers_base_h
#define turi_mps_layer_helpers_base_h

#include <unity/toolkits/mps/layer_helpers/types.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
	namespace mps {
		struct EXPORT Layer: public model_base {
			public:
				std::string m_name;
				layer_type m_type;
				Layer(){};

			protected:
				Layer(std::string name, int type):m_name(name),
					m_type(static_cast<layer_type>(type)) {};

			public:
				BEGIN_CLASS_MEMBER_REGISTRATION("_Layer")
				END_CLASS_MEMBER_REGISTRATION
		};
	}
}

#endif