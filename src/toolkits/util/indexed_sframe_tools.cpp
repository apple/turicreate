/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/util/indexed_sframe_tools.hpp>
#include <set>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/logging/assertions.hpp>

namespace turi {


/** Constructs a vector of the unique values present in an sframe
 * column having integer type.  The resulting vector is in sorted
 * order, so membership can be queried using std::binary_search.
 */
std::vector<size_t> get_unique_values(std::shared_ptr<sarray<flexible_type> > indexed_column) {

  DASSERT_TRUE(indexed_column->get_type() == flex_type_enum::INTEGER);

  std::set<size_t> seen_items;

  size_t num_segments = indexed_column->num_segments();

  auto reader = indexed_column->get_reader();

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    auto it = reader->begin(sidx);
    auto it_end = reader->end(sidx);

    for(; it != it_end; ++it)
      seen_items.insert((size_t)(*it));
  }

  return std::vector<size_t>(seen_items.begin(), seen_items.end());
}

/** Constructs a vector of the unique values present in an sframe
 * column having integer type.  The resulting vector is in sorted
 * order, so membership can be queried using std::binary_search.
 */
std::shared_ptr<sarray<flexible_type> > make_unique(std::shared_ptr<sarray<flexible_type> > indexed_column) {

  std::vector<size_t> values = get_unique_values(indexed_column);

  std::shared_ptr<sarray<flexible_type> > new_x(new sarray<flexible_type>);

  new_x->open_for_write();
  new_x->set_type(flex_type_enum::INTEGER);

  size_t num_segments = new_x->num_segments();

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    auto it_out = new_x->get_output_iterator(sidx);

    size_t start_idx = (sidx * values.size()) / num_segments;
    size_t end_idx   = ((sidx+1) * values.size()) / num_segments;

    for(size_t i = start_idx; i < end_idx; ++i, ++it_out)
      *it_out = values[i];
  }

  new_x->close();

  return new_x;
}


indexed_column_groupby::indexed_column_groupby(
    std::shared_ptr<sarray<flexible_type> > src_column,
    std::shared_ptr<sarray<flexible_type> > dst_column,
    bool sort,
    bool uniquify) {

  size_t num_segments = src_column->num_segments();

  DASSERT_EQ(src_column->num_segments(), dst_column->num_segments());

  auto src_reader = src_column->get_reader();
  auto dst_reader = dst_column->get_reader();

  DASSERT_TRUE(src_column->get_type() == flex_type_enum::INTEGER);
  DASSERT_TRUE(dst_column->get_type() == flex_type_enum::INTEGER);

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {
    auto src_it     = src_reader->begin(sidx);
    auto src_it_end = src_reader->end(sidx);
    auto dst_it     = dst_reader->begin(sidx);
    auto dst_it_end = dst_reader->end(sidx);

    for(; src_it != src_it_end; ++src_it, ++dst_it) {

      size_t src_value = *src_it;
      size_t dst_value = *dst_it;

      auto lookup_it = group_lookup.find(src_value);

      if(lookup_it == group_lookup.end())
        group_lookup[src_value] = {dst_value};
      else
        lookup_it->second.push_back(dst_value);
    }

    DASSERT_TRUE(dst_it == dst_it_end);
  }

  if(sort) {

    for(auto& p : group_lookup) {
      std::sort(p.second.begin(), p.second.end());

      if(uniquify) {
        auto new_end_it = std::unique(p.second.begin(), p.second.end());
        p.second.resize(new_end_it - p.second.begin());
      }
    }
  }
}

const std::vector<size_t>& indexed_column_groupby::dest_group(size_t src_value) const {

  auto lookup_it = group_lookup.find(src_value);

  if(lookup_it == group_lookup.end())
    return empty_vector;
  else
    return lookup_it->second;
}

}
