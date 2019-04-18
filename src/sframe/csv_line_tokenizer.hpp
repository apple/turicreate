/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_LIB_SFRAME_CSV_LINE_TOKENIZER_HPP
#define TURI_UNITY_LIB_SFRAME_CSV_LINE_TOKENIZER_HPP
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>
#include <memory>
#include <flexible_type/flexible_type.hpp>
#include <parallel/mutex.hpp>
#include <iostream>

namespace turi {

class flexible_type_parser;

/**
 * \ingroup sframe_physical
 * \addtogroup csv_utils CSV Parsing and Writing
 * \{
 */

/**
 * CSV Line Tokenizer.
 * 
 * To use, simply set the appropriate options inside the struct, and use one of
 * the tokenize_line functions to parse a line inside a CSV file.
 *
 * \note This parser at the moment only handles the case where each row of
 * the CSV is on one line. It is in fact very possible that this is not the 
 * case. Pandas in particular permits line breaks inside of quoted strings,
 * and vectors, and that is quite problematic.  
 */
struct csv_line_tokenizer {
  /**
   * If set to true, quotes inside a field will be preserved (Default false). 
   * i.e. if set to true, the 2nd entry in the following row will be read as
   * ""hello world"" with the quote characters.
   * \verbatim
   *   1,"hello world",5
   * \endverbatim
   */
  bool preserve_quoting = false;

  /**
   * If escape_char is used.
   */
  bool use_escape_char = true;
  /**
   * The character to use to identify the beginning of a C escape sequence 
   * (Defualt '\'). i.e. "\n" will be converted to the '\n' character, "\\"
   * will be converted to "\", etc. Note that only the single character 
   * escapes are converted. unicode (\Unnnn), octal (\nnn), hexadecimal (\xnn)
   * are not interpreted.
   */
  char escape_char = '\\';

  /**
   * If set to true, initial spaces before fields are ignored (Default true).
   */
  bool skip_initial_space = true;


  /**
   * The delimiter character to use to separate fields (Default ",")
   */
  std::string delimiter = ",";


  /**
   * The string to use to separate lines. Defaults to "\n".
   * Setting the new line string to "\n" has special effects in that it
   * causes "\r", "\r\n" and "\n" to be all interpreted as new lines.
   */
  std::string line_terminator = "\n";

  /**
   * The character used to begin a comment (Default '#'). An occurance of 
   * this character outside of quoted strings will cause the parser to
   * ignore the remainder of the line.
   * \verbatim
   * # this is a 
   * # comment
   * user,name,rating
   * 123,hello,45
   * 312,chu, 21
   * 333,zzz, 3 # this is also a comment
   * 444,aaa, 51
   * \endverbatim
   */
  char comment_char = '#';

  /**
   * Whether comment char is used
   */
  bool has_comment_char = true;

  /**
   * If set to true, pairs of quote characters in a quoted string 
   * are interpreted as a single quote (Default false).
   * For instance, if set to true, the 2nd field of the 2nd line is read as
   * \b "hello "world""
   * \verbatim
   * user, message
   * 123, "hello ""world"""
   * \endverbatim
   */
  bool double_quote = false;

  /**
   * The quote character to use (Default '\"')
   */
  char quote_char = '\"';

  /**
   * The strings which will be parsed as missing values.
   *
   * (also see empty_string_in_na_values)
   */
  std::vector<std::string> na_values;

  /**
   * string values which map to numeric 1
   */
  std::unordered_set<std::string> true_values;

  /**
   * string values which map to numeric 0
   */
  std::unordered_set<std::string> false_values;

  /**
   * If this is set (defaults to false), then
   * the true/false/na substitutions are only permitted on raw
   * unparsed strings; that is strings before dequoting, de-escaping, etc.
   */
  bool only_raw_string_substitutions = false;

  /**
   * Constructor. Does nothing but set up internal buffers.
   */
  csv_line_tokenizer();

  /**
   * called before any parsing functions are used. Initializes the spirit parser.
   */
  void init();

  /**
   * Tokenize a single CSV line into seperate fields.
   * The output vector will be cleared, and each field will be inserted into
   * the output vector. Returns true on success and false on failure.
   *
   * \param str Pointer to string to tokenize. 
   *            Contents of string may be modified. 
   * \param len Length of string to tokenize
   * \param output Output vector which will contain the result
   *
   * \returns true on success, false on failure.
   */
  bool tokenize_line(const char* str, size_t len,
                     std::vector<std::string>& output);

  /**
   * Tokenize a single CSV line into seperate fields, calling a callback
   * for each parsed token.
   *
   * The function is of the form:
   * \code
   * bool receive_token(const char* buffer, size_t len) {
   *   // add the first len bytes of the buffer as the parsed token
   *   // return true on success and false on failure.
   *
   *   // if this function returns false, the tokenize_line call will also
   *   // return false
   *
   *   // The buffer may be modified
   * }
   * \endcode
   * 
   * For instance, to insert the parsed tokens into an output vector, the
   * following code could be used:
   * 
   * \code
   * return tokenize_line(str, 
   *                 [&](const char* buf, size_t len)->bool {
   *                   output.emplace_back(buf, len);
   *                   return true;
   *                 });
   * \endcode
   *
   * \param str Pointer to line to tokenize. Contents of string may be modified.
   * \param len Length of line to tokenize
   * \param fn Callback function which is called on every token
   *
   * \returns true on success, false on failure.
   */
  bool tokenize_line(const char* str, size_t len,
                     std::function<bool (std::string&, size_t)> fn);

  /**
   * Tokenizes a line directly into array of flexible_type and type specifiers.
   * This version of tokenize line is strict, requiring that the length of 
   * the output vector matches up exactly with the number of columns, and the
   * types of the flexible_type be fully specified.
   *
   * For instance:
   * If my input line is
   * \verbatim
   *     1, hello world, 2.0
   * \endverbatim
   *
   * then output vector must have 3 elements. 
   *
   * If the types of the 3 elements in the output vector are: 
   * [flex_type_enum::INTEGER, flex_type_enum::STRING, flex_type_enum::FLOAT]
   * then, they will be parsed as such emitting an output of 
   * [1, "hello world", 2.0].
   *
   * However, if the types of the 3 elements in the output vector are: 
   * [flex_type_enum::STRING, flex_type_enum::STRING, flex_type_enum::STRING]
   * then, the output will contain be ["1", "hello world", "2.0"].
   *
   * Type interpretation failures will produce an error.
   * For instance if the types are
   * [flex_type_enum::STRING, flex_type_enum::INTEGER, flex_type_enum::STRING],
   * since the second element cannot be correctly interpreted as an integer,
   * the tokenization will fail.
   *
   * The types current supported are:
   *  - flex_type_enum::INTEGER
   *  - flex_type_enum::FLOAT
   *  - flex_type_enum::STRING
   *  - flex_type_enum::VECTOR (a vector of numbers specified like [1 2 3]
   *                            but allowing separators to be spaces, commas(,)
   *                            or semicolons(;). The separator should not
   *                            match the CSV separator since the parsers are 
   *                            independent)
   *
   * The tokenizer will not modify the types of the output vector. However,
   * if permit_undefined is specified, the output type can be set to
   * flex_type_enum::UNDEFINED for an empty non-string field. For instance:
   *
   *
   * If my input line is
   * \verbatim
   *     1, , 2.0
   * \endverbatim
   * If I have type specifiers
   * [flex_type_enum::INTEGER, flex_type_enum::STRING, flex_type_enum::FLOAT]
   * This will be parsed as [1, "", 2.0] regardless of permit_undefined.
   *
   * However if I have type specifiers
   * [flex_type_enum::INTEGER, flex_type_enum::INTEGER, flex_type_enum::FLOAT]
   * and permit_undefined == false, This will be parsed as [1, 0, 2.0].
   *
   * And if I have type specifiers
   * [flex_type_enum::INTEGER, flex_type_enum::INTEGER, flex_type_enum::FLOAT]
   * and permit_undefined == true, This will be parsed as [1, UNDEFINED, 2.0].
   *
   * \param str Pointer to line to tokenize
   * \param len Length of line to tokenize
   * \param output The output vector which is of the same length as the number
   * of columns, and has all the types specified.
   * \param permit_undefined Allows output vector to repr
   * \param output_order a pointer to an array of the same length as the output.
   *                     Essentially column 'i' will be written to output_order[i].
   *                     if output_order[i] == -1, the column is ignored.
   *                     If output_order == nullptr, this is equivalent to the
   *                     having output_order[i] == i
   *
   * \returns the number of output entries filled.
   */
  size_t tokenize_line(char* str, size_t len,
                       std::vector<flexible_type>& output,
                       bool permit_undefined,
                       const std::vector<size_t>* output_order = nullptr);


  /**
   * Parse the buf content into flexible_type.
   * The type of the flexible_type is determined by the out variable.
   *
   * If recursive_parse is set to true, things which parse to strings will 
   * attempt to be reparsed. This allows for instance
   * the quoted element "123" to be parsed as an integer instead of a string.
   *
   * If recursive_parse is true, the contents of the buffer may be modified
   * (the buffer itself is used to maintain the recursive parse state)
   */
  bool parse_as(char** buf, size_t len, 
                const char* raw, size_t rawlen,
                flexible_type& out, bool recursive_parse=false);

  /**
   * Returns a printable string describing the parse error.
   * This is only filled when \ref tokenize_line fails.
   * The string is *not* cleared when tokenize line succeeds so this should
   * not be used for flagging parse errors.
   */
  const std::string& get_last_parse_error_diagnosis() const;

 private:
  // internal buffer
  std::string field_buffer;
  // current length of internal buffer
  size_t field_buffer_len = 0;

  // the printable string describing the parse error
  std::string parse_error;
  // internal error. filled when tokenizer fails. This is appended to parse_error
  // when appropriate
  std::string tokenizer_impl_error;
  ssize_t tokenizer_impl_fail_pos = -1;

  // the state of the tokenizer state machine
  enum class tokenizer_state {
    START_FIELD, IN_FIELD, IN_QUOTED_FIELD
  };

  /**
   * \param str Pointer to line to tokenize
   * \param len Length of line to tokenize
   * \param add_token Callback function which is called on every successful token.
   *                  This function is allowed to modify the input string.
   * \param lookahead_fn Callback function which is called to look ahead
   *                     for the end of the token when bracketing [], {} is
   *                     encountered. it is called with a (char**, len) and return
   *                     true/false on success/failure. This function must not
   *                     modify the input string.
   * \param undotoken Callback function which is called to undo the previously
   *                  parsed token. Only called when lookahead succeeds, but later
   *                  parsing fails, thus requiring cancellation of the lookahead.
   *
   * \note Whether this function modifies the input string is dependent on 
   * whether add_token modifies the input string.
   */
  template <typename Fn, typename Fn2, typename Fn3>
  bool tokenize_line_impl(char* str, size_t len, 
                          Fn add_token,
                          Fn2 lookahead,
                          Fn3 undotoken);

  std::shared_ptr<flexible_type_parser> parser;

  // some precomputed information about the delimiter so we avoid excess
  // string comparisons of the delimiter value
  bool delimiter_is_new_line = false;
  bool delimiter_is_space_but_not_tab = false;
  char delimiter_first_character;
  bool delimiter_is_singlechar = false;
  bool delimiter_is_not_empty = true;
  bool empty_string_in_na_values = false;
  bool is_regular_line_terminator = true;



  /**
   * Perform substitutions of true/false/na values
   */
  bool check_substitutions(const char* buf, size_t len, flexible_type& out);
};
/// \}
} // namespace turi

namespace std {
static inline ostream& operator<<(ostream& os, const turi::csv_line_tokenizer& t) {
  os << "Tokenizer("
     << "preseve_quoting=" << t.preserve_quoting << ", "
     << "use_escape_char='" << t.use_escape_char << "', "
     << "escape_char='" << t.escape_char << "', "
     << "skip_initial_space=" << t.skip_initial_space << ", "
     << "delimiter=\"" << t.delimiter << "\", "
     << "line_terminator=\"" << t.line_terminator << "\", "
     << "comment_char=\'" << t.comment_char << "', "
     << "has_comment_char=" << t.has_comment_char << ","
     << "double_quote=" << t.double_quote << ","
     << "quote_char=\'" << t.quote_char << "\'"
     << "na_values=";

  for (size_t i = 0; i < t.na_values.size(); ++i) {
    os << t.na_values[i];
    if (i + 1 != t.na_values.size()) {
      os << ",";
    }
  }
  os << ")";

  return os;
}
}  // namespace std

#endif
