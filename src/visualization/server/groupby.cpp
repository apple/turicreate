/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "groupby.hpp"

namespace turi {
namespace visualization {

void summary_stats::add_element_simple(const flexible_type& value) {
  m_average.add_element_simple(value);
  m_count.add_element_simple(value);
  m_max.add_element_simple(value);
  m_min.add_element_simple(value);
  m_sum.add_element_simple(value);
  m_stdv.add_element_simple(value);
  m_variance.add_element_simple(value);
}

void summary_stats::combine(const summary_stats& other) {
  m_average.combine(other.m_average);
  m_count.combine(other.m_count);
  m_max.combine(other.m_max);
  m_min.combine(other.m_min);
  m_sum.combine(other.m_sum);
  m_stdv.combine(other.m_stdv);
  m_variance.combine(other.m_variance);
}

void summary_stats::partial_finalize() {
  m_average.partial_finalize();
  m_count.partial_finalize();
  m_max.partial_finalize();
  m_min.partial_finalize();
  m_sum.partial_finalize();
  m_stdv.partial_finalize();
  m_variance.partial_finalize();
}

flexible_type summary_stats::emit() const {
  return flex_dict({
    {"mean", m_average.emit()},
    {"count", m_count.emit()},
    {"max", m_max.emit()},
    {"min", m_min.emit()},
    {"sum", m_sum.emit()},
    {"std", m_stdv.emit()},
    {"var", m_variance.emit()}
  });
}

void summary_stats::set_input_type(flex_type_enum type) {
  m_average.set_input_type(type);
  // set_input_type is not supported for count. not sure why not...
  //m_count.set_input_type(type);
  m_max.set_input_type(type);
  m_min.set_input_type(type);
  m_sum.set_input_type(type);
  m_stdv.set_input_type(type);
  m_variance.set_input_type(type);
}

void groupby_quantile_result::insert_category(const flexible_type& category) {
  groupby_result<groupby_operators::quantile>::insert_category(category);
  auto& agg = m_aggregators.at(category);
  agg.init(std::vector<double>({0, 0.25, 0.50, 0.75, 1.0}));
}

}}
