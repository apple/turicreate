#ifndef __TC_PLOT_HPP_
#define __TC_PLOT_HPP_

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <unity/lib/api/plot_interface.hpp>
#include <string>

namespace turi {
  namespace visualization {

    class Plot: public plot_base{
      public:

        Plot(){};

        ~Plot(){};

        Plot(const std::string path_to_client, const std::string vega_spec, std::shared_ptr<transformation_base> transformer, double size_array):
                                              m_vega_spec(vega_spec),
                                              m_path_to_client(path_to_client),
                                              m_size_array(size_array),
                                              m_transformer(transformer){}
        void show();
        void materialize();
        std::string get_spec();
        std::string get_data();

        std::string m_vega_spec;
        std::string m_path_to_client;
        double m_size_array;
        std::shared_ptr<transformation_base> m_transformer;
    };
  }
}

#endif
