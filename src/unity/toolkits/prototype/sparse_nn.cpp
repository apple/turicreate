#include <unity/toolkits/prototype/sparse_nn.hpp>
#include <util/fast_top_k.hpp>

namespace turi {
namespace prototype {

void sparse_nn::train(const gl_sframe& data, const std::string& id_column) {
  // Number this.  To be used later.
  ids.resize(data.size());
  std::iota(ids.begin(), ids.end(), 0);
  num_columns = data.num_columns();

  for (flex_type_enum t : data.column_types()) {
    if (t != flex_type_enum::STRING && t != flex_type_enum::INTEGER) {
      log_and_throw("All columns in this model must be integers or strings.");
    }
  }

  std::vector<std::string> columns = data.column_names();

  // Track which rows are "hit" (have matching feature in a given column) for
  // each feature in the original data.
  std::map<uint128_t, std::vector<size_t> > hit_tracker;

  // Unpack the data into the intermediate structure.
  size_t row_index = 0;
  for (const auto& row : data.range_iterator()) {
    for (size_t i = 0; i < num_columns; ++i) {
      // Go through and update the hit locations for each match.
      if (columns[i] == id_column) {
        ids[row_index] = row[i];
        continue;
      }

      hit_tracker[feature_hash(columns[i], row[i])].push_back(row_index);
    }

    ++row_index;
  }

  // Now go through and set up the query data structures.
  hashes.clear();
  hashes.reserve(hit_tracker.size());

  access_bounds.clear();
  access_bounds.reserve(hit_indices.size());

  hit_indices.clear();
  hit_indices.reserve(data.size() * data.num_columns());

  // hit_tracker is already sorted, so we can just directly set up the lookup
  // tables now
  for (const auto& p : hit_tracker) {
    uint128_t hash = p.first;
    const auto& hits = p.second;

    size_t s = hit_tracker.size();

    hashes.push_back(hash);
    access_bounds.push_back({s, s + hits.size()});
    hit_indices.insert(hit_indices.end(), hits.begin(), hits.end());
  }
}

// Perform a fast query of the model.
GL_HOT_NOINLINE_FLATTEN flex_dict sparse_nn::query(const flex_dict& fd,
                                                   size_t k) const {
  atomic<size_t> current_index = 0;

  std::vector<atomic<uint32_t> > hit_counts(ids.size(), 0);

  in_parallel([&](size_t thread_idx, size_t num_threads) GL_HOT_FLATTEN {

    // Each thread takes the next one
    while (true) {
      size_t idx = (++current_index) - 1;

      if (idx >= num_columns) {
        break;
      }

      if (fd[idx].first.get_type() != flex_type_enum::STRING) {
        log_and_throw("Query column in position " + std::to_string(idx) +
                      " not a string column name.");
      }

      const std::string& column = fd[idx].first.get<flex_string>();
      const uint128_t& h = feature_hash(column, fd[idx].second);

      auto it = std::lower_bound(hashes.begin(), hashes.end(), h);

      if (it == hashes.end() || *it != h) {
        continue;  // no match.
      }

      size_t lookup_index = it - hashes.begin();
      auto b = access_bounds[lookup_index];

      for (size_t i = b.first; i < b.second; ++i) {
        ++hit_counts[i];
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
    // Return value is a pair: (ids, jaccard similarity)
    ret[i] = {
        ids[hits_idx[i].second],
        double(hits_idx[i].first) / (2 * num_columns - hits_idx[i].first)};
  }

  return ret;
}

void sparse_nn::save_impl(oarchive& oarc) const {
  oarc << num_columns << ids << hashes << access_bounds << hit_indices;
}

void sparse_nn::load_version(iarchive& iarc, size_t version) {
  ASSERT_EQ(version, SPARSE_NN_VERSION);

  iarc >> num_columns >> ids >> hashes >> access_bounds >> hit_indices;
}

}  // namespace prototype
}  // namespace turi
