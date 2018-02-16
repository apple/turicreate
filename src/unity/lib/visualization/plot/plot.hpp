#ifndef __TC_PLOT_HPP_
#define __TC_PLOT_HPP_

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <string>
#include <export.hpp>

namespace turi {
  namespace visualization {

    class EXPORT Plot: public toolkit_class_base{
      public:

        Plot(){};

        Plot(const std::string& path_to_client, const std::string& vega_spec, std::shared_ptr<transformation_base> transformer, double size_array):
                                              vega_spec(vega_spec),
                                              path_to_client(path_to_client),
                                              size_array(size_array),
                                              transformer(transformer){}

        void show();
        void materialize();
        std::string get_spec();

        BEGIN_CLASS_MEMBER_REGISTRATION("_Plot")
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::show)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::materialize)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::get_spec)
        END_CLASS_MEMBER_REGISTRATION

      private:
        std::string vega_spec;
        std::string path_to_client;
        double size_array;
        std::shared_ptr<transformation_base> transformer;
    };
  }
}

#endif
