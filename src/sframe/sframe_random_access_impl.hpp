/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_RANDOM_ACCESS_IMPL_H_
#define TURI_SFRAME_RANDOM_ACCESS_IMPL_H_

#include <sframe/sframe_random_access.hpp>
#include <sframe/sframe_random_access_buffers_impl.hpp>

using std::function;

namespace turi { namespace sframe_random_access {

/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_random_access SFrame Random Access Backend
 * \{
 */

template<typename T>
parallel_hash_map<T>::parallel_hash_map()
  : maps_(turi::thread_pool::get_instance().size()) { }

template<typename T>
void parallel_hash_map<T>::put(uint128_t k, const T& v) {
  maps_[static_cast<int64_t>(k / get_hash_chunk_size())] = v;
}

template<typename T>
T& parallel_hash_map<T>::operator[](uint128_t k) {
  return maps_[static_cast<int64_t>(k / get_hash_chunk_size())][k];
}

template<typename T>
typename unordered_map<uint128_t, T>::iterator parallel_hash_map<T>::find(uint128_t k) {
  return maps_[static_cast<int64_t>(k / get_hash_chunk_size())].find(k);
}

template<typename T>
typename unordered_map<uint128_t, T>::iterator parallel_hash_map<T>::end(uint128_t k) {
  return maps_[static_cast<int64_t>(k / get_hash_chunk_size())].end();
}

template<typename T>
int64_t parallel_hash_map<T>::count(uint128_t k) {
  return maps_[static_cast<int64_t>(k / get_hash_chunk_size())].count(k);
}

template<typename T>
void parallel_hash_map<T>::clear() {
  for (auto& m : maps_) {
    m.clear();
  }
}

/**
 * Enumerates the possible scalar operations over numeric values. Currently only
 * comparison operators, integer and floating-point addition are supported, but
 * many others can be added in the future.
 */
enum class scalar_builtin_enum {
  EQ,
  NE,
  LT,
  LE,
  GT,
  GE,
  ADD,
};

/**
 * Returns the arity (number of arguments) of a scalar operation.
 */
inline int64_t arity(scalar_builtin_enum x) {
  switch (x) {
  case scalar_builtin_enum::EQ:
  case scalar_builtin_enum::NE:
  case scalar_builtin_enum::LT:
  case scalar_builtin_enum::LE:
  case scalar_builtin_enum::GT:
  case scalar_builtin_enum::GE:
  case scalar_builtin_enum::ADD:
    return 2;
  default:
    AU();
  }
}

/**
 * Returns the resulting dtype of a scalar operation, given its input dtypes.
 */
inline dtype_enum get_result_dtype(
  scalar_builtin_enum x, dtype_enum input_dtype) {

  switch (x) {
  case scalar_builtin_enum::EQ:
  case scalar_builtin_enum::NE:
  case scalar_builtin_enum::LT:
  case scalar_builtin_enum::LE:
  case scalar_builtin_enum::GT:
  case scalar_builtin_enum::GE:
    return dtype_enum::BOOL;
  case scalar_builtin_enum::ADD:
    return input_dtype;
  default:
    AU();
  }
}

enum class value_type_tag_enum {
  DATA_TABLE,
  OPTIONAL,
  STRING,
  DATETIME,
  IMAGE,
  IMAGE_DATA,
};

SERIALIZE_POD(value_type_tag_enum);

/**
 * Enumerates the possible cases of the \ref value_type union.
 */
enum class value_type_enum {
  /**
   * Column type.
   */
  COLUMN,
  /**
   * Multidimensional array type.
   */
  ND_VECTOR,
  /**
   * Record type (e.g., table),
   */
  RECORD,
  /**
   * Variant type.
   */
  EITHER,
  /**
   * Function type.
   */
  FUNCTION,
  /**
   * Index type (internal use only).
   */
  INDEX,
};

SERIALIZE_POD(value_type_enum);

inline ostream& operator<<(ostream& os, value_type_enum v) {
  switch (v) {
  case value_type_enum::COLUMN: os << "COLUMN"; break;
  case value_type_enum::ND_VECTOR: os << "ND_VECTOR"; break;
  case value_type_enum::RECORD: os << "RECORD"; break;
  case value_type_enum::EITHER: os << "EITHER"; break;
  case value_type_enum::FUNCTION: os << "FUNCTION"; break;
  case value_type_enum::INDEX: os << "INDEX"; break;
  default:
    AU();
  }
  return os;
}

inline ostream& operator<<(ostream& os, value_type_tag_enum v) {
  switch (v) {
  case value_type_tag_enum::DATA_TABLE: os << "DATA_TABLE"; break;
  case value_type_tag_enum::OPTIONAL: os << "OPTIONAL"; break;
  case value_type_tag_enum::STRING: os << "STRING"; break;
  case value_type_tag_enum::DATETIME: os << "DATETIME"; break;
  case value_type_tag_enum::IMAGE: os << "IMAGE"; break;
  case value_type_tag_enum::IMAGE_DATA: os << "IMAGE_DATA"; break;
  default:
    AU();
  }
  return os;
}

SERIALIZE_POD(index_mode_enum);
SERIALIZE_POD(index_lookup_mode_enum);

DECL_STRUCT(value_type);

DECL_STRUCT(value_type_column);
DECL_STRUCT(value_type_nd_vector);
DECL_STRUCT(value_type_record);
DECL_STRUCT(value_type_either);
DECL_STRUCT(value_type_function);
DECL_STRUCT(value_type_index);

typedef typename boost::make_recursive_variant<
  value_type_column_p,
  value_type_nd_vector_p,
  value_type_record_p,
  value_type_either_p,
  value_type_function_p,
  value_type_index_p
>::type value_type_v;

/**
 * Represents the type of a random-access SFrame \ref value object.  The \ref
 * value_type struct is a tagged union of several possible cases. For details,
 * see \ref value_type_enum. Note that for convenience, types may optionally
 * also be annotated with a user-friendly tag (\ref value_type_tag_enum).
 */
struct value_type : public enable_shared_from_this<value_type> {
  static const char* object_id_;

  value_type_v v_;
  optional<value_type_tag_enum> tag_;

  bool known_direct_;

  value_type(value_type_v v, optional<value_type_tag_enum> tag)
    : v_(v), tag_(tag) {
    known_direct_ = (this->which() == value_type_enum::ND_VECTOR);
  }

  template<typename T> static value_type_p create(
    const T& t, optional<value_type_tag_enum> tag) {

    return make_shared<value_type>(value_type_v(t), tag);
  }

  template<typename T> static value_type_p create(const T& t) {
    return value_type::create(t, NONE<value_type_tag_enum>());
  }

  static value_type_p create_nd_vector(int64_t ndim, dtype_enum dtype);
  static value_type_p create_column(
    value_type_p element_type, optional<int64_t> length, bool known_unique);
  static value_type_p create_bool_column();
  static value_type_p create_string();
  static value_type_p create_image();
  static value_type_p create_scalar(dtype_enum dtype);
  static value_type_p create_record(
    vector<pair<string, value_type_p>> field_types);
  static value_type_p create_data_table(
    vector<pair<string, value_type_p>> field_types);
  static value_type_p create_empty_record();
  static value_type_p create_optional(value_type_p some_ty);
  static value_type_p create_function(value_type_p left, value_type_p right);
  static value_type_p create_index(
    vector<value_type_p> source_column_types, index_mode_enum index_mode);

  value_type_enum which();
  string which_str();

  template<typename T> T& as() {
    return vget<value_type_enum, T>(v_);
  }

  template<typename T> const T& as() const {
    return vget<value_type_enum, T>(v_);
  }

  optional<value_type_p> unpack_optional_ext();
  bool is_optional();
  value_type_p unpack_optional();

  vector<pair<string, value_type_p>> as_record_items() const;
  pair<int64_t, dtype_enum> as_nd_vector_items() const;

  string struct_hash();
};

bool struct_eq(value_type_p x, value_type_p y);
bool type_valid(value_type_p target, value_type_p sub);
void assert_type_valid(value_type_p target, value_type_p sub);

SERIALIZE_DECL(value_type_p);

struct value_type_nd_vector {
  int64_t ndim_;
  dtype_enum dtype_;
  value_type_nd_vector(int64_t ndim, dtype_enum dtype)
    : ndim_(ndim), dtype_(dtype) { }
  static inline value_type_nd_vector_p create(int64_t ndim, dtype_enum dtype) {
    return make_shared<value_type_nd_vector>(ndim, dtype);
  }

  inline void save_sub(ostream& os) const {
    write_bin(os, ndim_);
    write_bin(os, dtype_);
  }

  inline static value_type_nd_vector_p load_sub(istream& is) {
    auto ndim = read_bin<int64_t>(is);
    auto dtype = read_bin<dtype_enum>(is);
    return value_type_nd_vector::create(ndim, dtype);
  }
};

struct value_type_column {
  value_type_p element_type_;
  optional<int64_t> length_;
  bool known_unique_;

  value_type_column(
    value_type_p element_type, optional<int64_t> length, bool known_unique)
    : element_type_(element_type), length_(length),
      known_unique_(known_unique) { }

  inline void save_sub(ostream& os) const {
    write_bin(os, element_type_);
    write_bin(os, length_);
    write_bin(os, known_unique_);
  }

  inline static shared_ptr<value_type_column> load_sub(istream& is) {
    auto element_type = read_bin<value_type_p>(is);
    auto length = read_bin<optional<int64_t>>(is);
    auto known_unique = read_bin<bool>(is);
    return make_shared<value_type_column>(element_type, length, known_unique);
  }
};

struct value_type_record {
  vector<pair<string, value_type_p>> field_types_;
  value_type_record(vector<pair<string, value_type_p>> field_types)
    : field_types_(field_types) { }

  inline void save_sub(ostream& os) const {
    write_bin(os, field_types_);
  }

  inline static shared_ptr<value_type_record> load_sub(istream& is) {
    auto field_types = read_bin<vector<pair<string, value_type_p>>>(is);
    return make_shared<value_type_record>(field_types);
  }
};

struct value_type_function {
  value_type_p left_;
  value_type_p right_;
  value_type_function(value_type_p left, value_type_p right)
    : left_(left), right_(right) { }

  inline void save_sub(ostream& os) const {
    write_bin(os, left_);
    write_bin(os, right_);
  }

  inline static shared_ptr<value_type_function> load_sub(istream& is) {
    auto left = read_bin<value_type_p>(is);
    auto right = read_bin<value_type_p>(is);
    return make_shared<value_type_function>(left, right);
  }
};

struct value_type_index {
  vector<value_type_p> source_column_types_;
  index_mode_enum index_mode_;
  value_type_index(
    vector<value_type_p> source_column_types, index_mode_enum index_mode)
    : source_column_types_(source_column_types), index_mode_(index_mode) { }

  inline void save_sub(ostream& os) const {
    write_bin(os, source_column_types_);
    write_bin(os, index_mode_);
  }

  inline static shared_ptr<value_type_index> load_sub(istream& is) {
    auto source_column_types = read_bin<vector<value_type_p>>(is);
    auto index_mode = read_bin<index_mode_enum>(is);
    return make_shared<value_type_index>(source_column_types, index_mode);
  }
};

struct value_type_either {
  vector<pair<string, value_type_p>> case_types_;
  value_type_either(vector<pair<string, value_type_p>> case_types)
    : case_types_(case_types) { }

  inline void save_sub(ostream& os) const {
    write_bin(os, case_types_);
  }

  inline static shared_ptr<value_type_either> load_sub(istream& is) {
    auto case_types = read_bin<vector<pair<string, value_type_p>>>(is);
    return make_shared<value_type_either>(case_types);
  }
};

value_type_column_p value_type_column_create(
  value_type_p element_type, optional<int64_t> length, bool is_unique);

inline value_type_p value_type_table_create(
  vector<string> column_names,
  vector<value_type_p> column_element_types) {

  vector<pair<string, value_type_p>> ret;

  ASSERT_EQ(len(column_names), len(column_element_types));
  for (int64_t i = 0; i < len(column_names); i++) {
    ret.push_back(
      make_pair(
        column_names[i],
        value_type::create_column(
          column_element_types[i], NONE<int64_t>(), false)));
  }

  auto ret_record = value_type_v(make_shared<value_type_record>(ret));
  return make_shared<value_type>(
    ret_record, SOME(value_type_tag_enum::DATA_TABLE));
}

inline value_type_column_p value_type_column_create(
  value_type_p element_type, optional<int64_t> length, bool known_unique) {

  return make_shared<value_type_column>(element_type, length, known_unique);
}

ostream& operator<<(ostream& os, value_type_p x);

inline string to_string(value_type_p x) {
  ostringstream os;
  os << x;
  return os.str();
}

inline value_type_p value_type_parse(const string& src) {
  if (ends_with(src, "?")) {
    value_type_p some_ty = value_type_parse(src.substr(0, src.length() - 1));
    return value_type::create_optional(some_ty);
  }

  if (src == "str") {
    return value_type::create_string();
  } else if (src == "image") {
    return value_type::create_image();
  } else if (src == "int") {
    return value_type::create_scalar(dtype_enum::I64);
  } else {
    cerr << "value_type_parse: Not yet supported: " << src << endl;
    AU();
    return nullptr;
  }
}

value_type_p value_type_create_nd_vector(int64_t ndim, const string& dtype_str);

inline value_p value::create_empty_record() {
  auto ret_ty = value_type::create_empty_record();
  return value::create(
    make_shared<value_record>(ret_ty, vector<value_p>()),
    ret_ty,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

inline value_p value::create_optional_none(value_type_p ty) {
  ASSERT_TRUE(ty->is_optional());
  return value::create(
    make_shared<value_either>(ty, int64_t(0), value::create_empty_record()),
    ty,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

inline value_p value::create_optional_some(value_type_p ty, value_p v) {
  ASSERT_TRUE(ty->is_optional());
  return value::create(
    make_shared<value_either>(ty, int64_t(1), v),
    ty,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

inline value_type_p value::get_type() {
  return ty_;
}

template<typename T> value_p value::create(
  T v, value_type_p ty, optional<ref_context_p> accum_refs,
  optional<url_p> url_context, optional<int64_t> id) {

  return make_shared<value>(value_v(v), ty, accum_refs, url_context, id);
}

inline value_enum value::which() {
  return static_cast<value_enum>(v_.which());
}

template<typename T> T& value::as() {
  return vget<value_enum, T>(v_);
}

template<typename T> const T& value::as() const {
  return vget<value_enum, T>(v_);
}

/**
 * If a given value is an indirect reference, follow the reference to obtain the
 * actual value.
 */
value_p value_deref(value_p);

/**
 * If a given value is really a column (e.g., is not an indirect reference),
 * returns a raw pointer to that column. This is useful as an optimization to
 * avoid repeatedly testing whether a value is a column in an inner loop.
 */
inline optional<value_column*> value::get_as_direct_column() {
  ASSERT_EQ(ty_->which(), value_type_enum::COLUMN);
  auto self = value_deref(shared_from_this());
  if (self->ty_->as<value_type_column_p>()->element_type_->known_direct_ &&
      self->which() == value_enum::COLUMN) {
    return SOME(self->as<value_column_p>().get());
  } else {
    return NONE<value_column*>();
  }
}

DECL_STRUCT(url);

/**
 * Represents a URL (path on disk). As an optimization, we also assign a unique
 * numeric ID to each URL created.
 */
struct url {
  string url_path_;
  int64_t url_id_;

  static url_p by_path(string url_path);

  // Note: internal use only
  url(string url_path) : url_path_(url_path) { }

  static recursive_mutex& get_url_id_map_lock();

  // Note: should only access while holding lock
  static unordered_map<int64_t, weak_ptr<url>>& get_url_id_map();
  static std::atomic<int64_t>& get_next_url_id();
};

/**
 * Indirect references may be local (to another value in memory), or point to an
 * SFrame on disk by its path.
 */
enum class value_ref_location_enum {
  LOCAL,
  SFRAME_URL,
};

SERIALIZE_POD(value_ref_location_enum);

DECL_STRUCT(value_ref_location);

DECL_STRUCT(value_ref_location_local);
DECL_STRUCT(value_ref_location_sframe_url);

typedef typename boost::make_recursive_variant<
  value_ref_location_local_p,
  value_ref_location_sframe_url_p
>::type value_ref_location_v;

struct value_ref_location_local {
  int64_t id_;
  value_ref_location_local(int64_t id) : id_(id) { }
};

struct value_ref_location_sframe_url {
  string url_path_;
  value_ref_location_sframe_url(string url_path) : url_path_(url_path) { }
};

struct value_ref_location {
  value_ref_location_v v_;

  template<typename T> static value_ref_location_p create(T v) {
    return make_shared<value_ref_location>(value_ref_location_v(v));
  }

  static value_ref_location_p create_local(int64_t id) {
    return value_ref_location::create(
      make_shared<value_ref_location_local>(id));
  }

  static value_ref_location_p create_sframe_url(string url_path) {
    return value_ref_location::create(
      make_shared<value_ref_location_sframe_url>(url_path));
  }

  inline value_ref_location_enum which() {
    return static_cast<value_ref_location_enum>(v_.which());
  }

  template<typename T> T& as() {
    return vget<value_ref_location_enum, T>(v_);
  }

  template<typename T> const T& as() const {
    return vget<value_ref_location_enum, T>(v_);
  }

  value_ref_location(value_ref_location_v v) : v_(v) { }
};

/**
 * Indirect references can either refer to some value as a whole, or to a given
 * element, contiguous range, or subset of a column value.
 */
enum class value_ref_enum {
  VALUE,
  COLUMN_ELEMENT,
  COLUMN_RANGE,
  COLUMN_SUBSET,
};


SERIALIZE_POD(value_ref_enum);

struct value_ref;
using value_ref_p = shared_ptr<value_ref>;

DECL_STRUCT(value_column);
DECL_STRUCT(value_nd_vector);
DECL_STRUCT(value_record);
DECL_STRUCT(value_either);
DECL_STRUCT(value_ref);
DECL_STRUCT(value_index);
DECL_STRUCT(value_thunk);

SERIALIZE_POD(value_enum);

ostream& operator<<(ostream& os, value_enum x);
ostream& operator<<(ostream& os, value_ref_enum x);

typedef typename boost::make_recursive_variant<
  value_column_p,
  value_nd_vector_p,
  value_record_p,
  value_either_p,
  value_ref_p,
  value_index_p,
  value_thunk_p
>::type value_v;

DECL_STRUCT(value);
DECL_STRUCT(query);
value_type_p get_type(query_p x);

DECL_STRUCT(ref_context);

struct ref_context : public enable_shared_from_this<ref_context> {
  void enroll_ref_target(value_p target);
  static ref_context_p create();

  recursive_mutex ref_targets_lock_;

  // Note: should only access while holding lock
  vector<value_p> ref_targets_;
};

SERIALIZE_POD(column_reduce_op_enum);

value_p reduce_op_init(
  column_reduce_op_enum reduce_op, value_type_p result_type);
value_p reduce_op_exec(
  column_reduce_op_enum reduce_op, value_p lhs, value_p rhs);

ostream& operator<<(ostream& os, column_reduce_op_enum reduce_op);

column_reduce_op_enum reduce_op_enum_from_string(string x);

ostream& operator<<(ostream& os, group_by_spec_enum x);

struct group_by_spec_original_table { };

struct group_by_spec_reduce {
  column_reduce_op_enum reduce_op_;
  query_p source_column_;

  group_by_spec_reduce(
    column_reduce_op_enum reduce_op, query_p source_column)
    : reduce_op_(reduce_op), source_column_(source_column) { }
};

struct group_by_spec_select_one {
  query_p source_column_;

  group_by_spec_select_one(query_p source_column)
    : source_column_(source_column) { }
};

inline group_by_spec_enum group_by_spec::which() {
  return static_cast<group_by_spec_enum>(v_.which());
}

template<typename T> T& group_by_spec::as() {
  return vget<group_by_spec_enum, T>(v_);
}

template<typename T> const T& group_by_spec::as() const {
  return vget<group_by_spec_enum, T>(v_);
}

constexpr int64_t COLUMN_TABLE_ENTRY_SIZE_BYTES = 3 * sizeof(int64_t);

DECL_STRUCT(column_view_variable);

enum class column_format_enum {
  VARIABLE,
};

ostream& operator<<(ostream& os, column_format_enum x);

SERIALIZE_POD(column_format_enum);

typedef typename boost::make_recursive_variant<
  column_view_variable_p
>::type column_view_v;

struct column_metadata {
  static const char* object_id_;
};

/**
 * Builder for a random-access SFrame column, analogous to the standard SArray
 * builder. To write serially, use the \ref column_builder::append function; to
 * write in parallel, first resize via \ref column_builder::extend_length_raw,
 * then use the \ref column_builder::put function. As with a standard SArray,
 * call \ref column_builder::finalize to obtain the resulting fully-serialized
 * column value.
 */
struct column_builder {
  value_type_p entry_type_;

  shared_ptr<binary_data_builder_fixed> top_acc_;
  shared_ptr<binary_data_builder_variable> entries_acc_;

  int64_t num_entries_current_;

  column_format_enum format_;

  ref_context_p ref_context_;

  bool is_finalized_;

  column_builder(value_type_p entry_type, column_format_enum format);

  inline int64_t get_table_entry_offset(int64_t entry_index) {
    int64_t ret = entry_index * COLUMN_TABLE_ENTRY_SIZE_BYTES;
    return ret;
  }

  void append_raw(buffer src);
  void append(const value_p& entry);

  inline void put_raw(buffer src, int64_t i, int64_t worker_index) {
    bin_handle h = entries_acc_->append(src.addr_, src.length_, worker_index);
    {
      int64_t header[3];
      header[0] = h.index_;
      header[1] = h.offset_;
      header[2] = h.len_;

      ASSERT_TRUE(sizeof(header) == COLUMN_TABLE_ENTRY_SIZE_BYTES);

      top_acc_->put_data_unchecked(
        get_table_entry_offset(i),
        &header[0],
        COLUMN_TABLE_ENTRY_SIZE_BYTES
        );
    }
  }

  void put(const value_p& entry, int64_t i, int64_t worker_index);

  inline void extend_length_raw(int64_t num_entries_new) {
    assert(!is_finalized_);
    if (num_entries_new <= num_entries_current_) {
      return;
    }
    top_acc_->reserve_length(num_entries_new * COLUMN_TABLE_ENTRY_SIZE_BYTES);
    num_entries_current_ = num_entries_new;
  }

  inline void extend_with_entries(const vector<value_p>& entries) {
    int64_t start_index = num_entries_current_;
    assert(!is_finalized_);
    this->extend_length_raw(start_index + len(entries));
    for (int64_t i = 0; i < len(entries); i++) {
      this->put(entries[i], start_index + i, 0);
    }
  }

  value_p at(int64_t i);

  value_p finalize();
  value_p finalize(bool known_unique);
};

using column_builder_p = shared_ptr<column_builder>;

inline column_builder_p column_builder_create(value_type_p entry_type) {
  return make_shared<column_builder>(entry_type, column_format_enum::VARIABLE);
}

/**
 * Convenience builder for a random-access SFrame table, which just maintains a
 * series of column builders internally.
 */
struct table_builder {
  vector<string> column_names_;
  vector<column_builder_p> column_builders_;

  bool is_finalized_;

  table_builder(vector<string> column_names,
                vector<value_type_p> column_types)
    : column_names_(column_names),
      is_finalized_(false) {

    ASSERT_EQ(len(column_names), len(column_types));
    for (int64_t i = 0; i < len(column_types); i++) {
      column_builders_.push_back(column_builder_create(column_types[i]));
    }
  }

  void append(const vector<value_p>& entries);
  value_p finalize();
};

using table_builder_p = shared_ptr<table_builder>;

inline table_builder_p table_builder_create(
  vector<string> column_names, vector<value_type_p> column_types) {

  return make_shared<table_builder>(column_names, column_types);
}

/**
 * Provides efficient random-access view of a serialized column value in memory.
 */
struct column_view_variable {
  binary_data_view_fixed_p meta_view_;
  binary_data_view_fixed_p top_view_;
  binary_data_view_variable_p entries_view_;

  optional<url_p> url_context_;

  value_type_p type_;

  int64_t num_entries_;
  column_format_enum format_;

  value_type_p entry_type_;

  column_view_variable(string base_path, optional<url_p> url_context);
  column_view_variable(
    binary_data_view_fixed_p meta_view,
    binary_data_view_fixed_p top_view,
    binary_data_view_variable_p entries_view,
    optional<url_p> url_context);

  inline int64_t get_table_entry_offset(int64_t entry_index) {
    auto ret = entry_index * COLUMN_TABLE_ENTRY_SIZE_BYTES;
    return ret;
  }

  value_p at(int64_t i);

  inline bin_handle at_raw_locate(int64_t i) {
    int64_t bin_handle_raw[3];
    top_view_->get_data(
      bin_handle_raw,
      get_table_entry_offset(i),
      COLUMN_TABLE_ENTRY_SIZE_BYTES);

    bin_handle h;
    h.index_ = bin_handle_raw[0];
    h.offset_ = bin_handle_raw[1];
    h.len_ = bin_handle_raw[2];

    return h;
  }

  inline buffer at_raw(int64_t i) {
    auto h = this->at_raw_locate(i);
    return entries_view_->get_data_raw(h);
  }

  inline uint128_t at_raw_hash(int64_t i) {
    auto h = this->at_raw_locate(i);
    return entries_view_->get_data_hash(h);
  }
};

template<typename T>
inline void column_value_append_raw_scalar(column_builder_p& x, T v) {
  x->append_raw(buffer(reinterpret_cast<char*>(&v), sizeof(v)));
}

template<typename T>
inline void column_value_put_raw_scalar(
  column_builder_p& x, T v, int64_t i, int64_t worker_index) {
  x->put_raw(buffer(reinterpret_cast<char*>(&v), sizeof(v)), i, worker_index);
}

template<typename T>
inline void column_value_put_raw_1d(
  column_builder_p& x, const char* addr, int64_t len, int64_t i, int64_t worker_index) {

  int64_t size_bytes = len * sizeof(T);
  int64_t size_words = ceil_divide<int64_t>(size_bytes, 8);
  char* addr_ext = reinterpret_cast<char*>(malloc(16 + size_words * 8));
  reinterpret_cast<int64_t*>(addr_ext)[0] = 1;
  reinterpret_cast<int64_t*>(addr_ext)[1] = len;
  memcpy(addr_ext + 16, addr, size_bytes);
  x->put_raw(buffer(addr_ext, 16 + size_bytes), i, worker_index);
}

value_p read_bin_value(istream& is, optional<url_p> load_url);

void write_bin_value(
  ostream& os, value_p x, optional<ref_context_p> ctx,
  optional<unordered_set<int64_t>*> local_refs_acc);

value_p value_column_at(value_p v, int64_t i);
value_p value_column_at_deref(value_p x, int64_t i);

void value_column_iterate(
  vector<value_p> v, function<bool(int64_t, vector<value_p>)> yield);
void value_column_iterate(value_p v, function<bool(int64_t, value_p)> yield);

value_p value_deref(value_p x);

struct value_nd_vector {
  void* base_addr_;
  bool base_addr_owned_;

  dtype_enum dtype_;
  vector<int64_t> shape_;
  vector<int64_t> strides_;
  bool contiguous_;

  inline int64_t size() {
    return product(shape_);
  }

  // NOTE: source must be contiguous
  inline static value_nd_vector_p create_from_buffer_copy(
    const void* src_addr, dtype_enum dtype, int64_t num_elements,
    vector<int64_t> shape, vector<int64_t> strides) {

    ASSERT_EQ(product(shape), num_elements);

    int64_t total_size = num_elements * dtype_size_bytes(dtype);

    void* base_addr = malloc(total_size);
    bool base_addr_owned = true;
    memcpy(base_addr, src_addr, total_size);
    bool contiguous = true;

    return value_nd_vector::create(
      base_addr, base_addr_owned, dtype, shape, strides, contiguous);
  }

  inline static value_nd_vector_p create_from_buffer_copy_1d(
    const void* src_addr, dtype_enum dtype, int64_t num_elements) {

    return create_from_buffer_copy(
      src_addr, dtype, num_elements, {num_elements,}, {1,});
  }

  inline static value_p create_from_string(const string& x) {
    auto ret = value_nd_vector::create_from_buffer_copy_1d(
      reinterpret_cast<const void*>(x.c_str()), dtype_enum::I8, len(x));
    return value::create(
      ret,
      value_type::create_string(),
      NONE<ref_context_p>(),
      NONE<url_p>(),
      NONE<int64_t>());
  }

  inline static value_p create_scalar_zero(dtype_enum dtype) {
    uint64_t zero = 0;
    auto ret = value_nd_vector::create_from_buffer_copy(
      &zero, dtype, 1, vector<int64_t>(), vector<int64_t>());
    return value::create(
      ret,
      value_type::create_scalar(dtype),
      NONE<ref_context_p>(),
      NONE<url_p>(),
      NONE<int64_t>());
  }

  template<typename T, dtype_enum Dtype>
  inline static value_p create_scalar(T x) {
    auto ret = value_nd_vector::create_from_buffer_copy(
      &x, Dtype, 1, vector<int64_t>(), vector<int64_t>());
    return value::create(
      ret,
      value_type::create_scalar(Dtype),
      NONE<ref_context_p>(),
      NONE<url_p>(),
      NONE<int64_t>());
  }

  inline static value_p create_scalar_int64(int64_t x) {
    return create_scalar<int64_t, dtype_enum::I64>(x);
  }

  inline static value_p create_scalar_float64(double x) {
    return create_scalar<double, dtype_enum::F64>(x);
  }

  inline static value_p create_scalar_bool(bool x) {
    return create_scalar<bool, dtype_enum::BOOL>(x);
  }

  int64_t value_scalar_int64();
  bool value_scalar_bool();

  value_nd_vector(
    void* base_addr, bool base_addr_owned, dtype_enum dtype,
    const vector<int64_t>& shape, const vector<int64_t>& strides, bool contiguous)
    : base_addr_(base_addr), base_addr_owned_(base_addr_owned), dtype_(dtype),
      shape_(shape), strides_(strides), contiguous_(contiguous) { }

  template<typename... Ts> static value_nd_vector_p create(Ts... args) {
    return make_shared<value_nd_vector>(args...);
  }

  ~value_nd_vector() {
    if (base_addr_owned_) {
      free(base_addr_);
    }
  }
};

inline void value_nd_vector_copy_to_buffer(
  void* dst_addr, value_p src) {

  src = value_deref(src);
  auto src_v = src->as<value_nd_vector_p>();
  ASSERT_TRUE(src_v->contiguous_);
  memcpy(
    dst_addr,
    src_v->base_addr_,
    src_v->size() * dtype_size_bytes(src_v->dtype_));
}

inline vector<int64_t> value_nd_vector_shape(value_p src) {
  src = value_deref(src);
  auto src_v = src->as<value_nd_vector_p>();
  return src_v->shape_;
}

inline dtype_enum value_nd_vector_dtype(value_p src) {
  src = value_deref(src);
  auto src_v = src->as<value_nd_vector_p>();
  return src_v->dtype_;
}

struct value_record {
  value_type_p type_;
  vector<value_p> entries_;

  value_record(value_type_p type, const vector<value_p>& entries)
    : type_(type), entries_(entries) { }
};

inline vector<string> value_record_get_keys(value_p src) {
  auto src_v = src->as<value_record_p>();
  auto cc = src_v->type_->as<value_type_record_p>();
  vector<string> ret;
  for (auto x : cc->field_types_) {
    ret.push_back(x.first);
  }
  return ret;
}

inline vector<value_p> value_record_get_values(value_p src) {
  auto src_v = src->as<value_record_p>();
  return src_v->entries_;
}

inline value_p value_create_table_from_columns(
  vector<string> column_names, vector<value_p> columns) {

  vector<value_type_p> ret_element_types;
  for (auto x : columns) {
    ret_element_types.push_back(
      x->get_type()->as<value_type_column_p>()->element_type_);
  }

  auto ret_type = value_type_table_create(column_names, ret_element_types);
  auto ret = value::create(
    make_shared<value_record>(ret_type, columns),
    ret_type,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
  return ret;
}

struct value_either {
  value_type_p type_;

  int64_t val_which_;
  value_p val_data_;

  value_either(value_type_p type, int64_t val_which, value_p val_data)
    : type_(type), val_which_(val_which), val_data_(val_data) { }
};

struct value_ref {
  value_type_p type_;

  value_ref_enum ref_which_;

  optional<value_p> target_;

  optional<int64_t> column_element_;
  optional<int64_t> column_range_lo_;
  optional<int64_t> column_range_hi_;
  optional<value_p> column_subset_;

  static value_ref_p create_value(value_type_p type, value_p target);
  static value_ref_p create_value_column(value_p column);
  static value_p create_column_element(
    value_type_p type, value_p target, int64_t i);
  static value_p create_column_range(
    value_p target, int64_t range_lo, int64_t range_hi);
  static value_p create_column_subset(value_p target, value_p column_subset);

  value_p ref_column_at_index(int64_t i);

  // NOTE: should only be called by create_* methods, not directly
  value_ref(value_type_p type, value_ref_enum ref_which)
    : type_(type), ref_which_(ref_which) { }
};

struct value_index {
  value_p index_keys_;
  value_p index_values_flat_;
  value_p index_values_grouped_;
  vector<uint128_t> index_hashes_;
  parallel_hash_map<int64_t> index_map_singleton_;
  parallel_hash_map<pair<int64_t, int64_t>> index_map_range_;
  index_mode_enum index_mode_;

  value_index(
    value_p index_keys,
    value_p index_values_flat,
    value_p index_values_grouped,
    vector<uint128_t> index_hashes,
    parallel_hash_map<int64_t> index_map_singleton,
    parallel_hash_map<pair<int64_t, int64_t>> index_map_range,
    index_mode_enum index_mode)
    : index_keys_(index_keys),
      index_values_flat_(index_values_flat),
      index_values_grouped_(index_values_grouped),
      index_hashes_(index_hashes),
      index_map_singleton_(index_map_singleton),
      index_map_range_(index_map_range),
      index_mode_(index_mode) { }
};

struct value_thunk {
  value_type_p type_;
  query_p query_;

  value_thunk(query_p query) : type_(get_type(query)), query_(query) { }
};

struct value_column {
  column_format_enum format_;
  column_view_v view_;

  column_view_variable* view_variable_cached_ { nullptr };

  value_column(column_view_v view)
    : format_(static_cast<column_format_enum>(view.which())), view_(view) {

    switch (format_) {
    case column_format_enum::VARIABLE:
      view_variable_cached_ =
        vget<column_format_enum, column_view_variable_p>(view_).get();
      break;
    default: AU();
    }
  }

  inline int64_t length() {
    switch (format_) {
    case column_format_enum::VARIABLE:
      return view_variable_cached_->num_entries_;
    default: AU();
    }
  }

  inline value_p at(int64_t i) {
    switch (format_) {
    case column_format_enum::VARIABLE: return view_variable_cached_->at(i);
    default: AU();
    }
  }

  inline buffer at_raw(int64_t i) {
    switch (format_) {
    case column_format_enum::VARIABLE: return view_variable_cached_->at_raw(i);
    default: AU();
    }
  }

  inline uint128_t at_raw_hash(int64_t i) {
    switch (format_) {
    case column_format_enum::VARIABLE:
      return view_variable_cached_->at_raw_hash(i);
    default: AU();
    }
  }

  static inline value_column_p create(column_view_v view) {
    return make_shared<value_column>(view);
  }

  static value_p load_column_from_disk_path(
    string path, optional<ref_context_p> refs_accum,
    optional<url_p> url_context, optional<int64_t> value_id);

  static value_p load_column_from_binary_data(
    binary_data_view_fixed_p meta_view,
    binary_data_view_fixed_p top_view,
    binary_data_view_variable_p entries_view,
    optional<ref_context_p> refs_accum,
    optional<url_p> url_context,
    optional<int64_t> value_id);
};

inline int64_t column_value_get_raw_scalar(
  value_column* x, int64_t i, type_specializer<int64_t>) {
  int64_t ret = 0;
  memcpy(&ret, x->at_raw(i).addr_, sizeof(int64_t));
  return ret;
}

inline double column_value_get_raw_scalar(
  value_column* x, int64_t i, type_specializer<double>) {
  double ret = 0;
  memcpy(&ret, x->at_raw(i).addr_, sizeof(double));
  return ret;
}

template<typename T> T column_value_get_raw_scalar(value_column* x, int64_t i) {
  return column_value_get_raw_scalar(x, i, type_specializer<T>());
}

inline string column_value_get_raw_string(value_column* x, int64_t i) {
  buffer src = x->at_raw(i);
  auto len = reinterpret_cast<int64_t*>(src.addr_)[1];
  return string(src.addr_ + 16, len);
}

bool value_eq(value_p x, value_p y);

void write_struct_hash_data(ostream& os, value_p x);

ostream& operator<<(ostream& os, value_p v);

inline string to_string(value_p x) {
  ostringstream os;
  os << x;
  return os.str();
}

/**
 * Evaluate a binary operation on numeric scalars, given the addresses of its
 * operands and destination.
 */
inline void eval_raw_binary(
  scalar_builtin_enum op, void* dst, void* src0, void* src1,
  dtype_enum input_dtype) {

  switch (op) {
  case scalar_builtin_enum::LT:
    switch (input_dtype) {
    case dtype_enum::I64:
      *reinterpret_cast<bool*>(dst) =
        (*reinterpret_cast<int64_t*>(src0) < *reinterpret_cast<int64_t*>(src1));
      break;

    case dtype_enum::F64:
      *reinterpret_cast<bool*>(dst) =
        (*reinterpret_cast<double*>(src0) < *reinterpret_cast<double*>(src1));
      break;

    default:
      cerr << input_dtype << endl;
      AU();
    }
    break;

  case scalar_builtin_enum::ADD:
    switch (input_dtype) {
    case dtype_enum::I64:
      *reinterpret_cast<int64_t*>(dst) =
        (*reinterpret_cast<int64_t*>(src0) + *reinterpret_cast<int64_t*>(src1));
      break;

    case dtype_enum::F64:
      *reinterpret_cast<double*>(dst) =
        (*reinterpret_cast<double*>(src0) + *reinterpret_cast<double*>(src1));
      break;

    default:
      cerr << input_dtype << endl;
      AU();
    }
    break;

  default:
    AU();
  }
}

/**
 * Evaluate a binary operation on numeric scalars, given its operands.
 */
inline value_p eval(scalar_builtin_enum op, vector<value_p> args) {
  auto input_dtype = NONE<dtype_enum>();

  vector<void*> cc_arg_addrs;
  for (auto arg : args) {
    auto cc_arg = arg->as<value_nd_vector_p>();
    ASSERT_EQ(cc_arg->shape_.size(), 0);
    if (!!input_dtype) {
      ASSERT_EQ(cc_arg->dtype_, *input_dtype);
    } else {
      input_dtype = SOME(cc_arg->dtype_);
    }
    cc_arg_addrs.push_back(cc_arg->base_addr_);
  }

  ASSERT_TRUE(!!input_dtype);

  ASSERT_EQ(cc_arg_addrs.size(), arity(op));
  auto ret = value_nd_vector::create_scalar_zero(
    get_result_dtype(op, *input_dtype));

  ASSERT_EQ(arity(op), 2);
  eval_raw_binary(
    op,
    ret->as<value_nd_vector_p>()->base_addr_,
    cc_arg_addrs[0],
    cc_arg_addrs[1],
    *input_dtype);

  return ret;
}

}}

#endif
