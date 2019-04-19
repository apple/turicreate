/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <sframe/sframe.hpp>
#include <sframe/sframe_saving.hpp>
#include <sframe/sframe_config.hpp>
#include <sframe/sarray.hpp>
#include <sframe/algorithm.hpp>
#include <fileio/temp_files.hpp>
#include <fileio/sanitize_url.hpp>
#include <unity/lib/unity_global.hpp>
#include <unity/lib/unity_global_singleton.hpp>
#include <sframe/groupby_aggregate.hpp>
#include <sframe/groupby_aggregate_operators.hpp>
#include <sframe/csv_line_tokenizer.hpp>
#include <sframe/csv_writer.hpp>
#include <flexible_type/flexible_type_spirit_parser.hpp>
#include <sframe/join.hpp>
#include <unity/lib/auto_close_sarray.hpp>
#include <sframe_query_engine/planning/planner.hpp>
#include <sframe_query_engine/planning/optimization_engine.hpp>
#include <sframe_query_engine/operators/all_operators.hpp>
#include <sframe_query_engine/operators/operator_properties.hpp>
#include <sframe_query_engine/algorithm/sort.hpp>
#include <sframe_query_engine/algorithm/ec_sort.hpp>
#include <sframe_query_engine/algorithm/groupby_aggregate.hpp>
#include <sframe_query_engine/operators/operator_properties.hpp>
#include <exceptions/error_types.hpp>

#include <unity/lib/visualization/plot.hpp>
#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/histogram.hpp>
#include <unity/lib/visualization/escape.hpp>
#include <unity/lib/visualization/columnwise_summary.hpp>
#include <unity/lib/visualization/item_frequency.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <unity/lib/visualization/thread.hpp>
#include <unity/lib/visualization/summary_view.hpp>
#include <unity/lib/visualization/vega_data.hpp>
#include <unity/lib/visualization/vega_spec.hpp>

#include <unity/lib/image_util.hpp>
#include <unity/lib/unity_sketch.hpp>
#include <algorithm>
#include <string>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <logger/logger.hpp>
#include <util/basic_types.hpp>

#ifdef TC_HAS_PYTHON
#include <lambda/pylambda_function.hpp>
#endif
namespace turi {

using namespace turi::query_eval;

static std::shared_ptr<sframe> get_empty_sframe() {
  // make empty sframe and keep it around, reusing it whenever
  // I need an empty sframe. We are intentionally leaking this object.
  // Otherwise the termination of this will race against the cleanup of the
  // cache files.
  static std::shared_ptr<sframe>* sf = nullptr;
  static turi::mutex static_sf_lock;
  std::lock_guard<turi::mutex> guard(static_sf_lock);
  if (sf == nullptr) {
    sf = new std::shared_ptr<sframe>();
    (*sf) = std::make_shared<sframe>();
    (*sf)->open_for_write({}, {}, "", 1);
    (*sf)->close();
  }
  return *sf;
}
unity_sframe::unity_sframe() {
  this->set_sframe(get_empty_sframe());
}

unity_sframe::~unity_sframe() { clear(); }

void unity_sframe::construct_from_dataframe(const dataframe_t& df) {
  log_func_entry();
  clear();
  this->set_sframe(std::make_shared<sframe>(df));
}

void unity_sframe::construct_from_sframe(const sframe& sf) {
  log_func_entry();
  clear();
  this->set_sframe(std::make_shared<sframe>(sf));
}

void unity_sframe::construct_from_sframe_index(std::string location) {
  logstream(LOG_INFO) << "Construct sframe from location: " << sanitize_url(location) << std::endl;
  clear();

  auto status = fileio::get_file_status(location);
  if (fileio::is_web_protocol(fileio::get_protocol(location))) {
    // if it is a web protocol, we cannot be certain what type of file it is.
    // HEURISTIC:
    //   assume it is a "directory" and try to load dir_archive.ini
    //   if we can open it, it is a regular file. Otherwise not.
    if (fileio::try_to_open_file(location + "/dir_archive.ini")) {
      status = fileio::file_status::DIRECTORY;
    } else {
      status = fileio::file_status::REGULAR_FILE;
    }
  }

  if (status == fileio::file_status::MISSING) {
    // missing file. fail quick
    log_and_throw_io_failure(sanitize_url(location) + " not found.");
  } if (status == fileio::file_status::REGULAR_FILE) {
    // its a regular file, load it normally
    auto sframe_ptr = std::make_shared<sframe>(location);
    this->set_sframe(sframe_ptr);
  } else if (status == fileio::file_status::DIRECTORY) {
    // its a directory, open the directory and verify that it contains an
    // sarray and then load it if it does
    dir_archive dirarc;
    dirarc.open_directory_for_read(location);
    std::string content_value;
    if (dirarc.get_metadata("contents", content_value) == false ||
        content_value != "sframe") {
      log_and_throw_io_failure("Archive does not contain an SFrame");
    }
    std::string prefix = dirarc.get_next_read_prefix();
    auto sframe_ptr = std::make_shared<sframe>(prefix + ".frame_idx");
    this->set_sframe(sframe_ptr);
    dirarc.close();
  } else if(status == fileio::file_status::FS_UNAVAILABLE) {
    log_and_throw_io_failure("Cannot read from filesystem. Check log for details.");
  }
}

std::map<std::string, std::shared_ptr<unity_sarray_base>> unity_sframe::construct_from_csvs(
    std::string url,
    std::map<std::string, flexible_type> csv_parsing_config,
    std::map<std::string, flex_type_enum> column_type_hints) {

  logstream(LOG_INFO) << "Construct sframe from csvs at "
                      << sanitize_url(url) << std::endl;
  std::stringstream ss;
  ss << "Parsing config:\n";
  for (auto& pair: csv_parsing_config) {
    ss << "\t" << pair.first << ": " << pair.second << "\n";
  }
  logstream(LOG_INFO) << ss.str();

  clear();
  csv_line_tokenizer tokenizer;
  // first the defaults
  bool use_header = true;
  bool continue_on_failure = false;
  bool store_errors = false;
  size_t row_limit = 0;
  size_t skip_rows = 0;
  std::vector<std::string> output_columns;
  tokenizer.delimiter = ",";
  tokenizer.has_comment_char = false;
  tokenizer.escape_char = '\\';
  tokenizer.use_escape_char = true;
  tokenizer.double_quote = true;
  tokenizer.quote_char = '\"';
  tokenizer.skip_initial_space = true;
  tokenizer.na_values.clear();

  if (csv_parsing_config.count("use_header")) {
    use_header = !csv_parsing_config["use_header"].is_zero();
  }
  if (csv_parsing_config.count("continue_on_failure")) {
    continue_on_failure = !csv_parsing_config["continue_on_failure"].is_zero();
  }
  if (csv_parsing_config.count("store_errors")) {
    store_errors = !csv_parsing_config["store_errors"].is_zero();
  }
  if (csv_parsing_config.count("row_limit")) {
    row_limit = (flex_int)(csv_parsing_config["row_limit"]);
  }
  if (csv_parsing_config.count("skip_rows")) {
    skip_rows = (flex_int)(csv_parsing_config["skip_rows"]);
  }
  if (csv_parsing_config["delimiter"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string)csv_parsing_config["delimiter"];
    tokenizer.delimiter = tmp;
  } else if (csv_parsing_config["delimiter"].get_type() == flex_type_enum::UNDEFINED) {
    tokenizer.delimiter = "";
  }
  if (csv_parsing_config["comment_char"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string)csv_parsing_config["comment_char"];
    if (tmp.length() > 0) {
      tokenizer.comment_char= tmp[0];
      tokenizer.has_comment_char = true;
    }
  }
  if (csv_parsing_config.count("use_escape_char")) {
    tokenizer.skip_initial_space = !csv_parsing_config["use_escape_char"].is_zero();
  }
  if (csv_parsing_config["escape_char"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string)csv_parsing_config["escape_char"];
    if (tmp.length() > 0) tokenizer.escape_char = tmp[0];
  }
  if (csv_parsing_config.count("double_quote")) {
    tokenizer.double_quote = !csv_parsing_config["double_quote"].is_zero();
  }
  if (csv_parsing_config["quote_char"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string)csv_parsing_config["quote_char"];
    if (tmp.length() > 0) tokenizer.quote_char = tmp[0];
  } else if (csv_parsing_config["quote_char"].get_type() == flex_type_enum::UNDEFINED) {
    tokenizer.quote_char = NULL;
  }
  if (csv_parsing_config.count("skip_initial_space")) {
    tokenizer.skip_initial_space = !csv_parsing_config["skip_initial_space"].is_zero();
  }
  if (csv_parsing_config.count("only_raw_string_substitutions")) {
    tokenizer.only_raw_string_substitutions = !csv_parsing_config["only_raw_string_substitutions"].is_zero();
  }
  if (csv_parsing_config["na_values"].get_type() == flex_type_enum::LIST) {
    flex_list rec = csv_parsing_config["na_values"];
    tokenizer.na_values.clear();
    for (size_t i = 0;i < rec.size(); ++i) {
      if (rec[i].get_type() == flex_type_enum::STRING) {
        tokenizer.na_values.push_back((std::string)rec[i]);
      }
    }
  }
  if (csv_parsing_config["line_terminator"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string)csv_parsing_config["line_terminator"];
    tokenizer.line_terminator = tmp;
  } else if (csv_parsing_config["line_terminator"].get_type() == flex_type_enum::UNDEFINED) {
    tokenizer.line_terminator = "";
  }
  if (csv_parsing_config["output_columns"].get_type() == flex_type_enum::LIST) {
    flex_list rec = csv_parsing_config["output_columns"];
    output_columns.clear();
    for (size_t i = 0;i < rec.size(); ++i) {
      if (rec[i].get_type() == flex_type_enum::STRING) {
        output_columns.push_back((std::string)rec[i]);
      }
    }
  }
  if (csv_parsing_config["true_values"].get_type() == flex_type_enum::LIST) {
    flex_list rec = csv_parsing_config["true_values"];
    std::unordered_set<std::string> true_values;
    tokenizer.true_values.clear();
    for (size_t i = 0;i < rec.size(); ++i) {
      if (rec[i].get_type() == flex_type_enum::STRING) {
        tokenizer.true_values.insert((std::string)rec[i]);
      }
    }
  }

  if (csv_parsing_config["false_values"].get_type() == flex_type_enum::LIST) {
    flex_list rec = csv_parsing_config["false_values"];
    std::unordered_set<std::string> false_values;
    tokenizer.false_values.clear();
    for (size_t i = 0;i < rec.size(); ++i) {
      if (rec[i].get_type() == flex_type_enum::STRING) {
        tokenizer.false_values.insert((std::string)rec[i]);
      }
    }
  }
  tokenizer.init();

  auto sframe_ptr = std::make_shared<sframe>();

  auto errors = sframe_ptr->init_from_csvs(url,
                                           tokenizer,
                                           use_header,
                                           continue_on_failure,
                                           store_errors,
                                           column_type_hints,
                                           output_columns,
                                           row_limit,
                                           skip_rows);

  this->set_sframe(sframe_ptr);

  std::map<std::string, std::shared_ptr<unity_sarray_base>> errors_unity;
  for (auto& kv : errors) {
    std::shared_ptr<unity_sarray> sa(new unity_sarray());
    sa->construct_from_sarray(kv.second);
    errors_unity.insert(std::make_pair(kv.first, sa));
  }

  return errors_unity;
}


void unity_sframe::construct_from_planner_node(std::shared_ptr<planner_node> node,
                                               const std::vector<std::string>& column_names) {
  clear();

  materialize_options opts;
  opts.only_first_pass_optimizations = true;
  m_planner_node = optimization_engine::optimize_planner_graph(node, opts);

  // Do we need to materialize it for safety's sake?
  if(planner().online_materialization_recommended(m_planner_node)) {
    logstream(LOG_INFO) << "Forced materialization of SFrame due to size of lazy graph: " << std::endl;
    m_planner_node = planner().materialize_as_planner_node(m_planner_node);
  }

  m_column_names = column_names;
}

void unity_sframe::save_frame(std::string target_directory) {
  try {
    dir_archive dirarc;
    dirarc.open_directory_for_write(target_directory);
    dirarc.set_metadata("contents", "sframe");
    std::string prefix = dirarc.get_next_write_prefix();
    save_frame_by_index_file(prefix + ".frame_idx");
    dirarc.close();
  } catch(...) {
    throw;
  }
}


void unity_sframe::save_frame_reference(std::string target_directory) {
  try {
    dir_archive dirarc;
    dirarc.open_directory_for_write(target_directory);
    dirarc.set_metadata("contents", "sframe");
    std::string prefix = dirarc.get_next_write_prefix();
    sframe_save_weak_reference(*get_underlying_sframe(), prefix + ".frame_idx");
    dirarc.close();
  } catch(...) {
    throw;
  }
}

void unity_sframe::save_frame_by_index_file(std::string index_file) {
  log_func_entry();
  auto sf = get_underlying_sframe();
  sf->save(index_file);
}

void unity_sframe::save(oarchive& oarc) const {
  oarc << true;
  std::string prefix = oarc.get_prefix();
  const_cast<unity_sframe*>(this)->save_frame_by_index_file(prefix + ".frame_idx");
}

void unity_sframe::load(iarchive& iarc) {
  clear();
  bool has_sframe;
  iarc >> has_sframe;
  if (has_sframe) {
    sframe sf;
    iarc >> sf;
    construct_from_sframe(sf);
  }
}

void unity_sframe::clear() {
  m_planner_node.reset();
  m_column_names.clear();
  m_cached_sframe.reset();
}

size_t unity_sframe::size() {
  size_t ret = infer_planner_node_length(get_planner_node());
  if (ret == (size_t)(-1)) {
    return get_underlying_sframe()->size();
  }
  return ret;
}

size_t unity_sframe::num_columns() {
  return m_column_names.size();
}

size_t unity_sframe::column_index(const std::string &name) {
  Dlog_func_entry();

  auto it = std::find(m_column_names.begin(), m_column_names.end(), name);
  if(it == m_column_names.end()) { 
    log_and_throw(std::string("Column '") + name + "' not found.");;
  }
  return std::distance(m_column_names.begin(), it);
}

const std::string& unity_sframe::column_name(size_t index) {
  Dlog_func_entry();

  return m_column_names.at(index);
}


bool unity_sframe::contains_column(const std::string& name) {
  Dlog_func_entry();

  const auto& sf = this->get_underlying_sframe();
  return sf->contains_column(name);
}

std::shared_ptr<unity_sarray_base> unity_sframe::select_column(const std::string &name) {
  Dlog_func_entry();

  // Error checking
  logstream(LOG_DEBUG) << "Select Column " << name << std::endl;
  auto _column_names = this->column_names();
  auto _column_index_iter = std::find(_column_names.begin(), _column_names.end(), name);
  if (_column_index_iter == _column_names.end()) {
    log_and_throw (std::string("Column name " + name + " does not exist."));
  }

  // Construct the project operator with the column index
  size_t column_index = _column_index_iter - _column_names.begin();
  auto new_planner_node = op_project::make_planner_node(this->get_planner_node(), {column_index});

  std::shared_ptr<unity_sarray> ret(new unity_sarray());
  ret->construct_from_planner_node(new_planner_node);
  return ret;
}

std::shared_ptr<unity_sframe_base> unity_sframe::select_columns(
    const std::vector<std::string> &names) {
  Dlog_func_entry();

  // Error checking
  // Check if there is duplicate column names
  std::set<std::string> name_set(names.begin(), names.end());
  if (name_set.size() != names.size()) {
    log_and_throw("There are duplicate column names in the name list");
  }

  // Check if column names are valid
  auto this_column_names = this->column_names();
  auto this_column_types = this->dtype();
  std::vector<size_t> project_column_indices;
  for (const auto& name: names) {
    auto iter = std::find(this_column_names.begin(), this_column_names.end(), name);
    if (iter == this_column_names.end()) {
      log_and_throw("Column name " + name + " does not exist.");
    }
    project_column_indices.push_back(iter - this_column_names.begin());
  }

  if (names.empty()) {
    return std::make_shared<unity_sframe>();
  }

  // Construct the project operator with the column index
  auto new_planner_node = op_project::make_planner_node(this->get_planner_node(), {project_column_indices});
  std::vector<std::string> new_column_names;
  std::vector<flex_type_enum> new_column_types;
  for (auto& i : project_column_indices) {
    new_column_names.push_back(this_column_names[i]);
  }

  std::shared_ptr<unity_sframe> ret(new unity_sframe());
  ret->construct_from_planner_node(new_planner_node,
                                   new_column_names);
  return ret;
}

void unity_sframe::add_column(std::shared_ptr<unity_sarray_base> data,
                              const std::string& column_name) {
  Dlog_func_entry();

  // Sanity check
  ASSERT_TRUE(data != nullptr);

  // Auto generates column name for empty name input.
  std::string new_column_name = column_name;
  if (new_column_name.empty()) {
    new_column_name = generate_next_column_name();
  }

  auto colnames = this->column_names();
  if (std::find(colnames.begin(), colnames.end(), column_name) != colnames.end()) {
    log_and_throw("Column " + column_name + " already exists.");
  }

  // Base case:
  // If current sframe is empty, we construct a sarray source node
  std::shared_ptr<unity_sarray> new_column = std::static_pointer_cast<unity_sarray>(data);
  if (num_columns() == 0) {
    this->construct_from_planner_node(
      new_column->get_planner_node(),
      {new_column_name});
    return;
  }

  // Regular case:
  // Check that new column has the same size
  if (this->size() != new_column->size()) {
    log_and_throw(std::string("Column \"") + column_name +
                  "\" has different size than current columns!");
  }

  // Make a union operator node
  auto new_planner_node = op_union::make_planner_node(this->get_planner_node(),
                                                      new_column->get_planner_node());
  auto new_column_names = this->column_names();
  new_column_names.push_back(new_column_name);
  this->construct_from_planner_node(
      new_planner_node,
      new_column_names);
}

void unity_sframe::add_columns(
    std::list<std::shared_ptr<unity_sarray_base>> data_list,
    std::vector<std::string> name_vec) {
  Dlog_func_entry();
  std::vector<std::shared_ptr<unity_sframe_base>> ret_vec;
  std::vector<std::shared_ptr<unity_sarray_base>> data_vec(data_list.begin(), data_list.end());

  const std::string empty_str = std::string("");
  name_vec.resize(data_list.size(), empty_str);

  // Back up the planner node and column names
  auto backup_planner_node = std::make_shared<planner_node>(*(this->get_planner_node()));
  auto backup_column_names = this->column_names();

  for(size_t i = 0; i < data_vec.size(); ++i) {
    try {
      this->add_column(data_vec[i], name_vec[i]);
    } catch(...) {
      // rollback
      this->construct_from_planner_node(backup_planner_node,
                                        backup_column_names);
      throw;
    }
  }
  m_cached_sframe.reset();
}

void unity_sframe::set_column_name(size_t i, std::string name) {
  Dlog_func_entry();
  logstream(LOG_DEBUG) << "Args: " << i << "," << name << std::endl;
  if (i >= num_columns()) {
    log_and_throw("Column index out of bound.");
  }
  std::vector<std::string> colnames = column_names();
  for (size_t j = 0; j < num_columns(); ++j) {
    if (j != i && colnames[j] == name) {
      log_and_throw(std::string("Column name " + name + " already exists"));
    }
  }
  m_column_names[i] = name;
  m_cached_sframe.reset();
}

void unity_sframe::remove_column(size_t i) {
  Dlog_func_entry();
  logstream(LOG_INFO) << "Args: " << i << std::endl;
  if(i >= num_columns()) {
    log_and_throw("Column index out of bound.");
  }

  std::vector<size_t> project_column_indices;
  for (size_t j = 0; j < num_columns(); ++j) {
    if (j == i) continue;
    project_column_indices.push_back(j);
  }

  if (project_column_indices.empty()) {
    // make empty sframe
    auto sf = std::make_shared<sframe>();
    sf->open_for_write({}, {}, "", 1);
    sf->close();
    this->set_sframe(sf);
  } else {
    auto new_planner_node = op_project::make_planner_node(
        this->get_planner_node(), project_column_indices);
    auto new_column_names = this->column_names();
    new_column_names.erase(new_column_names.begin() + i);
    auto new_column_types = this->dtype();
    new_column_types.erase(new_column_types.begin() + i);
    this->construct_from_planner_node(
        new_planner_node,
        new_column_names);
  }
}

void unity_sframe::swap_columns(size_t i, size_t j) {
  Dlog_func_entry();
  logstream(LOG_DEBUG) << "Args: " << i << ", " << j << std::endl;
  if(i >= num_columns()) {
    log_and_throw("Column index value of " + std::to_string(i) + " is out of bound.");
  }
  if(j >= num_columns()) {
    log_and_throw("Column index value of " + std::to_string(j) + " is out of bound.");
  }

  std::vector<std::string> new_column_names = column_names();
  std::vector<size_t> new_column_indices(num_columns());
  for (size_t idx = 0; idx < num_columns(); ++idx) {
    new_column_indices[idx] = idx;

  }
  std::swap(new_column_indices[i], new_column_indices[j]);
  std::swap(new_column_names[i], new_column_names[j]);

  auto new_planner_node = op_project::make_planner_node(this->get_planner_node(),
                                                        new_column_indices);
  this->construct_from_planner_node(new_planner_node, new_column_names);
}

std::shared_ptr<sframe> unity_sframe::get_underlying_sframe() {
  Dlog_func_entry();

  if (!m_cached_sframe) {
    if (!is_materialized()) {
      materialize();
    }
    m_cached_sframe = std::make_shared<sframe>(
        planner().materialize(this->get_planner_node()));

    // make sure the physical sframe has consistant column names
    for (size_t i = 0; i < num_columns(); ++i) {
      m_cached_sframe->set_column_name(i, m_column_names[i]);
    }
  }

  return m_cached_sframe;
}

void unity_sframe::set_sframe(const std::shared_ptr<sframe>& sf_ptr) {
  Dlog_func_entry();
  m_planner_node = op_sframe_source::make_planner_node(*sf_ptr);
  m_column_names = sf_ptr->column_names();
  m_cached_sframe = sf_ptr;
}


std::shared_ptr<unity_sarray_base> unity_sframe::transform(const std::string& lambda,
                                           flex_type_enum type,
                                           bool skip_undefined, // unused
                                           int random_seed) {
  log_func_entry();
#ifdef TC_HAS_PYTHON
  auto new_planner_node = op_lambda_transform::make_planner_node(
      this->get_planner_node(), lambda, type,
      this->column_names(),
      skip_undefined, random_seed);

  std::shared_ptr<unity_sarray> ret(new unity_sarray());
  ret->construct_from_planner_node(new_planner_node);
  return ret;
#else
  log_and_throw("Python functions not supported");
#endif
}

std::shared_ptr<unity_sarray_base> unity_sframe::transform_native(const function_closure_info& toolkit_fn_name,
                                           flex_type_enum type,
                                           bool skip_undefined, // unused
                                           int seed) {
  log_func_entry();

  //  find the function
  auto native_execute_function =
      get_unity_global_singleton()
      ->get_toolkit_function_registry()
      ->get_native_function(toolkit_fn_name);
  std::vector<std::string> colnames = column_names();

  auto lambda =
      [native_execute_function, colnames](
          const sframe_rows::row& row)->flexible_type {
        std::vector<std::pair<flexible_type, flexible_type> > input(colnames.size());
        ASSERT_EQ(row.size(), colnames.size());
        for (size_t i = 0;i < colnames.size(); ++i) {
          input[i] = {colnames[i], row[i]};
        }
        variant_type var = to_variant(input);
        return variant_get_value<flexible_type>(native_execute_function({var}));
      };
  return this->transform_lambda(lambda, type, seed);
}

std::shared_ptr<unity_sarray_base> unity_sframe::transform_lambda(
      std::function<flexible_type(const sframe_rows::row&)> lambda,
      flex_type_enum type,
      int random_seed) {
  log_func_entry();

  auto new_planner_node = op_transform::make_planner_node(this->get_planner_node(),
                                                          lambda,
                                                          type,
                                                          random_seed);
  std::shared_ptr<unity_sarray> ret(new unity_sarray());
  ret->construct_from_planner_node(new_planner_node);
  return ret;
}

std::shared_ptr<unity_sframe_base> unity_sframe::flat_map(
    const std::string& lambda,
    std::vector<std::string> column_names,
    std::vector<flex_type_enum> column_types,
    bool skip_undefined,
    int seed) {
#ifdef TC_HAS_PYTHON
  log_func_entry();
  DASSERT_EQ(column_names.size(), column_types.size());
  DASSERT_TRUE(!column_names.empty());
  DASSERT_TRUE(!column_types.empty());

  sframe out_sf;
  out_sf.open_for_write(column_names, column_types, "", SFRAME_DEFAULT_NUM_SEGMENTS);

  lambda::pylambda_function pylambda_fn(lambda);
  pylambda_fn.set_skip_undefined(skip_undefined);
  pylambda_fn.set_random_seed(seed);
  auto this_column_names = this->column_names();

  auto transform_callback = [&](size_t segment_id, const std::shared_ptr<sframe_rows>& data) {
    auto output_iter = out_sf.get_output_iterator(segment_id);
    std::vector<flexible_type> lambda_output_rows;
    pylambda_fn.eval(this_column_names, *data, lambda_output_rows);
    for (flexible_type& result: lambda_output_rows) {
      if (result.get_type() == flex_type_enum::UNDEFINED) {
        continue;
      } else if (result.get_type() == flex_type_enum::LIST) {
        flex_list& out_rows = result.mutable_get<flex_list>();
        for (auto& out_row: out_rows) {
          *output_iter++ = std::move(out_row);
        }
      } else if (result.get_type() == flex_type_enum::VECTOR) {
        if (result.get<flex_vec>().size() > 0) {
          std::string message = "Cannot convert " + std::string(result) +
            " to " + flex_type_enum_to_name(flex_type_enum::LIST);
          logstream(LOG_ERROR) <<  message << std::endl;
          throw(bad_cast(message));

        }
      } else {
        std::string message = "Cannot convert " + std::string(result) +
          " to " + flex_type_enum_to_name(flex_type_enum::LIST);
        logstream(LOG_ERROR) <<  message << std::endl;
        throw(bad_cast(message));
      }
    }
    return false;
  };
  query_eval::planner().materialize(this->get_planner_node(), transform_callback, SFRAME_DEFAULT_NUM_SEGMENTS);
  out_sf.close();
  auto ret = std::make_shared<unity_sframe>();
  ret->construct_from_sframe(out_sf);
  return ret;
#else
  log_and_throw("Python lambda functions not supported");
#endif
}


std::vector<flex_type_enum> unity_sframe::dtype() {
  Dlog_func_entry();
  return infer_planner_node_type(this->get_planner_node());
}


std::vector<std::string> unity_sframe::column_names() {
  Dlog_func_entry();
  return m_column_names;
}



std::shared_ptr<unity_sframe_base> unity_sframe::head(size_t nrows) {
  log_func_entry();

  // prepare for writing to the new sframe
  sframe sf_head;
  sf_head.open_for_write(column_names(), dtype(), "", 1);
  auto out = sf_head.get_output_iterator(0);

  size_t row_counter = 0;
  if (nrows > 0)  {
    auto callback = [&out, &row_counter, nrows](size_t segment_id,
                                                const std::shared_ptr<sframe_rows>& data) {
      for (const auto& row : (*data)) {
        *out = row;
        ++out;
        ++row_counter;
        if (row_counter == nrows) return true;
      }
      return false;
    };

    query_eval::planner().materialize(this->get_planner_node(),
                                      callback,
                                      1 /* process in as 1 segment */);
  }
  sf_head.close();
  std::shared_ptr<unity_sframe> ret(new unity_sframe());
  ret->construct_from_sframe(sf_head);
  return ret;
}


dataframe_t unity_sframe::_head(size_t nrows) {
  auto result = head(nrows);
  dataframe_t ret = result->to_dataframe();
  return ret;
};

dataframe_t unity_sframe::_tail(size_t nrows) {
  auto result = tail(nrows);
  dataframe_t ret = result->to_dataframe();
  return ret;
};

std::shared_ptr<unity_sframe_base> unity_sframe::tail(size_t nrows) {
  log_func_entry();
  logstream(LOG_INFO) << "Args: " << nrows << std::endl;
  size_t end = size();
  nrows = std::min<size_t>(nrows, end);
  size_t start = end - nrows;
  return copy_range(start, 1, end);
}

std::list<std::shared_ptr<unity_sframe_base>> unity_sframe::logical_filter_split(
  std::shared_ptr<unity_sarray_base> logical_filter_array) {
  return {logical_filter(logical_filter_array),
          logical_filter(logical_filter_array->right_scalar_operator(1, "-"))};
}

std::shared_ptr<unity_sframe_base> unity_sframe::logical_filter(
    std::shared_ptr<unity_sarray_base> index) {
  log_func_entry();

  ASSERT_TRUE(index != nullptr);

  std::shared_ptr<unity_sarray> filter_array = std::static_pointer_cast<unity_sarray>(index);

  std::shared_ptr<unity_sarray> other_array_binarized =
      std::static_pointer_cast<unity_sarray>(
      filter_array->transform_lambda(
            [](const flexible_type& f)->flexible_type {
              return (flex_int)(!f.is_zero());
            }, flex_type_enum::INTEGER, true, 0));


  auto equal_length = query_eval::planner().test_equal_length(this->get_planner_node(),
                                                              other_array_binarized->get_planner_node());

  if (!equal_length) {
    log_and_throw("Logical filter array must have the same size");
  }


  auto new_planner_node = op_logical_filter::make_planner_node(this->get_planner_node(),
                                                               other_array_binarized->get_planner_node());

  std::shared_ptr<unity_sframe> ret_unity_sframe(new unity_sframe());
  ret_unity_sframe->construct_from_planner_node(new_planner_node,
                                                this->column_names());
  return ret_unity_sframe;
}

std::shared_ptr<unity_sframe_base> unity_sframe::append(
    std::shared_ptr<unity_sframe_base> other) {
  log_func_entry();

  DASSERT_TRUE(other != nullptr);
  std::shared_ptr<unity_sframe> other_sframe = std::static_pointer_cast<unity_sframe>(other);

  // zero columns
  if (this->num_columns() == 0) {
    return other;
  } else if (other_sframe->num_columns() == 0) {
    auto ret = std::make_shared<unity_sframe>();
    auto new_planner_node = std::make_shared<planner_node>(*(this->get_planner_node()));
    ret->construct_from_planner_node(new_planner_node, this->column_names());
    return ret;
  }

  // Error checking
  {
    if (this->num_columns() != other_sframe->num_columns()) {
      log_and_throw("Two SFrames have different number of columns");
    }
    std::vector<std::string> column_names = this->column_names();
    std::vector<std::string> other_column_names = other_sframe->column_names();

    size_t num_columns = column_names.size();

    if(column_names != other_column_names) {
      std::sort(column_names.begin(), column_names.end());
      std::sort(other_column_names.begin(), other_column_names.end());

      if(column_names != other_column_names) {

      std::vector<std::string> in_this;

      std::set_difference(column_names.begin(), column_names.end(),
                          other_column_names.begin(), other_column_names.end(),
                          std::inserter(in_this, in_this.begin()));

        std::ostringstream ss;
        ss << "Error: Columns [" << in_this
           << "] not found in appending SFrame.";

        log_and_throw(ss.str().c_str());
      }
    }

    auto column_types = this->dtype();
    auto other_column_types = other_sframe->dtype();

    for(size_t i = 0; i < num_columns; i++) {

      // check column type matches
      if (column_types[i] != other_column_types[i]) {
        std::ostringstream ss;
        ss << "Column types are not the same in two SFrames (Column "
           << column_names[i] << ", attempting to append column of type "
           << flex_type_enum_to_name(other_column_types[i])
           << " to column of type " << flex_type_enum_to_name(column_types[i])
           << ").";

        log_and_throw(ss.str().c_str());
      }
    }
  }

  auto new_planner_node = op_append::make_planner_node(this->get_planner_node(),
                                                       other_sframe->get_planner_node());
  std::shared_ptr<unity_sframe> ret_unity_sframe(new unity_sframe());
  ret_unity_sframe->construct_from_planner_node(new_planner_node,
                                                this->column_names());
  return ret_unity_sframe;
}

void unity_sframe::begin_iterator() {
  log_func_entry();

  // Empty sframe just return
  if (this->size() == 0)
    return;

  auto sframe_ptr = get_underlying_sframe();
  iterator_sframe_ptr = sframe_ptr->get_reader();
  // init the iterators
  iterator_current_segment_iter.reset(new sframe_iterator(iterator_sframe_ptr->begin(0)));
  iterator_current_segment_enditer.reset(new sframe_iterator(iterator_sframe_ptr->end(0)));
  iterator_next_segment_id = 1;
}

std::vector< std::vector<flexible_type> > unity_sframe::iterator_get_next(size_t len) {
  std::vector< std::vector<flexible_type> > ret;

  // Empty sframe just return
  if (this->size() == 0)
    return ret;

  // try to extract len elements
  ret.reserve(len);
  // loop across segments
  while(1) {
    // loop through current segment
    while(*iterator_current_segment_iter != *iterator_current_segment_enditer) {
      ret.push_back(**iterator_current_segment_iter);
      ++(*iterator_current_segment_iter);
      if (ret.size() >= len) break;
    }
    if (ret.size() >= len) break;
    // if we run out of data in the current segment, advance to the next segment
    // if we run out of segments, quit.
    if (iterator_next_segment_id >= iterator_sframe_ptr->num_segments()) break;
    iterator_current_segment_iter.reset(new sframe_iterator(
        iterator_sframe_ptr->begin(iterator_next_segment_id)));
    iterator_current_segment_enditer.reset(new sframe_iterator(
        iterator_sframe_ptr->end(iterator_next_segment_id)));
    ++iterator_next_segment_id;
  }
  return ret;
}

void unity_sframe::save_as_csv(const std::string& url,
                               std::map<std::string, flexible_type> writing_config) {
  log_func_entry();
  logstream(LOG_INFO) << "Args: " << sanitize_url(url) << std::endl;

  csv_writer writer;
  // first the defaults
  writer.delimiter = ",";
  writer.escape_char = '\\';
  writer.use_escape_char = true;
  writer.double_quote = true;
  writer.quote_char = '\"';
  writer.quote_level = csv_writer::csv_quote_level::QUOTE_NONNUMERIC;
  writer.header = true;
  writer.na_value = "";
  std::string file_header;
  std::string file_footer;
  std::string line_prefix;
  bool no_prefix_on_first_value = false;


  if (writing_config["delimiter"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string) writing_config["delimiter"];
    if(tmp.length() > 0) writer.delimiter = tmp;
  }
  if (writing_config["escape_char"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string)writing_config["escape_char"];
    if (tmp.length() > 0) writer.escape_char = tmp[0];
    else writer.use_escape_char = false;
  }
  if (writing_config.count("double_quote")) {
    writer.double_quote = !writing_config["double_quote"].is_zero();
  }
  if (writing_config["quote_char"].get_type() == flex_type_enum::STRING) {
    std::string tmp = (flex_string)writing_config["quote_char"];
    if (tmp.length() > 0) writer.quote_char = tmp[0];
  }
  if (writing_config.count("quote_level")) {
    auto quote_level = writing_config["quote_level"];
    if (quote_level == 0) {
      writer.quote_level = csv_writer::csv_quote_level::QUOTE_MINIMAL;
    } else if (quote_level == 1) {
      writer.quote_level = csv_writer::csv_quote_level::QUOTE_ALL;
    } else if (quote_level == 2) {
      writer.quote_level = csv_writer::csv_quote_level::QUOTE_NONNUMERIC;
    } else if (quote_level == 3) {
      writer.quote_level = csv_writer::csv_quote_level::QUOTE_NONE;
    } else {
      log_and_throw("Invalid quote level");
    }
  }
  if (writing_config.count("header")) {
    writer.header= !writing_config["header"].is_zero();
  }

  if (writing_config.count("line_terminator")) {
    std::string tmp = (flex_string) writing_config["line_terminator"];
    if(tmp.length() > 0) writer.line_terminator = tmp;
  }

  if (writing_config.count("na_value")) {
    std::string tmp = (flex_string) writing_config["na_value"];
    if(tmp.length() > 0) writer.na_value = tmp;
  }

  if (writing_config.count("file_header")) {
    file_header = (flex_string) writing_config["file_header"];
  }
  if (writing_config.count("file_footer")) {
    file_footer = (flex_string) writing_config["file_footer"];
  }
  if (writing_config.count("line_prefix")) {
    line_prefix = (flex_string) writing_config["line_prefix"];
  }
  if (writing_config.count("_no_prefix_on_first_value")) {
    no_prefix_on_first_value = !writing_config["_no_prefix_on_first_value"].is_zero();
  }

  general_ofstream fout(url);
  if (!file_header.empty()) fout << file_header << writer.line_terminator;
  if (!fout.good()) {
    log_and_throw(std::string("Unable to open " + sanitize_url(url) + " for write"));
  }

  // write the header
  size_t num_cols = this->num_columns();
  if (num_cols == 0) return;

  if (writer.header) writer.write_verbatim(fout, this->column_names());

  bool first_value = true;
  auto write_callback = [&writer, &fout, &line_prefix,
       &no_prefix_on_first_value, &first_value]
      (size_t segment_id, const std::shared_ptr<sframe_rows>& data) {
    for (const auto& row : *(data)) {
      if (!line_prefix.empty()) {
        if ((!first_value) || // not the first value. write the line prefix
            (!no_prefix_on_first_value) // first value, write line prefix if
                                        // no_prefix_on_first_value == false
                                        // (yes the double negative is annoying)
            ) {
          fout.write(line_prefix.c_str(), line_prefix.size());
        }
      }
      first_value = false;
      writer.write(fout, row);
    }
    return false;
  };

  query_eval::planner().materialize(this->get_planner_node(), write_callback, 1);
  if (!fout.good()) {
    log_and_throw_io_failure("Fail to write.");
  }
  if (!file_footer.empty()) fout << file_footer << writer.line_terminator;
  fout.close();
}

std::shared_ptr<unity_sframe_base> unity_sframe::sample(float percent,
                                                        int random_seed,
                                                        bool exact) {
  logstream(LOG_INFO) << "Args: " << percent << ", " << random_seed << std::endl;
  auto logical_filter_array = std::static_pointer_cast<unity_sarray>(
    unity_sarray::make_uniform_boolean_array(size(), percent, random_seed, exact));
  return logical_filter(logical_filter_array);
}

void unity_sframe::materialize() {
  query_eval::planner().materialize(m_planner_node);
}


bool unity_sframe::is_materialized() {
  auto optimized_node = optimization_engine::optimize_planner_graph(get_planner_node(),
                                                                    materialize_options());
  if (is_source_node(optimized_node)) {
    m_planner_node = optimized_node;
    return true;
  }
  return false;
}

bool unity_sframe::has_size() {
  return infer_planner_node_length(m_planner_node) != -1;
}

std::string unity_sframe::query_plan_string() {
  std::stringstream ss;
  ss << get_planner_node() << std::endl;
  return ss.str();
}

std::list<std::shared_ptr<unity_sframe_base>>
unity_sframe::random_split(float percent, int random_seed, bool exact) {
  log_func_entry();
  logstream(LOG_INFO) << "Args: " << percent << ", " << random_seed << std::endl;

  auto logical_filter_array = std::static_pointer_cast<unity_sarray>(
    unity_sarray::make_uniform_boolean_array(size(), percent, random_seed, exact));
  return logical_filter_split(logical_filter_array);
}

std::shared_ptr<unity_sframe_base> unity_sframe::groupby_aggregate(
    const std::vector<std::string>& key_columns,
    const std::vector<std::vector<std::string>>& group_columns,
    const std::vector<std::string>& group_output_columns,
    const std::vector<std::string>& group_operations) {

  std::vector<std::shared_ptr<group_aggregate_value>> operators;
  for (const auto& op: group_operations) operators.push_back(get_builtin_group_aggregator(op));
  return groupby_aggregate(key_columns, group_columns, group_output_columns, operators);
}

std::shared_ptr<unity_sframe_base> unity_sframe::groupby_aggregate(
    const std::vector<std::string>& key_columns,
    const std::vector<std::vector<std::string>>& group_columns,
    const std::vector<std::string>& group_output_columns,
    const std::vector<std::shared_ptr<group_aggregate_value>>& group_operations) {
  log_func_entry();

  // logging stuff
  {
    logstream(LOG_INFO) << "Args: Keys: ";
    for (auto i: key_columns) logstream(LOG_INFO) << i << ",";
    logstream(LOG_INFO) << "\tGroups: ";
    for (auto cols: group_columns) {
      for(auto col: cols) {
        logstream(LOG_INFO) << col << ",";
      }
      logstream(LOG_INFO) << " | ";
    }
    logstream(LOG_INFO) << "\tOperations: ";
    for (auto i: group_operations) logstream(LOG_INFO) << i << ",";
    logstream(LOG_INFO) << std::endl;
  }

  // Prepare the operators
  ASSERT_EQ(group_columns.size(), group_operations.size());
  std::vector<std::pair<std::vector<std::string>,
      std::shared_ptr<group_aggregate_value> > > operators;
  for (size_t i = 0;i < group_columns.size(); ++i) {
    // avoid copying empty column string
    // this is the case for aggregate::COUNT()
    std::vector<std::string> column_names;
    for (const auto& col : group_columns[i]) {
      if (!col.empty()) column_names.push_back(col);
    }
    operators.push_back( {column_names, group_operations[i]} );
  }

  auto grouped_sf = query_eval::groupby_aggregate(get_planner_node(),
                                                  column_names(),
                                                  key_columns,
                                                  group_output_columns,
                                                  operators);

  std::shared_ptr<unity_sframe> ret(new unity_sframe());
  ret->construct_from_sframe(*grouped_sf);
  return ret;
}


std::shared_ptr<unity_sframe_base> unity_sframe::join(
    std::shared_ptr<unity_sframe_base> right,
    const std::string join_type,
    std::map<std::string,std::string> join_keys) {
  log_func_entry();
  std::shared_ptr<unity_sframe> ret(new unity_sframe());
  std::shared_ptr<unity_sframe> us_right = std::static_pointer_cast<unity_sframe>(right);

  auto sframe_ptr = get_underlying_sframe();
  auto right_sframe_ptr = us_right->get_underlying_sframe();
  sframe joined_sf = turi::join(*sframe_ptr,
                                    *right_sframe_ptr,
                                    join_type,
                                    join_keys);
  ret->construct_from_sframe(joined_sf);
  return ret;
}

std::shared_ptr<unity_sframe_base>
unity_sframe::sort(const std::vector<std::string>& sort_keys,
                   const std::vector<int>& sort_ascending) {
  log_func_entry();

  if (sort_keys.size() != sort_ascending.size()) {
    log_and_throw("sframe::sort key vector and ascending vector size mismatch");
  }

  if (sort_keys.size() == 0) {
    log_and_throw("sframe::sort, nothing to sort");
  }

  std::vector<size_t> sort_indices = _convert_column_names_to_indices(sort_keys);
  std::vector<bool> b_sort_ascending;
  for(auto sort_order: sort_ascending) {
    b_sort_ascending.push_back((bool)sort_order);
  }

  auto sorted_sf = turi::ec_sort(this->get_planner_node(),
                                     this->column_names(),
                                     sort_indices,
                                     b_sort_ascending);
  std::shared_ptr<unity_sframe> ret(new unity_sframe());
  ret->construct_from_sframe(*sorted_sf);
  return ret;
}

std::shared_ptr<unity_sarray_base> unity_sframe::pack_columns(
    const std::vector<std::string>& pack_column_names,
    const std::vector<std::string>& key_names,
    flex_type_enum dtype,
    const flexible_type& fill_na) {

  log_func_entry();

  // Error checking
  if (pack_column_names.size() == 0) {
    throw "There is no column to pack";
  }

  if (dtype != flex_type_enum::DICT &&
    dtype != flex_type_enum::LIST &&
    dtype != flex_type_enum::VECTOR) {
    log_and_throw("Resulting sarray dtype should be list/array/dict type");
  }

  std::set<flexible_type> pack_column_set(pack_column_names.begin(), pack_column_names.end());
  if (pack_column_set.size() != pack_column_names.size()) {
    throw "There are duplicate names in packed columns";
  }

  // select packing columns
  auto projected_sf = std::static_pointer_cast<unity_sframe>(this->select_columns(pack_column_names));

  auto dict_transform_callback = [=](const sframe_rows::row& row)->flexible_type{
    flex_dict out_val;
    out_val.reserve(row.size());
    for (size_t col = 0; col < row.size(); col++) {
      if (row[col] != FLEX_UNDEFINED) {
        out_val.push_back(std::make_pair(key_names[col], row[col]));
      } else {
        if (fill_na.get_type() != flex_type_enum::UNDEFINED) {
          out_val.push_back(std::make_pair(key_names[col], fill_na));
        }
      }
    }
    return out_val;
  };

  auto list_transform_callback = [=](const sframe_rows::row& row)->flexible_type {
    flex_list out_val(row.size());
    for (size_t col = 0; col < row.size(); col++) {
      if (row[col] != FLEX_UNDEFINED) {
        out_val[col] = row[col];
      } else {
        out_val[col] = fill_na;
      }
    }
    return out_val;
  };

  auto vector_transform_callback = [=](const sframe_rows::row& row)->flexible_type {
    flex_vec out_val(row.size());
    for (size_t col = 0; col < row.size(); col++) {
      if (!row[col].is_na()) {
        out_val[col] = row[col];
      } else {
        if (fill_na == FLEX_UNDEFINED) {
          out_val[col] = NAN;
        } else {
          out_val[col] = (double)fill_na;
        }
      }
    }
    return out_val;
  };

  std::shared_ptr<unity_sarray> ret(new unity_sarray());
  if (dtype == flex_type_enum::DICT) {
    auto new_planner_node = op_transform::make_planner_node(projected_sf->get_planner_node(),
                                                            dict_transform_callback,
                                                            dtype);
    ret->construct_from_planner_node(new_planner_node);
  } else if (dtype == flex_type_enum::LIST) {
    auto new_planner_node = op_transform::make_planner_node(projected_sf->get_planner_node(),
                                                            list_transform_callback,
                                                            dtype);
    ret->construct_from_planner_node(new_planner_node);
  } else {
    auto new_planner_node = op_transform::make_planner_node(projected_sf->get_planner_node(),
                                                            vector_transform_callback,
                                                            dtype);
    ret->construct_from_planner_node(new_planner_node);
  }
  return ret;
}

std::shared_ptr<unity_sframe_base> unity_sframe::stack(
    const std::string& stack_column_name,
    const std::vector<std::string>& new_column_names,
    const std::vector<flex_type_enum>& new_column_types,
    bool drop_na) {

  log_func_entry();

  // check validity of column names
  auto all_column_names = this->column_names();
  auto all_column_types = this->dtype();
  std::set<std::string> my_columns(all_column_names.begin(), all_column_names.end());
  bool stack_column_exists = false;
  for(auto name : new_column_names) {
    if (my_columns.count(name) && name != stack_column_name) {
      throw "Column name '" + name + "' is already used by current SFrame, pick a new column name";
    }
    if (my_columns.count(stack_column_name) > 0) {
      stack_column_exists = true;
    }
  }
  if (!stack_column_exists) {
    log_and_throw("Cannot find stack column " + stack_column_name);
  }

  // validate column types
  size_t new_column_count = 0;
  flex_type_enum stack_column_type = this->select_column(stack_column_name)->dtype();
  if (stack_column_type == flex_type_enum::DICT) {
    new_column_count = 2;
  } else if (stack_column_type == flex_type_enum::VECTOR) {
    new_column_count = 1;
  } else if (stack_column_type == flex_type_enum::LIST) {
    new_column_count = 1;
  } else {
    throw "Column type is not supported for stack";
  }

  if (new_column_types.size() != new_column_count) {
    throw "column types given is not matching the expected number";
  }
  if (new_column_names.size() != new_column_count) {
    throw "column names given is not matching the expected number";
  }
  // check uniqueness of output column name if given
  if (new_column_names.size() == 2 &&
     new_column_names[0] == new_column_names[1] &&
     !new_column_names[0].empty()) {
      throw "There is duplicate column names in new_column_names parameter";
  }

  // create return SFrame
  size_t num_columns = this->num_columns();
  std::vector<std::string> ret_column_names;
  std::vector<flex_type_enum> ret_column_types;
  ret_column_names.reserve(num_columns + new_column_count - 1);
  ret_column_types.reserve(num_columns + new_column_count - 1);

  for(size_t i = 0; i < num_columns; ++i) {
    if (all_column_names[i] != stack_column_name) {
      ret_column_names.push_back(all_column_names[i]);
      ret_column_types.push_back(all_column_types[i]);
    }
  }

  ret_column_names.insert(ret_column_names.end(),new_column_names.begin(), new_column_names.end());
  ret_column_types.insert(ret_column_types.end(),new_column_types.begin(), new_column_types.end());

  auto sframe_ptr = std::make_shared<sframe>();
  sframe_ptr->open_for_write(ret_column_names, ret_column_types,
                             "", SFRAME_DEFAULT_NUM_SEGMENTS);
  size_t stack_col_idx = _convert_column_names_to_indices({stack_column_name})[0];

  auto transform_callback = [&](size_t segment_id,
                                const std::shared_ptr<sframe_rows>& data) {

    auto output_iter = sframe_ptr->get_output_iterator(segment_id);
    std::vector<flexible_type> out_row_buffer(num_columns + new_column_count - 1);

    for (const auto& row: (*data)) {
      const flexible_type& val = row[stack_col_idx];
      if (val.get_type() == flex_type_enum::UNDEFINED || val.size() == 0) {
        if (!drop_na) {
          if (stack_column_type == flex_type_enum::DICT) {
            out_row_buffer[num_columns - 1] = FLEX_UNDEFINED;
            out_row_buffer[num_columns] = FLEX_UNDEFINED;
          } else {
            out_row_buffer[num_columns - 1] = FLEX_UNDEFINED;
          }
          // copy the rest columns
          for(size_t i = 0, j = 0; i < num_columns; i++) {
            if (i != stack_col_idx) {
              out_row_buffer[j++] = row[i];
            }
          }
          // write to out sframe
          *output_iter++ = out_row_buffer;
        }
      } else {
        for(size_t row_idx = 0; row_idx < val.size(); row_idx++) {
          if (stack_column_type == flex_type_enum::DICT) {
            const flex_dict& dict_val = val.get<flex_dict>();
            out_row_buffer[num_columns - 1] = dict_val[row_idx].first;
            out_row_buffer[num_columns] = dict_val[row_idx].second;
          } else if (stack_column_type == flex_type_enum::LIST) {
            out_row_buffer[num_columns - 1] = val.array_at(row_idx);
          } else {
            out_row_buffer[num_columns - 1] = val[row_idx];
          }
          // copy the rest columns
          for(size_t i = 0, j = 0; i < num_columns; i++) {
            if (i != stack_col_idx) {
              out_row_buffer[j++] = row[i];
            }
          }
          // write to out sframe
          *output_iter++ = out_row_buffer;
        }
      }
    }
    return false;
  };

  // turi::multi_transform(m_lazy_sframe, *sframe_ptr, transform_fn);
  query_eval::planner().materialize(this->get_planner_node(),
                                    transform_callback, SFRAME_DEFAULT_NUM_SEGMENTS);
  sframe_ptr->close();

  auto ret = std::make_shared<unity_sframe>();
  ret->construct_from_sframe(*sframe_ptr);
  return ret;
}

std::shared_ptr<unity_sframe_base>
unity_sframe::copy_range(size_t start, size_t step, size_t end) {
  log_func_entry();
  if (step == 0) log_and_throw("Range step size must be at least 1");
  // end cannot be past the end
  end = std::min(end, size());

  std::shared_ptr<unity_sframe> ret(new unity_sframe());

  // Fast path: range slice with step 1, we can slice the input using the query planner.
  if ((start < end) && (step == 1)) {
    auto current_node = this->get_planner_node();
    auto sliced_node = query_eval::planner().slice(current_node, start, end);
    // slice may partially materialize the node. Save it to avoid repeated materialization
    m_planner_node = current_node;
    ret->construct_from_planner_node(sliced_node, this->column_names());
    return ret;
  }

  sframe writer;
  writer.open_for_write(column_names(),
                        dtype(),
                        std::string(""), 1);
  if (start < end) {
    // If the range begins from the start, we do a lazy read.
    // Otherwise, we will materialize the sframe.
    //
    // This is quite an annoying heuristic.
    // We should also be able to do the lazy callback way
    // which carefully slices the inputs to get the right values.
    // This avoids the annoying sequential read. Ponder.
    if (is_materialized() || start > 0) {
      auto sframe_ptr = this->get_underlying_sframe();
      turi::copy_range(*sframe_ptr, writer, start, step, end);
    } else {
      size_t current_row = 0;
      auto out = writer.get_output_iterator(0);
      auto callback = [&current_row, &out, start, step, end](size_t segment_id,
                                                             const std::shared_ptr<sframe_rows>& data) {
        for (auto& row: (*data)) {
          if (current_row >= end) return true;
          if (current_row < start || (current_row - start) % step != 0) {
            ++current_row;
            continue;
          }
          *out++ = row;
          ++current_row;
        }
        return false;
      };
      query_eval::planner().materialize(this->get_planner_node(), callback, 1);
    }
  } // else we return an empty sframe.
  writer.close();
  ret->construct_from_sframe(writer);
  return ret;
}

std::list<std::shared_ptr<unity_sframe_base>> unity_sframe::drop_missing_values(
    const std::vector<std::string> &column_names, bool all, bool split) {
  log_func_entry();

  // Error checking
  if (column_names.size() > this->num_columns()) {
    log_and_throw("Too many column names given.");
  }

  // Filter function
  std::vector<size_t> column_indices = _convert_column_names_to_indices(column_names);

  std::function<flexible_type(const sframe_rows::row&)> filter_fn;
  if (all) {
    filter_fn = [column_indices](const sframe_rows::row& row)->flexible_type {
                                  size_t num_missing_values = 0;
                                  for(const auto &i : column_indices) {
                                    if(row[i].is_na()) {
                                      ++num_missing_values;
                                    }
                                  }
                                  return (num_missing_values != column_indices.size());
                                };
  } else {
    filter_fn = [column_indices](const sframe_rows::row& row)->flexible_type {
                                  for(const auto &i : column_indices) {
                                    if (row[i].is_na())
                                      return false;
                                  }
                                  return true;
                                };
  }
  auto filter_sarray = std::static_pointer_cast<unity_sarray>(
    transform_lambda(filter_fn, flex_type_enum::INTEGER, 0));

  auto ret = std::list<std::shared_ptr<unity_sframe_base>>();
  if (split) {
    return logical_filter_split(filter_sarray);
  } else {
    return {logical_filter(filter_sarray), std::make_shared<unity_sframe>()};
  }
}

dataframe_t unity_sframe::to_dataframe() {
  dataframe_t ret;
  for (size_t i = 0; i < num_columns(); ++i) {
    auto name = column_names()[i];
    auto type = dtype()[i];
    ret.names.push_back(name);
    ret.types[name] = type;
    ret.values[name] = select_column(name)->to_vector();
  }
  return ret;
}

/**
 * Convert column names to column indices.
 *
 * If input column_names is empty, return 0,1,2,...num_columns-1
 *
 * Throw if column_names has duplication, or some column name does not exist.
 */
std::vector<size_t> unity_sframe::_convert_column_names_to_indices(
    const std::vector<std::string> &column_names) {

  std::unordered_set<size_t> dedup_set;
  std::vector<size_t> column_indices;

  auto this_column_names = this->column_names();
  if(column_names.size()) {
    for(auto &i : column_names) {
      // Fine if this throws, it will just be propagated back with a fine message
      auto iter = std::find(this_column_names.begin(), this_column_names.end(), i);
      if (iter == this_column_names.end()) {
        log_and_throw(std::string("Column ") + i + " does not exist");
      };
      size_t index_to_add = iter - this_column_names.begin();
      if (dedup_set.count(index_to_add)) {
        log_and_throw(std::string("Duplicate column names: ") + i);
      }
      dedup_set.insert(index_to_add);
      column_indices.push_back(index_to_add);
    }
  } else {
    // Add all columns
    for(size_t i = 0; i < num_columns(); ++i) {
      column_indices.push_back(i);
    }
  }
  return column_indices;
}

void unity_sframe::delete_on_close() {
  if (is_materialized()) {
    get_underlying_sframe()->delete_files_on_destruction();
  }
}

std::shared_ptr<planner_node> unity_sframe::get_planner_node() {
  return m_planner_node;
}

/**
 * Generate a new column name given existing column names.
 * New column name is in the form of X.1
 */
std::string unity_sframe::generate_next_column_name() {
  const auto& current_column_names = this->column_names();
  std::string name = std::string("X") + std::to_string(current_column_names.size() + 1);
  std::unordered_set<std::string> current_name_set(current_column_names.begin(),
                                                   current_column_names.end());

  // Resolve conflicts if the name is already taken
  while (current_name_set.count(name)) {
    name += ".";
    size_t number = 1;
    std::string non_conflict_name = name + std::to_string(number);
    while(current_name_set.count(non_conflict_name)) {
      ++number;
      non_conflict_name = name + std::to_string(number);
    }
    name = non_conflict_name;
  }
  return name;
}

void unity_sframe::show(const std::string& path_to_client) {
  using namespace turi;
  using namespace turi::visualization;

  std::shared_ptr<Plot> plt = std::dynamic_pointer_cast<Plot>(this->plot());

  if(plt != nullptr){
    plt->show(path_to_client);
  }
}

std::shared_ptr<model_base> unity_sframe::plot(){
  using namespace turi;
  using namespace turi::visualization;

  std::shared_ptr<unity_sframe_base> self = this->select_columns(this->column_names());

  return plot_columnwise_summary(self);
}

void unity_sframe::explore(const std::string& path_to_client, const std::string& title) {
  using namespace turi;
  using namespace turi::visualization;

  std::shared_ptr<unity_sframe_base> self = this->select_columns(this->column_names());

  logprogress_stream << "Materializing SFrame" << std::endl;
  this->materialize();

  if(self->size() == 0){
    log_and_throw("Nothing to explore; SFrame is empty.");
  }

  std::string titleString = turi::visualization::extra_label_escape(title);
  // This materializes if not already
  auto underlying_sframe = get_underlying_sframe();

  ::turi::visualization::run_thread([path_to_client, titleString, self, underlying_sframe]() {

    // get a reader just once.
    auto reader = underlying_sframe->get_reader();

    visualization::process_wrapper ew(path_to_client);
    const auto& column_types = self->dtype();
    const auto& column_names = self->column_names();
    std::queue<visualization::vega_data::Image> image_queue;

    using namespace boost;
    using namespace local_time;
    using namespace gregorian;
    using posix_time::time_duration;

    time_zone_names empty_timezone("", "", "", "");
    time_duration empty_utc_offset(0,0,0);
    dst_adjustment_offsets empty_adj_offsets(time_duration(0,0,0),
                                             time_duration(0,0,0),
                                             time_duration(0,0,0));
    time_zone_ptr empty_tz(
        new custom_time_zone(empty_timezone, empty_utc_offset,
                             empty_adj_offsets,
                             boost::shared_ptr<dst_calc_rule>()));

    // send table spec
    {
      std::stringstream ss;
      ss << "{\"table_spec\":{\"column_names\": [";
      for (size_t i=0; i<self->num_columns(); i++) {
        const auto& name = column_names[i];
        ss << visualization::extra_label_escape(name);
        if (i != self->num_columns() - 1) {
          ss << ",";
        }
      }
      ss << "], \"size\": ";
      ss << self->size();
      ss << ", \"title\": ";
      ss << titleString;
      ss << ", \"column_types\": [";
      for (size_t i=0; i<self->num_columns(); i++) {
        const auto& type = column_types[i];
        ss << "\"" << flex_type_enum_to_name(type) << "\"";
        if (i != self->num_columns() - 1) {
          ss << ",";
        }
      }
      ss << "]}}" << std::endl;
      ew << ss.str();
    }

    auto getRows = [self, &reader, &ew, &column_names, &empty_tz, &image_queue](size_t start, size_t end) {

      // send table data
      {
        std::queue<visualization::vega_data::Image> empty;
        std::swap( image_queue, empty );

        sframe_rows rows;
        reader->read_rows(start, end, rows);
        std::stringstream ss;

        // for DateTime string formatting
        ss.exceptions(std::ios_base::failbit);
        ss.imbue(std::locale(ss.getloc(),
                              new boost::local_time::local_time_facet("%Y-%m-%d %H:%M:%S%ZP")));
        // {"data_spec": {"values": [{"a": "A","b": 28}, {"a": "B","b": 55}, {"a": "C","b": 43},{"a": "D","b": 91}, {"a": "E","b": 81}, {"a": "F","b": 53},{"a": "G","b": 19}, {"a": "H","b": 87}, {"a": "I","b": 52}]}}
        ss << "{\"data_spec\": {\"values\": [";
        size_t i = 0;
        for (const auto& row: rows) {
          ss << "{";
          size_t count = start + i;
          ss << "\"__idx\": \"" << count << "\",";
          for (size_t j=0; j<row.size(); j++) {
            const auto& columnName = column_names[j];
            const auto& value = row[j];

            ss << visualization::extra_label_escape(columnName) << ": ";
            ss << escapeForTable(value, empty_tz, &image_queue, count, columnName);

            if (j != row.size() - 1) {
              ss << ",";
            }
          }
          ss << "}";
          if (i != rows.num_rows() - 1) {
            ss << ",";
          }
          ++i;
        }
        ss << "]}}" << std::endl;
        ew << ss.str();
      }
    };

    auto getAccordion = [self, &ew, &column_names, &empty_tz](std::string column_name, size_t index) {

        ASSERT_TRUE(std::find(column_names.begin(), column_names.end(), column_name) != column_names.end());
        DASSERT_LT(index, self->size());
        DASSERT_GE(index, 0);

        auto accordion_sa = self->select_column(column_name);
        auto gl_sa = gl_sarray(accordion_sa);

        flexible_type value = gl_sa[index];

        switch (value.get_type()) {
          case flex_type_enum::UNDEFINED:
            break;
          case flex_type_enum::FLOAT:
            {
              std::stringstream ss;
              ss << "{\"accordion_spec\": {\"index\": " << index << ", \"column\":" << turi::visualization::extra_label_escape(column_name);
              ss << ", \"type\": " << value.get_type();
              ss << ", \"data\": " << value.get<flex_float>();
              ss << "}}" << std::endl;
              ew << ss.str();
            }
            break;
          case flex_type_enum::INTEGER:
            {
              std::stringstream ss;
              ss << "{\"accordion_spec\": {\"index\": " << index << ", \"column\":" << turi::visualization::extra_label_escape(column_name);
              ss << ", \"type\": " << value.get_type();
              ss << ", \"data\": " << value.get<flex_int>();
              ss << "}}" << std::endl;
              ew << ss.str();
            }
            break;
          case flex_type_enum::IMAGE:
            {
              std::stringstream ss;
              flex_image img = value.get<flex_image>();
              img = turi::image_util::encode_image(img);

              const unsigned char * image_data = img.get_image_data();
              size_t image_data_size = img.m_image_data_size;

              ss << "{\"accordion_spec\": {\"index\": " << index << ", \"column\":" << turi::visualization::extra_label_escape(column_name);
              ss << ", \"type\": " << value.get_type();
              ss << ", \"data\": ";
              ss << "{\"width\": " << img.m_width << ", ";
              ss << "\"height\": " << img.m_height << ", ";
              ss << "\"data\": \"";
              std::copy(
                to_base64(image_data),
                to_base64(image_data + image_data_size),
                std::ostream_iterator<char>(ss)
              );

              ss << "\", \"format\": \"";
              switch (img.m_format) {
                case Format::JPG:
                  ss << "jpeg";
                  break;
                case Format::PNG:
                  ss << "png";
                  break;
                case Format::RAW_ARRAY:
                  ss << "raw";
                  break;
                case Format::UNDEFINED:
                  // TODO - not sure what to do here.
                  // For now, treat it as raw, but this will probably
                  // display garbage for the user.
                  ss << "raw";
                  break;
              }
              ss << "\"}}}\n";
              ew << ss.str();
            }

            break;
          case flex_type_enum::DATETIME:
            {
              std::stringstream ss;
              ss << "{\"accordion_spec\": {\"index\": " << index << ", \"column\":" << turi::visualization::extra_label_escape(column_name);
              ss << ", \"type\": " << value.get_type();
              ss << ", \"data\": ";
              ss << "\"";
              const auto& dt = value.get<flex_date_time>();

              if (dt.time_zone_offset() != flex_date_time::EMPTY_TIMEZONE) {
                std::string prefix = "0.";
                int sign_adjuster = 1;
                if(dt.time_zone_offset() < 0) {
                  sign_adjuster = -1;
                  prefix = "-0.";
                }
                boost::local_time::time_zone_ptr zone(
                    new boost::local_time::posix_time_zone(
                        "GMT" + prefix +
                        std::to_string(sign_adjuster *
                                       dt.time_zone_offset() *
                                       flex_date_time::TIMEZONE_RESOLUTION_IN_MINUTES)));
                boost::local_time::local_date_time az(
                    flexible_type_impl::ptime_from_time_t(dt.posix_timestamp(),
                                                          dt.microsecond()), zone);
                ss << az;
              } else {
                boost::local_time::local_date_time az(
                    flexible_type_impl::ptime_from_time_t(dt.posix_timestamp(),
                                                          dt.microsecond()),
                    empty_tz);
                ss << az;
              }

              ss << "\"}}" << std::endl;
              ew << ss.str();
            }
            break;
          case flex_type_enum::VECTOR:
            {
              std::stringstream ss;
              ss << "{\"accordion_spec\": {\"index\": " << index << ", \"column\":" << turi::visualization::extra_label_escape(column_name);
              ss << ", \"type\": " << value.get_type();
              ss << ", \"data\": ";
              std::stringstream strm;
              const flex_vec& vec = value.get<flex_vec>();

              strm << "[";
              for (size_t i = 0; i < vec.size(); ++i) {
                strm << vec[i];
                if (i + 1 < vec.size()) strm << ", ";
              }
              strm << "]";
              std::string default_string;

              ss << turi::visualization::extra_label_escape(strm.str());
              ss << "}}" << std::endl;
              ew << ss.str();
            }
            break;
          case flex_type_enum::LIST:
          case flex_type_enum::DICT:
          case flex_type_enum::ND_VECTOR:
          case flex_type_enum::STRING:
          default:
            {
              std::stringstream ss;
              ss << "{\"accordion_spec\": {\"index\": " << index << ", \"column\":" << turi::visualization::extra_label_escape(column_name);
              ss << ", \"type\": " << value.get_type();
              ss << ", \"data\": " << escapeForTable(value, empty_tz);
              ss << "}}" << std::endl;
              ew << ss.str();
              break;
            }
        };
    };

    // pass the first 1k rows
    getRows(0, 100);

    const size_t resized_height = 200;

    while (ew.good()) {
      // get input, send responses
      std::string input;
      ew >> input;
      if (input.empty()) {
        if (image_queue.size() == 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
          // send one image from the queue while we're not doing anything else
          std::stringstream ss;

          visualization::vega_data::Image image_processing = image_queue.front();

          flex_image img_temporary = image_processing.img;
          double image_ratio = ((img_temporary.m_width*1.0)/(img_temporary.m_height*1.0));
          double calculated_width = (image_ratio * resized_height);
          size_t resized_width = static_cast<int>(calculated_width);
          flex_image img = turi::image_util::resize_image(img_temporary,
                  resized_width, resized_height, img_temporary.m_channels, img_temporary.is_decoded());
          img = turi::image_util::encode_image(img);

          const unsigned char * image_data = img.get_image_data();

          size_t image_data_size = img.m_image_data_size;

          ss << "{\"image_spec\":{\"data\": [{\"idx\": " << image_processing.idx << ", ";
          ss << "\"column\": " << image_processing.column << ", ";
          ss << "\"image\": \"";

          std::copy(
            to_base64(image_data),
            to_base64(image_data + image_data_size),
            std::ostream_iterator<char>(ss)
          );

          ss << "\", \"format\": \"";
          switch (img.m_format) {
            case Format::JPG:
              ss << "jpeg";
              break;
            case Format::PNG:
              ss << "png";
              break;
            case Format::RAW_ARRAY:
              ss << "raw";
              break;
            case Format::UNDEFINED:
              ss << "raw";
              break;
          }

          ss << "\"}]}}" << std::endl;

          ew << ss.str();
          image_queue.pop();
        }

        continue;
      }

      // parse the message as json
      flex_int start = -1, end = -1, index = -1;
      std::string column_name;

      enum MethodType {GetRows = 0, GetAccordion = 1};
      auto response = NONE<MethodType>();

      auto sa = gl_sarray(std::vector<flexible_type>(1, input)).astype(flex_type_enum::DICT);
      flex_dict dict = sa[0].get<flex_dict>();
      for (const auto& pair : dict) {
        const auto& key = pair.first.get<flex_string>();
        const auto& value = pair.second;
        if (key == "method") {
          if(value.get<flex_string>() == "get_rows"){
            response = SOME(GetRows);
          }else if(value.get<flex_string>() == "get_accordion"){
            response = SOME(GetAccordion);
          }
        } else if (key == "start") {
          start = value.get<flex_int>();
        } else if (key == "end") {
          end = value.get<flex_int>();
        }else if (key == "column") {
          column_name = value.get<flex_string>();
        }else if (key == "index"){
          index = value.get<flex_int>();
        }
      }

      if (!!response && *response == GetRows) {
        getRows(start, end);
      } else if (!!response && *response == GetAccordion) {
        getAccordion(column_name, index);
      } else {
        std_log_and_throw(
          std::runtime_error, "Unsupported case (should be either GetRows or GetAccordion).");
        ASSERT_UNREACHABLE();
      }
    }
  });
}

} // namespace turi
