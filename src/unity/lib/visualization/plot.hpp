#ifndef __TC_PLOT_HPP_
#define __TC_PLOT_HPP_

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <unity/lib/extensions/model_base.hpp>
#include <string>
#include <capi/TuriCreate.h>

#ifdef __APPLE__
#ifndef TC_BUILD_IOS
extern "C" {
#include <CoreGraphics/CoreGraphics.h>
}
#endif // TC_BUILD_IOS
#endif // __APPLE__

namespace turi {
  namespace visualization {

    class Plot: public model_base {
      private:
        std::string m_id;
        std::string m_vega_spec;
        double m_size_array;
        std::shared_ptr<transformation_base> m_transformer;

      public:
        Plot();
        Plot(const std::string vega_spec, std::shared_ptr<transformation_base> transformer, double size_array):
                                              m_vega_spec(vega_spec),
                                              m_size_array(size_array),
                                              m_transformer(transformer){}
        void show(const std::string& path_to_client, tc_plot_variation variation = tc_plot_variation_default);
        void materialize() const;
        std::string get_url() const;
        const std::string& get_id() const;

        // vega specification
        std::string get_spec(tc_plot_variation variation=tc_plot_variation_default,
                             bool include_data=false) const;

        // streaming data aggregation
        double get_percent_complete() const; // out of 1.0
        bool finished_streaming() const;
        std::string get_next_data() const;

        // non-streaming data aggregation: causes full materialization
        std::string get_data() const;

#ifdef __APPLE__
#ifndef TC_BUILD_IOS
        // Streaming render (based on current computation -
        // call materialize first to get a final rendering up front).
        // Returns true if streaming finished, false otherwise.
        bool render(CGContextRef context, tc_plot_variation variation=tc_plot_variation_default);

        // Low-level rendering path - takes a Vega spec as input.
        static void render(const std::string& vega_spec, CGContextRef context);
#endif // TC_BUILD_IOS
#endif // __APPLE__

        BEGIN_CLASS_MEMBER_REGISTRATION("_Plot")
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::show, "path_to_client", "variation")
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::materialize)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::get_spec, "variation", "include_data")
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::get_data)
        REGISTER_CLASS_MEMBER_FUNCTION(Plot::get_url)
        END_CLASS_MEMBER_REGISTRATION
    };
  }
}

#endif
