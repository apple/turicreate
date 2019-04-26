/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/lib/visualization/escape.hpp>
#include <unity/lib/visualization/vega_spec.hpp>

#include <capi/TuriCreate.h>
#include <logger/assertions.hpp>

// generated include files for vega spec JSON
#include <unity/lib/visualization/vega_spec/boxes_and_whiskers.h>
#include <unity/lib/visualization/vega_spec/categorical.h>
#include <unity/lib/visualization/vega_spec/categorical_heatmap.h>
#include <unity/lib/visualization/vega_spec/heatmap.h>
#include <unity/lib/visualization/vega_spec/histogram.h>
#include <unity/lib/visualization/vega_spec/scatter.h>
#include <unity/lib/visualization/vega_spec/summary_view.h>

// use boost::replace_all to get rid of newlines
#include <boost/algorithm/string/replace.hpp>

namespace turi {
namespace visualization {

/*
 * Prepares a raw JSON format string
 * by doing the following:
 * 1. Strips all newlines.
 */
std::string make_format_string(unsigned char *raw_format_str_ptr,
                                      size_t raw_format_str_len) {
  auto raw_format_str = std::string(
    reinterpret_cast<char *>(raw_format_str_ptr),
    raw_format_str_len);

  boost::replace_all(raw_format_str, "\n", "");
  return raw_format_str;
}

static void _format_impl(std::string& ret,
                                const std::string& placeholder,
                                const std::string& replacement) {
    std::string::size_type pos = ret.find(placeholder);
    while( pos != std::string::npos ) {
        ret.replace(pos, placeholder.length(), replacement);
        pos = ret.find(placeholder, ++pos);
    }
}

std::string format(const std::string& format_str, const std::unordered_map<std::string, std::string>& format_params) {
  // TODO: optimize this.
  // For now, it's O(n * k), where n is the number of format parameters, and
  // k is the number of instances of each parameter.
  std::string ret = format_str;
  for (const auto& it : format_params) {
    _format_impl(ret, it.first, it.second);
  }
  return ret;
}

static std::string label_or_default(const flexible_type& label,
                                    const std::string& _default) {
  if (label == FLEX_UNDEFINED) {
    // undefined should render as null in JSON
    return "null";

  } else if (label == tc_plot_title_default_label) {
    // substitute the default label
    return extra_label_escape(_default, false /* include_quotes */);

  } else {
    // user-provided label should render with quotes/escaping
    return extra_label_escape(label.get<flex_string>(), false /* include_quotes */);
  }
}

static std::string title_or_default(const flexible_type& title, const std::string& default_title) {
  if (title == FLEX_UNDEFINED) {
    // undefined/not provided should render as null in JSON
    return "null";
  } else if (title == tc_plot_title_default_label) {
    return extra_label_escape(default_title, true /* include_quotes */);
  } else {
    // user-provided label should render with quotes/escaping
    return extra_label_escape(title.get<flex_string>(), true /* include_quotes */);
  }

}

static std::string title_or_default(const flexible_type& title, const std::string& xlabel, const std::string& ylabel) {
  if (title == FLEX_UNDEFINED) {
    // undefined/not provided should render as null in JSON
    return "null";
  } else if (title == tc_plot_title_default_label) {
    return extra_label_escape(xlabel + " vs. " + ylabel, true /* include_quotes */);
  } else {
    // user-provided label should render with quotes/escaping
    return extra_label_escape(title.get<flex_string>(), true /* include_quotes */);
  }

}

EXPORT std::string histogram_spec(const flexible_type& _title,
                                  const flexible_type& _xlabel,
                                  const flexible_type& _ylabel,
                                  flex_type_enum dtype) {
  std::string default_title = std::string("Distribution of Values [") +
                                     flex_type_enum_to_name(dtype) +
                                     "]";
  flexible_type title = title_or_default(_title, default_title);
  flexible_type xlabel = label_or_default(_xlabel, "Values");
  flexible_type ylabel = label_or_default(_ylabel, "Count");

  auto format_string = make_format_string(vega_spec_histogram_json, vega_spec_histogram_json_len);
  return format(format_string, {
    {"{{title}}", title},
    {"{{xlabel}}", xlabel},
    {"{{ylabel}}", ylabel},
  });
}

EXPORT std::string categorical_spec(const flexible_type& _title,
                                    const flexible_type& _xlabel,
                                    const flexible_type& _ylabel,
                                    flex_type_enum dtype) {
  std::string default_title = std::string("Distribution of Values [") +
                                     flex_type_enum_to_name(dtype) +
                                     "]";

  flexible_type title = title_or_default(_title, default_title);
  flexible_type xlabel = label_or_default(_xlabel, "Count");
  flexible_type ylabel = label_or_default(_ylabel, "Values");

  auto format_string = make_format_string(vega_spec_categorical_json, vega_spec_categorical_json_len);
  return format(format_string, {
    {"{{title}}", title},
    {"{{xlabel}}", xlabel},
    {"{{ylabel}}", ylabel},
  });
}


EXPORT std::string summary_view_spec(size_t length_elements){
  size_t height = static_cast<size_t>((300.0 * length_elements) + 80.0);

  auto format_string = make_format_string(vega_spec_summary_view_json, vega_spec_summary_view_json_len);
  return format(format_string, {
    {"{{computed_height}}", std::to_string(height)}
  });
}

std::string scatter_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {

  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = title_or_default(_title, xlabel, ylabel);

  auto format_string = make_format_string(vega_spec_scatter_json, vega_spec_scatter_json_len);
  return format(format_string, {
    {"{{title}}", title},
    {"{{xlabel}}", xlabel},
    {"{{ylabel}}", ylabel},
  });
}

std::string heatmap_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {
  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = title_or_default(_title, xlabel, ylabel);

  auto format_string = make_format_string(vega_spec_heatmap_json, vega_spec_heatmap_json_len);
  return format(format_string, {
    {"{{title}}", title},
    {"{{xlabel}}", xlabel},
    {"{{ylabel}}", ylabel},
  });
}

std::string categorical_heatmap_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {
  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = title_or_default(_title, xlabel, ylabel);

  auto format_string = make_format_string(vega_spec_categorical_heatmap_json, vega_spec_categorical_heatmap_json_len);
  return format(format_string, {
    {"{{title}}", title},
    {"{{xlabel}}", xlabel},
    {"{{ylabel}}", ylabel},
  });
}

std::string boxes_and_whiskers_spec(const flexible_type& _xlabel, const flexible_type& _ylabel, const flexible_type& _title) {
  std::string xlabel = label_or_default(_xlabel, "X");
  std::string ylabel = label_or_default(_ylabel, "Y");
  std::string title = title_or_default(_title, xlabel, ylabel);

  auto format_string = make_format_string(vega_spec_boxes_and_whiskers_json, vega_spec_boxes_and_whiskers_json_len);
  return format(format_string, {
    {"{{title}}", title},
    {"{{xlabel}}", xlabel},
    {"{{ylabel}}", ylabel},
  });
}

} // visualization
} // turi
