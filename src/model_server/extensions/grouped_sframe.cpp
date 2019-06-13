/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/extensions/grouped_sframe.hpp>

namespace turi {

/// Public methods
void grouped_sframe::group(const gl_sframe &sf, const std::vector<std::string>
    column_names, bool is_grouped) {
  if(m_inited)
    log_and_throw("Group has already been called on this object!");

  // Do our "grouping" if it hasn't already been done
  if(!is_grouped) {
    m_grouped_sf = sf.sort(column_names);
  } else {
    m_grouped_sf = sf;
  }
  m_key_col_names = column_names;

  // Get indices from column names
  std::vector<size_t> col_ids;
  std::unordered_set<size_t> dedup_set;
  for(const auto &i : column_names) {
    auto col_id = sf.column_index(i);
    col_ids.push_back(col_id);
    auto ins_ret = dedup_set.insert(col_id);
    if(!ins_ret.second)
      log_and_throw("Found duplicate column name: " + i);
  }

  // Build the directory of ranges to allow querying of the groups
  // (this is an extra, sequential pass over the data)
  auto sf_range = m_grouped_sf.range_iterator();
  auto iter = sf_range.begin();
  size_t cnt = 0;
  std::vector<flexible_type> prev_elem(col_ids.size());
  std::vector<flexible_type> cur_elem(col_ids.size());
  bool first = true;
  for(; iter != sf_range.end(); ++iter, ++cnt) {
    // Create cur_elem
    int col_cnt = 0;
    for(const auto &i : col_ids) {
      cur_elem[col_cnt] = (*iter)[i];
      ++col_cnt;
    }

    // Check for new group
    if((cur_elem != prev_elem) || first) {
      first = false;
      m_key2range.insert(std::make_pair(cur_elem, m_range_directory.size()));
      m_range_directory.push_back(cnt);
      if(cur_elem.size() == 1)
        m_group_names.push_back(cur_elem[0]);
      else
        m_group_names.push_back(cur_elem);
    }

    prev_elem = cur_elem;
  }

  if(col_ids.size() > 1) {
    m_group_type = flex_type_enum::LIST;
  } else {
    m_group_type = prev_elem[0].get_type();
  }

  m_inited = true;
}

gl_sframe grouped_sframe::get_group(std::vector<flexible_type> key) {
  if(!m_inited) {
    log_and_throw("The 'group' operation needs to occur before getting a "
        "group!");
  }
  //TODO: This is a HUGE hack. From the Python side, a list of ints is turned
  //in to a list of floats unless there's a None in it. We add a None so this
  //doesn't happen and take it away to look up the group. We'll actually fix
  //the problem soon.
  if((key.size() > 1) && (key.back().get_type() == flex_type_enum::UNDEFINED)) {
    key.pop_back();
  }
  auto ret = m_key2range.find(key);
  if(ret == m_key2range.end()) {
    log_and_throw("Group not found!");
  }

  size_t range_dir_idx = ret->second;

  return this->get_group_by_index(range_dir_idx);
}


gl_sarray grouped_sframe::groups() {
  if(!m_inited)
    log_and_throw("The 'group' operation needs to occur before getting the "
        "list of groups!");

  // Construct the SArray of group names if it hasn't been constructed
  if(m_groups_sa.size() == 0) {
    // Determine type
    auto wr = std::make_shared<gl_sarray_writer>(m_group_type,8);
    for(const auto &i : m_group_names) {
      wr->write(i, 0);
    }

    m_groups_sa = wr->close();
  }

  return m_groups_sa;
}


std::vector<std::pair<flexible_type,gl_sframe>>
grouped_sframe::iterator_get_next(size_t len) {
  if(!m_inited)
    log_and_throw("The 'group' operation needs to occur before iteration!");
  if(!m_iterating)
    log_and_throw("Must begin iteration before iterating!");

  std::vector<std::pair<flexible_type,gl_sframe>> ret;

  if(len < 1) {
    return ret;
  } else {
    auto items_left = m_range_directory.size() - m_cur_iterator_idx;
    if(len > items_left) {
      len = items_left;
    }
    ret.resize(len);
  }

  size_t cur_cnt = 0;
  for(; (m_cur_iterator_idx < m_range_directory.size()) && (cur_cnt < len);
      ++m_cur_iterator_idx, ++cur_cnt) {
    auto sf = this->get_group_by_index(m_cur_iterator_idx);
    auto name = m_group_names[m_cur_iterator_idx];
    ret[cur_cnt] = std::make_pair(name, sf);
  }

  if(cur_cnt < len) {
    m_iterating = false;
  }

  return ret;
}

gl_sframe grouped_sframe::group_info() const {
  if (m_group_names.size() == 0) {
    log_and_throw("No groups present. Cannot obtain group info.");
  }

  // Return column names.
  std::vector<std::string> ret_column_names = m_key_col_names;
  ret_column_names.push_back("group_size");
  DASSERT_EQ(ret_column_names.size(), m_key_col_names.size() + 1);

  // Return column types from the first group info.
  DASSERT_TRUE(m_group_names.size() > 1);
  std::vector<flex_type_enum> ret_column_types;
  flexible_type first_key = m_group_names[0];
  flex_type_enum key_type = first_key.get_type();
  if (key_type == flex_type_enum::LIST) {
    for (size_t k = 0; k < first_key.size(); k++) {
      ret_column_types.push_back(first_key.array_at(k).get_type());
    }
  } else {
    ret_column_types.push_back(key_type);
  }
  ret_column_types.push_back(flex_type_enum::INTEGER);
  DASSERT_EQ(ret_column_types.size(), ret_column_names.size());

  // Prepare for writing.
  size_t num_segments = thread::cpu_count();
  gl_sframe_writer writer(ret_column_names, ret_column_types, num_segments);
  size_t range_dir_size = m_range_directory.size();

  // Write the group info.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    size_t start_idx = range_dir_size * thread_idx / num_threads;
    size_t end_idx = range_dir_size * (thread_idx + 1) / num_threads;

    for (size_t i = start_idx; i < end_idx; i++) {
      size_t range_start = m_range_directory[i];
      size_t range_end = 0;
      if((i + 1) == m_range_directory.size()) {
        range_end = m_grouped_sf.size();
      } else {
        range_end = m_range_directory[i + 1];
      }
      size_t num_rows = range_end - range_start;
      std::vector<flexible_type> vals = m_group_names[i];
      vals.push_back(num_rows);
      DASSERT_EQ(vals.size(), ret_column_names.size());
      writer.write(vals, thread_idx);
    }
  });
  return writer.close();
}

/// Private methods
gl_sframe grouped_sframe::get_group_by_index(size_t range_dir_idx) {
  int64_t range_start = m_range_directory[range_dir_idx];
  int64_t range_end;
  if((range_dir_idx+1) == m_range_directory.size()) {
    range_end = m_grouped_sf.size();
  } else {
    range_end = m_range_directory[range_dir_idx+1];
  }

  return m_grouped_sf[{range_start,range_end}];
}


} // namespace turi

using namespace turi;

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(grouped_sframe)
END_CLASS_REGISTRATION
