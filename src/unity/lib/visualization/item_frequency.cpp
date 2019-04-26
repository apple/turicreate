/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/lib/visualization/batch_size.hpp>
#include <unity/lib/visualization/escape.hpp>
#include <unity/lib/visualization/item_frequency.hpp>
#include <unity/lib/visualization/vega_spec.hpp>

#include <string>

using namespace turi::visualization;

std::vector<item_frequency_result> item_frequency::split_input(size_t num_threads) {
  // TODO - do we need to do anything here? perhaps not.
  return std::vector<item_frequency_result>(num_threads);
}

void item_frequency::merge_results(std::vector<item_frequency_result>& transformers) {
  for (const auto& other : transformers) {
    this->m_transformer->combine(other);
  }
}

void item_frequency_result::add_element_simple(const flexible_type& flex) {
  groupby_operators::frequency_count::add_element_simple(flex);

  /*
   * add element to summary stats
   */
  m_count.add_element_simple(flex);
  m_count_distinct.add_element_simple(flex);
  m_non_null_count.add_element_simple(flex);
}

void item_frequency_result::combine(const group_aggregate_value& other) {
  groupby_operators::frequency_count::combine(other);

  /* combine summary stats */
  auto item_frequency_other = dynamic_cast<const item_frequency_result&>(other);
  m_count.combine(item_frequency_other.m_count);
  m_count_distinct.combine(item_frequency_other.m_count_distinct);
  m_non_null_count.combine(item_frequency_other.m_non_null_count);
}

std::string item_frequency_result::vega_summary_data() const {
  std::stringstream ss;

  flex_int num_missing = m_count.emit() - m_non_null_count.emit();
  std::string data = vega_column_data(true);

  ss << "\"type\": \"str\",";
  ss << "\"num_unique\": " << m_count_distinct.emit() << ",";
  ss << "\"num_missing\": " << num_missing << ",";
  ss << "\"categorical\": [" << data << "],";
  ss << "\"numeric\": []";

  return ss.str();

}

static void add_item_and_count(std::stringstream& ss, const std::string& value, size_t i, size_t count, double total_count) {
  if(i != 0){
    ss << ",";
  }

  ss << "{\"label\": ";

  if(value.length() >= 200){
    ss << escape_string(value.substr(0,199) + std::to_string(i));
  }else{
    ss << escape_string(value);
  }

  ss << ",\"label_idx\": ";
  ss << i;
  ss << ",\"count\": ";
  ss << count;
  ss << ",\"percentage\": \"";
  ss << ((100.0 * count)/total_count);
  ss << "%\"}";
}

std::string item_frequency_result::vega_column_data(bool sframe) const {
  std::stringstream ss;
  size_t x = 0;

  auto items_list = emit().get<flex_dict>();
  size_t size_list;
  if(sframe) {
    size_list = std::min(10UL, items_list.size());
  }else{
    size_list = std::min(12UL, items_list.size());
  }

  std::sort(items_list.begin(), items_list.end(), [](const std::pair<turi::flexible_type,flexible_type> &left, const std::pair<turi::flexible_type,flexible_type> &right) {
    DASSERT_EQ(left.second.get_type(), flex_type_enum::INTEGER);
    DASSERT_EQ(right.second.get_type(), flex_type_enum::INTEGER);

    if (left.second == right.second) {
      // ignore undefined (always sort lower -- it'll get ignored later)
      if (left.first.get_type() == flex_type_enum::UNDEFINED ||
          right.first.get_type() == flex_type_enum::UNDEFINED) {
        return false;
      }

      DASSERT_EQ(left.first.get_type(), flex_type_enum::STRING);
      DASSERT_EQ(right.first.get_type(), flex_type_enum::STRING);

      // if count is equal, sort ascending by label
      return right.first > left.first;
    }
    // sort descending by count
    return left.second > right.second;
  });

  size_t total_count = m_count.emit();
  size_t count_so_far = 0;
  for(size_t i=0; i<size_list; i++) {
    const auto& pair = items_list[i];
    const auto& flex_value = pair.first;
    size_t count = pair.second.get<flex_int>();
    count_so_far += count;
    if (flex_value.get_type() == flex_type_enum::UNDEFINED) {
      add_item_and_count(ss, "(null)", x, count, total_count);
    } else {
      DASSERT_TRUE(flex_value.get_type() == flex_type_enum::STRING);
      const auto& value = flex_value.get<flex_string>();
      add_item_and_count(ss, value, x, count, total_count);
    }
    x++;

    // if we have already accounted for over 95% of the data,
    // and we still have 5 or more labels to go, OR
    // if it's the last slot and we still have labels unaccounted for,
    // combine remaining values into an "other" bin.
    size_t labels_remaining = items_list.size() - (i+1);
    size_t count_remaining = total_count - count_so_far;
    double fraction_count_remaining = (double)count_remaining / (double)total_count;
    if ((labels_remaining >= 5 && fraction_count_remaining < 0.05) ||
        (i == size_list - 1 && (items_list.size() - size_list > 0))) {
      std::stringstream combined_value;
      combined_value << "Other (";
      combined_value << labels_remaining << " labels)";
      add_item_and_count(ss, combined_value.str(), x, count_remaining, total_count);
      break;
    }
  }

  return ss.str();
}

namespace turi {
  namespace visualization {

    std::shared_ptr<Plot> plot_item_frequency(
      const gl_sarray& sa, const flexible_type& xlabel, const flexible_type& ylabel, 
      const flexible_type& title) {

        using namespace turi;
        using namespace turi::visualization;

        logprogress_stream << "Materializing SArray" << std::endl;
        sa.materialize();

        if (sa.size() == 0) {
          log_and_throw("Nothing to show; SArray is empty.");
        }

        std::shared_ptr<const gl_sarray> self = std::make_shared<const gl_sarray>(sa);

        item_frequency item_freq;
        item_freq.init(*self, batch_size(sa));

        auto transformer = std::dynamic_pointer_cast<item_frequency_result>(item_freq.get());
        auto result = transformer->emit().get<flex_dict>();
        std::string category_spec = categorical_spec(title, xlabel, ylabel, self->dtype());

        double size_array = static_cast<double>(self->size());

        std::shared_ptr<transformation_base> shared_unity_transformer = std::make_shared<item_frequency>(item_freq);
        return std::make_shared<Plot>(category_spec, shared_unity_transformer, size_array);
      }

  }
}

