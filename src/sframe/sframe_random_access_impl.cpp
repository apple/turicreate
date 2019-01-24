/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <sframe/sframe_random_access.hpp>

#include <sframe/sarray.hpp>
#include <sframe_query_engine/experimental/sframe_random_access_query_impl.hpp>
#include <unity/toolkits/util/random_sframe_generation.hpp>
#include <util/basic_types.hpp>
#include <util/fs_util.hpp>

using turi::fs_util::copy_directory_recursive;
using turi::fs_util::make_directories_strict;

using std::istringstream;
using std::make_pair;
using std::max;
using std::min;
using std::set;

namespace turi { namespace sframe_random_access {

value_type_p value_type::create_column(
  value_type_p element_type, optional<int64_t> length, bool known_unique) {

  return value_type::create(
    value_type_column_create(element_type, length, known_unique));
}

value_type_p value_type::create_bool_column() {
  return value_type::create_column(
    value_type::create_scalar(dtype_enum::BOOL), NONE<int64_t>(), false);
}

value_type_p value_type::create_nd_vector(int64_t ndim, dtype_enum dtype) {
  return value_type::create(value_type_nd_vector::create(ndim, dtype));
}

value_type_p value_type::create_string() {
  return value_type::create(
    value_type_nd_vector::create(1, dtype_enum::I8),
    value_type_tag_enum::STRING);
}

value_type_p value_type::create_image() {
  return value_type::create(
    value_type_nd_vector::create(1, dtype_enum::I8),
    value_type_tag_enum::IMAGE);
}

value_type_p value_type::create_scalar(dtype_enum dtype) {
  return value_type::create_nd_vector(0, dtype);
}

value_type_p value_type::create_record(
  vector<pair<string, value_type_p>> field_types) {
  return value_type::create(
    make_shared<value_type_record>(field_types));
}

value_type_p value_type::create_data_table(
  vector<pair<string, value_type_p>> field_types) {
  return value_type::create(
    make_shared<value_type_record>(field_types));
}

value_type_p value_type::create_empty_record() {
  return value_type::create_record(vector<pair<string, value_type_p>>());
}

value_type_p value_type::create_optional(value_type_p some_ty) {
  vector<pair<string, value_type_p>> case_types;

  case_types.push_back(
    make_pair(string("None"), value_type::create_empty_record()));
  case_types.push_back(
    make_pair(string("Some"), some_ty));

  return value_type::create(
    make_shared<value_type_either>(case_types),
    value_type_tag_enum::OPTIONAL);
}

value_type_p value_type::create_function(
  value_type_p left, value_type_p right) {
  return value_type::create(make_shared<value_type_function>(left, right));
}

value_type_p value_type::create_index(
  vector<value_type_p> source_column_types, index_mode_enum index_mode) {

  return value_type::create(
    make_shared<value_type_index>(source_column_types, index_mode));
}

value_type_enum value_type::which() {
  return static_cast<value_type_enum>(v_.which());
}

string value_type::which_str() {
  ostringstream os;
  os << this->which();
  return os.str();
}

optional<value_type_p> value_type::unpack_optional_ext() {
  if (!tag_ || *tag_ != value_type_tag_enum::OPTIONAL) {
    return NONE<value_type_p>();
  }

  auto cc = this->as<value_type_either_p>();
  return SOME(cc->case_types_[1].second);
}

bool value_type::is_optional() {
  return !!unpack_optional_ext();
}

value_type_p value_type::unpack_optional() {
  return *unpack_optional_ext();
}

vector<pair<string, value_type_p>> value_type::as_record_items() const {
  auto cc = this->as<value_type_record_p>();
  return cc->field_types_;
}

pair<int64_t, dtype_enum> value_type::as_nd_vector_items() const {
  auto cc = this->as<value_type_nd_vector_p>();
  return make_pair(cc->ndim_, cc->dtype_);
}

string value_type::struct_hash() {
  ostringstream os;
  write_bin(os, shared_from_this());
  return hash_string_value(os.str());
}

bool struct_eq(value_type_p x, value_type_p y) {
  return x->struct_hash() == y->struct_hash();
}

void write_bin(ostream& os, value_type_p x, type_specializer<value_type_p>) {
  write_object_header(os, x.get());
  auto which = static_cast<value_type_enum>(x->v_.which());
  write_bin(os, which);
  write_bin(os, x->tag_);

  switch (which) {
  case value_type_enum::COLUMN:
    x->as<value_type_column_p>()->save_sub(os);
    break;
  case value_type_enum::ND_VECTOR:
    x->as<value_type_nd_vector_p>()->save_sub(os);
    break;
  case value_type_enum::RECORD:
    x->as<value_type_record_p>()->save_sub(os);
    break;
  case value_type_enum::EITHER:
    x->as<value_type_either_p>()->save_sub(os);
    break;
  case value_type_enum::FUNCTION:
    x->as<value_type_function_p>()->save_sub(os);
    break;
  case value_type_enum::INDEX:
    x->as<value_type_index_p>()->save_sub(os);
    break;
  default:
    cerr << "Unrecognized value_type_enum: " << int64_t(x->v_.which()) << endl;
    AU();
  }
}

value_type_p read_bin(istream& is, type_specializer<value_type_p>) {
  read_object_header_check<value_type>(is);
  auto which = read_bin<value_type_enum>(is);
  auto tag = read_bin<optional<value_type_tag_enum>>(is);

  switch (which) {
  case value_type_enum::COLUMN:
    return value_type::create(value_type_column::load_sub(is), tag);
  case value_type_enum::ND_VECTOR:
    return value_type::create(value_type_nd_vector::load_sub(is), tag);
  case value_type_enum::RECORD:
    return value_type::create(value_type_record::load_sub(is), tag);
  case value_type_enum::EITHER:
    return value_type::create(value_type_either::load_sub(is), tag);
  case value_type_enum::FUNCTION:
    return value_type::create(value_type_function::load_sub(is), tag);
  case value_type_enum::INDEX:
    return value_type::create(value_type_index::load_sub(is), tag);
  default:
    cerr << "Unrecognized value_type_enum: " << int64_t(which) << endl;
    AU();
  }
}

ostream& operator<<(ostream& os, value_type_p x) {
  switch (x->which()) {
  case value_type_enum::COLUMN: {
    auto cc = x->as<value_type_column_p>();
    os << "[" << cc->element_type_;
    if (!!cc->length_) {
      os << ":" << (*cc->length_);
    }
    if (cc->known_unique_) {
      os << "!";
    }
    os << "]";
    break;
  }

  case value_type_enum::ND_VECTOR: {
    auto cc = x->as<value_type_nd_vector_p>();

    if (!!x->tag_) {
      if (*(x->tag_) == value_type_tag_enum::STRING) {
        ASSERT_EQ(cc->ndim_, 1);
        ASSERT_EQ(cc->dtype_, dtype_enum::I8);
        os << "str";
      } else if (*(x->tag_) == value_type_tag_enum::IMAGE) {
        ASSERT_EQ(cc->ndim_, 1);
        ASSERT_EQ(cc->dtype_, dtype_enum::I8);
        os << "image";
      } else {
        AU();
      }
    } else {
      if (cc->ndim_ == 0 && cc->dtype_ == dtype_enum::I64) {
        os << "int";
      } else {
        os << dtype_to_char(cc->dtype_) << cc->ndim_;
      }
    }
    break;
  }

  case value_type_enum::RECORD: {
    auto cc = x->as<value_type_record_p>();

    if (!!x->tag_) {
      if (*(x->tag_) == value_type_tag_enum::DATA_TABLE) {
        os << "Table ";
      } else {
        AU();
      }
    }

    os << "{";
    for (int64_t i = 0; i < len(cc->field_types_); i++) {
      auto p = cc->field_types_[i];
      if (i > 0) {
        os << ", ";
      }
      os << p.first << ": " << p.second;
    }
    os << "}";
    break;
  }

  case value_type_enum::EITHER: {
    if (x->is_optional()) {
      os << x->unpack_optional() << "?";
    } else {
      fmt(cerr, "General sum types not yet supported\n");
      AU();
    }
    break;
  }

  case value_type_enum::FUNCTION: {
    auto cc = x->as<value_type_function_p>();
    os << cc->left_ << " -> " << cc->right_;
    break;
  }

  case value_type_enum::INDEX: {
    os << "<index>";
    break;
  }

  default:
    cerr << x->which() << endl;
    AU();
    break;
  }

  return os;
}

/**
 * Returns true if a given \ref value_type is valid for a target \ref
 * value_type, i.e., if it is a subtype of the target type.
 */
bool type_valid(value_type_p target, value_type_p sub) {
  if (target->which() != sub->which()) {
    return false;
  }

  if (!!target->tag_ && (!sub->tag_ || (*target->tag_ != *sub->tag_))) {
    return false;
  }

  switch (target->which()) {
  case value_type_enum::COLUMN: {
    auto ct = target->as<value_type_column_p>();
    auto cs = sub->as<value_type_column_p>();

    if (!type_valid(ct->element_type_, cs->element_type_)) {
      return false;
    }

    if (!!ct->length_ && (!cs->length_ || (*cs->length_ != *ct->length_))) {
      return false;
    }

    if (ct->known_unique_ && !cs->known_unique_) {
      return false;
    }

    return true;
    break;
  }

  case value_type_enum::ND_VECTOR: {
    auto ct = target->as<value_type_nd_vector_p>();
    auto cs = sub->as<value_type_nd_vector_p>();
    return ((ct->ndim_ == cs->ndim_) && (ct->dtype_ == cs->dtype_));
    break;
  }

  case value_type_enum::RECORD: {
    auto ct = target->as<value_type_record_p>();
    auto cs = sub->as<value_type_record_p>();

    int64_t n = len(ct->field_types_);
    if (len(cs->field_types_) != n) {
      return false;
    }

    for (int64_t i = 0; i < n; i++) {
      if (ct->field_types_[i].first != cs->field_types_[i].first) {
        return false;
      }

      if (!type_valid(
            ct->field_types_[i].second, cs->field_types_[i].second)) {
        return false;
      }
    }

    return true;
    break;
  }

  case value_type_enum::EITHER: {
    if (target->is_optional() && sub->is_optional()) {
      return type_valid(target->unpack_optional(), sub->unpack_optional());
    } else {
      fmt(cerr, "Non-optional sum types not yet supported\n");
      AU();
    }
    break;
  }

  case value_type_enum::FUNCTION: {
    AU();
    break;
  }

  default:
    cerr << target->which() << endl;
    AU();
    break;
  }
}

void assert_type_valid(value_type_p target, value_type_p sub) {
  bool is_valid = type_valid(target, sub);
  if (!is_valid) {
    fmt(
      cerr,
      " *** Type mismatch\n"
      "     Expected: %v\n"
      "     Received: %v\n", target, sub);
    AU();
  }
}

value_type_p value_type_create_nd_vector(int64_t ndim, const string& dtype_str) {
  return value_type::create_nd_vector(ndim, dtype_from_char(dtype_str));
}

ostream& operator<<(ostream& os, value_enum x) {
  switch (x) {
  case value_enum::COLUMN: os << "COLUMN"; break;
  case value_enum::ND_VECTOR: os << "ND_VECTOR"; break;
  case value_enum::RECORD: os << "RECORD"; break;
  case value_enum::EITHER: os << "EITHER"; break;
  case value_enum::REF: os << "REF"; break;
  case value_enum::INDEX: os << "INDEX"; break;
  case value_enum::THUNK: os << "THUNK"; break;
  default:
    cerr << static_cast<int64_t>(x) << endl;
    AU();
  }

  return os;
}

ostream& operator<<(ostream& os, value_ref_enum x) {
  switch (x) {
  case value_ref_enum::VALUE: os << "VALUE"; break;
  case value_ref_enum::COLUMN_ELEMENT: os << "COLUMN_ELEMENT"; break;
  case value_ref_enum::COLUMN_RANGE: os << "COLUMN_RANGE"; break;
  case value_ref_enum::COLUMN_SUBSET: os << "COLUMN_SUBSET"; break;
  default:
    cerr << static_cast<int64_t>(x) << endl;
    AU();
  }

  return os;
}

ref_context_p ref_context::create() {
  return make_shared<ref_context>();
}

ostream& operator<<(ostream& os, column_format_enum x) {
  switch (x) {
  case column_format_enum::VARIABLE:  os << "VARIABLE"; break;
  default:
    os << static_cast<int64_t>(x) << endl;
    AU();
  }
  return os;
}

column_builder::column_builder(
  value_type_p entry_type, column_format_enum format) {

  entry_type_ = entry_type;
  top_acc_ = make_shared<binary_data_builder_fixed>();
  entries_acc_ = make_shared<binary_data_builder_variable>(
    turi::thread_pool::get_instance().size());
  format_ = format;
  ref_context_ = ref_context::create();
  is_finalized_ = false;

  ASSERT_TRUE(format_ == column_format_enum::VARIABLE);

  num_entries_current_ = 0;
}

void column_builder::put(const value_p& entry, int64_t i, int64_t worker_index) {
  string entry_str;
  {
    ostringstream os;
    entry->save_raw(os, SOME(ref_context_), NONE<unordered_set<int64_t>*>());
    entry_str = os.str();
  }

  this->put_raw(buffer(&entry_str[0], entry_str.length()), i, worker_index);
}

void column_builder::append_raw(buffer src) {
  ASSERT_TRUE(!is_finalized_);
  int64_t start_index = num_entries_current_;
  this->extend_length_raw(start_index + 1);
  this->put_raw(src, start_index, 0);
}

void column_builder::append(const value_p& entry) {
  ASSERT_TRUE(!is_finalized_);
  int64_t start_index = num_entries_current_;
  this->extend_length_raw(start_index + 1);

  assert_type_valid(entry_type_, entry->ty_);

  string entry_str;
  {
    ostringstream os;
    entry->save_raw(os, SOME(ref_context_), NONE<unordered_set<int64_t>*>());
    entry_str = os.str();
  }

  constexpr int64_t worker_index = 0;
  bin_handle h = entries_acc_->append(
    &entry_str[0], entry_str.length(), worker_index);
  {
    ostringstream os;
    write_bin(os, h.index_);
    write_bin(os, h.offset_);
    write_bin(os, h.len_);
    string table_entry_str = os.str();

    ASSERT_EQ(table_entry_str.size(), COLUMN_TABLE_ENTRY_SIZE_BYTES);

    top_acc_->put_data(
      get_table_entry_offset(start_index),
      &table_entry_str[0],
      COLUMN_TABLE_ENTRY_SIZE_BYTES
      );
  }
}

value_p column_builder::finalize(bool known_unique) {
  ASSERT_TRUE(!is_finalized_);

  int64_t num_entries_final = num_entries_current_;

  auto res_type = value_type::create_column(
    entry_type_, SOME<int64_t>(num_entries_final), known_unique);

  ASSERT_TRUE(format_ == column_format_enum::VARIABLE);

  auto meta_acc = make_shared<binary_data_builder_fixed>();

  meta_acc->append_object_header(static_cast<value*>(nullptr));
  meta_acc->append_value(res_type);
  meta_acc->append_value(value_enum::COLUMN);
  meta_acc->append_value(format_);

  auto top_view = make_shared<binary_data_view_fixed>(
    top_acc_->block_handle_);
  auto meta_view = make_shared<binary_data_view_fixed>(
    meta_acc->block_handle_);
  auto entries_view = make_shared<binary_data_view_variable>(
    entries_acc_->block_handles_);

  return value_column::load_column_from_binary_data(
    meta_view,
    top_view,
    entries_view,
    ref_context_,
    NONE<url_p>(),
    NONE<int64_t>());
}

value_p column_builder::finalize() {
  return this->finalize(false);
}

void table_builder::append(const vector<value_p>& entries) {
  ASSERT_EQ(entries.size(), column_builders_.size());
  for (int64_t i = 0; i < len(entries); i++) {
    column_builders_[i]->append(entries[i]);
  }
}

value_p table_builder::finalize() {
  vector<value_type_p> ret_element_types;
  vector<value_p> ret_columns;
  for (int64_t i = 0; i < len(column_builders_); i++) {
    ret_element_types.push_back(column_builders_[i]->entry_type_);
    ret_columns.push_back(column_builders_[i]->finalize());
  }
  auto ret_type = value_type_table_create(column_names_, ret_element_types);
  auto ret = value::create(
    make_shared<value_record>(ret_type, ret_columns),
    ret_type,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
  is_finalized_ = true;
  return ret;
}

column_view_variable::column_view_variable(
  string base_path, optional<url_p> url_context)
  : url_context_(url_context) {

  meta_view_ = make_shared<binary_data_view_fixed>(base_path + "/meta");
  top_view_ = make_shared<binary_data_view_fixed>(base_path + "/top");
  entries_view_ = make_shared<binary_data_view_variable>(
    base_path + "/entries");

  auto is_meta = meta_view_->get_istream();

  read_object_header_check<value>(*is_meta);
  type_ = read_bin<value_type_p>(*is_meta);
  entry_type_ = type_->as<value_type_column_p>()->element_type_;
  num_entries_ = *(type_->as<value_type_column_p>()->length_);
  auto which = read_bin<value_enum>(*is_meta);
  ASSERT_TRUE(which == value_enum::COLUMN);
  format_ = read_bin<column_format_enum>(*is_meta);
  ASSERT_TRUE(format_ == column_format_enum::VARIABLE);
}

column_view_variable::column_view_variable(
  binary_data_view_fixed_p meta_view,
  binary_data_view_fixed_p top_view,
  binary_data_view_variable_p entries_view,
  optional<url_p> url_context)
  : meta_view_(meta_view),
    top_view_(top_view),
    entries_view_(entries_view),
    url_context_(url_context) {

  auto is_meta = meta_view_->get_istream();

  read_object_header_check<value>(*is_meta);
  type_ = read_bin<value_type_p>(*is_meta);
  entry_type_ = type_->as<value_type_column_p>()->element_type_;
  num_entries_ = *(type_->as<value_type_column_p>()->length_);
  auto which = read_bin<value_enum>(*is_meta);
  ASSERT_TRUE(which == value_enum::COLUMN);
  format_ = read_bin<column_format_enum>(*is_meta);
  ASSERT_TRUE(format_ == column_format_enum::VARIABLE);
}

value_p column_view_variable::at(int64_t i) {
  ASSERT_GE(i, 0);
  ASSERT_LT(i, num_entries_);

  auto h_str = top_view_->get_data_string(
    get_table_entry_offset(i), COLUMN_TABLE_ENTRY_SIZE_BYTES);
  bin_handle h;
  {
    istringstream is(h_str);
    h.index_ = read_bin<int64_t>(is);
    h.offset_ = read_bin<int64_t>(is);
    h.len_ = read_bin<int64_t>(is);
  }

  auto data_str = entries_view_->get_data_string(h);
  {
    istringstream is(data_str);
    value_p ret = value::load_raw(is, entry_type_, url_context_);
    return ret;
  }
}

value_p column_builder::at(int64_t i) {
  ASSERT_GE(i, 0);
  ASSERT_LT(i, num_entries_current_);

  auto h_str = top_acc_->get_data_string(
    get_table_entry_offset(i), COLUMN_TABLE_ENTRY_SIZE_BYTES);
  bin_handle h;
  {
    istringstream is(h_str);
    h.index_ = read_bin<int64_t>(is);
    h.offset_ = read_bin<int64_t>(is);
    h.len_ = read_bin<int64_t>(is);
  }

  auto data_str = entries_acc_->get_data_string(h);
  {
    istringstream is(data_str);
    value_p ret = value::load_raw(is, entry_type_, NONE<url_p>());
    return ret;
  }
}

ostream& operator<<(ostream& os, column_reduce_op_enum reduce_op) {
  switch (reduce_op) {
  case column_reduce_op_enum::SUM:  os << "SUM"; return os;
  default:
    AU();
  }
}

value_p reduce_op_init(
  column_reduce_op_enum reduce_op, value_type_p result_type) {

  ASSERT_EQ(result_type->which(), value_type_enum::ND_VECTOR);
  auto dtype = result_type->as<value_type_nd_vector_p>()->dtype_;

  if (dtype == dtype_enum::I64) {
    if (reduce_op == column_reduce_op_enum::SUM) {
      return value_nd_vector::create_scalar_int64(0);
    }
  } else if (dtype == dtype_enum::F64) {
    if (reduce_op == column_reduce_op_enum::SUM) {
      return value_nd_vector::create_scalar_float64(0.0);
    }
  }

  cerr << "Reduce operation not yet supported" << endl;
  AU();
}

value_p reduce_op_exec(
  column_reduce_op_enum reduce_op, value_p lhs, value_p rhs) {

  auto val_type = lhs->ty_;
  ASSERT_EQ(val_type->which(), value_type_enum::ND_VECTOR);
  auto dtype = val_type->as<value_type_nd_vector_p>()->dtype_;

  if (dtype == dtype_enum::I64) {
    auto lhs_val = lhs->get_value_scalar_int64();
    auto rhs_val = rhs->get_value_scalar_int64();
    if (reduce_op == column_reduce_op_enum::SUM) {
      return value_nd_vector::create_scalar_int64(lhs_val + rhs_val);
    }
  } else if (dtype == dtype_enum::F64) {
    auto lhs_val = lhs->get_value_scalar_float64();
    auto rhs_val = rhs->get_value_scalar_float64();
    if (reduce_op == column_reduce_op_enum::SUM) {
      return value_nd_vector::create_scalar_float64(lhs_val + rhs_val);
    }
  }

  cerr << "Reduce operation not yet supported" << endl;
  AU();
}

group_by_spec_p group_by_spec::create_original_table() {
  return make_shared<group_by_spec>(
    group_by_spec_v(
      make_shared<group_by_spec_original_table>()));
}

group_by_spec_p group_by_spec_create_reduce(
  column_reduce_op_enum reduce_op, query_p source_column) {

  return make_shared<group_by_spec>(
    group_by_spec_v(
      make_shared<group_by_spec_reduce>(reduce_op, source_column)));
}

group_by_spec_p group_by_spec::create_reduce(
  string reduce_op_str, value_p source_column) {

  auto reduce_op = reduce_op_enum_from_string(reduce_op_str);
  return group_by_spec_create_reduce(
    reduce_op, query::from_value(source_column));
}

group_by_spec_p group_by_spec_create_select_one(query_p source_column) {
  return make_shared<group_by_spec>(
    group_by_spec_v(
      make_shared<group_by_spec_select_one>(source_column)));
}

group_by_spec_p group_by_spec::create_select_one(value_p source_column) {
  return group_by_spec_create_select_one(query::from_value(source_column));
}

ostream& operator<<(ostream& os, group_by_spec_enum x) {
  switch (x) {
  case group_by_spec_enum::ORIGINAL_TABLE:  os << "ORIGINAL_TABLE"; return os;
  case group_by_spec_enum::REDUCE:          os << "REDUCE"; return os;
  case group_by_spec_enum::SELECT_ONE:      os << "SELECT_ONE"; return os;
  default:
    AU();
  }
}

column_reduce_op_enum reduce_op_enum_from_string(string x) {
  if (x == "SUM") {
    return column_reduce_op_enum::SUM;
  } else {
    fmt(cerr, "Reduce operation not recognized: %v\n", x);
    AU();
  }
}

value::value(value_v v, value_type_p ty, optional<ref_context_p> accum_refs,
             optional<url_p> url_context, optional<int64_t> id)
  : v_(v), ty_(ty), url_context_(url_context), value_id_(id),
    ref_context_(accum_refs) {

  if (!!url_context) {
    ASSERT_EQ(v.which(), value_enum::COLUMN);
  }
}

void write_bin_value(
  ostream& os, value_p x, optional<ref_context_p> ctx,
  optional<unordered_set<int64_t>*> local_refs_acc) {

  write_object_header(os, x.get());
  write_bin(os, x->ty_);
  x->save_raw(os, ctx, local_refs_acc);
}

value_p read_bin_value(istream& is, optional<url_p> load_url) {
  read_object_header_check<value>(is);
  auto type = read_bin<value_type_p>(is);

  return value::load_raw(is, type, load_url);
}

void value::save_raw(
  ostream& os, optional<ref_context_p> ctx,
  optional<unordered_set<int64_t>*> local_refs_acc) {

  if (!!local_refs_acc) {
    if (!!ref_context_) {
      lock_guard<recursive_mutex> lock {
        (*ref_context_)->ref_targets_lock_ };
      for (const auto& x : (*ref_context_)->ref_targets_) {
        (*local_refs_acc)->insert(x->get_value_id());
      }
    }
  }

  auto which = static_cast<value_enum>(v_.which());

  if (!ty_->known_direct_) {
    write_bin(os, which);
  }

  switch (which) {
  case value_enum::ND_VECTOR: {
    auto cc = this->as<value_nd_vector_p>();
    if (cc->shape_.size() > 0) {
      write_bin(os, cc->shape_);
    }
    if (cc->contiguous_) {
      os.write(
        reinterpret_cast<char*>(cc->base_addr_),
        product(cc->shape_) * dtype_size_bytes(cc->dtype_));
    } else {
      fmt(cerr, "Non-contiguous nd_vectors (e.g. slices) not yet supported\n");
      AU();
    }
    break;
  }

  case value_enum::RECORD: {
    auto cc = this->as<value_record_p>();
    for (int64_t i = 0; i < len(cc->entries_); i++) {
      cc->entries_[i]->save_raw(os, ctx, local_refs_acc);
    }
    break;
  }

  case value_enum::EITHER: {
    auto cc = this->as<value_either_p>();
    write_bin(os, cc->val_which_);
    cc->val_data_->save_raw(os, ctx, local_refs_acc);
    break;
  }

  case value_enum::COLUMN: {
    auto target = shared_from_this();

    auto id_mode = value_ref_location_enum::LOCAL;
    if (!!target->url_context_) {
      id_mode = value_ref_location_enum::SFRAME_URL;
    }

    write_bin(os, id_mode);

    if (id_mode == value_ref_location_enum::LOCAL) {
      auto id = target->get_value_id();
      write_bin<int64_t>(os, id);
      if (!!local_refs_acc) {
        (*local_refs_acc)->insert(id);
      }
    } else if (id_mode == value_ref_location_enum::SFRAME_URL) {
      auto id = target->get_value_id();
      write_bin<string>(os, (*target->url_context_)->url_path_);
      write_bin<int64_t>(os, id);
    } else {
      AU();
    }

    if (!!ctx) {
      (*ctx)->enroll_ref_target(target);
    }

    break;
  }

  case value_enum::REF: {
    auto cc = this->as<value_ref_p>();

    write_bin(os, cc->ref_which_);

    if (cc->ref_which_ == value_ref_enum::VALUE) {
      AU();
    } else {
      if (!!cc->target_) {
        write_bin(os, int8_t(1));
        write_bin_value(os, *cc->target_, ctx, local_refs_acc);
      } else {
        write_bin(os, int8_t(0));
      }

      write_bin(os, cc->column_element_);
      write_bin(os, cc->column_range_lo_);
      write_bin(os, cc->column_range_hi_);

      if (!!cc->column_subset_) {
        write_bin(os, int8_t(1));
        write_bin_value(os, *cc->column_subset_, ctx, local_refs_acc);
      } else {
        write_bin(os, int8_t(0));
      }
    }

    break;
  }

  case value_enum::INDEX: {
    fmt(cerr, "Serialization of indices not yet supported\n");
    AU();
  }

  default:
    cerr << which << endl;
    AU();
  }
}

value_ref_p value_ref::create_value(value_type_p type, value_p target) {
  if (type->which() != value_type_enum::COLUMN) {
    fmt(cerr, "Non-column refs not yet supported\n");
    AU();
  }

  auto ret = make_shared<value_ref>(type, value_ref_enum::VALUE);
  ASSERT_TRUE(target->which() != value_enum::REF);
  ret->target_ = SOME(target);
  return ret;
}

value_ref_p value_ref::create_value_column(value_p column) {
  ASSERT_TRUE(column->which() == value_enum::COLUMN);
  auto cc = column->as<value_column_p>();
  return value_ref::create_value(column->ty_, column);
}

value_p value_ref::create_column_element(
  value_type_p type, value_p target, int64_t i) {
  fmt(cerr, "Column element references not yet supported\n");
  AU();
}

value_p value_ref::create_column_subset(
  value_p target, value_p column_subset) {
  auto target_type = target->ty_->as<value_type_column_p>();
  auto ret_length = column_subset->get_column_length();
  auto ret_type = value_type::create_column(
    target_type->element_type_,
    SOME<int64_t>(ret_length),
    target_type->known_unique_);

  auto ret = make_shared<value_ref>(ret_type, value_ref_enum::COLUMN_SUBSET);
  ASSERT_TRUE(target->which() != value_enum::REF);
  ret->target_ = SOME(target);
  ret->column_subset_ = SOME(column_subset);
  return value::create(
    ret,
    ret_type,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

value_p value_ref::create_column_range(
  value_p target, int64_t range_lo, int64_t range_hi) {
  auto target_type = target->ty_->as<value_type_column_p>();
  ASSERT_TRUE(range_lo <= range_hi);
  auto ret_length = range_hi - range_lo;
  auto ret_type = value_type::create_column(
    target_type->element_type_,
    SOME<int64_t>(ret_length),
    target_type->known_unique_);

  auto ret = make_shared<value_ref>(ret_type, value_ref_enum::COLUMN_RANGE);
  ASSERT_TRUE(target->which() != value_enum::REF);
  ret->target_ = SOME(target);
  ret->column_range_lo_ = SOME(range_lo);
  ret->column_range_hi_ = SOME(range_hi);
  return value::create(
    ret,
    ret_type,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

value_p value_ref::ref_column_at_index(int64_t i) {
  ASSERT_EQ(type_->which(), value_type_enum::COLUMN);

  switch (ref_which_) {
  case value_ref_enum::VALUE: {
    return value_ref::create_column_element(
      type_->as<value_type_column_p>()->element_type_,
      *target_,
      i);
  }
  case value_ref_enum::COLUMN_ELEMENT: {
    fmt(cerr, "Column element references not yet supported\n");
    AU();
  }
  case value_ref_enum::COLUMN_RANGE: {
    auto ri = *column_range_lo_ + i;
    return value_ref::create_column_element(
      type_->as<value_type_column_p>()->element_type_,
      *target_,
      ri);
  }
  case value_ref_enum::COLUMN_SUBSET: {
    auto ri =
      value_column_at(*column_subset_, i)
      ->as<value_nd_vector_p>()->value_scalar_int64();
    return value_ref::create_column_element(
      type_->as<value_type_column_p>()->element_type_,
      *target_,
      ri);
  }
  default:
    cerr << static_cast<int64_t>(ref_which_) << endl;
    AU();
  }
}

void write_struct_hash_data(ostream& os, value_p x);

void write_struct_hash_data(ostream& os, optional<value_p> x) {
  if (!!x) {
    write_bin(os, static_cast<int8_t>(1));
    write_struct_hash_data(os, *x);
  } else {
    write_bin(os, static_cast<int8_t>(0));
  }
}

void write_struct_hash_data(ostream& os, value_p x) {
  auto which = static_cast<value_enum>(x->which());

  switch (which) {
  case value_enum::COLUMN: {
    auto ref = value::create(
      value_ref::create_value_column(x),
      x->ty_,
      NONE<ref_context_p>(),
      NONE<url_p>(),
      NONE<int64_t>());
    write_struct_hash_data(os, ref);
    break;
  }
  case value_enum::ND_VECTOR: {
    x->save_raw(os, NONE<ref_context_p>(), NONE<unordered_set<int64_t>*>());
    break;
  }
  case value_enum::RECORD: {
    auto cc = x->as<value_record_p>();
    for (int64_t i = 0; i < len(cc->entries_); i++) {
      write_struct_hash_data(os, cc->entries_[i]);
    }
    break;
  }
  case value_enum::EITHER: {
    auto cc = x->as<value_either_p>();
    write_bin(os, cc->val_which_);
    write_struct_hash_data(os, cc->val_data_);
    break;
  }
  case value_enum::REF: {
    auto cc = x->as<value_ref_p>();
    write_bin(os, cc->ref_which_);

    if (cc->ref_which_ == value_ref_enum::VALUE) {
      auto id = (*cc->target_)->get_value_id();
      write_bin(os, static_cast<int64_t>(0));
      write_bin(os, id);
    } else {
      write_struct_hash_data(os, cc->target_);
      write_bin(os, cc->column_element_);
      write_bin(os, cc->column_range_lo_);
      write_bin(os, cc->column_range_hi_);
      write_struct_hash_data(os, cc->column_subset_);
    }

    break;
  }
  default:
    cerr << which << endl;
    AU();
  }
}

value_enum value_type_to_direct_constructor(value_type_enum x) {
  switch (x) {
  case value_type_enum::COLUMN: return value_enum::COLUMN;
  case value_type_enum::ND_VECTOR: return value_enum::ND_VECTOR;
  case value_type_enum::RECORD: return value_enum::RECORD;
  case value_type_enum::EITHER: return value_enum::EITHER;
  default: AU();
  }
}

value_p value::load_raw(
  istream& is, value_type_p type, optional<url_p> url_context) {
  value_enum which;

  if (!type->known_direct_) {
    which = read_bin<value_enum>(is);
  } else {
    which = value_type_to_direct_constructor(type->which());
  }

  switch (which) {
  case value_enum::COLUMN: {
    auto id_mode = read_bin<value_ref_location_enum>(is);
    if (id_mode == value_ref_location_enum::LOCAL) {
      auto id = read_bin<int64_t>(is);
      return value::get_value_by_id(url_context, id);
    } else if (id_mode == value_ref_location_enum::SFRAME_URL) {
      auto url_path = read_bin<string>(is);
      auto url_context_new = SOME<url_p>(url::by_path(url_path));
      auto id = read_bin<int64_t>(is);
      return value::get_value_by_id(url_context_new, id);
    } else {
      AU();
    }
    break;
  }
  case value_enum::ND_VECTOR: {
    ASSERT_EQ(type->which(), value_type_enum::ND_VECTOR);
    auto dtype = type->as<value_type_nd_vector_p>()->dtype_;
    vector<int64_t> shape;
    if (type->as<value_type_nd_vector_p>()->ndim_ > 0) {
      shape = read_bin<vector<int64_t>>(is);
    }
    int64_t total_size = product(shape) * dtype_size_bytes(dtype);
    auto base_addr = malloc(total_size);
    is.read(reinterpret_cast<char*>(base_addr), total_size);
    auto strides = contiguous_strides(shape);
    bool base_addr_owned = true;
    bool contiguous = true;
    return value::create(
      make_shared<value_nd_vector>(
        base_addr, base_addr_owned, dtype, shape, strides, contiguous),
      type,
      NONE<ref_context_p>(),
      NONE<url_p>(),
      NONE<int64_t>());
  }
  case value_enum::RECORD: {
    ASSERT_EQ(type->which(), value_type_enum::RECORD);
    auto cc_type = type->as<value_type_record_p>();
    vector<value_p> ret_fields;
    for (auto p : cc_type->field_types_) {
      auto field_ty = p.second;
      ret_fields.push_back(value::load_raw(is, field_ty, url_context));
    }
    return value::create(
      make_shared<value_record>(type, ret_fields),
      type,
      NONE<ref_context_p>(),
      NONE<url_p>(),
      NONE<int64_t>());
  }
  case value_enum::EITHER: {
    ASSERT_EQ(type->which(), value_type_enum::EITHER);
    auto cc_type = type->as<value_type_either_p>();
    int64_t val_which = read_bin<int64_t>(is);
    auto val_ty = ::at(cc_type->case_types_, val_which).second;
    auto val_data = value::load_raw(is, val_ty, url_context);
    return value::create(
      make_shared<value_either>(type, val_which, val_data),
      type,
      NONE<ref_context_p>(),
      NONE<url_p>(),
      NONE<int64_t>());
  }
  case value_enum::REF: {
    auto ref_which = read_bin<value_ref_enum>(is);
    if (ref_which == value_ref_enum::VALUE) {
      AU();
    } else {
      auto target = NONE<value_p>();
      auto target_c = read_bin<int8_t>(is);
      if (target_c == int8_t(1)) {
        auto target_v = read_bin_value(is, url_context);
        target = SOME(target_v);
      } else {
        ASSERT_EQ(target_c, int8_t(0));
      }
      auto column_element = read_bin<optional<int64_t>>(is);
      auto column_range_lo = read_bin<optional<int64_t>>(is);
      auto column_range_hi = read_bin<optional<int64_t>>(is);
      auto column_subset = NONE<value_p>();
      auto column_subset_c = read_bin<int8_t>(is);
      if (column_subset_c == int8_t(1)) {
        auto column_subset_v = read_bin_value(is, url_context);
        column_subset = SOME(column_subset_v);
      } else {
        ASSERT_EQ(column_subset_c, int8_t(0));
      }
      auto ret = make_shared<value_ref>(type, ref_which);
      ret->target_ = *target;
      ret->column_element_ = column_element;
      ret->column_range_lo_ = column_range_lo;
      ret->column_range_hi_ = column_range_hi;
      ret->column_subset_ = column_subset;
      return value::create(
        ret,
        type,
        NONE<ref_context_p>(),
        NONE<url_p>(),
        NONE<int64_t>());
    }
  }
  case value_enum::INDEX: {
    fmt(cerr, "Serialization of indices not yet supported\n");
    AU();
  }
  default:
    cerr << which << endl;
    AU();
    break;
  }
}

int64_t value::get_column_length() {
  ASSERT_EQ(ty_->which(), value_type_enum::COLUMN);

  switch (this->which()) {
  case value_enum::COLUMN:
    return this->as<value_column_p>()->length();
  case value_enum::REF: {
    auto cc = this->as<value_ref_p>();
    switch (cc->ref_which_) {
    case value_ref_enum::VALUE:
      return (*cc->target_)->get_column_length();
    case value_ref_enum::COLUMN_ELEMENT:
      return (*cc->target_)->as<value_column_p>()
        ->at(*cc->column_element_)->get_column_length();
    case value_ref_enum::COLUMN_SUBSET:
      return (*cc->column_subset_)->get_column_length();
    case value_ref_enum::COLUMN_RANGE:
      return (*cc->column_range_hi_) - (*cc->column_range_lo_);
    default:
      AU();
    }
  }
  default:
    AU();
  }
}

value_p value::get_record_at_field_name(const string& field_name) {
  auto cc = this->as<value_record_p>();
  ASSERT_EQ(ty_->which(), value_type_enum::RECORD);
  auto ty_cc = ty_->as<value_type_record_p>();
  for (int64_t i = 0; i < len(ty_cc->field_types_); i++) {
    if (field_name == ty_cc->field_types_[i].first) {
      return ::at(cc->entries_, i);
    }
  }
  cerr << "Field not found: " << field_name << endl;
  AU();
}

value_p value::sum() {
  ASSERT_EQ(ty_->which(), value_type_enum::COLUMN);
  return value::create_thunk(
    query::from_value(shared_from_this())->sum());
}

value_p value::materialize() {
  switch (which()) {
  case value_enum::THUNK: {
    auto cc = this->as<value_thunk_p>();
    return eval(cc->query_);
  }

  case value_enum::COLUMN:
  case value_enum::ND_VECTOR:
  case value_enum::RECORD:
  case value_enum::EITHER:
  case value_enum::REF:
    return shared_from_this();

  default:
    AU();
    break;
  }
}

void value::save(const string& output_path) {
  make_directories_strict(output_path);

  auto top_acc = make_shared<binary_data_builder_fixed>();

  unordered_set<int64_t> local_refs_acc;

  string val_str;
  {
    ostringstream os;
    write_bin_value(
      os, shared_from_this(), NONE<ref_context_p>(),
      SOME<unordered_set<int64_t>*>(&local_refs_acc));
    val_str = os.str();
  }

  set<int64_t> object_ids(local_refs_acc.begin(), local_refs_acc.end());

  string output_path_objects = output_path + "/objects";
  make_directories_strict(output_path_objects);

  for (auto id : object_ids) {
    value_p v = value::get_value_by_id(NONE<url_p>(), id);
    ASSERT_EQ(v->which(), value_enum::COLUMN);
    auto vc = v->as<value_column_p>();
    auto dst = turi::fs_util::join(
      {output_path_objects, cc_sprintf("%08d", id),});
    turi::fs_util::make_directories_strict(dst);
    ASSERT_EQ(vc->format_, column_format_enum::VARIABLE);
    vc->view_variable_cached_->top_view_->save(dst + "/top");
    vc->view_variable_cached_->meta_view_->save(dst + "/meta");
    vc->view_variable_cached_->entries_view_->save(dst + "/entries");
  }

  top_acc->append(&val_str[0], len(val_str));
  top_acc->save(output_path + "/top");
}

string value::get_value_string() {
  ASSERT_EQ(ty_->tag_, value_type_tag_enum::STRING);
  auto src_v = value_deref(shared_from_this())->as<value_nd_vector_p>();
  ASSERT_TRUE(src_v->contiguous_);
  return string(reinterpret_cast<char*>(src_v->base_addr_), src_v->size());
}

int64_t value::get_value_scalar_int64() {
  ASSERT_EQ(ty_->which(), value_type_enum::ND_VECTOR);
  auto cc = value_deref(shared_from_this())->as<value_nd_vector_p>();
  ASSERT_EQ(cc->shape_.size(), 0);
  ASSERT_EQ(cc->dtype_, dtype_enum::I64);
  return *reinterpret_cast<int64_t*>(cc->base_addr_);
}

double value::get_value_scalar_float64() {
  ASSERT_EQ(ty_->which(), value_type_enum::ND_VECTOR);
  auto cc = value_deref(shared_from_this())->as<value_nd_vector_p>();
  ASSERT_EQ(cc->shape_.size(), 0);
  ASSERT_EQ(cc->dtype_, dtype_enum::F64);
  return *reinterpret_cast<double*>(cc->base_addr_);
}

uint64_t value::get_integral_value() {
  ASSERT_EQ(ty_->which(), value_type_enum::ND_VECTOR);
  auto cc = value_deref(shared_from_this())->as<value_nd_vector_p>();
  ASSERT_EQ(cc->shape_.size(), 0);

  switch (cc->dtype_) {
  case dtype_enum::I8:
    return *reinterpret_cast<int8_t*>(cc->base_addr_);
  case dtype_enum::U8:
    return *reinterpret_cast<uint8_t*>(cc->base_addr_);
  case dtype_enum::I64:
    return *reinterpret_cast<int64_t*>(cc->base_addr_);
  default:
    fmt(cerr, "Dtype not yet supported\n");
    AU();
  }
}

value_p value::load_from_path(const string& input_path) {
  auto top_view = make_shared<binary_data_view_fixed>(input_path + "/top");

  auto is_top = top_view->get_istream();
  return read_bin_value(*is_top, SOME(url::by_path(input_path)));
}

value_p value::create_scalar_int64(int64_t x) {
  return value_nd_vector::create_scalar_int64(x);
}

value_p value::create_string(string s) {
  return value::create(
    value_nd_vector::create_from_buffer_copy_1d(
      &s[0], dtype_enum::I8, s.size()),
    value_type::create_string(),
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

value_p value::create_index(
  value_p index_keys,
  value_p index_values_flat,
  value_p index_values_grouped,
  vector<uint128_t> index_hashes,
  parallel_hash_map<int64_t> index_map_singleton,
  parallel_hash_map<pair<int64_t, int64_t>> index_map_range,
  vector<value_type_p> source_column_types,
  index_mode_enum index_mode) {

  return value::create(
    make_shared<value_index>(
      index_keys,
      index_values_flat,
      index_values_grouped,
      index_hashes,
      index_map_singleton,
      index_map_range,
      index_mode),
    value_type::create_index(source_column_types, index_mode),
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

value_p value::create_thunk(query_p x) {
  return value::create(
    make_shared<value_thunk>(x),
    turi::sframe_random_access::get_type(x),
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

value_p value::create_record(value_type_p type, vector<value_p> fields) {
  ASSERT_EQ(type->which(), value_type_enum::RECORD);
  auto ty = type->as<value_type_record_p>();
  ASSERT_EQ(ty->field_types_.size(), fields.size());
  return value::create(
    make_shared<value_record>(type, fields),
    type,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
}

value_p value::create_table(
  vector<string> column_names, vector<value_p> column_values) {

  vector<value_type_p> column_types;
  for (auto x : column_values) {
    column_types.push_back(x->get_type());
  }
  auto ret_type = value_type_table_create(column_names, column_types);
  auto ret = value::create(
    make_shared<value_record>(ret_type, column_values),
    ret_type,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());
  return ret;
}

value_p value::create_column_from_integers(vector<int64_t>& values, bool unique) {
  unordered_set<int64_t> values_s;
  if (unique) {
    for (auto vi : values) {
      ASSERT_TRUE(values_s.count(vi) == 0);
      values_s.insert(vi);
    }
  }

  auto ret = column_builder_create(value_type::create_scalar(dtype_enum::I64));
  for (auto x : values) {
    ret->append(value_nd_vector::create_scalar_int64(x));
  }
  return ret->finalize(unique);
}

value_p value::build_index(vector<value_p> source_columns, index_mode_enum index_mode) {
  static unordered_map<string, value_p> build_index_cache;
  static recursive_mutex build_index_cache_mutex;

  {
    lock_guard<recursive_mutex> build_index_cache_lock {
      build_index_cache_mutex };
    auto it = build_index_cache.find(struct_hash(source_columns));
    if (it != build_index_cache.end()) {
      return it->second;
    }
  }

  parallel_hash_map<int64_t> ret_keys_map;
  parallel_hash_map<vector<int64_t>> ret_values_map;

  vector<uint128_t> ret_hashes;
  parallel_hash_map<int64_t> ret_index_map_singleton;
  parallel_hash_map<pair<int64_t, int64_t>> ret_index_map_range;

  int64_t n = ::at(source_columns, 0)->get_column_length();
  int64_t m = source_columns.size();
  vector<bool> is_direct_column(m);
  vector<value_column*> source_columns_fast;

  for (int64_t j = 0; j < m; j++) {
    auto sj_opt = source_columns[j]->get_as_direct_column();
    is_direct_column[j] = !!sj_opt;
    if (is_direct_column[j]) {
      source_columns_fast.push_back(*sj_opt);
    } else {
      source_columns_fast.push_back(nullptr);
    }
  }

  int64_t nt = turi::thread_pool::get_instance().size();
  int64_t chunk_size = ceil_divide(n, nt);

  using local_res_type = vector<pair<uint128_t, int64_t>>;
  vector<vector<local_res_type>> local_res(nt);

  for (int64_t k = 0; k < nt; k++) {
    for (int64_t r = 0; r < nt; r++) {
      local_res[k].push_back(local_res_type());
    }
  }

  auto hash_chunk_size = get_hash_chunk_size();

  turi::in_parallel_debug([&](int64_t k, int64_t num_threads_actual) {
    ASSERT_EQ(num_threads_actual, nt);

    int64_t start_k = k * chunk_size;
    int64_t end_k = min<int64_t>((k+1) * chunk_size, n);

    vector<uint128_t> vi_hashes(m, 0);

    for (int64_t i = start_k; i < end_k; i++) {
      {
        for (int64_t j = 0; j < m; j++) {
          if (is_direct_column[j]) {
            vi_hashes[j] = source_columns_fast[j]->at_raw_hash(i);
          } else {
            ostringstream os;
            auto w = value_column_at(source_columns[j], i);
            w = value_deref(w);
            write_bin_value(os, w, NONE<ref_context_p>(), NONE<unordered_set<int64_t>*>());
            auto os_str = os.str();
            vi_hashes[j] = turi::hash128(&os_str[0], os_str.length());
          }
        }
      }

      uint128_t vi_hash;
      if (m == 1) {
        vi_hash = vi_hashes[0];
      } else {
        vi_hash = turi::hash128(
          reinterpret_cast<char*>(&vi_hashes[0]), m * sizeof(uint128_t));
      }

      int64_t r = static_cast<int64_t>(vi_hash / hash_chunk_size);
      local_res[r][k].push_back(make_pair(vi_hash, i));
    }
  });

  turi::in_parallel_debug([&](int64_t r, int64_t num_threads_actual) {
    ASSERT_EQ(num_threads_actual, nt);

    for (int64_t k = 0; k < nt; k++) {
      for (auto p : local_res[r][k]) {
        auto it = ret_values_map.find(p.first);
        if (it == ret_values_map.end(p.first)) {
          ret_values_map[p.first] = vector<int64_t>();
          ret_keys_map[p.first] = 0;
        }
      }
    }

    for (int64_t k = 0; k < nt; k++) {
      for (auto p : local_res[r][k]) {
        auto it = ret_values_map.find(p.first);
        if (it->second.size() == 0) {
          ret_keys_map[p.first] += p.second;
        }
        it->second.push_back(p.second);
      }
    }
  });

  auto ret_keys = column_builder_create(
    value_type::create_scalar(dtype_enum::I64));

  auto ret_values_grouped_builder = column_builder_create(
    value_type::create_column(
      value_type::create_scalar(dtype_enum::I64),
      NONE<int64_t>(),
      true));

  value_p ret_values_flat;
  vector<vector<pair<int64_t, int64_t>>> ret_ranges { static_cast<size_t>(nt) };

  auto ret_values_flat_builder =
    column_builder_create(value_type::create_scalar(dtype_enum::I64));

  vector<int64_t> map_num_hashes_accum;
  int64_t map_num_hashes_accum_curr = 0;
  vector<int64_t> map_num_values_accum;
  int64_t map_num_values_accum_curr = 0;

  for (auto& m : ret_values_map.maps_) {
    map_num_hashes_accum.push_back(map_num_hashes_accum_curr);
    map_num_values_accum.push_back(map_num_values_accum_curr);
    for (auto p : m) {
      ret_hashes.push_back(p.first);
      map_num_values_accum_curr += p.second.size();
    }
    map_num_hashes_accum_curr += m.size();
  }

  map_num_hashes_accum.push_back(map_num_hashes_accum_curr);
  map_num_values_accum.push_back(map_num_values_accum_curr);

  for (auto x : ret_hashes) {
    ret_keys->append(
      value_nd_vector::create_scalar_int64(ret_keys_map[x]));
  }

  ret_values_flat_builder->extend_length_raw(map_num_values_accum_curr);

  turi::in_parallel_debug([&](int64_t r, int64_t num_threads_actual) {
    ASSERT_EQ(num_threads_actual, nt);

    int64_t hash_range_base_curr = ::at(map_num_hashes_accum, r);
    int64_t value_range_base_curr = ::at(map_num_values_accum, r);

    for (int64_t i = ::at(map_num_hashes_accum, r);
         i < ::at(map_num_hashes_accum, r+1);
         i++) {

      auto h = ::at(ret_hashes, i);
      auto p = make_pair(h, ret_values_map[h]);
      ret_index_map_singleton[p.first] = hash_range_base_curr;
      ++hash_range_base_curr;

      int64_t value_range_base_curr_init = value_range_base_curr;
      for (auto x : p.second) {
        column_value_put_raw_scalar<int64_t>(
          ret_values_flat_builder, x, value_range_base_curr, r);
        ++value_range_base_curr;
      }
      ret_ranges[r].push_back(
        make_pair(value_range_base_curr_init, value_range_base_curr));
      ret_index_map_range[p.first] = make_pair(
        value_range_base_curr_init, value_range_base_curr);
    }
  });

  ret_values_flat = ret_values_flat_builder->finalize(true);

  for (auto& v : ret_ranges) {
    for (auto p : v) {
      ret_values_grouped_builder->append(
        value_ref::create_column_range(ret_values_flat, p.first, p.second));
    }
  }

  vector<value_type_p> source_types;
  for (auto source_column : source_columns) {
    source_types.push_back(source_column->ty_);
  }

  auto ret = value::create_index(
    ret_keys->finalize(true),
    ret_values_flat,
    ret_values_grouped_builder->finalize(),
    ret_hashes,
    ret_index_map_singleton,
    ret_index_map_range,
    source_types,
    index_mode);

  {
    lock_guard<recursive_mutex> build_index_cache_lock {
      build_index_cache_mutex };
    build_index_cache[struct_hash(source_columns)] = ret;
  }

  return ret;
}

value_p value::index_lookup_by_hash(
  value_p index, uint128_t hash, index_lookup_mode_enum mode) {

  auto cc = value_deref(index)->as<value_index_p>();

  ASSERT_TRUE(cc->index_mode_ == index_mode_enum::EQUALS);
  ASSERT_TRUE(mode == index_lookup_mode_enum::EQUALS);

  auto it = cc->index_map_singleton_.find(hash);
  if (it == cc->index_map_singleton_.end(hash)) {
    vector<int64_t> ret_empty;
    return value::create_column_from_integers(ret_empty, true);
  } else {
    int64_t ret_offset = it->second;
    auto values = value_deref(cc->index_values_grouped_)->as<value_column_p>();
    auto ret = values->at(ret_offset);
    return ret;
  }
}

value_p value::index_lookup(
  value_p index, vector<value_p> keys, index_lookup_mode_enum mode) {

  auto cc = value_deref(index)->as<value_index_p>();

  // Other index types (e.g., numerical ordering) not yet supported
  ASSERT_TRUE(cc->index_mode_ == index_mode_enum::EQUALS);
  ASSERT_TRUE(mode == index_lookup_mode_enum::EQUALS);

  uint128_t keys_hash;
  {
    ostringstream os;
    for (auto k : keys) {
      k->save_raw(os, NONE<ref_context_p>(), NONE<unordered_set<int64_t>*>());
    }
    string w = os.str();
    keys_hash = turi::hash128(&w[0], w.length());
  }

  return value::index_lookup_by_hash(index, keys_hash, mode);
}

void ref_context::enroll_ref_target(value_p target) {
  lock_guard<recursive_mutex> lock__ { ref_targets_lock_ };
  ref_targets_.push_back(target);
}

unordered_map<int64_t, weak_ptr<url>>& url::get_url_id_map() {
  static unordered_map<int64_t, weak_ptr<url>> id_map;
  return id_map;
}

recursive_mutex& url::get_url_id_map_lock() {
  static recursive_mutex mut;
  return mut;
}

std::atomic<int64_t>& url::get_next_url_id() {
  static std::atomic<int64_t> next_url_id { 0 };
  return next_url_id;
}

url_p url::by_path(string url_path) {
  auto ret = make_shared<url>(url_path);
  ret->url_id_ = url::get_next_url_id();
  {
    lock_guard<recursive_mutex> lock__ { url::get_url_id_map_lock() };
    url::get_url_id_map()[ret->url_id_] = ret;
  }
  return ret;
}

value_id_map_weak_ptr_type& value::get_value_id_map() {
  static value_id_map_weak_ptr_type id_map;
  return id_map;
}

recursive_mutex& value::get_value_id_map_lock() {
  static recursive_mutex mut;
  return mut;
}

value_p value::get_value_by_id(optional<url_p> url_context, int64_t value_id) {
  lock_guard<recursive_mutex> lock { value::get_value_id_map_lock() };
  auto& m = value::get_value_id_map();

  auto id_ext = make_pair(int64_t(-1), value_id);
  if (!!url_context) {
    id_ext = make_pair((*url_context)->url_id_, value_id);
  }

  auto it = m.find(id_ext);

  if (it == m.end()) {
    ASSERT_TRUE(!!url_context);
    auto src_path = turi::fs_util::join({
      (*url_context)->url_path_, "objects", cc_sprintf("%08d", value_id),});
    auto ret = value_column::load_column_from_disk_path(
      src_path, NONE<ref_context_p>(), url_context, SOME<int64_t>(value_id));
    m[id_ext] = ret;

    static value_id_map_shared_ptr_type id_map_url_persistent;
    id_map_url_persistent[id_ext] = ret;

    return ret;
  } else {
    weak_ptr<value> ret = it->second;
    try {
      return value_p(ret);
    } catch (std::bad_weak_ptr& exn) {
      log_and_throw(
        "value::get_value_by_id: requested value no longer present");
    }
  }
}

int64_t value::get_value_id() {
  ASSERT_EQ(this->which(), value_enum::COLUMN);

  if (!!value_id_) {
    return *value_id_;
  }

  ASSERT_TRUE(!url_context_);

  static std::atomic<int64_t> next_value_id { 0 };
  int64_t ret = next_value_id.fetch_add(1);

  auto id_ext = make_pair(int64_t(-1), ret);

  value::get_value_id_map()[id_ext] = shared_from_this();

  value_id_ = SOME<int64_t>(ret);
  return ret;
}

value_p value::at(value_p x) {
  if (type_valid(value_type::create_bool_column(), x->ty_)) {
    if (ty_->tag_ == SOME(value_type_tag_enum::DATA_TABLE)) {
      auto index_column = query::create_column_from_mask(query::from_value(x));
      return value::create_thunk(
        query::create_table_at_column(
          query::from_value(shared_from_this()), index_column));
    } else if (ty_->which() == value_type_enum::COLUMN) {
      auto index_column = query::create_column_from_mask(query::from_value(x));
      return value::create_thunk(
        query::create_column_at_column(
          query::from_value(shared_from_this()), index_column));
    }
  }

  if (type_valid(value_type::create_scalar(dtype_enum::I64), x->ty_)) {
    if (ty_->which() == value_type_enum::COLUMN) {
      return value::create_thunk(
        query::create_column_at_index(
          query::from_value(shared_from_this()), query::from_value(x)));
    } else if (ty_->tag_ == SOME(value_type_tag_enum::DATA_TABLE)) {
      return value::create_thunk(
        query::create_table_at_index(
          query::from_value(shared_from_this()), query::from_value(x)));
    }
  }

  fmt(cerr, " *** Type error or subscript type not supported: %v\n", x->ty_);
  AU();
}

value_p value::at_string(string x) {
  switch (which()) {
  case value_enum::RECORD: {
    auto cc = this->as<value_record_p>();
    auto cc_type = cc->type_->as<value_type_record_p>();
    for (int64_t i = 0; i < len(cc_type->field_types_); i++) {
      if (cc_type->field_types_[i].first == x) {
        return ::at(cc->entries_, i);
      }
    }
  }

  default:
    fmt(cerr, "Type error or indexing mode not yet supported\n");
    AU();
    return nullptr;
  }
}

value_p value::at_int(int64_t x) {
  return value::at(value::create_scalar_int64(x));
}

value_p value::equals_string(string x) {
  return value::create_thunk(
    query::from_value(shared_from_this())->equals_string_poly(x));
}

value_p value::equals_int(int64_t x) {
  return value::create_thunk(
    query::from_value(shared_from_this())->equals_int_poly(x));
}

value_p value::equals_value_poly(value_p x) {
  return value::create_thunk(
    query::from_value(shared_from_this())->equals_value_poly(x));
}

value_p value::op_boolean_lt(value_p x) {
  vector<query_p> args = {
    query::from_value(shared_from_this()),
    query::from_value(x),
  };
  return value::create_thunk(
    query_builtin_poly(scalar_builtin_enum::LT, args));
}

value_p value::op_add(value_p x) {
  vector<query_p> args = {
    query::from_value(shared_from_this()),
    query::from_value(x),
  };
  return value::create_thunk(
    query_builtin_poly(scalar_builtin_enum::ADD, args));
}

vector<query_p> query_table_join_body(
  vector<query_p> join_columns_left,
  vector<query_p> join_columns_right,
  vector<query_p> other_columns_left,
  vector<query_p> other_columns_right) {

  auto join_index_left =
    query::create_build_index(join_columns_left, index_mode_enum::EQUALS);
  auto join_index_right =
    query::create_build_index(join_columns_right, index_mode_enum::EQUALS);

  vector<query_p> ret_columns;

  for (auto ci : other_columns_left) {
    ret_columns.push_back(
      query::create_column_join(
        ci, join_index_left, join_index_right, column_join_mode::INNER,
        column_join_position::LEFT));
  }

  for (auto ci : join_columns_left) {
    ret_columns.push_back(
      query::create_column_join(
        ci, join_index_left, join_index_right, column_join_mode::INNER,
        column_join_position::LEFT));
  }

  for (auto ci : other_columns_right) {
    ret_columns.push_back(
      query::create_column_join(
        ci, join_index_right, join_index_left, column_join_mode::INNER,
        column_join_position::RIGHT));
  }

  return ret_columns;
}

query_p query_table_join(
  query_p table_left,
  query_p table_right,
  vector<string> join_column_names_left,
  vector<string> join_column_names_right) {

  vector<query_p> join_columns_left;
  vector<query_p> join_columns_right;
  vector<query_p> other_columns_left;
  vector<query_p> other_columns_right;

  ASSERT_TRUE(
    table_left->get_type()->tag_ == SOME(value_type_tag_enum::DATA_TABLE));
  ASSERT_TRUE(
    table_right->get_type()->tag_ == SOME(value_type_tag_enum::DATA_TABLE));

  auto ty_left = table_left->get_type()->as<value_type_record_p>();
  auto ty_right = table_right->get_type()->as<value_type_record_p>();

  int64_t n_join = join_column_names_left.size();
  ASSERT_EQ(join_column_names_right.size(), n_join);

  vector<bool> joined_left(ty_left->field_types_.size(), false);
  vector<bool> joined_right(ty_right->field_types_.size(), false);

  for (int64_t i = 0; i < n_join; i++) {
    auto name_left_i = at(join_column_names_left, i);
    auto name_right_i = at(join_column_names_right, i);

    bool found_left_i = false;
    bool found_right_i = false;

    for (int64_t j = 0; j < len(ty_left->field_types_); j++) {
      if (at(ty_left->field_types_, j).first == name_left_i) {
        ASSERT_TRUE(!found_left_i);
        found_left_i = true;
        join_columns_left.push_back(
          query::create_record_at_field_index(table_left, j));
        joined_left[j] = true;
      }
    }

    if (!found_left_i) {
      fmt(cerr, "Join column not found in table: %v\n", name_left_i);
      AU();
    }

    for (int64_t j = 0; j < len(ty_right->field_types_); j++) {
      if (at(ty_right->field_types_, j).first == name_right_i) {
        ASSERT_TRUE(!found_right_i);
        found_right_i = true;
        join_columns_right.push_back(
          query::create_record_at_field_index(table_right, j));
        joined_right[j] = true;
      }
    }

    if (!found_right_i) {
      fmt(cerr, "Join column not found in table: %v\n", name_right_i);
      AU();
    }
  }

  vector<string> ret_field_names;

  for (int64_t j = 0; j < len(ty_left->field_types_); j++) {
    if (!joined_left[j]) {
      other_columns_left.push_back(
        query::create_record_at_field_index(table_left, j));
      ret_field_names.push_back(at(ty_left->field_types_, j).first);
    }
  }

  for (auto name : join_column_names_left) {
    ret_field_names.push_back(name);
  }

  for (int64_t j = 0; j < len(ty_right->field_types_); j++) {
    if (!joined_right[j]) {
      other_columns_right.push_back(
        query::create_record_at_field_index(table_right, j));
      ret_field_names.push_back(at(ty_right->field_types_, j).first);
    }
  }

  auto ret_columns = query_table_join_body(
    join_columns_left,
    join_columns_right,
    other_columns_left,
    other_columns_right);

  vector<value_type_p> ret_field_types;
  for (int64_t i = 0; i < len(ret_columns); i++) {
    auto orig_type_i = at(ret_columns, i)->get_type()
      ->as<value_type_column_p>();
    ret_field_types.push_back(orig_type_i->element_type_);
  }

  return query::create_record_from_fields(
    value_type_table_create(
      ret_field_names,
      ret_field_types
    ),
    ret_columns);
}

query_p query_table_join_auto(
  query_p table_left,
  query_p table_right) {

  vector<string> join_column_names_left;
  vector<string> join_column_names_right;

  auto ty_left = table_left->get_type()->as<value_type_record_p>();
  auto ty_right = table_right->get_type()->as<value_type_record_p>();

  unordered_set<string> names_left;

  for (int64_t i = 0; i < len(ty_left->field_types_); i++) {
    auto name_i = at(ty_left->field_types_, i).first;
    ASSERT_EQ(names_left.count(name_i), 0);
    names_left.insert(name_i);
  }

  for (int64_t i = 0; i < len(ty_right->field_types_); i++) {
    auto name_i = at(ty_right->field_types_, i).first;
    if (names_left.count(name_i) != 0) {
      join_column_names_left.push_back(name_i);
      join_column_names_right.push_back(name_i);
    }
  }

  return query_table_join(
    table_left,
    table_right,
    join_column_names_left,
    join_column_names_right);
}

query_p query_table_group_by_body(
  query_p source_table,
  query_p ind_column_keys,
  query_p ind_column_values,
  group_by_spec_p output_spec) {

  auto f_gen_values =
    [source_table, ind_column_values, output_spec](query_p i) {

    switch (output_spec->which()) {
    case group_by_spec_enum::ORIGINAL_TABLE: {
      return query::create_table_at_column(
        source_table,
        query::create_column_at_index(ind_column_values, i));
    }
    case group_by_spec_enum::REDUCE: {
      auto spec_cc = output_spec->as<group_by_spec_reduce_p>();
      auto reduce_column = query::create_column_at_column(
        spec_cc->source_column_,
        query::create_column_at_index(ind_column_values, i));
      return query::create_column_reduce(reduce_column, spec_cc->reduce_op_);
    }
    case group_by_spec_enum::SELECT_ONE: {
      auto spec_cc = output_spec->as<group_by_spec_select_one_p>();
      auto index_column = query::create_column_at_index(ind_column_values, i);
      auto zero = query::from_value(value_nd_vector::create_scalar_int64(0));
      return query::create_column_at_index(
        spec_cc->source_column_,
        query::create_column_at_index(index_column, zero));
    }
    default:
      AU();
    }
  };

  auto val_column = query::create_column_generator(
    query::create_lambda(
      f_gen_values, value_type::create_scalar(dtype_enum::I64)),
    query::create_column_length(ind_column_keys));

  return val_column;
}

query_p query_table_group_by(
  query_p source_table,
  vector<string> field_names,
  vector<pair<string, group_by_spec_p>> output_specs) {

  ASSERT_TRUE(
    source_table->get_type()->tag_ == SOME(value_type_tag_enum::DATA_TABLE));

  vector<query_p> source_columns;
  for (auto field_name : field_names) {
    auto source_column =
      query::create_record_at_field_name(source_table, field_name);
    source_columns.push_back(source_column);
  }

  auto index = query::create_build_index(
    source_columns, index_mode_enum::EQUALS);

  auto ind_column_keys = query::create_index_get_keys(index);
  auto ind_column_values = query::create_index_get_values(index);

  vector<string> ret_field_names = field_names;
  vector<value_type_p> ret_field_types;
  for (auto source_column : source_columns) {
    ret_field_types.push_back(
      source_column->get_type()->as<value_type_column_p>()->element_type_);
  }

  vector<query_p> ret_columns;
  for (auto source_column : source_columns) {
    ret_columns.push_back(
      query::create_column_at_column(source_column, ind_column_keys));
  }

  for (auto output_spec : output_specs) {
    ret_field_names.push_back(output_spec.first);
    auto val_column = query_table_group_by_body(
      source_table,
      ind_column_keys,
      ind_column_values,
      output_spec.second);
    ret_columns.push_back(val_column);

    ret_field_types.push_back(
      val_column->get_type()->as<value_type_column_p>()->element_type_);
  }

  return query::create_record_from_fields(
    value_type_table_create(
      ret_field_names,
      ret_field_types
    ),
    ret_columns);
}

query_p query_column_unique(query_p source_column) {
  ASSERT_TRUE(source_column->get_type()->which() == value_type_enum::COLUMN);
  vector<query_p> source_columns = {source_column,};

  auto index = query::create_build_index(
    source_columns, index_mode_enum::EQUALS);

  return query::create_column_at_column(
    source_column,
    query::create_index_get_keys(index));
}

value_p value::group_by(
  vector<string> field_names,
  vector<pair<string, group_by_spec_p>> output_specs) {

  return value::create_thunk(
    query_table_group_by(
      query::from_value(shared_from_this()), field_names, output_specs));
}

value_p value::unique() {
  return value::create_thunk(
    query_column_unique(query::from_value(shared_from_this())));
}

value_p value::join_auto(value_p x) {
  return value::create_thunk(
    query_table_join_auto(
      query::from_value(shared_from_this()), query::from_value(x)));
}

int64_t value_nd_vector::value_scalar_int64() {
  ASSERT_EQ(shape_.size(), 0);
  ASSERT_EQ(dtype_, dtype_enum::I64);
  return *reinterpret_cast<int64_t*>(base_addr_);
}

bool value_nd_vector::value_scalar_bool() {
  ASSERT_EQ(shape_.size(), 0);
  ASSERT_EQ(dtype_, dtype_enum::BOOL);
  return *reinterpret_cast<bool*>(base_addr_);
}

constexpr int64_t COLUMN_DISPLAY_COMPACT_MAX = 16;
constexpr int64_t STRING_DISPLAY_MAX = 16;

ostream& operator<<(ostream& os, value_p v);

value_p value_column_at(value_p v, int64_t i) {
  v = value_deref(v);

  int64_t n = v->get_column_length();
  ASSERT_GE(i, 0);
  ASSERT_LT(i, n);

  switch (v->which()) {
  case value_enum::COLUMN: {
    auto cc = v->as<value_column_p>();
    return cc->at(i);
  }

  case value_enum::REF: {
    auto cc = v->as<value_ref_p>();

    switch (cc->ref_which_) {
    case value_ref_enum::COLUMN_SUBSET: {
      auto v_base = *cc->target_;
      auto col_base = v_base->as<value_column_p>();
      auto vi = value_column_at(*cc->column_subset_, i);
      int64_t ii = vi->as<value_nd_vector_p>()->value_scalar_int64();
      return col_base->at(ii);
    }

    case value_ref_enum::VALUE: {
      AU();
      break;
    }

    case value_ref_enum::COLUMN_ELEMENT: {
      AU();
      break;
    }

    case value_ref_enum::COLUMN_RANGE: {
      auto v_base = *cc->target_;
      auto col_base = v_base->as<value_column_p>();
      return col_base->at(*cc->column_range_lo_ + i);
    }

    default:
      AU();
    }

    break;
  }

  case value_enum::ND_VECTOR:
  case value_enum::RECORD:
  case value_enum::EITHER:
  case value_enum::INDEX:
  case value_enum::THUNK:
    AU();

  default:
    cerr << v->which() << endl;
    AU();
  }

  AU();
}

void value_column_iterate(
  vector<value_p> vs, function<bool(int64_t, vector<value_p>)> yield) {

  vector<string> ret;

  if (len(vs) == 0) {
    return;
  }

  vector<value_p> vs_new;

  auto n = vs[0]->get_column_length();
  for (int64_t i = 0; i < len(vs); i++) {
    auto vi = value_deref(vs[i]);
    ASSERT_EQ(vi->get_column_length(), n);
    vs_new.push_back(vi);
  }

  vs = vs_new;

  for (int64_t i = 0; i < n; i++) {
    vector<value_p> res_i;

    for (auto v : vs) {
      res_i.push_back(value_column_at(v, i));
    }

    bool ok = yield(i, res_i);
    if (!ok) {
      break;
    }
  }
}

void value_column_iterate(value_p v, function<bool(int64_t, value_p)> yield) {
  vector<value_p> vs = {v,};
  value_column_iterate(vs, [&](int64_t i, vector<value_p> res_i) {
    return yield(i, res_i[0]);
  });
}

vector<string> print_column_extract_display_values(value_p x) {
  vector<string> ret;

  value_column_iterate(x, [&](int64_t i, value_p xi) {
    if (i >= COLUMN_DISPLAY_COMPACT_MAX) {
      return false;
    }
    ret.push_back(to_string(xi));
    return true;
  });

  return ret;
}

inline string center_string(string x, int64_t width) {
  int64_t num_spaces_total = max<int64_t>(0, width - x.length());
  if (num_spaces_total == 0) {
    return x;
  }
  int64_t num_spaces_left = num_spaces_total / 2;
  int64_t num_spaces_right = num_spaces_total - num_spaces_left;
  return (
    cc_repstr(" ", num_spaces_left) + x + cc_repstr(" ", num_spaces_right));
}

ostream& operator<<(ostream& os, value_p v) {
  if (v->ty_->is_optional()) {
    ASSERT_TRUE(v->which() == value_enum::EITHER);
    auto cc = v->as<value_either_p>();
    if (cc->val_which_ == 0) {
      os << "None";
    } else {
      os << cc->val_data_;
    }
    return os;
  }

  switch (v->which()) {
  case value_enum::RECORD: {
    auto cc = v->as<value_record_p>();

    if (v->ty_->tag_ == SOME(value_type_tag_enum::DATA_TABLE)) {
      auto ty_cc = v->ty_->as<value_type_record_p>();
      auto num_columns = len(ty_cc->field_types_);
      ASSERT_EQ(cc->entries_.size(), num_columns);
      vector<string> column_names;
      auto num_rows_display = NONE<int64_t>();
      auto num_rows_actual = NONE<int64_t>();
      auto row_heights = NONE<vector<int64_t>>();
      vector<vector<vector<string>>> column_display_values;
      int64_t table_display_width = 1;
      vector<int64_t> column_widths_proper;

      for (int64_t i = 0; i < num_columns; i++) {
        auto column_name_i = ty_cc->field_types_[i].first;
        int64_t column_width_proper_i = column_name_i.length();
        column_names.push_back(column_name_i);

        int64_t num_rows_actual_i = cc->entries_[i]->get_column_length();
        if (!num_rows_actual) {
          num_rows_actual = SOME(num_rows_actual_i);
        } else {
          ASSERT_EQ(num_rows_actual_i, *num_rows_actual);
        }

        auto display_ret_i_orig =
          print_column_extract_display_values(cc->entries_[i]);

        vector<vector<string>> display_ret_i;
        for (int64_t j = 0; j < len(display_ret_i_orig); j++) {
          auto orig_j = strip_all(display_ret_i_orig[j], "\n");
          display_ret_i.push_back(::split(orig_j, "\n"));
        }
        column_display_values.push_back(display_ret_i);
        if (!num_rows_display) {
          num_rows_display = SOME<int64_t>(display_ret_i.size());
          row_heights = SOME<vector<int64_t>>(vector<int64_t>(*num_rows_display, 0));
        }
        ASSERT_EQ(display_ret_i.size(), *num_rows_display);

        for (int64_t j = 0; j < *num_rows_display; j++) {
          int64_t new_row_height_j = display_ret_i[j].size();
          for (int64_t k = 0; k < new_row_height_j; k++) {
            column_width_proper_i = max<int64_t>(
              column_width_proper_i, display_ret_i[j][k].length());
          }
          (*row_heights)[j] = max<int64_t>((*row_heights)[j], new_row_height_j);
        }

        column_widths_proper.push_back(column_width_proper_i);
        table_display_width += (column_width_proper_i + 3);
      }

      auto print_bar = [&]() {
        for (int64_t i = 0; i < num_columns; i++) {
          os << "+";
          os << cc_repstr("-", at(column_widths_proper, i) + 2);
        }
        os << "+" << endl;
      };

      os << endl;

      print_bar();

      for (int64_t i = 0; i < num_columns; i++) {
        os << "| ";
        os << center_string(at(column_names, i), at(column_widths_proper, i));
        os << " ";
      }
      os << "|" << endl;

      print_bar();

      auto row_height_max = extract(vector_max(*row_heights), int64_t(0));

      if (!!num_rows_display) {
        for (int64_t j = 0; j < num_rows_display; j++) {
          for (int64_t k = 0; k < (*row_heights)[j]; k++) {
            for (int64_t i = 0; i < num_columns; i++) {
              string str_ijk;
              if (k < len(at(at(column_display_values, i), j))) {
                str_ijk = center_string(
                  at(at(at(column_display_values, i), j), k),
                  at(column_widths_proper, i)
                );
              } else {
                str_ijk = center_string("", at(column_widths_proper, i));
              }

              if (i > 0) {
                os << " ";
              }
              os << "| " << str_ijk;
            }
            os << " |" << endl;
          }

          if (row_height_max > 1) {
            print_bar();
          }
        }
      }

      if (row_height_max <= 1) {
        print_bar();
      }

      string footer = cc_sprintf(
        "[%ld rows x %ld columns]",
        (!!num_rows_actual ? *num_rows_actual : 0), num_columns);
      footer += cc_repstr(
        " ", max<int64_t>(0, table_display_width - footer.length()));
      os << footer;
    } else {
      os << "<record>";
    }
    break;
  }

  case value_enum::ND_VECTOR: {
    auto cc = v->as<value_nd_vector_p>();
    auto cc_ty = v->ty_->as<value_type_nd_vector_p>();

    bool handled = false;

    if (v->ty_->tag_ == SOME(value_type_tag_enum::STRING)) {
      ASSERT_EQ(cc_ty->ndim_, 1);
      ASSERT_EQ(cc_ty->dtype_, dtype_enum::I8);
      int64_t len_actual = cc->size();
      int64_t len_display = min<int64_t>(STRING_DISPLAY_MAX, len_actual);
      ASSERT_TRUE(cc->contiguous_);
      string str_val;
      auto base = reinterpret_cast<const char*>(cc->base_addr_);
      for (int64_t i = 0; i < len_display; i++) {
        if (isprint(base[i])) {
          str_val += base[i];
        } else {
          str_val += "\\x";
          str_val += format_hex(string(&base[i], 1));
        }
      }
      os << "\"" << str_val;
      if (len_actual > len_display) {
        os << "...";
      }
      os << "\"";
      handled = true;

    } else if (v->ty_->tag_ == SOME(value_type_tag_enum::IMAGE)) {
      os << "<image>";
      handled = true;

    } else if (cc_ty->ndim_ == 0 &&
               cc_ty->dtype_ == dtype_enum::I64) {

      int64_t v = *reinterpret_cast<int64_t*>(cc->base_addr_);
      os << v;
      handled = true;

    } else if (cc_ty->ndim_ == 0 &&
               cc_ty->dtype_ == dtype_enum::F64) {

      double v = *reinterpret_cast<double*>(cc->base_addr_);
      os << cc_sprintf("%6f", v);
      handled = true;

    } else if (cc_ty->ndim_ == 0 &&
               cc_ty->dtype_ == dtype_enum::BOOL) {

      bool v = *reinterpret_cast<bool*>(cc->base_addr_);
      os << v;
      handled = true;
    }

    if (!handled) {
      os << "<nd_vector>";
    }

    break;
  }

  case value_enum::COLUMN: {
    auto cc = v->as<value_column_p>();
    int64_t len_actual = cc->length();
    os << "Column<" << len_actual << ">: [";
    int64_t len_display = min<int64_t>(COLUMN_DISPLAY_COMPACT_MAX, len_actual);
    for (int64_t i = 0; i < len_display; i++) {
      os << cc->at(i);
      if (i < len_display - 1) {
        os << ", ";
      }
    }
    if (len_actual > len_display) {
      os << ", ...";
    }
    os << "]";
    break;
  }

  case value_enum::THUNK: {
    os << "<thunk>";
    break;
  }

  case value_enum::REF: {
    auto cc = v->as<value_ref_p>();

    switch (cc->ref_which_) {
    case value_ref_enum::COLUMN_SUBSET: {
      auto v_base = *cc->target_;
      auto col_base = v_base->as<value_column_p>();
      auto col_index = (*cc->column_subset_);

      int64_t len_actual = col_index->get_column_length();
      os << "Column<" << len_actual << ">: [";
      int64_t len_display = min<int64_t>(COLUMN_DISPLAY_COMPACT_MAX, len_actual);
      for (int64_t i = 0; i < len_display; i++) {
        int64_t ii = value_column_at(col_index, i)
          ->as<value_nd_vector_p>()->value_scalar_int64();
        os << col_base->at(ii);
        if (i < len_display - 1) {
          os << ", ";
        }
      }
      if (len_actual > len_display) {
        os << ", ...";
      }
      os << "]";

      break;
    }
    case value_ref_enum::VALUE:
    case value_ref_enum::COLUMN_ELEMENT:
    case value_ref_enum::COLUMN_RANGE: {
      os << "<ref: " << v->ty_ << ">";
      break;
    }
    default:
      AU();
    }

    break;
  }

  default:
    fmt(os, "<%v>", v->which());
  }

  return os;
}

value_p value_column_at_deref(value_p x, int64_t i) {
  ASSERT_EQ(x->ty_->which(), value_type_enum::COLUMN);
  x = value_deref(x);

  if (x->which() == value_enum::COLUMN) {
    return value_column_at(x, i);
  }

  ASSERT_EQ(x->which(), value_enum::REF);
  auto cc = x->as<value_ref_p>();

  switch (cc->ref_which_) {
  case value_ref_enum::VALUE: {
    AU();
  }
  case value_ref_enum::COLUMN_ELEMENT: {
    AU();
  }
  case value_ref_enum::COLUMN_RANGE: {
    auto ri = *cc->column_range_lo_ + i;
    return value_column_at(*cc->target_, ri);
  }
  case value_ref_enum::COLUMN_SUBSET: {
    auto ri =
      value_column_at(
        *cc->column_subset_, i)->as<value_nd_vector_p>()->value_scalar_int64();
    return value_column_at(*cc->target_, ri);
  }
  default:
    cerr << static_cast<int64_t>(cc->ref_which_) << endl;
    AU();
  }
}

value_p value_deref(value_p x) {
  if (x->which() == value_enum::REF) {
    auto xc = x->as<value_ref_p>();

    switch (xc->ref_which_) {
    case value_ref_enum::VALUE: {
      auto ret = *xc->target_;
      ASSERT_TRUE(type_valid(xc->type_, ret->ty_));
      return ret;
    }

    case value_ref_enum::COLUMN_ELEMENT: {
      auto rc = *xc->target_;
      ASSERT_EQ(rc->which(), value_enum::COLUMN);
      auto ret = rc->as<value_column_p>()->at(*xc->column_element_);
      ASSERT_TRUE(type_valid(xc->type_, ret->ty_));
      return value_deref(ret);
    }

    case value_ref_enum::COLUMN_SUBSET:
    case value_ref_enum::COLUMN_RANGE: {
      return x;
    }

    default:
      fmt(cerr, "Reference type yet supported: %v\n", xc->ref_which_);
      AU();
    }
  } else {
    return x;
  }
}

bool value_eq(value_p x, value_p y) {
  x = value_deref(x);
  y = value_deref(y);
  ASSERT_EQ(x->which(), y->which());

  switch (x->which()) {
  case value_enum::ND_VECTOR: {
    auto xc = x->as<value_nd_vector_p>();
    auto yc = y->as<value_nd_vector_p>();
    ASSERT_EQ(xc->dtype_, yc->dtype_);
    ASSERT_EQ(xc->shape_.size(), yc->shape_.size());
    if (xc->shape_ != yc->shape_) {
      return false;
    }
    ASSERT_TRUE(xc->contiguous_);
    ASSERT_TRUE(yc->contiguous_);
    return !memcmp(
      xc->base_addr_,
      yc->base_addr_,
      xc->size() * dtype_size_bytes(xc->dtype_));
  }

  default:
    fmt(cerr, "Type error or equality test not yet supported\n");
    AU();
  }
}

value_p value_column::load_column_from_disk_path(
  string path, optional<ref_context_p> refs_accum, optional<url_p> url_context,
  optional<int64_t> value_id) {

  auto ret_view = make_shared<column_view_variable>(path, url_context);
  auto ret_view_v = column_view_v(ret_view);
  auto ret = value::create(
    value_column::create(ret_view_v),
    ret_view->type_,
    refs_accum,
    url_context,
    value_id);
  return ret;
}

value_p value_column::load_column_from_binary_data(
  binary_data_view_fixed_p meta_view,
  binary_data_view_fixed_p top_view,
  binary_data_view_variable_p entries_view,
  optional<ref_context_p> refs_accum,
  optional<url_p> url_context,
  optional<int64_t> value_id) {

  auto ret_view = make_shared<column_view_variable>(
    meta_view, top_view, entries_view, url_context);
  auto ret_view_v = column_view_v(ret_view);
  auto ret = value::create(
    value_column::create(ret_view_v),
    ret_view->type_,
    refs_accum,
    url_context,
    value_id);
  return ret;
}

value_type_p import_column_type_raw_sf(flex_type_enum type) {
  switch (type) {
  case flex_type_enum::INTEGER:
    return value_type::create_scalar(dtype_enum::I64);

  case flex_type_enum::FLOAT:
    return value_type::create_scalar(dtype_enum::F64);

  case flex_type_enum::STRING:
    return value_type::create_string();

  case flex_type_enum::VECTOR:
  case flex_type_enum::LIST:
  case flex_type_enum::DICT:
  case flex_type_enum::DATETIME:
  case flex_type_enum::IMAGE:
  case flex_type_enum::ND_VECTOR:
    log_and_throw("flex_type_enum case not yet supported");

  case flex_type_enum::UNDEFINED:
    log_and_throw(
      "Error: flex_type_enum::UNDEFINED found as the type of an SArray");

  default:
    cerr << static_cast<int64_t>(type) << endl;
    AU();
  }
}

void get_raw_sf_scalar(
  value_column* src, int64_t i, sarray<flexible_type>::iterator& dst,
  dtype_enum dtype) {

  switch (dtype) {
  case dtype_enum::I64: {
    (*dst) = column_value_get_raw_scalar<int64_t>(src, i);
    break;
  }
  case dtype_enum::F64: {
    (*dst) = column_value_get_raw_scalar<double>(src, i);
    break;
  }
  default:
    log_and_throw("Error: data type not yet supported");
    AU();
  }

  ++dst;
}

void get_raw_sf_string(
  value_column* src, int64_t i, sarray<flexible_type>::iterator& dst) {

  (*dst) = column_value_get_raw_string(src, i);
  ++dst;
}

void put_raw_sf(
  column_builder_p& builder, flexible_type v, int64_t i, int64_t worker_index) {

  switch (v.get_type()) {
  case flex_type_enum::INTEGER:
    column_value_put_raw_scalar<int64_t>(builder, v.to<int64_t>(), i, worker_index);
    break;
  case flex_type_enum::FLOAT:
    column_value_put_raw_scalar<double>(builder, v.to<double>(), i, worker_index);
    break;
  case flex_type_enum::STRING: {
    string s = v.to<string>();
    column_value_put_raw_1d<char>(builder, &s[0], s.length(), i, worker_index);
    break;
  }
  case flex_type_enum::VECTOR:
  case flex_type_enum::LIST:
  case flex_type_enum::DICT:
  case flex_type_enum::DATETIME:
  case flex_type_enum::IMAGE:
  case flex_type_enum::ND_VECTOR:
    log_and_throw("flex_type_enum case not yet supported");
  case flex_type_enum::UNDEFINED:
    log_and_throw("Error: flex_type_enum::UNDEFINED not supported");
  default:
    cerr << static_cast<int64_t>(v.get_type()) << endl;
    AU();
  }
}

value_p import_value_sf(flexible_type v) {
  switch (v.get_type()) {
  case flex_type_enum::INTEGER:
    return value_nd_vector::create_scalar_int64(v.to<int64_t>());
  case flex_type_enum::FLOAT:
    return value_nd_vector::create_scalar_float64(v.to<double>());
  case flex_type_enum::STRING: {
    string s = v.to<string>();
    return value_nd_vector::create_from_string(s);
  }
  case flex_type_enum::VECTOR:
  case flex_type_enum::LIST:
  case flex_type_enum::DICT:
  case flex_type_enum::DATETIME:
  case flex_type_enum::IMAGE:
  case flex_type_enum::ND_VECTOR:
    log_and_throw("flex_type_enum case not yet supported");
  case flex_type_enum::UNDEFINED:
    log_and_throw("Error: flex_type_enum::UNDEFINED not supported");
  default:
    cerr << static_cast<int64_t>(v.get_type()) << endl;
    AU();
  }
}

value_p from_sframe(const gl_sframe& sf) {
  vector<string> column_names = sf.column_names();
  ASSERT_TRUE(all_distinct(column_names));
  vector<flex_type_enum> column_types = sf.column_types();

  int64_t num_columns = len(column_names);
  ASSERT_EQ(len(column_types), num_columns);

  vector<value_type_p> ret_column_element_types;
  vector<value_p> ret_columns;

  for (int64_t i = 0; i < num_columns; i++) {
    auto sf_type_i = at(column_types, i);
    gl_sarray sf_column_i = sf.select_column(at(column_names, i));

    auto raw_type_i = import_column_type_raw_sf(sf_type_i);

    bool is_optional = false;
    auto type_i = raw_type_i;
    if (sf_column_i.num_missing() != 0) {
      is_optional = true;
      type_i = value_type::create_optional(raw_type_i);
    }

    auto builder_i = column_builder_create(type_i);

    int64_t n = sf_column_i.size();

    builder_i->extend_length_raw(n);

    int64_t nt = turi::thread_pool::get_instance().size();
    int64_t chunk_size = ceil_divide(n, nt);

    turi::in_parallel_debug([&](int64_t k, int64_t num_threads_actual) {
      ASSERT_EQ(num_threads_actual, nt);

      int64_t start_k = k * chunk_size;
      int64_t end_k = min<int64_t>((k+1) * chunk_size, n);

      if (start_k >= n) {
        return;
      }

      int64_t j = start_k;

      for (flexible_type v_ij : sf_column_i.range_iterator(start_k, end_k)) {
        if (is_optional) {
          if (v_ij.get_type() == flex_type_enum::UNDEFINED) {
            builder_i->put(value::create_optional_none(type_i), j, k);
          } else {
            builder_i->put(
              value::create_optional_some(
                type_i, import_value_sf(v_ij)), j, k);
          }
        } else {
          ASSERT_EQ(v_ij.get_type(), sf_type_i);
          put_raw_sf(builder_i, v_ij, j, k);
        }
        ++j;
      }
    });

    auto column_i = builder_i->finalize();
    ret_columns.push_back(column_i);
    ret_column_element_types.push_back(type_i);
  }

  auto ret_type = value_type_table_create(
    column_names, ret_column_element_types);
  auto ret = value::create(
    make_shared<value_record>(ret_type, ret_columns),
    ret_type,
    NONE<ref_context_p>(),
    NONE<url_p>(),
    NONE<int64_t>());

  return ret;
}

gl_sarray column_to_sarray(value_p v, value_type_p ty) {
  auto n = v->get_column_length();
  auto ret = make_shared<sarray<flexible_type>>();
  ret->open_for_write();
  bool is_string = false;
  auto src = v->get_as_direct_column();

  auto cc_ty = ty->as<value_type_nd_vector_p>();
  if (cc_ty->ndim_ == 0) {
    switch (cc_ty->dtype_) {
    case dtype_enum::I64: ret->set_type(flex_type_enum::INTEGER); break;
    case dtype_enum::F64: ret->set_type(flex_type_enum::FLOAT); break;
    default:
      AU();
    }
  } else if (
    cc_ty->ndim_ == 1 &&
    cc_ty->dtype_ == dtype_enum::I8 &&
    ty->tag_ == value_type_tag_enum::STRING) {
    ret->set_type(flex_type_enum::STRING);
    is_string = true;
  } else {
    cerr << "Data type not yet supported" << endl;
    AU();
  }

  auto dst = ret->get_output_iterator(0);
  for (int64_t i = 0; i < n; i++) {
    if (!!src) {
      if (is_string) {
        get_raw_sf_string(*src, i, dst);
      } else {
        get_raw_sf_scalar(*src, i, dst, cc_ty->dtype_);
      }
    } else {
      if (is_string) {
        (*dst) = value_column_at(v, i)->get_value_string();
      } else if (cc_ty->dtype_ == dtype_enum::I64) {
        (*dst) = value_column_at(v, i)->get_value_scalar_int64();
      } else if (cc_ty->dtype_ == dtype_enum::F64) {
        (*dst) = value_column_at(v, i)->get_value_scalar_float64();
      } else {
        AU();
      }
      ++dst;
    }
  }

  ret->close();

  return gl_sarray(ret);
}

gl_sframe to_sframe(value_p v) {
  ASSERT_TRUE(!!v->ty_->tag_);
  ASSERT_EQ(*v->ty_->tag_, value_type_tag_enum::DATA_TABLE);
  auto column_types = v->ty_->as<value_type_record_p>()->field_types_;
  int64_t n = len(column_types);
  auto cc = v->as<value_record_p>();
  gl_sframe ret;
  for (int64_t i = 0; i < n; i++) {
    auto fname = at(column_types, i).first;
    auto fty = at(column_types, i).second->as<value_type_column_p>();
    if (fty->element_type_->which() == value_type_enum::ND_VECTOR) {
      ret.add_column(
        column_to_sarray(at(cc->entries_, i), fty->element_type_), fname);
    } else {
      cerr << "Data type not yet supported" << endl;
      AU();
    }
  }
  return ret;
}

const char* column_metadata::object_id_    = "CM";
const char* object_ids_builtin::pair_      = "PA";
const char* query::object_id_              = "QU";
const char* value::object_id_              = "VA";
const char* object_ids_builtin::vector_    = "VE";
const char* value_type::object_id_         = "VT";

}}
