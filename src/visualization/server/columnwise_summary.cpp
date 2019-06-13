#include "batch_size.hpp"
#include "columnwise_summary.hpp"
#include <unity/lib/unity_sframe.hpp>

namespace turi {
  namespace visualization {
    std::shared_ptr<Plot> plot_columnwise_summary(std::shared_ptr<unity_sframe_base> sf) {

      logprogress_stream << "Materializing SFrame" << std::endl;
      sf->materialize();

      if (sf->size() == 0) {
        log_and_throw("Nothing to show; SFrame is empty.");
      }

      transformation_collection column_transformers;

      std::vector<std::string> column_names;

      size_t i = 0;
      bool warned_on_unsupported_dtype = false;
      bool warned_on_too_many_columns = false;
      for (const std::string& col : sf->column_names()) {
        std::shared_ptr<unity_sarray_base> sarr = sf->select_column(col);

        if (i >= 50 && !warned_on_too_many_columns) {
          // we are past the limit for reasonable perf in the view.
          // warn and omit the column.
          warned_on_too_many_columns = true;
          logprogress_stream << "Warning: Skipping column '"
                             << col
                             << "' ["
                             << flex_type_enum_to_name(sarr->dtype())
                             << "]. Unable to show more than 50 columns."
                             << std::endl
                             << "Further warnings of more than 50 columns will be suppressed."
                             << std::endl;
          continue;
        }

        switch (sarr->dtype()) {
          case flex_type_enum::INTEGER:
          case flex_type_enum::FLOAT:
          case flex_type_enum::STRING:
          {
            i++;
            column_names.push_back(col);
            break;
          }
          default:
            if (!warned_on_unsupported_dtype) {
              warned_on_unsupported_dtype = true;
              logprogress_stream << "Warning: Skipping column '"
                                 << col
                                 << "'. Unable to show columns of type '"
                                 << flex_type_enum_to_name(sarr->dtype())
                                 << "'; only [int, float, str] can be shown."
                                 << std::endl
                                 << "Further warnings of unsupported type will be suppressed."
                                 << std::endl;
            }
            break;
        }
      }

      // now that we've collected columns, pick a batch size and add transformers
      gl_sframe gl_sf(sf->select_columns(column_names));
      size_t sf_batch_size = batch_size(gl_sf);
      for (const std::string& col : column_names) {
        std::shared_ptr<unity_sarray_base> sarr = sf->select_column(col);
        switch (sarr->dtype()) {
          case flex_type_enum::INTEGER:
          {
            std::shared_ptr<histogram<flex_int>> hist = std::make_shared<histogram<flex_int>>();
            hist->init(sarr, sf_batch_size);
            column_transformers.push_back(hist);
            break;
          }
          case flex_type_enum::FLOAT:
          {
            std::shared_ptr<histogram<flex_float>> hist = std::make_shared<histogram<flex_float>>();
            hist->init(sarr, sf_batch_size);
            column_transformers.push_back(hist);
            break;
          }
          case flex_type_enum::STRING:
          {
            std::shared_ptr<item_frequency> item_freq = std::make_shared<item_frequency>();
            item_freq->init(sarr, sf_batch_size);
            column_transformers.push_back(item_freq);
            break;
          }
          default:
            break;
        }
      }

      DASSERT_EQ(column_transformers.size(), column_names.size());
      if(column_transformers.size() == 0){
        log_and_throw("Nothing to show, because there are no columns of type [int, float, str]");
      }

      std::vector<flex_type_enum> column_types;

      for(size_t i = 0; i < column_names.size(); i++){
        std::shared_ptr<unity_sarray_base> sarr = sf->select_column(column_names[i]);
        column_types.push_back(sarr->dtype());
      }

      std::shared_ptr<summary_view_transformation> summary_view_transformers = std::make_shared<summary_view_transformation>(column_transformers, column_names, column_types, sf->size());
      std::string summary_view_vega_spec  = summary_view_spec(column_transformers.size());

      std::shared_ptr<transformation_base> shared_unity_transformer = std::static_pointer_cast<transformation_base>(summary_view_transformers);

      return std::make_shared<Plot>(summary_view_vega_spec, shared_unity_transformer, (sf->size() * column_transformers.size()));
    }

  }
}
