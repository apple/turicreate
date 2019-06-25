/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <map>
#include <core/storage/sframe_data/dataframe.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <core/storage/fileio/general_fstream.hpp>
namespace turi {
/**************************************************************************/
/*                                                                        */
/*                               dataframe                                */
/*                                                                        */
/**************************************************************************/
  /*
   * Helper function for read_csv.
   * Parse a flexible_type from a string.
   *
   * Note:
   * Supported types: int, float, string, and undefined.
   * Empty string will is treated as UNDEFINED.
   */
  flexible_type parse_flexible_type(const std::string& str) {
    flexible_type ret;
    using boost::spirit::qi::phrase_parse;
    using boost::spirit::qi::double_;
    using boost::spirit::qi::int_;
    using boost::spirit::lit;
    using boost::spirit::qi::eoi;
    using boost::spirit::qi::eps;
    using boost::spirit::qi::space;
    using boost::spirit::qi::blank;

    bool r = phrase_parse(str.begin(), str.end(),
      (
          *blank >> (
            int_ [([&](int v){
                   // std::cerr << str << ":int\n";
                   ret = v;
                   })] |
            double_[([&](double v){
                     // std::cerr << str << ":double\n";
                     ret = v;
                     })]
          ) >> *blank
      ), eoi);

    if (!r) {
      if (str.empty()) {
        // std::cerr << str << ":nan\n";
        ret.reset(flex_type_enum::UNDEFINED);
      } else {
        // std::cerr << str << ":string\n";
        ret = trim(str);
      }
    }
    return ret;
  }

  void dataframe_t::set_type(std::string key, flex_type_enum type) {
    if (!contains(key)) {
      log_and_throw("Column " + key + " not found.");
    }
    std::vector<flexible_type>& column = values[key];
    switch(type) {
     case flex_type_enum::INTEGER:
       parallel_for(0, nrows(),
                    [&](size_t i) {
                      if (column[i].get_type() != flex_type_enum::UNDEFINED) {
                        column[i] = (flex_int)column[i];
                      }
                    });
       break;
     case flex_type_enum::FLOAT:
       parallel_for(0, nrows(),
                    [&](size_t i) {
                      if (column[i].get_type() != flex_type_enum::UNDEFINED) {
                        column[i] = (flex_float)column[i];
                      }
                    });
       break;
     case flex_type_enum::STRING:
       parallel_for(0, nrows(),
                    [&](size_t i) {
                      if (column[i].get_type() != flex_type_enum::UNDEFINED) {
                        column[i] = (flex_string)column[i];
                      }
                    });
       break;
     default:
       log_and_throw("Set column type into " + std::string(flex_type_enum_to_name(type)) + " is not supported");
    }
    types[key] = type;
  }


  void dataframe_t::print() const {
    auto iter = values.begin();
    while (iter != values.end()) {
      const std::string& colname = iter->first;
      auto typeiter = types.find(colname);
      ASSERT_TRUE(typeiter != types.end());
      flex_type_enum t = typeiter->second;
      std::cerr << "column: " << colname
                << "| type: " << flex_type_enum_to_name(t) << "\n";
      auto subiter = iter->second.begin();
      while (subiter != iter->second.end())  {
        std::cerr << (std::string)(*subiter) << "\t";
        ++subiter;
      }
      std::cerr << "\n";
      ++iter;
    }
  }


  void dataframe_t::set_column(std::string key,
                               const std::vector<flexible_type>& val,
                               flex_type_enum type) {
    if (!values.count(key)) {
      names.push_back(key);
    }
    values[key] = val;
    types[key] = type;
  }


  void dataframe_t::set_column(std::string key,
                               std::vector<flexible_type>&& val,
                               flex_type_enum type) {
    if (!values.count(key)) {
      names.push_back(key);
    }
    values[key] = std::move(val);
    types[key] = type;
  }

  void dataframe_t::remove_column(std::string key) {
    size_t colid = -1;
    for (size_t i = 0; i < ncols(); ++i) {
      if (names[i] == key) {
        colid = i;
        break;
      }
    }
    if (colid == (size_t)-1) {
      return;
    } else {
      names.erase(names.begin() + colid);
      types.erase(types.find(key));
      values.erase(values.find(key));
    }
  }

  void dataframe_t::read_csv(const std::string& path, char delimiter, bool use_header) {
      logstream(LOG_EMPH) << "Loading dataframe from " << path << std::endl;
      std::string line;
      timer ti;

      general_ifstream fin(path);

      if (!fin.good()) {
        log_and_throw("Fail reading " + path);
      }

      size_t ncols(0);
      size_t nrows(0);
      std::vector<std::string> column_names;
      std::vector< std::vector<flexible_type> > column_values;
      std::vector<flex_type_enum> column_types;
      boost::escaped_list_separator<char> sep('\\', delimiter, '\"');

      // Read until the first non empty line
      std::vector<std::string> first_line;
      while (getline(fin, line)) {
        if (!line.empty())
          break;
      };
      // Return here if fail reading the header.
      if (line.empty()) {
        logstream(LOG_WARNING) << "Ignore empty file " << path << std::endl;
        return;
      }
      // Parse the first line to get number of columns.
      boost::tokenizer<boost::escaped_list_separator<char> > tokens(line, sep);
      // std::cerr << "parsing header ";
      for (auto& tok : tokens) {
        std::string val(tok);
        boost::trim(val);
        first_line.push_back(val);
        ++ncols;
        // std::cerr << val << "\t";
      }
      // std::cerr << "\n";

      column_values.resize(ncols);
      if (use_header) {
        // first line is header
        column_names = first_line;
      } else {
        // use Xi as column name and insert the first row
        for (size_t i = 0; i < ncols; ++i) {
          column_names.push_back("X"+boost::lexical_cast<std::string>(i+1));
          column_values[i].push_back(parse_flexible_type(first_line[i]));
        }
        ++nrows;
      }

      // Parse body, set column_values
      timer local_ti;
      while(fin.good()) {
        getline(fin, line);
        // std::cerr << "line: " << line << "\n";
        if (line.empty())
          break;

        boost::tokenizer<boost::escaped_list_separator<char> > tokens(line, sep);
        std::vector<flexible_type> tmp;
        for (auto& tok : tokens) {
          tmp.push_back(parse_flexible_type(tok));
        }
        if (tmp.size() != ncols) {
          std::cerr << "ignore line: " << line << ". Unexpected number of columns.\n";
          continue;
        };
        for (size_t j = 0 ; j < ncols; ++j) {
          column_values[j].push_back(tmp[j]);
        }
        ++nrows;
        if (local_ti.current_time() > 5.0) {
          logstream(LOG_INFO) << nrows << " Lines read" << std::endl;
          local_ti.start();
        }
      }
      logstream(LOG_EMPH) << "Finish parsing file. ncol = "
                          << ncols << " nrow = " << nrows << "\n";
      fin.close();

      if (nrows == 0) {
        log_and_throw("File " + path + " has no data.");
      }

      // Type inference and unification, set column_types;
      parallel_for(0, ncols,
                   [&](size_t i) {
          std::vector<flexible_type>& values = column_values[i];
          flex_type_enum t = values[0].get_type();
          bool type_changed;

          for (size_t j = 1; j < nrows; ++j) {
            if (!flex_type_is_convertible(values[j].get_type(), t)) {
              t = values[j].get_type();
              type_changed = true;
            }
            if (t == flex_type_enum::STRING) {
              break;
            }
          }

          if (t != flex_type_enum::INTEGER && t != flex_type_enum::FLOAT && t != flex_type_enum::STRING) {
            log_and_throw("Unsupported column type " + std::string(flex_type_enum_to_name(t)) + " at column " + column_names[i]);
          }
          column_types.push_back(t);
          std::cerr << "column " << i << " is type " <<  flex_type_enum_to_name(t) << "\n";

          // type unification
          if (type_changed) {
            std::cerr << "cast column " << i << " to " <<  flex_type_enum_to_name(t) << "\n";
            switch(t) {
             case flex_type_enum::INTEGER:
               for (size_t j = 0; j < nrows; ++j) {
                 if (values[j].get_type() != flex_type_enum::UNDEFINED) {
                   values[j] = (flex_int)values[j];
                 }
               }
               break;
             case flex_type_enum::FLOAT:
               for (size_t j = 0; j < nrows; ++j) {
                 if (values[j].get_type() != flex_type_enum::UNDEFINED) {
                   values[j] = (flex_float)values[j];
                 }
               }
               break;
             case flex_type_enum::STRING:
               for (size_t j = 0; j < nrows; ++j) {
                 if (values[j].get_type() != flex_type_enum::UNDEFINED) {
                   values[j] = (flex_string)values[j];
                 }
               }
               break;
             default:
               log_and_throw("TypeError. Attempt to unify column " +
                     column_names[i] + " to type: " + flex_type_enum_to_name(t));
            }
          } // end if type changed
      });

      // construct dataframe
      names.swap(column_names);
      for (size_t i = 0 ;i < ncols; ++i) {
        types[names[i]] = column_types[i];
        values[names[i]].swap(column_values[i]);
      }

      std::stringstream ss;
      ss << "Finish loading dataframe in " << ti.current_time() << "secs \n";
      for (auto& s : types) {
          ss << s.first << ":" << flex_type_enum_to_name(s.second) << "\t";
      }
      logstream(LOG_EMPH) << ss.str() << std::endl;

      // uncomment to debug
      // print();
  }

/**************************************************************************/
/*                                                                        */
/*                dataframe_row_iterator static functions                 */
/*                                                                        */
/**************************************************************************/

dataframe_row_iterator dataframe_row_iterator::begin(const dataframe_t& dt) {
  dataframe_row_iterator iter;
  iter.names = dt.names;
  for(auto& key: dt.names) {
    iter.types.push_back(dt.types.at(key));
    iter.iterators.push_back({dt.values.at(key).begin(), dt.values.at(key).end()});
  }
  // get the number of rows by getting the length of the first column
  if (iter.iterators.size() > 0) {
    iter.num_rows = std::distance(iter.iterators[0].first,
                                  iter.iterators[0].second);
  } else {
    iter.num_rows = 0;
  }
  iter.num_columns = iter.names.size();
  iter.current_column = 0;
  iter.current_row = 0;
  iter.idx = 0;
  iter.num_el = iter.num_rows * iter.num_columns;
  return iter;
}


dataframe_row_iterator dataframe_row_iterator::end(const dataframe_t& dt) {
  // use begin to fill out all the basic information of the iterator
  dataframe_row_iterator iter = dataframe_row_iterator::begin(dt);
  // move all the indexes to the end of the dataframe
  iter.current_column = iter.num_columns;
  iter.current_row = iter.num_rows;
  iter.idx = iter.num_el;
  for (size_t i = 0;i < iter.iterators.size(); ++i) {
    iter.iterators[i].first = iter.iterators[i].second;
  }
  return iter;
}

void dataframe_row_iterator::skip_rows(size_t num_rows_to_skip) {
  // the obvious row() + num_rows_to_skip >= row_size() does not work since
  // a large num_rows_to_skip may result in a numeric overflow
  if (row_size() - row() <= num_rows_to_skip) {
    // move all the indexes to the end of the dataframe
    current_column = num_columns;
    current_row = num_rows;
    idx = num_el;
    for (size_t i = 0;i < iterators.size(); ++i) {
      iterators[i].first = iterators[i].second;
    }
  } else {
    // advance the row index by num_rows_to_skip
    current_row += num_rows_to_skip;
    idx += num_columns * num_rows_to_skip;
    for (size_t i = 0;i < iterators.size(); ++i) {
      iterators[i].first += num_rows_to_skip;
    }
  }

}


/**************************************************************************/
/*                                                                        */
/*                       Parallel Dataframe Iterate                       */
/*                                                                        */
/**************************************************************************/
void parallel_dataframe_iterate(const dataframe_t& df,
                                std::function<void(dataframe_row_iterator& iter,
                                                   size_t startrow,
                                                   size_t endrow)> partialrowfn) {
  in_parallel([&](size_t thread_id, size_t num_threads) {
    // split the rows into groups of rows_per_thread
    size_t rows_per_thread = df.nrows() / num_threads;
    dataframe_row_iterator thlocal_iter = dataframe_row_iterator::begin(df);

    // each thread is going to cover rows_per_thread rows, except for the
    // last thread which must cover all the way to the end
    size_t start_row = rows_per_thread * (thread_id);
    size_t last_row = rows_per_thread * (thread_id + 1); // technically one
                                                            // past the last row
    thlocal_iter.skip_rows(start_row);
    if (thread_id == num_threads - 1) last_row = df.nrows();
    partialrowfn(thlocal_iter, start_row, last_row);
  });
}

} // namespace turi
