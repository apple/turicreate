/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_reader.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/storage/sframe_data/parallel_csv_parser.hpp>
#include <core/storage/sframe_data/csv_writer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <boost/filesystem.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <core/storage/sframe_data/sframe_saving.hpp>
#include <core/storage/sframe_data/sframe_compact.hpp>
#include <core/system/exceptions/error_types.hpp>
namespace turi {

sframe::sframe(const sframe& other) {
  Dlog_func_entry();
  if (other.inited) {
    if (other.writing) {
      log_and_throw("Cannot copy an array which is writing");
    } else {
      index_info = other.index_info;
      index_file = other.index_file;
      columns = other.columns;
      inited = true;
      writing = false;
    }
  }
}

sframe& sframe::operator=(const sframe& other) {
  ASSERT_MSG(!writing, "Cannot copy over an array which is currently writing");
  reset();
  if (other.inited) {
    ASSERT_MSG(!other.writing, "Cannot copy an array which is writing");
    index_info = other.index_info;
    index_file = other.index_file;
    columns = other.columns;
    inited = true;
    writing = false;
  } else {
    inited = false;
  }
  return *this;
}

sframe::sframe(const dataframe_t& data) {
  Dlog_func_entry();
  // extract the column information
  std::vector<std::string> column_names = data.names;
  std::vector<flex_type_enum> column_types(column_names.size());
  std::vector<const std::vector<flexible_type>*> column_values(column_names.size());

  for (size_t i = 0;i < column_names.size(); ++i) {
    column_types[i] = data.types.at(column_names[i]);
    column_values[i] = &(data.values.at(column_names[i]));
  }

  // create the sframe
  open_for_write(column_names, column_types, "", 1);
  group_writer->set_options("disable_padding", 1);

  auto output_iter = get_output_iterator(0);
  std::vector<flexible_type> buf(column_names.size());
  size_t nrows = data.nrows();
  for (size_t i = 0; i < nrows; ++i) {
    for (size_t j = 0; j < buf.size(); ++j) {
      buf[j] = (*(column_values[j]))[i];
    }
    (*output_iter) = buf;
  }
  close();
};



/**
 * Move Assignment operator.
 * Moves other into this. Other will be cleared as if it is a newly
 * constructed sframe object.
 */
sframe& sframe::operator=(sframe&& other) {
  index_info = std::move(other.index_info);
  index_file = std::move(other.index_file);
  columns = std::move(other.columns);
  group_writer = std::move(other.group_writer);
  inited = std::move(other.inited);
  writing = std::move(other.writing);

  other.index_info = sframe_index_file_information();
  other.index_file = "";
  other.columns.clear();

  other.inited = false;
  other.writing = false;
  return *this;
}

std::map<std::string, std::shared_ptr<sarray<flexible_type>>> sframe::init_from_csvs(
    const std::string& url,
    csv_line_tokenizer& tokenizer,
    bool use_header,
    bool continue_on_failure,
    bool store_errors,
    std::map<std::string, flex_type_enum> column_type_hints,
    std::vector<std::string> output_columns,
    size_t row_limit,
    size_t skip_rows) {
  csv_file_handling_options opts;
  opts.use_header = use_header;
  opts.continue_on_failure = continue_on_failure;
  opts.store_errors = store_errors;
  opts.column_type_hints = column_type_hints;
  opts.output_columns = output_columns;
  opts.row_limit = row_limit;
  opts.skip_rows = skip_rows;
  return parse_csvs_to_sframe(url, tokenizer, opts, *this);
}

sframe::~sframe() {
  Dlog_func_entry();
}

void sframe::create_arrays_for_reading(sframe_index_file_information frame_index_info) {
  Dlog_func_entry();
  logstream(LOG_DEBUG) << "Opening Frame for Reading of size (" << frame_index_info.nrows
                      << "," << frame_index_info.ncolumns << ")" << std::endl;
  reset();
  writing = false;
  index_info = frame_index_info;
  // open each column for read
  for (size_t i = 0; i < index_info.ncolumns; ++i) {
    columns.emplace_back(new sarray<flexible_type>);
  }
  /*
   * In a regular saved sframe, each sarray has index file of the following form:
   *  1st col : group_index.sidx:0
   *  2nd col : group_index.sidx:1
   *  3rd col : group_index.sidx:2
   *  etc.
   *
   * We would like to avoid every one of the sarray to have to reparse the
   * group_index.sidx file.
   */
  std::map<std::string, group_index_file_information> index_groups;
  for (size_t i = 0; i < index_info.ncolumns; ++i) {
    std::string group_index_file =
        parse_v2_segment_filename(index_info.column_files[i]).first;
    if (index_groups.count(group_index_file) == 0) {
      index_groups[group_index_file] =
          read_array_group_index_file(group_index_file);
    }
  }

  for (size_t i = 0; i < index_info.ncolumns; ++i) {
    std::string group_index_file;
    size_t colid;
    std::tie(group_index_file, colid) =
        parse_v2_segment_filename(index_info.column_files[i]);
    if (index_groups[group_index_file].version == 1) {
      columns[i]->open_for_read(frame_index_info.column_files[i]);
    } else {
      columns[i]->open_for_read(index_groups[group_index_file].columns[colid]);
    }
  }

  keep_array_file_ref();
}

void sframe::create_arrays_for_reading(
          const std::vector<std::shared_ptr<sarray<flexible_type> > >& new_columns,
          const std::vector<std::string>& column_names,
          bool fail_on_column_names) {
  Dlog_func_entry();
  reset();
  writing = false;
  if(new_columns.empty()) {
    return;
  }
  // fill index_info manually
  columns = new_columns;
  index_info.column_files.resize(columns.size());
  index_info.version = 0;
  index_info.ncolumns = columns.size();
  index_info.nrows = columns[0]->size();

  // Sanity check that the column structure for each column is correct
  for(const auto &i : columns) {
    // number of rows must match
    if(i->size() != index_info.nrows) {
      std::stringstream msg;
      msg << "Columns do not have the same length! ";
      msg << "Expected " << index_info.nrows;
      msg << ", found " << i->size() << ".";
      log_and_throw(msg.str());
    }
  }

  // Check uniqueness of column names
  if(fail_on_column_names) {
    std::set<std::string> column_name_set(column_names.begin(),
                                          column_names.end());
    if(column_names.size() != column_name_set.size()) {
      log_and_throw(std::string("All column names must be unique!"));
    }
  }

  // fill up the column names
  for (size_t i = 0;i < columns.size(); ++i) {
    if(column_names.size() > i) {
      index_info.column_names.push_back(generate_valid_column_name(column_names[i]));
    } else {
      index_info.column_names.push_back(generate_valid_column_name(std::string("")));
    }
    // fill up the column files
    index_info.column_files[i] = new_columns[i]->get_index_file();
  }
}

/**
 * Internal function. Given the index_file, this function
 * initializes each of the sarrays for writing; filling up
 * the columns array.
 */
void sframe::create_arrays_for_writing(const std::vector<std::string>& column_names,
                                       const std::vector<flex_type_enum>& column_types,
                                       size_t nsegments,
                                       const std::string& frame_sidx_file,
                                       bool fail_on_column_names) {
  Dlog_func_entry();
  logstream(LOG_DEBUG) << "Opening Frame for writing to " << frame_sidx_file
                      << " with " << nsegments << " segments and "
                      << column_names.size() << " columns" << std::endl;
  reset();
  writing = true;

  // fill up index_info
  index_info.column_files.resize(column_names.size());
  index_info.version = 0;
  index_info.ncolumns = column_names.size();
  index_info.nrows = 0;

  // Add column names, in the case there are lots of columns, the check for contains_column
  // is very slow, we try to avoid checking that if we know up front all names already
  std::unordered_set<std::string> unique_names(column_names.begin(), column_names.end());
  bool all_names_unique = unique_names.size() == column_names.size();

  index_info.column_names.reserve(column_names.size());
  for(size_t i = 0; i < column_names.size(); ++i) {
    if (column_names[i].empty() || !all_names_unique) {
      index_info.column_names.push_back(generate_valid_column_name(column_names[i]));
    } else {
      index_info.column_names.push_back(column_names[i]);
    }

    // If this wasn't asking for an automatic name (by being empty) and is different
    // then there was a conflict in naming
    if(fail_on_column_names &&
        !column_names[i].empty() &&
        (index_info.column_names.back() != column_names[i])) {
      log_and_throw("All column names must be unique!");
    }
  }

  std::string suffix = ".frame_idx";
  bool uses_temp_files = false;
  if (frame_sidx_file.empty()) {
    uses_temp_files = true;
    index_file =
        fileio::fixed_size_cache_manager::get_instance().get_temp_cache_id(suffix);
  } else {
    if (boost::algorithm::ends_with(frame_sidx_file, suffix)) {
      index_file = frame_sidx_file;
    } else {
      log_and_throw(std::string("Index file must end with " + suffix));
    }
  }

  std::string prefix =
      index_file.substr(0, index_file.length() - suffix.length());

  group_writer.reset(new sarray_group_format_writer_v2<flexible_type>);
  if (uses_temp_files) {
    std::string group_sidx =
        fileio::fixed_size_cache_manager::get_instance().get_temp_cache_id(".sidx");
    group_writer->open(group_sidx, nsegments, index_info.ncolumns);
  } else {
    group_writer->open(prefix + ".sidx", nsegments, index_info.ncolumns);
  }

  // open each column for write. Write to the same location as the frame index
  for (size_t i = 0; i < index_info.ncolumns; ++i) {
    group_writer->get_index_info().columns[i].metadata["__type__"] =
        std::to_string(static_cast<int>(column_types[i]));
  }
}


sframe sframe::append(const sframe& other) const {
  // both cannot be writing
  ASSERT_EQ(writing, false);
  ASSERT_EQ(other.writing, false);
  // if one is inited, return the other
  if (!other.inited) return *this;
  if (!inited) return other;


  // cannot combine across format version
  ASSERT_EQ(index_info.version, other.index_info.version);
  // validate columns are identical in both number, name, and type
  ASSERT_EQ(column_names().size(), other.column_names().size());
  for (size_t i = 0;i < column_names().size(); ++i) {
    ASSERT_EQ(column_name(i), other.column_name(i));
    ASSERT_EQ((int)column_type(i), (int)other.column_type(i));
  }

  sframe ret = (*this);
  // validated. now combine each column individually
  for (size_t i = 0;i < ret.columns.size(); ++i) {
    // append the columns
    ret.columns[i] = std::make_shared<sarray<flexible_type>>
        (ret.columns[i]->append(*other.columns[i]));
  }
  ret.index_info.nrows += other.index_info.nrows;
  ret.try_compact();
  return ret;
}

void sframe::try_compact() {
  for (auto& col : columns) col->try_compact();
}

std::unique_ptr<sframe::reader_type> sframe::get_reader() const {
  Dlog_func_entry();
  ASSERT_MSG(inited, "Invalid SFrame");
  ASSERT_MSG(!writing, "SFrame not opened for reading");
  std::unique_ptr<reader_type> reader(new reader_type());
  reader->init(*this);
  return reader;
}

std::unique_ptr<sframe::reader_type> sframe::get_reader(size_t num_segments) const {
  Dlog_func_entry();
  ASSERT_MSG(inited, "Invalid SFrame");
  ASSERT_MSG(!writing, "SFrame not opened for reading");
  std::unique_ptr<reader_type> reader(new reader_type());
  reader->init(*this, num_segments);
  return reader;
}


std::unique_ptr<sframe::reader_type> sframe::get_reader(const std::vector<size_t>& segment_lengths) const {
  Dlog_func_entry();
  ASSERT_MSG(inited, "Invalid SFrame");
  ASSERT_MSG(!writing, "SFrame not opened for reading");
  std::unique_ptr<reader_type> reader(new reader_type());
  reader->init(*this, segment_lengths);
  return reader;
}

/**************************************************************************/
/*                                                                        */
/*                     Other SFrame Unique Accessors                      */
/*                                                                        */
/**************************************************************************/

dataframe_t sframe::to_dataframe() {
  log_func_entry();
  dataframe_t ret;
  for (size_t i = 0; i < num_columns(); ++i) {
    std::string name = column_name(i);
    ret.set_column(name, std::vector<flexible_type>(), column_type(i));
    std::vector<flexible_type>& out_column = ret.values[name];
    turi::copy(*columns[i], std::inserter(out_column, out_column.begin()));
  }
  return ret;
}


std::shared_ptr<sarray<flexible_type> > sframe::select_column(size_t column_id) const {
  if (column_id < num_columns()) {
    return columns[column_id];
  } else {
    log_and_throw (std::string("Select column index out of bound. " +
                       std::to_string(column_id)));
  }
}

std::shared_ptr<sarray<flexible_type> > sframe::select_column(const std::string &name) const {
  size_t col_index = column_index(name);
  return select_column(col_index);
}

sframe sframe::select_columns(const std::vector<std::string>& names) const {
  log_func_entry();
  std::vector<std::shared_ptr<sarray<flexible_type> > > new_columns;
  for (const auto& name : names) {
    size_t col_index = column_index(name);
    new_columns.push_back(columns[col_index]);
  }
  return sframe(new_columns, names);
}


sframe sframe::add_column(std::shared_ptr<sarray<flexible_type> > sarr_ptr,
                          const std::string& column_name) const {
  log_func_entry();
  if (num_columns() == 0) {
    // appending to an empty sframe. just return a new sframe of 1 column
    std::vector<std::shared_ptr<sarray<flexible_type> > > new_columns{sarr_ptr};
    std::vector<std::string> new_column_names{column_name};
    return sframe(new_columns, new_column_names);
  }

  // Make sure we're given a correctly formed column
  if(num_rows() != sarr_ptr->size()) {
    log_and_throw(std::string("Column must have the same # of rows as sframe."));
  }

  std::vector<std::shared_ptr<sarray<flexible_type> > > new_columns = columns;
  std::vector<std::string> new_column_names = index_info.column_names;
  new_columns.push_back(sarr_ptr);

  // We can pick a non-conflicting name, but if you're adding a column, you
  // probably want to be reminded there's a conflict and resolve it yourself.
  if(contains_column(column_name)) {
    log_and_throw(std::string("Attempt to add a column with existing name: "
                      + column_name + ". All column names must be unique!"));
  }

  std::string tmp = generate_valid_column_name(column_name);
  new_column_names.push_back(tmp);

  return sframe(new_columns, new_column_names);
}

std::string sframe::generate_valid_column_name(const std::string &column_name) const {
  std::string name;

  if(column_name.empty()) {
    // generate a column name
    name = std::string("X") + std::to_string(index_info.column_names.size()+1);
  } else {
    name = column_name;
  }

  // Resolve conflicts if the name is already taken
  if(contains_column(name)) {
    name += ".";
    size_t number = 1;
    std::string non_conflict_name = name + std::to_string(number);
    while(contains_column(non_conflict_name) > 0) {
      ++number;
      non_conflict_name = name + std::to_string(number);
    }
    name = non_conflict_name;
  }

  return name;
}


void sframe::set_column_name(size_t i, const std::string& name) {
  ASSERT_LT(i, num_columns());
  index_info.column_names[i] = name;
}

sframe sframe::remove_column(size_t i) const {
  ASSERT_LT(i, num_columns());

  std::vector<std::shared_ptr<sarray<flexible_type> > > new_columns = columns;
  std::vector<std::string> new_column_names = index_info.column_names;
  new_columns.erase(new_columns.begin() + i);
  new_column_names.erase(new_column_names.begin() + i);
  return sframe(new_columns, new_column_names);
}

sframe sframe::swap_columns(size_t column_1, size_t column_2) const {
  ASSERT_LT(column_1, num_columns());
  ASSERT_LT(column_2, num_columns());

  std::vector<std::shared_ptr<sarray<flexible_type> > > new_columns = columns;
  std::vector<std::string> new_column_names = index_info.column_names;

  std::swap(new_columns[column_1], new_columns[column_2]);
  std::swap(new_column_names[column_1], new_column_names[column_2]);

  return sframe(new_columns, new_column_names);
}

sframe sframe::replace_column(std::shared_ptr<sarray<flexible_type>> sarr_ptr,
                              const std::string& column_name) const {
  ASSERT_TRUE(contains_column(column_name));
  std::string tmp_column_name = "__" + column_name + "__";
  while(contains_column(tmp_column_name)) {
    tmp_column_name += "__";
  }
  sframe newsf = add_column(sarr_ptr, tmp_column_name);
  size_t oldidx = newsf.column_index(column_name);
  size_t newidx = newsf.column_index(tmp_column_name);
  newsf = newsf.swap_columns(oldidx, newidx);
  newsf = newsf.remove_column(newidx);
  newsf.set_column_name(oldidx, column_name);
  return newsf;
}

/**************************************************************************/
/*                                                                        */
/*                           Writing Functions                            */
/*                                                                        */
/**************************************************************************/

bool sframe::set_num_segments(size_t numseg) {
  Dlog_func_entry();
  ASSERT_MSG(inited, "Invalid SFrame");
  ASSERT_MSG(!writing, "SFrame not opened for writing");
  if (numseg == 0) return false;
  if (numseg != num_segments()) {
    // re-open
    std::string idx_file = group_writer->get_index_info().group_index_file;
    size_t ncols = group_writer->get_index_info().columns.size();
    group_writer.reset(new sarray_group_format_writer_v2<flexible_type>());
    group_writer->open(idx_file, numseg, ncols);
  }
  return true;
}

sframe::iterator sframe::get_output_iterator(size_t segmentid) {
  ASSERT_MSG(inited, "Invalid SFrame");
  ASSERT_MSG(writing, "SFrame not opened for writing");
  ASSERT_MSG((segmentid < num_segments() || num_segments() == 0), "Invalid segment ID");
  std::vector<flex_type_enum> ctypes = column_types();
  return sframe::iterator(
      [=](const std::vector<flexible_type>& val)->void{
        // check that all the types match up
        if (UNLIKELY(val.size() != ctypes.size())) {
          std::stringstream ss;
          ss << "Can not write to SFrame, got the wrong number of columns. "
             << "Expected: " << ctypes.size() << " columns. Got: " << val.size()
             << " columns.";
          log_and_throw(ss.str());
        }
        bool badtype = false;
        for (size_t i = 0;i < val.size(); ++i) {
          if (val[i].get_type() != flex_type_enum::UNDEFINED &&
              ctypes[i] != flex_type_enum::UNDEFINED &&
              val[i].get_type() != ctypes[i]) {
            badtype = true;
            break;
          }
        }
        if (!badtype) {
          // there are no bad types, write
          this->write(segmentid, val);
        } else {
          // there are bad types. rewrite the value
          std::vector<flexible_type> newval(val.size());
          for (size_t i = 0;i < val.size(); ++i) {
            if (val[i].get_type() != flex_type_enum::UNDEFINED &&
                ctypes[i] != flex_type_enum::UNDEFINED &&
                val[i].get_type() != ctypes[i]) {
              if (flex_type_is_convertible(val[i].get_type(), ctypes[i])) {
                newval[i].reset(ctypes[i]);
                newval[i].soft_assign(val[i]);
              } else {
                std::string message = "Cannot convert " + std::string(val[i]) + " to " + flex_type_enum_to_name(ctypes[i]);
                logstream(LOG_ERROR) <<  message << std::endl;
                throw(bad_cast(message));
              }
            } else {
              newval[i] = val[i];
            }
          }
          this->write(segmentid, std::move(newval));
        }
      },
      [=](std::vector<flexible_type>&& val)->void{
        // check that all the types match up
        // but with this one we can modify val directly
        if (UNLIKELY(val.size() != ctypes.size())) {
          std::stringstream ss;
          ss << "Can not write to SFrame, got the wrong number of columns. "
             << "Expected: " << ctypes.size() << " columns. Got: " << val.size()
             << " columns.";
          log_and_throw(ss.str());
        }

        for (size_t i = 0;i < val.size(); ++i) {
          if (val[i].get_type() != ctypes[i] &&
              val[i].get_type() != flex_type_enum::UNDEFINED &&
              ctypes[i] != flex_type_enum::UNDEFINED) {
            if (flex_type_is_convertible(val[i].get_type(), ctypes[i])) {
              flexible_type newval(ctypes[i]);
              newval.soft_assign(val[i]);
              val[i] = std::move(newval);
            } else {
              std::string message = "Cannot convert " + std::string(val[i]) + " to " + flex_type_enum_to_name(ctypes[i]);
              logstream(LOG_ERROR) <<  message << std::endl;
              throw(bad_cast(message));
            }
          }
        }
        this->write(segmentid, std::forward<std::vector<flexible_type>>(val));
      },
      [=](const sframe_rows& sfr)->void {
        if (sfr.num_columns() != ctypes.size()) {
          std::stringstream ss;
          ss << "Write to sframe with row size mismatch. "
             << "Expected: " << ctypes.size() << " Actual: " << sfr.num_columns();
          log_and_throw(ss.str());
        } else {
          this->write(segmentid, sfr.type_check({ctypes}));
        }
      });
}

void sframe::flush_write_to_segment(size_t segment) {
  if (group_writer) {
    group_writer->flush_segment(segment);
  } else {
    log_and_throw("Attempting to flush an SFrame not opened for writing");
  }
}

void sframe::close() {
  group_writer->close();
  group_writer->write_index_file();
  group_index_file_information group_index = group_writer->get_index_info();
  Dlog_func_entry();
  ASSERT_MSG(inited, "Invalid SFrame");
  ASSERT_MSG(writing, "SFrame not opened for writing");
  if (index_info.ncolumns > 0) {
    index_info.nrows = 0;
    for (size_t rows: group_writer->get_index_info().columns[0].segment_sizes) {
      index_info.nrows += rows;
    }
    index_info.column_files.resize(index_info.ncolumns);
    for (size_t col = 0; col < index_info.ncolumns; ++col) {
      index_info.column_files[col] = group_index.columns[col].index_file;
    }
  } else {
    index_info.nrows = 0;
  }

  if (!group_index.group_index_file.empty()) {
    index_file_handle.push_back(
        fileio::file_handle_pool::get_instance().register_file(
            group_index.group_index_file));
  }
  group_writer.reset();
  write_sframe_index_file(index_file, index_info);
  inited = true;
  writing = false;
  columns.resize(index_info.ncolumns);
  for (size_t i = 0;i < index_info.ncolumns; ++i) {
    columns[i].reset(new sarray<flexible_type>());
    columns[i]->open_for_read(group_index.columns[i]);
  }
  // we can now read.
  keep_array_file_ref();
}


void sframe::save_as_csv(std::string csv_file,
                         csv_writer& writer) {
  general_ofstream fout(csv_file);
  if (!fout.good()) {
    log_and_throw(std::string("Unable to open " + sanitize_url(csv_file) + " for write"));
  }
  auto reader = get_reader(1);
  auto iter_begin = reader->begin(0);
  auto iter_end = reader->end(0);

  writer.write_verbatim(fout, column_names());

  while(iter_begin != iter_end) {
    writer.write(fout, (*iter_begin));
    ++iter_begin;
  }
}

bool sframe::set_metadata(const std::string& key, std::string val) {
  Dlog_func_entry();
  ASSERT_MSG(inited, "Invalid SFrame");
  ASSERT_MSG(writing, "SFrame not opened for writing");
  index_info.metadata[key] = val;
  return true;
}


void sframe::reset() {
  Dlog_func_entry();
  index_file = "";
  index_info = sframe_index_file_information();
  columns.clear();
}


void sframe::write(size_t segmentid, const std::vector<flexible_type>& t) {
  DASSERT_TRUE(group_writer != NULL);
  group_writer->write_segment(segmentid, t);
}

void sframe::write(size_t segmentid, std::vector<flexible_type>&& t) {
  DASSERT_TRUE(group_writer != NULL);
  group_writer->write_segment(segmentid, std::forward<std::vector<flexible_type>>(t));
}

void sframe::write(size_t segmentid, const sframe_rows& t) {
  DASSERT_TRUE(group_writer != NULL);
  group_writer->write_segment(segmentid, t);
}

void sframe::save(std::string index_file) const {
  ASSERT_TRUE(inited);
  ASSERT_FALSE(writing);
  std::string expected_ext(".frame_idx");
  if(!boost::algorithm::ends_with(index_file, expected_ext)) {
    log_and_throw("Index file must end with " + expected_ext);
  }

  sframe_save(*this, index_file);
}

void sframe::debug_print() {
    std::stringstream ss;
    auto names = column_names();
    auto types = column_types();

    ss << "column_names:\n";
    for (auto& s: names) ss << s << "\t";
    ss << "\n";
    ss << "column_types:\n";
    for (auto& t: types) ss << flex_type_enum_to_name(t) << "\t";
    ss << "\n";
    ss << "num_rows:" << num_rows() << "\ndata:\n";
    auto reader = get_reader();
    std::vector<std::vector<flexible_type>> buffer;
    reader->read_rows(0, num_rows(), buffer);
    std::for_each(buffer.begin(), buffer.end(), [&](const std::vector<flexible_type>& row) {
        for (const auto& val: row) { ss << (std::string)(val) << "\t"; }
        ss << "\n";
    });
    std::cerr << ss.str() << std::endl;
}

void sframe::save(oarchive& oarc) const {
  std::string prefix = oarc.get_prefix();
  save(prefix + ".frame_idx");
}

void sframe::load(iarchive& iarc) {
  std::string prefix = iarc.get_prefix();
  auto frame_index_info = read_sframe_index_file(prefix + ".frame_idx");
  open_for_read(frame_index_info);
}

bool sframe::delete_files_on_destruction() {
  for(auto &i: columns) {
    i->delete_files_on_destruction();
  }
  for(auto &fh : index_file_handle) {
    fh->delete_on_destruction();
  }

  return true;
}

void sframe::keep_array_file_ref() {
  // Add cache entries for frame_idx
  if (!index_file.empty()) {
    index_file_handle.push_back(
        fileio::file_handle_pool::get_instance().register_file(index_file));
  }
  if (!index_info.file_name.empty()) {
    index_file_handle.push_back(
        fileio::file_handle_pool::get_instance().register_file(index_info.file_name));
  }
  // And all group sarray index files
  std::set<std::string> index_groups;
  for (size_t i = 0; i < index_info.ncolumns; ++i) {
    std::string group_index_file =
        parse_v2_segment_filename(index_info.column_files[i]).first;
    index_groups.insert(group_index_file);
  }

  for(auto &file : index_groups) {
    index_file_handle.push_back(
        fileio::file_handle_pool::get_instance().register_file(file));
  }

}
} // end of namespace
