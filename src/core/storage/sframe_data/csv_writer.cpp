/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/csv_writer.hpp>
#include <core/data/flexible_type/string_escape.hpp>
#include <core/logging/logger.hpp>
namespace turi {

void csv_writer::write_verbatim(std::ostream& out,
                                const std::vector<std::string>& row) {
  for (size_t i = 0;i < row.size(); ++i) {
    out << row[i];
    // put a delimiter after every element except for the last element.
    if (i + 1 < row.size()) out << delimiter;
  }
  out << line_terminator;
}

void csv_writer::csv_print_internal(std::string& out, const flexible_type& val) {
  switch(val.get_type()) {
    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT:
      out += std::string(val);
      break;
    case flex_type_enum::DATETIME:
      out += std::string(val);
      break;
    case flex_type_enum::VECTOR:
      out += '[';
      for(size_t i = 0;i < val.get<flex_vec>().size(); ++i) {
        csv_print_internal(out, val.get<flex_vec>()[i]);
        if (i + 1 < val.get<flex_vec>().size()) out += ',';
      }
      out += ']';
      break;
    case flex_type_enum::STRING:
      // do not print double quotes
      escape_string(val.get<flex_string>(), escape_char, use_escape_char,
                    quote_char, true, false,
                    m_string_escape_buffer, m_string_escape_buffer_len);
      out += std::string(m_string_escape_buffer.c_str(), m_string_escape_buffer_len);
      break;
    case flex_type_enum::LIST:
      out += '[';
      for(size_t i = 0;i < val.get<flex_list>().size(); ++i) {
        csv_print_internal(out, val.get<flex_list>()[i]);
        if (i + 1 < val.get<flex_list>().size()) out += ',';
      }
      out += ']';
      break;
    case flex_type_enum::DICT:
      out += '{';
      for(size_t i = 0;i < val.get<flex_dict>().size(); ++i) {
        csv_print_internal(out, val.get<flex_dict>()[i].first);
        out += ':';
        csv_print_internal(out, val.get<flex_dict>()[i].second);
        if (i + 1 < val.get<flex_dict>().size()) out += ',';
      }
      out += '}';
      break;
    case flex_type_enum::UNDEFINED:
      break;
    default:
      out += (std::string)val;
      break;
  }
}

void csv_writer::csv_print(std::ostream& out,
                           const flexible_type& val,
                           bool allow_empty_output) {
  bool str_needs_delimiter = false;
  bool str_has_quote_char = false;
  switch(val.get_type()) {
    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT:
      if (quote_level == csv_quote_level::QUOTE_ALL) {
        out << quote_char << std::string(val) << quote_char; // quote numbers only at QUOTE_ALL
      } else {
        out << std::string(val);
      }
      break;
    case flex_type_enum::DATETIME:
    case flex_type_enum::VECTOR:
      if (quote_level == csv_quote_level::QUOTE_NONE) {
        out << std::string(val);
      } else {
        // quote this field at any level higher than QUOTE_NONE
        out << quote_char << std::string(val) << quote_char;
      }
      break;
    case flex_type_enum::STRING:
      /*
       * I have 4 quoting mechanisms to pick from
       * 1) full quoting and escaping
       * 2) no quoting but full escaping
       * 3) no quoting but only double quote escaping
       * 4) no quoting but no escaping
       */
      if (quote_level == csv_quote_level::QUOTE_ALL) {
        // quote all, pass through the whole escaping sequence
        escape_string(val.get<flex_string>(), escape_char, use_escape_char,
                      quote_char, true,
                      double_quote,
                      m_string_escape_buffer, m_string_escape_buffer_len);
        out.write(m_string_escape_buffer.c_str(), m_string_escape_buffer_len);
      } else {
        // not quote all. we can pick from a bunch of heuristics
        // to get minimal quoting
        const flex_string& valstr = val.get<flex_string>();
        // if there is a special character, or escape character or
        // line terminater in the string, we need full escaping
        //
        // if there is a quote char in the string, we need  at least
        // double quote escaping
        for (const char c : valstr) {
          if (str_needs_delimiter == false &&
              (c == '\t' || c == '\r' || c== '\n' || c == '\b' || c == escape_char ||
               (!line_terminator.empty() && c == line_terminator[0]) ||
               (!delimiter.empty() && c == delimiter[0]))) {
            str_needs_delimiter = true;
          }
          if (str_has_quote_char == false && c == quote_char) {
            str_has_quote_char = true;
          }
          if (str_has_quote_char && str_needs_delimiter) break;
        }

        if (allow_empty_output == false && valstr.length() == 0) {
          out << quote_char << quote_char;
        } else if (str_needs_delimiter == false && str_has_quote_char == false) {
          // - no delimiterization needed.
          out.write(valstr.c_str(), valstr.length());
        } else if (str_needs_delimiter == false &&
                   str_has_quote_char == true &&
                   double_quote == true) {
          // - no delimiterization needed.
          // - we have double quote to handle quotes
          escape_string(valstr, escape_char, false,
                        quote_char, false,
                        double_quote,
                        m_string_escape_buffer, m_string_escape_buffer_len);
          out.write(m_string_escape_buffer.c_str(), m_string_escape_buffer_len);
        }  else if (quote_level == csv_quote_level::QUOTE_NONE) {
          // do not quote at all, just escape
          escape_string(valstr, escape_char, use_escape_char,
                        quote_char, false,
                        double_quote,
                        m_string_escape_buffer, m_string_escape_buffer_len);
          out.write(m_string_escape_buffer.c_str(), m_string_escape_buffer_len);
        } else {
          // the regular case
          escape_string(val.get<flex_string>(), escape_char, use_escape_char,
                        quote_char, true,
                        double_quote,
                        m_string_escape_buffer, m_string_escape_buffer_len);
          out.write(m_string_escape_buffer.c_str(), m_string_escape_buffer_len);
        }
      }
      break;
    case flex_type_enum::LIST:
    case flex_type_enum::DICT:
      if (quote_level == csv_quote_level::QUOTE_NONE) {
        m_complex_type_temporary.clear();
        csv_print_internal(m_complex_type_temporary, val);
        out.write(m_complex_type_temporary.c_str(), m_complex_type_temporary.length());
      } else {
        m_complex_type_temporary.clear();
        csv_print_internal(m_complex_type_temporary, val);
        escape_string(m_complex_type_temporary, escape_char, use_escape_char,
                      quote_char, true,
                      double_quote,
                      m_complex_type_escape_buffer,
                      m_complex_type_escape_buffer_len);
        out.write(m_complex_type_escape_buffer.c_str(), m_complex_type_escape_buffer_len);
      }
      break;
    case flex_type_enum::UNDEFINED:
      if (quote_level == csv_quote_level::QUOTE_ALL) {
        out << quote_char << na_value << quote_char;
      } else {
        out.write(na_value.c_str(), na_value.length());
      }
      break;
    default:
      if (quote_level == csv_quote_level::QUOTE_NONE) {
        out << std::string(val);
      } else {
        // quote this field at any level higher than QUOTE_NONE
        out << quote_char << std::string(val) << quote_char;
      }
      break;
  }
}

void csv_writer::write(std::ostream& out,
                       const std::vector<flexible_type>& row) {
  for (size_t i = 0;i < row.size(); ++i) {
    csv_print(out, row[i], row.size() > 1);
    // put a delimiter after every element except for the last element.
    if (i + 1 < row.size()) out << delimiter;
  }
  out << line_terminator;
}



} // namespace turi
