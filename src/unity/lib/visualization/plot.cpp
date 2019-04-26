#include <iostream>
#include <memory>

#include "plot.hpp"
#include <unity/lib/visualization/dark_mode.hpp>
#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/thread.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <unity/lib/visualization/vega_data.hpp>
#include <unity/lib/visualization/vega_spec.hpp>
#include <unity/lib/visualization/histogram.hpp>
#include <unity/lib/visualization/item_frequency.hpp>
#include <unity/lib/visualization/summary_view.hpp>
#include <unity/lib/visualization/vega_spec/config.h>
#include <sstream>

namespace turi{
  namespace visualization{
    void Plot::show(const std::string& path_to_client, tc_plot_variation variation) {

      std::shared_ptr<Plot> self = std::make_shared<Plot>(*this);

      ::turi::visualization::run_thread([self, path_to_client, variation]() {
        process_wrapper ew(path_to_client);

        // Include the first batch of data in the initial spec.
        // Batch size is dependent on specific plot type & data.
        ew << "{\"vega_spec\": " << self->get_spec(variation, true /* include_data */) << "}\n";

        // If the plot is done rendering, we can skip sending data_spec after vega_spec.
        while(!self->m_transformer->eof() && ew.good()) {
          vega_data vd;

          vd << self->m_transformer->get()->vega_column_data();

          double num_rows_processed =  static_cast<double>(self->m_transformer->get_rows_processed());
          double percent_complete = num_rows_processed/self->m_size_array;

          ew << "{\"data_spec\": " << vd.get_data_spec(percent_complete) << "}\n";
        }
      });
    }

    void Plot::materialize() {
      do {
        m_transformer->get()->vega_column_data();
      } while(!m_transformer->eof());
      DASSERT_EQ(get_percent_complete(), 1.0);
    }

    bool Plot::finished_streaming() const {
      return m_transformer->eof();
    }

    double Plot::get_percent_complete() const {
      return m_transformer->get_percent_complete();
    }

    std::string Plot::get_next_data() {
      vega_data vd;
      vd << m_transformer->get()->vega_column_data();
      return vd.get_data_spec(get_percent_complete());
    }

    std::string Plot::get_data() {
      this->materialize();
      DASSERT_TRUE(m_transformer->eof());
      vega_data vd;
      vd << m_transformer->get()->vega_column_data();
      return vd.get_data_spec(100 /* percent_complete */);
    }

    std::string Plot::get_spec(tc_plot_variation variation,
                               bool include_data) {
      // Replace config from predefined config (maintained separately so we don't
      // have to repeat the same config in each file, and we can make sure it stays
      // consistent across the different plots)
      std::string ret = m_vega_spec;
      static std::string config_str = make_format_string(vega_spec_config_json, vega_spec_config_json_len);
      ret = format(ret, {{"{{config}}", config_str}});

      // Apply templated configuration values based on variation

      // Defaults
      std::string gridColor = escape_string("rgba(204,204,204,1.0)");
      std::string axisTitlePadding = "20";
      std::string axisTitleFontSize = "14";
      std::string axisTitleFontWeight = escape_string("normal");
      std::string labelColor = escape_string("rgba(0,0,0,0.847)");
      std::string labelFont = escape_string("\"San Francisco\", HelveticaNeue, Arial");
      std::string labelFontSize = "12";
      std::string labelPadding = "10";
      std::string titleColor = labelColor;
      std::string titleFont = labelFont;
      std::string titleFontWeight = escape_string("normal");
      std::string titleFontSize = "18";
      std::string titleOffset = "30";
      std::string tickColor = escape_string("rgb(136,136,136)");
      std::string data = "";

      // Default (medium) size is 720x550
      std::string width = "720";
      std::string height = "550";

      // Overrides for dark mode
      tc_plot_variation color_variation = (tc_plot_variation)((uint32_t)variation & 0xf0);
      if (color_variation == tc_plot_color_dark ||
          (color_variation == tc_plot_variation_default && is_system_dark_mode())) {
        labelColor = escape_string("rgba(255,255,255,0.847)");
        gridColor = escape_string("rgba(255,255,255,0.098)");
        titleColor = labelColor;
        tickColor = escape_string("#A4AAAD");
      }

      // Overrides for small size
      tc_plot_variation size_variation = (tc_plot_variation)((uint32_t)variation & 0x0f);
      if (size_variation == tc_plot_size_small) {
        // Small size is 320x280
        width = "320";
        height = "280";
        axisTitleFontSize = "11";
        axisTitlePadding = "8";
        labelFontSize = "9";
        labelPadding = "4";
        titleFontSize = "13";
        titleOffset = "16";
      } else if (size_variation == tc_plot_size_large) {
        // Large size is 960x840
        width = "960";
        height = "840";
        axisTitleFontSize = "22";
        axisTitleFontWeight = escape_string("bold");
        axisTitlePadding = "18";
        labelFontSize = "18";
        labelPadding = "18";
        titleFontSize = "26";
        titleFontWeight = escape_string("bold");
        titleOffset = "30";
      }

      // Override for data inclusion
      if (include_data) {
        data = ", \"values\": [" + m_transformer->get()->vega_column_data() + "]";
      }

      return format(ret, {
        {"{{gridColor}}", gridColor},
        {"{{axisTitlePadding}}", axisTitlePadding},
        {"{{axisTitleFontSize}}", axisTitleFontSize},
        {"{{axisTitleFontWeight}}", axisTitleFontWeight},
        {"{{labelColor}}", labelColor},
        {"{{labelFont}}", labelFont},
        {"{{labelFontSize}}", labelFontSize},
        {"{{labelPadding}}", labelPadding},
        {"{{titleColor}}", titleColor},
        {"{{titleFont}}", titleFont},
        {"{{titleFontSize}}", titleFontSize},
        {"{{titleFontWeight}}", titleFontWeight},
        {"{{titleOffset}}", titleOffset},
        {"{{tickColor}}", tickColor},
        {"{{width}}", width},
        {"{{height}}", height},
        {"{{pre_filled_data_values}}", data},
      });
    }
  }

  BEGIN_CLASS_REGISTRATION
  REGISTER_CLASS(visualization::Plot)
  END_CLASS_REGISTRATION
}
