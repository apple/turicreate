/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ctime>
#include <parallel/pthread_tools.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <sframe/sarray.hpp>
#include <sframe/sarray_reader.hpp>
#include <sframe/sarray_reader_buffer.hpp>
#include <unity/lib/image_util.hpp>
#include <sframe_query_engine/planning/planner.hpp>

namespace turi {

static turi::mutex reader_shared_ptr_lock;

/**
 * Given an array of flexible_type of mixed type, find the common base type
 * among all of them that I can use to represent the entire array.
 * Fails if no such type exists.
 */
flex_type_enum infer_type_of_list(const std::vector<flexible_type>& vec) {
  std::set<flex_type_enum> types;

  // Since most of the types we encountered are likely to be the same,
  // as an optimization only add new ones to the set and ignore the
  // previous type.
  flex_type_enum last_type = flex_type_enum::UNDEFINED;
  for (const flexible_type& val: vec) {
    if(val.get_type() != last_type && val.get_type() != flex_type_enum::UNDEFINED) {
      types.insert(val.get_type());
      last_type = val.get_type();
    }
  }

  try {
    return get_common_type(types);
  } catch(std::string &e) {
    throw std::string("Cannot infer Array type. Not all elements of array are the same type.");
  }
}

/**
 * Utility function to throw an error if a vector is of unequal length.
 * \param[in] gl_sarray of type vector 
 */
void check_vector_equal_size(const gl_sarray& in) {
  // Initialize. 
  DASSERT_TRUE(in.dtype() == flex_type_enum::VECTOR); 
  size_t n_threads = thread::cpu_count();
  n_threads = std::max(n_threads, size_t(1));
  size_t m_size = in.size();
          
  // Throw the following error. 
  auto throw_error = [] (size_t row_number, size_t expected, size_t current) {
    std::stringstream ss;
    ss << "Vectors must be of the same size. Row " << row_number 
       << " contains a vector of size " << current << ". Expected a vector of"
       << " size " << expected << "." << std::endl;
    log_and_throw(ss.str());
  };
  
  // Within each block of the SArray, check that the vectors have the same size.
  std::vector<size_t> expected_sizes (n_threads, size_t(-1));
  in_parallel([&](size_t thread_idx, size_t n_threads) {
    size_t start_row = thread_idx * m_size / n_threads; 
    size_t end_row = (thread_idx + 1) * m_size / n_threads;
    size_t expected_size = size_t(-1);
    size_t row_number = start_row;
    for (const auto& v: in.range_iterator(start_row, end_row)) {
      if (v != FLEX_UNDEFINED) {
        if (expected_size == size_t(-1)) {
          expected_size = v.size();
          expected_sizes[thread_idx] = expected_size; 
        } else {
          DASSERT_TRUE(v.get_type() == flex_type_enum::VECTOR);
          if (expected_size != v.size()) {
            throw_error(row_number, expected_size, v.size());
          }
        }
      }
      row_number++;
    }
  });

  // Make sure sizes accross blocks are also the same. 
  size_t vector_size = size_t(-1);
  for (size_t thread_idx = 0; thread_idx < n_threads; thread_idx++) {
    // If this block contains all None values, skip it.
    if (expected_sizes[thread_idx] != size_t(-1)) {

      if (vector_size == size_t(-1)) {
          vector_size = expected_sizes[thread_idx]; 
      } else {
         if (expected_sizes[thread_idx] != vector_size) {
           throw_error(thread_idx * m_size / n_threads, 
                              vector_size, expected_sizes[thread_idx]);
         } 
      }
    }
  }
}

/**************************************************************************/
/*                                                                        */
/*                         gl_sarray Constructors                         */
/*                                                                        */
/**************************************************************************/

gl_sarray::gl_sarray() {
  instantiate_new();
}

gl_sarray::gl_sarray(const gl_sarray& other) {
  m_sarray = other.get_proxy();
}
gl_sarray::gl_sarray(gl_sarray&& other) {
  m_sarray = std::move(other.get_proxy());
}

gl_sarray::gl_sarray(const std::string& directory) {
  instantiate_new();
  m_sarray->construct_from_sarray_index(directory);
}

gl_sarray& gl_sarray::operator=(const gl_sarray& other) {
  m_sarray = other.get_proxy();
  return *this;
}

gl_sarray& gl_sarray::operator=(gl_sarray&& other) {
  m_sarray = std::move(other.get_proxy());
  return *this;
}

std::shared_ptr<unity_sarray> gl_sarray::get_proxy() const {
  return m_sarray;
}

gl_sarray::gl_sarray(const std::vector<flexible_type>& values, flex_type_enum dtype) {
  if (dtype == flex_type_enum::UNDEFINED) dtype = infer_type_of_list(values);
  instantiate_new();
  get_proxy()->construct_from_vector(values, dtype);
}

void gl_sarray::construct_from_vector(const std::vector<flexible_type>& values, flex_type_enum dtype) {
  if (dtype == flex_type_enum::UNDEFINED) dtype = infer_type_of_list(values);
  get_proxy()->construct_from_vector(values, dtype);
}

gl_sarray::gl_sarray(const std::initializer_list<flexible_type>& values) {
  flex_type_enum dtype = infer_type_of_list(values);
  instantiate_new();
  get_proxy()->construct_from_vector(values, dtype);
}

gl_sarray gl_sarray::from_const(const flexible_type& value, size_t size) {
  gl_sarray ret;
  ret.get_proxy()->construct_from_const(value, size);
  return ret;
}

gl_sarray gl_sarray::from_sequence(size_t start, size_t end, bool reverse) {
  if (end < start) throw std::string("End must be greater than start");
  return unity_sarray::create_sequential_sarray(end - start, 
                                                start, 
                                                reverse);
}

/**************************************************************************/
/*                                                                        */
/*                   gl_sarray Implicit Type Converters                   */
/*                                                                        */
/**************************************************************************/

gl_sarray::gl_sarray(std::shared_ptr<unity_sarray> sarray) {
  m_sarray = sarray;
}

gl_sarray::gl_sarray(std::shared_ptr<unity_sarray_base> sarray) {
  m_sarray = std::dynamic_pointer_cast<unity_sarray>(sarray);
}

gl_sarray::gl_sarray(std::shared_ptr<sarray<flexible_type> > sa)
    : m_sarray(new unity_sarray)
{
  m_sarray->construct_from_sarray(sa);
}

gl_sarray::operator std::shared_ptr<unity_sarray>() const {
  return get_proxy();
}
gl_sarray::operator std::shared_ptr<unity_sarray_base>() const {
  return get_proxy();
}

std::shared_ptr<sarray<flexible_type> > gl_sarray::materialize_to_sarray() const {
  return get_proxy()->get_underlying_sarray();
}

/**************************************************************************/
/*                                                                        */
/*                      gl_sarray Operator Overloads                      */
/*                                                                        */
/**************************************************************************/

#define DEFINE_OP(OP)\
    gl_sarray gl_sarray::operator OP(const gl_sarray& other) const { \
      return get_proxy()->vector_operator(other.get_proxy(), #OP); \
    } \
    gl_sarray gl_sarray::operator OP(const flexible_type& other) const { \
      return get_proxy()->left_scalar_operator(other, #OP); \
    } \
    gl_sarray operator OP(const flexible_type& opnd, const gl_sarray& opnd2) { \
      return opnd2.get_proxy()->right_scalar_operator(opnd, #OP); \
    }\
    gl_sarray gl_sarray::operator OP ## =(const gl_sarray& other) { \
      (*this) = get_proxy()->vector_operator(other.get_proxy(), #OP); \
      return *this; \
    } \
    gl_sarray gl_sarray::operator OP ## =(const flexible_type& other) { \
      (*this) = get_proxy()->left_scalar_operator(other, #OP); \
      return *this; \
    } 

DEFINE_OP(+)
DEFINE_OP(-)
DEFINE_OP(*)
DEFINE_OP(/)
#undef DEFINE_OP


#define DEFINE_COMPARE_OP(OP) \
    gl_sarray gl_sarray::operator OP(const gl_sarray& other) const { \
      return get_proxy()->vector_operator(other.get_proxy(), #OP); \
    } \
    gl_sarray gl_sarray::operator OP(const flexible_type& other) const { \
      return get_proxy()->left_scalar_operator(other, #OP); \
    } 

DEFINE_COMPARE_OP(<)
DEFINE_COMPARE_OP(>)
DEFINE_COMPARE_OP(<=)
DEFINE_COMPARE_OP(>=)
DEFINE_COMPARE_OP(==)
#undef DEFINE_COMPARE_OP

gl_sarray gl_sarray::operator&(const gl_sarray& other) const {
  return get_proxy()->vector_operator(other.get_proxy(), "&");
} 

gl_sarray gl_sarray::operator|(const gl_sarray& other) const {
  return get_proxy()->vector_operator(other.get_proxy(), "|");
} 

gl_sarray gl_sarray::operator&&(const gl_sarray& other) const {
  return get_proxy()->vector_operator(other.get_proxy(), "&");
}

gl_sarray gl_sarray::operator||(const gl_sarray& other) const {
  return get_proxy()->vector_operator(other.get_proxy(), "|");
}

gl_sarray gl_sarray::contains(const flexible_type& other) const {
  return get_proxy()->left_scalar_operator(other, "in");
}

flexible_type gl_sarray::operator[](int64_t i) const {
  if (i < 0 || (size_t)i >= get_proxy()->size()) {
    throw std::string("Index out of range");
  }
  ensure_has_sarray_reader();
  std::vector<flexible_type> rows(1);
  size_t rows_read  = m_sarray_reader->read_rows(i, i + 1, rows);
  ASSERT_TRUE(rows.size() > 0);
  ASSERT_EQ(rows_read, 1);
  return rows[0];
}


gl_sarray gl_sarray::operator[](const gl_sarray& slice) const {
  return get_proxy()->logical_filter(slice.get_proxy());
}

gl_sarray gl_sarray::operator[](const std::initializer_list<int64_t>& _slice) const {
  std::vector<int64_t> slice(_slice);
  int64_t start = 0, step = 1, stop = 0;
  if (slice.size() == 2) {
    start = slice[0]; stop = slice[1];
  } else if (slice.size() == 3) {
    start = slice[0]; step = slice[1]; stop = slice[2];
  } else {
    throw std::string("Invalid slice. Slice must be of the form {start, end} or {start, step, end}");
  }
  if (start < 0) start = size() + start;
  if (stop < 0) stop = size() + stop;
  return get_proxy()->copy_range(start, step, stop);
}
/**************************************************************************/
/*                                                                        */
/*                               Iterators                                */
/*                                                                        */
/**************************************************************************/
void gl_sarray::materialize_to_callback(
    std::function<bool(size_t, const std::shared_ptr<sframe_rows>&)> callback,
    size_t nthreads) {
  if (nthreads == (size_t)(-1)) nthreads = SFRAME_DEFAULT_NUM_SEGMENTS;
  turi::query_eval::planner().materialize(get_proxy()->get_planner_node(),
                                              callback,
                                              nthreads);
}


gl_sarray_range gl_sarray::range_iterator(size_t start, size_t end) const {
  if (end == (size_t)(-1)) end = get_proxy()->size();
  if (start > end) {
    throw std::string("start must be less than end");
  }
  // basic range check. start must point to existing element, end can point
  // to one past the end. 
  // but additionally, we need to permit the special case start == end == 0
  // so that you can iterate over empty frames.
  if (!((start < get_proxy()->size() && end <= get_proxy()->size()) ||
        (start == 0 && end == 0))) {
    throw std::string("Index out of range");
  }
  ensure_has_sarray_reader();
  return gl_sarray_range(m_sarray_reader, start, end);
}

/**************************************************************************/
/*                                                                        */
/*                          All Other Functions                           */
/*                                                                        */
/**************************************************************************/

void gl_sarray::save(const std::string& directory, const std::string& format) const {
  if (format == "binary") {
    get_proxy()->save_array(directory);
  } else if (format == "text" || format == "csv") {
    gl_sframe sf;
    sf["X1"] = (*this);
    sf.save(directory, "csv");
  } else {
    throw std::string("Unknown format");
  }
}

size_t gl_sarray::size() const {
  return get_proxy()->size();
}
bool gl_sarray::empty() const {
  return size() == 0;
}
flex_type_enum gl_sarray::dtype() const {
  return get_proxy()->dtype();
}
void gl_sarray::materialize() {
  get_proxy()->materialize();
}
bool gl_sarray::is_materialized() const {
  return get_proxy()->is_materialized();
}

gl_sarray gl_sarray::head(size_t n) const {
  return get_proxy()->head(n);
}

gl_sarray gl_sarray::tail(size_t n) const {
  return get_proxy()->tail(n);
}

gl_sarray gl_sarray::count_words(bool to_lower, turi::flex_list delimiters) const {
  return get_proxy()->count_bag_of_words({{"to_lower",to_lower}, {"delimiters",delimiters}});
}
gl_sarray gl_sarray::count_ngrams(size_t n, std::string method, 
                                  bool to_lower, bool ignore_space) const {
  if (method == "word") {
    return get_proxy()->count_ngrams(n, {{"to_lower",to_lower}, 
                                      {"ignore_space",ignore_space}});
  } else if (method == "character") {
    return get_proxy()->count_character_ngrams(n, {{"to_lower",to_lower}, 
                                                {"ignore_space",ignore_space}});
  } else {
    throw std::string("Invalid 'method' input  value. Please input either 'word' or 'character' ");
    __builtin_unreachable();
  }

}
gl_sarray gl_sarray::dict_trim_by_keys(const std::vector<flexible_type>& keys,
                            bool exclude) const {
  return get_proxy()->dict_trim_by_keys(keys, exclude);
}
gl_sarray gl_sarray::dict_trim_by_values(const flexible_type& lower,
                                         const flexible_type& upper) const {
  return get_proxy()->dict_trim_by_values(lower, upper);
}

gl_sarray gl_sarray::dict_keys() const {
  return get_proxy()->dict_keys();
}
gl_sarray gl_sarray::dict_values() const {
  return get_proxy()->dict_values();
}
gl_sarray gl_sarray::dict_has_any_keys(const std::vector<flexible_type>& keys) const {
  return get_proxy()->dict_has_any_keys(keys);
}
gl_sarray gl_sarray::dict_has_all_keys(const std::vector<flexible_type>& keys) const {
  return get_proxy()->dict_has_all_keys(keys);
}

gl_sarray gl_sarray::apply(std::function<flexible_type(const flexible_type&)> fn,
                           flex_type_enum dtype,
                           bool skip_undefined) const {
  return get_proxy()->transform_lambda(fn, dtype, skip_undefined, time(NULL));
}

gl_sarray gl_sarray::filter(std::function<bool(const flexible_type&)> fn,
                            bool skip_undefined) const {
  return (*this)[apply([fn](const flexible_type& arg)->flexible_type {
                         return fn(arg);
                       }, flex_type_enum::INTEGER, skip_undefined)];
}

gl_sarray gl_sarray::sample(double fraction) const {
  return get_proxy()->sample(fraction, time(NULL));
}
gl_sarray gl_sarray::sample(double fraction, size_t seed, bool exact) const {
  return get_proxy()->sample(fraction, seed, exact);
}
bool gl_sarray::all() const {
  return get_proxy()->all();
}
bool gl_sarray::any() const {
  return get_proxy()->any();
}
flexible_type gl_sarray::max() const {
  return get_proxy()->max();
}
flexible_type gl_sarray::min() const {
  return get_proxy()->min();
}
flexible_type gl_sarray::sum() const {
  return get_proxy()->sum();
}
flexible_type gl_sarray::mean() const {
  return get_proxy()->mean();
}
flexible_type gl_sarray::std() const {
  return get_proxy()->std();
}
size_t gl_sarray::nnz() const {
  return get_proxy()->nnz();
}
size_t gl_sarray::num_missing() const {
  return get_proxy()->num_missing();
}

gl_sarray gl_sarray::datetime_to_str(const std::string& str_format) const {
  return get_proxy()->datetime_to_str(str_format);
}
gl_sarray gl_sarray::str_to_datetime(const std::string& str_format) const {
  return get_proxy()->str_to_datetime(str_format);
}
gl_sarray gl_sarray::pixel_array_to_image(size_t width, size_t height, size_t channels,
                                          bool undefined_on_failure) const {
  return image_util:: vector_sarray_to_image_sarray(
      std::dynamic_pointer_cast<unity_sarray>(get_proxy()), 
      width, height, channels, undefined_on_failure);
}

gl_sarray gl_sarray::astype(flex_type_enum dtype, bool undefined_on_failure) const {
  return get_proxy()->astype(dtype, undefined_on_failure);
}
gl_sarray gl_sarray::clip(flexible_type lower, flexible_type upper) const {
  if (lower == FLEX_UNDEFINED) lower = NAN;
  if (upper == FLEX_UNDEFINED) upper = NAN;
  return get_proxy()->clip(lower, upper);
}
gl_sarray gl_sarray::clip_lower(flexible_type threshold) const {
  return get_proxy()->clip(threshold, NAN);
}
gl_sarray gl_sarray::clip_upper(flexible_type threshold) const {
  return get_proxy()->clip(NAN, threshold);
}


gl_sarray gl_sarray::dropna() const {
  return get_proxy()->drop_missing_values();
}
gl_sarray gl_sarray::fillna(flexible_type value) const {
  return get_proxy()->fill_missing_values(value);
}
gl_sarray gl_sarray::topk_index(size_t topk, bool reverse) const {
  return get_proxy()->topk_index(topk, reverse);
}

gl_sarray gl_sarray::append(const gl_sarray& other) const {
  return get_proxy()->append(other.get_proxy());
}

gl_sarray gl_sarray::unique() const {
  gl_sframe sf({{"a",(*this)}});
  sf = sf.groupby({"a"});
  return sf.select_column("a");
}

gl_sarray gl_sarray::item_length() const {
  return get_proxy()->item_length();
}

gl_sframe gl_sarray::split_datetime(const std::string& column_name_prefix,
                                    const std::vector<std::string>& _limit,
                                    bool tzone) const {
  std::vector<std::string> limit = _limit;
  if (tzone && std::find(limit.begin(), limit.end(), "timezone") == limit.end()) {
    limit.push_back("timezone");
  }
  std::map<std::string, flex_type_enum> default_types{
    {"year", flex_type_enum::INTEGER},
    {"month", flex_type_enum::INTEGER},
    {"day", flex_type_enum::INTEGER},
    {"hour", flex_type_enum::INTEGER},
    {"minute", flex_type_enum::INTEGER},
    {"second", flex_type_enum::INTEGER},
    {"timezone", flex_type_enum::FLOAT}};

  std::vector<flex_type_enum> column_types(limit.size());
  for (size_t i = 0;i < limit.size(); ++i) {
    if (default_types.count(limit[i]) == 0) {
      throw std::string("Unrecognized date time limit specifier");
    } else {
      column_types[i] = default_types[limit[i]];
    }
  }
  std::vector<flexible_type> flex_limit(limit.begin(), limit.end());
  return get_proxy()->expand(column_name_prefix, flex_limit, column_types);
}

gl_sframe gl_sarray::unpack(const std::string& column_name_prefix, 
                           const std::vector<flex_type_enum>& _column_types,
                           const flexible_type& na_value, 
                           const std::vector<flexible_type>& _limit) const {
  auto column_types = _column_types;
  auto limit = _limit;
  if (dtype() != flex_type_enum::DICT && dtype() != flex_type_enum::LIST &&
      dtype() != flex_type_enum::VECTOR) {
    throw std::string("Only SArray of dict/list/array type supports unpack");
  }
  if (limit.size() > 0) {
    std::set<flex_type_enum> limit_types;
    for (const flexible_type& l : limit) limit_types.insert(l.get_type());
    if (limit_types.size() != 1) {
      throw std::string("\'limit\' contains values that are different types");
    } 
    if (dtype() != flex_type_enum::DICT && 
        *(limit_types.begin()) != flex_type_enum::INTEGER) {
      throw std::string("\'limit\' must contain integer values.");
    }
    if (std::set<flexible_type>(limit.begin(), limit.end()).size() != limit.size()) {
      throw std::string("\'limit\' contains duplicate values.");
    }
  }

  if (column_types.size() > 0) {
    if (limit.size() > 0) {
      if (limit.size() != column_types.size()) {
        throw std::string("limit and column_types do not have the same length");
      }
    } else if (dtype() == flex_type_enum::DICT) {
      throw std::string("if 'column_types' is given, 'limit' has to be provided to unpack dict type.");
    } else {
      limit.reserve(column_types.size());
      for (size_t i = 0;i < column_types.size(); ++i) limit.push_back(i);
    }
  } else {
    auto head_rows = head(100).dropna();
    std::vector<size_t> lengths(head_rows.size());
    for (size_t i = 0;i < head_rows.size(); ++i) lengths[i] = head_rows[i].size();
    if (lengths.size() == 0 || *std::max_element(lengths.begin(), lengths.end()) == 0) {
      throw std::string("Cannot infer number of items from the SArray, "
                        "SArray may be empty. please explicitly provide column types");
    }
    if (dtype() != flex_type_enum::DICT) {
      size_t length = *std::max_element(lengths.begin(), lengths.end());
      if (limit.size() == 0) {
        limit.resize(length);
        for (size_t i = 0;i < length; ++i) limit[i] = i;
      } else {
        length = limit.size();  
      }

      if (dtype() == flex_type_enum::VECTOR) {
        column_types.resize(length, flex_type_enum::FLOAT);
      } else {
        column_types.clear();
        for(const auto& i : limit) {
          std::vector<flexible_type> f;
          for (size_t j = 0;j < head_rows.size(); ++j) {
            auto x = head_rows[j];
            if (x != flex_type_enum::UNDEFINED && x.size() > i) {
              f.push_back(x.array_at(i));
            }
          }
          column_types.push_back(infer_type_of_list(f));
        }
      }

    }
  }
  if (dtype() == flex_type_enum::DICT && column_types.size() == 0) {
    return get_proxy()->unpack_dict(column_name_prefix,
                                 limit,
                                 na_value);
  } else {
    return get_proxy()->unpack(column_name_prefix,
                            limit,
                            column_types,
                            na_value);
  } 
}


gl_sarray gl_sarray::sort(bool ascending) const {
  gl_sframe sf({{"a",(*this)}});
  sf = sf.sort("a", ascending);
  return sf.select_column("a");
}

gl_sarray gl_sarray::subslice(flexible_type start, 
                              flexible_type stop, 
                              flexible_type step) {
  auto dt = dtype();
  if (dt != flex_type_enum::STRING && 
      dt != flex_type_enum::VECTOR &&
      dt != flex_type_enum::LIST) {
    log_and_throw("SArray must contain strings, arrays or lists");
  }
  return get_proxy()->subslice(start, step, stop);
}

gl_sarray gl_sarray::builtin_rolling_apply(const std::string &fn_name,
                                           ssize_t start,
                                           ssize_t end,
                                           size_t min_observations) const {
  return get_proxy()->builtin_rolling_apply(fn_name, start, end, min_observations);
}


void gl_sarray::show(const std::string& path_to_client,
                     const std::string& title,
                     const std::string& xlabel,
                     const std::string& ylabel) const {
  get_proxy()->show(path_to_client, title, xlabel, ylabel);
}

gl_sarray gl_sarray::cumulative_aggregate(
     std::shared_ptr<group_aggregate_value> aggregator) const { 
  
  flex_type_enum input_type = this->dtype();
  flex_type_enum output_type = aggregator->set_input_types({input_type});
  if (! aggregator->support_type(input_type)) {
    std::stringstream ss;
    ss << "Cannot perform this operation on an SArray of type "
       << flex_type_enum_to_name(input_type) << "." << std::endl;
    log_and_throw(ss.str());
  } 

  // Empty case.  
  size_t m_size = this->size();
  if (m_size == 0) {
    return gl_sarray({}, output_type);
  }
  
  // Make a copy of an newly initialize aggregate for each thread.
  size_t n_threads = thread::cpu_count();
  gl_sarray_writer writer(output_type, n_threads);
  std::vector<std::shared_ptr<group_aggregate_value>> aggregators;
  for (size_t i = 0; i < n_threads; i++) {
      aggregators.push_back(
          std::shared_ptr<group_aggregate_value>(aggregator->new_instance()));
  } 

  // Skip Phases 1,2 when single threaded or more threads than rows.
  if ((n_threads > 1) && (m_size > n_threads)) {
    
    // Phase 1: Compute prefix-sums for each block.
    in_parallel([&](size_t thread_idx, size_t n_threads) {
      size_t start_row = thread_idx * m_size / n_threads; 
      size_t end_row = (thread_idx + 1) * m_size / n_threads;
      for (const auto& v: this->range_iterator(start_row, end_row)) {
        DASSERT_TRUE(thread_idx < aggregators.size());
        if (v != FLEX_UNDEFINED) {
          aggregators[thread_idx]->add_element_simple(v);
        }
      }
    });

    // Phase 2: Combine prefix-sum(s) at the end of each block.
    for (size_t i = n_threads - 1; i > 0; i--) {
      for (size_t j = 0; j < i; j++) {
        DASSERT_TRUE(i < aggregators.size());
        DASSERT_TRUE(j < aggregators.size());
        aggregators[i]->combine(*aggregators[j]);
      }
    }
  }
  
  // Phase 3: Reaggregate with an re-intialized prefix-sum from previous blocks. 
  auto reagg_fn = [&](size_t thread_idx, size_t n_threads) {
    flexible_type y = FLEX_UNDEFINED;
    size_t start_row = thread_idx * m_size / n_threads; 
    size_t end_row = (thread_idx + 1) * m_size / n_threads;
    std::shared_ptr<group_aggregate_value> re_aggregator (
                                              aggregator->new_instance());
  
    // Initialize with the merged value. 
    if (thread_idx >= 1) {
      DASSERT_TRUE(thread_idx - 1 < aggregators.size());
      y = aggregators[thread_idx - 1]->emit();
      re_aggregator->combine(*aggregators[thread_idx - 1]);
    }

    // Write prefix-sum
    for (const auto& v: this->range_iterator(start_row, end_row)) {
      if (v != FLEX_UNDEFINED) {
        re_aggregator->add_element_simple(v);
        y = re_aggregator->emit();
      }
      writer.write(y, thread_idx);
    }
  };
  
  // Run single threaded if more threads than rows. 
  if (m_size > n_threads) {
    in_parallel(reagg_fn);
  } else {
    reagg_fn(0, 1);   
  }
  return writer.close();
}

gl_sarray gl_sarray::builtin_cumulative_aggregate(const std::string& name) const {
  flex_type_enum input_type = this->dtype();
  std::shared_ptr<group_aggregate_value> aggregator;

  // Cumulative sum, and avg support vector types.
  if (name == "__builtin__cum_sum__") {
    switch(input_type) {
      case flex_type_enum::VECTOR: {
        check_vector_equal_size(*this);
        aggregator = get_builtin_group_aggregator(std::string("__builtin__vector__sum__")); 
        break;
      }
      default:
        aggregator = get_builtin_group_aggregator(std::string("__builtin__sum__")); 
        break;
    }
  } else if (name == "__builtin__cum_avg__") {
    switch(input_type) {
      case flex_type_enum::VECTOR: {
        check_vector_equal_size(*this);
        aggregator = get_builtin_group_aggregator(std::string("__builtin__vector__avg__")); 
        break;
      }
      default:
        aggregator = get_builtin_group_aggregator(std::string("__builtin__avg__")); 
        break;
    }
  } else if (name == "__builtin__cum_max__") {
      aggregator = get_builtin_group_aggregator(std::string("__builtin__max__")); 
  } else if (name == "__builtin__cum_min__") {
      aggregator = get_builtin_group_aggregator(std::string("__builtin__min__")); 
  } else if (name == "__builtin__cum_var__") {
      aggregator = get_builtin_group_aggregator(std::string("__builtin__var__")); 
  } else if (name == "__builtin__cum_std__") {
      aggregator = get_builtin_group_aggregator(std::string("__builtin__stdv__")); 
  } else {
    log_and_throw("Internal error. Unknown cumulative aggregator " + name);
  }
  return this->cumulative_aggregate(aggregator);
}

gl_sarray gl_sarray::cumulative_sum() const {
  return builtin_cumulative_aggregate("__builtin__cum_sum__");
}
gl_sarray gl_sarray::cumulative_min() const {
  return builtin_cumulative_aggregate("__builtin__cum_min__");
}
gl_sarray gl_sarray::cumulative_max() const {
  return builtin_cumulative_aggregate("__builtin__cum_max__");
}
gl_sarray gl_sarray::cumulative_avg() const {
  return builtin_cumulative_aggregate("__builtin__cum_avg__");
}
gl_sarray gl_sarray::cumulative_std() const {
  return builtin_cumulative_aggregate("__builtin__cum_std__");
}
gl_sarray gl_sarray::cumulative_var() const {
  return builtin_cumulative_aggregate("__builtin__cum_var__");
}

std::ostream& operator<<(std::ostream& out, const gl_sarray& other) {
  auto t = other.head(10);
  auto dtype = other.dtype();
  out << "dtype: " << flex_type_enum_to_name(dtype) << "\n";
  out << "Rows: " << other.size() << "\n";
  out << "[";
  bool first = true;
  for(auto i : t.range_iterator()) {
    if (!first) out << ",";
    if (dtype == flex_type_enum::STRING) out << "\"";
    if (i.get_type() == flex_type_enum::UNDEFINED) out << "None";
    else out << i;
    if (dtype == flex_type_enum::STRING) out << "\"";
    first = false;
  }
  out << "]" << "\n";
  return out;
}

void gl_sarray::instantiate_new() {
  m_sarray = std::make_shared<unity_sarray>();
}

void gl_sarray::ensure_has_sarray_reader() const {
  if (!m_sarray_reader) {
    std::lock_guard<mutex> guard(reader_shared_ptr_lock);
    if (!m_sarray_reader) {
      m_sarray_reader = 
          std::move(get_proxy()->get_underlying_sarray()->get_reader());
    }
  }
}

/**************************************************************************/
/*                                                                        */
/*                            gl_sarray_range                             */
/*                                                                        */
/**************************************************************************/

gl_sarray_range::gl_sarray_range(
    std::shared_ptr<sarray_reader<flexible_type> > m_sarray_reader,
    size_t start, size_t end) {
  m_sarray_reader_buffer = 
      std::make_shared<sarray_reader_buffer<flexible_type>>
          (m_sarray_reader, start, end);
  // load the first value if available
  if (m_sarray_reader_buffer->has_next()) {
    m_current_value = std::move(m_sarray_reader_buffer->next());
  }
}
gl_sarray_range::iterator gl_sarray_range::begin() {
  return iterator(*this, true);
}
gl_sarray_range::iterator gl_sarray_range::end() {
  return iterator(*this, false);
}

/**************************************************************************/
/*                                                                        */
/*                       gl_sarray_range::iterator                        */
/*                                                                        */
/**************************************************************************/

gl_sarray_range::iterator::iterator(gl_sarray_range& range, bool is_start) {
  m_owner = &range;
  if (is_start) m_counter = 0;
  else m_counter = range.m_sarray_reader_buffer->size();
}

void gl_sarray_range::iterator::increment() {
  ++m_counter;
  if (m_owner->m_sarray_reader_buffer->has_next()) {
    m_owner->m_current_value = std::move(m_owner->m_sarray_reader_buffer->next());
  }
}
void gl_sarray_range::iterator::advance(size_t n) {
  n = std::min(n, m_owner->m_sarray_reader_buffer->size());
  for (size_t i = 0;i < n ; ++i) increment();
}

const gl_sarray_range::type& gl_sarray_range::iterator::dereference() const {
  return m_owner->m_current_value;
}

/**************************************************************************/
/*                                                                        */
/*                         gl_sarray_writer_impl                          */
/*                                                                        */
/**************************************************************************/

class gl_sarray_writer_impl {
 public:
  gl_sarray_writer_impl(flex_type_enum type, size_t num_segments);
  void write(const flexible_type& f, size_t segmentid);
  size_t num_segments() const;
  gl_sarray close();
 private:
  std::shared_ptr<sarray<flexible_type> > m_out_sarray;
  std::vector<sarray<flexible_type>::iterator> m_output_iterators;
};

gl_sarray_writer_impl::gl_sarray_writer_impl(flex_type_enum type, size_t num_segments) {
  // open the output array
  if (num_segments == (size_t)(-1)) num_segments = SFRAME_DEFAULT_NUM_SEGMENTS;
  m_out_sarray = std::make_shared<sarray<flexible_type>>();
  m_out_sarray->open_for_write(num_segments);
  m_out_sarray->set_type(type);

  // store the iterators
  m_output_iterators.resize(m_out_sarray->num_segments());
  for (size_t i = 0;i < m_out_sarray->num_segments(); ++i) {
    m_output_iterators[i] = m_out_sarray->get_output_iterator(i);
  }
}

void gl_sarray_writer_impl::write(const flexible_type& f, size_t segmentid) {
  ASSERT_LT(segmentid, m_output_iterators.size());
  *(m_output_iterators[segmentid]) = f;
}

size_t gl_sarray_writer_impl::num_segments() const {
  return m_output_iterators.size();
}

gl_sarray gl_sarray_writer_impl::close() {
  m_output_iterators.clear();
  m_out_sarray->close();
  auto usarray = std::make_shared<unity_sarray>();
  usarray->construct_from_sarray(m_out_sarray);
  return usarray;
}

/**************************************************************************/
/*                                                                        */
/*                            gl_sarray_writer                            */
/*                                                                        */
/**************************************************************************/

gl_sarray_writer::gl_sarray_writer(flex_type_enum type, 
                                   size_t num_segments) {
  // create the pimpl
  m_writer_impl.reset(new gl_sarray_writer_impl(type, num_segments));
}

void gl_sarray_writer::write(const flexible_type& f, 
                             size_t segmentid) {
  m_writer_impl->write(f, segmentid);
}

size_t gl_sarray_writer::num_segments() const {
  return m_writer_impl->num_segments();
}
gl_sarray gl_sarray_writer::close() {
  return m_writer_impl->close();
}


gl_sarray_writer::~gl_sarray_writer() { }
} // namespace turi

