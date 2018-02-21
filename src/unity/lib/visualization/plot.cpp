#include <iostream>
#include <memory>

#include "plot.hpp"
#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/thread.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <unity/lib/visualization/vega_data.hpp>
#include <unity/lib/visualization/histogram.hpp>
#include <unity/lib/visualization/item_frequency.hpp>
#include <unity/lib/visualization/summary_view.hpp>
#include <sstream>

namespace turi{
  namespace visualization{
    void Plot::show() {
      using namespace turi;
      using namespace turi::visualization;

      std::shared_ptr<Plot> self = std::make_shared<Plot>(*this);

      ::turi::visualization::run_thread([self]() {
        process_wrapper ew(self->path_to_client);
        ew << self->vega_spec;

        while(ew.good()) {
          vega_data vd;

          vd << self->transformer.get()->get()->vega_column_data();

          double num_rows_processed =  static_cast<double>(self->transformer->get_rows_processed());
          double percent_complete = num_rows_processed/self->size_array;

          ew << vd.get_data_spec(percent_complete);

          if (self->transformer->eof()) {
             break;
          }
        }
      });
    }

    void Plot::materialize() {
      do {
        transformer.get()->get()->vega_column_data();
      } while(!transformer->eof());
    }

    std::string Plot::get_data() {
      this->materialize();
      vega_data vd;

      while(true) {
        vd << transformer.get()->get()->vega_column_data();
        if (transformer->eof()) {
          break;
        }
      }

      std::stringstream ss;
      ss << vd.get_data_spec(100);
      return ss.str();
    }

    std::string Plot::get_spec() {
      return vega_spec;
    }
  }
}
