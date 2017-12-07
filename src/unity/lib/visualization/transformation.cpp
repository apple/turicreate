/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "transformation.hpp"

using namespace turi;
using namespace turi::visualization;

fused_transformation_output::fused_transformation_output(const std::vector<std::shared_ptr<transformation_output>> outputs)
  : m_outputs(outputs) {
}

std::string fused_transformation_output::vega_column_data(bool sframe) const {
  std::stringstream ret;

  for (const auto& output : m_outputs) {
    ret << output->vega_column_data(sframe);
  }

  return ret.str();
}

fused_transformation::fused_transformation(const std::vector<std::shared_ptr<transformation_base>> transformers)
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

std::shared_ptr<transformation_output> fused_transformation::get() {
  std::vector<std::shared_ptr<transformation_output>> ret;
  for (const auto& transformer : m_transformers) {
    std::shared_ptr<transformation_output> output = transformer->get();
    ret.push_back(output);
  }
  return std::make_shared<fused_transformation_output>(ret);
}

bool fused_transformation::eof() const {
  // all have the same batch size and same number of rows processed.
  // we also guaranteed in the constructor that there is at least 1 transformer.
  // we need only ask the 1 we know of whether it's eof.
  return m_transformers[0]->eof();
}

flex_int fused_transformation::get_rows_processed() const {
  // all have the same batch size and same number of rows processed.
  // we also guaranteed in the constructor that there is at least 1 transformer.
  // we need only ask the 1 we know of for rows_processed.
  return m_transformers[0]->get_rows_processed();
}

size_t fused_transformation::get_batch_size() const {
  // all have the same batch size and same number of rows processed.
  // we also guaranteed in the constructor that there is at least 1 transformer.
  // we need only ask the 1 we know of for rows_processed.
  return m_transformers[0]->get_batch_size();
}

std::shared_ptr<fused_transformation> transformation_collection::fuse() {
  return std::make_shared<fused_transformation>(*this);
}
