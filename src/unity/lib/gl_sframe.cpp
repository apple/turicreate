/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ctime>
#include <deque>
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <parallel/pthread_tools.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <sframe/sframe.hpp>
#include <sframe/sframe_reader.hpp>
#include <sframe/sframe_reader_buffer.hpp>
#include <sframe/dataframe.hpp>
#include <sframe_query_engine/planning/planner.hpp>
#include <table_printer/table_printer.hpp>

namespace turi {

/**
 * Constructs a dataframe from data represented a collection of 
 * columns of flexible_type.
 */
void make_dataframe_from_data(
    const std::map<std::string, std::vector<flexible_type> >& data,
    dataframe_t& df) {

  if (data.size() > 0) {
    size_t nrows = data.begin()->second.size(); 
    for (const auto& keyval: data) {
      if (keyval.second.size() != nrows) {
        throw std::string("Columns must be of the same length");
      }
    }
  }
  df.values = data;
  for (const auto& keyval: data) {
    df.names.push_back(keyval.first);
    df.types[keyval.first] = infer_type_of_list(keyval.second);
  }
}

std::shared_ptr<unity_sframe> gl_sframe::get_proxy() const { return m_sframe; }

/**************************************************************************/
/*                                                                        */
/*                              Aggregators                               */
/*                                                                        */
/**************************************************************************/
namespace aggregate {
groupby_descriptor_type SUM(const std::string& col) {
  return {"__builtin__sum__", {col}};
}
groupby_descriptor_type ARGMAX(const std::string& agg, const std::string& out) {
  return {"__builtin__argmax__", {agg, out}};
}
groupby_descriptor_type ARGMIN(const std::string& agg, const std::string& out) {
  return {"__builtin__argmin__", {agg, out}};
}
groupby_descriptor_type MAX(const std::string& col) {
  return {"__builtin__max__", {col}};
}
groupby_descriptor_type MIN(const std::string& col) {
  return {"__builtin__min__", {col}};
}
groupby_descriptor_type COUNT() {
  return {"__builtin__count__", std::vector<std::string>()};
}
groupby_descriptor_type MEAN(const std::string& col) {
  return {"__builtin__avg__", {col}};
}
groupby_descriptor_type AVG(const std::string& col) {
  return {"__builtin__avg__", {col}};
}
groupby_descriptor_type VAR(const std::string& col) {
  return {"__builtin__var__", {col}};
}
groupby_descriptor_type VARIANCE(const std::string& col) {
  return {"__builtin__var__", {col}};
}
groupby_descriptor_type STD(const std::string& col) {
  return {"__builtin__stdv__", {col}};
}
groupby_descriptor_type STDV(const std::string& col) {
  return {"__builtin__stdv__", {col}};
}
groupby_descriptor_type SELECT_ONE(const std::string& col) {
  return {"__builtin__select_one__", {col}};
}
groupby_descriptor_type CONCAT(const std::string& col) {
  return {"__builtin__concat__list__", {col}};
}
groupby_descriptor_type CONCAT(const std::string& key, const std::string& value) {
  return {"__builtin__concat__dict__", {key, value}};
}

groupby_descriptor_type COUNT_DISTINCT(const std::string& col) {
  return {"__builtin__count__distinct__", {col}};
}

groupby_descriptor_type QUANTILE(const std::string& col, double quantile) {
  std::vector<double> q{quantile};
  return QUANTILE(col, q);
}
groupby_descriptor_type QUANTILE(const std::string& col, const std::vector<double>& quantiles) {
  std::string query = "__builtin__quantile__[";
  for (size_t i = 0;i < quantiles.size(); ++i) {
    query = query + std::to_string(quantiles[i]); 
    if (i < quantiles.size() - 1) query = query + ",";
  }
  query = query + "]";
  return {query, {col}};
}

groupby_descriptor_type::groupby_descriptor_type(const std::string& builtin_operator_name,
                                                 const std::vector<std::string>& group_columns) :
      m_group_columns(group_columns), m_aggregator(get_builtin_group_aggregator(builtin_operator_name)) { }

groupby_descriptor_type::groupby_descriptor_type(std::shared_ptr<group_aggregate_value> aggregator,
                                                 const std::vector<std::string>& group_columns) :
      m_group_columns(group_columns), m_aggregator(aggregator) { }

} // aggregate

/**************************************************************************/
/*                                                                        */
/*                         gl_sframe constructors                         */
/*                                                                        */
/**************************************************************************/

gl_sframe::gl_sframe() {
  instantiate_new();
}

gl_sframe::gl_sframe(const gl_sframe& other) {
  m_sframe = std::dynamic_pointer_cast<unity_sframe>(
    other.get_proxy()->select_columns(other.get_proxy()->column_names()));
}

gl_sframe::gl_sframe(gl_sframe&& other) {
  m_sframe = other.get_proxy();
}

gl_sframe::gl_sframe(const std::string& directory) {
  instantiate_new();
  m_sframe->construct_from_sframe_index(directory);
}

void gl_sframe::construct_from_sframe_index(const std::string& directory){
  m_sframe->construct_from_sframe_index(directory);
}

void gl_sframe::construct_from_csvs(std::string csv_file, csv_parsing_config_map csv_config,
  str_flex_type_map column_type_hints){
  if (column_type_hints.size() == 0) {
    // we need to do type inference
    gl_sframe temp;
    auto config_copy = csv_config;
    config_copy["row_limit"] = 100;
    str_flex_type_map undefined_type_hints{{"__all_columns__",flex_type_enum::UNDEFINED}};
    temp.construct_from_csvs(csv_file, config_copy, undefined_type_hints);

    for (const auto colname : temp.column_names()) {
      // get a range over the entire array
      auto ra = temp[colname].range_iterator();
      std::vector<flexible_type> column_values;
      std::copy(ra.begin(), ra.end(), std::inserter(column_values, column_values.end()));
      column_type_hints[colname] = infer_type_of_list(column_values);
    }
  }
  m_sframe->construct_from_csvs(csv_file, csv_config, column_type_hints); 
}

gl_sframe::gl_sframe(const std::map<std::string, std::vector<flexible_type> >& data) {
  instantiate_new();
  dataframe_t df;
  make_dataframe_from_data(data, df);
  get_proxy()->construct_from_dataframe(df);
}

void gl_sframe::construct_from_dataframe(const std::map<std::string, std::vector<flexible_type> >& data) {
  dataframe_t df;
  make_dataframe_from_data(data, df);
  m_sframe->construct_from_dataframe(df);
}

gl_sframe& gl_sframe::operator=(const gl_sframe& other) {
  m_sframe = std::dynamic_pointer_cast<unity_sframe>(
    other.get_proxy()->select_columns(other.get_proxy()->column_names()));
  return *this;
}


gl_sframe& gl_sframe::operator=(gl_sframe&& other) {
  m_sframe = other.get_proxy();
  return *this;
}

gl_sframe::gl_sframe(const std::map<std::string, gl_sarray>& data) {
  instantiate_new();
  std::list<std::shared_ptr<unity_sarray_base>> arraylist;
  std::vector<std::string> names;
  for (const auto& col: data) {
    names.push_back(col.first); 
    arraylist.push_back(col.second);
  }
  get_proxy()->add_columns(arraylist, names);
}

gl_sframe::gl_sframe(
    std::initializer_list<std::pair<std::string, gl_sarray>> ilist) {
  std::map<std::string, gl_sarray> data;
  for (auto& i: ilist) data[i.first] = i.second;
  instantiate_new();

  std::list<std::shared_ptr<unity_sarray_base>> arraylist;
  std::vector<std::string> names;
  for (const auto& col: data) {
    names.push_back(col.first); 
    arraylist.push_back(col.second);
  }
  get_proxy()->add_columns(arraylist, names);
}

/**************************************************************************/
/*                                                                        */
/*                   gl_sframe Implicit Cast Operators                    */
/*                                                                        */
/**************************************************************************/

gl_sframe::gl_sframe(std::shared_ptr<unity_sframe> sframe) {
  m_sframe = sframe;
}

gl_sframe::gl_sframe(std::shared_ptr<unity_sframe_base> sframe) {
  m_sframe = std::dynamic_pointer_cast<unity_sframe>(sframe);
}

gl_sframe::gl_sframe(const sframe& sf)
    : m_sframe(new unity_sframe)
{
  m_sframe->construct_from_sframe(sf);
}

gl_sframe::operator std::shared_ptr<unity_sframe>() const {
  return get_proxy();
}
gl_sframe::operator std::shared_ptr<unity_sframe_base>() const {
  return get_proxy();
}

sframe gl_sframe::materialize_to_sframe() const {
  return *(get_proxy()->get_underlying_sframe());
}

/**************************************************************************/
/*                                                                        */
/*                          gl_sframe operators                           */
/*                                                                        */
/**************************************************************************/
std::vector<flexible_type> gl_sframe::operator[](int64_t i) {
  if (i < 0 || (size_t)i >= size()) {
    throw std::string("Index out of range");
  }
  std::vector<std::vector<flexible_type> > rows(1);
  size_t rows_read  = get_sframe_reader()->read_rows(i, i + 1, rows);
  ASSERT_TRUE(rows.size() > 0);
  ASSERT_EQ(rows_read, 1);
  return rows[0];
}

std::vector<flexible_type> gl_sframe::operator[](int64_t i) const {
  if (i < 0 || (size_t)i >= size()) {
    throw std::string("Index out of range");
  }
  std::vector<std::vector<flexible_type> > rows(1);
  size_t rows_read  = get_sframe_reader()->read_rows(i, i + 1, rows);
  ASSERT_TRUE(rows.size() > 0);
  ASSERT_EQ(rows_read, 1);
  return rows[0];
}


gl_sframe gl_sframe::operator[](const std::initializer_list<int64_t>& _slice) {
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

gl_sframe gl_sframe::operator[](const std::initializer_list<int64_t>& _slice) const {
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

const_gl_sarray_reference gl_sframe::operator[](const std::string& column) const {
  return const_gl_sarray_reference(*this, column);
}

gl_sarray_reference gl_sframe::operator[](const std::string& column) {
  return gl_sarray_reference(*this, column);
}

gl_sframe gl_sframe::operator[](const std::vector<std::string>& columns) const {
  return select_columns(columns);
}

gl_sframe gl_sframe::operator[](const std::initializer_list<std::string>& columns) const {
  return select_columns(columns);
}

gl_sframe gl_sframe::operator[](const std::initializer_list<std::string>& columns) {
  return select_columns(columns);
}

gl_sframe gl_sframe::operator[](const gl_sarray& logical_filter) const {
  return get_proxy()->logical_filter(logical_filter);
}

/**************************************************************************/
/*                                                                        */
/*                               Iterators                                */
/*                                                                        */
/**************************************************************************/

void gl_sframe::materialize_to_callback(
    std::function<bool(size_t, const std::shared_ptr<sframe_rows>&)> callback,
    size_t nthreads) {
  if (nthreads == (size_t)(-1)) nthreads = SFRAME_DEFAULT_NUM_SEGMENTS;
  turi::query_eval::planner().materialize(get_proxy()->get_planner_node(),
                                              callback,
                                              nthreads);
}

gl_sframe_range gl_sframe::range_iterator(size_t start, size_t end) const {
  if (end == (size_t)(-1)) end = get_proxy()->size();
  if (start > end) {
    throw std::string("start must be less than end");
  }
  // basic range check. start must point to existing element, end can point
  // to one past the end. 
  // but additionally, we need to permit the special case start == end == 0
  // so that you can iterate over empty arrays.
  if (!((start < get_proxy()->size() && end <= get_proxy()->size()) ||
        (start == 0 && end == 0))) {
    throw std::string("Index out of range");
  }
  return gl_sframe_range(get_sframe_reader(), start, end);
}

/**************************************************************************/
/*                                                                        */
/*                               Functions                                */
/*                                                                        */
/**************************************************************************/

size_t gl_sframe::size() const {
  return get_proxy()->size();
}
bool gl_sframe::empty() const {
  return get_proxy()->size() == 0;
}

bool gl_sframe::is_materialized() const {
  return get_proxy()->is_materialized();
}

bool gl_sframe::has_size() const {
  return get_proxy()->has_size();
}

void gl_sframe::materialize() {
  get_proxy()->materialize();
}

void gl_sframe::save(const std::string& _path, const std::string& _format) const {
  std::string path = _path;
  std::string format = _format;
  // fill format is not filled
  if (format == "") {
    if (boost::algorithm::ends_with(path, ".csv") || boost::algorithm::ends_with(path, ".csv.gz")) {
      format = "csv";
    } else {
      format = "binary";
    }
  }

  // append .csv if is csv
  if (format == "csv") {
    if(!(boost::algorithm::ends_with(path, ".csv") || boost::algorithm::ends_with(path, ".csv.gz"))) {
      path = path + ".csv";
    }
  } else if (format != "binary") {
    throw std::string("Invalid format. Supported formats are \'csv\' and \'binary\'");
  }

  // save
  if (format == "csv") {
    get_proxy()->save_as_csv(path, std::map<std::string, flexible_type>());
  } else if (format == "binary") {
    get_proxy()->save_frame(path);
  }
}

void gl_sframe::save_reference(const std::string& path) const {
    get_proxy()->save_frame_reference(path);
}

std::vector<flex_type_enum> gl_sframe::column_types() const {
  return get_proxy()->dtype();
}
size_t gl_sframe::num_columns() const {
  return get_proxy()->num_columns();
}

void gl_sframe::instantiate_new() {
  m_sframe = std::make_shared<unity_sframe>();
}

std::shared_ptr<sframe_reader> gl_sframe::get_sframe_reader() const {
  return get_proxy()->get_underlying_sframe()->get_reader();
}

std::vector<std::string> gl_sframe::column_names() const {
  return get_proxy()->column_names();
}

gl_sframe gl_sframe::head(size_t n) const {
  return get_proxy()->head(n);
}
gl_sframe gl_sframe::tail(size_t n) const {
  return get_proxy()->tail(n);
}


gl_sarray gl_sframe::apply(std::function<flexible_type(const sframe_rows::row&)> fn,
                flex_type_enum dtype) const {
  return get_proxy()->transform_lambda(fn, dtype, time(NULL));
}

gl_sframe gl_sframe::sample(double fraction) const {
  return get_proxy()->sample(fraction, time(NULL));
}
gl_sframe gl_sframe::sample(double fraction, size_t seed, bool exact) const {
  return get_proxy()->sample(fraction, seed, exact);
}
std::pair<gl_sframe, gl_sframe> gl_sframe::random_split(double fraction) const {
  return random_split(fraction, time(NULL));
}

std::pair<gl_sframe, gl_sframe> gl_sframe::random_split(double fraction, size_t seed, bool exact) const {
  auto list = get_proxy()->random_split(fraction, seed, exact);
  std::pair<gl_sframe, gl_sframe> ret;
  ASSERT_EQ(list.size(), 2);
  auto iter = list.begin(); ret.first = (*iter);
  ++iter; ret.second = (*iter);
  return ret;
}

gl_sframe gl_sframe::topk(const std::string& column_name, 
                          size_t k, bool reverse) const {
  return (*this)[(*this)[column_name].topk_index(k, reverse)].sort(column_name, reverse);
}

size_t gl_sframe::column_index(const std::string &column_name) const {
  return get_proxy()->column_index(column_name);
}

/**  Returns the name of column `index`.
 */
const std::string& gl_sframe::column_name(size_t index) const {
  return get_proxy()->column_name(index);
}

bool gl_sframe::contains_column(const std::string& column_name) const {
  return get_proxy()->contains_column(column_name);
}

gl_sarray gl_sframe::select_column(const std::string& colname) const {
  return get_proxy()->select_column(colname);
}
gl_sframe gl_sframe::select_columns(const std::vector<std::string>& colnames) const {
  return get_proxy()->select_columns(colnames);
}

void gl_sframe::add_column(const flexible_type& data, const std::string& name) {
  get_proxy()->add_column(gl_sarray::from_const(data, size()), name);
}

void gl_sframe::add_column(const gl_sarray& data, const std::string& name) {
  get_proxy()->add_column(data, name);
}
void gl_sframe::replace_add_column(const gl_sarray& data, const std::string& name) {
  if (num_columns() == 0) {
    // starting from empty sframe
    add_column(data, name);
    return;
  } 
  
  auto colnames = column_names();
  std::set<std::string> colnameset(colnames.begin(), colnames.end());
  // new column
  if (colnameset.count(name) == 0) {
    add_column(data, name);
    return;
  } 
  // replacing a column
  if (num_columns() == 1) {
    // special handling for single column. 
    // we want to permit replacing the column even when the size changes.
    // so we remove it first, try to add it, then if there is a failure,
    // put it back.
    auto saved_column = (*this)[name];
    remove_column(name);
    try {
      add_column(data, name);
    } catch (...) {
      add_column(saved_column, name);
    }
  }  else {
    std::string tempname = "-" + name;
    while(colnameset.count(tempname)) tempname = "-" + tempname;
    add_column(data, tempname);
    swap_columns(tempname, name);
    remove_column(name);
    rename({{tempname, name}});
  }
  
}

void gl_sframe::add_columns(const gl_sframe& data) {
  std::list<std::shared_ptr<unity_sarray_base>> arraylist;
  std::vector<std::string> names;
  for (const auto& col: data.column_names()) {
    names.push_back(col); 
    arraylist.push_back(data.select_column(col));
  }
  get_proxy()->add_columns(arraylist, names);
}


void gl_sframe::remove_column(const std::string& name) {
  auto colnames = column_names();
  auto iter = std::find(colnames.begin(), colnames.end(), name);
  if (iter == colnames.end()) throw std::string("No such column name");

  get_proxy()->remove_column(iter - colnames.begin());
}
void gl_sframe::swap_columns(const std::string& column_1, const std::string& column_2) {
  auto colnames = column_names();
  auto iter_1 = std::find(colnames.begin(), colnames.end(), column_1);
  if (iter_1 == colnames.end()) throw std::string("No such column name");
  auto iter_2 = std::find(colnames.begin(), colnames.end(), column_2);
  if (iter_2 == colnames.end()) throw std::string("No such column name");

  get_proxy()->swap_columns(iter_1 - colnames.begin(), iter_2 - colnames.begin());
}

void gl_sframe::rename(const std::map<std::string, std::string>& old_to_new_names) {
  auto colnames = column_names();
  for (const auto& vals: old_to_new_names) {
    auto iter = std::find(colnames.begin(), colnames.end(), vals.first);
    if (iter == colnames.end()) throw std::string("No such column name");
    get_proxy()->set_column_name(iter - colnames.begin(), vals.second);
  }
}

gl_sframe gl_sframe::append(const gl_sframe& other) const {
  if (num_columns() != other.num_columns()) {
    throw std::string("Two SFrames have to have the same number of columns");
  }  
  return get_proxy()->append(other.select_columns(column_names()));
}

gl_sframe gl_sframe::groupby(const std::vector<std::string>& groupkeys, 
                             const std::map<std::string, aggregate::groupby_descriptor_type>& operators) const{
  std::vector<std::vector<std::string>> group_columns;
  std::vector<std::string> output_columns;
  std::vector<std::shared_ptr<group_aggregate_value>> group_ops;
  for (const auto& op: operators) {
    std::shared_ptr<group_aggregate_value> aggregator;
    if (op.second.m_aggregator->name() == "Sum" && (*this)[op.second.m_group_columns[0]].dtype() == flex_type_enum::VECTOR){
      aggregator = get_builtin_group_aggregator("__builtin__vector__sum__");
    } else if (op.second.m_aggregator->name() == "Avg" && (*this)[op.second.m_group_columns[0]].dtype() == flex_type_enum::VECTOR){
      aggregator = get_builtin_group_aggregator("__builtin__vector__avg__");
    } else {
      aggregator = op.second.m_aggregator;
    }
    output_columns.push_back(op.first);
    group_columns.push_back(op.second.m_group_columns);
    group_ops.push_back(aggregator);
  }
  return get_proxy()->groupby_aggregate(groupkeys,
                                        group_columns,
                                        output_columns,
                                        group_ops);
}

gl_sframe gl_sframe::join(const gl_sframe& right, 
                          const std::vector<std::string>& joinkeys, 
                          const std::string& how) const {
  std::map<std::string, std::string> keys;
  for (const auto& i: joinkeys) keys[i] = i;
  return get_proxy()->join(right, how, keys);
}

gl_sframe gl_sframe::join(const gl_sframe& right, 
                          const std::map<std::string, std::string>& joinkeys, 
                          const std::string& how) const {
  return get_proxy()->join(right, how, joinkeys);
}

gl_sframe gl_sframe::filter_by(const gl_sarray& values, 
                               const std::string& column_name, 
                               bool exclude) const {
  auto colnames = column_names();
  std::set<std::string> colnameset(colnames.begin(), colnames.end());
  if (colnameset.count(column_name) == 0) {
    throw std::string("Column ") + column_name + " not in SFrame";
  }
  if ((*this)[column_name].dtype() != values.dtype()) {
    throw std::string("Type of given values does not match type of column ") + 
        column_name + " in SFrame";
  }
  gl_sframe value_sf({{column_name, values}});
  value_sf = value_sf.unique();
  if (exclude == false) {
    return join(value_sf, {column_name}, "inner");
  } else {
    std::string id_name = "id";
    while(colnameset.count(id_name)) id_name += "1";
    value_sf[id_name] = 1;
    auto retsf = join(value_sf, {column_name}, "left");
    retsf = retsf[retsf[id_name] == FLEX_UNDEFINED];
    retsf.remove_column(id_name);
    return retsf;
  }

}


gl_sframe gl_sframe::pack_columns(const std::vector<std::string>& columns,
                                  const std::string& new_column_name,
                                  flex_type_enum dtype,
                                  flexible_type fill_na) const {
  if (columns.size() == 0) {
    throw std::string("Please provide at least two columns to pack");
  }

  auto cur_colnames = column_names();
  std::set<std::string> cur_colname_set(cur_colnames.begin(), cur_colnames.end());
  for (auto i : columns) {
    if (cur_colname_set.count(i) == 0) {
      throw std::string("Current SFrame has no column called " + i);
    }
  }

  if (dtype != flex_type_enum::LIST &&
      dtype != flex_type_enum::VECTOR && 
      dtype != flex_type_enum::DICT) {
    throw std::string("Resulting dtype has to be one of dict/vector/list type");
  }

  if (dtype == flex_type_enum::VECTOR && 
      (fill_na.get_type() != flex_type_enum::UNDEFINED ||
       fill_na.get_type() != flex_type_enum::FLOAT ||
       fill_na.get_type() != flex_type_enum::INTEGER)) {
    throw std::string("fill_na value for array needs to be numeric type");
  }
  std::vector<std::string> dictkeys = columns;

  std::vector<std::string> rest_columns;
  for(auto i : column_names()) {
    if (std::find(columns.begin(), columns.end(), i) == columns.end()) {
      rest_columns.push_back(i);
    }
  }

  auto array = get_proxy()->pack_columns(columns, dictkeys, dtype, fill_na);
  auto ret = select_columns(rest_columns);
  ret.add_column(array, new_column_name);
  return ret;
}


gl_sframe gl_sframe::pack_columns(const std::string& column_prefix,
                                  const std::string& new_column_name,
                                  flex_type_enum dtype,
                                  flexible_type fill_na) const {
  std::vector<std::string> columns;
  for (auto i: column_names()) {
    if (boost::algorithm::starts_with(i, column_prefix)) {
      columns.push_back(i);
    }
  }
  if (columns.size() == 0) {
    throw std::string("There are no columns beginning with prefix ") + column_prefix;
  }
  return pack_columns(columns, new_column_name, dtype, fill_na);
}

gl_sframe gl_sframe::split_datetime(const std::string& expand_column,
                            const std::string& _column_name_prefix, 
                            const std::vector<std::string>& limit,
                            bool tzone) const {
  std::string column_name_prefix = _column_name_prefix;
  auto colnames = column_names();
  std::set<std::string> colnames_set(colnames.begin(), colnames.end());
  if (colnames_set.count(expand_column) == 0) {
    throw std::string("column \'" + expand_column + "\' does not exist in current SFrame");
  }
  if (column_name_prefix == "") {
    column_name_prefix = expand_column;
  }

  gl_sframe new_sf = select_column(expand_column).
      split_datetime(column_name_prefix, limit, tzone);

  std::vector<std::string> rest_columns;
  for (auto& colname: new_sf.column_names()) {
    if (colname != expand_column) {
      rest_columns.push_back(colname);
      if (colnames_set.count(colname)) new_sf.rename({{colname, colname + ".1"}});
    }
  }
  auto ret_sf = select_columns(rest_columns);
  ret_sf.add_columns(new_sf);
  return ret_sf;
}

gl_sframe gl_sframe::unpack(const std::string& unpack_column,
                            const std::string& _column_name_prefix, 
                            const std::vector<flex_type_enum>& column_types,
                            const flexible_type& na_value, 
                            const std::vector<flexible_type>& limit) const {
  std::string column_name_prefix = _column_name_prefix;
  auto colnames = column_names();
  std::set<std::string> colnames_set(colnames.begin(), colnames.end());
  if (colnames_set.count(unpack_column) == 0) {
    throw std::string("column \'" + unpack_column + "\' does not exist in current SFrame");
  }

  gl_sframe new_sf = select_column(unpack_column).
      unpack(column_name_prefix, column_types, na_value, limit);

  std::vector<std::string> rest_columns;

  for (auto& colname: column_names()) {
    if (colname != unpack_column) rest_columns.push_back(colname);
  }

  for (auto& colname: new_sf.column_names()) {
    if (colname != unpack_column) {
      if (colnames_set.count(colname)) new_sf.rename({{colname, colname + ".1"}});
    }
  }
  auto ret_sf = select_columns(rest_columns);
  ret_sf.add_columns(new_sf);
  return ret_sf;
}

gl_sframe gl_sframe::stack(const std::string& column_name,
                           const std::string& new_column_name,
                           bool drop_na) const {
  if (new_column_name == "") {
    return stack(column_name, std::vector<std::string>(), drop_na);
  } else {
    return stack(column_name, std::vector<std::string>{new_column_name}, drop_na);
  }
}

gl_sframe gl_sframe::stack(const std::string& column_name,
                           const std::vector<std::string>& _new_column_name,
                           bool drop_na) const {
  auto coltype = select_column(column_name).dtype();
  std::vector<std::string> new_column_name = _new_column_name;
  std::vector<flex_type_enum> new_column_type;

  // fail if column is not vector, recursive or list
  if (coltype != flex_type_enum::VECTOR &&
      coltype != flex_type_enum::LIST &&
      coltype != flex_type_enum::DICT) {
    throw std::string("Stack is only supported for column of dict/list/array type.");
  }

  // fill new column names
  if (new_column_name.size() == 0) {
    // infer column names
    if (coltype == flex_type_enum::VECTOR || coltype == flex_type_enum::LIST) {
      new_column_name = {""};
    } else if (coltype == flex_type_enum::DICT) {
      new_column_name = {"", ""};
    }
  } else if (new_column_name.size() == 1) {
    if (coltype == flex_type_enum::DICT) {
      throw std::string("new_column_name must contain 2 values to stack a dictionary.");
    }
  } else if (new_column_name.size() == 2) {
    if (coltype == flex_type_enum::VECTOR || coltype == flex_type_enum::LIST) {
      throw std::string("new_column_name must contain 1 value to stack an list or array");
    }
  }

  auto colnames = column_names();
  for (auto newcolname : new_column_name) {
    if (std::find(colnames.begin(), colnames.end(), newcolname) != colnames.end() && 
        newcolname != column_name) {
      throw std::string("Column name with ") + newcolname + " already exists";
    }
  }

  // infer types
  auto h = select_column(column_name).head(100).dropna();
  if (h.dtype() == flex_type_enum::VECTOR) {
    new_column_type = {flex_type_enum::FLOAT};
  } else if (h.dtype() == flex_type_enum::LIST) {
    // list. loop through the first 100 elements, and loop through 
    // each element
    std::vector<flexible_type> values;
    auto range = h.range_iterator();
    for (auto cell: range) {
      if (cell.get_type() == flex_type_enum::LIST) {
        flex_list list_vals = cell;
        std::copy(list_vals.begin(), list_vals.end(), std::inserter(values, values.end()));
      }
    }
    new_column_type = {infer_type_of_list(values)};
  } else if (h.dtype() == flex_type_enum::DICT) {
    // dict. loop through the first 100 elements, and loop through 
    // each element
    std::vector<flexible_type> keys;
    std::vector<flexible_type> values;
    for (const auto& x: h.range_iterator()) {
      if (x.get_type() != flex_type_enum::UNDEFINED) {
        for (const auto& val: x.to<flex_dict>()) {
          keys.push_back(val.first);
          values.push_back(val.second);
        }
      }
    }
    new_column_type = {infer_type_of_list(keys), infer_type_of_list(values)};
  }
  return get_proxy()->stack(column_name, new_column_name, 
                            new_column_type, drop_na);
}

gl_sframe gl_sframe::unstack(const std::string& column,
                             const std::string& new_column_name) const {
  std::vector<std::string> key_columns = column_names();
  auto iter = std::find(key_columns.begin(), key_columns.end(), column);
  if (iter == key_columns.end()) {
    throw std::string("column name ") + column + " not found";
  } 
  key_columns.erase(iter);
  if (new_column_name != "") {
    return groupby(key_columns, {{new_column_name, aggregate::CONCAT(column)}});
  } else {
    return groupby(key_columns, {{"", aggregate::CONCAT(column)}});
  }
}

gl_sframe gl_sframe::unstack(const std::vector<std::string>& columns,
                             const std::string& new_column_name) const {
  if (columns.size() != 2) throw std::string("columns must be a vector of two strings");
  std::vector<std::string> key_columns = column_names();
  for (auto& column: columns) {
    auto iter = std::find(key_columns.begin(), key_columns.end(), column);
    if (iter == key_columns.end()) {
      throw std::string("column name ") + column + " not found";
    } 
    key_columns.erase(iter);
  }
  if (new_column_name != "") {
    return groupby(key_columns, {{new_column_name, aggregate::CONCAT(columns[0], columns[1])}});
  } else {
    return groupby(key_columns, {{"", aggregate::CONCAT(columns[0], columns[1])}});
  }
}

gl_sframe gl_sframe::unique() const {
  return groupby(column_names(), 
                 std::map<std::string, aggregate::groupby_descriptor_type>());
}

gl_sframe gl_sframe::sort(const std::string& column, bool ascending) const {
  return get_proxy()->sort({column}, {ascending});
}
gl_sframe gl_sframe::sort(const std::vector<std::string>& columns, bool ascending) const {
  return get_proxy()->sort(columns, std::vector<int>(columns.size(), ascending));
}
gl_sframe gl_sframe::sort(const std::initializer_list<std::string>& columns, bool ascending) const {
  return get_proxy()->sort(columns, std::vector<int>(columns.size(), ascending));
}
gl_sframe gl_sframe::sort(const std::vector<std::pair<std::string, bool>>& column_and_ascending) const {
  std::vector<std::string> keys;
  std::vector<int> order;
  for (auto& col: column_and_ascending) {
    keys.push_back(col.first);
    order.push_back(col.second);
  }
  return get_proxy()->sort(keys, order);
}
gl_sframe gl_sframe::dropna(const std::vector<std::string>& columns, 
                            std::string how) const {
  auto ret = get_proxy()->drop_missing_values(columns, how == "all", false);
  ASSERT_EQ(ret.size(), 2);
  return *(ret.begin());
}
std::pair<gl_sframe, gl_sframe> gl_sframe::dropna_split(const std::vector<std::string>& columns, 
                                                        std::string how) const {
  auto ret = get_proxy()->drop_missing_values(columns, how == "all", false);
  ASSERT_EQ(ret.size(), 2);
  return {*(ret.begin()), *(ret.rbegin())};
}

gl_sframe gl_sframe::fillna(const std::string& column, flexible_type value) const {
  auto ret = *this;
  ret.replace_add_column(select_column(column).fillna(value), column);
  return ret;
}

gl_sframe gl_sframe::add_row_number(const std::string& column_name, size_t start) const {
  auto ret = *this;
  ret.add_column(gl_sarray::from_sequence(start, size()), column_name);
  return ret;
}

std::ostream& operator<<(std::ostream& out, const gl_sframe& other) {
  auto t = other.head(10);
  constexpr size_t COL_WIDTH= 14;
  constexpr size_t NUM_COLS = 5; // 80 / 16
  out << "\n";
  out << "Columns:\n";
  auto colnames = other.column_names();
  auto coldtype = other.column_types();
  for (size_t i = 0; i < colnames.size(); ++i) {
    out << "    " << colnames[i] << "\t" << flex_type_enum_to_name(coldtype[i]) << "\n";
  }
  if (other.has_size()) {
    out << "Rows: " << other.size() << "\n";
  } else {
    out << "Rows: ?\n";
  }
  out << "Data:\n";
  std::deque<std::string> colname_queue(colnames.begin(), colnames.end());
  while(!colname_queue.empty())  {
    std::vector<std::string> colnames;
    std::vector<std::pair<std::string, size_t> > cols;
    for (size_t i = 0;i < NUM_COLS && !colname_queue.empty(); ++i) {
      colnames.push_back(colname_queue.front());
      cols.push_back({colname_queue.front(), COL_WIDTH});
      colname_queue.pop_front();
    }
    table_printer printer(cols);
    printer.set_output_stream(out);
    printer.print_header();

    auto tsel = t[colnames];
    for(auto& row : tsel.range_iterator()) {
      std::vector<flexible_type> fval = row;
      for (auto& i :fval) {
        if (i.get_type() == flex_type_enum::UNDEFINED) {
          i = "None";
        }
      }
      printer.print_row(fval);
    }
    printer.print_footer();
  }

  if (other.has_size()) {
    out << "[" << other.size() << " rows x " << colnames.size() << " columns]\n";
  } else {
    out << "? rows x " << colnames.size() << " columns]\n";
  }
  return out;
}
/**************************************************************************/
/*                                                                        */
/*                            gl_sframe_range                             */
/*                                                                        */
/**************************************************************************/

gl_sframe_range::gl_sframe_range(
    std::shared_ptr<sframe_reader> _sframe_reader,
    size_t start, size_t end) {
  m_sframe_reader_buffer = 
      std::make_shared<sframe_reader_buffer>(_sframe_reader, start, end);
  // load the first value if available
  if (m_sframe_reader_buffer->has_next()) {
    m_sframe_reader_buffer->next();
  }
}
gl_sframe_range::iterator gl_sframe_range::begin() {
  return iterator(*this, true);
}
gl_sframe_range::iterator gl_sframe_range::end() {
  return iterator(*this, false);
}

/**************************************************************************/
/*                                                                        */
/*                       gl_sframe_range::iterator                        */
/*                                                                        */
/**************************************************************************/

gl_sframe_range::iterator::iterator(gl_sframe_range& range, bool is_start) {
  m_owner = &range;
  if (is_start) m_counter = 0;
  else m_counter = range.m_sframe_reader_buffer->size();
}

void gl_sframe_range::iterator::increment() {
  ++m_counter;
  if (m_owner->m_sframe_reader_buffer->has_next()) {
    m_owner->m_sframe_reader_buffer->next();
  }
}
void gl_sframe_range::iterator::advance(size_t n) {
  n = std::min(n, m_owner->m_sframe_reader_buffer->size());
  for (size_t i = 0;i < n ; ++i) increment();
}

const gl_sframe_range::type& gl_sframe_range::iterator::dereference() const {
  return m_owner->m_sframe_reader_buffer->current();
}

/**************************************************************************/
/*                                                                        */
/*                          gl_sarray_reference                           */
/*                                                                        */
/**************************************************************************/

gl_sarray_reference::gl_sarray_reference(gl_sarray_reference&& other)
  : m_sf(other.m_sf), m_column_name(other.m_column_name) { }

gl_sarray_reference& gl_sarray_reference::operator=(gl_sarray_reference&& other) {
  *this = static_cast<gl_sarray_reference&>(other);
  return *this;
}


gl_sarray_reference::gl_sarray_reference(gl_sframe& sf, std::string column_name)
  : m_sf(sf), m_column_name(column_name) { } 

gl_sarray_reference& gl_sarray_reference::operator=(const gl_sarray_reference& other) {
  m_sf.replace_add_column(gl_sarray(other), m_column_name);
  return *this;
}
gl_sarray_reference& gl_sarray_reference::operator=(const gl_sarray& other) {
  m_sf.replace_add_column(other, m_column_name);
  return *this;
}

gl_sarray_reference& gl_sarray_reference::operator=(const flexible_type& value) {
  if (m_sf.size() == 0) {
    m_sf.replace_add_column(gl_sarray::from_const(value, 1), m_column_name);
  } else {
    m_sf.replace_add_column(gl_sarray::from_const(value, m_sf.size()), m_column_name);
  }
  return *this;
}

std::shared_ptr<unity_sarray> gl_sarray_reference::get_proxy() const {
  return m_sf.select_column(m_column_name).get_proxy();
}


/**************************************************************************/
/*                                                                        */
/*                    const_gl_sarray_reference                           */
/*                                                                        */
/**************************************************************************/

const_gl_sarray_reference::const_gl_sarray_reference(const_gl_sarray_reference&& other)
  : m_sf(other.m_sf), m_column_name(other.m_column_name) { }


const_gl_sarray_reference::const_gl_sarray_reference(const gl_sframe& sf, std::string column_name)
  : m_sf(sf), m_column_name(column_name) { } 

std::shared_ptr<unity_sarray> const_gl_sarray_reference::get_proxy() const {
  return m_sf.select_column(m_column_name).get_proxy();
}




/**************************************************************************/
/*                                                                        */
/*                         gl_sframe_writer_impl                          */
/*                                                                        */
/**************************************************************************/

class gl_sframe_writer_impl {
 public:
  gl_sframe_writer_impl(const std::vector<std::string>& column_names,
                        const std::vector<flex_type_enum>& column_types, 
                        size_t num_segments);
  void write(const flexible_type& f, size_t segmentid);
  size_t num_segments() const;
  gl_sframe close();
 private:
  sframe m_out_sframe;
  std::vector<sframe::iterator> m_output_iterators;
};

gl_sframe_writer_impl::gl_sframe_writer_impl(const std::vector<std::string>& column_names,
                                             const std::vector<flex_type_enum>& column_types, 
                                             size_t num_segments) {
  // open the output frame
  if (num_segments == (size_t)(-1)) num_segments = SFRAME_DEFAULT_NUM_SEGMENTS;
  m_out_sframe.open_for_write(column_names, column_types, "", num_segments);

  // store the iterators
  m_output_iterators.resize(m_out_sframe.num_segments());
  for (size_t i = 0;i < m_out_sframe.num_segments(); ++i) {
    m_output_iterators[i] = m_out_sframe.get_output_iterator(i);
  }
}

void gl_sframe_writer_impl::write(const flexible_type& f, size_t segmentid) {
  ASSERT_LT(segmentid, m_output_iterators.size());
  *(m_output_iterators[segmentid]) = f;
}

size_t gl_sframe_writer_impl::num_segments() const {
  return m_output_iterators.size();
}

gl_sframe gl_sframe_writer_impl::close() {
  m_output_iterators.clear();
  m_out_sframe.close();
  auto usframe = std::make_shared<unity_sframe>();
  usframe->construct_from_sframe(m_out_sframe);
  return usframe;
}

void gl_sframe::show(const std::string& path_to_client) const {
  get_proxy()->show(path_to_client);
}

std::shared_ptr<model_base> gl_sframe::plot() const {
  return get_proxy()->plot();
}

/**************************************************************************/
/*                                                                        */
/*                            gl_sframe_writer                            */
/*                                                                        */
/**************************************************************************/

gl_sframe_writer::gl_sframe_writer(const std::vector<std::string>& column_names,
                                   const std::vector<flex_type_enum>& column_types, 
                                   size_t num_segments) {
  // create the pimpl
  m_writer_impl.reset(new gl_sframe_writer_impl(column_names, column_types, num_segments));
}

void gl_sframe_writer::write(const std::vector<flexible_type>& f, 
                             size_t segmentid) {
  m_writer_impl->write(f, segmentid);
}

size_t gl_sframe_writer::num_segments() const {
  return m_writer_impl->num_segments();
}

gl_sframe gl_sframe_writer::close() {
  return m_writer_impl->close();
}

gl_sframe_writer::~gl_sframe_writer() { }

} // turicreate
