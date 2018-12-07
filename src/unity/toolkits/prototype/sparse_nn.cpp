/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/prototype/sparse_nn.hpp>
#include <util/fast_top_k.hpp>

namespace turi {
namespace prototype {

void sparse_nn::train(const gl_sframe& data, const std::string& id_column) {
  // Number this.  To be used later.
  m_ids.resize(data.size());
  std::iota(m_ids.begin(), m_ids.end(), 0);

  if (!id_column.empty()) {
    if (!data.contains_column(id_column)) {
      log_and_throw("data does not contain id column.");
    }
    m_num_columns = data.num_columns() - 1;
  } else {
    m_num_columns = data.num_columns();
  }

  for (flex_type_enum t : data.column_types()) {
    if (t != flex_type_enum::STRING && t != flex_type_enum::INTEGER) {
      log_and_throw("All columns in this model must be integers or strings.");
    }
  }

  std::vector<std::string> columns = data.column_names();

  // Track which rows are "hit" (have matching feature in a given column) for
  // each feature in the original data.
  std::map<hash_type, std::vector<size_t> > hit_tracker;

  // Unpack the data into the intermediate structure.
  size_t row_index = 0;
  for (const auto& row : data.range_iterator()) {
    for (size_t i = 0; i < data.num_columns(); ++i) {
      // Go through and update the hit locations for each match.
      if (id_column != "" && columns[i] == id_column) {
        m_ids[row_index] = row[i];
        continue;
      }

      hit_tracker[feature_hash(columns[i], row[i])].push_back(row_index);
    }

    ++row_index;
  }

  // Now go through and set up the query data structures.
  m_hashes.clear();
  m_hashes.reserve(hit_tracker.size());

  m_access_bounds.clear();
  m_access_bounds.reserve(m_hit_indices.size());

  m_hit_indices.clear();
  m_hit_indices.reserve(data.size() * data.num_columns());

  // hit_tracker is already sorted, so we can just directly set up the lookup
  // tables now
  for (const auto& p : hit_tracker) {
    hash_type hash = p.first;
    const auto& hits = p.second;

    size_t s = m_hit_indices.size();

    m_hashes.push_back(hash);
    m_access_bounds.push_back({s, s + hits.size()});
    m_hit_indices.insert(m_hit_indices.end(), hits.begin(), hits.end());
  }
}

// Perform a fast query of the model.
flex_dict sparse_nn::query(const flex_dict& fd, size_t k) const {
  if (m_num_columns == 0) {
    log_and_throw("Model not trained yet.");
  }

  atomic<size_t> current_index = 0;

  std::vector<atomic<uint32_t> > hit_counts(m_ids.size(), 0);

  // Using in_parallel here with an atomic counter as the size of each of these
  // lookup tables varies significantly.
  in_parallel([&](size_t thread_idx, size_t num_threads) GL_HOT_FLATTEN {

    // Each thread takes the next one
    while (true) {
      size_t idx = current_index++;

      if (idx >= fd.size()) {
        break;
      }

      if (fd[idx].first.get_type() != flex_type_enum::STRING) {
        log_and_throw("Query column in position " + std::to_string(idx) +
                      " not a string column name.");
      }

      const std::string& column = fd[idx].first.get<flex_string>();
      const hash_type& h = feature_hash(column, fd[idx].second);

      auto it = std::lower_bound(m_hashes.begin(), m_hashes.end(), h);

      if (it == m_hashes.end() || *it != h) {
        continue;  // no match.
      }

      size_t lookup_index = it - m_hashes.begin();
      auto b = m_access_bounds[lookup_index];

      for (size_t i = b.first; i < b.second; ++i) {
        ++hit_counts[m_hit_indices[i]];
      }
    }
  });

  // Create a set of hit overlaps and indices in order to extract the top k
  std::vector<std::pair<uint32_t, uint32_t> > hits_idx(hit_counts.size());
  for (size_t i = 0; i < hit_counts.size(); ++i) {
    hits_idx[i] = {hit_counts[i], i};
  }

  extract_and_sort_top_k(hits_idx, k);

  // Package up the return values.
  flex_dict ret(k);

  for (size_t i = 0; i < k; ++i) {
    // Return value is a pair: (m_ids, jaccard similarity)
    ret[i] = {m_ids[hits_idx[i].second],
              double(hits_idx[i].first) /
                  (m_num_columns + fd.size() - hits_idx[i].first)};
  }

  return ret;
}

void sparse_nn::save_impl(oarchive& oarc) const {
  oarc << m_num_columns << m_ids << m_hashes << m_access_bounds << m_hit_indices;
}

void sparse_nn::load_version(iarchive& iarc, size_t version) {
  ASSERT_EQ(version, SPARSE_NN_VERSION);

  iarc >> m_num_columns >> m_ids >> m_hashes >> m_access_bounds >> m_hit_indices;
}

}  // namespace prototype
}  // namespace turi
