/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <future>
#include <thread>
#include <unordered_set>
#include <core/parallel/mutex.hpp>
#include <ml/sketches/unity_sketch.hpp>
#include <core/storage/sframe_interface/unity_sarray.hpp>
#include <model_server/lib/flex_dict_view.hpp>
#include <ml/sketches/hyperloglog.hpp>
#include <ml/sketches/countsketch.hpp>
#include <ml/sketches/quantile_sketch.hpp>
#include <ml/sketches/space_saving_flextype.hpp>
#include <ml/sketches/streaming_quantile_sketch.hpp>
#include <core/parallel/lambda_omp.hpp>

namespace turi {

const double unity_sketch::SKETCH_COMMIT_INTERVAL = 3.0;

void unity_sketch::construct_from_sarray(
    std::shared_ptr<unity_sarray_base> uarray, bool background, const std::vector<flexible_type>& keys) {
  auto array = std::static_pointer_cast<unity_sarray>(uarray)->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type>::reader_type>
      reader(array->get_reader());

  std::unordered_set<flexible_type> key_set = std::unordered_set<flexible_type>(keys.begin(), keys.end());
  init(NULL, array->get_type(), key_set, reader);
  if (m_size == 0) {
    // fast exit for 0 length array.
    // set meainingful values for everything and die
    // Also, not all sketches provide meaningful answers on an empty length
    // array. (quantile?). We need special handling for that.
    empty_sketch();
    return;
  }

  // build up the thread local datastructures
  m_commit_timer.start();
  m_background_future =
      std::async(std::launch::async,
         [this, reader, key_set](){
         in_parallel([this, reader, key_set](size_t thr, size_t nthreads) {
               size_t row_start = thr * reader->size() / nthreads;
               size_t row_end = (thr + 1) * reader->size() / nthreads;
               std::vector<flexible_type> data;
               // iterate through all the rows
               while (row_start < row_end) {
                if (m_cancel) break;

                 size_t last_row = std::min(row_start + 1024, row_end);
                 reader->read_rows(row_start, last_row, data);
                 std::unique_lock<turi::mutex> thrlocal_lock(m_thrlocks[thr]);

                 for (const flexible_type& val: data) {
                    accumulate_one_value(thr, val, key_set);
                 }

                 m_thrlocal[thr].num_elements_processed += data.size();
                 m_rows_processed_by_threads.inc(data.size());

                 row_start = last_row;
               }
       });
    });
  if (!background) {
    m_background_future.wait();
    commit_global_if_out_of_date();
  }
}

unity_sketch::~unity_sketch() {
  if (m_background_future.valid()) {
    m_cancel = true;
    m_background_future.wait();
  }
}

flex_type_enum unity_sketch::infer_dict_value_type(std::shared_ptr<sarray<flexible_type>::reader_type> reader) {

  // Read some number of rows and decide the DICT value type, try to use numeric
  // sketch if possible, otherwise use discrete sketch
  std::vector<flexible_type> data;
  reader->read_rows(0, std::min((size_t)100, (size_t)reader->size()), data);
  bool has_data = false;

  for (const flexible_type& val: data) {
    if (val == FLEX_UNDEFINED) continue;
    has_data = true;
    for(auto dict_val : val.get<flex_dict>()) {
      if (!flex_type_is_convertible(dict_val.second.get_type(), flex_type_enum::FLOAT)) {
        return flex_type_enum::STRING;
      }
    }
  }
  if (has_data == false) return flex_type_enum::STRING;
  else return flex_type_enum::FLOAT;
}

void unity_sketch::accumulate_one_value(size_t thr, const flexible_type& val, const std::unordered_set<flexible_type>& keys) {
  if (val == FLEX_UNDEFINED) {
    ++m_thrlocal[thr].undefined_count;

    // fill in with NA for missing value
    if (!keys.empty()) {
      for(auto key: keys) {
        m_element_sub_sketch[key]->accumulate_one_value(thr, FLEX_UNDEFINED);
        increase_nested_element_count(*(m_element_sub_sketch[key]), thr, 1);
      }
    }

    return;
  }

  if (m_is_numeric) {
    accumulate_numeric_sketch(thr, val);
  }

  if (m_is_list) {
    accumulate_discrete_sketch(thr, flex_string(val));
  } else {
    accumulate_discrete_sketch(thr, val);
  }

  if (m_is_list) {
    size_t element_count = val.size();
    m_element_len_sketch->accumulate_one_value(thr, element_count);
    increase_nested_element_count(*m_element_len_sketch, thr, 1);

    if (m_stored_type == flex_type_enum::DICT) {
      accumulate_dict_value(val, thr, keys);
    } else if (m_stored_type == flex_type_enum::VECTOR) {
      accumulate_vector_value(val, thr, keys);
    } else if (m_stored_type == flex_type_enum::LIST) {
      accumulate_list_value(val, thr);
    }
  }
}

void unity_sketch::accumulate_list_value(const flexible_type& val, size_t thr) {
  increase_nested_element_count(*m_element_sketch, thr, val.size());

  for(auto rec_val : val.get<flex_list>()) {
    if (rec_val == FLEX_UNDEFINED) {
      m_element_sketch->accumulate_one_value(thr, FLEX_UNDEFINED);
    } else {
      m_element_sketch->accumulate_one_value(thr, flex_string(rec_val));
    }
  }
}

void unity_sketch::accumulate_vector_value(const flexible_type& val, size_t thr, const std::unordered_set<flexible_type>& keys) {
  increase_nested_element_count(*m_element_sketch, thr, val.size());

  const flex_vec& vec = val.get<flex_vec>();
  for(auto vect_val : vec) {
    m_element_sketch->accumulate_one_value(thr, vect_val);
  }

  // accumulate sub element sketch
  if (!keys.empty()) {
    for(auto index: keys) {
      if (index < vec.size()) {
        m_element_sub_sketch[index]->accumulate_one_value(thr, vec[index]);
      } else {
        m_element_sub_sketch[index]->accumulate_one_value(thr, FLEX_UNDEFINED);
      }
      increase_nested_element_count(*(m_element_sub_sketch[index]), thr, 1);
    }
  }
}

void unity_sketch::accumulate_dict_value(const flexible_type& val, size_t thr, const std::unordered_set<flexible_type>& keys) {
  increase_nested_element_count(*m_dict_key_sketch, thr, val.size());
  increase_nested_element_count(*m_dict_value_sketch, thr, val.size());

  const flex_dict& dict = val.get<flex_dict>();
  std::unordered_map<flexible_type, flexible_type> converted_dict;
  for(auto dict_val : dict) {
    flexible_type key_to_accumulate =
                dict_val.first == FLEX_UNDEFINED ?
                    dict_val.first:
                    flexible_type(flex_string(dict_val.first));

    flexible_type value_to_accumulate;
    if (dict_val.second == FLEX_UNDEFINED) {
      value_to_accumulate = FLEX_UNDEFINED;
    } else {
      if (flex_type_is_convertible(dict_val.second.get_type(), m_dict_value_sketch_type)) {
        flexible_type tmp(m_dict_value_sketch_type);
        tmp.soft_assign(dict_val.second);
        value_to_accumulate = std::move(tmp);
      } else {
        log_and_throw("Cannot convert dictionary value '" + flex_string(dict_val.second) + \
          "'' to type '" + flex_type_enum_to_name(m_dict_value_sketch_type) + "'");
      }
    }

    // remember the converted dictionary value for key accumulation below
    converted_dict.insert(std::pair<flexible_type, flexible_type>(key_to_accumulate, value_to_accumulate));

    m_dict_key_sketch->accumulate_one_value(thr, key_to_accumulate);
    m_dict_value_sketch->accumulate_one_value(thr, value_to_accumulate);
  }

  // accumulate sub element sketch, cannot do it inside loop above because
  // we need to loop through each key and check existence of the key in the
  // dictionary, if not, we treat it as missing value
  if (!keys.empty()) {
    for(auto key: keys) {
      if (converted_dict.find(key) != converted_dict.end()) {
        m_element_sub_sketch[key]->accumulate_one_value(thr, converted_dict[key]);
      } else {
        m_element_sub_sketch[key]->accumulate_one_value(thr, FLEX_UNDEFINED);
      }
      increase_nested_element_count(*(m_element_sub_sketch[key]), thr, 1);
    }
  }
}

// Initialize the sketch object
void unity_sketch::init(
  unity_sketch* parent_sketch,
  flex_type_enum type,
  const std::unordered_set<flexible_type>& keys,
  std::shared_ptr<sarray<flexible_type>::reader_type> reader) {

  if (reader != NULL) {
    m_size = reader->size();
  }

  m_is_child_sketch = parent_sketch != NULL;
  m_stored_type = type;
  m_is_numeric = (m_stored_type == flex_type_enum::INTEGER ||
                m_stored_type == flex_type_enum::FLOAT);

  m_is_list = (m_stored_type == flex_type_enum::DICT ||
                  m_stored_type == flex_type_enum::LIST ||
                  m_stored_type == flex_type_enum::VECTOR);

  reset_global_sketches_and_statistics();

  if (m_is_list) {
    m_element_len_sketch = std::make_shared<unity_sketch>();
    m_element_len_sketch->init(this, flex_type_enum::INTEGER);
  }

  if (m_stored_type == flex_type_enum::LIST) {
    m_element_sketch = std::make_shared<unity_sketch>();
    m_element_sketch->init(this, flex_type_enum::STRING);
  }

  if (m_stored_type == flex_type_enum::VECTOR) {
    m_element_sketch = std::make_shared<unity_sketch>();
    m_element_sketch->init(this, flex_type_enum::FLOAT);

    if (!keys.empty()) {
      for(auto key : keys) {
        m_element_sub_sketch[key] = std::make_shared<unity_sketch>();
        m_element_sub_sketch[key]->init(this, flex_type_enum::FLOAT);
      }
    }
  }

  if (m_stored_type == flex_type_enum::DICT) {
    DASSERT_TRUE(reader != NULL);

    // infer DICT value type, we try to use numeric sketch if possible, otherwise
    // we treat the dict value as str
    m_dict_value_sketch_type = infer_dict_value_type(reader);

    m_dict_key_sketch = std::make_shared<unity_sketch>();
    m_dict_key_sketch->init(this, flex_type_enum::STRING);

    m_dict_value_sketch = std::make_shared<unity_sketch>();
    m_dict_value_sketch->init(this, m_dict_value_sketch_type);

    if (!keys.empty()) {
      for(auto key : keys) {
        m_element_sub_sketch[key] = std::make_shared<unity_sketch>();
        m_element_sub_sketch[key]->init(this, m_dict_value_sketch_type);
      }
    }
  }

  m_thrlocal.resize(thread_pool::get_instance().size());
  m_thrlocks.resize(m_thrlocal.size());
  for (size_t i = 0;i < m_thrlocal.size(); ++i) {
    m_thrlocal[i].discrete_sketch.reset();
    m_thrlocal[i].numeric_sketch.reset();
  }
}

bool unity_sketch::sketch_ready() {
  // For child sketch, we really don't know how many items are there total,
  // so resort to parent for readiness info
  if (m_is_child_sketch) {
    return m_sketch_ready;
  } else {
    return m_rows_processed_by_threads.value == m_size;
  }
}

size_t unity_sketch::num_elements_processed() {
  std::unique_lock<turi::mutex> global_lock(lock);
  return m_rows_processed_by_threads.value;
}

std::map<flexible_type, std::shared_ptr<unity_sketch_base>>
unity_sketch::element_sub_sketch(const std::vector<flexible_type>& keys) {
  if (m_stored_type != flex_type_enum::VECTOR && m_stored_type != flex_type_enum::DICT) {
    log_and_throw("sub_element_sketch is only available for SArray of dictionary or array type");
  }

  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);

  std::unordered_set<flexible_type> key_set(keys.begin(), keys.end());

  std::map<flexible_type, std::shared_ptr<unity_sketch_base>> ret;
  for(auto sub_sketch : m_element_sub_sketch) {
    if (key_set.empty() || key_set.count(sub_sketch.first) > 0) {
      ret[sub_sketch.first].reset(new unity_sketch(sub_sketch.second, this->sketch_ready()));
    }
  }

  return ret;
}

std::shared_ptr<unity_sketch_base> unity_sketch::element_length_summary() {
  if (!m_is_list) log_and_throw("Element length summary is not available for types that are not list/dict/array type.");
  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);
  return std::shared_ptr<unity_sketch>(
      new unity_sketch(m_element_len_sketch, this->sketch_ready()));
}

std::shared_ptr<unity_sketch_base> unity_sketch::element_summary() {
  if (m_stored_type != flex_type_enum::VECTOR && m_stored_type != flex_type_enum::LIST) {
    log_and_throw("Array summary is only available for SArray of array or list type.");
  }

  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);
  return std::shared_ptr<unity_sketch>(
      new unity_sketch(m_element_sketch, this->sketch_ready()));
}

std::shared_ptr<unity_sketch_base> unity_sketch::dict_key_summary() {
  if (m_stored_type != flex_type_enum::DICT) log_and_throw("Dict key summary is only available for SArray of dict type.");

  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);
  return std::shared_ptr<unity_sketch>(
      new unity_sketch(m_dict_key_sketch, this->sketch_ready()));
}

std::shared_ptr<unity_sketch_base> unity_sketch::dict_value_summary() {
  if (m_stored_type != flex_type_enum::DICT) log_and_throw("Dict value summary is only available for SArray of dict type.");
  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);
  return std::shared_ptr<unity_sketch>(
      new unity_sketch(m_dict_value_sketch, this->sketch_ready()));
}

double unity_sketch::get_quantile(double quantile) {
  if (!m_is_numeric) log_and_throw("Quantiles not available for a non-numeric column");
  if (m_size == 0) log_and_throw("Quantiles do not exist for 0 length array");
  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);
  if (m_numeric_sketch.quantiles) {
    return m_numeric_sketch.quantiles->query_quantile(quantile);
  } else {
    log_and_throw("Quantile Sketch not available");
  }
}
double unity_sketch::frequency_count(flexible_type val) {
  if (m_size == 0) return 0.0;

  // Figure out what type do we use for the key value
  flex_type_enum tmpval_type = m_is_list ? flex_type_enum::STRING : m_stored_type;

  if (!flex_type_is_convertible(val.get_type(), tmpval_type)) {
    log_and_throw("Invalid type");
  }

  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);
  if (m_discrete_sketch.count) {
    // try to convert to the original array type for querying
    flexible_type tempval(tmpval_type);
    tempval.soft_assign(val);
    return std::max<double>(0.0, m_discrete_sketch.count->estimate(tempval));
  } else {
    log_and_throw("Count Sketch not available");
  }
}

std::vector<std::pair<flexible_type, size_t> > unity_sketch::frequent_items() {
  if (m_size == 0) return std::vector<std::pair<flexible_type, size_t> >();
  commit_global_if_out_of_date();

  std::unique_lock<turi::mutex> global_lock(lock);
  if (m_discrete_sketch.frequent) {
    auto items = m_discrete_sketch.frequent->frequent_items();
    std::vector<std::pair<flexible_type, size_t> > ret;
    for (auto& item: items) {
      double count = m_discrete_sketch.count->estimate(item.first);
      if (count > 0) {
        item.second = (size_t)count;
        ret.push_back(item);
      }
    }
    return ret;
  } else {
    log_and_throw("Frequent Items Sketch not available");
  }
}
double unity_sketch::num_unique() {
  if (m_size == 0) return 0.0;
  commit_global_if_out_of_date();
  std::unique_lock<turi::mutex> global_lock(lock);
  if (m_discrete_sketch.unique) {
    return m_discrete_sketch.unique->estimate();
  } else {
    log_and_throw("Unique Count Sketch not available");
  }
}

void unity_sketch::reset_global_sketches_and_statistics() {
  m_discrete_sketch.reset();
  m_numeric_sketch.reset();
  if (m_element_len_sketch) {
    m_element_len_sketch->reset_global_sketches_and_statistics();
  }
  if(m_element_sketch) {
    m_element_sketch->reset_global_sketches_and_statistics();
  }
  if (m_element_sketch) {
    m_element_sketch->reset_global_sketches_and_statistics();
  }
  if (m_dict_key_sketch) {
    m_dict_key_sketch->reset_global_sketches_and_statistics();
    m_dict_value_sketch->reset_global_sketches_and_statistics();
  }

  if (!m_element_sub_sketch.empty()) {
    for(auto sub_sketch : m_element_sub_sketch) {
      m_element_sub_sketch[sub_sketch.first]->reset_global_sketches_and_statistics();
    }
  }

  if (m_is_child_sketch) {
    m_size = 0;
  }
  m_undefined_count = 0;
  m_num_elements_processed = 0;
}

void unity_sketch::combine_global(std::vector<turi::mutex>& thr_locks) {
  // acquire lock on the globals and combine with the threads.
  std::unique_lock<turi::mutex> global_lock(lock);
  reset_global_sketches_and_statistics();

  // merge all the sketches
  for (size_t i = 0;i < m_thrlocal.size(); ++i) {
    std::unique_lock<turi::mutex> thrlocal_lock(thr_locks[i]);
    m_num_elements_processed += m_thrlocal[i].num_elements_processed;
    m_discrete_sketch.combine(m_thrlocal[i].discrete_sketch);
    m_undefined_count += m_thrlocal[i].undefined_count;
    if (m_is_numeric) {
      m_numeric_sketch.combine(m_thrlocal[i].numeric_sketch);
    }


  }

  if (m_is_child_sketch) {
    m_size = m_num_elements_processed;
  }

  if (m_is_numeric) {
    m_numeric_sketch.finalize();
  }

  if (m_is_list) {
    m_element_len_sketch->combine_global(thr_locks);
  }

  if (m_stored_type == flex_type_enum::DICT) {
    m_dict_key_sketch->combine_global(thr_locks);
    m_dict_value_sketch->combine_global(thr_locks);
  }

  if (m_element_sketch) {
    m_element_sketch->combine_global(thr_locks);
  }

  if (!m_element_sub_sketch.empty()) {
    for(auto sub_sketch : m_element_sub_sketch) {
      m_element_sub_sketch[sub_sketch.first]->combine_global(thr_locks);
    }
  }
}

void unity_sketch::commit_global_if_out_of_date() {
  // if there are thread sketches we have not seen
  if (m_num_elements_processed < m_rows_processed_by_threads.value &&
      (m_rows_processed_by_threads == m_size || m_commit_timer.current_time() > SKETCH_COMMIT_INTERVAL)) {
    logstream(LOG_INFO) << "num elements: " << m_num_elements_processed << "\t"
                       << "rows_processed: " << m_rows_processed_by_threads.value << "\t"
                       << "time:" << m_commit_timer.current_time() << std::endl;

    combine_global(m_thrlocks);

    // done with all of my allocated rows.
    m_commit_timer.start();

    // if all threads are done, do a join
    if (m_num_elements_processed == m_size && !m_is_child_sketch) {
      m_background_future.wait();
      m_thrlocal.clear();
    }
  }
}

void unity_sketch::cancel() {
  if (m_background_future.valid()) {
    m_cancel = true;
    m_background_future.wait();
  }
}


/**************************************************************************/
/*                                                                        */
/*           unity_sketch::numeric_sketch_struct implementation           */
/*                                                                        */
/**************************************************************************/

void unity_sketch::numeric_sketch_struct::reset() {
  quantiles.reset(new sketches::streaming_quantile_sketch<double>());
  min = std::numeric_limits<double>::max();
  max = std::numeric_limits<double>::lowest();
  sum = 0;
  mean = 0;
  num_items = 0;
  m2 = 0;
}

void unity_sketch::numeric_sketch_struct::combine(const numeric_sketch_struct& other) {
  // make a temp copy of the qunatile we may do finalize multiple times for
  // background sketch, and sketch doesn't like that
  sketches::streaming_quantile_sketch<double> tmp(*(other.quantiles));
  tmp.substream_finalize();

  quantiles->combine(tmp);

  min = std::min(other.min, min);
  max = std::max(other.max, max);
  sum += other.sum;

  if (num_items + other.num_items > 0) {

    double delta = other.mean - mean;
    double tmp1 = (double)num_items/(double)(num_items + other.num_items);
    double tmp2 = (double)other.num_items/(double)(num_items + other.num_items);
    mean = mean * tmp1 + other.mean * tmp2;
    m2 += other.m2 + delta * num_items * delta * other.num_items/(double)(num_items + other.num_items);
    num_items += other.num_items;
  }
}

void unity_sketch::numeric_sketch_struct::accumulate(double dval) {
  if(std::isnan(dval))
    return;

  quantiles->add(dval);

  min = std::min(min, dval);
  max = std::max(max, dval);
  sum += dval;

  num_items++;
  double delta = dval - mean;
  mean += delta / num_items;
  m2 += delta * (dval - mean);
}

void unity_sketch::numeric_sketch_struct::finalize() {
  quantiles->combine_finalize();
}

/**************************************************************************/
/*                                                                        */
/*          unity_sketch::discrete_sketch_struct implementation           */
/*                                                                        */
/**************************************************************************/

void unity_sketch::discrete_sketch_struct::reset() {
  count.reset(new sketches::countsketch<flexible_type>());
  frequent.reset(new sketches::space_saving_flextype());
  unique.reset(new sketches::hyperloglog());
}

void unity_sketch::discrete_sketch_struct::accumulate(const flexible_type& val) {
  if(val.get_type() == flex_type_enum::FLOAT && std::isnan(val.get<flex_float>()))
    return;

  count->add(val);
  frequent->add(val);
  unique->add(val);
}

void unity_sketch::discrete_sketch_struct::combine(const discrete_sketch_struct& other) {
  count->combine(*(other.count));
  frequent->combine(*(other.frequent));
  unique->combine(*(other.unique));
}


}
