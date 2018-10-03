#ifndef __TC_PLOT_HPP_
#define __TC_PLOT_HPP_

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <unity/lib/extensions/model_base.hpp>
#include <string>

namespace turi {
  namespace visualization {

    class Plot: public model_base {

      public:
        Plot(){};
        Plot(const std::string path_to_client, const std::string vega_spec, std::shared_ptr<transformation_base> transformer, double size_array):
                                              m_vega_spec(vega_spec),
                                              m_path_to_client(path_to_client),
                                              m_size_array(size_array),
                                              m_transformer(transformer){}
        void show();
        void materialize();

        // vega specification
        std::string get_spec();

        // streaming data aggregation
        double get_percent_complete(); // out of 1.0
        bool finished_streaming();
        std::string get_next_data();

        // non-streaming data aggregation: causes full materialization
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
