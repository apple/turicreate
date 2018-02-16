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

        Plot(const std::string& vega_spec,std::shared_ptr<transformation_base> transformer):
                                              vega_spec(vega_spec), transformer(transformer){}

        void show();
        void materialize();
        std::string get_spec();

        BEGIN_CLASS_MEMBER_REGISTRATION("Plot")
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::show)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::materialize)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::get_spec)
        END_CLASS_MEMBER_REGISTRATION

      private:
        std::string vega_spec;
        std::shared_ptr<transformation_base> transformer;
    };
  }
}

#endif
