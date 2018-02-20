#include "summary_view.hpp"

#include <unity/lib/visualization/vega_data.hpp>
#include <unity/lib/visualization/vega_spec.hpp>
#include <math.h>

using namespace turi;
using namespace turi::visualization;

summary_view_transformation_output::summary_view_transformation_output(const std::vector<std::shared_ptr<transformation_output>> outputs)
  : m_outputs(outputs) {
}

std::string summary_view_transformation_output::vega_column_data(bool sframe) const {
  std::stringstream ss;
  ss << "{\"a\": " << std::to_string(index) << ",";
  std::string title = extra_label_escape(column_names[index]);
  ss << "\"title\": " << title << ",";
  ss << "\"num_row\": " << size << ",";

  switch (column_types[index]) {
    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT:
    case flex_type_enum::STRING:
    {
      auto h_result = std::dynamic_pointer_cast<sframe_transformation_output>(m_outputs[0]);
      ss << h_result->vega_summary_data();
      ss << "}";
      break;
    }
    default:
      throw std::runtime_error("Unexpected dtype. SFrame plot expects int, float or str dtypes.");
  }
  return ss.str();
}

summary_view_transformation::summary_view_transformation(const std::vector<std::shared_ptr<transformation_base>> transformers)
  : m_transformers(transformers) {
    // check some assumptions that make fuse work
    //
    // 1. Must have 1 or more transformers
    if (transformers.size() < 1) {
      throw std::runtime_error("Expected 1 or more transformers when fusing transformers.");
    }
    // 2. Transformers must all have the same batch size
    for (size_t i=1; i<transformers.size(); i++) {
      if (transformers[i]->get_batch_size() != transformers[0]->get_batch_size()) {
        throw std::runtime_error("All transformers being fused must have the same batch size.");
      }
    }
}

std::shared_ptr<transformation_output> summary_view_transformation::get() {
  std::vector<std::shared_ptr<transformation_output>> ret;

  std::shared_ptr<transformation_output> output =  m_transformers[index] -> get();
  ret.push_back(output);

  auto fused_out = std::make_shared<summary_view_transformation_output>(ret);

  fused_out->column_names = column_names;
  fused_out->column_types = column_types;
  fused_out->size = size;
  fused_out->index = index;

  index = index + 1;

  if(index >= column_names.size()){
    index = 0;
  }

  return fused_out;
}

bool summary_view_transformation::eof() const {
  // all have the same batch size and same number of rows processed.
  // we also guaranteed in the constructor that there is at least 1 transformer.
  // we need only ask the 1 we know of whether it's eof.
  return m_transformers[(m_transformers.size()-1)]->eof();
}

flex_int summary_view_transformation::get_rows_processed() const {
  // all have the same batch size and same number of rows processed.
  // we also guaranteed in the constructor that there is at least 1 transformer.
  // we need only ask the 1 we know of for rows_processed.
  size_t all_rows_processed = 0;

  for (const auto& transformer : m_transformers) {
    all_rows_processed += transformer->get_rows_processed();
  }

  return flex_int(all_rows_processed);
}

size_t summary_view_transformation::get_batch_size() const {
  // all have the same batch size and same number of rows processed.
  // we also guaranteed in the constructor that there is at least 1 transformer.
  // we need only ask the 1 we know of for rows_processed.
  return m_transformers[0]->get_batch_size();
}
