/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FLEXIBLE_TYPE_SPIRIT_PARSER_HPP
#define TURI_FLEXIBLE_TYPE_SPIRIT_PARSER_HPP

#include <flexible_type/flexible_type.hpp>
#include <flexible_type/string_parser.hpp>

namespace boost { namespace spirit {
  namespace iso8859_1 {
    class space;
  }
  namespace qi {
    class eoi;
  }

}}



namespace turi {

/**
 * The actual grammar definitions
 */
template <typename Iterator, typename SpaceType>
struct flexible_type_parser_impl;

/**
 * A flexible_type_parser which takes in strings and returns flexible_types
 */
class flexible_type_parser {
 public:
  flexible_type_parser(std::string delimiter = ",", 
                       bool use_escape_char = true, 
                       char escape_char = '\\',
                       const std::unordered_set<std::string>& na_val = std::unordered_set<std::string>(),
                       const std::unordered_set<std::string>& true_val = std::unordered_set<std::string>(),
                       const std::unordered_set<std::string>& false_val = std::unordered_set<std::string>(),
                       bool only_raw_string_substitutions=false);
  /**
   * Parses a generalized flexible type from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      general_flexible_type_parse(const char** str, size_t len);

  /**
   * Parses a non-string flexible type from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      non_string_flexible_type_parse(const char** str, size_t len);

  /**
   * Parses a flex_dict from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      dict_parse(const char** str, size_t len);

  /**
   * Parses a flex_list from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      recursive_parse(const char** str, size_t len);

  /**
   * Parses a flex_vec from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      vector_parse(const char** str, size_t len);

  /**
   * Parses a double from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      double_parse(const char** str, size_t len);

  /**
   * Parses an integer from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      int_parse(const char** str, size_t len);


  /**
   * Parses an string from a string. The *str pointer will be
   * updated to point to the character after the last character parsed.
   * Returns a pair of (parsed value, success)
   */
  std::pair<flexible_type, bool>
      string_parse(const char** str, size_t len);

 private:
  std::shared_ptr<flexible_type_parser_impl<const char*, decltype(boost::spirit::iso8859_1::space)> > parser;
  std::shared_ptr<flexible_type_parser_impl<const char*, decltype(boost::spirit::qi::eoi)> > non_space_parser;
  bool delimiter_has_space(const std::string& separator);
  bool m_delimiter_has_space;
};

} // namespace turi

#endif // TURI_FLEXIBLE_TYPE_SPIRIT_PARSER_HPP
