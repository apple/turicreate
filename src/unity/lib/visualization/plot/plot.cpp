#include "plot.hpp"
#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/thread.hpp>
#include <unity/lib/visualization/vega_data.hpp>

namespace turi{
  namespace visualization{
    void Plot::show() {
      using namespace turi;
      using namespace turi::visualization;

      ::turi::visualization::run_thread([this]() {
        process_wrapper ew(path_to_client);
        ew << vega_spec;

        while (ew.good()) {
          vega_data vd;
          auto result = transformer->get();

          vd << result->vega_column_data();

          double num_rows_processed =  static_cast<double>(transformer->get_rows_processed());
          double percent_complete = num_rows_processed/size_array;

          ew << vd.get_data_spec(percent_complete);

          if (transformer->eof()) {
            break;
          }
        }
      });
    }

    void Plot::materialize() {

    }

    std::string Plot::get_spec() {
      return "hello";
    }
  }
}

using namespace turi;

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(turi::visualization::Plot)
END_CLASS_REGISTRATION
