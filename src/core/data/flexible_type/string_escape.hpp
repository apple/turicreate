/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FLEXIBLE_TYPE_STRING_ESCAPE_HPP
#define TURI_FLEXIBLE_TYPE_STRING_ESCAPE_HPP
#include <string>
namespace turi {

/**
 * Unescapes a string inplace
 */
void unescape_string(std::string& cal, bool use_escape_char, char escape_char, 
                     char quote_char, bool double_quote);

/**
 * Unescapes a string inplace
 */
void unescape_string(std::string& cal, char escape_char,
                     char quote_char, bool double_quote);

/**
 * Unescapes a string inplace, returning the new length
 */
size_t unescape_string(char* cal, 
                       size_t length, bool use_escape_char, char escape_char, 
                       char quote_char, bool double_quote);
/**
 * Unescapes a string inplace, returning the new length
 */
size_t unescape_string(char* cal,
                       size_t length, char escape_char,
                       char quote_char, bool double_quote);

/**
 * Escapes a string from val into output.
 * The length of the output string is in returned in output_len.
 * Note that output.length() may be greater than the output_len.
 *
 * \param val The string to escape
 * \param escape_char The escape character to use (recommended '\\')
 * \param use_escape_char If true, escape character is used. Note that
 *       if this is false, the resultant string may not always be parseable.
 * \param quote_char The quote character to use. (recommended '\"')
 * \param use_quote_char If the output string should be quoted
 * \param double_quote If double quotes are converted to single quotes.
 */
void escape_string(const std::string& val, 
                   char escape_char,
                   bool use_escape_char,
                   char quote_char,
                   bool use_quote_char,
                   bool double_quote,
                   std::string& output, 
                   size_t& output_len);

} // namespace turi

#endif
