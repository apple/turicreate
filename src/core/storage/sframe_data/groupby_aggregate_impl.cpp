/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unordered_set>
#include <memory>
#include <queue>
#include <thread>
#include <core/storage/sframe_data/groupby_aggregate_impl.hpp>
#include <core/storage/sframe_data/sarray_reader_buffer.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/util/fs_util.hpp>
#include <core/storage/sframe_data/groupby_aggregate.hpp>

namespace turi {
namespace groupby_aggregate_impl {

/****************************************************************************/
/*                                                                          */
/*                             groupbp_element                              */
/*                                                                          */
/****************************************************************************/
groupby_element::groupby_element(std::vector<flexible_type> group_key,
                                 const std::vector<group_descriptor>& group_desc) {
  init(std::move(group_key), group_desc);
}

groupby_element::groupby_element(const std::string& val,
                                 const std::vector<group_descriptor>& group_desc) {
  iarchive iarc(val.c_str(), val.length());
  load(iarc, group_desc);
}

void groupby_element::init(std::vector<flexible_type> group_key,
                           const std::vector<group_descriptor>& group_desc) {
  key = std::move(group_key);
  values.resize(group_desc.size());
  for (size_t i = 0;i < group_desc.size(); ++i) {
    values[i].reset(group_desc[i].aggregator->new_instance());
  }
  compute_hash();
}


void groupby_element::save(oarchive& oarc) const {
  oarc << key;
  for (size_t i = 0;i < values.size(); ++i) {
    values[i]->save(oarc);
  }
}

void groupby_element::load(iarchive& iarc,
                           const std::vector<group_descriptor>& group_desc) {
  iarc >> key;
  values.resize(group_desc.size());
  for (size_t i = 0;i < values.size(); ++i) {
    values[i].reset(group_desc[i].aggregator->new_instance());
    values[i]->load(iarc);
  }
  compute_hash();
}


template <typename VT, typename VS>
bool flexible_type_vector_equality(const VT& a,
                                   const VS& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].get_type() != b[i].get_type()) return false;
    if (a[i].get_type() == flex_type_enum::UNDEFINED &&
        b[i].get_type() == flex_type_enum::UNDEFINED) continue;
    else if (a[i] != b[i]) return false;
  }
  return true;
}


template <typename VT, typename VS>
bool flexible_type_vector_equality(const VT& a,
                                   size_t alen,
                                   const VS& b,
                                   size_t blen) {
  if (alen != blen) return false;
  for (size_t i = 0; i < alen; ++i) {
    if (a[i].get_type() != b[i].get_type()) return false;
    if (a[i].get_type() == flex_type_enum::UNDEFINED &&
        b[i].get_type() == flex_type_enum::UNDEFINED) continue;
    else if (a[i] != b[i]) return false;
  }
  return true;
}


template <typename VT, typename VS>
bool flexible_type_vector_lt(const VT& a,
                             const VS& b) {
  if (a.size() < b.size()) return true;
  for (size_t i = 0; i < a.size(); ++i) {
    auto atype = a[i].get_type();
    auto btype = b[i].get_type();
    if (atype < btype) return true;
    else if (atype > btype) return false;
    if (atype == flex_type_enum::UNDEFINED &&
        btype == flex_type_enum::UNDEFINED) continue;
    if (a[i] < b[i]) return true;
    else if (a[i] > b[i]) return false;
  }
  return false;
}

bool groupby_element::operator<(const groupby_element& other) const {
  if (hash() != other.hash()) {
    return hash() < other.hash();
  } else {
    // if hash collision, use the full compare
    return flexible_type_vector_lt(key, other.key);
  }
}


bool groupby_element::operator==(const groupby_element& other) const {
  if (hash() != other.hash()) {
    return false;
  } else {
    // hashes are equal. Do a full equality test
    return flexible_type_vector_equality(key, other.key);
  }
}

bool groupby_element::operator>(const groupby_element& other) const {
  if (hash() != other.hash()) {
    return hash() > other.hash();
  } else {
    // if hash collision, use the full compare
    return !flexible_type_vector_lt(key, other.key) && !flexible_type_vector_equality(key, other.key);
  }
}

void groupby_element::operator+=(const groupby_element& other) {
  for (size_t i = 0; i < values.size(); ++i) {
    values[i]->combine(*other.values[i]);
  }
}

template <typename T>
void groupby_element::add_element(const T& val,
                                  const std::vector<group_descriptor>& group_desc) const {
  for (size_t i = 0; i < group_desc.size(); ++i) {
    size_t num_input_columns = group_desc[i].column_numbers.size();

    if (num_input_columns == 0) {
      values[i]->add_element_simple(0);
    } else if (num_input_columns == 1) {
      auto& grouped_column_number = group_desc[i].column_numbers[0];
      if (grouped_column_number < val.size()) {
        values[i]->add_element_simple(val[grouped_column_number]);
      } else {
        values[i]->add_element_simple(FLEX_UNDEFINED);
      }
    } else {
      std::vector<flexible_type> vals(num_input_columns);
      for(size_t j = 0; j < num_input_columns; j++) {
        size_t grouped_column_number = group_desc[i].column_numbers[j];
        if (grouped_column_number < val.size()) {
          vals[j] = val[grouped_column_number];
        } else {
          vals[j] = flexible_type(flex_type_enum::UNDEFINED);
        }
      }
      values[i]->add_element(vals);
    }
  }
}

size_t groupby_element::hash_key(const std::vector<flexible_type>& key) {
  size_t ret = 0;
  for (size_t i = 0;i < key.size(); ++i) {
    ret = hash64_combine(ret, key[i].hash());
  }
  return ret;
}

size_t groupby_element::hash_key(const std::vector<flexible_type>& key, size_t len) {
  size_t ret = 0;
  for (size_t i = 0;i < len; ++i) {
    ret = hash64_combine(ret, key[i].hash());
  }
  return ret;
}


size_t groupby_element::hash_key(const sframe_rows::row& key) {
  size_t ret = 0;
  for (size_t i = 0;i < key.size(); ++i) {
    ret = hash64_combine(ret, key[i].hash());
  }
  return ret;
}

size_t groupby_element::hash_key(const sframe_rows::row& key, size_t len) {
  size_t ret = 0;
  for (size_t i = 0;i < len; ++i) {
    ret = hash64_combine(ret, key[i].hash());
  }
  return ret;
}

void groupby_element::compute_hash() {
  hash_val = hash_key(key);
}

size_t groupby_element::hash() const {
  return hash_val;
}

/****************************************************************************/
/*                                                                          */
/*                         group_aggregate_container                        */
/*                                                                          */
/****************************************************************************/

/* static thread_local member initialization */
thread_local group_aggregate_container::tls_segment_set
    group_aggregate_container::tss_{};

group_aggregate_container::group_aggregate_container(size_t max_buffer_size,
                                                     size_t num_segments)
    : max_buffer_size(max_buffer_size),
      num_segments(num_segments),
      task_id_(0),
      gl_lock_pool_(num_segments),
      gl_chunk_size_set_(num_segments) {
  if (num_segments == 0) {
    log_and_throw("num_segments is not allowed to be 0");
  }

  size_t num_local_buffers =
      fs_util::get_file_handle_limit() / num_segments / 2;

  logstream(LOG_INFO) << "num_local_buffers: " << num_local_buffers << std::endl;

  num_local_buffers =
      num_local_buffers == 0
          ? 1
          : std::min<size_t>(num_local_buffers, thread_pool::get_instance().size());

  // construct local buffers
  for (size_t ii = 0; ii < num_local_buffers; ++ii) {
    local_buffer_set_.emplace_back(num_segments);
    local_buffer_set_.back().sa_buffer_ptr_->open_for_write(num_segments);
  }
}

void group_aggregate_container::define_group(
    std::vector<size_t> column_numbers,
    std::shared_ptr<group_aggregate_value> aggregator) {
  group_descriptor desc;
  desc.column_numbers = column_numbers;
  desc.aggregator = aggregator;
  group_descriptors.push_back(desc);
}

void group_aggregate_container::init_tls() {
  ASSERT_MSG(num_segments > 0, "num_segments cannot be 0");

  ASSERT_MSG(UNLIKELY(!tss_.init_), "double init is not allowed");

  tss_.segments_.resize(num_segments);
  for (size_t ii = 0; ii < num_segments; ++ii) {
    tss_.segments_[ii].segment_id_ = ii;
  }

  {
    std::lock_guard<turi::simple_spinlock> lock(gl_buffer_lock_);
    tss_.id_ = ++task_id_;
  }

  tss_.init_ = true;
}

void group_aggregate_container::flush_tls() {
  throw_if_not_initialized();

  // MUST flush and delete raw pointer elements
  for (size_t ii = 0; ii < num_segments; ++ii) flush_segment(ii);

  tss_.init_ = false;
  tss_.segments_.clear();
}

void group_aggregate_container::add(const std::vector<flexible_type>& val,
                                    size_t num_keys) {
  throw_if_not_initialized();

  size_t hash = groupby_element::hash_key(val, num_keys);
  size_t target_segment = hash % num_segments;
  // acquire lock on the segment
  // look for the id in the group_keys structure
  auto& segments = tss_.segments_;
  auto& groupby_element_vec_ptr = segments[target_segment].elements_[hash];
  if (groupby_element_vec_ptr == NULL)
    groupby_element_vec_ptr = new std::vector<groupby_element>;
  // note. not auto&. This needs to take a real value (pointer) and not a
  // reference since the auto& will make it really a reference to a pointer to
  // a vector<groupby_element> which will make it not robust to array resizes.
  auto groupby_element_vec = groupby_element_vec_ptr;
  for (size_t i = 0; i < groupby_element_vec->size(); ++i) {
    if (flexible_type_vector_equality((*groupby_element_vec)[i].key,
                                      (*groupby_element_vec)[i].key.size(), val,
                                      num_keys)) {
      (*groupby_element_vec)[i].add_element(val, group_descriptors);
      if (segments[target_segment].elements_.size() >= max_buffer_size) {
        flush_segment(target_segment);
      }
      return;
    }
  }

  // element not found
  groupby_element_vec->push_back(groupby_element{
      std::vector<flexible_type>(val.begin(), val.begin() + num_keys),
      group_descriptors});
  (*groupby_element_vec)[groupby_element_vec->size() - 1].add_element(
      val, group_descriptors);
}

/* using thread_local for optimization */
void group_aggregate_container::add(const sframe_rows::row& val,
                                    size_t num_keys) {
  throw_if_not_initialized();

  size_t hash = groupby_element::hash_key(val, num_keys);
  size_t target_segment = hash % num_segments;

  auto& segments = tss_.segments_;
  auto& groupby_element_vec_ptr = segments[target_segment].elements_[hash];
  if (groupby_element_vec_ptr == NULL)
    groupby_element_vec_ptr = new std::vector<groupby_element>;
  // note. not auto&. This needs to take a real value (pointer) and not a
  // reference since the auto& will make it really a reference to a pointer to
  // a vector<groupby_element> which will make it not robust to array resizes.
  auto groupby_element_vec = groupby_element_vec_ptr;

  for (size_t i = 0; i < groupby_element_vec->size(); ++i) {
    if (flexible_type_vector_equality((*groupby_element_vec)[i].key,
                                      (*groupby_element_vec)[i].key.size(), val,
                                      num_keys)) {
      (*groupby_element_vec)[i].add_element(val, group_descriptors);
      if (segments[target_segment].elements_.size() >= max_buffer_size) {
        flush_segment(target_segment);
      }
      return;
    }
  }

  std::vector<flexible_type> keys;
  keys.reserve(num_keys);
  for (size_t i = 0; i < num_keys; ++i) keys.push_back(val[i]);

  groupby_element_vec->push_back(
      groupby_element{std::move(keys), group_descriptors});

  (*groupby_element_vec)[groupby_element_vec->size() - 1].add_element(
      val, group_descriptors);
}

void group_aggregate_container::flush_segment(size_t segmentid) {
  logstream(LOG_DEBUG) << "flush buffer from task_id: " << tss_.id_
                       << ", segment_id" << segmentid << std::endl;

  auto& segments = tss_.segments_;
  if (segments[segmentid].elements_.size() == 0) return;

  decltype(segments[segmentid].elements_) local;
  local.swap(segments[segmentid].elements_);

  if (local.size() == 0) return;

  // sort the buckets by hash key
  std::vector<std::pair<size_t, std::vector<groupby_element>*> >
      local_ordered_by_hash;
  local_ordered_by_hash.reserve(local.size());
  std::move(local.begin(), local.end(),
            std::inserter(local_ordered_by_hash, local_ordered_by_hash.end()));
  std::sort(local_ordered_by_hash.begin(), local_ordered_by_hash.end());

  std::vector<groupby_element> local_sorted;
  for (const auto& hash_entries : local_ordered_by_hash) {
    if (hash_entries.second->size() > 1) {
      std::sort(hash_entries.second->begin(), hash_entries.second->end());
    }
    std::move(hash_entries.second->begin(), hash_entries.second->end(),
              std::inserter(local_sorted, local_sorted.end()));
    delete hash_entries.second;
  }

  for (auto& item : local_sorted) {
    for (auto& value : item.values) {
      value->partial_finalize();
    }
  }
  // ok. now we can write! lock the file
  size_t round_robin = (++merry_go_round_) % local_buffer_set_.size();

  auto& local_buffer = local_buffer_set_[round_robin];

  local_buffer.refctr_++;
  /// lock the segmeent from local_buffer before using it
  {
    std::lock_guard<turi::simple_spinlock> slk(
        local_buffer.sa_seg_locks_[segmentid]);

    logstream(LOG_INFO) << "flush buffer from task_id: " << tss_.id_
                        << ", segment_id: " << segmentid
                        << ", on buffer: " << round_robin << std::endl;

    auto outiter = local_buffer.sa_buffer_ptr_->get_output_iterator(segmentid);

    std::vector<char> buffer;
    for (auto& item : local_sorted) {
      buffer.clear();
      oarchive out(buffer);
      out << item;
      *(outiter) = std::string(buffer.data(), buffer.size());
      ++(outiter);
    }

    local_buffer.sa_seg_chunks_[segmentid].push_back(local_sorted.size());
  }
  local_buffer.refctr_--;
}

void group_aggregate_container::merge_local_buffer_set() {
  ASSERT_MSG(
      UNLIKELY(!gl_buffer_.is_opened_for_write()),
      "intermediate_buffer shall be closed before stealing local_buffer.");

  turi::timer ti;
  logstream(LOG_INFO) << "Merging local buffer set " << std::endl;

  // merge all other local buffers to first local buffer
  std::unique_ptr<sarray<std::string>> buffer_to_merge =
      std::move(local_buffer_set_[0].sa_buffer_ptr_);

  auto& chunk_to_merge = local_buffer_set_[0].sa_seg_chunks_;

  for (auto& chunk : chunk_to_merge) {
    chunk.reserve(chunk.size() * local_buffer_set_.size());
  }

  // skip empty segments
  std::vector<size_t> non_empty_segment;
  non_empty_segment.reserve(num_segments);

  for (size_t ii = 1; ii < local_buffer_set_.size(); ++ii) {
    auto& local_buffer = local_buffer_set_[ii];
    while (local_buffer.refctr_ > 0) cpu_relax();
    // no writers now, close it
    local_buffer.sa_buffer_ptr_->close();

    non_empty_segment.clear();
    auto reader = local_buffer.sa_buffer_ptr_->get_reader();
    DASSERT_EQ(reader->num_segments(), num_segments);

    for (size_t ii = 0; ii < num_segments; ii++) {
      if (reader->segment_length(ii)) {
        non_empty_segment.push_back(ii);
      }
    };

    parallel_for(0, non_empty_segment.size(), [&](size_t ii) {
      size_t segment_id = non_empty_segment[ii];

      auto reader = local_buffer.sa_buffer_ptr_->get_reader();
      DASSERT_EQ(reader->num_segments(), num_segments);

      auto outiter = buffer_to_merge->get_output_iterator(segment_id);
      auto begin = reader->begin(segment_id);
      auto end = reader->end(segment_id);

      logstream(LOG_DEBUG) << "segment_id: " << segment_id
                           << ", segment_length: "
                           << reader->segment_length(segment_id)
                           << ", buffer: " << ii << std::endl;

      // sequential write of from other sarray
      while (begin != end) {
        *outiter = *begin;
        ++begin;
        ++outiter;
      }
      // merge chunk size
      chunk_to_merge[segment_id].insert(
          std::end(chunk_to_merge[segment_id]),
          local_buffer.sa_seg_chunks_[segment_id].begin(),
          local_buffer.sa_seg_chunks_[segment_id].end());
    });

  }

  // steal the first local buffer
  while (local_buffer_set_.front().refctr_ > 0) cpu_relax();
  buffer_to_merge->close();
  gl_buffer_ = std::move(*buffer_to_merge);
  gl_chunk_size_set_ = std::move(chunk_to_merge);

  logstream(LOG_INFO) << "Merging finished in " << ti.current_time_millis()
                          << " ms." << std::endl;
}

void group_aggregate_container::group_and_write(sframe& out) {
  ASSERT_MSG(UNLIKELY(!tss_.init_),
             "call flush_tls fisrt before write out groupby result");

  merge_local_buffer_set();

  if (gl_buffer_.is_opened_for_write()) gl_buffer_.close();

  std::shared_ptr<sarray<std::string>::reader_type> reader =
      gl_buffer_.get_reader();

  logstream(LOG_INFO) << "Groupby output segment balance: ";
  for (size_t i = 0; i < reader->num_segments(); ++i) {
    logstream(LOG_INFO) << reader->segment_length(i) << " ";
  }
  logstream(LOG_INFO) << std::endl;

  parallel_for(0, reader->num_segments(), [&](size_t i) {
    this->group_and_write_segment(out, reader, i);
  });
}

void group_aggregate_container::group_and_write_segment(
    sframe& out, std::shared_ptr<sarray<std::string>::reader_type> reader,
    size_t segmentid) {

  // prepare the begin row and end row for each chunk.
  size_t segment_start = 0;
  for (size_t i = 0; i < segmentid; ++i) {
    segment_start += reader->segment_length(i);
  }

  // each chunk stores a sequential read of the segments,
  // and elements in each chunk are already sorted.
  std::vector<sarray_reader_buffer<std::string> > chunks;

  size_t prev_row_start = segment_start;
  for (size_t i = 0; i < gl_chunk_size_set_[segmentid].size(); ++i) {
    size_t row_start = prev_row_start;
    size_t row_end = row_start + gl_chunk_size_set_[segmentid][i];
    prev_row_start = row_end;
    chunks.push_back(
        sarray_reader_buffer<std::string>(reader, row_start, row_end));
  }

  // here is where we are going to write to
  auto outiter = out.get_output_iterator(segmentid);

  // merge the chunks and write to the out iterator
  typedef std::pair<groupby_element, size_t> pq_value_type;
  std::vector<pq_value_type> pq;

  // insert one element from each chunk into the priority queue.
  for (size_t i = 0; i < chunks.size(); ++i) {
    if (chunks[i].has_next()) {
      std::pair<groupby_element, size_t> pqelem;
      pqelem.first = groupby_element(chunks[i].next(), group_descriptors);
      pqelem.second = i;
      pq.push_back(std::move(pqelem));
    }
  }
  // nothing to do
  if (pq.size() == 0) return;

  std::make_heap(pq.begin(), pq.end(), std::greater<pq_value_type>());

  std::vector<flexible_type> emission_vector;
  while (!pq.empty()) {
    // element to group
    size_t id;
    groupby_element cur;
    std::tie(cur, id) = std::move(pq.front());
    std::pop_heap(pq.begin(), pq.end(), std::greater<pq_value_type>());
    pq.pop_back();
    // refill
    if (chunks[id].has_next()) {
      std::pair<groupby_element, size_t> pqelem;
      pqelem.first = groupby_element(chunks[id].next(), group_descriptors);
      pqelem.second = id;
      pq.push_back(std::move(pqelem));
      std::push_heap(pq.begin(), pq.end(), std::greater<pq_value_type>());
    }

    while (pq.size() > 0 && pq[0].first == cur) {
      groupby_element addcur;
      size_t id;
      std::tie(addcur, id) = std::move(pq.front());
      std::pop_heap(pq.begin(), pq.end(), std::greater<pq_value_type>());
      pq.pop_back();
      cur += addcur;
      // refill
      if (chunks[id].has_next()) {
        std::pair<groupby_element, size_t> pqelem;
        pqelem.first = groupby_element(chunks[id].next(), group_descriptors);
        pqelem.second = id;
        pq.push_back(std::move(pqelem));
        std::push_heap(pq.begin(), pq.end(), std::greater<pq_value_type>());
      }
    }

    // emit
    emission_vector.resize(cur.key.size() + cur.values.size());
    for (size_t i = 0; i < cur.key.size(); ++i) emission_vector[i] = cur.key[i];
    for (size_t i = 0; i < cur.values.size(); ++i) {
      emission_vector[i + cur.key.size()] = cur.values[i]->emit();
    }
    *outiter = emission_vector;
    ++outiter;
  }
}

}  // namespace groupby_aggregate_impl
}  // namespace turi
