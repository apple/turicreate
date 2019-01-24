/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <sframe_query_engine/experimental/sframe_random_access_query_impl.hpp>

#include <stack>
#include <queue>

using std::istringstream;
using std::make_pair;
using std::max;
using std::min;
using std::stack;
using std::queue;

namespace turi { namespace sframe_random_access {

/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_random_access SFrame Random Access Backend
 * \{
 */

query_apply::query_apply(query_p function, query_p argument)
  : function_(function), argument_(argument) {
  ASSERT_TRUE(function_->which() == query_enum::LAMBDA);
}

query_column_generator::query_column_generator(
  query_p item_function, query_p result_length)
  : item_function_(item_function), result_length_(result_length) {

  ASSERT_TRUE(item_function_->which() == query_enum::LAMBDA);
  auto res_entry_type =
    item_function->get_type()->as<value_type_function_p>()->right_;
  result_type_ = value_type::create_column(res_entry_type, NONE<int64_t>(), false);
}

query_column_reduce::query_column_reduce(
  query_p column, column_reduce_op_enum reduce_op)
  : column_(column), reduce_op_(reduce_op) {

  auto res_entry_type =
    column_->get_type()->as<value_type_column_p>()->element_type_;

  switch (reduce_op_) {
  case column_reduce_op_enum::SUM:
    ASSERT_EQ(res_entry_type->which(), value_type_enum::ND_VECTOR);
    result_type_ = res_entry_type;
    break;
  default:
    AU();
  }
}

query_column_join::query_column_join(
  query_p source_column, query_p source_index, query_p other_index,
  column_join_mode mode, column_join_position position)
  : source_column_(source_column), source_index_(source_index),
    other_index_(other_index), mode_(mode), position_(position) {

  if (static_cast<int64_t>(mode_) != static_cast<int64_t>(column_join_mode::INNER)) {
    fmt(cerr, "Outer join not yet supported\n");
    AU();
  }
}

void query::write_bin_non_struct_params(
  ostream& os, optional<ref_context_p> ctx) {

  write_bin(os, this->get_type());

  switch (which()) {
  case query_enum::CONSTANT: {
    auto cc = this->as<query_constant_p>();
    write_bin_value(os, cc->value_, ctx, NONE<unordered_set<int64_t>*>());
    break;
  }
  case query_enum::VARIABLE: {
    auto cc = this->as<query_variable_p>();
    write_bin(os, cc->name_);
    write_bin(os, cc->type_);
    break;
  }
  case query_enum::SCALAR_BUILTIN: {
    auto cc = this->as<query_scalar_builtin_p>();
    write_bin(os, static_cast<int64_t>(cc->op_));
    break;
  }
  case query_enum::COLUMN_GENERATOR: {
    auto cc = this->as<query_column_generator_p>();
    write_bin(os, cc->result_type_);
    break;
  }
  case query_enum::COLUMN_REDUCE: {
    auto cc = this->as<query_column_reduce_p>();
    write_bin(os, cc->result_type_);
    write_bin(os, cc->reduce_op_);
    break;
  }
  case query_enum::COLUMN_JOIN: {
    auto cc = this->as<query_column_join_p>();
    write_bin(os, static_cast<int64_t>(cc->mode_));
    write_bin(os, static_cast<int64_t>(cc->position_));
    break;
  }
  case query_enum::RECORD_AT_FIELD: {
    auto cc = this->as<query_record_at_field_p>();
    write_bin(os, cc->field_index_);
    break;
  }
  case query_enum::LAMBDA:
  case query_enum::APPLY:
  case query_enum::COLUMN_LENGTH:
  case query_enum::EQUALS:
  case query_enum::COLUMN_AT_INDEX:
  case query_enum::COLUMN_TO_MASK:
  case query_enum::COLUMN_FROM_MASK:
  case query_enum::COLUMN_AT_COLUMN:
  case query_enum::RECORD_FROM_FIELDS:
  case query_enum::INDEX_GET_KEYS:
  case query_enum::INDEX_GET_VALUES: {
    break;
  }
  case query_enum::BUILD_INDEX: {
    auto cc = this->as<query_build_index_p>();
    write_bin(os, cc->index_mode_);
    break;
  }
  case query_enum::INDEX_LOOKUP: {
    auto cc = this->as<query_index_lookup_p>();
    write_bin(os, cc->index_lookup_mode_);
    break;
  }
  default:
    cerr << static_cast<int64_t>(which()) << endl;
    AU();
  }
}

void write_struct_hash_data(ostream& os, query_p x) {
  write_bin<string>(os, query::object_id_);

  switch (x->which()) {
  case query_enum::CONSTANT: {
    auto cc = x->as<query_constant_p>();
    write_bin<string>(os, struct_hash(cc->value_));
    break;
  }
  default:
    x->write_bin_non_struct_params(os, NONE<ref_context_p>());
  }

  for (auto xi : x->struct_deps_full()) {
    write_string_raw(os, struct_hash(xi));
  }
}

ostream& operator<<(ostream& os, query_enum x) {
  switch (x) {
  case query_enum::CONSTANT: os << "CONSTANT"; break;
  case query_enum::VARIABLE: os << "VARIABLE"; break;
  case query_enum::SCALAR_BUILTIN: os << "SCALAR_BUILTIN"; break;
  case query_enum::LAMBDA: os << "LAMBDA"; break;
  case query_enum::APPLY: os << "APPLY"; break;
  case query_enum::COLUMN_LENGTH: os << "COLUMN_LENGTH"; break;
  case query_enum::COLUMN_GENERATOR: os << "COLUMN_GENERATOR"; break;
  case query_enum::COLUMN_REDUCE: os << "COLUMN_REDUCE"; break;
  case query_enum::COLUMN_JOIN: os << "COLUMN_JOIN"; break;
  case query_enum::EQUALS: os << "EQUALS"; break;
  case query_enum::COLUMN_AT_INDEX: os << "COLUMN_AT_INDEX"; break;
  case query_enum::COLUMN_TO_MASK: os << "COLUMN_TO_MASK"; break;
  case query_enum::COLUMN_FROM_MASK: os << "COLUMN_FROM_MASK"; break;
  case query_enum::COLUMN_AT_COLUMN: os << "COLUMN_AT_COLUMN"; break;
  case query_enum::RECORD_AT_FIELD: os << "RECORD_AT_FIELD"; break;
  case query_enum::RECORD_FROM_FIELDS: os << "RECORD_FROM_FIELDS"; break;
  case query_enum::BUILD_INDEX: os << "BUILD_INDEX"; break;
  case query_enum::INDEX_GET_KEYS: os << "INDEX_GET_KEYS"; break;
  case query_enum::INDEX_GET_VALUES: os << "INDEX_GET_VALUES"; break;
  case query_enum::INDEX_LOOKUP: os << "INDEX_LOOKUP"; break;
  default:
    AU();
  }
  return os;
}

value_type_p query::infer_type() {
  switch (which()) {
  case query_enum::CONSTANT: {
    auto cc = this->as<query_constant_p>();
    return cc->value_->get_type();
  }
  case query_enum::VARIABLE: {
    auto cc = this->as<query_variable_p>();
    return cc->type_;
  }
  case query_enum::LAMBDA: {
    auto cc = this->as<query_lambda_p>();
    return value_type::create_function(
      cc->var_->get_type(), cc->body_->get_type());
  }
  case query_enum::APPLY: {
    fmt(cerr, "Application of lambda function not yet supported\n");
    AU();
  }
  case query_enum::COLUMN_LENGTH: {
    return value_type::create_scalar(dtype_enum::I64);
  }
  case query_enum::COLUMN_GENERATOR: {
    auto cc = this->as<query_column_generator_p>();
    auto cc_f = cc->item_function_->get_type()->as<value_type_function_p>();
    return value_type::create_column(cc_f->right_, NONE<int64_t>(), false);
  }
  case query_enum::COLUMN_REDUCE: {
    auto cc = this->as<query_column_reduce_p>();
    return cc->result_type_;
  }
  case query_enum::COLUMN_JOIN: {
    auto cc = this->as<query_column_join_p>();
    return cc->source_column_->get_type();
  }
  case query_enum::EQUALS: {
    return value_type::create_scalar(dtype_enum::BOOL);
  }
  case query_enum::SCALAR_BUILTIN: {
    auto cc = this->as<query_scalar_builtin_p>();
    auto input_type =
      at(cc->arguments_, 0)->get_type()->as<value_type_nd_vector_p>();
    return value_type::create_scalar(
      get_result_dtype(cc->op_, input_type->dtype_));
  }
  case query_enum::COLUMN_AT_INDEX: {
    auto cc = this->as<query_column_at_index_p>();
    return cc->column_->get_type()->as<value_type_column_p>()->element_type_;
  }
  case query_enum::COLUMN_TO_MASK: {
    return value_type::create_column(
      value_type::create_scalar(dtype_enum::BOOL), NONE<int64_t>(), false);
  }
  case query_enum::COLUMN_FROM_MASK: {
    return value_type::create_column(
      value_type::create_scalar(dtype_enum::I64), NONE<int64_t>(), true);
  }
  case query_enum::COLUMN_AT_COLUMN: {
    auto cc = this->as<query_column_at_column_p>();
    auto source_column_type =
      cc->source_column_->get_type()->as<value_type_column_p>();
    auto index_column_type =
      cc->index_column_->get_type()->as<value_type_column_p>();
    return value_type::create_column(
      source_column_type->element_type_,
      index_column_type->length_,
      (source_column_type->known_unique_ && index_column_type->known_unique_));
  }
  case query_enum::RECORD_AT_FIELD: {
    auto cc = this->as<query_record_at_field_p>();
    return at(
      cc->record_->get_type()->as<value_type_record_p>()->field_types_,
      cc->field_index_).second;
  }
  case query_enum::RECORD_FROM_FIELDS: {
    auto cc = this->as<query_record_from_fields_p>();
    return cc->type_;
  }
  case query_enum::BUILD_INDEX: {
    auto cc = this->as<query_build_index_p>();
    vector<value_type_p> source_types;
    for (auto source_column : cc->source_columns_) {
      source_types.push_back(source_column->get_type());
    }
    return value_type::create_index(source_types, cc->index_mode_);
  }
  case query_enum::INDEX_GET_KEYS: {
    auto cc = this->as<query_index_get_keys_p>();
    return value_type::create_column(
      value_type::create_scalar(dtype_enum::I64), NONE<int64_t>(), true);
  }
  case query_enum::INDEX_GET_VALUES: {
    return value_type::create_column(
      value_type::create_column(
        value_type::create_scalar(dtype_enum::I64), NONE<int64_t>(), true),
      NONE<int64_t>(),
      false);
  }
  case query_enum::INDEX_LOOKUP: {
    return value_type::create_column(
      value_type::create_scalar(dtype_enum::I64), NONE<int64_t>(), true);
  }
  default:
    cerr << static_cast<int64_t>(which()) << endl;
    AU();
  }
}

vector<query_p> query::struct_deps_toplevel() {
  switch (which()) {
  case query_enum::CONSTANT:
  case query_enum::VARIABLE: {
    return {};
  }
  case query_enum::LAMBDA: {
    auto cc = this->as<query_lambda_p>();
    return cc->captures_;
  }
  case query_enum::APPLY: {
    auto cc = this->as<query_apply_p>();
    return {cc->function_, cc->argument_};
  }
  case query_enum::COLUMN_LENGTH: {
    auto cc = this->as<query_column_length_p>();
    return {cc->column_};
  }
  case query_enum::COLUMN_GENERATOR: {
    auto cc = this->as<query_column_generator_p>();
    return {cc->item_function_, cc->result_length_};
  }
  case query_enum::COLUMN_REDUCE: {
    auto cc = this->as<query_column_reduce_p>();
    return {cc->column_,};
  }
  case query_enum::COLUMN_JOIN: {
    auto cc = this->as<query_column_join_p>();
    return {cc->source_column_, cc->source_index_, cc->other_index_,};
  }
  case query_enum::EQUALS: {
    auto cc = this->as<query_equals_p>();
    return {cc->x_, cc->y_};
  }
  case query_enum::SCALAR_BUILTIN: {
    auto cc = this->as<query_scalar_builtin_p>();
    return cc->arguments_;
  }
  case query_enum::COLUMN_AT_INDEX: {
    auto cc = this->as<query_column_at_index_p>();
    return {cc->column_, cc->index_};
  }
  case query_enum::COLUMN_TO_MASK: {
    auto cc = this->as<query_column_to_mask_p>();
    return {cc->source_column_, cc->result_length_};
  }
  case query_enum::COLUMN_FROM_MASK: {
    auto cc = this->as<query_column_from_mask_p>();
    return {cc->mask_};
  }
  case query_enum::COLUMN_AT_COLUMN: {
    auto cc = this->as<query_column_at_column_p>();
    return {cc->source_column_, cc->index_column_};
  }
  case query_enum::RECORD_AT_FIELD: {
    auto cc = this->as<query_record_at_field_p>();
    return {cc->record_};
  }
  case query_enum::RECORD_FROM_FIELDS: {
    auto cc = this->as<query_record_from_fields_p>();
    return cc->fields_;
  }
  case query_enum::BUILD_INDEX: {
    auto cc = this->as<query_build_index_p>();
    return cc->source_columns_;
  }
  case query_enum::INDEX_GET_KEYS: {
    auto cc = this->as<query_index_get_keys_p>();
    return {cc->source_index_};
  }
  case query_enum::INDEX_GET_VALUES: {
    auto cc = this->as<query_index_get_values_p>();
    return {cc->source_index_};
  }
  case query_enum::INDEX_LOOKUP: {
    auto cc = this->as<query_index_lookup_p>();
    vector<query_p> ret;
    ret.push_back(cc->source_index_);
    for (auto vi : cc->source_values_) {
      ret.push_back(vi);
    }
    return ret;
  }
  default:
    cerr << static_cast<int64_t>(which()) << endl;
    AU();
    return {};
  }
}

vector<query_p> query::struct_deps_full() {
  vector<query_p> ret = this->struct_deps_toplevel();

  if (which() == query_enum::LAMBDA) {
    auto cc = this->as<query_lambda_p>();
    ret.push_back(cc->var_);
    ret.push_back(cc->body_);
    for (auto x : cc->capture_vars_) {
      ret.push_back(x);
    }
  }

  return ret;
}

query_p query::with_struct_deps_toplevel(vector<query_p> new_deps) {
  ASSERT_EQ(new_deps.size(), this->struct_deps_toplevel().size());

  switch (which()) {
  case query_enum::CONSTANT:
  case query_enum::VARIABLE: {
    return this->shared_from_this();
  }
  case query_enum::LAMBDA: {
    auto cc = this->as<query_lambda_p>();
    vector<query_p> new_captures = new_deps;
    return query::create(
      make_shared<query_lambda>(
        cc->var_, cc->body_, cc->capture_vars_, new_captures));
  }
  case query_enum::APPLY: {
    return query::create(
      make_shared<query_apply>(at(new_deps, 0), at(new_deps, 1)));
  }
  case query_enum::COLUMN_LENGTH: {
    return query::create(make_shared<query_column_length>(at(new_deps, 0)));
  }
  case query_enum::COLUMN_GENERATOR: {
    return query::create(
      make_shared<query_column_generator>(at(new_deps, 0), at(new_deps, 1)));
  }
  case query_enum::COLUMN_REDUCE: {
    auto cc = this->as<query_column_reduce_p>();
    return query::create(
      make_shared<query_column_reduce>(at(new_deps, 0), cc->reduce_op_));
  }
  case query_enum::COLUMN_JOIN: {
    auto cc = this->as<query_column_join_p>();
    return query::create(
      make_shared<query_column_join>(
        at(new_deps, 0), at(new_deps, 1), at(new_deps, 2), cc->mode_,
        cc->position_));
  }
  case query_enum::EQUALS: {
    return query::create(
      make_shared<query_equals>(at(new_deps, 0), at(new_deps, 1)));
  }
  case query_enum::SCALAR_BUILTIN: {
    auto cc = this->as<query_scalar_builtin_p>();
    return query::create(make_shared<query_scalar_builtin>(cc->op_, new_deps));
  }
  case query_enum::COLUMN_AT_INDEX: {
    return query::create(
      make_shared<query_column_at_index>(at(new_deps, 0), at(new_deps, 1)));
  }
  case query_enum::COLUMN_TO_MASK: {
    return query::create(
      make_shared<query_column_to_mask>(at(new_deps, 0), at(new_deps, 1)));
  }
  case query_enum::COLUMN_FROM_MASK: {
    return query::create(make_shared<query_column_from_mask>(at(new_deps, 0)));
  }
  case query_enum::COLUMN_AT_COLUMN: {
    return query::create(
      make_shared<query_column_at_column>(at(new_deps, 0), at(new_deps, 1)));
  }
  case query_enum::RECORD_AT_FIELD: {
    auto cc = this->as<query_record_at_field_p>();
    return query::create(
      make_shared<query_record_at_field>(at(new_deps, 0), cc->field_index_));
  }
  case query_enum::RECORD_FROM_FIELDS: {
    auto cc = this->as<query_record_from_fields_p>();
    return query::create(
      make_shared<query_record_from_fields>(cc->type_, new_deps));
  }
  case query_enum::BUILD_INDEX: {
    auto cc = this->as<query_build_index_p>();
    return query::create(
      make_shared<query_build_index>(new_deps, cc->index_mode_));
  }
  case query_enum::INDEX_GET_KEYS: {
    return query::create(make_shared<query_index_get_keys>(at(new_deps, 0)));
  }
  case query_enum::INDEX_GET_VALUES: {
    return query::create(make_shared<query_index_get_values>(at(new_deps, 0)));
  }
  case query_enum::INDEX_LOOKUP: {
    auto cc = this->as<query_index_lookup_p>();
    auto new_index = at(new_deps, 0);
    vector<query_p> new_source_columns;
    for (int64_t i = 1; i < len(new_deps); i++) {
      new_source_columns.push_back(at(new_deps, i));
    }
    return query::create(
      make_shared<query_index_lookup>(
        new_index, new_source_columns, cc->index_lookup_mode_));
  }
  default:
    cerr << static_cast<int64_t>(which()) << endl;
    AU();
    return {};
  }
}

pair<query_multi_map_p, query_multi_map_p> struct_deps_map_toplevel(
  query_p x) {

  auto res = make_shared<query_multi_map>();
  auto res_rev = make_shared<query_multi_map>();

  stack<query_p> S;
  S.push(x);

  while (!S.empty()) {
    auto curr = S.top();
    if (res->count(struct_hash(curr)) != 0) {
      S.pop();
      continue;
    }

    auto deps = curr->struct_deps_toplevel();

    bool recur = false;
    for (auto dep : deps) {
      if (res->count(struct_hash(dep)) == 0) {
        recur = true;
        S.push(dep);
      }
    }

    if (recur) {
      continue;
    }

    res->insert(make_pair(struct_hash(curr), make_pair(curr, deps)));

    for (auto dep : deps) {
      auto it_rev = res_rev->find(struct_hash(dep));
      if (it_rev == res_rev->end()) {
        res_rev->insert(
          make_pair(
            struct_hash(dep), make_pair(dep, vector<query_p>({curr,}))));
      } else {
        it_rev->second.second.push_back(curr);
      }
    }

    S.pop();
  }

  return make_pair(res, res_rev);
}

query_p replace_all_toplevel(query_p x, query_map_p replace_env) {
  auto res_env = make_shared<query_map>();

  stack<query_p> S;
  S.push(x);

  while (!S.empty()) {
    auto curr = S.top();
    if (res_env->count(struct_hash(curr)) != 0) {
      S.pop();
      continue;
    }

    if (replace_env->count(struct_hash(curr)) != 0) {
      res_env->insert(
        make_pair(struct_hash(curr), replace_env->at(struct_hash(curr))));
      S.pop();
      continue;
    }

    auto deps = curr->struct_deps_toplevel();

    bool recur = false;
    for (auto dep : deps) {
      if (res_env->count(struct_hash(dep)) == 0) {
        S.push(dep);
        recur = true;
      }
    }

    if (recur) {
      continue;
    }

    vector<query_p> new_deps;
    for (auto dep : deps) {
      new_deps.push_back(res_env->at(struct_hash(dep)));
    }

    replace_env->insert(
      make_pair(struct_hash(curr), curr->with_struct_deps_toplevel(new_deps)));

    S.pop();
  }

  return replace_env->at(struct_hash(x));
}

vector<query_p> extract_independent(query_p x, vector<query_p> vars) {
  auto deps_map = struct_deps_map_toplevel(x);
  auto qs_independent = make_shared<query_set>();
  auto qs_visited = make_shared<query_set>();

  for (auto p : *deps_map.first) {
    qs_independent->insert(make_pair(p.first, true));
  }

  queue<query_p> Q;
  for (auto v : vars) {
    Q.push(v);
  }

  while (!Q.empty()) {
    auto curr = Q.front();
    Q.pop();

    qs_independent->erase(struct_hash(curr));
    auto it_rev = deps_map.second->find(struct_hash(curr));
    if (it_rev != deps_map.second->end()) {
      for (auto r : it_rev->second.second) {
        if (qs_visited->count(struct_hash(r)) == 0) {
          qs_visited->insert(make_pair(struct_hash(r), true));
          Q.push(r);
        }
      }
    }
  }

  vector<query_p> ret;

  for (auto p : *deps_map.first) {
    if (qs_independent->count(p.first) == 0) {
      continue;
    }

    if (p.first == struct_hash(x)) {
      ret.push_back(x);
      continue;
    }

    auto it_rev = deps_map.second->find(p.first);
    if (it_rev != deps_map.second->end()) {
      for (auto r : it_rev->second.second) {
        if (qs_independent->count(struct_hash(r)) == 0) {
          ret.push_back(p.second.first);
          break;
        }
      }
    }
  }

  return ret;
}

query_p query::create_lambda(
  function<query_p(query_p)> f, value_type_p var_type) {

  auto var = query::create_variable_auto(var_type);
  auto body = f(var);

  vector<query_p> capture_vars;
  auto captures = extract_independent(body, {var,});
  auto replace_env = make_shared<query_map>();

  for (auto capture : captures) {
    auto capture_var = query::create_variable_auto(capture->get_type());
    replace_env->insert(make_pair(struct_hash(capture), capture_var));
    capture_vars.push_back(capture_var);
  }

  auto new_body = replace_all_toplevel(body, replace_env);

  return query::create(
    make_shared<query_lambda>(var, new_body, capture_vars, captures));
}

query_p query_equals_query_poly_ext(
  query_p lhs, query_p rhs, function<void()> fail_fn) {

  auto lhs_type = lhs->get_type();
  auto rhs_type = rhs->get_type();

  if (rhs_type->which() == value_type_enum::COLUMN &&
      lhs_type->which() != value_type_enum::COLUMN) {
    return query_equals_query_poly_ext(lhs, rhs, fail_fn);
  }

  if (type_valid(lhs_type, rhs_type) || type_valid(rhs_type, lhs_type)) {
    return query::create_equals(lhs, rhs);
  } else if (lhs_type->which() == value_type_enum::COLUMN &&
             rhs_type->which() != value_type_enum::COLUMN) {
    if (type_valid(
          lhs_type->as<value_type_column_p>()->element_type_, rhs_type)) {
      auto index = query::create_build_index({lhs,}, index_mode_enum::EQUALS);
      return query::create_column_to_mask(
        query::create_index_lookup(
          index, {rhs,}, index_lookup_mode_enum::EQUALS),
        query::create_column_length(lhs));
    } else {
      fail_fn(); AU();
    }
  } else {
    fail_fn(); AU();
  }
}

query_p query_equals_query_poly(query_p lhs, query_p rhs) {
  auto fail_fn = [&]() {
    fmt(
      cerr, "Error: type mismatch\n  LHS: %v\n  RHS: %v\n",
      lhs->get_type(), rhs->get_type());
    AU();
  };
  return query_equals_query_poly_ext(lhs, rhs, fail_fn);
}

query_p query_builtin_poly(scalar_builtin_enum op, vector<query_p> args) {
  bool base = true;
  auto column_length = NONE<query_p>();

  for (auto x : args) {
    if (x->get_type()->which() != value_type_enum::ND_VECTOR) {
      ASSERT_EQ(x->get_type()->which(), value_type_enum::COLUMN);
      column_length = SOME(query::create_column_length(x));
      base = false;
    } else {
      auto cc = x->get_type()->as<value_type_nd_vector_p>();
      ASSERT_EQ(cc->ndim_, 0);
    }
  }

  if (base) {
    return query::create_scalar_builtin(op, args);
  } else {
    auto f_gen = [&](query_p i) {
      vector<query_p> args_new;
      for (auto x : args) {
        if (x->get_type()->which() == value_type_enum::COLUMN) {
          args_new.push_back(
            query::create_column_at_index(x, i));
        } else {
          args_new.push_back(x);
        }
      }
      return query_builtin_poly(op, args_new);
    };

    return query::create_column_generator(
      query::create_lambda(f_gen, value_type::create_scalar(dtype_enum::I64)),
      *column_length);
  }
}

query_p query::equals_value_poly(value_p x) {
  return query_equals_query_poly(shared_from_this(), query::from_value(x));
}

query_p query::equals_string_poly(string x) {
  return this->equals_value_poly(value::create_string(x));
}

query_p query::equals_int_poly(int64_t x) {
  return this->equals_value_poly(value::create_scalar_int64(x));
}

query_p query::sum() {
  ASSERT_EQ(this->get_type()->which(), value_type_enum::COLUMN);
  return query::create_column_reduce(
    shared_from_this(),
    column_reduce_op_enum::SUM);
}

value_type_p get_type(query_p x) {
  return x->get_type();
}

query_p map_query(function<query_p(query_p)> f, query_p x) {
  auto f_gen = [f, x](query_p i) {
    return f(query::create_column_at_index(x, i));
  };
  return query::create_column_generator(
    query::create_lambda(f_gen, value_type::create_scalar(dtype_enum::I64)),
    query::create_column_length(x));
}

using query_print_map = unordered_map<string, string>;
using query_print_map_p = shared_ptr<query_print_map>;

void print_query(
  ostream& os, query_p x, optional<query_print_map_p> env_outer, int64_t depth) {

  string prefix;
  if (depth < 3) {
    prefix += char('x' + depth);
  } else {
    ostringstream os;
    os << "w" << (depth - 3) << "_";
    prefix = os.str();
  }

  auto env = make_shared<query_print_map>();
  int64_t curr_display_index = 0;

  stack<query_p> S;
  S.push(x);

  while (!S.empty()) {
    auto curr = S.top();

    if (env->count(struct_hash(curr)) != 0) {
      S.pop();
      continue;
    }

    auto curr_deps = curr->struct_deps_toplevel();

    bool recur = false;
    for (auto dep : curr_deps) {
      if (env->count(struct_hash(dep)) == 0) {
        S.push(dep);
        recur = true;
      }
    }

    if (recur) {
      continue;
    }

    vector<string> curr_deps_display;
    for (auto dep : curr_deps) {
      curr_deps_display.push_back(env->at(struct_hash(dep)));
    }

    string curr_hash_prefix = format_hex(struct_hash(curr)).substr(0, 8);
    string curr_type_str = to_string(curr->get_type());
    if (len(curr_type_str) > 32) {
      curr_type_str = curr_type_str.substr(0, 32 - 3) + "...";
    }
    curr_type_str =
      cc_repstr(" ", max<int64_t>(0, 32 - len(curr_type_str))) + curr_type_str;

    string curr_display = prefix + cc_sprintf("%ld", curr_display_index);
    ++curr_display_index;
    os << endl;
    os << curr_hash_prefix << "  ";
    os << curr_type_str << "  " << cc_repstr(" ", 4 * depth);
    os << curr_display << " := ";
    os << curr->which();
    os << "(";

    bool intercepted = false;

    switch (curr->which()) {
    case query_enum::VARIABLE: {
      auto cc = curr->as<query_variable_p>();
      os << cc->name_;
      intercepted = true;
      break;
    }

    case query_enum::LAMBDA: {
      auto cc = curr->as<query_lambda_p>();
      ASSERT_EQ(cc->capture_vars_.size(), cc->captures_.size());
      os << cc->var_->as<query_variable_p>()->name_;
      for (int64_t i = 0; i < len(cc->capture_vars_); i++) {
        if (i > 0) {
          os << ", ";
        } else {
          os << "; ";
        }
        auto var_i = cc->capture_vars_[i]->as<query_variable_p>();
        os << var_i->name_ << " -> " << env->at(struct_hash(cc->captures_[i]));
      }
      intercepted = true;
      break;
    }

    default:
      break;
    }

    if (!intercepted) {
      for (int64_t i = 0; i < len(curr_deps); i++) {
        if (i > 0) {
          os << ", ";
        }
        os << env->at(struct_hash(curr_deps[i]));
      }
    }

    os << ")";

    if (curr->which() == query_enum::LAMBDA) {
      print_query(os, curr->as<query_lambda_p>()->body_, SOME(env), depth + 1);
    }

    env->insert(make_pair(struct_hash(curr), curr_display));
    S.pop();
  }
}

ostream& operator<<(ostream& os, query_p x) {
  print_query(os, x, NONE<query_print_map_p>(), 0);
  return os;
}

DECL_STRUCT(query_ordered);

struct query_entry {
  query_enum which_;
  string params_data_;
  vector<int64_t> input_ids_;
  optional<query_ordered_p> function_body_;

  query_entry(
    query_enum which, string params_data, vector<int64_t> input_ids,
    optional<query_ordered_p> function_body);
};

struct query_ordered {
  vector<query_entry> entries_;
  int64_t return_value_;
  ref_context_p ref_context_;

  query_ordered(
    vector<query_entry> entries, int64_t return_value, ref_context_p ctx)
    : entries_(entries), return_value_(return_value), ref_context_(ctx) { }
};

ostream& operator<<(ostream& os, query_ordered_p x);

query_ordered_p order_query(query_p x);

query_entry::query_entry(
  query_enum which, string params_data, vector<int64_t> input_ids,
  optional<query_ordered_p> function_body)
  : which_(which), params_data_(params_data), input_ids_(input_ids),
    function_body_(function_body) { }

void print_query_ordered(ostream& os, query_ordered_p x, int64_t depth) {
  string prefix;
  if (depth < 3) {
    prefix += char('x' + depth);
  } else {
    ostringstream os;
    os << "w" << (depth - 3) << "_";
    prefix = os.str();
  }

  for (int64_t i = 0; i < len(x->entries_); i++) {
    auto xi = x->entries_[i];
    istringstream is_params(xi.params_data_);
    auto xi_type = read_bin<value_type_p>(is_params);

    os << endl << cc_repstr(" ", 4 * depth) << prefix << i << " := ";
    os << xi.which_;
    os << "(";
    switch (xi.which_) {
    case query_enum::CONSTANT: {
      auto v = read_bin_value(is_params, NONE<url_p>());
      os << v;
      break;
    }

    default:
      break;
    }
    os << ")";

    if (x->entries_[i].which_ == query_enum::LAMBDA) {
      print_query_ordered(os, *x->entries_[i].function_body_, depth + 1);
    }
  }
}

ostream& operator<<(ostream& os, query_ordered_p x) {
  print_query_ordered(os, x, 0);
  return os;
}

using query_sort_map = unordered_map<string, int64_t>;
using query_sort_map_p = shared_ptr<query_sort_map>;

query_ordered_p sort_query(
  query_p x, optional<query_lambda_p> outer_query,
  optional<query_sort_map_p> outer_env) {

  auto env = make_shared<query_sort_map>();
  vector<query_entry> ret;
  auto ret_ctx = ref_context::create();

  stack<query_p> S;
  S.push(x);

  while (!S.empty()) {
    auto curr = S.top();
    if (env->count(struct_hash(curr)) != 0) {
      S.pop();
      continue;
    }

    if (curr->which() == query_enum::VARIABLE) {
      ASSERT_TRUE(!!outer_query);
      int64_t var_ref_type, var_ref_index = 0;
      if (struct_hash(curr) == struct_hash((*outer_query)->var_)) {
        var_ref_type = 0;
      } else {
        var_ref_type = 1;
        bool found = false;
        for (int64_t i = 0; i < len((*outer_query)->capture_vars_); i++) {
          if (struct_hash(curr) == struct_hash(
                at((*outer_query)->capture_vars_, i))) {
            found = true;
            var_ref_index = i;
          }
        }
        ASSERT_TRUE(found);
      }

      string bin_params;
      {
        ostringstream os_params;
        write_bin(os_params, curr->get_type());
        write_bin<int64_t>(os_params, var_ref_type);
        write_bin<int64_t>(os_params, var_ref_index);
        bin_params = os_params.str();
      }
      vector<int64_t> input_ids;

      query_entry curr_ordered(
        curr->which(),
        bin_params,
        input_ids,
        NONE<query_ordered_p>());

      int64_t curr_index = ret.size();
      ret.push_back(curr_ordered);
      env->insert(make_pair(struct_hash(curr), curr_index));

      continue;
    }

    vector<query_p> recur_items = curr->struct_deps_toplevel();;

    bool recur = false;
    for (auto ri : recur_items) {
      if (env->count(struct_hash(ri)) == 0) {
        S.push(ri);
        recur = true;
      }
    }

    if (recur) {
      continue;
    }

    optional<query_ordered_p> function_body;
    if (curr->which() == query_enum::LAMBDA) {
      auto cc = curr->as<query_lambda_p>();
      function_body = SOME(sort_query(cc->body_, SOME(cc), SOME(env)));
    }

    string bin_params;
    {
      ostringstream os_params;
      curr->write_bin_non_struct_params(os_params, ret_ctx);
      bin_params = os_params.str();
    }

    vector<int64_t> input_ids;
    for (auto ri : recur_items) {
      input_ids.push_back(env->at(struct_hash(ri)));
    }

    query_entry curr_ordered(
      curr->which(),
      bin_params,
      input_ids,
      function_body);

    int64_t curr_index = ret.size();
    ret.push_back(curr_ordered);
    env->insert(make_pair(struct_hash(curr), curr_index));
  }

  int64_t final_index = env->at(struct_hash(x));
  return make_shared<query_ordered>(ret, final_index, ret_ctx);
}

query_p optimize_query(query_p x) {
  auto res_env = make_shared<query_map>();

  stack<query_p> S;
  S.push(x);

  while (!S.empty()) {
    auto curr = S.top();
    if (res_env->count(struct_hash(curr)) != 0) {
      S.pop();
      continue;
    }

    auto deps = curr->struct_deps_toplevel();

    bool recur = false;
    for (auto dep : deps) {
      if (res_env->count(struct_hash(dep)) == 0) {
        S.push(dep);
        recur = true;
      }
    }

    if (recur) {
      continue;
    }

    vector<query_p> new_deps;
    for (auto dep : deps) {
      new_deps.push_back(res_env->at(struct_hash(dep)));
    }

    auto new_curr = curr->with_struct_deps_toplevel(new_deps);

    if (new_curr->which() == query_enum::COLUMN_FROM_MASK) {
      auto cc = new_curr->as<query_column_from_mask_p>();
      if (cc->mask_->which() == query_enum::COLUMN_TO_MASK) {
        auto mask_cc = cc->mask_->as<query_column_to_mask_p>();
        new_curr = mask_cc->source_column_;
      }
    }

    res_env->insert(make_pair(struct_hash(curr), new_curr));
    S.pop();
  }

  return res_env->at(struct_hash(x));
}

query_ordered_p sort_query(query_p x) {
  return sort_query(x, NONE<query_lambda_p>(), NONE<query_sort_map_p>());
}

DECL_STRUCT(eval_result_map);

struct eval_result_map {
  int64_t len_;
  vector<bool> done_;
  vector<bool> is_column_value_;
  vector<bool> is_column_builder_;

  vector<optional<value_p>> entries_;
  vector<optional<column_builder_p>> column_builder_entries_;
  vector<optional<value_p>> column_value_entries_;

  eval_result_map(int64_t len)
    : len_(len),
      done_(len, false),
      is_column_value_(len, false),
      is_column_builder_(len, false) {

    for (int64_t i = 0; i < len_; i++) {
      entries_.push_back(NONE<value_p>());
      column_builder_entries_.push_back(NONE<column_builder_p>());
      column_value_entries_.push_back(NONE<value_p>());
    }
  }
};

optional<int64_t> read_var_ref_index(const string& params_data) {
  istringstream is_params(params_data);
  read_bin<value_type_p>(is_params);
  auto var_ref_type = read_bin<int64_t>(is_params);
  auto var_ref_index = read_bin<int64_t>(is_params);
  if (var_ref_type == 0) {
    return NONE<int64_t>();
  } else if (var_ref_type == 1) {
    return SOME<int64_t>(var_ref_index);
  } else {
    cerr << var_ref_type << " " << var_ref_index << endl;
    AU();
  }
}

struct eval_stack_state {
  vector<pair<int64_t, int64_t>> context_;
  int64_t line_;
  eval_stack_state(int64_t line) : line_(line) { }
};

ostream& operator<<(ostream& os, const eval_stack_state& st) {
  for (auto p : st.context_) {
    os << "(" << p.first << "," << p.second << "):";
  }
  os << st.line_;
  return os;
}

eval_stack_state eval_stack_state_push(
  eval_stack_state st, int64_t line, int64_t iter) {
  auto ret = st;
  ret.context_.push_back(make_pair(line, iter));
  return ret;
}

inline value_p lookup_base(
  eval_result_map_p res,
  int64_t i,
  int64_t iter,
  bool deref_iter) {

  if (res->is_column_builder_[i]) {
    auto ci = *at(res->column_builder_entries_, i);
    ASSERT_TRUE(deref_iter);
    return ci->at(iter);
  } else if (res->is_column_value_[i]) {
    auto ci = *at(res->column_value_entries_, i);
    if (deref_iter) {
      return value_column_at(ci, iter);
    } else {
      return ci;
    }
  } else {
    return *at(res->entries_, i);
  }
}

inline value_p lookup_raw(
  query_ordered_p x,
  eval_result_map_p res,
  int64_t i,
  int64_t iter,
  optional<pair<int64_t, int64_t>> row_index_range,
  optional<value_p> outer_var,
  optional<vector<value_p>> outer_capture_vals,
  bool deref_iter) {

  auto xi = x->entries_[i];
  if (xi.which_ == query_enum::VARIABLE) {
    optional<int64_t> var_ref_index = NONE<int64_t>();
    var_ref_index = read_var_ref_index(xi.params_data_);
    if (!var_ref_index) {
      ASSERT_TRUE(!!outer_var == !row_index_range);
      if (!!outer_var) {
        return *outer_var;
      } else {
        return value_nd_vector::create_scalar_int64(iter);
      }
    } else {
      return at(*outer_capture_vals, *var_ref_index);
    }
  }

  return lookup_base(res, i, iter, deref_iter);
}

void eval_query_init(
  query_ordered_p x,
  eval_result_map_p res,
  optional<pair<int64_t, int64_t>> row_index_range,
  optional<value_p> outer_var,
  optional<vector<value_p>> outer_capture_vals,
  bool is_iter,
  optional<int64_t> iter_res_len) {

  auto lookup_raw_local = [&](int64_t i, int64_t iter, bool deref_iter) -> value_p {
    return lookup_raw(
      x, res, i, iter, row_index_range, outer_var, outer_capture_vals,
      deref_iter);
  };

  for (int64_t i = 0; i < len(x->entries_); i++) {
    auto xi = x->entries_[i];
    istringstream is_params(xi.params_data_);

    res->is_column_value_[i] = false;
    res->is_column_builder_[i] = is_iter;

    auto res_type = read_bin<value_type_p>(is_params);

    switch (xi.which_) {
    case query_enum::CONSTANT: {
      res->is_column_builder_[i] = false;
      auto v = read_bin_value(is_params, NONE<url_p>());
      res->entries_[i] = SOME(v);
      res->done_[i] = true;
      break;
    }
    case query_enum::VARIABLE:
    case query_enum::LAMBDA: {
      res->is_column_builder_[i] = false;
      res->done_[i] = true;
      break;
    }
    case query_enum::COLUMN_AT_INDEX: {
      if (is_iter) {
        auto index_arg = x->entries_[xi.input_ids_[1]];
        if (index_arg.which_ == query_enum::VARIABLE) {
          auto var_ref_index = read_var_ref_index(index_arg.params_data_);
          if (!var_ref_index) {
            if (!res->is_column_value_[xi.input_ids_[0]] &&
                !res->is_column_builder_[xi.input_ids_[0]]) {
              res->is_column_value_[i] = true;
              res->is_column_builder_[i] = false;
              res->column_value_entries_[i] = lookup_raw_local(
                xi.input_ids_[0], 0, false);
              res->done_[i] = true;
            }
          }
        }
      }
      break;
    }
    default:
      break;
    }

    if (res->is_column_builder_[i]) {
      auto builder_i = column_builder_create(res_type);
      builder_i->extend_length_raw(*iter_res_len);
      at(res->column_builder_entries_, i) = SOME(builder_i);
    }
  }
}

optional<value_p> eval_query(
  query_ordered_p x,
  eval_result_map_p res,
  eval_stack_state st,
  optional<pair<int64_t, int64_t>> row_index_range,
  optional<value_p> outer_var,
  optional<vector<value_p>> outer_capture_vals,
  int64_t worker_index) {

  auto lookup_raw_local = [&](int64_t i, int64_t iter, bool deref_iter) -> value_p {
    return lookup_raw(
      x, res, i, iter, row_index_range, outer_var, outer_capture_vals,
      deref_iter);
  };

  auto lookup = [&](int64_t i, int64_t iter) -> value_p {
    return lookup_raw_local(i, iter, true);
  };

  ASSERT_EQ(res->len_, x->entries_.size());
  for (int64_t i = 0; i < res->len_; i++) {
    auto xi = x->entries_[i];

    int64_t iter_lo = 0;
    int64_t iter_hi = 1;
    if (!!row_index_range) {
      iter_lo = row_index_range->first;
      iter_hi = row_index_range->second;
      if (!res->is_column_builder_[i] && iter_lo > 0) {
        continue;
      }
    }

    switch (xi.which_) {
    case query_enum::SCALAR_BUILTIN: {
      if (!!row_index_range) {
        istringstream is_params(xi.params_data_);
        auto xi_type = read_bin<value_type_p>(is_params);

        auto op = static_cast<scalar_builtin_enum>(read_bin<int64_t>(is_params));
        ASSERT_EQ(xi.input_ids_.size(), arity(op));

        bool is_fast_path = true;
        constexpr int64_t ARITY_MAX = 2;
        constexpr int64_t OUTPUT_SIZE_MAX = 16;
        vector<value_column*> column_args_fast(ARITY_MAX, nullptr);
        if (arity(op) > ARITY_MAX) {
          is_fast_path = false;
        }

        dtype_enum input_dtype = dtype_enum::I8;
        dtype_enum output_dtype = get_result_dtype(op, input_dtype);
        int64_t output_dtype_size = dtype_size_bytes(output_dtype);
        ASSERT_LE(output_dtype_size, OUTPUT_SIZE_MAX);

        for (int64_t r = 0; r < arity(op); r++) {
          if (res->is_column_value_[xi.input_ids_[r]]) {
            auto cr = value_deref(lookup_raw_local(
              xi.input_ids_[r], 0, false));
            auto cr_dtype = cr->get_type()
              ->as<value_type_column_p>()->element_type_
              ->as<value_type_nd_vector_p>()->dtype_;
            if (r == 0) {
              input_dtype = cr_dtype;
            } else {
              ASSERT_EQ(cr_dtype, input_dtype);
            }
            if (cr->which() == value_enum::COLUMN) {
              column_args_fast[r] = cr->as<value_column_p>().get();
            } else {
              is_fast_path = false;
            }
          } else {
            is_fast_path = false;
          }
        }

        if (is_fast_path) {
          ASSERT_EQ(arity(op), 2);
          void* src0 = nullptr;
          void* src1 = nullptr;
          char dst[OUTPUT_SIZE_MAX] = {0};
          column_builder* dst_final_fast =
            (*res->column_builder_entries_[i]).get();

          for (int64_t iter = iter_lo; iter < iter_hi; iter++) {
            src0 = column_args_fast[0]->at_raw(iter).addr_;
            src1 = column_args_fast[1]->at_raw(iter).addr_;
            eval_raw_binary(op, dst, src0, src1, input_dtype);
            dst_final_fast->put_raw(
              buffer(dst, output_dtype_size), iter, worker_index);
          }

          res->done_[i] = true;
        }
      }
      break;
    }
    default:
      break;
    }

    if (res->done_[i]) {
      continue;
    }

    if (!res->is_column_builder_[i]) {
      iter_lo = 0;
      iter_hi = 1;
    }

    optional<value_p> ri = NONE<value_p>();

    for (int64_t iter = iter_lo; iter < iter_hi; iter++) {
      istringstream is_params(xi.params_data_);
      auto xi_type = read_bin<value_type_p>(is_params);
      auto st_sub = st;
      st_sub.line_ = i;
      if (!!row_index_range) {
        ASSERT_GT(len(st_sub.context_), 0);
        auto p = st_sub.context_[len(st_sub.context_)-1];
        st_sub.context_[len(st_sub.context_)-1] = make_pair(p.first, iter);
      }

      switch (xi.which_) {
      case query_enum::CONSTANT:
      case query_enum::VARIABLE:
      case query_enum::LAMBDA: {
        AU();
        break;
      }
      case query_enum::COLUMN_LENGTH: {
        ASSERT_EQ(xi.input_ids_.size(), 1);
        auto v = lookup(xi.input_ids_[0], iter);
        ri = SOME(
          value_nd_vector::create_scalar_int64(v->get_column_length()));
        break;
      }
      case query_enum::COLUMN_AT_INDEX: {
        ASSERT_EQ(xi.input_ids_.size(), 2);
        auto cv = value_deref(lookup(xi.input_ids_[0], iter));
        auto ci = value_deref(lookup(xi.input_ids_[1], iter))
          ->as<value_nd_vector_p>()->value_scalar_int64();
        if (cv->which() == value_enum::REF) {
          ri = SOME(value_column_at_deref(cv, ci));
          // TODO: optimizations: only dereference if data is relatively small
        } else if (cv->which() == value_enum::COLUMN) {
          auto cc = cv->as<value_column_p>();
          ri = SOME(cc->at(ci));
        } else {
          cerr << cv->which() << endl;
          AU();
        }
        break;
      }
      case query_enum::COLUMN_GENERATOR: {
        auto fi = xi.input_ids_[0];
        auto f = *(at(x->entries_, fi).function_body_);
        auto res_len_v = lookup(xi.input_ids_[1], iter);
        auto res_len =
          res_len_v->as<value_nd_vector_p>()->value_scalar_int64();
        ASSERT_GE(res_len, 0);
        // cerr << "res_len: " << res_len << endl;

        auto res_type = read_bin<value_type_p>(is_params);
        auto res_map = make_shared<eval_result_map>(f->entries_.size());

        vector<value_p> capture_vals;
        for (auto ci : at(x->entries_, fi).input_ids_) {
          capture_vals.push_back(lookup(ci, iter));
        }

        eval_query_init(
          f, res_map, NONE<pair<int64_t, int64_t>>(), NONE<value_p>(),
          SOME(capture_vals), true, res_len);

        int64_t nt = turi::thread_pool::get_instance().size();
        int64_t chunk_size = ceil_divide(res_len, nt);

        turi::in_parallel_debug(
          [&](int64_t worker_index_sub, int64_t num_threads_actual) {
            ASSERT_EQ(num_threads_actual, nt);

            int64_t start_j = worker_index_sub * chunk_size;
            int64_t end_j = min<int64_t>((worker_index_sub+1) * chunk_size, res_len);

            int64_t block_size = (end_j - start_j);
            for (int64_t j = start_j; j < end_j; j += block_size) {
              int64_t lo = j;
              int64_t hi = min<int64_t>(end_j, j + block_size);
              eval_query(
                f, res_map, eval_stack_state_push(st_sub, i, j),
                SOME(make_pair(lo, hi)), NONE<value_p>(),
                SOME(capture_vals), worker_index_sub);
            }
          });

        ri = SOME(
          (*at(res_map->column_builder_entries_, f->return_value_))
          ->finalize());
        break;
      }
      case query_enum::COLUMN_REDUCE: {
        auto v = value_deref(lookup(xi.input_ids_[0], iter));
        auto result_type = read_bin<value_type_p>(is_params);
        auto reduce_op = read_bin<column_reduce_op_enum>(is_params);

        auto ret = reduce_op_init(reduce_op, result_type);
        value_column_iterate(v, [&](int64_t i, value_p vi) {
            ret = reduce_op_exec(reduce_op, ret, vi);
            return true;
          });

        ri = SOME(ret);
        break;
      }
      case query_enum::COLUMN_JOIN: {
        auto source_column = value_deref(lookup(xi.input_ids_[0], iter));
        auto source_index = value_deref(lookup(xi.input_ids_[1], iter));
        auto other_index = value_deref(lookup(xi.input_ids_[2], iter));
        auto mode = static_cast<column_join_mode>(read_bin<int64_t>(is_params));
        auto position =
          static_cast<column_join_position>(read_bin<int64_t>(is_params));

        if (mode != column_join_mode::INNER) {
          fmt(cerr, "Outer join not yet supported\n");
          AU();
        }

        auto cc_source_index = source_index->as<value_index_p>();

        auto ret = column_builder_create(
          source_column->get_type()->as<value_type_column_p>()->element_type_);

        value_column* source_column_fast = nullptr;
        auto source_opt = source_column->get_as_direct_column();
        if (!!source_opt) {
          source_column_fast = *source_opt;
        }

        value_index* source_index_fast =
          source_index->as<value_index_p>().get();
        value_index* other_index_fast =
          other_index->as<value_index_p>().get();

        auto iter_hashes = &source_index_fast->index_hashes_;
        if (position == column_join_position::RIGHT) {
          iter_hashes = &other_index_fast->index_hashes_;
        }

        for (int64_t i = 0; i < len(*iter_hashes); i++) {
          auto key_hash = at(*iter_hashes, i);
          pair<int64_t, int64_t> source_range = make_pair(0, 0);
          int64_t source_len = 0;
          pair<int64_t, int64_t> other_range = make_pair(0, 0);
          int64_t other_len = 0;

          auto source_range_it =
            source_index_fast->index_map_range_.find(key_hash);
          if (source_range_it != source_index_fast->index_map_range_.end(
                key_hash)) {
            source_range = source_range_it->second;
            source_len = source_range.second - source_range.first;
          }

          auto other_range_it = other_index_fast->index_map_range_.find(
            key_hash);
          if (other_range_it !=
              other_index_fast->index_map_range_.end(key_hash)) {
            other_range = other_range_it->second;
            other_len = other_range.second - other_range.first;
          }

          if (source_len == 0 || other_len == 0) {
            continue;
          }

          if (position == column_join_position::RIGHT) {
            for (int64_t j = source_range.first; j < source_range.second; j++) {
              auto vj = value_column_at(
                source_index_fast->index_values_flat_, j);
              for (int64_t k = 0; k < other_len; k++) {
                if (!!source_column_fast) {
                  ret->append_raw(
                    source_column_fast->at_raw(vj->get_value_scalar_int64()));
                } else {
                  ret->append(
                    value_column_at(
                      source_column, vj->get_value_scalar_int64()));
                }
              }
            }
          } else if (position == column_join_position::LEFT) {
            for (int64_t k = 0; k < other_len; k++) {
              for (int64_t j = source_range.first; j < source_range.second; j++) {
                auto vj = value_column_at(
                  source_index_fast->index_values_flat_, j);
                if (!!source_column_fast) {
                  ret->append_raw(
                    source_column_fast->at_raw(vj->get_value_scalar_int64()));
                } else {
                  ret->append(
                    value_column_at(
                      source_column, vj->get_value_scalar_int64()));
                }
              }
            }
          } else {
            AU();
          }
        }

        ri = SOME(ret->finalize());
        break;
      }
      case query_enum::EQUALS: {
        ASSERT_EQ(xi.input_ids_.size(), 2);
        auto x0 = lookup(xi.input_ids_[0], iter);
        auto x1 = lookup(xi.input_ids_[1], iter);
        ri = SOME(value_nd_vector::create_scalar_bool(value_eq(x0, x1)));
        break;
      }
      case query_enum::SCALAR_BUILTIN: {
        auto op = static_cast<scalar_builtin_enum>(read_bin<int64_t>(is_params));
        ASSERT_EQ(xi.input_ids_.size(), arity(op));
        vector<value_p> args;
        ASSERT_EQ(arity(op), xi.input_ids_.size());
        for (int64_t i = 0; i < arity(op); i++) {
          auto v = lookup(xi.input_ids_[i], iter);
          args.push_back(value_deref(v));
        }
        ri = SOME(eval(op, args));
        break;
      }
      case query_enum::COLUMN_TO_MASK: {
        ASSERT_EQ(xi.input_ids_.size(), 2);
        value_p source_column_raw = value_deref(
          lookup(xi.input_ids_[0], iter));
        int64_t result_length = value_deref(lookup(xi.input_ids_[1], iter))
          ->as<value_nd_vector_p>()->value_scalar_int64();
        vector<bool> res(result_length, false);
        value_column_iterate(source_column_raw, [&](int64_t i, value_p vi) {
            int64_t ii = vi->as<value_nd_vector_p>()->value_scalar_int64();
            ASSERT_GE(ii, 0);
            ASSERT_LT(ii, result_length);
            res[ii] = true;
            return true;
          });
        auto rb = column_builder_create(
          value_type::create_scalar(dtype_enum::BOOL));
        for (int64_t k = 0; k < result_length; k++) {
          rb->append(value_nd_vector::create_scalar_bool(res[k]));
        }
        ri = SOME(rb->finalize());
        break;
      }
      case query_enum::COLUMN_FROM_MASK: {
        ASSERT_EQ(xi.input_ids_.size(), 1);
        value_p ci_raw = value_deref(lookup(xi.input_ids_[0], iter));
        auto ci = ci_raw->as<value_column_p>();
        auto rb = column_builder_create(
          value_type::create_scalar(dtype_enum::I64));
        for (int64_t k = 0; k < ci->length(); k++) {
          if (ci->at(k)->as<value_nd_vector_p>()->value_scalar_bool()) {
            rb->append(value_nd_vector::create_scalar_int64(k));
          }
        }
        ri = SOME(rb->finalize());
        break;
      }
      case query_enum::COLUMN_AT_COLUMN: {
        ASSERT_EQ(xi.input_ids_.size(), 2);
        value_p ci = lookup(xi.input_ids_[0], iter);
        value_p ii = lookup(xi.input_ids_[1], iter);
        bool handled = false;

        if (ci->which() == value_enum::REF) {
          auto ci_ref = ci->as<value_ref_p>();
          switch (ci_ref->ref_which_) {
          case value_ref_enum::VALUE:
          case value_ref_enum::COLUMN_ELEMENT: {
            ci = value_deref(ci);
            break;
          }
          case value_ref_enum::COLUMN_RANGE: {
            fmt(cerr, "Subset of COLUMN_RANGE not yet supported\n");
            AU();
            handled = true;
            break;
          }
          case value_ref_enum::COLUMN_SUBSET: {
            fmt(cerr, "Subset of COLUMN_SUBSET not yet supported\n");
            AU();
            handled = true;
            break;
          }
          default:
            break;
          }
        }

        if (!handled) {
          ASSERT_EQ(ci->which(), value_enum::COLUMN);
          ri = SOME(
            value_ref::create_column_subset(
              ci,
              ii));
          break;
        }
      }
      case query_enum::RECORD_AT_FIELD: {
        ASSERT_EQ(xi.input_ids_.size(), 1);
        auto xr = value_deref(lookup(xi.input_ids_[0], iter));
        auto fi = read_bin<int64_t>(is_params);
        ri = SOME(at(xr->as<value_record_p>()->entries_, fi));
        break;
      }
      case query_enum::RECORD_FROM_FIELDS: {
        int64_t num_fields = xi.input_ids_.size();
        vector<value_p> field_vals;
        for (int64_t fi = 0; fi < num_fields; fi++) {
          field_vals.push_back(lookup(xi.input_ids_[fi], iter));
        }
        ri = SOME(value::create_record(xi_type, field_vals));
        break;
      }
      case query_enum::BUILD_INDEX: {
        int64_t n = xi.input_ids_.size();
        ASSERT_GE(n, 1);
        vector<value_p> source_columns;
        for (int64_t i = 0; i < n; i++) {
          auto source_column = value_deref(lookup(xi.input_ids_[i], iter));
          source_columns.push_back(source_column);
        }
        auto index_mode = read_bin<index_mode_enum>(is_params);
        ri = SOME(value::build_index(source_columns, index_mode));
        break;
      }
      case query_enum::INDEX_GET_KEYS: {
        ASSERT_EQ(xi.input_ids_.size(), 1);
        auto source_index = value_deref(lookup(xi.input_ids_[0], iter));
        ri = SOME(source_index->as<value_index_p>()->index_keys_);
        break;
      }
      case query_enum::INDEX_GET_VALUES: {
        ASSERT_EQ(xi.input_ids_.size(), 1);
        auto source_index = value_deref(lookup(xi.input_ids_[0], iter));
        ri = SOME(source_index->as<value_index_p>()->index_values_grouped_);
        break;
      }
      case query_enum::INDEX_LOOKUP: {
        int64_t n = xi.input_ids_.size() - 1;
        ASSERT_GE(n, 0);
        auto index = value_deref(lookup(xi.input_ids_[0], iter));
        vector<value_p> keys;
        for (int64_t i = 0; i < n; i++) {
          auto key = value_deref(lookup(xi.input_ids_[1+i], iter));
          keys.push_back(key);
        }
        auto mode = read_bin<index_lookup_mode_enum>(is_params);
        ri = SOME(value::index_lookup(index, keys, mode));
        break;
      }
      default:
        cerr << "Query case not yet supported: " << xi.which_ << endl;
        AU();
      }

      if (!!ri) {
        // cerr << "Main query eval complete: " << st_sub << endl;
        if (res->is_column_builder_[i]) {
          (*res->column_builder_entries_[i])->put(*ri, iter, worker_index);
        } else {
          ASSERT_TRUE(!res->is_column_value_[i]);
          res->entries_[i] = ri;
        }
      }
    }
  }

  if (!!row_index_range) {
    return NONE<value_p>();
  } else {
    return SOME(lookup(x->return_value_, 0));
  }
}

value_p eval(query_p x) {
  x = optimize_query(x);
  auto xs = sort_query(x, NONE<query_lambda_p>(), NONE<query_sort_map_p>());
  int64_t len = xs->entries_.size();
  auto res_map = make_shared<eval_result_map>(len);
  eval_query_init(
    xs, res_map, NONE<pair<int64_t, int64_t>>(), NONE<value_p>(),
    NONE<vector<value_p>>(), false, NONE<int64_t>());
  auto res = eval_query(
    xs, res_map, eval_stack_state(0),
    NONE<pair<int64_t, int64_t>>(), NONE<value_p>(), NONE<vector<value_p>>(), 0);
  return *res;
}

}}
