/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/flexible_type/string_escape.hpp>
namespace turi {

void escape_string(const std::string& val,
                   char escape_char,
                   bool use_escape_char,
                   char quote_char,
                   bool use_quote_char,
                   bool double_quote,
                   std::string& output, size_t& output_len) {
  // A maximum of 2 + 2 * input array size is needed.
  // (every character is escaped, and quotes on both end
  if (output.size() < 2 + 2 * val.size()) {
    output.resize(2 + 2 * val.size());
  }
  // add the left quote
  char* cur_out = &(output[0]);
  if (use_quote_char) (*cur_out++) = quote_char;
  // loop through the input string
  if (use_escape_char) {
    // allow generation of escape characters
    for (size_t i = 0; i < val.size(); ++i) {
      char c = val[i];
      switch(c) {
       case '\'':
         if (double_quote && quote_char == '\'') {
           (*cur_out++) = '\'';
           (*cur_out++) = '\'';
         } else if (use_quote_char && quote_char == '\'') {
           (*cur_out++) = escape_char;
           (*cur_out++) = '\'';
         } else {
           (*cur_out++) = '\'';
         }
         break;
       case '\"':
         if (double_quote && quote_char == '\"') {
           (*cur_out++) = '\"';
           (*cur_out++) = '\"';
         } else if (use_quote_char && quote_char == '\"') {
           (*cur_out++) = escape_char;
           (*cur_out++) = '\"';
         } else {
           (*cur_out++) = '\"';
         }
         break;
       case '\\':
         // do not "double escape" if we have \u or \x. i.e. \u does not emit \\u
         if (i < val.size() - 1 && (val[i+1] == 'u' || val[i+1] == 'x')) {
           (*cur_out++) = c;
         } else {
           (*cur_out++) = escape_char;
           (*cur_out++) = '\\';
         }
         break;
       case '\t':
         (*cur_out++) = escape_char;
         (*cur_out++) = 't';
         break;
       case '\b':
         (*cur_out++) = escape_char;
         (*cur_out++) = 'b';
         break;
       case '\r':
         (*cur_out++) = escape_char;
         (*cur_out++) = 'r';
         break;
       case '\n':
         (*cur_out++) = escape_char;
         (*cur_out++) = 'n';
         break;
       case 0:
         (*cur_out++) = escape_char;
         (*cur_out++) = 0;
         break;
       default:
         (*cur_out++) = c;
      }
    }
  } else {
    // disallow generation of escape characters. Only do double quoting
    // where necessary
    for (size_t i = 0; i < val.size(); ++i) {
      char c = val[i];
      switch(c) {
       case '\'':
         if (double_quote && quote_char == '\'') {
           (*cur_out++) = '\'';
           (*cur_out++) = '\'';
         } else {
           (*cur_out++) = '\'';
         }
         break;
       case '\"':
         if (double_quote) {
           (*cur_out++) = '\"';
           (*cur_out++) = '\"';
         } else {
           (*cur_out++) = '\"';
         }
         break;
       default:
         (*cur_out++) = c;
      }
    }

  }
  if (use_quote_char) (*cur_out++) = quote_char;
  size_t len = cur_out - &(output[0]);
  output_len = len;
}


/**
 * Returns the value of a single hex digit.
 * Return is in the range 0-15. (size_t)(-1) on failure.
 */
inline size_t hex_to_val(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  else if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  else if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  return (size_t)(-1);
}

/**
 * Parses a 4 character hexadecimal string
 * Returns true on success, and the value, and false on failure
 */
bool parse_hex_block(char* c, size_t& ret) {
  ret = 0;
  for (size_t i = 0;i < 4; ++i) {
    ret = ret * 16;
    size_t val = hex_to_val(c[i]);
    if (val == (size_t)(-1)) return false;
    ret = ret + val;
  }
  return true;
}

/**
 * Writes a unicode value to an output buffer c.
 * c must contain enough room for the value.
 * Returns tyhe number of bytes written on success, 0 on invalid codepoint
 */
size_t write_utf8(size_t code_point, char* c) {
  // copied from wikipedia
  if (code_point < 0x80) {
    (*c++) = code_point;
    return 1;
  } else if (code_point <= 0x7FF) {
    (*c++) = ((code_point >> 6) + 0xC0);
    (*c++) = ((code_point & 0x3F) + 0x80);
    return 2;
  } else if (code_point <= 0xFFFF) {
    (*c++) = ((code_point >> 12) + 0xE0);
    (*c++) = (((code_point >> 6) & 0x3F) + 0x80);
    (*c++) = ((code_point & 0x3F) + 0x80);
    return 3;
  } else if (code_point <= 0x10FFFF) {
    (*c++) = ((code_point >> 18) + 0xF0);
    (*c++) = (((code_point >> 12) & 0x3F) + 0x80);
    (*c++) = (((code_point >> 6) & 0x3F) + 0x80);
    (*c++) = ((code_point & 0x3F) + 0x80);
    return 4;
  }
  return 0;
}

size_t unescape_string(char* cal, size_t length, bool use_escape_char,
                       char escape_char, char quote_char, bool double_quote) {
  // to avoid allocating a new string, we are do this entirely in-place
  // This works because for all the escapes we have here, the output string
  // is shorter than the input.
  size_t in = 0;
  size_t out = 0;

  while(in != length) {
    if ((use_escape_char && cal[in] == escape_char) && in + 1 < length) {
      char echar = cal[in + 1];
      switch (echar) {
       case '\'':
         cal[out++] = '\'';
         ++in;
         break;
       case '\"':
         cal[out++] = '\"';
         ++in;
         break;
       case '\\':
         cal[out++] = '\\';
         ++in;
         break;
       case '/':
         cal[out++] = '/';
         ++in;
         break;
       case 't':
         cal[out++] = '\t';
         ++in;
         break;
       case 'b':
         cal[out++] = '\b';
         ++in;
         break;
       case 'r':
         cal[out++] = '\r';
         ++in;
         break;
       case 'n':
         cal[out++] = '\n';
         ++in;
         break;
       case 'u':
         // unicode!
         // in is on the '\'
         // we need to have at least 6 characters \uHHHH
         // But if the hex value HHHH is between 0xD800 and D8FF, it is a
         // surrogate pair, and we need to decode another 6 characters \uHHHH
         // see
         // http://en.wikipedia.org/wiki/UTF-16
         if (in + 6 <= length) {
           size_t unicode_val = 0;
           bool unicode_value_is_good = false;
           size_t unicode_block_length = 0;
           if (parse_hex_block(cal + in + 2, unicode_val)) {
             if (unicode_val >= 0xD800 && unicode_val <= 0xD8FF) {
               // unicode_surrogate_pair
               // we need one more code point
               size_t unicode_high_surrogate = unicode_val;
               size_t unicode_low_surrogate = 0;
               bool low_surrogate_is_good =
                   in + 12 <= length &&   // there is enough length for the next block
                   cal[in + 6] == escape_char && // next block is also \uHHHH
                   cal[in + 7] == 'u' &&
                   parse_hex_block(cal + in + 8, unicode_low_surrogate);
               // low surrogate must be within the acceptable range
               low_surrogate_is_good = low_surrogate_is_good &&
                   unicode_low_surrogate >= 0xDC00 &&
                   unicode_low_surrogate <= 0xDFFF;
               if (low_surrogate_is_good) {
                 unicode_val = ((unicode_high_surrogate - 0xD800) << 10) +
                               (unicode_low_surrogate - 0xDC00) + 0x10000;
                 unicode_block_length = 12;
                 unicode_value_is_good = true;
               }
             } else {
               unicode_block_length = 6;
               unicode_value_is_good = true;
             }
           }
           if (unicode_value_is_good) {
             // the last 1 is incremented by the end of the loop iteration
             // so we increment unicode_block_length - 1
             in += unicode_block_length - 1;
             // encode unicode_val to UTF-8
             size_t bytes_written = write_utf8(unicode_val, cal + out);
             if (bytes_written != 0) {
               out += bytes_written;
               break;
             } // if bytes written is 0, this is an invalid code point. fall through
           } // unicode value is bad fall through.
         }
       default:
         cal[out++] = cal[in]; // do nothing
      }
    }
    else if (double_quote &&
             cal[in]  == quote_char &&
             in + 1 < length && cal[in+1] == quote_char) {
      cal[out++] = quote_char;
      ++in;
    } else {
      cal[out++] = cal[in];
    }
    ++in;
  }
  return out;
}


void unescape_string(std::string& cal, bool use_escape_char, char escape_char,
                     char quote_char, bool double_quote) {
  size_t new_length = unescape_string(&(cal[0]), cal.length(),
                                      use_escape_char, escape_char,
                                      quote_char, double_quote);
  cal.resize(new_length);
}

/**
 * Unescapes a string inplace
 */
void unescape_string(std::string& cal, char escape_char,
                     char quote_char, bool double_quote) {

  unescape_string(cal, true, escape_char, quote_char, double_quote);
}

size_t unescape_string(char* cal,
                       size_t length, char escape_char,
                       char quote_char, bool double_quote) {

  return unescape_string(cal, length, true, escape_char, quote_char, double_quote);
}


}
