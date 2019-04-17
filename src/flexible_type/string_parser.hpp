/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FLEXIBLE_TYPE_STRING_PARSER_HPP
#define TURI_FLEXIBLE_TYPE_STRING_PARSER_HPP
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/qi.hpp>
#include <flexible_type/flexible_type.hpp>
#include <flexible_type/string_escape.hpp>

/*
 * Must of this is obtained from 
 * http://boost-spirit.com/home/articles/qi-example/creating-your-own-parser-component-for-spirit-qi/
 */

namespace parser_impl { 

/**
 * \internal
 * The string parsing configuration.
 *
 */
struct parser_config {
  /// If any of these character occurs outside of quoted string, 
  /// the string will be terminated
  std::string restrictions;
  /// If the delimiter string is seen anywhere outside of a quoted string,
  /// the string will be terminated.
  std::string delimiter;
  /// Whether escape char should be used
  bool use_escape_char = true;
  /// The character to use for an escape character
  char escape_char = '\\';
  /** whether double quotes inside of a quote are treated as a single quote.
   * i.e. """hello""" => \"hello\"
   */
  char double_quote = true;

  std::unordered_set<std::string> na_val;
  std::unordered_set<std::string> true_val;
  std::unordered_set<std::string> false_val;

  /**
   * If this is set (defaults to false), then
   * the true/false/na substitutions are only permitted on raw
   * unparsed strings; that is strings before dequoting, de-escaping, etc.
   */
  bool only_raw_string_substitutions = false;
};

BOOST_SPIRIT_TERMINAL_EX(restricted_string); 

} // namespace parser_impl 

namespace boost { 
namespace spirit {


template <typename T1>
struct use_terminal<qi::domain, 
    terminal_ex<parser_impl::tag::restricted_string, fusion::vector1<T1> > >
  : mpl::true_ {};


} } // namespace spirit, boost

namespace parser_impl {

// anonynous namespace 
namespace {
/**
 * A buffer which allocates up to STACK_BUF_SIZE
 * on the stack, but then switches to a std::string heap
 * once it exceeds the size.
 */
struct stack_buffer {
  constexpr static size_t STACK_BUF_SIZE = 128;
  char buf[STACK_BUF_SIZE];
  std::string altbuf;
  size_t pos = 0;
  inline void add_char(char c) {
    if (pos < STACK_BUF_SIZE) {
      // write into the stack based buffer
      buf[pos] = c;
    } else if (pos == STACK_BUF_SIZE) {
      // switch buffers
      altbuf = std::string(buf, STACK_BUF_SIZE);
      altbuf += c;
    }else {
      altbuf += c;
    }
    ++pos;
  }

  std::string& get_string() {
    if (pos <= STACK_BUF_SIZE) altbuf = std::string(buf, pos);
    return altbuf;
  }
};
} // anonynous namespace 

/*
 * \internal
 * This class defines a string parser which allows the parser writer to define
 * a list of characters which are not permitted in unquoted strings. Quoted 
 * strings have no restrictions on what characters they can contain.
 * Usage:
 * \code
 *   parser_impl::parser_config config;
 *   config.[set stuff up]
 *   rule = parser_impl::restricted_string(config);
 * \endcode
 */
struct string_parser
    : boost::spirit::qi::primitive_parser<string_parser> {
  // Define the attribute type exposed by this parser component
  template <typename Context, typename Iterator>
  struct attribute {
    typedef ::turi::flexible_type type;
  };

  parser_config config;

  bool has_delimiter = false;
  char delimiter_first_char;
  bool delimiter_is_singlechar = false;
  std::unordered_map<std::string, turi::flexible_type> map_vals; // handle na_val, true_val, false_val
  bool only_raw_string_substitutions = false;

  string_parser(){}
  string_parser(parser_config config):config(config) {
    has_delimiter = config.delimiter.length() > 0;
    delimiter_is_singlechar = config.delimiter.length() == 1;
    if (has_delimiter) delimiter_first_char = config.delimiter[0];
    for (auto s: config.na_val) {
      map_vals[s] = turi::flexible_type(turi::flex_type_enum::UNDEFINED);
    }
    for (auto s: config.true_val) {
      map_vals[s] = 1;
    }
    for (auto s: config.false_val) {
      map_vals[s] = 0;
    }
    only_raw_string_substitutions = config.only_raw_string_substitutions;
  }

  enum class tokenizer_state {
    START_FIELD, IN_FIELD, IN_QUOTED_FIELD,
  };

  static inline bool test_is_delimiter(const char* c, const char* end, 
                                       const char* delimiter, const char* delimiter_end) {
  // if I have more delimiter characters than the length of the string
  // quit.
    if (delimiter_end - delimiter > end - c) return false; 
    while (delimiter != delimiter_end) {
      if ((*c) != (*delimiter)) return false;
      ++c; ++delimiter;
    }
    return true;
  }
#define PUSH_CHAR(c) ret.add_char(c); escape_sequence = config.use_escape_char && (c == config.escape_char);

// insert a character into the field buffer. resizing it if necessary

  // This function is called during the actual parsing process
  template <typename Iterator, typename Context, typename Skipper, typename Attribute>
  bool parse(Iterator& first, Iterator const& last, 
             Context&, Skipper const& skipper, Attribute& attr) const {
    boost::spirit::qi::skip_over(first, last, skipper);
    Iterator cur = first;
    stack_buffer ret;
    const char* delimiter_begin = config.delimiter.c_str();
    const char* delimiter_end = delimiter_begin + config.delimiter.length();

    tokenizer_state state = tokenizer_state::START_FIELD; 
    bool keep_parsing = true;
    char quote_char = 0;
    const char* raw_field_begin = nullptr;
    // this is set to true for the character immediately after an escape character
    // and false all other times
    bool escape_sequence = false;
    while(keep_parsing && cur != last) {
      // since escape_sequence can only be true for one character after it is
      // set to true. I need a flag here. if reset_escape_sequence is true, the
      // at the end of the loop, I clear escape_sequence
      bool reset_escape_sequence = escape_sequence;

      // Next character in file
      char c = *cur;
      if(state != tokenizer_state::IN_QUOTED_FIELD && 
         config.restrictions.find(c) != std::string::npos) break;

      bool is_delimiter = 
          // current state is not in a quoted field since delimiters in quoted 
          // fields are fine.
          (state != tokenizer_state::IN_QUOTED_FIELD) &&
          // and there is a delimiter
          has_delimiter && 
          // and current character matches first character of delimiter
          // and delimiter is either a single character, or we need to do a 
          // more expensive test.
          delimiter_first_char == c &&
           (delimiter_is_singlechar || 
            test_is_delimiter(cur, last, delimiter_begin, delimiter_end));

      if (is_delimiter) break;

      ++cur;
      switch(state) {
       case tokenizer_state::START_FIELD:
         raw_field_begin = cur-1; // -1 because cur has already been incremented
         if (c == '\'' || c == '\"') {
           quote_char = c;
           state = tokenizer_state::IN_QUOTED_FIELD;
         } else {
           /* begin new unquoted field */
           PUSH_CHAR(c);
           state = tokenizer_state::IN_FIELD;
         }
         break;

       case tokenizer_state::IN_FIELD:
         /* normal character - save in field */
         PUSH_CHAR(c);
         break;

       case tokenizer_state::IN_QUOTED_FIELD:
         /* in quoted field */
         if (c == quote_char && !escape_sequence) {
           if (c == '\"' && config.double_quote) {
             /* doublequote; " represented by "" */
             // look ahead one character
             if (cur + 1 < last && *cur == quote_char) {
               PUSH_CHAR(c);
               ++cur;
               break;
             }
           }
           // we are done.
           keep_parsing = false;
         }
         else {
           /* normal character - save in field */
           PUSH_CHAR(c);
         }
         break;
      }
      if (reset_escape_sequence) escape_sequence = false;
    }
    if (cur == first) return false;
    else {
      first = cur;
      if (only_raw_string_substitutions == true && raw_field_begin != nullptr) {
        std::string raw_str = std::string(raw_field_begin, cur - raw_field_begin);
        boost::algorithm::trim_right(raw_str);
        auto map_val_iter = map_vals.find(raw_str);
        if (map_val_iter != map_vals.end()) {
          attr = map_val_iter->second;
          return true;
        }
      }

      std::string final_str = std::move(ret.get_string());
      if (!quote_char) boost::algorithm::trim_right(final_str);
      else if (quote_char) {
        // if was quoted field, we unescape the contents
        turi::unescape_string(final_str, config.use_escape_char, 
                              config.escape_char,
                              quote_char, config.double_quote);
      }

      if (only_raw_string_substitutions == false) {
        auto map_val_iter = map_vals.find(final_str);
        if (map_val_iter != map_vals.end()) {
          attr = map_val_iter->second;
          return true;
        }
      }
      attr = std::move(final_str);
      return true;
    }
    return true;
  }

// This function is called during error handling to create
// a human readable string for the error context.
  template <typename Context>
  boost::spirit::info what(Context&) const {
    return boost::spirit::info("string_parser");
  }
};
} // namespace parser_impl

namespace boost { 
namespace spirit { 
namespace qi {

// This is the factory function object invoked in order to create
// an instance of our iter_pos_parser.
template <typename Modifiers, typename T1>
struct make_primitive<terminal_ex<parser_impl::tag::restricted_string, fusion::vector1<T1>>, Modifiers> {
    typedef parser_impl::string_parser result_type;

    template <typename Terminal>
    result_type operator()(const Terminal& term, unused_type) const {
        return result_type(fusion::at_c<0>(term.args));
    }
};

}}} // namespace qi, spirit, boost

#undef PUSH_CHAR

#endif
