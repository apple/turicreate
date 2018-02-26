#ifndef __TC_PLOT_HPP_
#define __TC_PLOT_HPP_

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <string>

namespace turi {
  namespace visualization {

    class Plot: public toolkit_class_base {

      public:
        Plot(){};
        Plot(const std::string path_to_client, const std::string vega_spec, std::shared_ptr<transformation_base> transformer, double size_array):
                                              m_vega_spec(vega_spec),
                                              m_path_to_client(path_to_client),
                                              m_size_array(size_array),
                                              m_transformer(transformer){}
        void show();
        void materialize();
        std::string get_spec();
        std::string get_data();

        // TODO - these hould be private
        std::string m_vega_spec;
        std::string m_path_to_client;
        double m_size_array;
        std::shared_ptr<transformation_base> m_transformer;

        BEGIN_CLASS_MEMBER_REGISTRATION("_Plot")
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::show)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::materialize)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::get_spec)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::get_data)
        END_CLASS_MEMBER_REGISTRATION
    };
  }
}

#endif
