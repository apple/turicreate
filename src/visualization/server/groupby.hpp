/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_VIS_GROUPBY
#define __TC_VIS_GROUPBY

#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/util/sys_util.hpp>

#include "transformation.hpp"

namespace turi {
namespace visualization {

class summary_stats {
  private:
    groupby_operators::average m_average;
    groupby_operators::count m_count;
    groupby_operators::max m_max;
    groupby_operators::min m_min;
    groupby_operators::sum m_sum;
    groupby_operators::stdv m_stdv;
    groupby_operators::variance m_variance;

  public:
    void add_element_simple(const flexible_type& value);
    void combine(const summary_stats& other);
    void partial_finalize();
    flexible_type emit() const;
    void set_input_type(flex_type_enum type);
};

// Intended for boxes and whiskers or bar chart (bivariate plot, categorical
// vs. numeric). For now, just groups by one column (x), doing aggregation per
// category on a second column (y). Limited to the first n categories
// encountered in the x column.
// TODO -- pick the limited set of categories intelligently (n most popular
// rather than n first)
template<typename Aggregation>
class groupby_result {
  protected:
    // keeps track of one aggregator per category (unique value on first column)
    std::unordered_map<flexible_type, Aggregation> m_aggregators;

    virtual void insert_category(const flexible_type& category) {
      TURI_ATTRIBUTE_UNUSED_NDEBUG auto inserted =
        m_aggregators.emplace(category, Aggregation());
      DASSERT_TRUE(inserted.second); // emplace should succeed
      auto& agg = m_aggregators.at(category);
      DASSERT_TRUE(m_type != flex_type_enum::UNDEFINED);
      agg.set_input_type(m_type);
    }

  private:
    constexpr static size_t CATEGORY_LIMIT = 1000;
    flex_int m_omitted_categories = 0;
    flex_type_enum m_type = flex_type_enum::UNDEFINED;

    static void update_or_combine(Aggregation& aggregation, const flexible_type& other) {
      aggregation.add_element_simple(other);
    }
    static void update_or_combine(Aggregation& aggregation, const Aggregation& other) {
      // TODO this is bad -- we need a non-const Aggregation in order to call
      // partial_finalize, but this parameter is deeply const.
      const_cast<Aggregation&>(other).partial_finalize();
      aggregation.combine(other);
    }

  protected:
    template<typename T>
    void update_or_combine(const flexible_type& category, const T& value) {
      auto find_key = m_aggregators.find(category);
      if (find_key == m_aggregators.end()) {
        // insert new category if there is room
        if (m_aggregators.size() < CATEGORY_LIMIT) {
          this->insert_category(category);
          groupby_result::update_or_combine(m_aggregators.at(category), value);
        } else {
          m_omitted_categories++;
        }
      } else {
        groupby_result::update_or_combine((*find_key).second, value);
      }
    }
    void update(const flexible_type& category, const flexible_type& value) {
      const flex_type_enum type = value.get_type();
      if (type == flex_type_enum::UNDEFINED) {
        return; // ignore undefined values, they don't make sense in groupby
      }
      this->set_input_type(type);
      this->update_or_combine(category, value);
    }

  public:
    void combine(const groupby_result<Aggregation>& other) {
      this->set_input_type(other.get_input_type());
      for (const auto& pair : other.m_aggregators) {
        this->update_or_combine(pair.first, pair.second);
      }
    }
    void update(const std::vector<flexible_type>& values) {
      // by convention, values[0] is the grouped column,
      // and values[1] is the aggregated column
      DASSERT_GE(values.size(), 2);
      this->update(values[0], values[1]);
    }
    std::unordered_map<flexible_type, flexible_type> get_grouped() const {
      std::unordered_map<flexible_type, flexible_type> ret;
      for (const auto& pair : m_aggregators) {
        ret.emplace(pair.first, pair.second.emit());
      }
      return ret;
    }
    flex_int get_omitted() { return m_omitted_categories; }
    void set_input_type(flex_type_enum type) {
      if (m_type == flex_type_enum::UNDEFINED) {
        m_type = type;
      } else {
        DASSERT_TRUE(m_type == type);
      }
    }
    flex_type_enum get_input_type() const {
      return m_type;
    }
    void add_element_simple(const flexible_type& value) {
      DASSERT_TRUE(value.get_type() == flex_type_enum::LIST);
      this->update(value.get<flex_list>());
    }
};

template<typename Result>
class groupby : public transformation<gl_sframe, Result> {
  protected:
    virtual void merge_results(std::vector<Result>& transformers) override {
      for (auto& result : transformers) {
        this->m_transformer->combine(result);
      }
    }
};

class groupby_summary_result : public groupby_result<summary_stats> {
};

class groupby_summary : public groupby<groupby_summary_result> {
};

class groupby_quantile_result : public groupby_result<groupby_operators::quantile> {
  public:
    virtual void insert_category(const flexible_type& category) override;
};

class groupby_quantile : public groupby<groupby_quantile_result> {
};

}}

#endif // __TC_VIS_GROUPBY
