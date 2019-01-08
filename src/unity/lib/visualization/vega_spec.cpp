/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "vega_spec.hpp"

#include <logger/assertions.hpp>
#include <flexible_type/string_escape.hpp>

// generated include files for vega spec JSON
#include <unity/lib/visualization/vega_spec/boxes_and_whiskers.h>
#include <unity/lib/visualization/vega_spec/categorical.h>
#include <unity/lib/visualization/vega_spec/categorical_heatmap.h>
#include <unity/lib/visualization/vega_spec/heatmap.h>
#include <unity/lib/visualization/vega_spec/histogram.h>
#include <unity/lib/visualization/vega_spec/scatter.h>
#include <unity/lib/visualization/vega_spec/summary_view.h>

// use boost::format for string replacement in vega json
// and boost::replace_all to get rid of newlines
#include <boost/algorithm/string/replace.hpp>
#include <boost/format.hpp>

namespace turi {
namespace visualization {
std::string escape_string(const std::string& str) {
  std::string ret;
  size_t ret_len;
  ::turi::escape_string(str, '\\', true /* use_escape_char */, '\"', true /* use_quote_char */, false /* double_quote */, ret, ret_len);

  // ::turi::escape_string may yield an std::string padded with null terminators, and ret_len represents the true length.
  // truncate to the ret_len length.
  ret.resize(ret_len);
  DASSERT_EQ(ret.size(), strlen(ret.c_str()));

  return ret;
}

std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

std::string extra_label_escape(const std::string& str){
  std::string escaped_string = escape_string(str);
  escaped_string = replace_all(escaped_string, std::string("\\n"), std::string("\\\\n"));
  escaped_string = replace_all(escaped_string, std::string("\\t"), std::string("\\\\t"));
  escaped_string = replace_all(escaped_string, std::string("\\b"), std::string("\\\\b"));
  escaped_string = replace_all(escaped_string, std::string("\\r"), std::string("\\\\r"));

  return escaped_string;
}

/*
 * Prepares a raw JSON format string
 * by doing the following:
 * 1. Strips all newlines.
 */
static std::string make_format_string(unsigned char *raw_format_str_ptr,
                                      size_t raw_format_str_len) {
  auto raw_format_str = std::string(
    reinterpret_cast<char *>(raw_format_str_ptr),
    raw_format_str_len);

  boost::replace_all(raw_format_str, "\n", "");
  return raw_format_str;
}

static std::string label_or_default(const flexible_type& label,
                                    const std::string& _default) {
  if (label == FLEX_UNDEFINED) {
    // undefined should render as null in JSON
    return "null";

  } else if (label == "__TURI_DEFAULT_LABEL") {
    // substitute the default label
    return extra_label_escape(_default);

  } else {
    // user-provided label should render with quotes/escaping
    return extra_label_escape(label.get<flex_string>());
  }
}

static std::string label_or_null(const flexible_type& label) {
  if (label == FLEX_UNDEFINED || label == "__TURI_DEFAULT_LABEL") {
    // undefined/not provided should render as null in JSON
    return "null";

  } else {
    // user-provided label should render with quotes/escaping
    return extra_label_escape(label.get<flex_string>());
  }

}

EXPORT std::string histogram_spec(const flexible_type& _title,
                                  const flexible_type& _xlabel,
                                  const flexible_type& _ylabel,
                                  flex_type_enum dtype) {
  static std::string default_title = std::string("Distribution of Values [") +
                                     flex_type_enum_to_name(dtype) +
                                     "]";
  flexible_type title = label_or_default(_title, default_title);
  flexible_type xlabel = label_or_default(_xlabel, "Count");
  flexible_type ylabel = label_or_default(_ylabel, "Values");

  auto format_string = make_format_string(vega_spec_histogram_json, vega_spec_histogram_json_len);
  auto formatted = boost::format(format_string) %
    title %
    xlabel %
    ylabel;
  return formatted.str();
}

EXPORT std::string categorical_spec(size_t length_list,
                                    const flexible_type& _title,
                                    const flexible_type& _xlabel,
                                    const flexible_type& _ylabel,
                                    flex_type_enum dtype) {
  static std::string default_title = std::string("Distribution of Values [") +
                                     flex_type_enum_to_name(dtype) +
                                     "]";

  flexible_type title = label_or_default(_title, default_title);
  flexible_type xlabel = label_or_default(_xlabel, "Values");
  flexible_type ylabel = label_or_default(_ylabel, "Count");

  size_t height = static_cast<size_t>(static_cast<double>(length_list) * 25.0 + 160.0);
  auto format_string = make_format_string(vega_spec_categorical_json, vega_spec_categorical_json_len);
  auto formatted = boost::format(format_string) %
    height %
    title %
    xlabel %
    ylabel;
  return formatted.str();
}


EXPORT std::string summary_view_spec(size_t length_elements){
  size_t height = static_cast<size_t>((300.0 * length_elements) + 80.0);

  auto format_string = make_format_string(vega_spec_summary_view_json, vega_spec_summary_view_json_len);
  auto formatted = boost::format(format_string) %
    height;
  return formatted.str();
}

std::string scatter_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {

  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = label_or_null(_title);

  auto format_string = make_format_string(vega_spec_scatter_json, vega_spec_scatter_json_len);
  auto formatted = boost::format(format_string) %
    title %
    xlabel %
    ylabel;
  return formatted.str();
}

std::string heatmap_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {
  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = label_or_null(_title);

  auto format_string = make_format_string(vega_spec_heatmap_json, vega_spec_heatmap_json_len);
  auto formatted = boost::format(format_string) %
    title %
    xlabel %
    ylabel;
  return formatted.str();
}

std::string categorical_heatmap_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {
  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = label_or_null(_title);

  auto format_string = make_format_string(vega_spec_categorical_heatmap_json, vega_spec_categorical_heatmap_json_len);
  auto formatted = boost::format(format_string) %
    title %
    xlabel %
    ylabel;
  return formatted.str();
}

std::string boxes_and_whiskers_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {
  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = label_or_null(_title);

  auto format_string = make_format_string(vega_spec_boxes_and_whiskers_json, vega_spec_boxes_and_whiskers_json_len);
  auto formatted = boost::format(format_string) %
    title %
    xlabel %
    ylabel;
  return formatted.str();
}

} // visualization
} // turi
