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

      std::shared_ptr<Plot> self = std::make_shared<Plot>(*this);

      ::turi::visualization::run_thread([self]() {
        process_wrapper ew(self->m_path_to_client);
        ew << self->m_vega_spec;

        while(ew.good()) {
          vega_data vd;

          vd << self->m_transformer->get()->vega_column_data();

          double num_rows_processed =  static_cast<double>(self->m_transformer->get_rows_processed());
          double percent_complete = num_rows_processed/self->m_size_array;

          ew << vd.get_data_spec(percent_complete);

          if (self->m_transformer->eof()) {
             break;
          }
        }
      });
    }

    void Plot::materialize() {
      do {
        m_transformer->get()->vega_column_data();
      } while(!m_transformer->eof());
      DASSERT_EQ(get_percent_complete(), 1.0);
    }

    bool Plot::finished_streaming() {
      return m_transformer->eof();
    }

    double Plot::get_percent_complete() {
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

    std::string Plot::get_spec() {
      return m_vega_spec;
    }
  }

  BEGIN_CLASS_REGISTRATION
  REGISTER_CLASS(visualization::Plot)
  END_CLASS_REGISTRATION
}
