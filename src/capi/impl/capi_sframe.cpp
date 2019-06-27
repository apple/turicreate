#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>

#include <map>
#include <sstream>
#include <string>

#include <core/export.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>

extern "C" {

EXPORT tc_sframe* tc_sframe_create_empty(tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_sframe();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_create_copy(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  tc_sframe* ret = new_tc_sframe();
  ret->value = turi::gl_sframe(sf->value);
  return ret;

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_load(const char* url, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_sframe(url);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT void tc_sframe_save(const tc_sframe* sf, const char* url,
                           tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "tc_sframe");

  sf->value.save(url, "binary");

  ERROR_HANDLE_END(error);
}

// Adds the column to the sframe
EXPORT void tc_sframe_add_column(tc_sframe* sf, const char* column_name,
                                 const tc_sarray* sa, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "tc_sframe");

  sf->value.add_column(sa->value, column_name);

  ERROR_HANDLE_END(error);
}

// Removes the column from the sframe
EXPORT void tc_sframe_remove_column(tc_sframe* sf, const char* column_name,
                                    tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "tc_sframe");

  sf->value.remove_column(column_name);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sarray* tc_sframe_extract_column_by_name(const tc_sframe* sf,
                                                   const char* column_name,
                                                   tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_sarray(sf->value.select_column(column_name));

  ERROR_HANDLE_END(error, NULL);
}

// Wrap the printing.  Returns a string flexible type.
EXPORT tc_flexible_type* tc_sframe_text_summary(const tc_sframe* sf,
                                                tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if (sf == NULL) {
    set_error(error, "SFrame passed in to summarize is null.");
    return NULL;
  }

  std::ostringstream ss;
  ss << sf->value;

  return new_tc_flexible_type(ss.str());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_sframe_num_rows(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if (sf == NULL) {
    set_error(error, "SFrame passed in to num_rows is null.");
    return NULL;
  }

  return sf->value.size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_sframe_num_columns(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if (sf == NULL) {
    set_error(error, "SFrame passed in to num_columns is null.");
    return NULL;
  }

  return sf->value.num_columns();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_list* tc_sframe_column_names(const tc_sframe* sf,
                                            tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if (sf == NULL) {
    set_error(error, "SFrame passed in to summarize is null.");
    return NULL;
  }

  std::vector<std::string> column_names = sf->value.column_names();

  return new_tc_flex_list(
      turi::flex_list(column_names.begin(), column_names.end()));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_join_on_multiple_columns(
    tc_sframe* left, tc_sframe* right,
    tc_flex_list* join_columns,
    const char* how, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, left, "left tc_sframe", NULL);
  CHECK_NOT_NULL(error, right, "right tc_sframe", NULL);

  std::vector<std::string> join_columns_transform;

  for (const turi::flexible_type& elem : join_columns->value) {
    if (elem.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "Contains a non-string column name.");
      return NULL;
    }
    join_columns_transform.push_back(elem.get<turi::flex_string>());
  }
  
  return new_tc_sframe(
      left->value.join(right->value, join_columns_transform, how));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_join_on_single_column(tc_sframe* left,
                                                  tc_sframe* right,
                                                  const char* column,
                                                  const char* how,
                                                  tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, left, "left tc_sframe", NULL);
  CHECK_NOT_NULL(error, right, "right tc_sframe", NULL);
  CHECK_NOT_NULL(error, column, "column", NULL);
  CHECK_NOT_NULL(error, how, "how", NULL);

  tc_flex_list *join_columns = tc_flex_list_create(error);
  if (join_columns == NULL) { assert(false); return NULL; } // error should be populated already
  tc_flexible_type *flex_column = tc_ft_create_from_cstring(column, error);
  if (flex_column == NULL) { assert(false); return NULL; } // error should be populated already
  uint64_t added = tc_flex_list_add_element(join_columns, flex_column, error);
  if (added == uint64_t(-1)) { assert(false); return NULL; } // error should be populated already

  tc_sframe *ret = tc_sframe_join_on_multiple_columns(left, right, join_columns, how, error);
  tc_release(flex_column);
  tc_release(join_columns);
  return ret;

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sframe* tc_sframe_read_csv(const char* url,
                                     const tc_parameters* params,
                                     tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  tc_sframe* ret = new_tc_sframe();
  std::map<std::string, turi::flexible_type> config;
  std::map<std::string, turi::flex_type_enum> column_type_hints; 

  turi::variant_map_type params_copy;

  if (params != nullptr) {
    params_copy = params->value;
  }

  if (!params_copy.empty()) {
    // header: int
    auto it = params_copy.find("header");
    if (it != params_copy.end()) {
      int64_t header = turi::variant_get_ref<turi::flexible_type>(it->second)
                           .to<turi::flex_int>();
      config.insert({"use_header", header});
      params_copy.erase(it);
    }

    // delimiter: string
    it = params_copy.find("delimiter");
    if (it != params_copy.end()) {
      std::string delimiter =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_string>();
      config.insert({"delimiter", delimiter});
      params_copy.erase(it);
    }

    // comment_char: string
    it = params_copy.find("comment_char");
    if (it != params_copy.end()) {
      std::string comment_char =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_string>();
      config.insert({"comment_char", comment_char});
      params_copy.erase(it);
    }

    // escape_char: string
    it = params_copy.find("escape_char");
    if (it != params_copy.end()) {
      std::string escape_char =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_string>();
      config.insert({"escape_char", escape_char});
      params_copy.erase(it);
    }

    // quote_char: string
    it = params_copy.find("quote_char");
    if (it != params_copy.end()) {
      std::string quote_char =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_string>();
      config.insert({"quote_char", quote_char});
      params_copy.erase(it);
    }

    // error_bad_lines: int
    it = params_copy.find("error_bad_lines");
    if (it != params_copy.end()) {
      int64_t error_bad_lines =
          !(turi::variant_get_ref<turi::flexible_type>(it->second).is_zero());
      config.insert({"continue_on_failure", !error_bad_lines});
      params_copy.erase(it);
    }

    // double_quote: int
    it = params_copy.find("double_quote");
    if (it != params_copy.end()) {
      int64_t double_quote =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_int>();
      config.insert({"double_quote", double_quote});
      params_copy.erase(it);
    }

    // skip_initial_space: int
    it = params_copy.find("skip_initial_space");
    if (it != params_copy.end()) {
      int64_t skip_initial_space =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_int>();
      config.insert({"skip_initial_space", skip_initial_space});
      params_copy.erase(it);
    }

    // column_type_hints: flex_dict<string, flexible_type>
    it = params_copy.find("column_type_hints");

    if (it != params_copy.end()) {
      turi::flex_dict ft_column_type_hints =
          turi::variant_get_value<turi::flex_dict>(it->second);

      for (const auto& entry : ft_column_type_hints) {
        if (entry.first.get_type() != turi::flex_type_enum::STRING) {
          log_and_throw(
              "Invalid input to column_type_hints optional parameter: "
              "Keys must be a column names. ");
        }

        if (entry.second.get_type() == turi::flex_type_enum::STRING) {
          column_type_hints[entry.first] = turi::flex_type_enum_from_name(
              entry.second.get<turi::flex_string>());
        } else if (entry.second.get_type() == turi::flex_type_enum::INTEGER) {
          column_type_hints[entry.first] = static_cast<turi::flex_type_enum>(
              entry.second.get<turi::flex_int>());
        } else {
          log_and_throw(
              "Invalid input to column_type_hints optional parameter: "
              "requires "
              "a dictionary of column names to strings/enums giving column "
              "types.");
        }
      }

      params_copy.erase(it);
    }

    // na_values: flex_list<flexible_type>
    it = params_copy.find("na_values");
    if (it != params_copy.end()) {
      std::vector<turi::flexible_type> ft_na_values =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_list>();
      for (auto iter = ft_na_values.begin(); iter != ft_na_values.end();
           ++iter) {
        if ((*iter).get_type() != turi::flex_type_enum::STRING) {
          log_and_throw(
              "Invalid input to na_values optional parameter: requires a "
              "flex_list of strings");
          return NULL;
        }
      }
      config.insert({"na_values", ft_na_values});
      params_copy.erase(it);
    }

    // line_terminator: string
    it = params_copy.find("line_terminator");
    if (it != params_copy.end()) {
      std::string line_terminator =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_string>();
      config.insert({"line_terminator", line_terminator});
      params_copy.erase(it);
    }

    // usecols: flex_list<string>
    it = params_copy.find("output_columns");
    if (it != params_copy.end()) {
      std::vector<turi::flexible_type> ft_usecols =
          turi::variant_get_ref<turi::flexible_type>(it->second)
              .to<turi::flex_list>();
      for (auto iter = ft_usecols.begin(); iter != ft_usecols.end(); ++iter) {
        if ((*iter).get_type() != turi::flex_type_enum::STRING) {
          log_and_throw(
              "Invalid input to usecols optional parameter: requires a "
              "flex_list of strings");
          return NULL;
        }
      }
      config.insert({"output_columns", ft_usecols});
      params_copy.erase(it);
    }

    // nrows: int
    it = params_copy.find("row_limit");
    if (it != params_copy.end()) {
      int64_t nrows = turi::variant_get_ref<turi::flexible_type>(it->second)
                          .to<turi::flex_int>();
      config.insert({"row_limit", nrows});
      params_copy.erase(it);
    }

    // skiprows: int
    it = params_copy.find("skip_rows");
    if (it != params_copy.end()) {
      int64_t skiprows = turi::variant_get_ref<turi::flexible_type>(it->second)
                             .to<turi::flex_int>();
      config.insert({"skip_rows", skiprows});
      params_copy.erase(it);
    }

    // verbose: int
    it = params_copy.find("verbose");
    if (it != params_copy.end()) {
      int64_t verbose = turi::variant_get_ref<turi::flexible_type>(it->second)
                            .to<turi::flex_int>();
      config.insert({"verbose", verbose});
      params_copy.erase(it);
    }

    if(!params_copy.empty()) {
      std::ostringstream ss;

      ss << "Error: csv options ";
      while(!params_copy.empty()) {
        auto it = params_copy.begin();
        ss << it->first;
        params_copy.erase(it);
        if(!params_copy.empty()) {
          ss << ", ";
        }
      }

      ss << " not recognized.  Options are header [0/1], delimeter [string], comment_char [string], escape_char [string], quote_char [string], error_bad_lines [0/1], skip_initial_space [0/1], column_type_hints [list], na_values [any], line_terminator [string], usecols [list], nrows [int], skiprows [int], verbose [0/1].";

      for(const auto& p : params_copy) {
        ss << p.first;
      }

      log_and_throw(ss.str());
    }
  }

  std::cout << "column_type_hints = " << column_type_hints << std::endl; 
  ret->value.construct_from_csvs(url, config, column_type_hints);

  return ret;

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_read_json_lines(const char* url, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  tc_sframe* ret = new_tc_sframe();
  turi::csv_parsing_config_map config;
  turi::str_flex_type_map column_type_hints;

  config.insert({"header", 0});

  ret->value.construct_from_csvs(url, config, column_type_hints);
  if (ret->value.num_columns() != 1) {
    log_and_throw("Input JSON not of expected format");
    return NULL;
  }
  if (ret->value["X1"].dtype() == turi::flex_type_enum::DICT) {
    return (new_tc_sframe(ret->value.unpack("X1", "")));
  } else {
    return ret;
  }

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_read_json(const char* url, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  turi::gl_sframe sf;
  sf["X1"] = turi::gl_sarray::read_json(url);

  DASSERT_EQ(sf.num_columns(), 1);

  if(sf["X1"].empty()) {
    return new_tc_sframe(turi::gl_sframe());
  }

  sf = sf.unpack("X1", "");

  return new_tc_sframe(sf);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_export(const tc_sframe* sf, const char* url,
                            const char* format, const tc_parameters* options, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  // TODO: add options.
  sf->value.save(url, format);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_head(const tc_sframe* sf, size_t n,
                                 tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.head(n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_tail(const tc_sframe* sf, size_t n,
                                 tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.tail(n));

  ERROR_HANDLE_END(error, NULL);
}

// Return the name of a particular column.
EXPORT const char* tc_sframe_column_name(const tc_sframe* sf,
                                         size_t column_index,
                                         tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return sf->value.column_name(column_index).c_str();

  ERROR_HANDLE_END(error, NULL);
}

// Return the type of a particular column.
EXPORT tc_ft_type_enum tc_sframe_column_type(const tc_sframe* sf,
                                             const char* column_name,
                                             tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", FT_TYPE_UNDEFINED);

  return static_cast<tc_ft_type_enum>(sf->value[column_name].dtype());

  ERROR_HANDLE_END(error, FT_TYPE_UNDEFINED);
}

EXPORT void tc_sframe_random_split(const tc_sframe* sf, double fraction,
                                   size_t seed, const tc_sframe** left,
                                   const tc_sframe** right, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe");

  auto frames = sf->value.random_split(fraction, seed);

  *left = new_tc_sframe(frames.first);
  *right = new_tc_sframe(frames.second);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_append(const tc_sframe* top,
                                   const tc_sframe* bottom, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, top, "top sframe", tc_sframe_create_copy(bottom, error));
  CHECK_NOT_NULL(error, bottom, "bottom sframe", tc_sframe_create_copy(top, error));

  return new_tc_sframe(top->value.append(bottom->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT bool tc_sframe_is_materialized(const tc_sframe* src, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return src->value.is_materialized();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT bool tc_sframe_size_is_known(const tc_sframe* src, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return src->value.has_size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_save_reference(const tc_sframe* src, const char* path,
                                     tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sframe");

  src->value.save(path);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_materialize(tc_sframe* src, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sframe");

  src->value.materialize();

  ERROR_HANDLE_END(error);
}

EXPORT bool tc_sframe_contains_column(const tc_sframe* src,
                                      const char* col_name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return src->value.contains_column(col_name);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_sample(const tc_sframe* src, double fraction,
                                   uint64_t seed, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return new_tc_sframe(src->value.sample(fraction, seed));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_replace_add_column(tc_sframe* sf, const char* name,
                                         const tc_sarray* new_column,
                                         tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, new_column, "sarray");

  sf->value.replace_add_column(new_column->value, name);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_add_constant_column(tc_sframe* sf,
                                          const char* column_name,
                                          const tc_flexible_type* value,
                                          tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, value, "tc_flexible_type");

  sf->value.add_column(value->value, column_name);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_add_columns(tc_sframe* sf, const tc_sframe* other,
                                  tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, other, "sframe");

  sf->value.add_columns(other->value);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_topk(const tc_sframe* src, const char* column_name,
                                 uint64_t k, bool reverse, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return new_tc_sframe(src->value.topk(column_name, k, reverse));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_swap_columns(tc_sframe* sf, const char* column_1,
                                   const char* column_2, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe");

  sf->value.swap_columns(column_1, column_2);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_rename_column(tc_sframe* sf, const char* old_name,
                                    const char* new_name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe");

  sf->value.rename({{std::string(old_name), std::string(new_name)}});

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_rename_columns(tc_sframe* sf,
                                     const tc_flex_dict* name_mapping,
                                     tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, name_mapping, "tc_flex_dict");

  std::map<std::string, std::string> m;

  for (auto p : name_mapping->value) {
    if (p.first.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "entries are not of type str");
    }

    if (p.second.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "entries are not of type str");
    }

    m.insert(std::pair<std::string, std::string>(
        p.first.get<turi::flex_string>(), p.second.get<turi::flex_string>()));
  }

  sf->value.rename(m);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_filter_by(const tc_sframe* sf,
                                      const tc_sarray* values,
                                      const char* column_name, bool exclude,
                                      tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, values, "sarray", NULL);

  return new_tc_sframe(
      sf->value.filter_by(values->value, column_name, exclude));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_pack_columns_vector(
    const tc_sframe* sf, const tc_flex_list* columns, const char* column_name,
    tc_ft_type_enum type, tc_flexible_type* value, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  for (const turi::flexible_type& i : columns->value) {
    if (i.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "Contains a non-string column.");
      return NULL;
    }
  }

  std::vector<std::string> column_transform(columns->value.begin(),
                                            columns->value.end());

  return new_tc_sframe(sf->value.pack_columns(
      column_transform, column_name, static_cast<turi::flex_type_enum>(type),
      value->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_pack_columns_string(
    const tc_sframe* sf, const char* column_prefix, const char* column_name,
    tc_ft_type_enum type, tc_flexible_type* value, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.pack_columns(
      column_prefix, column_name, static_cast<turi::flex_type_enum>(type),
      value->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_split_datetime(const tc_sframe* sf,
                                           const char* expand_column,
                                           const char* column_prefix,
                                           const tc_flex_list* limit,
                                           bool tzone, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  for (const turi::flexible_type& i : limit->value) {
    if (i.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "Element of limit is not of type str");
      return NULL;
    }
  }

  std::vector<std::string> limit_transform(limit->value.begin(),
                                           limit->value.end());

  return new_tc_sframe(sf->value.split_datetime(expand_column, column_prefix,
                                                limit_transform, tzone));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unpack(const tc_sframe* sf,
                                   const char* unpack_column,
                                   tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.unpack(unpack_column));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unpack_detailed(
    const tc_sframe* sf, const char* unpack_column, const char* column_prefix,
    const tc_flex_enum_list* type, tc_flexible_type* value,
    const tc_flex_list* limit, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  std::vector<turi::flex_type_enum> type_transform;

  for (const turi::flex_type_enum& i : type->value) {
    type_transform.push_back(i);
  }

  return new_tc_sframe(sf->value.unpack(unpack_column, column_prefix,
                                        type_transform, value->value,
                                        limit->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_stack(const tc_sframe* sf, const char* column_name,
                                  tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.stack(column_name, column_name));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_stack_and_rename(const tc_sframe* sf,
                                             const char* column_name,
                                             const char* new_column_name,
                                             bool drop_na, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.stack(column_name, new_column_name, drop_na));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unstack(const tc_sframe* sf, const char* column,
                                    const char* new_column_name,
                                    tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.unstack(column, new_column_name));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unstack_vector(const tc_sframe* sf,
                                           const tc_flex_list* columns,
                                           const char* new_column_name,
                                           tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  for (const turi::flexible_type& i : columns->value) {
    if (i.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "Contains a non-string column.");
      return NULL;
    }
  }

  std::vector<std::string> columns_transform(columns->value.begin(),
                                             columns->value.end());

  return new_tc_sframe(sf->value.unstack(columns_transform, new_column_name));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unique(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.unique());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_sort_single_column(const tc_sframe* sf,
                                               const char* column,
                                               bool ascending,
                                               tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.sort(column, ascending));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_drop_na(const tc_sframe* sf,
                                    const tc_flex_list* columns,
                                    const char* how, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  std::vector<std::string> columns_transform;

  for (const turi::flexible_type& i : columns->value) {
    if (i.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "Contains a non-string column.");
      return NULL;
    }

    columns_transform.push_back(i.get<turi::flex_string>());
  }

  return new_tc_sframe(sf->value.dropna(columns_transform, how));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_sort_multiple_columns(const tc_sframe* sf,
                                                  const tc_flex_list* columns,
                                                  bool ascending,
                                                  tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  std::vector<std::string> columns_transform;

  for (const turi::flexible_type& i : columns->value) {
    if (i.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "Contains a non-string column.");
      return NULL;
    }

    columns_transform.push_back(i.get<turi::flex_string>());
  }

  return new_tc_sframe(sf->value.sort(columns_transform, ascending));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_slice(const tc_sframe* sf, const int64_t start,
                                  const int64_t slice, const int64_t end,
                                  tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(
      sf->value[{static_cast<long long>(start), static_cast<long long>(slice),
                 static_cast<long long>(end)}]);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_list* tc_sframe_extract_row(const tc_sframe* sf,
                                           const uint64_t row,
                                           tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_flex_list(sf->value[row]);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_replace_na(const tc_sframe* data,
                                       const char* column,
                                       const tc_flexible_type* value,
                                       tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, data, "sframe", NULL);
  CHECK_NOT_NULL(error, value, "flexible_type", NULL);

  return new_tc_sframe(data->value.fillna(column, value->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_groupby_aggregator* tc_groupby_aggregator_create(tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_groupby_aggregator();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_groupby_aggregator_add_parameterized_aggregator(
    tc_groupby_aggregator* gb, const char* dest_column,
    const tc_parameters* param, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();


  // PLACEHOLDER: TODO
  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_apply_mask(const tc_sframe* sf, const tc_sarray* mask, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "tc_sframe", NULL);
  CHECK_NOT_NULL(error, mask, "tc_sarray", NULL);

  return new_tc_sframe(sf->value[mask->value]);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT void tc_groupby_aggregator_add_simple_aggregator(
    tc_groupby_aggregator* gb, const char* agg_op, const char* dest_column,
    const char* src_column, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  typedef decltype(gb->value) agg_type;

#define AGG_OP(name, op_name)\
   {name, [](agg_type& gb, const char* dest, const char* src) { gb.emplace(dest, turi::aggregate::op_name(src)); } }

  static std::map<std::string, std::function<void(agg_type&, const char*, const char*)> > agg_map =
  {{"count", [](agg_type& gb, const char* dest, const char* src) { gb.emplace(dest, turi::aggregate::COUNT()); } },
    AGG_OP("sum", SUM),
    AGG_OP("max", MAX),
    AGG_OP("min", MIN),
    AGG_OP("avg", AVG),
    AGG_OP("var", VAR),
    AGG_OP("std", STD),
    AGG_OP("stdv", STDV),
    AGG_OP("select_one", SELECT_ONE),
    AGG_OP("count_distinct", COUNT_DISTINCT),
    AGG_OP("concat", CONCAT)
  };

  auto it = agg_map.find(agg_op);

  if(it == agg_map.end()) {

    std::ostringstream ss;
    ss << "Aggregation " << agg_op << " not recognized. "
       << "Available aggregation operators are ";

    for (const auto& p : agg_map) {
      ss << p.first << " ";
    }
    ss << ".";

    throw std::invalid_argument(ss.str());
  }

  it->second(gb->value, dest_column, src_column);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_groupby_aggregator_add_concat_two_columns(
    tc_groupby_aggregator* gb, const char* dest_column, const char* key,
    const char* val, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, gb, "groupby_aggregator");

  gb->value.insert({dest_column, turi::aggregate::CONCAT(key, val)});

  ERROR_HANDLE_END(error);
}

EXPORT void tc_groupby_aggregator_add_quantile(tc_groupby_aggregator* gb,
                                               const char* dest_column,
                                               const char* src_column,
                                               const double quantile,
                                               tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, gb, "groupby_aggregator");

  gb->value.insert(
      {dest_column, turi::aggregate::QUANTILE(src_column, quantile)});

  ERROR_HANDLE_END(error);
}

EXPORT void tc_groupby_aggregator_add_quantiles(tc_groupby_aggregator* gb,
                                                const char* dest_column,
                                                const char* src_column,
                                                const tc_flex_list* quantiles,
                                                tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, gb, "groupby_aggregator");
  CHECK_NOT_NULL(error, quantiles, "flex_list");

  std::vector<double> quantiles_transform;

  for (const turi::flexible_type& elem : quantiles->value) {
    if (elem.get_type() != turi::flex_type_enum::FLOAT) {
      set_error(error, "Contains a non-float quantile.");
      return;
    }
    quantiles_transform.push_back(elem.get<turi::flex_float>());
  }

  gb->value.insert({dest_column, turi::aggregate::QUANTILE(
                                     src_column, quantiles_transform)});

  ERROR_HANDLE_END(error);
}

EXPORT void tc_groupby_aggregator_add_argmax(tc_groupby_aggregator* gb,
                                             const char* dest_column,
                                             const char* agg, const char* out,
                                             tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, gb, "groupby_aggregator");

  gb->value.insert({dest_column, turi::aggregate::ARGMAX(agg, out)});

  ERROR_HANDLE_END(error);
}

EXPORT void tc_groupby_aggregator_add_argmin(tc_groupby_aggregator* gb,
                                             const char* dest_column,
                                             const char* agg, const char* out,
                                             tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, gb, "groupby_aggregator");

  gb->value.insert({dest_column, turi::aggregate::ARGMIN(agg, out)});

  ERROR_HANDLE_END(error);
}


EXPORT tc_sframe* tc_sframe_group_by(const tc_sframe* sf,
                                     const tc_flex_list* column_list,
                                     const tc_groupby_aggregator* gb,
                                     tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, column_list, "string_list", NULL);
  CHECK_NOT_NULL(error, gb, "groupby_aggregator", NULL);

  std::vector<std::string> column_list_transform;

  for (const turi::flexible_type& elem : column_list->value) {
    if (elem.get_type() != turi::flex_type_enum::STRING) {
      set_error(error, "Contains a non-string column.");
      return NULL;
    }
    column_list_transform.push_back(elem.get<turi::flex_string>());
  }

  return new_tc_sframe((sf->value).groupby(column_list_transform, gb->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sframe_apply(
    const tc_sframe* data,
    tc_flexible_type* (*callback)(
        tc_flex_list* row, void* context, tc_error** error),
    void (*context_release_callback)(void* context),
    void* context,
    tc_ft_type_enum type,
    tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, data, "SFrame passed in is null.", nullptr);
  CHECK_NOT_NULL(error, callback, "Callback function passed in is null.",
                nullptr);
  if (context != nullptr) {
    CHECK_NOT_NULL(error, context_release_callback,
                   "Context release function passed in is null.", nullptr);
  }

  // Use shared_ptr to ensure that the user data is released exactly once, after
  // all copies of the lambda below have been destroyed.
  std::shared_ptr<void> shared_context(context, context_release_callback);
  auto wrapper = [callback, shared_context](const turi::sframe_rows::row& row) {
    tc_error* error = nullptr;

    // Invoke the user callback.
    tc_flex_list in;
    in.value = row;  // Converts row to std::vector<flexible_type>

    tc_flexible_type* out;
    out = callback(&in, shared_context.get(), &error);

    // Propagate errors from user code up to whatever C-API throw-catch block
    // (hopefully) encloses the call that triggered this wrapper's invocation.
    if (error != nullptr) {
      std::string message = std::move(error->value);
      tc_release(&error);
      if (out != nullptr) tc_release(out);
      throw message;
    }
    if (out == nullptr) {
      throw std::string("Callback provided to tc_sframe_apply returned null "
                        "without setting error");
    }

    // Return the value that the callback produced.
    turi::flexible_type ret = std::move(out->value);
    tc_release(out);
    return ret;
  };

  return new_tc_sarray(data->value.apply(std::move(wrapper),
                                         turi::flex_type_enum(type)));

  ERROR_HANDLE_END(error, nullptr);
}

}
