/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/table_printer/table_element_printers.hpp>
#include <sstream>

// The implementations of the specific element printers and recorders

namespace turi { namespace table_internal {

/** Print a possibly truncated string to the output stream
 */
void _print_string(std::ostringstream& ss, size_t width, const std::string& s) {
  ss << ' ';

  if(s.size() > width) {
    ss << s.substr(0, width - 2);
    ss << "...";
  } else {
    ss << s;

    for(size_t i = s.size(); i < width; ++i)
      ss << ' ';
    ss << ' ';
  }

  ss << '|';
}

/** Print a formatted double to the output stream.
 */
void _print_double(std::ostringstream& ss, size_t width, double value) {

  if(long(value) == value) {
    std::string sv = std::to_string(long(value));
    if(sv.size() < width) {
      _print_string(ss, width, sv);
      return;
    }
  }

  ss << ' ';

  size_t start_pos = ss.tellp();

  bool printed_normally = false;

  {
    std::ostringstream ss_buf;
    ss_buf.width(width);
    ss_buf << std::left << value;

    if(size_t(ss_buf.tellp()) <= width) {
      ss << ss_buf.str();
      printed_normally = true;
    }
  }

  if(!printed_normally) {

    // Find a good precision with which to print this; loop until it
    // breaks or we hit 4 decimal places.
    size_t precision = 0;
    for(;precision < 5; ++precision) {

      std::ostringstream ss_buf;
      ss_buf.width(width);
      ss_buf << std::left << std::setprecision(precision) << value;

      if(ss_buf.tellp() > long(width)) {
        precision = (precision == 0) ? 0 : (precision - 1);

        std::ostringstream ss_buf_2;
        ss_buf_2.width(width);
        ss_buf_2 << std::left << std::setprecision(precision) << value;

        ss << ss_buf_2.str();
        break;
      }
    }
  }

  // Add in padding as needed.
  while(size_t(ss.tellp()) < start_pos + width)
    ss << ' ';

  ss << ' ' << '|';
}


/** Print a formatted boolean to the output stream
 */
void _print_bool(std::ostringstream& ss, size_t width, bool b) {
  if(width >= 5)
    _print_string(ss, width, std::string(b ? "True" : "False"));
  else
    _print_string(ss, width, std::string(b ? "T" : "F"));
}


/** Print a formatted long integer to the output stream.
 */
void _print_long(std::ostringstream& ss, size_t width, long v) {
  std::ostringstream ss_buf;
  ss_buf << std::left << v;

  if(size_t(ss_buf.tellp()) <= width) {
    _print_string(ss, width, ss_buf.str());
  } else {
    _print_double(ss, width, v);
  }
}

/** Print a formatted time to the output stream ss.
 */
void _print_time(std::ostringstream& ss, size_t width, double t) {

  std::stringstream ts;

  if(t < 0.001) {

    ts << (1000000*t) << "us";

  } else if(t < 1) {

    ts << (1000*t) << "ms";

  } else if(t < 60) {

    size_t nhs = size_t(floor(100*t)) % 100;
    ts << (int(floor(t))) << (nhs >= 10 ? "." : ".0") << nhs << "s";

  } else if(t < 3600) {

    ts << int(floor(t/60)) << "m " << (int(floor(t)) % 60) << "s";

  } else if(t < 86400) {    // < 1 days

    ts << int(floor(t/3600)) << "h " << ((int(floor(t)) % 3600) / 60) << "m";

  } else if(t < 10*86400) { // < 10 days

    ts << int(floor(t/86400)) << "d "
       << ((int(floor(t)) % 86400) /3600) << "h "
       << ((int(floor(t)) % 3600) / 60) << "m";

  } else {

    ts << int(floor(t/86400)) << "d "
       << ((int(floor(t)) % 86400) /3600) << "h ";
  }

  _print_string(ss, width, ts.str());
}

/** Print a formatted flexible type to the output stream.
 */
void _print_flexible_type(std::ostringstream& ss, size_t width, const flexible_type& t) {
  switch(t.get_type()) {

    case flex_type_enum::FLOAT:
      _print_double(ss, width, t.get<double>());
      return;

    case flex_type_enum::INTEGER:
      _print_long(ss, width, t.get<flex_int>());
      return;

    case flex_type_enum::STRING:
      _print_string(ss, width, t.get<flex_string>());
      return;

    default:
      _print_string(ss, width, std::string(t));
      return;
  }
}


}}
