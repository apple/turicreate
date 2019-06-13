/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <limits>
#include <core/export.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/random/random.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <toolkits/feature_engineering/content_interpretation.hpp>

/** Code for infering string content interpretations.
 */
namespace turi { namespace feature_engineering {

EXPORT flex_string infer_string_content_interpretation(gl_sarray data) {

  /** Three options -- categorical, text_short, text_long.  Depends on
   *  quite a bit here.  We'll update these rules in the future, when
   *  we figure out how useful this is and where it breaks.
   */
  std::array<char, 4> _sep_chars {{' ', ',', '.', '\t'}};

  // Track this so they can be excluded from the final average.
  auto n_undefined_ptr = std::make_shared<atomic<size_t> >(0);

  // -1 -- is categorical
  // 0 -- short string
  // 1 -- long string
  auto vote = [=](const flexible_type& ft) -> flexible_type GL_GCC_ONLY(GL_HOT_FLATTEN) {
    if(ft.get_type() == flex_type_enum::UNDEFINED) {
      ++(*n_undefined_ptr);
      return 0;
    } else {
      DASSERT_TRUE(ft.get_type() == flex_type_enum::STRING);

      const flex_string& s = ft.get<flex_string>();
      const auto& sep_chars = _sep_chars;

      auto is_sep_char = [&](char c) {
        return std::find(sep_chars.begin(), sep_chars.end(), c) != sep_chars.end();
      };

      size_t n_seps = std::count_if(s.begin(), s.end(), is_sep_char);

      if(n_seps >= 2) {
        if(s.size() >= 100) {
          // Vote for long text
          return 1;
        } else {
          // Vote for short text
          return 0;
        }
      } else {
        // Vote for categorical.
        return -1;
      }
    }
  };

  size_t n_undefined = *n_undefined_ptr;

  if(n_undefined == data.size()) {
    return "categorical";
  }

  flex_int total = data.apply(vote, flex_type_enum::INTEGER, false).sum();
  double vote_val  = double(total) / (data.size() - n_undefined);

  if(vote_val < -0.5) {
    return "categorical";
  } else if (vote_val > 0.5) {
    return "long_text";
  } else {
    return "short_text";
  }
}


EXPORT bool content_interpretation_valid(gl_sarray data, const flex_string& interpretation) {
  static std::map<flex_type_enum, std::set<flex_string> > valid_interpretations =
      { {flex_type_enum::STRING,    {"categorical", "short_text", "long_text"}},
        {flex_type_enum::FLOAT,     {"categorical", "numerical"}},
        {flex_type_enum::INTEGER,   {"categorical", "numerical"}},
        {flex_type_enum::VECTOR,    {"vector"}},
        {flex_type_enum::DICT,      {"sparse_vector"}},
        {flex_type_enum::LIST,      {"categorical"}},
        {flex_type_enum::IMAGE,     {"image"}},
        {flex_type_enum::DATETIME,  {"timestamp"}},
        {flex_type_enum::UNDEFINED, {"undefined"}} };

  return valid_interpretations.at(data.dtype()).count(interpretation);
}

/** Infers the interpretation of a given content.
 *
 *  Possible interpretations:
 *
 *    short_text: A short phrase or string.
 *    long_text: Interpreted as long or free-form text.
 *    categorical: Should be interpreted as a categorical variable.
 *    sparse_vector: dictionary of (key, value) pairs.
 *    numerical: Numerical column.
 *    vector: Interprets a vector column as a numerical vector.
 *    image: It's an image.
 *    timestamp: It's a timestamp.
 *
 *  If it starts with "undefined:", then the rest is the reason behind it.
 *
 */
EXPORT flex_string infer_content_interpretation(gl_sarray data) {

  flex_string interpretation;

  switch(data.dtype()) {
    case flex_type_enum::STRING: {
      /** Three options -- categorical, text_short, text_long.
       *  Depends on quite a bit.
       */
      interpretation = infer_string_content_interpretation(data);
      break;
    }

    case flex_type_enum::FLOAT: {
      /** Two options -- binary / categorical (if binary) or real.
       */
      auto is_real_categorical = [](const flexible_type& ft) -> flexible_type {

        return (ft.get_type() == flex_type_enum::UNDEFINED
                || ft.get<flex_float>() == 0
                || ft.get<flex_float>() == 1);
      };

      if(data.apply(is_real_categorical, flex_type_enum::INTEGER, false).all()) {
        interpretation = "categorical";
      } else {
        interpretation = "numerical";
      }
      break;
    }

    case flex_type_enum::INTEGER: {
      /** Two options -- binary or categorical or integer.
       */
      auto is_integer_categorical = [](const flexible_type& ft) -> flexible_type {

        return (ft.get_type() == flex_type_enum::UNDEFINED
                || ft.get<flex_int>() == 0
                || ft.get<flex_int>() == 1);
      };

      if(data.apply(is_integer_categorical, flex_type_enum::INTEGER, false).all()) {
        interpretation = "categorical";
      } else {
        interpretation = "numerical";
      }
      break;
    }

    case flex_type_enum::VECTOR: {
      /** If real and all vectors the same length (or 0/undefined for
       *  some), then the vector.
       *
       */
      interpretation = "vector";
      break;
    }

    case flex_type_enum::DICT: {
      /** Ensure that all values are numeric, and all keys are
       *  strings.  But for now, it's just this.
       *
       */
      interpretation = "sparse_vector";
      break;
    }

    case flex_type_enum::LIST: {
      /**  Categorical List
       *
       */
      interpretation = "categorical";
      break;
    }

    case flex_type_enum::IMAGE: {
      interpretation = "image";
      break;
    }

    case flex_type_enum::DATETIME: {
      interpretation = "datetime";
      break;
    }

    case flex_type_enum::UNDEFINED: {
      interpretation = "undefined";
      break;
    }

    case flex_type_enum::ND_VECTOR: {
      log_and_throw(std::string("Flexible type case currently unsupported: ND_VECTOR"));
      ASSERT_UNREACHABLE();
    }

    default: {
      log_and_throw(std::string("Flexible type case not recognized"));
      ASSERT_UNREACHABLE();
    }
  }

  DASSERT_TRUE(content_interpretation_valid(data, interpretation));

  return interpretation;
}

}}
