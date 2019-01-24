/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_RANDOM_ACCESS_QUERY_H_
#define TURI_SFRAME_RANDOM_ACCESS_QUERY_H_

#include <sframe/sframe_random_access_impl.hpp>

namespace turi { namespace sframe_random_access {

/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_random_access SFrame Random Access Backend
 * \{
 */

DECL_STRUCT(variable_name);

struct variable_name {
  string name_;

  variable_name(string name) : name_(name) { }

  inline static variable_name_p create_auto() {
    static std::atomic<int64_t> next_index_ { 0 };
    string ret = cc_sprintf(
      "_v%lld", static_cast<int64_t>(next_index_.fetch_add(1)));
    return make_shared<variable_name>(ret);
  }

  inline void save(ostream& os) {
    write_bin(os, name_);
  }

  inline static variable_name_p load(istream& is) {
    auto name = read_bin<string>(is);
    return make_shared<variable_name>(name);
  }
};

SERIALIZE_DECL(variable_name_p);

inline void write_bin(
  ostream& os, variable_name_p x, type_specializer<variable_name_p>) {
  x->save(os);
}

inline variable_name_p read_bin(
  istream& is, type_specializer<variable_name_p>) {
  return variable_name::load(is);
}

inline ostream& operator<<(ostream& os, variable_name_p x) {
  os << x->name_;
  return os;
}

enum class column_join_mode {
  INNER,
  OUTER,
};

enum class column_join_position {
  LEFT,
  RIGHT,
};

enum class query_enum {
  CONSTANT,
  VARIABLE,
  LAMBDA,
  APPLY,
  COLUMN_LENGTH,
  COLUMN_GENERATOR,
  COLUMN_REDUCE,
  COLUMN_JOIN,
  EQUALS,
  SCALAR_BUILTIN,
  COLUMN_AT_INDEX,
  COLUMN_TO_MASK,
  COLUMN_FROM_MASK,
  COLUMN_AT_COLUMN,
  RECORD_AT_FIELD,
  RECORD_FROM_FIELDS,
  BUILD_INDEX,
  INDEX_GET_KEYS,
  INDEX_GET_VALUES,
  INDEX_LOOKUP,
};

SERIALIZE_POD(query_enum);

ostream& operator<<(ostream& os, query_enum x);

DECL_STRUCT(query);

DECL_STRUCT(query_constant);
DECL_STRUCT(query_variable);
DECL_STRUCT(query_lambda);
DECL_STRUCT(query_apply);
DECL_STRUCT(query_column_length);
DECL_STRUCT(query_column_generator);
DECL_STRUCT(query_column_reduce);
DECL_STRUCT(query_column_join);
DECL_STRUCT(query_equals);
DECL_STRUCT(query_scalar_builtin);
DECL_STRUCT(query_column_at_index);
DECL_STRUCT(query_column_to_mask);
DECL_STRUCT(query_column_from_mask);
DECL_STRUCT(query_column_at_column);
DECL_STRUCT(query_record_at_field);
DECL_STRUCT(query_record_from_fields);
DECL_STRUCT(query_build_index);
DECL_STRUCT(query_index_get_keys);
DECL_STRUCT(query_index_get_values);
DECL_STRUCT(query_index_lookup);

typedef typename boost::make_recursive_variant<
  query_constant_p,
  query_variable_p,
  query_lambda_p,
  query_apply_p,
  query_column_length_p,
  query_column_generator_p,
  query_column_reduce_p,
  query_column_join_p,
  query_equals_p,
  query_scalar_builtin_p,
  query_column_at_index_p,
  query_column_to_mask_p,
  query_column_from_mask_p,
  query_column_at_column_p,
  query_record_at_field_p,
  query_record_from_fields_p,
  query_build_index_p,
  query_index_get_keys_p,
  query_index_get_values_p,
  query_index_lookup_p
>::type query_v;

struct query_constant {
  value_p value_;

  query_constant(value_p value) : value_(value) { }
};

struct query_variable {
  variable_name_p name_;
  value_type_p type_;

  query_variable(variable_name_p name, value_type_p type)
    : name_(name), type_(type) { }
};

struct query_lambda {
  query_p var_;
  query_p body_;
  vector<query_p> capture_vars_;
  vector<query_p> captures_;

  query_lambda(
    query_p var, query_p body, vector<query_p> capture_vars,
    vector<query_p> captures)
    : var_(var), body_(body), capture_vars_(capture_vars),
      captures_(captures) { }
};

struct query_apply {
  query_p function_;
  query_p argument_;

  query_apply(query_p function, query_p argument);
};

struct query_column_length {
  query_p column_;

  query_column_length(query_p column) : column_(column) { }
};

struct query_column_generator {
  query_p item_function_;
  query_p result_length_;

  value_type_p result_type_;

  query_column_generator(query_p item_function, query_p result_length);
};

struct query_column_reduce {
  query_p column_;
  column_reduce_op_enum reduce_op_;

  value_type_p result_type_;

  query_column_reduce(query_p column, column_reduce_op_enum reduce_op);
};

struct query_column_join {
  query_p source_column_;
  query_p source_index_;
  query_p other_index_;

  column_join_mode mode_;
  column_join_position position_;

  query_column_join(
    query_p source_column, query_p source_index, query_p other_index,
    column_join_mode mode, column_join_position position);
};

struct query_equals {
  query_p x_;
  query_p y_;

  query_equals(query_p x, query_p y) : x_(x), y_(y) { }
};

struct query_scalar_builtin {
  scalar_builtin_enum op_;
  vector<query_p> arguments_;

  query_scalar_builtin(scalar_builtin_enum op, vector<query_p> arguments)
    : op_(op), arguments_(arguments) { }
};

struct query_column_at_index {
  query_p column_;
  query_p index_;

  query_column_at_index(query_p column, query_p index)
    : column_(column), index_(index) { }
};

struct query_column_to_mask {
  query_p source_column_;
  query_p result_length_;

  query_column_to_mask(query_p source_column, query_p result_length)
    : source_column_(source_column), result_length_(result_length) { }
};

struct query_column_from_mask {
  query_p mask_;

  query_column_from_mask(query_p mask) : mask_(mask) { }
};

struct query_column_at_column {
  query_p source_column_;
  query_p index_column_;

  query_column_at_column(query_p source_column, query_p index_column)
    : source_column_(source_column), index_column_(index_column) { }
};

struct query_record_at_field {
  query_p record_;
  int64_t field_index_;

  query_record_at_field(query_p record, int64_t field_index)
    : record_(record), field_index_(field_index) { }
};

struct query_record_from_fields {
  value_type_p type_;
  vector<query_p> fields_;

  query_record_from_fields(value_type_p type, vector<query_p> fields)
    : type_(type), fields_(fields) { }
};

struct query_build_index {
  vector<query_p> source_columns_;
  index_mode_enum index_mode_;

  query_build_index(vector<query_p> source_columns, index_mode_enum index_mode)
    : source_columns_(source_columns), index_mode_(index_mode) { }
};

struct query_index_get_keys {
  query_p source_index_;

  query_index_get_keys(query_p source_index) : source_index_(source_index) { }
};

struct query_index_get_values {
  query_p source_index_;

  query_index_get_values(query_p source_index)
    : source_index_(source_index) { }
};

struct query_index_lookup {
  query_p source_index_;
  vector<query_p> source_values_;
  index_lookup_mode_enum index_lookup_mode_;

  query_index_lookup(
    query_p source_index, vector<query_p> source_values,
    index_lookup_mode_enum index_lookup_mode)
    : source_index_(source_index), source_values_(source_values),
      index_lookup_mode_(index_lookup_mode) { }
};

/**
 * Represents a relational query over a given set of values. A \ref query object
 * is a tagged union of the cases enumerated in \ref query_enum.
 */
struct query : public enable_shared_from_this<query> {
  static const char* object_id_;

  query_v v_;

  query(query_v v) : v_(v) { }

  inline query_enum which() {
    return static_cast<query_enum>(v_.which());
  }

  template<typename T> T& as() {
    return vget<query_enum, T>(v_);
  }

  template<typename T> const T& as() const {
    return vget<query_enum, T>(v_);
  }

  template<typename T> static query_p create(const T& t) {
    return make_shared<query>(query_v(t));
  }

  inline static query_p from_value(value_p v) {
    if (v->which() == value_enum::THUNK) {
      return v->as<value_thunk_p>()->query_;
    } else {
      return query::create(make_shared<query_constant>(v));
    }
  }

  inline static query_p create_equals(query_p x, query_p y) {
    return query::create(make_shared<query_equals>(x, y));
  }

  inline static query_p create_variable_auto(value_type_p var_type) {
    return query::create(
      make_shared<query_variable>(variable_name::create_auto(), var_type));
  }

  static query_p create_lambda(
    function<query_p(query_p)> f, value_type_p var_type);

  inline static query_p create_scalar_builtin(
    scalar_builtin_enum op, vector<query_p> args) {
    return query::create(make_shared<query_scalar_builtin>(op, args));
  }

  inline static query_p create_column_length(query_p column) {
    return query::create(make_shared<query_column_length>(column));
  }

  inline static query_p create_column_at_index(query_p column, query_p index) {
    return query::create(make_shared<query_column_at_index>(column, index));
  }

  inline static query_p create_column_at_column(
    query_p column, query_p index_column) {

    return query::create(
      make_shared<query_column_at_column>(column, index_column));
  }

  inline static query_p create_record_at_field_index(
    query_p record, int64_t field_index) {

    auto record_type = record->get_type();
    ASSERT_EQ(record_type->which(), value_type_enum::RECORD);
    auto ty = record_type->as<value_type_record_p>();
    ASSERT_GE(field_index, 0);
    ASSERT_LT(field_index, len(ty->field_types_));
    return query::create(
      make_shared<query_record_at_field>(record, field_index));
  }

  inline static query_p create_record_at_field_name(
    query_p record, string field_name) {

    auto record_type = record->get_type();
    ASSERT_EQ(record_type->which(), value_type_enum::RECORD);
    auto ty = record_type->as<value_type_record_p>();
    for (int64_t i = 0; i < len(ty->field_types_); i++) {
      if (field_name == ty->field_types_[i].first) {
        return query::create_record_at_field_index(record, i);
      }
    }

    cerr << "Field not found: " << field_name << endl;
    AU();
  }

  inline static query_p create_record_from_fields(
    value_type_p type, vector<query_p> fields) {

    return query::create(make_shared<query_record_from_fields>(type, fields));
  }

  inline static query_p create_table_at_column(
    query_p table, query_p index_column) {

    auto table_type = table->get_type();
    ASSERT_EQ(table_type->which(), value_type_enum::RECORD);
    ASSERT_EQ(*table_type->tag_, value_type_tag_enum::DATA_TABLE);
    auto ty = table_type->as<value_type_record_p>();

    vector<query_p> output_columns;
    for (int64_t i = 0; i < len(ty->field_types_); i++) {
      query_p x_in = query::create_record_at_field_index(table, i);
      query_p x_out = query::create(
        make_shared<query_column_at_column>(x_in, index_column));
      output_columns.push_back(x_out);
    }
    return query::create_record_from_fields(table_type, output_columns);
  }

  inline static query_p create_table_at_index(query_p table, query_p index) {
    auto table_type = table->get_type();
    ASSERT_EQ(table_type->which(), value_type_enum::RECORD);
    ASSERT_EQ(*table_type->tag_, value_type_tag_enum::DATA_TABLE);
    auto ty = table_type->as<value_type_record_p>();

    vector<query_p> output_columns;
    for (int64_t i = 0; i < len(ty->field_types_); i++) {
      query_p x_in = query::create_record_at_field_index(table, i);
      query_p x_out =
        query::create(make_shared<query_column_at_index>(x_in, index));
      output_columns.push_back(x_out);
    }
    return query::create_record_from_fields(
      value_type::create_record(ty->field_types_),
      output_columns);
  }

  inline static query_p create_column_to_mask(
    query_p source_column, query_p result_length) {

    return query::create(
      make_shared<query_column_to_mask>(source_column, result_length));
  }

  inline static query_p create_column_from_mask(query_p mask) {
    return query::create(make_shared<query_column_from_mask>(mask));
  }

  inline static query_p create_column_generator(
    query_p item_function, query_p result_length) {

    return query::create(
      make_shared<query_column_generator>(item_function, result_length));
  }

  inline static query_p create_column_reduce(
    query_p column, column_reduce_op_enum reduce_op) {

    return query::create(
      make_shared<query_column_reduce>(column, reduce_op));
  }

  inline static query_p create_column_join(
    query_p source_column, query_p source_index, query_p other_index,
    column_join_mode mode, column_join_position position) {

    return query::create(
      make_shared<query_column_join>(
        source_column, source_index, other_index, mode, position));
  }

  inline static query_p create_build_index(
    vector<query_p> source_columns, index_mode_enum index_mode) {
    return query::create(
      make_shared<query_build_index>(source_columns, index_mode));
  }

  inline static query_p create_index_get_keys(query_p source_index) {
    return query::create(make_shared<query_index_get_keys>(source_index));
  }

  inline static query_p create_index_get_values(query_p source_index) {
    return query::create(make_shared<query_index_get_values>(source_index));
  }

  inline static query_p create_index_lookup(
    query_p source_index, vector<query_p> source_values,
    index_lookup_mode_enum index_lookup_mode) {

    return query::create(
      make_shared<query_index_lookup>(
        source_index, source_values, index_lookup_mode));
  }

  inline query_p equals(value_p value) {
    return query::create_equals(shared_from_this(), query::from_value(value));
  }

  query_p sum();

  query_p equals_value_poly(value_p x);
  query_p equals_int_poly(int64_t x);
  query_p equals_string_poly(string x);

  inline value_type_p get_type() {
    return this->infer_type();
  }

  void write_bin_non_struct_params(ostream& os, optional<ref_context_p> ctx);
  value_type_p infer_type();

  vector<query_p> struct_deps_toplevel();
  vector<query_p> struct_deps_full();

  query_p with_struct_deps_toplevel(vector<query_p> new_deps);

  optional<string> struct_hash_cached_;
};

query_p query_builtin_poly(scalar_builtin_enum op, vector<query_p> args);

using query_set = unordered_map<string, bool>;
using query_set_p = shared_ptr<query_set>;
using query_map = unordered_map<string, query_p>;
using query_map_p = shared_ptr<query_map>;
using query_multi_map = unordered_map<string, pair<query_p, vector<query_p>>>;
using query_multi_map_p = shared_ptr<query_multi_map>;

pair<query_multi_map_p, query_multi_map_p> struct_deps_map_toplevel(query_p x);
query_p replace_all_toplevel(query_p x, query_map_p env);

void write_struct_hash_data(ostream& os, query_p x);

value_type_p get_type(query_p x);

query_p map_query(function<query_p(query_p)> f, query_p x);

ostream& operator<<(ostream& os, query_p x);

value_p eval(query_p x);

}}

#endif
