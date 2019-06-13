/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef RCPP_SERIALIZE_HPP
#define RCPP_SERIALIZE_HPP

#undef HAVE_VISIBILITY_ATTRIBUTE

#include <stdlib.h>
#include <string.h>
// most of functions used here are included from Rinternals.h
//#include <Rcpp.h>

#include <RApiSerializeAPI.h>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

// the serialization function from SEXP to std::string
inline std::string serializeToStr(SEXP object) {
    // using R's C API, all SEXP objects will be serialized into a raw vector
    Rcpp::RawVector val = serializeToRaw(object);

    // convert R raw vector into a std::string
    std::string res;

    for (size_t i = 0; i < val.size(); i++) {
        res = res + std::to_string(int(val[i])) + "\t";
    }

    return res;
}

// unserialize from the std::string
inline SEXP unserializeFromStr(std::string s) {
    // parse the std::string into a raw vector
    std::vector<std::string> strs;
    boost::regex e("^\\d.+");
    if (boost::regex_match(s, e)) {
        boost::split(strs,s,boost::is_any_of("\t"));
    }

    Rcpp::RawVector object(strs.size() - 1);

    for (size_t i = 0; i < strs.size() - 1; i++) {
        object[i] = static_cast<unsigned char>(std::stoi(strs[i]));
    }

    return unserializeFromRaw(object);
}

#endif
