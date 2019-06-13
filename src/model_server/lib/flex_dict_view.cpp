/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/flex_dict_view.hpp>

namespace turi {

  flex_dict_view::flex_dict_view(const flex_dict& value) {
    m_flex_dict_ptr = &value;
  }

  flex_dict_view::flex_dict_view(const flexible_type& value) {
    if (value.get_type() == flex_type_enum::DICT) {
      m_flex_dict_ptr = &(value.get<flex_dict>());
      return;
    }

    log_and_throw("Cannot construct a flex_dict_view object from type ");
  }

  const flexible_type& flex_dict_view::operator[](const flexible_type& key) const {
    for(const auto& value : (*m_flex_dict_ptr)) {
      if (value.first == key) {
        return value.second;
      }
    }

    std::stringstream s;
    s << "Cannot find key " << key << " in flex_dict.";
    log_and_throw(s.str());
  }

  bool flex_dict_view::has_key(const flexible_type& key) const {
    for(const auto& value : (*m_flex_dict_ptr)) {
      if (value.first == key) {
        return true;
      }
    }

    return false;
  }

  size_t flex_dict_view::size() const {
    return m_flex_dict_ptr->size();
  }

  const std::vector<flexible_type>& flex_dict_view::keys() {

    // lazy materialization of keys
    if (m_keys.size() != m_flex_dict_ptr->size()) {
      m_keys.reserve(m_flex_dict_ptr->size());
      for(const auto& value : (*m_flex_dict_ptr)) {
        m_keys.push_back(value.first);
      }
    }

    return m_keys;
  }

  const std::vector<flexible_type>& flex_dict_view::values() {
    // lazy materialization of values
    if (m_values.size() != m_flex_dict_ptr->size()) {
      m_values.reserve(m_flex_dict_ptr->size());
      for(const auto& value : (*m_flex_dict_ptr)) {
        m_values.push_back(value.second);
      }
    }

    return m_values;
  }

  flex_dict::const_iterator flex_dict_view::begin() const {
    return m_flex_dict_ptr->begin();
  }

  flex_dict::const_iterator flex_dict_view::end() const {
    return m_flex_dict_ptr->end();
  }
}
